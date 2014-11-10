/*
 * Extract - Wine-compatible program for extract *.cab files.
 *
 * Copyright 2007 Etersoft (Lyutin Anatoly)
 * Copyright 2009 Ilya Shpigor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <wine/unicode.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(extrac32);

static BOOL force_mode;
static BOOL show_content;

static void create_target_directory(LPWSTR Target)
{
    WCHAR dir[MAX_PATH];
    int res;

    strcpyW(dir, Target);
    *PathFindFileNameW(dir) = 0; /* Truncate file name */
    if(!PathIsDirectoryW(dir))
    {
        res = SHCreateDirectoryExW(NULL, dir, NULL);
        if(res != ERROR_SUCCESS && res != ERROR_ALREADY_EXISTS)
            WINE_ERR("Can't create directory: %s\n", wine_dbgstr_w(dir));
    }
}

static UINT WINAPI ExtCabCallback(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2)
{
    FILE_IN_CABINET_INFO_W *pInfo;
    FILEPATHS_W *pFilePaths;

    switch(Notification)
    {
        case SPFILENOTIFY_FILEINCABINET:
            pInfo = (FILE_IN_CABINET_INFO_W*)Param1;
            if(show_content)
            {
                FILETIME ft;
                SYSTEMTIME st;
                CHAR date[12], time[12], buf[2 * MAX_PATH];
                int count;
                DWORD dummy;

                /* DosDate and DosTime already represented at local time */
                DosDateTimeToFileTime(pInfo->DosDate, pInfo->DosTime, &ft);
                FileTimeToSystemTime(&ft, &st);
                GetDateFormatA(0, 0, &st, "MM'-'dd'-'yyyy", date, sizeof date);
                GetTimeFormatA(0, 0, &st, "HH':'mm':'ss", time, sizeof time);
                count = wsprintfA(buf, "%s %s %c%c%c%c %15u %S\n", date, time,
                        pInfo->DosAttribs & FILE_ATTRIBUTE_ARCHIVE  ? 'A' : '-',
                        pInfo->DosAttribs & FILE_ATTRIBUTE_HIDDEN   ? 'H' : '-',
                        pInfo->DosAttribs & FILE_ATTRIBUTE_READONLY ? 'R' : '-',
                        pInfo->DosAttribs & FILE_ATTRIBUTE_SYSTEM   ? 'S' : '-',
                        pInfo->FileSize, pInfo->NameInCabinet);
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, count, &dummy, NULL);
                return FILEOP_SKIP;
            }
            else
            {
                lstrcpyW(pInfo->FullTargetName, (LPCWSTR)Context);
                lstrcatW(pInfo->FullTargetName, pInfo->NameInCabinet);
                /* SetupIterateCabinet() doesn't create full path to target by itself,
                   so we should do it manually */
                create_target_directory(pInfo->FullTargetName);
                return FILEOP_DOIT;
            }
        case SPFILENOTIFY_FILEEXTRACTED:
            pFilePaths = (FILEPATHS_W*)Param1;
            WINE_TRACE("Extracted %s\n", wine_dbgstr_w(pFilePaths->Target));
            return NO_ERROR;
    }
    return NO_ERROR;
}

static void extract(LPCWSTR cabfile, LPWSTR destdir)
{
    if (!SetupIterateCabinetW(cabfile, 0, ExtCabCallback, destdir))
        WINE_ERR("Could not extract cab file %s\n", wine_dbgstr_w(cabfile));
}

static void copy_file(LPCWSTR source, LPCWSTR destination)
{
    WCHAR destfile[MAX_PATH];

    /* append source filename if destination is a directory */
    if (PathIsDirectoryW(destination))
    {
        PathCombineW(destfile, destination, PathFindFileNameW(source));
        destination = destfile;
    }

    if (PathFileExistsW(destination) && !force_mode)
    {
        static const WCHAR overwriteMsg[] = {'O','v','e','r','w','r','i','t','e',' ','"','%','s','"','?',0};
        static const WCHAR titleMsg[] = {'E','x','t','r','a','c','t',0};
        WCHAR msg[MAX_PATH+100];
        snprintfW(msg, sizeof(msg)/sizeof(msg[0]), overwriteMsg, destination);
        if (MessageBoxW(NULL, msg, titleMsg, MB_YESNO | MB_ICONWARNING) != IDYES)
            return;
    }

    WINE_TRACE("copying %s to %s\n", wine_dbgstr_w(source), wine_dbgstr_w(destination));
    CopyFileW(source, destination, FALSE);
}

