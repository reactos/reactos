/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Image directory caching
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "version.h"
#include "compat.h"
#include "options.h"
#include "help.h"
#include "image.h"

#include "log2lines.h"

static char *cache_name;
static char *tmp_name;

static int
unpack_iso(char *dir, char *iso)
{
    char Line[LINESIZE];
    int res = 0;
    char iso_tmp[PATH_MAX];
    int iso_copied = 0;
    FILE *fiso;

    strcpy(iso_tmp, iso);
    if ((fiso = fopen(iso, "a")) == NULL)
    {
        l2l_dbg(1, "Open of %s failed (locked for writing?), trying to copy first\n", iso);

        strcat(iso_tmp, "~");
        if (copy_file(iso, iso_tmp))
            return 3;
        iso_copied = 1;
    }
    else
        fclose(fiso);

    sprintf(Line, UNZIP_FMT, opt_7z, iso_tmp, dir);
    if (system(Line) < 0)
    {
        l2l_dbg(0, "\nCannot unpack %s (check 7z path!)\n", iso_tmp);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        res = 1;
    }
    else
    {
        l2l_dbg(2, "\nUnpacking reactos.cab in %s\n", dir);
        sprintf(Line, UNZIP_FMT_CAB, opt_7z, dir, dir);
        if (system(Line) < 0)
        {
            l2l_dbg(0, "\nCannot unpack reactos.cab in %s\n", dir);
            l2l_dbg(1, "Failed to execute: '%s'\n", Line);
            res = 2;
        }
    }
    if (iso_copied)
        remove(iso_tmp);
    return res;
}

int
cleanable(char *path)
{
    if (strcmp(basename(path),DEF_OPT_DIR) == 0)
        return 1;
    return 0;
}

int
check_directory(int force)
{
    char Line[LINESIZE];
    char freeldr_path[PATH_MAX];
    char iso_path[PATH_MAX];
    char compressed_7z_path[PATH_MAX];
    char *check_iso;
    char *check_dir;

    check_iso = strrchr(opt_dir, '.');
    l2l_dbg(1, "Checking directory: %s\n", opt_dir);
    if (check_iso && PATHCMP(check_iso, ".7z") == 0)
    {
        l2l_dbg(1, "Checking 7z image: %s\n", opt_dir);

        // First attempt to decompress to an .iso image
        strcpy(compressed_7z_path, opt_dir);
        if ((check_dir = strrchr(compressed_7z_path, PATH_CHAR)))
            *check_dir = '\0';
        else
            strcpy(compressed_7z_path, "."); // default to current dir

        sprintf(Line, UNZIP_FMT_7Z, opt_7z, opt_dir, compressed_7z_path);

        /* This of course only works if the .7z and .iso basenames are identical
         * which is normally true for ReactOS trunk builds:
         */
        strcpy(check_iso, ".iso");
        if (!file_exists(opt_dir) || force)
        {
            l2l_dbg(1, "Decompressing 7z image: %s\n", opt_dir);
            if (system(Line) < 0)
            {
                l2l_dbg(0, "\nCannot decompress to iso image %s\n", opt_dir);
                l2l_dbg(1, "Failed to execute: '%s'\n", Line);
                return 2;
            }
        }
        else
            l2l_dbg(2, "%s already decompressed\n", opt_dir);
    }

    if (check_iso && PATHCMP(check_iso, ".iso") == 0)
    {
        l2l_dbg(1, "Checking ISO image: %s\n", opt_dir);
        if (file_exists(opt_dir))
        {
            l2l_dbg(2, "ISO image exists: %s\n", opt_dir);
            strcpy(iso_path, opt_dir);
            *check_iso = '\0';
            sprintf(freeldr_path, "%s" PATH_STR "freeldr.ini", opt_dir);
            if (!file_exists(freeldr_path) || force)
            {
                l2l_dbg(0, "Unpacking %s to: %s ...", iso_path, opt_dir);
                unpack_iso(opt_dir, iso_path);
                l2l_dbg(0, "... done\n");
            }
            else
                l2l_dbg(2, "%s already unpacked in: %s\n", iso_path, opt_dir);
        }
        else
        {
            l2l_dbg(0, "ISO image not found: %s\n", opt_dir);
            return 1;
        }
    }
    cache_name = malloc(PATH_MAX);
    tmp_name = malloc(PATH_MAX);
    strcpy(cache_name, opt_dir);
    if (cleanable(opt_dir))
        strcat(cache_name, ALT_PATH_STR CACHEFILE);
    else
        strcat(cache_name, PATH_STR CACHEFILE);
    strcpy(tmp_name, cache_name);
    strcat(tmp_name, "~");
    return 0;
}

