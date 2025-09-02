#include "lvgl_spiffs_fs.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#ifndef LV_FS_LETTER_SPIFFS
#define LV_FS_LETTER_SPIFFS 'S'
#endif

static void build_path(char * out, size_t out_sz, const char * path)
{
    // Path from LVGL does not include drive letter, may start with '/'
    // Map to POSIX SPIFFS path under /spiffs
    if (!path) path = "";
    while (*path == '/') path++; // strip leading '/'
    if (*path) snprintf(out, out_sz, "/spiffs/%s", path);
    else snprintf(out, out_sz, "/spiffs");
}

#if LVGL_VERSION_MAJOR < 9

static lv_fs_res_t spiffs_open(lv_fs_drv_t * drv, void * file_p, const char * path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);
    const char * flags = (mode == LV_FS_MODE_WR) ? "wb" : (mode == LV_FS_MODE_RD) ? "rb" : "rb+";
    char full[256]; build_path(full, sizeof full, path);
    FILE * f = fopen(full, flags);
    if (!f) return LV_FS_RES_FS_ERR;
    *(FILE**)file_p = f;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_close(lv_fs_drv_t * drv, void * file_p)
{
    LV_UNUSED(drv);
    FILE * f = *(FILE**)file_p;
    if (f) fclose(f);
    *(FILE**)file_p = NULL;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    LV_UNUSED(drv);
    FILE * f = *(FILE**)file_p;
    size_t n = fread(buf, 1, btr, f);
    if (br) *br = (uint32_t)n;
    if (n < btr && ferror(f)) return LV_FS_RES_FS_ERR;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    LV_UNUSED(drv);
    FILE * f = *(FILE**)file_p;
    size_t n = fwrite(buf, 1, btw, f);
    if (bw) *bw = (uint32_t)n;
    if (n < btw) return LV_FS_RES_FS_ERR;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    FILE * f = *(FILE**)file_p;
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET : (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    return fseek(f, (long)pos, w) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t spiffs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos)
{
    LV_UNUSED(drv);
    FILE * f = *(FILE**)file_p;
    long p = ftell(f);
    if (p < 0) return LV_FS_RES_FS_ERR;
    if (pos) *pos = (uint32_t)p;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_remove(lv_fs_drv_t * drv, const char * path)
{
    LV_UNUSED(drv);
    char full[256]; build_path(full, sizeof full, path);
    return remove(full) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t spiffs_rename(lv_fs_drv_t * drv, const char * oldname, const char * newname)
{
    LV_UNUSED(drv);
    char full_old[256]; build_path(full_old, sizeof full_old, oldname);
    char full_new[256]; build_path(full_new, sizeof full_new, newname);
    return rename(full_old, full_new) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t spiffs_dir_open(lv_fs_drv_t * drv, void * rddir_p, const char * path)
{
    LV_UNUSED(drv);
    char full[256]; build_path(full, sizeof full, path);
    DIR * d = opendir(full);
    if (!d) return LV_FS_RES_FS_ERR;
    *(DIR**)rddir_p = d;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_dir_read(lv_fs_drv_t * drv, void * rddir_p, char * fn)
{
    LV_UNUSED(drv);
    DIR * d = *(DIR**)rddir_p;
    struct dirent * de;
    do {
        de = readdir(d);
        if (!de) { fn[0] = '\0'; return LV_FS_RES_OK; }
    } while (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0);
    strncpy(fn, de->d_name, LV_FS_MAX_FN_LENGTH);
    fn[LV_FS_MAX_FN_LENGTH] = '\0';
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_dir_close(lv_fs_drv_t * drv, void * rddir_p)
{
    LV_UNUSED(drv);
    DIR * d = *(DIR**)rddir_p;
    if (d) closedir(d);
    *(DIR**)rddir_p = NULL;
    return LV_FS_RES_OK;
}

#else

// LVGL v9.3 driver implementation (file-scope functions)
static void * spiffs_open_v9(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);
    const char * flags = (mode == LV_FS_MODE_WR) ? "wb" : (mode == LV_FS_MODE_RD) ? "rb" : "rb+";
    char full[256]; build_path(full, sizeof full, path);
    FILE * f = fopen(full, flags);
    return (void *)f; // LVGL stores this pointer as file descriptor
}

static lv_fs_res_t spiffs_close_v9(lv_fs_drv_t * drv, void * file_p)
{
    LV_UNUSED(drv);
    FILE * f = (FILE *)file_p;
    if (f) fclose(f);
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_read_v9(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    LV_UNUSED(drv);
    FILE * f = (FILE *)file_p;
    if (!f) return LV_FS_RES_INV_PARAM;
    size_t n = fread(buf, 1, btr, f);
    if (br) *br = (uint32_t)n;
    if (n < btr && ferror(f)) return LV_FS_RES_FS_ERR;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_write_v9(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw)
{
    LV_UNUSED(drv);
    FILE * f = (FILE *)file_p;
    if (!f) return LV_FS_RES_INV_PARAM;
    size_t n = fwrite(buf, 1, btw, f);
    if (bw) *bw = (uint32_t)n;
    if (n < btw) return LV_FS_RES_FS_ERR;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_seek_v9(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    FILE * f = (FILE *)file_p;
    if (!f) return LV_FS_RES_INV_PARAM;
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET : (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    return fseek(f, (long)pos, w) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t spiffs_tell_v9(lv_fs_drv_t * drv, void * file_p, uint32_t * pos)
{
    LV_UNUSED(drv);
    FILE * f = (FILE *)file_p;
    if (!f) return LV_FS_RES_INV_PARAM;
    long p = ftell(f);
    if (p < 0) return LV_FS_RES_FS_ERR;
    if (pos) *pos = (uint32_t)p;
    return LV_FS_RES_OK;
}

static void * spiffs_dir_open_v9(lv_fs_drv_t * drv, const char * path)
{
    LV_UNUSED(drv);
    char full[256]; build_path(full, sizeof full, path);
    DIR * d = opendir(full);
    return (void *)d;
}

static lv_fs_res_t spiffs_dir_read_v9(lv_fs_drv_t * drv, void * rddir_p, char * fn, uint32_t fn_len)
{
    LV_UNUSED(drv);
    DIR * d = (DIR *)rddir_p;
    if (!d || !fn || fn_len == 0) return LV_FS_RES_INV_PARAM;
    struct dirent * de;
    do {
        de = readdir(d);
        if (!de) { fn[0] = '\0'; return LV_FS_RES_OK; }
    } while (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0);
    // Ensure null-termination
    if (fn_len > 0) {
        strncpy(fn, de->d_name, fn_len - 1);
        fn[fn_len - 1] = '\0';
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_dir_close_v9(lv_fs_drv_t * drv, void * rddir_p)
{
    LV_UNUSED(drv);
    DIR * d = (DIR *)rddir_p;
    if (d) closedir(d);
    return LV_FS_RES_OK;
}

#endif // LVGL v8 vs v9

void lvgl_spiffs_fs_register(void)
{
#if LVGL_VERSION_MAJOR < 9
    static bool registered = false;
    if (registered) return;
    lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter = LV_FS_LETTER_SPIFFS;
    drv.cache_size = 0;
    drv.open_cb = spiffs_open;
    drv.close_cb = spiffs_close;
    drv.read_cb = spiffs_read;
    drv.write_cb = spiffs_write;
    drv.seek_cb = spiffs_seek;
    drv.tell_cb = spiffs_tell;
    drv.dir_open_cb = spiffs_dir_open;
    drv.dir_read_cb = spiffs_dir_read;
    drv.dir_close_cb = spiffs_dir_close;
    drv.remove_cb = spiffs_remove;
    drv.rename_cb = spiffs_rename;
    lv_fs_drv_register(&drv);
    registered = true;
#else
    static bool registered = false;
    if (registered) return;

    lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter = LV_FS_LETTER_SPIFFS;
    drv.cache_size = 0;
    drv.open_cb = spiffs_open_v9;
    drv.close_cb = spiffs_close_v9;
    drv.read_cb = spiffs_read_v9;
    drv.write_cb = spiffs_write_v9;
    drv.seek_cb = spiffs_seek_v9;
    drv.tell_cb = spiffs_tell_v9;
    drv.dir_open_cb = spiffs_dir_open_v9;
    drv.dir_read_cb = spiffs_dir_read_v9;
    drv.dir_close_cb = spiffs_dir_close_v9;

    lv_fs_drv_register(&drv);
    registered = true;
#endif
}