static LPWSTR *get_extrac_args(LPWSTR cmdline, int *pargc)
{
    enum {OUTSIDE_ARG, INSIDE_ARG, INSIDE_QUOTED_ARG} state;
    LPWSTR str;
    int argc;
    LPWSTR *argv;
    int max_argc = 16;
    BOOL new_arg;

    WINE_TRACE("cmdline: %s\n", wine_dbgstr_w(cmdline));
    str = HeapAlloc(GetProcessHeap(), 0, (strlenW(cmdline) + 1) * sizeof(WCHAR));
    if(!str) return NULL;
    strcpyW(str, cmdline);
    argv = HeapAlloc(GetProcessHeap(), 0, (max_argc + 1) * sizeof(LPWSTR));
    if(!argv)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return NULL;
    }

    /* Split command line to separate arg-strings and fill argv */
    state = OUTSIDE_ARG;
    argc = 0;
    while(*str)
    {
        new_arg = FALSE;
        /* Check character */
        if(isspaceW(*str))          /* white space */
        {
            if(state == INSIDE_ARG)
            {
                state = OUTSIDE_ARG;
                *str = 0;
            }
        }
        else if(*str == '"')        /* double quote */
            switch(state)
            {
                case INSIDE_QUOTED_ARG:
                    state = OUTSIDE_ARG;
                    *str = 0;
                    break;
                case INSIDE_ARG:
                    *str = 0;
                    /* Fall through */
                case OUTSIDE_ARG:
                    if(!*++str) continue;
                    state = INSIDE_QUOTED_ARG;
                    new_arg = TRUE;
                    break;
            }
        else                        /* regular character */
            if(state == OUTSIDE_ARG)
            {
                state = INSIDE_ARG;
                new_arg = TRUE;
            }

        /* Add new argv entry, if need */
        if(new_arg)
        {
            if(argc >= max_argc - 1)
            {
                /* Realloc argv here because there always should be
                   at least one reserved cell for terminating NULL */
                max_argc *= 2;
                argv = HeapReAlloc(GetProcessHeap(), 0, argv,
                        (max_argc + 1) * sizeof(LPWSTR));
                if(!argv)
                {
                    HeapFree(GetProcessHeap(), 0, str);
                    return NULL;
                }
            }
            argv[argc++] = str;
        }

        str++;
    }

    argv[argc] = NULL;
    *pargc = argc;

    if(TRACE_ON(extrac32))
    {
        int i;
        for(i = 0; i < argc; i++)
            WINE_TRACE("arg %d: %s\n", i, wine_dbgstr_w(argv[i]));
    }
    return argv;
}

int PASCAL wWinMain(HINSTANCE hInstance, HINSTANCE prev, LPWSTR cmdline, int show)
{
    LPWSTR *argv;
    int argc;
    int i;
    WCHAR check, cmd = 0;
    WCHAR path[MAX_PATH];
    LPCWSTR cabfile = NULL;

    path[0] = 0;

    /* Do not use CommandLineToArgvW() or __wgetmainargs() to parse
     * command line for this program. It should treat each quote as argument
     * delimiter. This doesn't match with behavior of mentioned functions.
     * Do not use args provided by wmain() for the same reason.
     */
    argv = get_extrac_args(cmdline, &argc);

    if(!argv)
    {
        WINE_ERR("Command line parsing failed\n");
        return 0;
    }

    /* Parse arguments */
    for(i = 0; i < argc; i++)
    {
        /* Get cabfile */
        if (argv[i][0] != '/' && argv[i][0] != '-')
        {
            if (!cabfile)
            {
                cabfile = argv[i];
                continue;
            } else
                break;
        }
        /* Get parameters for commands */
        check = toupperW( argv[i][1] );
        switch(check)
        {
            case 'A':
                WINE_FIXME("/A not implemented\n");
                break;
            case 'Y':
                force_mode = TRUE;
                break;
            case 'L':
                if ((i + 1) >= argc) return 0;
                if (!GetFullPathNameW(argv[++i], MAX_PATH, path, NULL))
                    return 0;
                break;
            case 'C':
            case 'E':
            case 'D':
                if (cmd) return 0;
                cmd = check;
                break;
            default:
                return 0;
        }
    }

    if (!cabfile)
        return 0;

    if (cmd == 'C')
    {
        if ((i + 1) != argc) return 0;
        if (!GetFullPathNameW(argv[i], MAX_PATH, path, NULL))
            return 0;
    }
    else if (!cmd)
        /* Use extraction by default if names of required files presents */
        cmd = i < argc ? 'E' : 'D';

    if (cmd == 'E' && !path[0])
        GetCurrentDirectoryW(MAX_PATH, path);

    PathAddBackslashW(path);

    /* Execute the specified command */
    switch(cmd)
    {
        case 'C':
            /* Copy file */
            copy_file(cabfile, path);
            break;
        case 'D':
            /* Display CAB archive */
            show_content = TRUE;
            /* Fall through */
        case 'E':
            /* Extract CAB archive */
            extract(cabfile, path);
            break;
    }
    return 0;
}
