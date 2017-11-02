/*
 *  ReactOS kernel
 *  Copyright (C) 2003, 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/mkhive.c
 * PURPOSE:         Hive maker
 * PROGRAMMERS:     Eric Kohl
 *                  Hervé Poussineau
 *                  Hermès Bélusca-Maïto
 */

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "mkhive.h"

#ifdef _MSC_VER
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif // _MSC_VER

#ifndef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif


void usage(void)
{
    printf("Usage: mkhive -h:hive1[,hiveN...] -d:<dstdir> <inffiles>\n\n"
           "  -h:hiveN  - Comma-separated list of hives to create. Possible values are:\n"
           "              SETUPREG, SYSTEM, SOFTWARE, DEFAULT, SAM, SECURITY, BCD.\n"
           "  -d:dstdir - The binary hive files are created in this directory.\n"
           "  inffiles  - List of INF files with full path.\n");
}

void convert_path(char *dst, char *src)
{
    int i;

    i = 0;
    while (src[i] != 0)
    {
#ifdef _WIN32
        if (src[i] == '/')
        {
            dst[i] = '\\';
        }
#else
        if (src[i] == '\\')
        {
            dst[i] = '/';
        }
#endif
        else
        {
            dst[i] = src[i];
        }

        i++;
    }
    dst[i] = 0;
}

int main(int argc, char *argv[])
{
    INT ret;
    UINT i;
    PCSTR HiveList = NULL;
    CHAR DestPath[PATH_MAX] = "";
    CHAR FileName[PATH_MAX];

    if (argc < 4)
    {
        usage();
        return -1;
    }

    printf("Binary hive maker\n");

    /* Read the options */
    for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
        if (argv[i][1] == 'h' && (argv[i][2] == ':' || argv[i][2] == '='))
        {
            HiveList = argv[i] + 3;
        }
        else if (argv[i][1] == 'd' && (argv[i][2] == ':' || argv[i][2] == '='))
        {
            convert_path(DestPath, argv[i] + 3);
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            return -1;
        }
    }

    /* Check whether we have all the parameters needed */
    if (!HiveList || !*HiveList)
    {
        fprintf(stderr, "The mandatory list of hives is missing.\n");
        return -1;
    }
    if (!*DestPath)
    {
        fprintf(stderr, "The mandatory output directory is missing.\n");
        return -1;
    }
    if (i >= argc)
    {
        fprintf(stderr, "Not enough parameters, or the list of INF files is missing.\n");
        return -1;
    }

    /* Initialize the registry */
    RegInitializeRegistry(HiveList);

    /* Default to failure */
    ret = -1;

    /* Now we should have the list of INF files: parse it */
    for (; i < argc; ++i)
    {
        convert_path(FileName, argv[i]);
        if (!ImportRegistryFile(FileName))
            goto Quit;
    }

    for (i = 0; i < MAX_NUMBER_OF_REGISTRY_HIVES; ++i)
    {
        /* Skip this registry hive if it's not in the list */
        if (!strstr(HiveList, RegistryHives[i].HiveName))
            continue;

        strcpy(FileName, DestPath);
        strcat(FileName, DIR_SEPARATOR_STRING);
        strcat(FileName, RegistryHives[i].HiveName);

        /* Exception for the special setup registry hive */
        // if (strcmp(RegistryHives[i].HiveName, "SETUPREG") == 0)
        if (i == 0)
            strcat(FileName, ".HIV");

        if (!ExportBinaryHive(FileName, RegistryHives[i].CmHive))
            goto Quit;

        /* If we happen to deal with the special setup registry hive, stop there */
        // if (strcmp(RegistryHives[i].HiveName, "SETUPREG") == 0)
        if (i == 0)
            break;
    }

    /* Success */
    ret = 0;

Quit:
    /* Shut down the registry */
    RegShutdownRegistry();

    if (ret == 0)
        printf("  Done.\n");

    return ret;
}

/* EOF */
