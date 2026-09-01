#ifndef AUDIO_H
#define AUDIO_H

/*
 * Minimal audio abstraction.
 *
 * The game only needs short WAV sound effects with per-play channel volume.
 * This interface hides the concrete backend (currently SDL_mixer) from game
 * code so a smaller native / Android audio implementation could replace it
 * without touching gameplay.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sound Sound;

/* Open the audio device. Returns non-zero on success. */
int audio_init(void);

/* Close the audio device. */
void audio_shutdown(void);

/* Load a WAV file, returning a handle (0 on failure). */
Sound *audio_load_wav(const char *path);

/* Free a previously loaded sound. Safe to call with 0. */
void audio_free(Sound *s);

/* Play a sound on a free channel at the given volume (0..128). */
void audio_play(Sound *s, int volume);

#ifdef __cplusplus
}
#endif

#endif
