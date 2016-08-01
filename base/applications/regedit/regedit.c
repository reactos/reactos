/*
 * Windows regedit.exe registry editor implementation.
 *
 * Copyright (C) 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "regedit.h"

static const LPCWSTR usage =
    L"Usage:\n"
    L"    regedit filenames\n"
    L"    regedit /E filename [regpath]\n"
    L"    regedit /D regpath\n"
    L"\n"
    L"filenames - List of registry files names\n"
    L"filename  - Registry file name\n"
    L"regpath   - Name of the registry key\n"
    L"\n"
    L"When is called without any switches adds contents of the specified\n"
    L"registry files to the registry.\n"
    L"\n"
    L"Switches:\n"
    L"    /E - Exports contents of the specified registry key to the specified\n"
    L"         file. Exports the whole registry if no key is specified.\n"
    L"    /D - Deletes specified registry key\n"
    L"    /S - Silent execution, can be used with any other switch.\n"
    L"         The only existing mode, exists for compatibility with Windows regedit.\n"
    L"    /V - Advanced mode, can be used with any other switch.\n"
    L"         Ignored, exists for compatibility with Windows regedit.\n"
    L"    /L - Location of system.dat file. Can be used with any other switch.\n"
    L"         Ignored. Exists for compatibility with Windows regedit.\n"
    L"    /R - Location of user.dat file. Can be used with any other switch.\n"
    L"         Ignored. Exists for compatibility with Windows regedit.\n"
    L"    /? - Print this help. Any other switches are ignored.\n"
    L"    /C - Create registry from. Not implemented.\n"
    L"\n"
    L"The switches are case-insensitive, can be prefixed either by '-' or '/'.\n"
    L"This program is command-line compatible with Microsoft Windows\n"
    L"regedit.\n";

typedef enum
{
    ACTION_UNDEF, ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;


LPCWSTR getAppName(void)
{
    return L"regedit";
}

/******************************************************************************
 * Copies file name from command line string to the buffer.
 * Rewinds the command line string pointer to the next non-space character
 * after the file name.
 * Buffer contains an empty string if no filename was found;
 *
 * params:
 * command_line - command line current position pointer
 *      where *s[0] is the first symbol of the file name.
 * file_name - buffer to write the file name to.
 */
void get_file_name(LPWSTR *command_line, LPWSTR file_name)
{
    WCHAR *s = *command_line;
    size_t pos = 0; /* position of pointer "s" in *command_line */
    file_name[0] = 0;

    if (!s[0])
    {
        return;
    }

    if (s[0] == L'"')
    {
        s++;
        (*command_line)++;
        while(s[0] != L'"')
        {
            if (!s[0])
            {
                fwprintf(stderr, L"%s: Unexpected end of file name!\n", getAppName());
                exit(1);
            }
            s++;
            pos++;
        }
    }
    else
    {
        while(s[0] && !iswspace(s[0]))
        {
            s++;
            pos++;
        }
    }
    memcpy(file_name, *command_line, pos * sizeof(WCHAR));
    /* remove the last backslash */
    if (file_name[pos - 1] == L'\\')
    {
        file_name[pos - 1] = L'\0';
    }
    else
    {
        file_name[pos] = L'\0';
    }

    if (s[0])
    {
        s++;
        pos++;
    }
    while(s[0] && iswspace(s[0]))
    {
        s++;
        pos++;
    }
    (*command_line) += pos;
}

