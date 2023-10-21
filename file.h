#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"

/* not to be confused with PATH_MAX. This is the max length of a path, including the null terminator */
#define FILE_MAX_PATH_LEN 2048

#ifdef __cplusplus
extern "C"
{
#endif

struct file_buffer_t
{
    size_t      buffer_size;
    uint8_t *   buffer;
};

enum FILE_DIR_ENT_TYPES
{
    FILE_DIR_ENT_TYPE_FILE,
    FILE_DIR_ENT_TYPE_DIR,
    FILE_DIR_ENT_TYPE_LAST
};

struct file_dir_ent_t
{
    char        name[256];
    uint32_t    type;
};

struct file_dir_t
{
    void *          dir;
    char            path[FILE_MAX_PATH_LEN];
    struct list_t   entries;
};

uint32_t file_FormatPath(const char *path, char *formatted_path, size_t buffer_size);

uint32_t file_AbsolutePath(const char *path, char *absolute_path, size_t buffer_size);

uint32_t file_LoadFile(const char *path, struct file_buffer_t *file_buffer);

uint32_t file_Exists(const char *path);

void file_FreeFileBuffer(struct file_buffer_t *file_buffer);

uint32_t file_OpenDir(const char *path, struct file_dir_t *dir);

// uint32_t file_ChangeDir(struct file_dir_t *dir, const char *path);

void file_RefreshDir(struct file_dir_t *dir);

void file_CloseDir(struct file_dir_t *dir);


#ifdef __cplusplus
}
#endif


#endif