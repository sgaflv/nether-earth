
/*
 * Android platform backend.
 *
 * No SDL is used here. Java owns the SurfaceView and forwards Android input
 * events; this file owns EGL/OpenGL ES 1.x and the platform key state.
 */
#include "platform.h"
#include "glport.h"

#include <android/native_window.h>
#include <android/keycodes.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define LOG_TAG "NetherEarth"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * EGL is created, used and destroyed exclusively on the game (render) thread.
 * The Java UI thread only publishes surface state and waits for the render
 * thread to acknowledge it:
 *
 *   s_surface_cond  - render thread waits on it for a usable surface.
 *   s_ack_cond      - UI thread waits on it until the render thread has
 *                     actually torn the EGL surface down, so the Surface
 *                     handed back to Android is no longer referenced.
 */
static pthread_cond_t  s_surface_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  s_ack_cond = PTHREAD_COND_INITIALIZER;

/* Owned by the render thread; only it may release the reference. */
static ANativeWindow *s_window = 0;

/* Published by the UI thread, consumed by the render thread. */
static ANativeWindow *s_pending_window = 0;
static int s_have_pending = 0;

static int s_surface_available = 0;
static int s_surface_w = 640;
static int s_surface_h = 480;

static EGLDisplay s_display = EGL_NO_DISPLAY;
static EGLSurface s_surface = EGL_NO_SURFACE;
static EGLContext s_context = EGL_NO_CONTEXT;
static EGLConfig  s_config = 0;
static int s_egl_alive = 0;
static int s_stencil_bits = 0;

/*
 * Android delivers input on the UI thread, asynchronously to the game's
 * frames, so the physical state and the per-frame snapshot the game reads are
 * kept apart:
 *
 *   s_key_down / s_pad_down    - live physical state, written by the UI thread.
 *   s_key_latch / s_pad_latch  - a press that arrived since the last frame.
 *                                Without it, a key pressed and released
 *                                between two frames would never be seen.
 *   s_keyboard / s_pad_buttons - this frame's snapshot (render thread).
 *   s_merged / s_old_merged    - what game code polls, plus the previous
 *                                frame for its own edge detection.
 */
static unsigned char s_key_down[KEY_COUNT];
static unsigned char s_key_latch[KEY_COUNT];
static unsigned char s_pad_down[KEY_COUNT];
static unsigned char s_pad_latch[KEY_COUNT];
static unsigned char s_keyboard[KEY_COUNT];
static unsigned char s_gamepad[KEY_COUNT];
static unsigned char s_merged[KEY_COUNT];
static unsigned char s_old_merged[KEY_COUNT];
static int s_gamepad_connected = 0;

static float s_lx = 0.0f, s_ly = 0.0f;
static float s_rx = 0.0f, s_ry = 0.0f;
static float s_lt = 0.0f, s_rt = 0.0f;

static int s_gpad_up_key = KEY_UP;
static int s_gpad_down_key = KEY_DOWN;
static int s_gpad_left_key = KEY_LEFT;
static int s_gpad_right_key = KEY_RIGHT;
static int s_gpad_fire_key = KEY_SPACE;
static int s_gpad_pause_key = KEY_F1;

static int is_gamepad_key_id(int key)
{
    return key >= KEY_GAMEPAD_A && key <= KEY_GAMEPAD_STICK_RIGHT;
}

static void set_bit(unsigned char *a, int key, int value)
{
    if (key >= 0 && key < KEY_COUNT)
        a[key] = value ? 1 : 0;
}

static void gpad_key(int gamepad_key, int alias_key)
{
    set_bit(s_gamepad, gamepad_key, 1);
    if (alias_key >= 0)
        set_bit(s_gamepad, alias_key, 1);
}

/* This frame's gamepad buttons, from which the key synthesis is rebuilt. */
static unsigned char s_pad_buttons[KEY_COUNT];

