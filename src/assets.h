#ifndef ASSETS_H
#define ASSETS_H

/*
 * Asset access abstraction.
 *
 * Game code should never hard-code the physical location of game assets or
 * use platform-specific directory enumeration directly. Instead it uses the
 * helpers below.
 *
 *   * asset_open()   - open a read asset (maps, models, textures, sound).
 *   * asset_list()   - enumerate file names in an asset sub-directory.
 *   * user_open()    - open a runtime-generated file (save games, config).
 *
 * The desktop backend keeps assets under ./assets and user data in the
 * current working directory. A future Android backend can point the same API
 * at packaged assets (AAssetManager) and a writable application-data dir
 * without changing game code.
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Set the base directory for game assets. Default is "" (current dir). */
void asset_set_root(const char *root);
const char *asset_root(void);

/*
 * Open a game asset. 'path' may be either absolute or relative (e.g.
 * "assets/models/foo.ase", "maps/city.map" or "models/foo.ase"); it is
 * resolved relative to the asset root. Returns a FILE* (read), or 0.
 */
FILE *asset_open(const char *path, const char *mode);

/*
 * Enumerate file names in an asset sub-directory (e.g. "maps") whose name
 * ends with the given extension (e.g. ".map"). Returns a NULL-terminated
 * array of malloc'd file names, or 0 if the directory cannot be read.
 * Free the result with asset_list_free().
 */
char **asset_list(const char *subdir, const char *ext);
void asset_list_free(char **list);

/* Set the base directory for runtime-generated user data (saves, config). */
/* Default is "" (current working directory). */
void user_set_dir(const char *dir);
const char *user_dir(void);

/*
 * Open a runtime-generated writable file (save game, debug report, config).
 * Resolved relative to the user-data directory.
 */
FILE *user_open(const char *path, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
