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

#include "main.h"


#define VERSION "1.00"
#ifdef UNICODE
#define TARGET  "UNICODE"
#else
#define TARGET  "MBCS"
#endif


#define TEST_BUFFER_SIZE 200

TCHAR test_buffer[TEST_BUFFER_SIZE];
BOOL verbose_flagged = 0;


int usage(char* argv0)
{
    printf("USAGE:\n");
    printf("\t%s clean - delete test output files\n", argv0);
    printf("\t%s test_number [unicode][ansi]\n", argv0);
    return 0;
}

int __cdecl main(int argc, char* argv[])
{
    int test_num = 0;
    int result = 0;

    printf("%s test application - build %s (%s)\n", argv[0], VERSION, TARGET);
    if (argc < 2) {
        return usage(argv[0]);
    }
    if (argc > 1) {
        if (strstr(argv[1], "clean") || strstr(argv[1], "clean")) {
#ifdef UNICODE
            return test_unicode_files(-1);
#else
            return test_ansi_files(-1);
#endif
        }
        test_num = atoi(argv[1]);
    }
    if (argc > 2) {
        if (argc > 3) {
            verbose_flagged = 1;
        }
        if (strstr(argv[2], "ansi") || strstr(argv[2], "ANSI")) {
            result = test_ansi_files(test_num);
        } else if (strstr(argv[2], "unicode") || strstr(argv[2], "UNICODE")) {
            result = test_unicode_files(test_num);
        } else {
            result = test_ansi_files(test_num);
            result = test_unicode_files(test_num);
        }
    } else {
#ifdef UNICODE
        result = test_unicode_files(test_num);
#else
        result = test_ansi_files(test_num);
#endif
    }
    printf("finished\n");
    return result;
}