static void rebuild_gamepad_locked(void)
{
    memset(s_gamepad, 0, sizeof(s_gamepad));
    if (!s_gamepad_connected)
        return;

    if (s_pad_buttons[KEY_GAMEPAD_A])     gpad_key(KEY_GAMEPAD_A, s_gpad_fire_key);
    if (s_pad_buttons[KEY_GAMEPAD_B])     gpad_key(KEY_GAMEPAD_B, s_gpad_pause_key);
    if (s_pad_buttons[KEY_GAMEPAD_X])     gpad_key(KEY_GAMEPAD_X, -1);
    if (s_pad_buttons[KEY_GAMEPAD_Y])     gpad_key(KEY_GAMEPAD_Y, s_gpad_fire_key);
    if (s_pad_buttons[KEY_GAMEPAD_START]) gpad_key(KEY_GAMEPAD_START, s_gpad_pause_key);
    if (s_pad_buttons[KEY_GAMEPAD_BACK])  gpad_key(KEY_GAMEPAD_BACK, s_gpad_pause_key);
    if (s_pad_buttons[KEY_GAMEPAD_LB])    gpad_key(KEY_GAMEPAD_LB, s_gpad_pause_key);
    if (s_pad_buttons[KEY_GAMEPAD_RB])    gpad_key(KEY_GAMEPAD_RB, KEY_PAGEDOWN);

    if (s_lt > 0.55f) gpad_key(KEY_GAMEPAD_LT, KEY_PAGEUP);
    if (s_rt > 0.55f) gpad_key(KEY_GAMEPAD_RT, KEY_PAGEDOWN);

    if (s_pad_buttons[KEY_GAMEPAD_DPAD_UP])    gpad_key(KEY_GAMEPAD_DPAD_UP, s_gpad_up_key);
    if (s_pad_buttons[KEY_GAMEPAD_DPAD_DOWN])  gpad_key(KEY_GAMEPAD_DPAD_DOWN, s_gpad_down_key);
    if (s_pad_buttons[KEY_GAMEPAD_DPAD_LEFT])  gpad_key(KEY_GAMEPAD_DPAD_LEFT, s_gpad_left_key);
    if (s_pad_buttons[KEY_GAMEPAD_DPAD_RIGHT]) gpad_key(KEY_GAMEPAD_DPAD_RIGHT, s_gpad_right_key);

    if (s_ly < -0.35f) gpad_key(KEY_GAMEPAD_STICK_UP, s_gpad_up_key);
    if (s_ly >  0.35f) gpad_key(KEY_GAMEPAD_STICK_DOWN, s_gpad_down_key);
    if (s_lx < -0.35f) gpad_key(KEY_GAMEPAD_STICK_LEFT, s_gpad_left_key);
    if (s_lx >  0.35f) gpad_key(KEY_GAMEPAD_STICK_RIGHT, s_gpad_right_key);

    if (s_ry < -0.35f) gpad_key(KEY_GAMEPAD_STICK_UP, KEY_PAGEDOWN);
    if (s_ry >  0.35f) gpad_key(KEY_GAMEPAD_STICK_DOWN, KEY_PAGEUP);
}