int
read_cache(void)
{
    FILE *fr;
    LIST_MEMBER *pentry;
    char *Line = NULL;
    int result = 0;

    Line = malloc(LINESIZE + 1);
    if (!Line)
    {
        l2l_dbg(1, "Alloc Line failed\n");
        return 1;
    }
    Line[LINESIZE] = '\0';

    fr = fopen(cache_name, "r");
    if (!fr)
    {
        l2l_dbg(1, "Open %s failed\n", cache_name);
        free(Line);
        return 2;
    }
    cache.phead = cache.ptail = NULL;

    while (fgets(Line, LINESIZE, fr) != NULL)
    {
        pentry = cache_entry_create(Line);
        if (!pentry)
        {
            l2l_dbg(2, "** Create entry failed of: %s\n", Line);
        }
        else
            entry_insert(&cache, pentry);
    }

    fclose(fr);
    free(Line);
    return result;
}

int
create_cache(int force, int skipImageBase)
{
    FILE *fr, *fw;
    char *Line = NULL, *Fname = NULL;
    int len, err;
    size_t ImageBase;

    if ((fw = fopen(tmp_name, "w")) == NULL)
    {
        l2l_dbg(1, "Apparently %s is not writable (mounted ISO?), using current dir\n", tmp_name);
        cache_name = basename(cache_name);
        tmp_name = basename(tmp_name);
    }
    else
    {
        l2l_dbg(3, "%s is writable\n", tmp_name);
        fclose(fw);
        remove(tmp_name);
    }

    if (force)
    {
        l2l_dbg(3, "Removing %s ...\n", cache_name);
        remove(cache_name);
    }
    else
    {
        if (file_exists(cache_name))
        {
            l2l_dbg(3, "Cache %s already exists\n", cache_name);
            return 0;
        }
    }

    Line = malloc(LINESIZE + 1);
    if (!Line)
        return 1;
    Line[LINESIZE] = '\0';

    remove(tmp_name);
    l2l_dbg(0, "Scanning %s ...\n", opt_dir);
    snprintf(Line, LINESIZE, DIR_FMT, opt_dir, tmp_name);
    l2l_dbg(1, "Executing: %s\n", Line);
    if (system(Line) != 0)
    {
        l2l_dbg(0, "Cannot list directory %s\n", opt_dir);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        remove(tmp_name);
        free(Line);
        return 2;
    }
    l2l_dbg(0, "Creating cache ...");

    if ((fr = fopen(tmp_name, "r")) != NULL)
    {
        if ((fw = fopen(cache_name, "w")) != NULL)
        {
            while (fgets(Line, LINESIZE, fr) != NULL)
            {
                len = strlen(Line);
                if (!len)
                    continue;

                Fname = Line + len - 1;
                if (*Fname == '\n')
                    *Fname = '\0';

                while (Fname > Line && *Fname != PATH_CHAR)
                    Fname--;
                if (*Fname == PATH_CHAR)
                    Fname++;
                if (*Fname && !skipImageBase)
                {
                    if ((err = get_ImageBase(Line, &ImageBase)) == 0)
                        fprintf(fw, "%s|%s|%0x\n", Fname, Line, (unsigned int)ImageBase);
                    else
                        l2l_dbg(3, "%s|%s|%0x, ERR=%d\n", Fname, Line, (unsigned int)ImageBase, err);
                }
            }
            fclose(fw);
        }
        l2l_dbg(0, "... done\n");
        fclose(fr);
    }
    remove(tmp_name);
    free(Line);
    return 0;
}

/* EOF */
