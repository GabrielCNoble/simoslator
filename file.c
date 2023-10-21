#include "file.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #define __USE_XOPEN2K8 1
#include <dirent.h>
#include <fcntl.h>
 
extern char m_work_dir[];

#define FILE_PATH_SEPARATOR '/'

uint32_t file_FormatPath(const char *path, char *formatted_path, size_t buffer_size)
{
    if(path != NULL && formatted_path != NULL)
    {
        uint32_t index = 0;
        size_t formatted_index = 0;
        size_t prev_separator_index = strlen(path);

        // printf("%s\n", path);

        while(path[index] != '\0' && formatted_index < buffer_size)
        {
            if(path[index] == FILE_PATH_SEPARATOR)
            {
                prev_separator_index = index;
                index++;
                formatted_path[formatted_index] = '/';
                formatted_index++;
                while(path[index] == FILE_PATH_SEPARATOR)
                {
                    prev_separator_index = index;
                    index++;
                }
            }
            else if(path[index] == '.' && index == prev_separator_index + 1)
            {
                index++;
                if(path[index] == '.')
                {
                    index++;

                    do
                    {
                        formatted_index--;
                    }
                    while(formatted_path[formatted_index] == FILE_PATH_SEPARATOR && formatted_index > 0);

                    if(formatted_index == 0 && formatted_path[formatted_index] == '.')
                    {
                        /* trying to go to parent directory of current directory, but we don't have that
                        in the current formatted path. */
                        return 0;
                    }

                    /* NOTE: this won't really work if the path contains a drive letter at the start, and it's not really intended to */
                    while(formatted_path[formatted_index] != FILE_PATH_SEPARATOR && formatted_index > 0)
                    {
                        formatted_path[formatted_index] = '\0';
                        formatted_index--;
                    }
                    
                    formatted_path[formatted_index] = '/';
                    formatted_index++;
                }

                while(path[index] != FILE_PATH_SEPARATOR && path[index] != '\0')
                {
                    index++;
                }

                while(path[index] == FILE_PATH_SEPARATOR && path[index] != '\0')
                {
                    prev_separator_index = index;
                    index++;
                }
                
                // if(path[index] == '/')
                // {
                //     index++;
                // }
            }
            else
            {
                formatted_path[formatted_index] = path[index];
                formatted_index++;
                index++;
            }
        }

        if(path[index] != '\0' || formatted_index == buffer_size)
        {
            return 0;
        }

        formatted_path[formatted_index] = '\0';

        return 1;
    }

    return 0;
}

uint32_t file_AbsolutePath(const char *path, char *absolute_path, size_t buffer_size)
{
    if(path != NULL && absolute_path != NULL && buffer_size > 0)
    {
        absolute_path[0] = '\0';
        size_t path_len = strlen(path) + 1;

        if(path[0] == '/')
        {
            return file_FormatPath(path, absolute_path, buffer_size);
        }

        for(size_t index = 0; index < path_len; index++)
        {
            if(path[index] == ':')
            {
                return file_FormatPath(path, absolute_path, buffer_size);
            }
        }
        
        size_t absolute_path_len = strlen(m_work_dir);
        if(absolute_path_len > buffer_size)
        {
            return 0;    
        }

        strncpy(absolute_path, m_work_dir, buffer_size);
        if(absolute_path[absolute_path_len] != '/')
        {
            if(absolute_path_len == buffer_size)
            {
                return 0;
            }

            strncat(absolute_path, "/", buffer_size - absolute_path_len);
            absolute_path_len++;
        }

        strncat(absolute_path, path, buffer_size - absolute_path_len);
        return file_FormatPath(absolute_path, absolute_path, buffer_size);
    }

    return 0;
}

const char *file_GetLastPathComponent(const char *path)
{
    const char *component = NULL;
    if(path != NULL)
    {
        size_t offset = strlen(path);
        while(offset > 0 && path[offset - 1] != FILE_PATH_SEPARATOR)
        {
            offset--;
        }

        component = path + offset;
    }

    return component;
}

