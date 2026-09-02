package com.example.nether;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Build;
import android.os.Bundle;
import android.os.Process;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import java.util.HashMap;
import java.util.Map;

/**
 * The whole Android front end: one Activity owning a SurfaceView, forwarding
 * key/joystick events to the native platform layer, and lending Java's
 * SoundPool to the native audio backend. There is no SDL involved, and the
 * game itself runs on a native thread (see android_bridge.cpp).
 */
public final class MainActivity extends Activity implements SurfaceHolder.Callback {

    private GameSurface surface;

    private static volatile MainActivity instance;

    /* Owned by the process, not the Activity: the native game thread calls
       into these from off the UI thread and outlives Activity recreation. */
    private static AssetManager assets;
    private static SoundPool soundPool;
    private static final Map<Integer, Boolean> soundReady = new HashMap<Integer, Boolean>();

    static {
        System.loadLibrary("nether");
    }

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);

        instance = this;

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        /* The APK's assets are read-only, so nether.cfg and the save games go
           to the app's private storage instead. */
        nativeSetUserDir(getFilesDir().getAbsolutePath());

        assets = getApplicationContext().getAssets();
        nativeSetAssetManager(assets);

        surface = new GameSurface();
        surface.getHolder().addCallback(this);
        surface.setFocusable(true);
        surface.setFocusableInTouchMode(true);
        setContentView(surface);
        surface.requestFocus();

        /* Only after setContentView: hiding the system bars goes through the
           window's DecorView, which does not exist before that. */
        hideSystemUi();

        initAudio();

        /* No-op if the game thread is already running, which is the case when
           the Activity is merely being recreated: it just picks up the new
           Surface. The native side waits for one before touching EGL. */
        nativeStartGame();
    }

    @Override protected void onDestroy() {
        super.onDestroy();

        if (instance == this) {
            instance = null;
        }

        /* The game keeps process-wide global state and cannot be restarted in
           place, so a finishing Activity takes the process with it. */
        if (isFinishing()) {
            Process.killProcess(Process.myPid());
        }
    }

    /* ------------------------------------------------------------------ */
    /* Fullscreen                                                         */
    /* ------------------------------------------------------------------ */

    private void hideSystemUi() {
        /*
         * Going fullscreen is cosmetic, but the framework paths behind it are
         * not uniformly null-safe across TV builds (PhoneWindow.
         * getInsetsController() dereferences the DecorView unchecked on some
         * Android 11 firmware). A game that will not start at all is a far
         * worse outcome than visible system bars.
         */
        try {
            hideSystemUiUnchecked();
        } catch (Throwable t) {
            android.util.Log.w("NetherEarth", "Could not hide the system bars", t);
        }
    }

    private void hideSystemUiUnchecked() {
        if (Build.VERSION.SDK_INT >= 30) {
            /* Forces installDecor(); getInsetsController() needs it. */
            getWindow().getDecorView();

            getWindow().setDecorFitsSystemWindows(false);
            WindowInsetsController c = getWindow().getInsetsController();
            if (c != null) {
                c.hide(WindowInsets.Type.systemBars());
                c.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                ViewFlags.IMMERSIVE_STICKY |
                ViewFlags.FULLSCREEN |
                ViewFlags.HIDE_NAVIGATION |
                ViewFlags.LAYOUT_FULLSCREEN |
                ViewFlags.LAYOUT_HIDE_NAVIGATION |
                ViewFlags.LAYOUT_STABLE);
        }
    }

    @Override public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    /* ------------------------------------------------------------------ */
    /* Surface                                                            */
    /* ------------------------------------------------------------------ */

    @Override public void surfaceCreated(SurfaceHolder holder) {
        nativeSurfaceCreated(holder.getSurface());
    }

    @Override public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        nativeSurfaceChanged(w, h);
    }

    @Override public void surfaceDestroyed(SurfaceHolder holder) {
        /* Blocks until the native render thread has released the Surface. */
        nativeSurfaceDestroyed();
    }

    /* ------------------------------------------------------------------ */
    /* Input                                                              */
    /* ------------------------------------------------------------------ */

    private static boolean isGamepadSource(int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
               (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
               (source & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
    }

    private final class GameSurface extends SurfaceView {
        GameSurface() { super(MainActivity.this); }

        @Override public boolean dispatchKeyEvent(KeyEvent e) {
            if (e.getAction() == KeyEvent.ACTION_DOWN ||
                e.getAction() == KeyEvent.ACTION_UP) {

                boolean gp = isGamepadSource(e.getSource());

                /* Only report the event as handled if the game actually binds
                   that key, so volume and the other system keys on a TV
                   remote keep working. */
                if (nativeKey(e.getKeyCode(),
                              e.getAction() == KeyEvent.ACTION_DOWN,
                              gp)) {
                    return true;
                }
            }
            return super.dispatchKeyEvent(e);
        }

        @Override public boolean dispatchGenericMotionEvent(MotionEvent e) {
            if ((e.getSource() & InputDevice.SOURCE_JOYSTICK) ==
                    InputDevice.SOURCE_JOYSTICK) {

                float lx = e.getAxisValue(MotionEvent.AXIS_X);
                float ly = e.getAxisValue(MotionEvent.AXIS_Y);
                float rx = e.getAxisValue(MotionEvent.AXIS_Z);
                float ry = e.getAxisValue(MotionEvent.AXIS_RZ);
                float lt = e.getAxisValue(MotionEvent.AXIS_LTRIGGER);
                float rt = e.getAxisValue(MotionEvent.AXIS_RTRIGGER);

                /* Some Android TV controllers expose triggers as GAS/BRAKE. */
                if (lt == 0.0f) lt = e.getAxisValue(MotionEvent.AXIS_BRAKE);
                if (rt == 0.0f) rt = e.getAxisValue(MotionEvent.AXIS_GAS);

                nativeMotion(lx, ly, rx, ry, lt, rt, true);

                /* D-pad exposed as a hat switch (motion axes) on this
                   controller family, not as AKEYCODE_DPAD_* key events. */
                nativeHat(e.getAxisValue(MotionEvent.AXIS_HAT_X),
                          e.getAxisValue(MotionEvent.AXIS_HAT_Y));
                return true;
            }
            return super.dispatchGenericMotionEvent(e);
        }
    }

    /* ------------------------------------------------------------------ */
    /* Audio - called from the native game thread                          */
    /* ------------------------------------------------------------------ */

    private static synchronized void initAudio() {
        if (soundPool != null) return;

        AudioAttributes aa = new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build();

        soundPool = new SoundPool.Builder()
            .setAudioAttributes(aa)
            .setMaxStreams(8)
            .build();

        soundPool.setOnLoadCompleteListener(new SoundPool.OnLoadCompleteListener() {
            @Override public void onLoadComplete(SoundPool pool, int sampleId, int status) {
                synchronized (soundReady) {
                    soundReady.put(sampleId, status == 0);
                }
            }
        });
    }

    public static int audioLoad(String path) {
        initAudio();

        AssetManager am = assets;
        if (am == null) return -1;

        while (path.startsWith("assets/")) path = path.substring(7);

        AssetFileDescriptor afd = null;
        try {
            /* Requires the WAVs to be stored uncompressed; see the
               androidResources.noCompress entry in app/build.gradle. */
            afd = am.openFd(path);
            return soundPool.load(afd, 1);
        } catch (Exception ex) {
            return -1;
        } finally {
            if (afd != null) {
                try { afd.close(); } catch (Exception ignored) { }
            }
        }
    }

    public static void audioPlay(int id, int volume) {
        SoundPool pool = soundPool;
        if (pool == null || id < 0) return;

        /* SoundPool decodes asynchronously; playing before that finishes is
           an error, so early shots are simply dropped. */
        synchronized (soundReady) {
            if (!Boolean.TRUE.equals(soundReady.get(id))) return;
        }

        float v = Math.max(0, Math.min(128, volume)) / 128.0f;
        pool.play(id, v, v, 1, 0, 1.0f);
    }

    public static void audioFree(int id) {
        SoundPool pool = soundPool;
        if (pool == null || id < 0) return;

        pool.unload(id);
        synchronized (soundReady) { soundReady.remove(id); }
    }

    public static synchronized void audioShutdown() {
        if (soundPool == null) return;

        soundPool.release();
        soundPool = null;
        synchronized (soundReady) { soundReady.clear(); }
    }

    /* ------------------------------------------------------------------ */
    /* Shutdown - called from the native game thread                       */
    /* ------------------------------------------------------------------ */

    /** The player chose EXIT GAME: close the task and end the process. */
    public static void onNativeExit() {
        final MainActivity a = instance;

        if (a == null) {
            Process.killProcess(Process.myPid());
            return;
        }

        a.runOnUiThread(new Runnable() {
            @Override public void run() {
                a.finishAndRemoveTask();
                Process.killProcess(Process.myPid());
            }
        });
    }

    /* setSystemUiVisibility() constants, inlined because the fields
       themselves are deprecated on the compile SDK. */
    private static final class ViewFlags {
        static final int IMMERSIVE_STICKY        = 0x00001000;
        static final int FULLSCREEN              = 0x00000004;
        static final int HIDE_NAVIGATION         = 0x00000002;
        static final int LAYOUT_FULLSCREEN       = 0x00000400;
        static final int LAYOUT_HIDE_NAVIGATION  = 0x00000200;
        static final int LAYOUT_STABLE           = 0x00000100;
    }

    private static native void nativeSetAssetManager(AssetManager manager);
    private static native void nativeSetUserDir(String dir);
    private static native void nativeSurfaceCreated(Surface surface);
    private static native void nativeSurfaceDestroyed();
    private static native void nativeSurfaceChanged(int w, int h);
    private static native boolean nativeKey(int keyCode, boolean down, boolean gamepad);
    private static native void nativeMotion(float lx, float ly, float rx, float ry,
                                            float lt, float rt, boolean gamepad);
    private static native void nativeHat(float hx, float hy);
    private static native void nativeStartGame();
}
