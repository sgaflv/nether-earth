
/*
 * Android asset backend.
 *
 * Gradle packages the repository's existing assets/ directory directly; this
 * backend reads those APK assets through AAssetManager. No assets directory
 * is copied to app-internal storage. Readable assets are cached in RAM so
 * FILE*-based legacy parsers can keep using fread/fscanf/fseek/fclose.
 */
#include "assets.h"
/* Declares android_assets_set_manager()/shutdown() with C linkage, which is
   how android_bridge.cpp calls them. */
#include "android_bridge.h"

#include <android/asset_manager.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static AAssetManager *s_manager = 0;
static char s_user_dir[512] = "";

struct AssetCache {
    char *name;
    unsigned char *data;
    size_t size;
    AssetCache *next;
};
static AssetCache *s_cache = 0;

void android_assets_set_manager(AAssetManager *manager) { s_manager = manager; }

void android_assets_shutdown(void)
{
    AssetCache *p=s_cache;
    while(p) {
        AssetCache *n=p->next;
        free(p->name); free(p->data); free(p);
        p=n;
    }
    s_cache=0;
}

/*
 * Turn a game-side path into an APK asset name.
 *
 * AAssetManager does an exact lookup in the APK's asset table, so unlike
 * fopen() it does not forgive the "./", the "assets/" prefix or the doubled
 * separators the call sites produce (the .ase loader builds texture paths as
 * "assets/textures/" + "/" + name, for instance).
 */
static const char *normalize_asset_path(const char *path, char *buf, size_t n)
{
    size_t w = 0;

    if (!path || n == 0) return 0;

    for (;;) {
        if (strncmp(path, "./", 2) == 0) { path += 2; continue; }
        if (*path == '/') { path++; continue; }
        if (strncmp(path, "assets/", 7) == 0) { path += 7; continue; }
        break;
    }

    /* Copy, collapsing runs of '/' into one. */
    while (*path && w + 1 < n) {
        if (*path == '/' && w > 0 && buf[w-1] == '/') { path++; continue; }
        buf[w++] = *path++;
    }
    buf[w] = 0;

    return buf;
}

static AssetCache *load_cached(const char *name)
{
    for (AssetCache *p=s_cache; p; p=p->next)
        if (strcmp(p->name,name)==0) return p;

    if (!s_manager) return 0;
    AAsset *a=AAssetManager_open(s_manager,name,AASSET_MODE_BUFFER);
    if (!a) return 0;

    off_t len=AAsset_getLength(a);
    if (len<0 || (uint64_t)len>SIZE_MAX) {
        AAsset_close(a); return 0;
    }

    unsigned char *data=(unsigned char*)malloc((size_t)len);
    if (!data) {
        AAsset_close(a); return 0;
    }

    size_t done=0;
    while(done<(size_t)len) {
        int got=AAsset_read(a,data+done,(size_t)len-done);
        if(got<=0) {
            free(data); AAsset_close(a); return 0;
        }
        done+=(size_t)got;
    }
    AAsset_close(a);

    AssetCache *p=(AssetCache*)calloc(1,sizeof(*p));
    if(!p) { free(data); return 0; }
    p->name=strdup(name);
    p->data=data;
    p->size=(size_t)len;
    p->next=s_cache;
    s_cache=p;
    return p;
}

void asset_set_root(const char *root) { (void)root; }
const char *asset_root(void) { return "APK/assets"; }

FILE *asset_open(const char *path,const char *mode)
{
    char name[1024];
    if(!path || !mode || mode[0]!='r') return 0;
    normalize_asset_path(path,name,sizeof(name));

    AssetCache *a=load_cached(name);
    if(!a) return 0;

    /* fmemopen provides the same stdio interface expected by the existing
       parsers. Its backing memory belongs to the cache and lives for the
       process, so fclose() does not invalidate the cached asset. */
    return fmemopen(a->data,a->size,mode);
}

char **asset_list(const char *subdir,const char *ext)
{
    char dir[1024];
    char **list;
    int count=0;
    const char *name;
    AAssetDir *dp;

    if(!s_manager) return 0;
    normalize_asset_path(subdir ? subdir : "",dir,sizeof(dir));

    dp=AAssetManager_openDir(s_manager,dir);
    if(!dp) return 0;

    list=(char**)calloc(1,sizeof(char*));
    if(!list) { AAssetDir_close(dp); return 0; }

    while((name=AAssetDir_getNextFileName(dp))!=0) {
        char **grown;

        if(ext && ext[0]) {
            size_t len=strlen(name), elen=strlen(ext);
            if(len<elen || strcmp(name+len-elen,ext)!=0) continue;
        }

        grown=(char**)realloc(list,(count+2)*sizeof(char*));
        if(!grown) break;
        list=grown;

        list[count]=strdup(name);
        if(!list[count]) break;
        ++count;
    }
    AAssetDir_close(dp);

    list[count]=0;
    return list;
}

void asset_list_free(char **list)
{
    if(!list) return;
    for(int i=0;list[i];++i) free(list[i]);
    free(list);
}

void user_set_dir(const char *dir)
{
    if(!dir) dir="";
    strncpy(s_user_dir,dir,sizeof(s_user_dir)-1);
    s_user_dir[sizeof(s_user_dir)-1]=0;
}

const char *user_dir(void) { return s_user_dir; }

FILE *user_open(const char *path,const char *mode)
{
    if(!path) return 0;
    char full[1024];
    if(s_user_dir[0]) snprintf(full,sizeof(full),"%s/%s",s_user_dir,path);
    else snprintf(full,sizeof(full),"%s",path);
    return fopen(full,mode);
}
