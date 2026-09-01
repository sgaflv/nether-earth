/*
 * Audio abstraction - SDL_mixer backend.
 *
 * Game code only uses the audio.h API; all SDL_mixer usage lives here.
 * A future Android backend can replace this file with a minimal PCM mixer.
 */

#include "audio.h"

#include <SDL_mixer.h>


struct Sound {
	Mix_Chunk *chunk;
};

int audio_init(void)
{
	if (Mix_OpenAudio(22050, AUDIO_S16, 2, 1024) != 0)
		return 0;
	return 1;
}

void audio_shutdown(void)
{
	Mix_CloseAudio();
}

Sound *audio_load_wav(const char *path)
{
	Mix_Chunk *chunk = Mix_LoadWAV(path);
	if (chunk == 0)
		return 0;

	Sound *s = new Sound();
	s->chunk = chunk;
	return s;
}

void audio_free(Sound *s)
{
	if (s != 0) {
		if (s->chunk != 0)
			Mix_FreeChunk(s->chunk);
		delete s;
	}
}

void audio_play(Sound *s, int volume)
{
	if (s != 0 && s->chunk != 0)
		Mix_Volume(Mix_PlayChannel(-1, s->chunk, 0), volume);
}
