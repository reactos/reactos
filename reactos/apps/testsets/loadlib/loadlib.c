/*
 *  ReactOS test program - 
 *
 *  loadlib.c
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

#include <windows.h>
#include "loadlib.h"


#define APP_VERSION 1
#define MAX_LIBS    25

#ifdef UNICODE
#define TARGET  "UNICODE"
BOOL bUseAnsi = FALSE;
#else
#define TARGET  "MBCS"
BOOL bUseAnsi = TRUE;
#endif
BOOL verbose_flagged = FALSE;
BOOL debug_flagged = FALSE;
BOOL loop_flagged = FALSE;
BOOL recursive_flagged = FALSE;

HANDLE OutputHandle;
HANDLE InputHandle;


void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args, fmt);
   wvsprintfA(buffer, fmt, args);
   WriteConsoleA(OutputHandle, buffer, lstrlenA(buffer), NULL, NULL);
   va_end(args);
}

long getinput(char* buf, int buflen)
{
    DWORD result;

    ReadConsoleA(InputHandle, buf, buflen, &result, NULL);
    return (long)result;
}

DWORD ReportLastError(void)
{
    DWORD dwError = GetLastError();
    if (dwError != ERROR_SUCCESS) {
        PSTR msg = NULL;
        if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            0, dwError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PSTR)&msg, 0, NULL)) {
            if (msg != NULL) {
                dprintf("ReportLastError() %d - %s\n", dwError, msg);
            } else {
                dprintf("ERROR: ReportLastError() %d - returned TRUE but with no msg string!\n", dwError);
            }
        } else {
            dprintf("ReportLastError() %d - unknown error\n", dwError);
        }
        if (msg != NULL) {
            LocalFree(msg);
        }
    }
    return dwError;
}

const char* appName(const char* argv0)
{
    const char* name;

    name = (const char*)strrchr(argv0, '\\');
    if (name != NULL) 
        return name + 1;
    return argv0;
}

int usage(const char* appName)
{
    dprintf("USAGE: %s libname [libname ...] [unicode]|[ansi] [loop][recurse]\n", appName);
    dprintf("\tWhere libname(s) is one or more libraries to load.\n");
    dprintf("\t[unicode] - perform tests using UNICODE api calls\n");
    dprintf("\t[ansi] - perform tests using ANSI api calls\n");
    dprintf("\t    default is %s\n", TARGET);
    dprintf("\t[loop] - run test process in continuous loop\n");
    dprintf("\t[recurse] - load libraries recursively rather than sequentually\n");
    dprintf("\t[debug] - enable debug mode (unused)\n");
    dprintf("\t[verbose] - enable verbose output (unused)\n");
    return 0;
}

DWORD LoadLibraryList(char** libnames, int counter, BOOL bUseAnsi)
{
    HMODULE hModule;

    dprintf("Attempting to LoadLibrary");
    if (bUseAnsi) {
        dprintf("A(%s) - ", *libnames);
        hModule = LoadLibraryA(*libnames);
    } else {
        int len;
        wchar_t libnameW[500];
        len = mbstowcs(libnameW, *libnames, strlen(*libnames));
        if (len) {
            libnameW[len] = L'\0';
            dprintf("W(%S) - ", libnameW);
            hModule = LoadLibraryW(libnameW);
        } else {
            return ERROR_INVALID_PARAMETER;
        }
    }
    if (hModule == NULL) {
        dprintf("\nERROR: failed to obtain handle to module %s - %x\n", *libnames, hModule);
        return ReportLastError();
    }
    dprintf("%x\n", hModule);

    if (counter--) {
        LoadLibraryList(++libnames, counter, bUseAnsi);
    }

    if (!FreeLibrary(hModule)) {
        dprintf("ERROR: failed to free module %s - %x\n", *libnames, hModule);
        return ReportLastError();
    } else {
        dprintf("FreeLibrary(%x) - successfull.\n", hModule);
    }
    return 0L;
}

int __cdecl main(int argc, char* argv[])
{
    char* libs[MAX_LIBS];
    int lib_count = 0;
    int test_num = 0;
    int result = 0;
    int i = 0;

    AllocConsole();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

    dprintf("%s application - build %03d (default: %s)\n", appName(argv[0]), APP_VERSION, TARGET);
    if (argc < 2) {
        /*return */usage(appName(argv[0]));
    }
    memset(libs, 0, sizeof(libs));
    for (i = 1; i < argc; i++) {
        if (lstrcmpiA(argv[i], "ansi") == 0) {
            bUseAnsi = TRUE;
        } else if (lstrcmpiA(argv[i], "unicode") == 0) {
            bUseAnsi = FALSE;
        } else if (lstrcmpiA(argv[i], "loop") == 0) {
            loop_flagged = 1;
        } else if (lstrcmpiA(argv[i], "recurse") == 0) {
            recursive_flagged = 1;
        } else if (lstrcmpiA(argv[i], "verbose") == 0) {
            verbose_flagged = 1;
        } else if (lstrcmpiA(argv[i], "debug") == 0) {
            debug_flagged = 1;
        } else {
            if (lib_count < MAX_LIBS) {
                libs[lib_count] = argv[i];
                ++lib_count;
            }
        }
    }
    if (lib_count) {
        do {
            if (recursive_flagged) {
                result = LoadLibraryList(libs, lib_count - 1, bUseAnsi);
            } else {
                for (i = 0; i < lib_count; i++) {
                    result = LoadLibraryList(&libs[i], 0, bUseAnsi);
                    //if (result != 0) break;
                }
            }
        } while (loop_flagged);
    } else {
        int len;
        char buffer[500];
        do {
            dprintf("\nEnter library name to attempt loading: ");
            len = getinput(buffer, sizeof(buffer) - 1);
            if (len > 2) {
                char* buf = buffer;
                buffer[len-2] = '\0';
                result = LoadLibraryList(&buf, 0, bUseAnsi);
            } else break;
        } while (!result && len);
    }
    dprintf("finished\n");
    return result;
}


#ifdef _NOCRT
char* args[] = { "loadlib.exe", "advapi32.dll", "user32.dll", "recurse"};
int __cdecl mainCRTStartup(void)
{
    return main(3, args);
}
#endif /*__GNUC__*/