static int android_key_to_key(int k)
{
    switch (k) {
    case AKEYCODE_BACK:        return KEY_GAMEPAD_BACK;
    case AKEYCODE_ESCAPE:      return KEY_ESCAPE;
    case AKEYCODE_ENTER:       return KEY_RETURN;
    case AKEYCODE_DPAD_UP:     return KEY_UP;
    case AKEYCODE_DPAD_DOWN:   return KEY_DOWN;
    case AKEYCODE_DPAD_LEFT:   return KEY_LEFT;
    case AKEYCODE_DPAD_RIGHT:  return KEY_RIGHT;
    case AKEYCODE_DPAD_CENTER: return KEY_SPACE;
    case AKEYCODE_BUTTON_A:    return KEY_GAMEPAD_A;
    case AKEYCODE_BUTTON_B:    return KEY_GAMEPAD_B;
    case AKEYCODE_BUTTON_X:    return KEY_GAMEPAD_X;
    case AKEYCODE_BUTTON_Y:    return KEY_GAMEPAD_Y;
    case AKEYCODE_BUTTON_L1:   return KEY_GAMEPAD_LB;
    case AKEYCODE_BUTTON_R1:   return KEY_GAMEPAD_RB;
    case AKEYCODE_BUTTON_L2:   return KEY_GAMEPAD_LT;
    case AKEYCODE_BUTTON_R2:   return KEY_GAMEPAD_RT;
    case AKEYCODE_BUTTON_START:return KEY_GAMEPAD_START;
    case AKEYCODE_BUTTON_SELECT:return KEY_GAMEPAD_BACK;
    case AKEYCODE_BUTTON_MODE: return KEY_GAMEPAD_START;
    case AKEYCODE_SPACE:       return KEY_SPACE;
    case AKEYCODE_TAB:         return KEY_TAB;
    case AKEYCODE_DEL:         return KEY_BACKSPACE;
    case AKEYCODE_PAGE_UP:     return KEY_PAGEUP;
    case AKEYCODE_PAGE_DOWN:   return KEY_PAGEDOWN;
    case AKEYCODE_HOME:        return KEY_HOME;
    case AKEYCODE_MOVE_HOME:   return KEY_HOME;
    case AKEYCODE_MOVE_END:    return KEY_END;
    case AKEYCODE_INSERT:      return KEY_INSERT;
    case AKEYCODE_FORWARD_DEL: return KEY_DELETE;
    case AKEYCODE_SHIFT_LEFT:  return KEY_LSHIFT;
    case AKEYCODE_SHIFT_RIGHT: return KEY_RSHIFT;
    case AKEYCODE_CTRL_LEFT:   return KEY_LCTRL;
    case AKEYCODE_CTRL_RIGHT:  return KEY_RCTRL;
    case AKEYCODE_ALT_LEFT:    return KEY_LALT;
    case AKEYCODE_ALT_RIGHT:   return KEY_RALT;
    case AKEYCODE_F1: return KEY_F1; case AKEYCODE_F2: return KEY_F2;
    case AKEYCODE_F3: return KEY_F3; case AKEYCODE_F4: return KEY_F4;
    case AKEYCODE_F5: return KEY_F5; case AKEYCODE_F6: return KEY_F6;
    case AKEYCODE_F7: return KEY_F7; case AKEYCODE_F8: return KEY_F8;
    case AKEYCODE_F9: return KEY_F9; case AKEYCODE_F10:return KEY_F10;
    case AKEYCODE_F11:return KEY_F11; case AKEYCODE_F12:return KEY_F12;
    default: break;
    }
    if (k >= AKEYCODE_A && k <= AKEYCODE_Z)
        return KEY_A + (k - AKEYCODE_A);
    if (k >= AKEYCODE_0 && k <= AKEYCODE_9)
        return KEY_0 + (k - AKEYCODE_0);
    return KEY_UNKNOWN;
}

/*
 * Display, config and GL context. Created once and then kept: the EGL
 * *surface* comes and goes with the SurfaceView, but tearing the context down
 * with it would drop every texture the game has uploaded, and the game has no
 * way of knowing it should reload them.
 */
