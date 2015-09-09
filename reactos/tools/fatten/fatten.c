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
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

FATFS g_Filesystem;

static int isMounted = 0;
static char buff[32768];

// tool needed by fatfs
DWORD get_fattime()
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
            timeinfo->tm_mon,
            timeinfo->tm_year - 1980,
        }
    };

    return myTime.whole;
    }
}

int is_command(const char* parg)
{
#if _WIN32
    return (parg[0] == '/') || (parg[0] == '-');
#else
    return (parg[0] == '-');
#endif
}

#define NEED_PARAMS(_min_,_max_) \
    do {\
        if(nargs<_min_) { printf("Too few args for command %s.\n",argv[-1]); PRINT_HELP_AND_QUIT(); } \
        if(nargs>_max_) { printf("Too many args for command %s.\n",argv[-1]); PRINT_HELP_AND_QUIT(); } \
    } while(0)

int need_mount()
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
        printf("Error: could not mount disk (%d). \n", ret); \
        PRINT_HELP_AND_QUIT(); \
    } } while(0)

void print_help(char const * const name)
{
    printf("Syntax: %s image_file [list of commands]\n\n", name);
    printf("Commands: [Note: both '/' and '-' are accepted as command prefixes.] \n");
    printf("    /format <sectors> [<filesystem>]         Formats the disk image.\n");
    printf("    /boot <sector file>          Writes a new boot sector.\n");
    printf("    /add <src path> <dst path>   Copies an external file or directory\n"
        "                                 into the image.\n");
    printf("    /extract <src path> <dst path>  Copies a file or directory from the image\n"
        "                                 into an external file or directory.\n");
    printf("    /move <src path> <new path>  Moves/renames a file or directory.\n");
    printf("    /copy <src path> <new path>  Copies a file or directory.\n");
    printf("    /mkdir <src path> <new path> Creates a directory.\n");
    printf("    /rmdir <src path> <new path> Creates a directory.\n");
    printf("    /list [<pattern>]            Lists files a directory (defaults to root).\n");
    //printf("    /recursive                   Enables recursive processing for directories.\n");
}

#define PRINT_HELP_AND_QUIT() \
    do { \
        ret = 1; \
        print_help(oargv[0]); \
        goto exit; \
    } while (0)

int main(int oargc, char* oargv[])
{
    int ret;
    int    argc = oargc - 1;
    char** argv = oargv + 1;

    // first parameter must be the image file.
    if (argc == 0)
    {
        PRINT_HELP_AND_QUIT();
    }

    if (is_command(argv[0]))
    {
        printf("Error: first parameter must be a filename, found '%s' instead. \n", argv[0]);
        PRINT_HELP_AND_QUIT();
    }

    if (disk_openimage(0, argv[0]))
    {
        printf("Error: could not open image file '%s'. \n", argv[0]);
        PRINT_HELP_AND_QUIT();
    }

    argc--;
    argv++;

    while (argc > 0)
    {
        char *parg = *argv;
        int nargs = 0;
        int i = 0;

        if (!is_command(parg))
        {
            printf("Error: Expected a command, found '%s' instead. \n", parg);
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

            NEED_PARAMS(1, 1);

            // Arg 1: number of sectors
            sectors = atoi(argv[0]);

            if (sectors <= 0)
            {
                printf("Error: Sectors must be > 0\n");
                ret = 1;
                goto exit;
            }

            disk_ioctl(0, SET_SECTOR_COUNT, &sectors);

            NEED_MOUNT();

            ret = f_mkfs("0:", 1, sectors < 4096 ? 1 : 8);
            if (ret)
            {
                printf("ERROR: Formatting drive: %d.\n", ret);
                PRINT_HELP_AND_QUIT();
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
                printf("Error: unable to open external file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            if(!fread(buff, 512, 1, fe))
            {
                printf("Error: unable to read boot sector from file '%s'.", argv[0]);
                ret = 1;
                goto exit;
            }

            NEED_MOUNT();

            if(disk_read(0, temp, 0, 1))
            {
                printf("Error: unable to read existing boot sector from image.");
                ret = 1;
                goto exit;
            }

            if (g_Filesystem.fs_type == FS_FAT32)
            {
                printf("TODO: writing boot sectors for FAT32 images not yet supported.");
                ret = 1;
                goto exit;
            }
            else
            {
                // Quick&dirty hardcoded length.
                memcpy(buff + 2, temp + 2, 0x3E - 0x02);
            }

            if (disk_write(0, buff, 0, 1))
            {
                printf("Error: unable to write new boot sector to image.");
                ret = 1;
                goto exit;
            }

            fclose(fe);
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
                printf("Error: unable to open external file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            if (f_open(&fv, argv[1], FA_WRITE | FA_CREATE_ALWAYS))
            {
                printf("Error: unable to open file '%s' for writing.", argv[1]);
                fclose(fe);
                ret = 1;
                goto exit;
            }

            while ((rdlen = fread(buff, 1, 32768, fe)) > 0)
            {
                f_write(&fv, buff, rdlen, &wrlen);
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
                printf("Error: unable to open file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }

            fv = fopen(argv[1], "wb");

            if (!fv)
            {
                printf("Error: unable to open external file '%s' for writing.", argv[1]);
                f_close(&fe);
                ret = 1;
                goto exit;
            }

            while ((f_read(&fe, buff, 32768, &rdlen) == 0) && (rdlen > 0))
            {
                fwrite(buff, 1, rdlen, fv);
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
                printf("Error moving/renaming '%s' to '%s'", argv[0], argv[1]);
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
                printf("Error: unable to open file '%s' for reading.", argv[0]);
                ret = 1;
                goto exit;
            }
            if (f_open(&fv, argv[1], FA_WRITE | FA_CREATE_ALWAYS))
            {
                printf("Error: unable to open file '%s' for writing.", argv[1]);
                f_close(&fe);
                ret = 1;
                goto exit;
            }

            while ((f_read(&fe, buff, 32768, &rdlen) == 0) && (rdlen > 0))
            {
                f_write(&fv, buff, rdlen, &wrlen);
            }

            f_close(&fe);
            f_close(&fv);
        }
        else if (strcmp(parg, "mkdir") == 0)
        {
            NEED_PARAMS(1, 1);

            NEED_MOUNT();

            // Arg 1: folder path
            f_mkdir(argv[0]);
        }
        else if (strcmp(parg, "delete") == 0)
        {
            NEED_PARAMS(1, 1);

            NEED_MOUNT();

            // Arg 1: file/folder path (cannot delete non-empty folders)
            f_unlink(argv[0]);
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
                printf("Error opening directory '%s'.\n", root);
                ret = 1;
                goto exit;
            }

            printf("Listing directory contents of: %s\n", root);

            info.lfname = lfname;
            info.lfsize = 256;
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
            printf("Error: Unknown or invalid command: %s\n", argv[-1]);
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

