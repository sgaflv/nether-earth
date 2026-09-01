/*
 * Asset access abstraction - desktop backend.
 *
 * Desktop keeps game assets under ./assets (the repository's authoritative
 * asset tree) and user data in the current working directory. Directory
 * enumeration uses POSIX opendir/readdir so there is no Windows-only code in
 * game logic. A future Android backend replaces this file with an
 * AAssetManager-based implementation plus a writable application-data dir.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>

#include "assets.h"

static char s_asset_root[512] = "";
static char s_user_dir[512] = "";

void asset_set_root(const char *root)
{
	if (root == 0)
		root = "";
	strncpy(s_asset_root, root, sizeof(s_asset_root) - 1);
	s_asset_root[sizeof(s_asset_root) - 1] = 0;
}

const char *asset_root(void)
{
	return s_asset_root;
}

FILE *asset_open(const char *path, const char *mode)
{
	if (path == 0)
		return 0;

	/* If the path already includes an assets/ prefix, or the asset root is */
	/* empty, open it directly so existing call sites keep working.          */
	if (strstr(path, "assets/") != 0 || s_asset_root[0] == 0)
		return fopen(path, mode);

	char full[1024];
	snprintf(full, sizeof(full), "%s/%s", s_asset_root, path);
	return fopen(full, mode);
}

char **asset_list(const char *subdir, const char *ext)
{
	char dir[1024];
	if (s_asset_root[0] == 0)
		snprintf(dir, sizeof(dir), "assets/%s", subdir);
	else
		snprintf(dir, sizeof(dir), "%s/%s", s_asset_root, subdir);

	DIR *dp = opendir(dir);
	if (dp == 0)
		return 0;

	char **list = (char **)calloc(1, sizeof(char *));
	int count = 0;
	struct dirent *ep;

	while ((ep = readdir(dp)) != 0) {
		const char *name = ep->d_name;
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;
		if (ext == 0 || (ext[0] != 0)) {
			size_t len = strlen(name);
			size_t elen = strlen(ext);
			if (len < elen || strcmp(name + len - elen, ext) != 0)
				continue;
		}
		list = (char **)realloc(list, (count + 2) * sizeof(char *));
		list[count] = (char *)malloc(strlen(name) + 1);
		strcpy(list[count], name);
		count++;
	}
	closedir(dp);

	list[count] = 0;
	return list;
}

void asset_list_free(char **list)
{
	if (list == 0)
		return;
	int i;
	for (i = 0; list[i] != 0; i++)
		free(list[i]);
	free(list);
}

void user_set_dir(const char *dir)
{
	if (dir == 0)
		dir = "";
	strncpy(s_user_dir, dir, sizeof(s_user_dir) - 1);
	s_user_dir[sizeof(s_user_dir) - 1] = 0;
}

const char *user_dir(void)
{
	return s_user_dir;
}

FILE *user_open(const char *path, const char *mode)
{
	if (path == 0)
		return 0;
	if (s_user_dir[0] == 0)
		return fopen(path, mode);

	char full[1024];
	snprintf(full, sizeof(full), "%s/%s", s_user_dir, path);
	return fopen(full, mode);
}
