/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FAT Image Creator
 * FILE:            tools/fatten/fatten.c
 * PURPOSE:         FAT Image Creator (for EFI Boot)
 * PROGRAMMERS:     David Quintana
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#if _WIN32
#include <io.h>
#else
#define DIR HOST_DIR
#include <dirent.h>
#undef DIR
#endif
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

static FATFS g_Filesystem;
static int isMounted = 0;
static unsigned char buff[32768];

#if _WIN32 && !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif

#define FAT12_16_BPB_LENGTH 59
#define FAT32_BPB_LENGTH    87
#define FAT32_EXTRA_SECTOR  14
#define LIST_LINE_SIZE      16384

// tool needed by fatfs
DWORD get_fattime(void)
{
    /* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
    /* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */

    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    {
    union FatTime {
        struct {
            DWORD Second : 5; // div 2
            DWORD Minute : 6;
            DWORD Hour   : 5;
            DWORD Day    : 5;
            DWORD Month  : 4;
            DWORD Year   : 7; // year-1980
        };
        DWORD whole;
    } myTime = {
        {
            timeinfo->tm_sec / 2,
            timeinfo->tm_min,
            timeinfo->tm_hour,
            timeinfo->tm_mday,
            timeinfo->tm_mon  +  1,
            timeinfo->tm_year - 80,
        }
    };

    return myTime.whole;
    }
}

void print_help(const char* name)
{
    printf("\n");
    printf("Syntax: %s image_file [list of commands]\n\n", name);
#if _WIN32
    printf("Commands: [Note: both '/' and '-' are accepted as command prefixes.]\n");
#else
    printf("Commands:\n");
#endif
    // printf("    -format <sectors> [<filesystem>] [<custom header label>]\n"
    printf("    -format <sectors> [<custom header label>]\n"
           "            Formats the disk image.\n");
    printf("    -boot <sector file>\n"
           "            Writes a new boot sector.\n");
    printf("    -add <src path> <dst path>\n"
           "            Copies an external file or directory into the image.\n");
    printf("    -addfiles <list file>\n"
           "            Copies files from a newline-delimited dst=src list into the image.\n"
           "            Bare entries create directories inside the image.\n");
    printf("    -extract <src path> <dst path>\n"
           "            Copies a file or directory from the image into an external file\n"
           "            or directory.\n");
    printf("    -move <src path> <new path>\n"
           "            Moves/renames a file or directory.\n");
    printf("    -copy <src path> <new path>\n"
           "            Copies a file or directory.\n");
    printf("    -mkdir <path>\n"
           "            Creates a directory.\n");
    printf("    -delete <path>\n"
           "            Deletes a file or empty directory.\n");
    printf("    -list [<pattern>]\n"
           "            Lists files a directory (defaults to root).\n");
}

#define PRINT_HELP_AND_QUIT() \
    do { \
        ret = 1; \
        print_help(oargv[0]); \
        goto exit; \
    } while (0)

int is_command(const char* parg)
{
#if _WIN32
    return (parg[0] == '/') || (parg[0] == '-');
#else
    return (parg[0] == '-');
#endif
}

#define NEED_PARAMS(_min_, _max_) \
    do {\
        if (nargs < _min_) { fprintf(stderr, "Error: Too few args for command %s.\n" , argv[-1]); PRINT_HELP_AND_QUIT(); } \
        if (nargs > _max_) { fprintf(stderr, "Error: Too many args for command %s.\n", argv[-1]); PRINT_HELP_AND_QUIT(); } \
    } while(0)

int need_mount(void)
{
    int r;

    if (isMounted)
        return FR_OK;

    r = f_mount(&g_Filesystem, "0:", 1);
    if (r == FR_NO_FILESYSTEM)
        r = f_mount(&g_Filesystem, "0:", 0);
    if (r)
        return r;

    isMounted = 1;
    return FR_OK;
}

static void invalidate_mount(void)
{
    f_mount(NULL, "0:", 0);
    memset(&g_Filesystem, 0, sizeof(g_Filesystem));
    isMounted = 0;
}

static char* duplicate_string(const char* src)
{
    size_t length;
    char* copy;

    if (!src)
        return NULL;

    length = strlen(src) + 1;
    copy = malloc(length);
    if (!copy)
        return NULL;

    memcpy(copy, src, length);
    return copy;
}