BOOL PerformRegAction(REGEDIT_ACTION action, LPWSTR s, BOOL silent)
{
    switch (action)
    {
        case ACTION_ADD:
        {
            WCHAR szText[512];
            WCHAR filename[MAX_PATH];
            FILE *fp;

            get_file_name(&s, filename);
            if (!filename[0])
            {
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, NULL, L"No file name is specified.");
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, usage);
                exit(4);
            }

            while (filename[0])
            {
                /* Request import confirmation */
                if (!silent)
                {
                    int choice;

                    LoadStringW(hInst, IDS_IMPORT_PROMPT, szText, COUNT_OF(szText));

                    choice = InfoMessageBox(NULL, MB_YESNOCANCEL | MB_ICONWARNING, szTitle, szText, filename);

                    switch (choice)
                    {
                        case IDNO:
                            goto cont;
                        case IDCANCEL:
                            /* The cancel case is useful if the user is importing more than one registry file
                            at a time, and wants to back out anytime during the import process. This way, the
                            user doesn't have to resort to ending the regedit process abruptly just to cancel
                            the operation. */
                            return TRUE;
                        default:
                            break;
                    }
                }

                /* Open the file */
                fp = _wfopen(filename, L"r");

                /* Import it */
                if (fp == NULL || !import_registry_file(fp))
                {
                    /* Error opening the file */
                    if (!silent)
                    {
                        LoadStringW(hInst, IDS_IMPORT_ERROR, szText, COUNT_OF(szText));
                        InfoMessageBox(NULL, MB_OK | MB_ICONERROR, szTitle, szText, filename);
                    }
                }
                else
                {
                    /* Show successful import */
                    if (!silent)
                    {
                        LoadStringW(hInst, IDS_IMPORT_OK, szText, COUNT_OF(szText));
                        InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, szText, filename);
                    }
                }

                /* Close the file */
                if (fp) fclose(fp);

cont:
                get_file_name(&s, filename);
            }
            break;
        }

        case ACTION_DELETE:
        {
            WCHAR reg_key_name[KEY_MAX_LEN];
            get_file_name(&s, reg_key_name);
            if (!reg_key_name[0])
            {
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, NULL, L"No registry key is specified for removal.");
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, usage);
                exit(6);
            }
            delete_registry_key(reg_key_name);
            break;
        }

        case ACTION_EXPORT:
        {
            WCHAR filename[MAX_PATH];

            filename[0] = L'\0';
            get_file_name(&s, filename);
            if (!filename[0])
            {
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, NULL, L"No file name is specified.");
                InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, usage);
                exit(7);
            }

            if (s[0])
            {
                WCHAR reg_key_name[KEY_MAX_LEN];
                get_file_name(&s, reg_key_name);
                export_registry_key(filename, reg_key_name, REG_FORMAT_4);
            }
            else
            {
                export_registry_key(filename, NULL, REG_FORMAT_4);
            }
            break;
        }

        default:
            fwprintf(stderr, L"%s: Unhandled action!\n", getAppName());
            exit(8);
            break;
    }

    return TRUE;
}

/**
 * Process unknown switch.
 *
 * Params:
 *   chu - the switch character in upper-case.
 *   s - the command line string where s points to the switch character.
 */
static void error_unknown_switch(WCHAR chu, LPWSTR s)
{
    if (iswalpha(chu))
    {
        fwprintf(stderr, L"%s: Undefined switch /%c!\n", getAppName(), chu);
    }
    else
    {
        fwprintf(stderr, L"%s: Alphabetic character is expected after '%c' "
                          L"in switch specification\n", getAppName(), *(s - 1));
    }
    exit(1);
}

BOOL ProcessCmdLine(LPWSTR lpCmdLine)
{
    BOOL silent = FALSE;
    REGEDIT_ACTION action = ACTION_UNDEF;
    LPWSTR s = lpCmdLine;       /* command line pointer */
    WCHAR ch = *s;              /* current character */

    while (ch && ((ch == L'-') || (ch == L'/')))
    {
        WCHAR chu;
        WCHAR ch2;

        s++;
        ch = *s;
        ch2 = *(s + 1);
        chu = towupper(ch);
        if (!ch2 || iswspace(ch2))
        {
            if (chu == L'S')
            {
                /* Silence dialogs */
                silent = TRUE;
            }
            else if (chu == L'V')
            {
                /* Ignore this switch */
            }
            else
            {
                switch (chu)
                {
                    case L'D':
                        action = ACTION_DELETE;
                        break;
                    case L'E':
                        action = ACTION_EXPORT;
                        break;
                    case L'?':
                        InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, usage);
                        exit(3);
                        break;
                    default:
                        error_unknown_switch(chu, s);
                        break;
                }
            }
            s++;
        }
        else
        {
            if (ch2 == L':')
            {
                switch (chu)
                {
                    case L'L':
                        /* fall through */
                    case L'R':
                        s += 2;
                        while (*s && !iswspace(*s))
                        {
                            s++;
                        }
                        break;
                    default:
                        error_unknown_switch(chu, s);
                        break;
                }
            }
            else
            {
                /* this is a file name, starting from '/' */
                s--;
                break;
            }
        }
        /* skip spaces to the next parameter */
        ch = *s;
        while (ch && iswspace(ch))
        {
            s++;
            ch = *s;
        }
    }

    if (*s && action == ACTION_UNDEF)
        action = ACTION_ADD;

    if (action != ACTION_UNDEF)
        return PerformRegAction(action, s, silent);
    else
        return FALSE;
}