uint32_t file_LoadFile(const char *path, struct file_buffer_t *file_buffer)
{
    if(file_buffer != NULL && path != NULL)
    {
        char formatted_path[FILE_MAX_PATH_LEN];
        
        if(file_FormatPath(path, formatted_path, sizeof(formatted_path)))
        {
            FILE *file = fopen(formatted_path, "rb");

            if(file != NULL)
            {
                fseek(file, 0, SEEK_END);
                file_buffer->buffer_size = (size_t)ftell(file);
                rewind(file);

                /* extra zeroed space at the end, to accomodate for text files */
                file_buffer->buffer = calloc(1, file_buffer->buffer_size + 1);
                fread(file_buffer->buffer, file_buffer->buffer_size, 1, file);
                fclose(file);

                return 1;
            }
        }
    }

    return 0;
}

uint32_t file_Exists(const char *path)
{
    if(path != NULL)
    {
        char formatted_path[FILE_MAX_PATH_LEN];
        if(file_FormatPath(path, formatted_path, sizeof(formatted_path)))
        {
            FILE *file = fopen(formatted_path, "rb");
            if(file != NULL)
            {
                fclose(file);
                return 1;
            }
        }
    }

    return 0;
}

void file_FreeFileBuffer(struct file_buffer_t *file_buffer)
{
    if(file_buffer != NULL && file_buffer->buffer != NULL)
    {
        free(file_buffer->buffer);
        file_buffer->buffer = NULL;
        file_buffer->buffer_size = 0;
    }
}

uint32_t file_OpenDir(const char *path, struct file_dir_t *dir)
{
    DIR *current_dir = opendir(path);
    if(current_dir != NULL)
    {
        if(dir->dir != NULL)
        {
            closedir(dir->dir);
        }
        else
        {
            dir->entries = list_Create(sizeof(struct file_dir_ent_t), 512);
        }

        dir->dir = current_dir;
        if(file_AbsolutePath(path, dir->path, sizeof(dir->path)))
        {
            file_RefreshDir(dir);
            return 1;
        }
    }

    return 0;
}

// uint32_t file_ChangeDir(struct file_dir_t *dir, const char *path)
// {
//     static char new_path[FILE_MAX_PATH_LEN];

//     if(dir != NULL)
//     {
//         if(dir->dir != NULL)
//         {
//             strcpy(new_path, dir->path);
//             strcat(new_path, "/");
//             strcat(new_path, path);

//             return file_OpenDir(new_path, dir);
//         }

//         return file_OpenDir(path, dir);
//     }

//     return 0;
// }

void file_RefreshDir(struct file_dir_t *dir)
{
    static char entry_path[FILE_MAX_PATH_LEN];
    size_t dir_path_len;
    if(dir != NULL && dir->dir != NULL)
    {
        strncpy(entry_path, dir->path, sizeof(entry_path) - 1);
        entry_path[sizeof(entry_path) - 1] = '\0';
        dir_path_len = strlen(dir->path);
        strncat(entry_path, "/", (sizeof(entry_path) - 1) - dir_path_len);
        dir_path_len++;

        if(dir_path_len >= sizeof(entry_path) - 1)
        {
            printf("Directory path [%s] is too large", entry_path);
            return;
        }

        dir->entries.cursor = 0;
        rewinddir(dir->dir);
        struct dirent *entry = readdir(dir->dir);
        while(entry)
        {
            uint64_t index = list_AddElement(&dir->entries, NULL);
            struct file_dir_ent_t *dir_entry = list_GetElement(&dir->entries, index);
            strcpy(dir_entry->name, entry->d_name);
            entry_path[dir_path_len] = '\0';
            strncat(entry_path, dir_entry->name, (sizeof(entry_path) - 1) - dir_path_len);
            DIR *probe_dir = opendir(entry_path);
            if(probe_dir != NULL)
            {
                closedir(probe_dir);
                dir_entry->type = FILE_DIR_ENT_TYPE_DIR;
            }
            else
            {
                dir_entry->type = FILE_DIR_ENT_TYPE_FILE;
            }
            entry = readdir(dir->dir);
        }
    }
}

void file_CloseDir(struct file_dir_t *dir)
{
    if(dir != NULL && dir->dir != NULL)
    {
        closedir(dir->dir);
        dir->entries.cursor = 0;
    }
}