static int ensure_context_locked(void)
{
    /*
     * The game draws its drop shadows through the stencil buffer, so a
     * stencil-capable config is requested first; only if the driver has none
     * do we fall back to a plain config (platform_stencil_bits() then reports
     * 0 and the caller turns shadows off).
     */
    EGLint attrs_stencil[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16, EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    EGLint attrs_plain[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLint ctxattrs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
    EGLint n = 0;

    if (s_context != EGL_NO_CONTEXT)
        return 1;

    if (s_display == EGL_NO_DISPLAY) {
        s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (s_display == EGL_NO_DISPLAY) {
            LOGE("eglGetDisplay failed");
            return 0;
        }
        /* Android reference-counts this against eglTerminate(). */
        if (!eglInitialize(s_display, 0, 0)) {
            LOGE("eglInitialize failed");
            s_display = EGL_NO_DISPLAY;
            return 0;
        }
    }

    if (!eglChooseConfig(s_display, attrs_stencil, &s_config, 1, &n) || n == 0) {
        LOGE("No stencil-capable EGL config; shadows will be disabled");
        n = 0;
        if (!eglChooseConfig(s_display, attrs_plain, &s_config, 1, &n) || n == 0) {
            LOGE("No usable OpenGL ES 1.x EGL config");
            return 0;
        }
    }

    s_stencil_bits = 0;
    eglGetConfigAttrib(s_display, s_config, EGL_STENCIL_SIZE, &s_stencil_bits);

    s_context = eglCreateContext(s_display, s_config, EGL_NO_CONTEXT, ctxattrs);
    if (s_context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return 0;
    }

    return 1;
}

static void destroy_surface_locked(void)
{
    if (s_display == EGL_NO_DISPLAY)
        return;

    eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (s_surface != EGL_NO_SURFACE) {
        eglDestroySurface(s_display, s_surface);
        s_surface = EGL_NO_SURFACE;
    }

    s_egl_alive = 0;
}

static int create_surface_locked(void)
{
    if (!s_window || !ensure_context_locked())
        return 0;

    s_surface = eglCreateWindowSurface(s_display, s_config, s_window, 0);
    if (s_surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed (0x%x)", eglGetError());
        return 0;
    }

    if (!eglMakeCurrent(s_display, s_surface, s_surface, s_context)) {
        EGLint err = eglGetError();

        eglDestroySurface(s_display, s_surface);
        s_surface = EGL_NO_SURFACE;

        if (err == EGL_CONTEXT_LOST) {
            /* GPU reset. The textures are gone with the context and the game
               cannot be told to re-upload them, so say so loudly instead of
               drawing garbage. */
            LOGE("EGL context lost; restart the app to reload the textures");
            eglDestroyContext(s_display, s_context);
            s_context = EGL_NO_CONTEXT;
        } else {
            LOGE("eglMakeCurrent failed (0x%x)", err);
        }
        return 0;
    }

    /* Tie the frame rate to the display: the game logic runs on its own
       20 ms clock, so there is nothing to gain from drawing faster. */
    eglSwapInterval(s_display, 1);

    {
        EGLint w = 0, h = 0;
        eglQuerySurface(s_display, s_surface, EGL_WIDTH, &w);
        eglQuerySurface(s_display, s_surface, EGL_HEIGHT, &h);
        if (w > 0 && h > 0) { s_surface_w = w; s_surface_h = h; }
    }

    s_egl_alive = 1;
    return 1;
}

/* Full teardown, context included: only for platform_destroy_window(). */
static void destroy_egl_locked(void)
{
    destroy_surface_locked();

    if (s_display != EGL_NO_DISPLAY) {
        if (s_context != EGL_NO_CONTEXT)
            eglDestroyContext(s_display, s_context);
        eglTerminate(s_display);
    }

    s_display = EGL_NO_DISPLAY;
    s_context = EGL_NO_CONTEXT;
    s_config = 0;
}

/*
 * Bring EGL in line with the surface state published by the UI thread.
 * Render thread only; the caller must hold s_mutex.
 */
static void sync_egl_locked(void)
{
    if (s_have_pending) {
        destroy_surface_locked();
        if (s_window)
            ANativeWindow_release(s_window);
        s_window = s_pending_window;
        s_pending_window = 0;
        s_have_pending = 0;
    }

    if (!s_surface_available) {
        destroy_surface_locked();
        if (s_window) {
            ANativeWindow_release(s_window);
            s_window = 0;
        }
    } else if (!s_egl_alive && s_window) {
        create_surface_locked();
    }

    /* Let a UI thread blocked in surfaceDestroyed() continue. */
    pthread_cond_broadcast(&s_ack_cond);
}

void android_platform_surface_created(ANativeWindow *window)
{
    pthread_mutex_lock(&s_mutex);

    /* A window published but not yet picked up by the render thread would
       otherwise leak its reference. */
    if (s_have_pending && s_pending_window)
        ANativeWindow_release(s_pending_window);

    s_pending_window = window;
    s_have_pending = 1;
    s_surface_available = (window != 0);

    if (window) {
        s_surface_w = ANativeWindow_getWidth(window);
        s_surface_h = ANativeWindow_getHeight(window);
    }

    pthread_cond_broadcast(&s_surface_cond);
    pthread_mutex_unlock(&s_mutex);
}

void android_platform_surface_destroyed(void)
{
    struct timespec deadline;

    pthread_mutex_lock(&s_mutex);
    s_surface_available = 0;
    pthread_cond_broadcast(&s_surface_cond);

    /*
     * Android may recycle the Surface as soon as this returns, so wait for
     * the render thread to drop its EGL surface and window reference. The
     * wait is bounded: a render thread stuck in a long asset load must never
     * turn into an ANR on the UI thread.
     */
    /* CLOCK_REALTIME: that is the clock PTHREAD_COND_INITIALIZER uses. */
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;

    while (s_egl_alive || s_window || s_have_pending) {
        if (pthread_cond_timedwait(&s_ack_cond, &s_mutex, &deadline) == ETIMEDOUT) {
            LOGE("Timed out waiting for the render thread to release the surface");
            break;
        }
    }

    pthread_mutex_unlock(&s_mutex);
}

void android_platform_surface_size_changed(int w, int h)
{
    pthread_mutex_lock(&s_mutex);
    s_surface_w = w;
    s_surface_h = h;
    pthread_mutex_unlock(&s_mutex);
}

int android_platform_key(int android_keycode, int down, int gamepad)
{
    int key = android_key_to_key(android_keycode);
    if (key == KEY_UNKNOWN)
        return 0;

    pthread_mutex_lock(&s_mutex);
    if (gamepad || is_gamepad_key_id(key)) {
        s_gamepad_connected = 1;
        if (key == KEY_UP) key = KEY_GAMEPAD_DPAD_UP;
        else if (key == KEY_DOWN) key = KEY_GAMEPAD_DPAD_DOWN;
        else if (key == KEY_LEFT) key = KEY_GAMEPAD_DPAD_LEFT;
        else if (key == KEY_RIGHT) key = KEY_GAMEPAD_DPAD_RIGHT;
        set_bit(s_pad_down, key, down);
        if (down) set_bit(s_pad_latch, key, 1);
    } else {
        set_bit(s_key_down, key, down);
        if (down) set_bit(s_key_latch, key, 1);
    }
    pthread_mutex_unlock(&s_mutex);

    return 1;
}

void android_platform_motion(float lx, float ly, float rx, float ry, float lt, float rt, int gamepad)
{
    pthread_mutex_lock(&s_mutex);
    if (gamepad) s_gamepad_connected = 1;
    s_lx = lx; s_ly = ly; s_rx = rx; s_ry = ry; s_lt = lt; s_rt = rt;
    pthread_mutex_unlock(&s_mutex);
}

unsigned int platform_ticks(void)
{
    static struct timespec base;
    static int initialized = 0;
    struct timespec now;
    if (!initialized) { clock_gettime(CLOCK_MONOTONIC, &base); initialized = 1; }
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (unsigned int)((now.tv_sec-base.tv_sec)*1000u +
                          (now.tv_nsec-base.tv_nsec)/1000000u);
}

void platform_sleep(unsigned int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, 0);
}

int platform_init(void)
{
    pthread_mutex_lock(&s_mutex);
    memset(s_key_down,0,sizeof(s_key_down));
    memset(s_key_latch,0,sizeof(s_key_latch));
    memset(s_pad_down,0,sizeof(s_pad_down));
    memset(s_pad_latch,0,sizeof(s_pad_latch));
    memset(s_keyboard,0,sizeof(s_keyboard));
    memset(s_gamepad,0,sizeof(s_gamepad));
    memset(s_pad_buttons,0,sizeof(s_pad_buttons));
    memset(s_merged,0,sizeof(s_merged));
    memset(s_old_merged,0,sizeof(s_old_merged));
    s_gamepad_connected = 0;
    pthread_mutex_unlock(&s_mutex);
    return 1;
}

void platform_shutdown(void)
{
    platform_destroy_window();
}

/*
 * The window size and fullscreen flag are meaningless here: the SurfaceView
 * always covers the whole (fullscreen, immersive) TV screen. This blocks
 * until Java has handed over a Surface, which is why the game runs on its own
 * thread rather than the UI thread.
 */
int platform_create_window(int w, int h, int fullscreen)
{
    int ok;

    (void)w; (void)h; (void)fullscreen;

    pthread_mutex_lock(&s_mutex);

    while (!s_egl_alive) {
        sync_egl_locked();

        if (s_egl_alive)
            break;

        /* A surface is there and EGL still would not take it: a real
           failure, as opposed to simply not having a surface yet. */
        if (s_surface_available && s_window)
            break;

        pthread_cond_wait(&s_surface_cond, &s_mutex);
    }

    ok = s_egl_alive;
    pthread_mutex_unlock(&s_mutex);

    return ok;
}

void platform_destroy_window(void)
{
    pthread_mutex_lock(&s_mutex);
    destroy_egl_locked();
    pthread_cond_broadcast(&s_ack_cond);
    pthread_mutex_unlock(&s_mutex);
}

void platform_swap_buffers(void)
{
    pthread_mutex_lock(&s_mutex);

    sync_egl_locked();

    /*
     * While the app is in the background there is no surface to draw on.
     * Park the game thread here instead of spinning through frames that go
     * nowhere; platform_ticks() keeps running and the main loop's catch-up
     * clamp absorbs the gap on resume.
     */
    while (!s_surface_available) {
        pthread_cond_wait(&s_surface_cond, &s_mutex);
        sync_egl_locked();
    }

    if (!s_egl_alive) {
        /*
         * A surface exists but EGL would not take it. Nothing blocks the
         * game loop then (eglSwapBuffers is what normally paces it against
         * vsync), so throttle here rather than spin a core flat out.
         */
        pthread_mutex_unlock(&s_mutex);
        platform_sleep(16);
        return;
    }

    if (eglSwapBuffers(s_display, s_surface) == EGL_FALSE) {
        EGLint err = eglGetError();

        /* The window went away without us being told yet. Drop the surface
           (not the context) and let the next sync rebuild it. */
        if (err == EGL_BAD_SURFACE || err == EGL_BAD_NATIVE_WINDOW) {
            LOGE("eglSwapBuffers lost the surface (0x%x)", err);
            destroy_surface_locked();
            pthread_cond_broadcast(&s_ack_cond);
        }
    }

    pthread_mutex_unlock(&s_mutex);
}

int platform_stencil_bits(void)
{
    int bits;
    pthread_mutex_lock(&s_mutex);
    bits = s_stencil_bits;
    pthread_mutex_unlock(&s_mutex);
    return bits;
}

void platform_get_drawable_size(int *w, int *h)
{
    pthread_mutex_lock(&s_mutex);
    *w = s_surface_w > 0 ? s_surface_w : 640;
    *h = s_surface_h > 0 ? s_surface_h : 480;
    pthread_mutex_unlock(&s_mutex);
}

void platform_show_cursor(int show) { (void)show; }

void platform_log_gl_info(void)
{
    const GLubyte *v = glGetString(GL_VERSION);
    const GLubyte *r = glGetString(GL_RENDERER);
    const GLubyte *g = glGetString(GL_VENDOR);
    LOGI("GL version: %s", v ? (const char*)v : "(null)");
    LOGI("GL renderer: %s", r ? (const char*)r : "(null)");
    LOGI("GL vendor: %s", g ? (const char*)g : "(null)");
}

unsigned int platform_num_keys(void) { return KEY_COUNT; }

void platform_pump_input(void)
{
    int i;

    pthread_mutex_lock(&s_mutex);

    /* Genuinely last frame's state: recomputing it from the live physical
       state here would make it identical to s_merged below, and every
       "pressed this frame" test in the game would be dead. */
    memcpy(s_old_merged, s_merged, sizeof(s_old_merged));

    for (i=0; i<KEY_COUNT; ++i) {
        s_keyboard[i]    = (s_key_down[i] || s_key_latch[i]) ? 1 : 0;
        s_pad_buttons[i] = (s_pad_down[i] || s_pad_latch[i]) ? 1 : 0;
        s_key_latch[i] = 0;
        s_pad_latch[i] = 0;
    }

    rebuild_gamepad_locked();

    for (i=0; i<KEY_COUNT; ++i)
        s_merged[i] = (s_keyboard[i] || s_gamepad[i]) ? 1 : 0;

    pthread_mutex_unlock(&s_mutex);
}

const unsigned char *platform_get_keyboard_state(void) { return s_merged; }
const unsigned char *platform_get_old_keyboard_state(void) { return s_old_merged; }
/*
 * There is no window to close on Android. Leaving the app (HOME, or the app
 * being swapped out) suspends the game in platform_swap_buffers() instead of
 * ending it, and the Activity kills the process when it really is finishing.
 */
int platform_quit_requested(void) { return 0; }

int platform_key_pressed_raw(int key)
{
    int r = 0;
    if (key < 0 || key >= KEY_COUNT) return 0;
    pthread_mutex_lock(&s_mutex);
    r = s_key_down[key] != 0;
    pthread_mutex_unlock(&s_mutex);
    return r;
}

const char *platform_key_name(int key)
{
    static char name[32];
    if (key >= KEY_A && key <= KEY_Z) {
        name[0] = (char)('A' + key - KEY_A); name[1] = 0; return name;
    }
    if (key >= KEY_0 && key <= KEY_9) {
        name[0] = (char)('0' + key - KEY_0); name[1] = 0; return name;
    }
    switch(key) {
    case KEY_SPACE: return "SPACE"; case KEY_RETURN:return "RETURN";
    case KEY_ESCAPE:return "ESC"; case KEY_UP:return "UP"; case KEY_DOWN:return "DOWN";
    case KEY_LEFT:return "LEFT"; case KEY_RIGHT:return "RIGHT";
    case KEY_F1:return "F1"; case KEY_F12:return "F12";
    case KEY_GAMEPAD_A:return "PAD_A"; case KEY_GAMEPAD_B:return "PAD_B";
    case KEY_GAMEPAD_X:return "PAD_X"; case KEY_GAMEPAD_Y:return "PAD_Y";
    case KEY_GAMEPAD_START:return "PAD_START"; case KEY_GAMEPAD_BACK:return "PAD_BACK";
    case KEY_GAMEPAD_LB:return "PAD_LB"; case KEY_GAMEPAD_RB:return "PAD_RB";
    case KEY_GAMEPAD_LT:return "PAD_LT"; case KEY_GAMEPAD_RT:return "PAD_RT";
    case KEY_GAMEPAD_DPAD_UP:return "PAD_UP"; case KEY_GAMEPAD_DPAD_DOWN:return "PAD_DOWN";
    case KEY_GAMEPAD_DPAD_LEFT:return "PAD_LEFT"; case KEY_GAMEPAD_DPAD_RIGHT:return "PAD_RIGHT";
    case KEY_GAMEPAD_STICK_UP:return "STICK_UP"; case KEY_GAMEPAD_STICK_DOWN:return "STICK_DOWN";
    case KEY_GAMEPAD_STICK_LEFT:return "STICK_LEFT"; case KEY_GAMEPAD_STICK_RIGHT:return "STICK_RIGHT";
    default: break;
    }
    snprintf(name,sizeof(name),"KEY_%d",key);
    return name;
}

int platform_gamepad_connected(void)
{
    int r;
    pthread_mutex_lock(&s_mutex); r=s_gamepad_connected; pthread_mutex_unlock(&s_mutex);
    return r;
}

void platform_gamepad_name(char *buf, int buflen)
{
    if (!buf || buflen <= 0) return;
    snprintf(buf, buflen, "%s", platform_gamepad_connected() ? "Android Gamepad" : "(none)");
}

void platform_set_gamepad_mapping(int up,int down,int left,int right,int fire,int pause_key)
{
    pthread_mutex_lock(&s_mutex);
    if (!is_gamepad_key_id(up) && up>=0 && up<KEY_COUNT) s_gpad_up_key=up;
    if (!is_gamepad_key_id(down) && down>=0 && down<KEY_COUNT) s_gpad_down_key=down;
    if (!is_gamepad_key_id(left) && left>=0 && left<KEY_COUNT) s_gpad_left_key=left;
    if (!is_gamepad_key_id(right) && right>=0 && right<KEY_COUNT) s_gpad_right_key=right;
    if (!is_gamepad_key_id(fire) && fire>=0 && fire<KEY_COUNT) s_gpad_fire_key=fire;
    if (!is_gamepad_key_id(pause_key) && pause_key>=0 && pause_key<KEY_COUNT) s_gpad_pause_key=pause_key;
    pthread_mutex_unlock(&s_mutex);
}
