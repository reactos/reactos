/*
 *  ReactOS test program - 
 *
 *  main.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"


#define VERSION 1

#ifdef UNICODE
#define TARGET  "UNICODE"
#else
#define TARGET  "MBCS"
#endif

BOOL verbose_flagged = 0;
BOOL status_flagged = 0;

int usage(char* argv0)
{
    printf("USAGE: %s test_id [unicode]|[ansi] [clean]|[status][verbose]\n", argv0);
    printf("\tWhere test_id is one of:\n");
    printf("\t0 - (default) regression mode, run tests 1-4 displaying failures only\n");
    printf("\t1 - Write DOS style eol data to file in text mode (text.dos)\n");
    printf("\t2 - Write NIX style eol data to file in binary mode (binary.dos)\n");
    printf("\t3 - Write DOS style eol data to file in text mode (text.nix)\n");
    printf("\t4 - Write NIX style eol data to file in binary mode (binary.nix)\n");
    printf("\t5 - Echo console line input\n");
    printf("\t6 - Dump console line input in hex format\n");
    printf("\t7 - The source code is your friend\n");
    printf("\t[unicode] - perform tests using UNICODE versions of library functions\n");
    printf("\t[ansi] - perform tests using ANSI versions of library functions\n");
    printf("\t    If neither unicode or ansi is specified build default is used\n");
    printf("\t[clean] - delete all temporary test output files\n");
    printf("\t[status] - enable extra status display while running\n");
    printf("\t[verbose] - enable verbose output when running\n");
    return 0;
}

int __cdecl main(int argc, char* argv[])
{
    int test_num = 0;
    int version = 0;
    int result = 0;
    int i = 0;

    printf("%s test application - build %03d (default: %s)\n", argv[0], VERSION, TARGET);
    if (argc < 2) {
        return usage(argv[0]);
    }
    for (i = 1; i < argc; i++) {
        if (strstr(argv[i], "ansi") || strstr(argv[i], "ANSI")) {
            version = 1;
        } else if (strstr(argv[i], "unicode") || strstr(argv[i], "UNICODE")) {
            version = 2;
        } else if (strstr(argv[i], "clean") || strstr(argv[i], "CLEAN")) {
            test_num = -1;
        } else if (strstr(argv[i], "verbose") || strstr(argv[i], "VERBOSE")) {
            verbose_flagged = 1;
        } else if (strstr(argv[i], "status") || strstr(argv[i], "STATUS")) {
            status_flagged = 1;
        } else {
            test_num = atoi(argv[1]);
            //if (test_num < 0
        }
    }
    for (i = test_num; i <= test_num; i++) {
        if (!test_num) {
            test_num = 4;
            i = 1;
        }
        switch (version) {
        case 1:
            result = run_ansi_tests(i);
            break;
        case 2:
            result = run_unicode_tests(i);
            break;
        default:
            result = run_ansi_tests(i);
            result = run_unicode_tests(i);
            break;
        }
    }
    printf("finished\n");
    return result;
}

#ifndef __GNUC__

char* args[] = { "fileio.exe", "0", "unicode", "verbose"};

int __cdecl mainCRTStartup(void)
{
    main(2, args);
    return 0;
}

#endif /*__GNUC__*/