static void normalize_separators(char* path)
{
    while (*path)
    {
        if (*path == '\\')
            *path = '/';
        path++;
    }
}

static char* trim_whitespace(char* text)
{
    char* end;

    while (*text && isspace((unsigned char)*text))
        text++;

    end = text + strlen(text);
    while ((end > text) && isspace((unsigned char)end[-1]))
        *--end = '\0';

    return text;
}

static int host_path_is_directory(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

static FRESULT create_image_dir_if_missing(const char* path)
{
    FILINFO info = { 0 };
    FRESULT result;

    if (!path || !*path || (strcmp(path, "/") == 0))
        return FR_OK;

    result = f_stat(path, &info);
    if (result == FR_OK)
        return (info.fattrib & AM_DIR) ? FR_OK : FR_EXIST;
    if ((result != FR_NO_FILE) && (result != FR_NO_PATH))
        return result;

    result = f_mkdir(path);
    return (result == FR_EXIST) ? FR_OK : result;
}

static FRESULT ensure_image_dir(const char* path)
{
    char* mutable_path;
    char* cursor;
    FRESULT result = FR_OK;

    if (!path || !*path || (strcmp(path, "/") == 0))
        return FR_OK;

    mutable_path = duplicate_string(path);
    if (!mutable_path)
        return FR_NOT_ENOUGH_CORE;

    normalize_separators(mutable_path);

    cursor = mutable_path;
    if (*cursor == '/')
        cursor++;

    while ((cursor = strchr(cursor, '/')) != NULL)
    {
        *cursor = '\0';
        result = create_image_dir_if_missing(mutable_path);
        if (result != FR_OK)
            goto cleanup;
        *cursor++ = '/';
    }

    result = create_image_dir_if_missing(mutable_path);

cleanup:
    free(mutable_path);
    return result;
}

static FRESULT ensure_image_parent_dirs(const char* path)
{
    char* mutable_path;
    char* slash;
    FRESULT result;

    mutable_path = duplicate_string(path);
    if (!mutable_path)
        return FR_NOT_ENOUGH_CORE;

    normalize_separators(mutable_path);
    slash = strrchr(mutable_path, '/');
    if (!slash)
    {
        result = FR_OK;
    }
    else if (slash == mutable_path)
    {
        result = FR_OK;
    }
    else
    {
        *slash = '\0';
        result = ensure_image_dir(mutable_path);
    }

    free(mutable_path);
    return result;
}

static int copy_host_file_to_image(const char* host_path, const char* image_path)
{
    FILE* source;
    FIL destination = { 0 };
    UINT read_length = 0;
    UINT write_length = 0;
    FRESULT result;
    int ret = 0;

    source = fopen(host_path, "rb");
    if (!source)
    {
        fprintf(stderr, "Error: Unable to open external file '%s' for reading.\n", host_path);
        return 1;
    }

    result = ensure_image_parent_dirs(image_path);
    if (result != FR_OK)
    {
        fprintf(stderr, "Error: Unable to create parent directories for '%s' (%d).\n", image_path, result);
        fclose(source);
        return 1;
    }

    result = f_open(&destination, image_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK)
    {
        fprintf(stderr, "Error: Unable to open file '%s' for writing (%d).\n", image_path, result);
        fclose(source);
        return 1;
    }

    while ((read_length = fread(buff, 1, sizeof(buff), source)) > 0)
    {
        result = f_write(&destination, buff, read_length, &write_length);
        if (result || (write_length < read_length))
        {
            fprintf(stderr, "Error: Unable to write '%u' bytes to disk (%d).\n", write_length, result);
            ret = 1;
            goto cleanup;
        }
    }

    if (ferror(source))
    {
        fprintf(stderr, "Error: Unable to read external file '%s'.\n", host_path);
        ret = 1;
    }

cleanup:
    f_close(&destination);
    fclose(source);
    return ret;
}

static char* join_host_path(const char* left, const char* right)
{
    size_t left_length = strlen(left);
    size_t right_length = strlen(right);
    int need_separator = (left_length > 0) && (left[left_length - 1] != '/') && (left[left_length - 1] != '\\');
    char* path = malloc(left_length + right_length + (need_separator ? 2 : 1));

    if (!path)
        return NULL;

    memcpy(path, left, left_length);
    if (need_separator)
        path[left_length++] = '/';
    memcpy(path + left_length, right, right_length + 1);
    return path;
}

static char* join_image_path(const char* left, const char* right)
{
    size_t left_length = strlen(left);
    size_t right_length = strlen(right);
    int need_separator = (left_length > 0) && (left[left_length - 1] != '/');
    char* path = malloc(left_length + right_length + (need_separator ? 2 : 1));

    if (!path)
        return NULL;

    memcpy(path, left, left_length);
    if (need_separator)
        path[left_length++] = '/';
    memcpy(path + left_length, right, right_length + 1);
    return path;
}

static int add_host_path_to_image(const char* host_path, const char* image_path)
{
    FRESULT result;

    if (host_path_is_directory(host_path))
    {
#if _WIN32
        struct _finddata_t find_data;
        intptr_t handle;
        char* pattern;
        int ret = 0;

        result = ensure_image_dir(image_path);
        if (result != FR_OK)
        {
            fprintf(stderr, "Error: Unable to create directory '%s' (%d).\n", image_path, result);
            return 1;
        }

        pattern = join_host_path(host_path, "*");
        if (!pattern)
        {
            fprintf(stderr, "Error: Out of memory while walking '%s'.\n", host_path);
            return 1;
        }

        handle = _findfirst(pattern, &find_data);
        free(pattern);
        if (handle == -1)
        {
            if (errno == ENOENT)
                return 0;

            fprintf(stderr, "Error: Unable to enumerate directory '%s' (%d).\n", host_path, errno);
            return 1;
        }

        do
        {
            char* child_host_path;
            char* child_image_path;

            if ((strcmp(find_data.name, ".") == 0) || (strcmp(find_data.name, "..") == 0))
                continue;

            child_host_path = join_host_path(host_path, find_data.name);
            child_image_path = join_image_path(image_path, find_data.name);
            if (!child_host_path || !child_image_path)
            {
                fprintf(stderr, "Error: Out of memory while walking '%s'.\n", host_path);
                free(child_host_path);
                free(child_image_path);
                ret = 1;
                break;
            }

            ret = add_host_path_to_image(child_host_path, child_image_path);
            free(child_host_path);
            free(child_image_path);
            if (ret)
                break;
        } while (_findnext(handle, &find_data) == 0);

        _findclose(handle);
        return ret;
#else
        HOST_DIR* dir;
        struct dirent* entry;
        int ret = 0;

        result = ensure_image_dir(image_path);
        if (result != FR_OK)
        {
            fprintf(stderr, "Error: Unable to create directory '%s' (%d).\n", image_path, result);
            return 1;
        }

        dir = opendir(host_path);
        if (!dir)
        {
            fprintf(stderr, "Error: Unable to enumerate directory '%s' (%d).\n", host_path, errno);
            return 1;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            char* child_host_path;
            char* child_image_path;

            if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
                continue;

            child_host_path = join_host_path(host_path, entry->d_name);
            child_image_path = join_image_path(image_path, entry->d_name);
            if (!child_host_path || !child_image_path)
            {
                fprintf(stderr, "Error: Out of memory while walking '%s'.\n", host_path);
                free(child_host_path);
                free(child_image_path);
                ret = 1;
                break;
            }

            ret = add_host_path_to_image(child_host_path, child_image_path);
            free(child_host_path);
            free(child_image_path);
            if (ret)
                break;
        }

        closedir(dir);
        return ret;
#endif
    }

    return copy_host_file_to_image(host_path, image_path);
}

static int add_files_from_list(const char* list_path)
{
    FILE* list_file;
    char line[LIST_LINE_SIZE];
    unsigned int line_number = 0;

    list_file = fopen(list_path, "rb");
    if (!list_file)
    {
        fprintf(stderr, "Error: Unable to open list file '%s' for reading.\n", list_path);
        return 1;
    }

    while (fgets(line, sizeof(line), list_file) != NULL)
    {
        char* entry;
        char* separator;
        size_t line_length = strlen(line);
        int line_complete = ((line_length > 0) && (line[line_length - 1] == '\n')) || feof(list_file);

        line_number++;
        if (!line_complete)
        {
            fprintf(stderr, "Error: List entry %u in '%s' exceeds %u bytes.\n", line_number, list_path, LIST_LINE_SIZE - 1);
            fclose(list_file);
            return 1;
        }

        entry = trim_whitespace(line);

        if ((*entry == '\0') || (*entry == '#'))
            continue;

        separator = strchr(entry, '=');
        if (separator)
        {
            char* image_path;
            char* host_path;

            *separator = '\0';
            image_path = trim_whitespace(entry);
            host_path = trim_whitespace(separator + 1);

            if ((*image_path == '\0') || (*host_path == '\0'))
            {
                fprintf(stderr, "Error: Invalid list entry %u in '%s'.\n", line_number, list_path);
                fclose(list_file);
                return 1;
            }

            if (add_host_path_to_image(host_path, image_path))
            {
                fprintf(stderr, "Error: Failed to import list entry %u from '%s'.\n", line_number, list_path);
                fclose(list_file);
                return 1;
            }
        }
        else
        {
            FRESULT result = ensure_image_dir(entry);
            if (result != FR_OK)
            {
                fprintf(stderr, "Error: Unable to create directory '%s' from list entry %u (%d).\n", entry, line_number, result);
                fclose(list_file);
                return 1;
            }
        }
    }

    fclose(list_file);
    return 0;
}

#define NEED_MOUNT() \
    do { ret = need_mount(); if(ret) \
    {\
        fprintf(stderr, "Error: Could not mount disk (%d).\n", ret); \
        goto exit; \
    } } while(0)

int main(int oargc, char* oargv[])
{
    int ret;
    int    argc = oargc - 1;
    char** argv = oargv + 1;

    // first parameter must be the image file.
    if (argc == 0)
    {
        fprintf(stderr, "Error: First parameter must be a filename.\n");
        PRINT_HELP_AND_QUIT();
    }

    if (is_command(argv[0]))
    {
        fprintf(stderr, "Error: First parameter must be a filename, found '%s' instead.\n", argv[0]);
        PRINT_HELP_AND_QUIT();
    }

    if (disk_openimage(0, argv[0]))
    {
        fprintf(stderr, "Error: Could not open image file '%s'.\n", argv[0]);
        ret = 1;
        goto exit;
    }

    argc--;
    argv++;

    while (argc > 0)
    {
        char* parg = *argv;
        int nargs = 0;
        int i = 0;

        if (!is_command(parg))
        {
            fprintf(stderr, "Error: Expected a command, found '%s' instead.\n", parg);
            PRINT_HELP_AND_QUIT();
        }

        parg++;
        argv++;
        argc--;

        // find next command, to calculare number of args
        while ((argv[i] != NULL) && !is_command(argv[i++]))
            nargs++;

        if (strcmp(parg, "format") == 0)
        {
            // NOTE: The fs driver detects which FAT format fits best based on size
            int sectors;

            NEED_PARAMS(1, 2);

            // Arg 1: number of sectors
            sectors = atoi(argv[0]);

            if (sectors <= 0)
            {
                fprintf(stderr, "Error: Sectors must be > 0\n");
                ret = 1;
                goto exit;
            }

            if (disk_ioctl(0, SET_SECTOR_COUNT, &sectors))
            {
                fprintf(stderr, "Error: Failed to set sector count to %d.\n", sectors);
                ret = 1;
                goto exit;
            }

            NEED_MOUNT();

            ret = f_mkfs("0:", 1, sectors < 4096 ? 1 : 8);
            if (ret)
            {
                fprintf(stderr, "Error: Formatting drive: %d.\n", ret);
                goto exit;
            }

            invalidate_mount();
            ret = need_mount();
            if (ret)
            {
                fprintf(stderr, "Error: Could not remount disk after formatting (%d).\n", ret);
                goto exit;
            }

            // Arg 2: custom header label (optional)
            if (nargs > 1)
            {
#define FAT_VOL_LABEL_LEN   11
                char vol_label[2 + FAT_VOL_LABEL_LEN + 1]; // Null-terminated buffer
                char* label = vol_label + 2; // The first two characters are reserved for the drive number "0:"
                char ch;

                int i, invalid = 0;
                int len = strlen(argv[1]);

                if (len <= FAT_VOL_LABEL_LEN)
                {
                    // Verify each character (should be printable ASCII)
                    // and copy it in uppercase.
                    for (i = 0; i < len; i++)
                    {
                        ch = toupper(argv[1][i]);
                        if ((ch < 0x20) || !isprint(ch))
                        {
                            invalid = 1;
                            break;
                        }

                        label[i] = ch;
                    }

                    if (!invalid)
                    {
                        // Pad the label with spaces
                        while (len < FAT_VOL_LABEL_LEN)
                        {
                            label[len++] = ' ';
                        }
                    }
                }
                else
                {
                    invalid = 1;
                }

                if (invalid)
                {
                    fprintf(stderr, "Error: Header label is limited to 11 printable uppercase ASCII symbols.");
                    ret = 1;
                    goto exit;
                }

                if (disk_read(0, buff, 0, 1))
                {
                    fprintf(stderr, "Error: Unable to read existing boot sector from image.");
                    ret = 1;
                    goto exit;
                }

                if (g_Filesystem.fs_type == FS_FAT32)
                {
                    memcpy(buff + 71, label, FAT_VOL_LABEL_LEN);
                }
                else
                {
                    memcpy(buff + 43, label, FAT_VOL_LABEL_LEN);
                }

                if (disk_write(0, buff, 0, 1))
                {
                    fprintf(stderr, "Error: Unable to write new boot sector to image.");
                    ret = 1;
                    goto exit;
                }

                // Set also the directory volume label
                memcpy(vol_label, "0:", 2);
                vol_label[2 + FAT_VOL_LABEL_LEN] = '\0';
                if (f_setlabel(vol_label))
                {
                    fprintf(stderr, "Error: Unable to set the volume label.");
                    ret = 1;
                    goto exit;
                }
            }
        }
        else if (strcmp(parg, "boot") == 0)
        {
            FILE* fe;
            BYTE* temp = buff + 1024;
            size_t boot_sector_size;

            NEED_PARAMS(1, 1);

            // Arg 1: boot file

            fe = fopen(argv[0], "rb");
            if (!fe)
            {
                fprintf(stderr, "Error: Unable to open external file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            boot_sector_size = fread(buff, 1, sizeof(buff) / 2, fe);
            if (boot_sector_size < 512)
            {
                fprintf(stderr, "Error: Unable to read boot sector from file '%s'.", argv[0]);
                fclose(fe);
                ret = 1;
                goto exit;
            }

            fclose(fe);

            NEED_MOUNT();

            if (disk_read(0, temp, 0, 1))
            {
                fprintf(stderr, "Error: Unable to read existing boot sector from image.");
                ret = 1;
                goto exit;
            }

            if (g_Filesystem.fs_type == FS_FAT32)
            {
                if (boot_sector_size < 1024)
                {
                    fprintf(stderr, "Error: FAT32 boot sector '%s' must contain both reserved sectors.\n", argv[0]);
                    ret = 1;
                    goto exit;
                }

                memcpy(buff + 3, temp + 3, FAT32_BPB_LENGTH);
            }
            else
            {
                memcpy(buff + 3, temp + 3, FAT12_16_BPB_LENGTH);
            }

            if (disk_write(0, buff, 0, 1))
            {
                fprintf(stderr, "Error: Unable to write new boot sector to image.");
                ret = 1;
                goto exit;
            }

            if (g_Filesystem.fs_type == FS_FAT32)
            {
                if (disk_write(0, buff + 512, FAT32_EXTRA_SECTOR, 1))
                {
                    fprintf(stderr, "Error: Unable to write FAT32 extra boot sector to image.");
                    ret = 1;
                    goto exit;
                }
            }
        }
        else if (strcmp(parg, "add") == 0)
        {
            NEED_PARAMS(2, 2);

            NEED_MOUNT();

            // Arg 1: external file to add
            // Arg 2: virtual filename
            ret = add_host_path_to_image(argv[0], argv[1]);
            if (ret)
                goto exit;
        }
        else if (strcmp(parg, "addfiles") == 0)
        {
            NEED_PARAMS(1, 1);

            NEED_MOUNT();

            ret = add_files_from_list(argv[0]);
            if (ret)
                goto exit;
        }
        else if (strcmp(parg, "extract") == 0)
        {
            FIL   fe = { 0 };
            FILE* fv;
            UINT rdlen = 0;

            NEED_PARAMS(2, 2);

            NEED_MOUNT();

            // Arg 1: virtual file to extract
            // Arg 2: external filename

            if (f_open(&fe, argv[0], FA_READ))
            {
                fprintf(stderr, "Error: Unable to open file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            fv = fopen(argv[1], "wb");
            if (!fv)
            {
                fprintf(stderr, "Error: Unable to open external file '%s' for writing.", argv[1]);
                f_close(&fe);
                ret = 1;
                goto exit;
            }

            while ((f_read(&fe, buff, sizeof(buff), &rdlen) == 0) && (rdlen > 0))
            {
                if (fwrite(buff, 1, rdlen, fv) < rdlen)
                {
                    fprintf(stderr, "Error: Unable to write '%d' bytes to file.", rdlen);
                    ret = 1;
                    goto exit;
                }
            }

            f_close(&fe);
            fclose(fv);
        }
        else if (strcmp(parg, "move") == 0)
        {
            NEED_PARAMS(2, 2);

            NEED_MOUNT();
            // Arg 1: src path & filename
            // Arg 2: new path & filename

            if (f_rename(argv[0], argv[1]))
            {
                fprintf(stderr, "Error: Unable to move/rename '%s' to '%s'", argv[0], argv[1]);
                ret = 1;
                goto exit;
            }
        }
        else if (strcmp(parg, "copy") == 0)
        {
            FIL fe = { 0 };
            FIL fv = { 0 };
            UINT rdlen = 0;
            UINT wrlen = 0;

            NEED_PARAMS(2, 2);

            NEED_MOUNT();
            // Arg 1: src path & filename
            // Arg 2: new path & filename

            if (f_open(&fe, argv[0], FA_READ))
            {
                fprintf(stderr, "Error: Unable to open file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }
            if (f_open(&fv, argv[1], FA_WRITE | FA_CREATE_ALWAYS))
            {
                fprintf(stderr, "Error: Unable to open file '%s' for writing.", argv[1]);
                f_close(&fe);
                ret = 1;
                goto exit;
            }

            while ((f_read(&fe, buff, sizeof(buff), &rdlen) == 0) && (rdlen > 0))
            {
                if (f_write(&fv, buff, rdlen, &wrlen) || wrlen < rdlen)
                {
                    fprintf(stderr, "Error: Unable to write '%d' bytes to disk.", wrlen);
                    ret = 1;
                    goto exit;
                }
            }

            f_close(&fe);
            f_close(&fv);
        }
        else if (strcmp(parg, "mkdir") == 0)
        {
            NEED_PARAMS(1, 1);

            NEED_MOUNT();

            // Arg 1: folder path
            if (f_mkdir(argv[0]))
            {
                fprintf(stderr, "Error: Unable to create directory.");
                ret = 1;
                goto exit;
            }
        }
        else if (strcmp(parg, "delete") == 0)
        {
            NEED_PARAMS(1, 1);

            NEED_MOUNT();

            // Arg 1: file/folder path (cannot delete non-empty folders)
            if (f_unlink(argv[0]))
            {
                fprintf(stderr, "Error: Unable to delete file or directory.");
                ret = 1;
                goto exit;
            }
        }
        else if (strcmp(parg, "list") == 0)
        {
            char* root = "/";
            DIR dir = { 0 };
            FILINFO info = { 0 };
            char lfname[257];

            NEED_PARAMS(0, 1);

            // Arg 1: folder path (optional)

            if (nargs == 1)
            {
                root = argv[0];
            }

            NEED_MOUNT();

            if (f_opendir(&dir, root))
            {
                fprintf(stderr, "Error: Unable to opening directory '%s' for listing.\n", root);
                ret = 1;
                goto exit;
            }

            printf("Listing directory contents of: %s\n", root);

            info.lfname = lfname;
            info.lfsize = sizeof(lfname)-1;
            while ((!f_readdir(&dir, &info)) && (strlen(info.fname) > 0))
            {
                if (strlen(info.lfname) > 0)
                    printf(" - %s (%s)\n", info.lfname, info.fname);
                else
                    printf(" - %s\n", info.fname);
            }
        }
        else
        {
            fprintf(stderr, "Error: Unknown or invalid command: %s\n", argv[-1]);
            PRINT_HELP_AND_QUIT();
        }
        argv += nargs;
        argc -= nargs;
    }

    ret = 0;

exit:

    disk_cleanup(0);

    return ret;
}
