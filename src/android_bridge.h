
#ifndef ANDROID_BRIDGE_H
#define ANDROID_BRIDGE_H
#include <android/asset_manager.h>
#ifdef __cplusplus
extern "C" {
#endif
void android_assets_set_manager(AAssetManager *manager);
void android_assets_shutdown(void);
int android_audio_load(const char *path);
void android_audio_play(int sound_id, int volume);
void android_audio_free(int sound_id);
void android_audio_shutdown(void);
#ifdef __cplusplus
}
#endif
#endif
