/*
 * Windows regedit.exe registry editor implementation.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#ifndef __REACTOS__
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "wine/debug.h"
#include "main.h"
#else
#include "regedit.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(regedit);

static void output_writeconsole(const WCHAR *str, DWORD wlen)
{
#ifdef __REACTOS__
    /* This is win32gui application, don't ever try writing to console.
     * For the console version we have a separate reg.exe application. */
    MessageBoxW(NULL, str, NULL, MB_ICONERROR);
#else
    DWORD count;

    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL))
    {
        DWORD len;
        char  *msgA;

        /* WriteConsole() fails on Windows if its output is redirected. If this occurs,
         * we should call WriteFile() with OEM code page.
         */
        len = WideCharToMultiByte(GetOEMCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        msgA = malloc(len);
        if (!msgA) return;

        WideCharToMultiByte(GetOEMCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        free(msgA);
    }
#endif
}

static void output_formatstring(const WCHAR *fmt, va_list va_args)
{
    WCHAR *str;
    DWORD len;

#ifdef __REACTOS__
    SetLastError(NO_ERROR);
#endif
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
#ifdef __REACTOS__
    if (len == 0 && GetLastError() != NO_ERROR)
#else
    if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
#endif
    {
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    output_writeconsole(str, len);
    LocalFree(str);
}

void WINAPIV output_message(unsigned int id, ...)
{
    WCHAR fmt[1536];
    va_list va_args;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        return;
    }
    va_start(va_args, id);
    output_formatstring(fmt, va_args);
    va_end(va_args);
}

void WINAPIV error_exit(unsigned int id, ...)
{
    WCHAR fmt[1536];
    va_list va_args;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
#ifndef __REACTOS__
        WINE_FIXME("LoadString failed with %lu\n", GetLastError());
#endif
        return;
    }
    va_start(va_args, id);
    output_formatstring(fmt, va_args);
    va_end(va_args);

    exit(0); /* regedit.exe always terminates with error code zero */
}

typedef enum {
    ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;

#ifdef __REACTOS__
static void PerformRegAction(REGEDIT_ACTION action, WCHAR **argv, int *i, BOOL silent)
#else
static void PerformRegAction(REGEDIT_ACTION action, WCHAR **argv, int *i)
#endif
{
    switch (action) {
    case ACTION_ADD: {
            WCHAR *filename = argv[*i];
            WCHAR *realname = NULL;
            FILE *reg_file;

#ifdef __REACTOS__
            /* Request import confirmation */
            if (!silent)
            {
                WCHAR szText[512];
                int choice;
                UINT mbType = MB_YESNO;

                LoadStringW(hInst, IDS_IMPORT_PROMPT, szText, ARRAY_SIZE(szText));

                if (argv[*i + 1] != NULL)
                {
                    /* Enable three buttons if there's another file coming */
                    mbType = MB_YESNOCANCEL;
                }

                choice = InfoMessageBox(NULL, mbType | MB_ICONQUESTION, szTitle, szText, filename);
                switch (choice)
                {
                    case IDNO:
                        return;
                    case IDCANCEL:
                        /* The cancel case is useful if the user is importing more than one registry file
                         * at a time, and wants to back out anytime during the import process. This way, the
                         * user doesn't have to resort to ending the regedit process abruptly just to cancel
                         * the operation.
                         * To achieve this, we skip all further command line arguments.
                         */
                        *i = INT_MAX - 1;
                        return;
                    default:
                        break;
                }
            }
#endif
            if (!lstrcmpW(filename, L"-"))
                reg_file = stdin;
            else
            {
                int size;

                size = SearchPathW(NULL, filename, NULL, 0, NULL, NULL);
                if (size > 0)
                {
                    realname = malloc(size * sizeof(WCHAR));
                    size = SearchPathW(NULL, filename, NULL, size, realname, NULL);
                }
                if (size == 0)
                {
                    output_message(STRING_FILE_NOT_FOUND, filename);
                    free(realname);
                    return;
                }
                reg_file = _wfopen(realname, L"rb");
                if (reg_file == NULL)
                {
                    _wperror(L"regedit");
                    output_message(STRING_CANNOT_OPEN_FILE, filename);
                    free(realname);
                    return;
                }
            }
            import_registry_file(reg_file);
            if (realname)
            {
                free(realname);
                fclose(reg_file);
            }
            break;
        }
    case ACTION_DELETE:
            delete_registry_key(argv[*i]);
            break;
    case ACTION_EXPORT: {
            WCHAR *filename = argv[*i];
            WCHAR *key_name = argv[++(*i)];

            if (key_name && *key_name)
                export_registry_key(filename, key_name, REG_FORMAT_5);
            else
                export_registry_key(filename, NULL, REG_FORMAT_5);
            break;
        }
    default:
#ifdef __REACTOS__
        output_message(STRING_UNHANDLED_ACTION);
#else
        error_exit(STRING_UNHANDLED_ACTION);
#endif
        break;
    }
}

BOOL ProcessCmdLine(WCHAR *cmdline)
{
    WCHAR **argv;
    int argc, i;
    REGEDIT_ACTION action = ACTION_ADD;
#ifdef __REACTOS__
    BOOL silent = FALSE;
#endif

    argv = CommandLineToArgvW(cmdline, &argc);

    if (!argv)
        return FALSE;

    if (argc == 1)
    {
        LocalFree(argv);
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
            break; /* No flags specified. */

        if (!argv[i][1] && argv[i][0] == '-')
            break; /* '-' is a filename. It indicates we should use stdin. */

        if (argv[i][1] && argv[i][2] && argv[i][2] != ':')
            break; /* This is a file path beginning with '/'. */

        switch (towupper(argv[i][1]))
        {
        case '?':
            error_exit(STRING_USAGE);
            break;
        case 'D':
            action = ACTION_DELETE;
            break;
        case 'E':
            action = ACTION_EXPORT;
            break;
        case 'C':
        case 'L':
        case 'M':
        case 'R':
            /* unhandled */;
            break;
        case 'S':
#ifdef __REACTOS__
            silent = TRUE;
            break;
#endif
        case 'V':
            /* ignored */;
            break;
        default:
            output_message(STRING_INVALID_SWITCH, argv[i]);
            error_exit(STRING_HELP);
        }
    }

    if (i == argc)
    {
        switch (action)
        {
        case ACTION_ADD:
        case ACTION_EXPORT:
            output_message(STRING_NO_FILENAME);
            break;
        case ACTION_DELETE:
            output_message(STRING_NO_REG_KEY);
            break;
        }
        error_exit(STRING_HELP);
    }

    for (; i < argc; i++)
#ifdef __REACTOS__
        PerformRegAction(action, argv, &i, silent);
#else
        PerformRegAction(action, argv, &i);
#endif

    LocalFree(argv);

    return TRUE;
}
