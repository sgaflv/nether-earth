
/* Android audio backend. Java SoundPool decodes/plays the packaged WAVs. */
#include "audio.h"
#include "android_bridge.h"

struct Sound { int id; };

int audio_init(void)
{
    return 1;
}

void audio_shutdown(void)
{
    android_audio_shutdown();
}

Sound *audio_load_wav(const char *path)
{
    int id=android_audio_load(path);
    if(id<0) return 0;
    Sound *s=new Sound();
    s->id=id;
    return s;
}

void audio_free(Sound *s)
{
    if(!s) return;
    android_audio_free(s->id);
    delete s;
}

void audio_play(Sound *s,int volume)
{
    if(s) android_audio_play(s->id,volume);
}
