/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FAT Image Creator
 * FILE:            tools/fatten/fatten.c
 * PURPOSE:         FAT Image Creator (for EFI Boot)
 * PROGRAMMERS:     David Quintana
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

static FATFS g_Filesystem;
static int isMounted = 0;
static unsigned char buff[32768];

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
    printf("    -extract <src path> <dst path>\n"
           "            Copies a file or directory from the image into an external file\n"
           "            or directory.\n");
    printf("    -move <src path> <new path>\n"
           "            Moves/renames a file or directory.\n");
    printf("    -copy <src path> <new path>\n"
           "            Copies a file or directory.\n");
    printf("    -mkdir <src path> <new path>\n"
           "            Creates a directory.\n");
    printf("    -rmdir <src path> <new path>\n"
           "            Creates a directory.\n");
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

    r = f_mount(&g_Filesystem, "0:", 0);
    if (r)
        return r;

    isMounted = 1;
    return FR_OK;
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

            NEED_PARAMS(1, 1);

            // Arg 1: boot file

            fe = fopen(argv[0], "rb");
            if (!fe)
            {
                fprintf(stderr, "Error: Unable to open external file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            if (!fread(buff, 512, 1, fe))
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
                printf("TODO: Writing boot sectors for FAT32 images not yet supported.");
                ret = 1;
                goto exit;
            }
            else
            {
#define FAT16_HEADER_START 3
#define FAT16_HEADER_END 62

                memcpy(buff + FAT16_HEADER_START, temp + FAT16_HEADER_START, FAT16_HEADER_END - FAT16_HEADER_START);
            }

            if (disk_write(0, buff, 0, 1))
            {
                fprintf(stderr, "Error: Unable to write new boot sector to image.");
                ret = 1;
                goto exit;
            }
        }
        else if (strcmp(parg, "add") == 0)
        {
            FILE* fe;
            FIL   fv = { 0 };
            UINT rdlen = 0;
            UINT wrlen = 0;

            NEED_PARAMS(2, 2);

            NEED_MOUNT();

            // Arg 1: external file to add
            // Arg 2: virtual filename

            fe = fopen(argv[0], "rb");
            if (!fe)
            {
                fprintf(stderr, "Error: Unable to open external file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            if (f_open(&fv, argv[1], FA_WRITE | FA_CREATE_ALWAYS))
            {
                fprintf(stderr, "Error: Unable to open file '%s' for writing.", argv[1]);
                fclose(fe);
                ret = 1;
                goto exit;
            }

            while ((rdlen = fread(buff, 1, sizeof(buff), fe)) > 0)
            {
                if (f_write(&fv, buff, rdlen, &wrlen) || wrlen < rdlen)
                {
                    fprintf(stderr, "Error: Unable to write '%d' bytes to disk.", wrlen);
                    ret = 1;
                    goto exit;
                }
            }

            fclose(fe);
            f_close(&fv);
        }
        else if (strcmp(parg, "extract") == 0)
        {
            FIL   fe = { 0 };
            FILE* fv;
            UINT rdlen = 0;
            UINT wrlen = 0;

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
