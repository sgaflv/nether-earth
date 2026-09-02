
#include "android_bridge.h"
#include "assets.h"
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "NetherEarth"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

extern void android_platform_surface_created(ANativeWindow*);
extern void android_platform_surface_destroyed(void);
extern void android_platform_surface_size_changed(int,int);
extern int  android_platform_key(int,int,int);
extern void android_platform_motion(float,float,float,float,float,float,int);

static JavaVM *s_vm=0;
static jclass s_activity=0;
static jmethodID s_audio_load=0, s_audio_play=0, s_audio_free=0, s_audio_shutdown=0;
static jmethodID s_on_native_exit=0;

/* The game keeps global state, so exactly one game thread per process. */
static pthread_mutex_t s_start_mutex=PTHREAD_MUTEX_INITIALIZER;
static int s_game_started=0;

static JNIEnv *get_env(int *attached)
{
    *attached=0;
    if(!s_vm) return 0;
    JNIEnv *env=0;
    if(s_vm->GetEnv((void**)&env,JNI_VERSION_1_6)==JNI_OK) return env;
    if(s_vm->AttachCurrentThread(&env,0)!=JNI_OK) return 0;
    *attached=1;
    return env;
}

static void release_env(int attached)
{
    if(attached && s_vm) s_vm->DetachCurrentThread();
}

extern "C" int android_audio_load(const char *path)
{
    int attached=0; JNIEnv *env=get_env(&attached);
    if(!env || !s_audio_load) return -1;
    while(path && strncmp(path,"assets/",7)==0) path+=7;
    jstring p=env->NewStringUTF(path ? path : "");
    jint id=env->CallStaticIntMethod(s_activity,s_audio_load,p);
    env->DeleteLocalRef(p);
    release_env(attached);
    return (int)id;
}

extern "C" void android_audio_play(int sound_id,int volume)
{
    int attached=0; JNIEnv *env=get_env(&attached);
    if(!env || !s_audio_play) return;
    env->CallStaticVoidMethod(s_activity,s_audio_play,(jint)sound_id,(jint)volume);
    release_env(attached);
}

extern "C" void android_audio_free(int sound_id)
{
    int attached=0; JNIEnv *env=get_env(&attached);
    if(!env || !s_audio_free) return;
    env->CallStaticVoidMethod(s_activity,s_audio_free,(jint)sound_id);
    release_env(attached);
}

extern "C" void android_audio_shutdown(void)
{
    int attached=0; JNIEnv *env=get_env(&attached);
    if(!env || !s_audio_shutdown) return;
    env->CallStaticVoidMethod(s_activity,s_audio_shutdown);
    release_env(attached);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void*)
{
    s_vm=vm;
    JNIEnv *env=0;
    if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK) return JNI_ERR;

    jclass local=env->FindClass("com/example/nether/MainActivity");
    if(!local) return JNI_ERR;
    s_activity=(jclass)env->NewGlobalRef(local);
    env->DeleteLocalRef(local);

    s_audio_load=env->GetStaticMethodID(s_activity,"audioLoad","(Ljava/lang/String;)I");
    s_audio_play=env->GetStaticMethodID(s_activity,"audioPlay","(II)V");
    s_audio_free=env->GetStaticMethodID(s_activity,"audioFree","(I)V");
    s_audio_shutdown=env->GetStaticMethodID(s_activity,"audioShutdown","()V");
    s_on_native_exit=env->GetStaticMethodID(s_activity,"onNativeExit","()V");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeSetAssetManager(JNIEnv *env,jobject,jobject mgr)
{
    AAssetManager *am=AAssetManager_fromJava(env,mgr);
    android_assets_set_manager(am);
}

/*
 * Writable application storage (Context.getFilesDir()). The APK's assets are
 * read-only, so nether.cfg and the save games have to live here.
 */
extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeSetUserDir(JNIEnv *env,jobject,jstring dir)
{
    if(!dir) return;
    const char *utf=env->GetStringUTFChars(dir,0);
    if(utf) {
        user_set_dir(utf);
        env->ReleaseStringUTFChars(dir,utf);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeSurfaceCreated(JNIEnv *env,jobject,jobject surface)
{
    ANativeWindow *w=ANativeWindow_fromSurface(env,surface);
    android_platform_surface_created(w);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeSurfaceDestroyed(JNIEnv *,jobject)
{
    android_platform_surface_destroyed();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeSurfaceChanged(JNIEnv *,jobject,jint w,jint h)
{
    android_platform_surface_size_changed((int)w,(int)h);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_nether_MainActivity_nativeKey(JNIEnv *,jobject,jint key,jboolean down,jboolean gamepad)
{
    return android_platform_key((int)key,down?1:0,gamepad?1:0) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeMotion(JNIEnv *,jobject,jfloat lx,jfloat ly,jfloat rx,jfloat ry,jfloat lt,jfloat rt,jboolean gamepad)
{
    android_platform_motion(lx,ly,rx,ry,lt,rt,gamepad?1:0);
}

/*
 * The game runs on its own thread so the Java UI thread stays free for
 * Surface callbacks and input dispatch; game_main() blocks until Java hands
 * over a Surface (see platform_create_window in platform_android.cpp).
 */
extern "C" int game_main(int argc,char **argv);

static void *game_thread(void*)
{
    char arg0[]="nether_earth";
    char *argv[]={arg0,0};
    JNIEnv *env=0;

    /*
     * Attach once for the whole life of the thread. The audio calls below
     * happen per sound effect, and attaching/detaching around each one would
     * make playing a shot far more expensive than the sound itself.
     */
    if(s_vm && s_vm->AttachCurrentThread(&env,0)!=JNI_OK)
        LOGE("Could not attach the game thread to the JVM; audio is disabled");

    game_main(1,argv);

    android_assets_shutdown();

    /* The player quit from the menu (or pressed F12): tear the task down. */
    if(env && s_on_native_exit)
        env->CallStaticVoidMethod(s_activity,s_on_native_exit);

    if(env && s_vm)
        s_vm->DetachCurrentThread();

    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nether_MainActivity_nativeStartGame(JNIEnv *,jobject)
{
    pthread_t t;

    /*
     * The Activity can be recreated (leanback launchers do this) while the
     * process lives on. Starting a second game thread on the same globals
     * would corrupt the game, so the existing one just picks up the new
     * Surface instead.
     */
    pthread_mutex_lock(&s_start_mutex);
    if(s_game_started) {
        pthread_mutex_unlock(&s_start_mutex);
        return;
    }
    s_game_started=1;
    pthread_mutex_unlock(&s_start_mutex);

    if(pthread_create(&t,0,game_thread,0)!=0) {
        LOGE("Could not create the game thread");
        pthread_mutex_lock(&s_start_mutex);
        s_game_started=0;
        pthread_mutex_unlock(&s_start_mutex);
        return;
    }
    pthread_detach(t);
}
