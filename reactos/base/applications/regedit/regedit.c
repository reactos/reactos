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

#include <regedit.h>


static const char *usage =
    "Usage:\n"
    "    regedit filenames\n"
    "    regedit /E filename [regpath]\n"
    "    regedit /D regpath\n"
    "\n"
    "filenames - List of registry files names\n"
    "filename  - Registry file name\n"
    "regpath   - Name of the registry key\n"
    "\n"
    "When is called without any switches adds contents of the specified\n"
    "registry files to the registry.\n"
    "\n"
    "Switches:\n"
    "    /E - Exports contents of the specified registry key to the specified\n"
    "         file. Exports the whole registry if no key is specified.\n"
    "    /D - Deletes specified registry key\n"
    "    /S - Silent execution, can be used with any other switch.\n"
    "         The only existing mode, exists for compatibility with Windows regedit.\n"
    "    /V - Advanced mode, can be used with any other switch.\n"
    "         Ignored, exists for compatibility with Windows regedit.\n"
    "    /L - Location of system.dat file. Can be used with any other switch.\n"
    "         Ignored. Exists for compatibility with Windows regedit.\n"
    "    /R - Location of user.dat file. Can be used with any other switch.\n"
    "         Ignored. Exists for compatibility with Windows regedit.\n"
    "    /? - Print this help. Any other switches are ignored.\n"
    "    /C - Create registry from. Not implemented.\n"
    "\n"
    "The switches are case-insensitive, can be prefixed either by '-' or '/'.\n"
    "This program is command-line compatible with Microsoft Windows\n"
    "regedit.\n";

typedef enum
{
    ACTION_UNDEF, ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;


const CHAR *getAppName(void)
{
    return "regedit";
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
void get_file_name(LPTSTR *command_line, LPTSTR file_name)
{
    TCHAR *s = *command_line;
    int pos = 0; /* position of pointer "s" in *command_line */
    file_name[0] = 0;

    if (!s[0])
    {
        return;
    }

    if (s[0] == _T('"'))
    {
        s++;
        (*command_line)++;
        while(s[0] != _T('"'))
        {
            if (!s[0])
            {
                fprintf(stderr, "%s: Unexpected end of file name!\n", getAppName());
                exit(1);
            }
            s++;
            pos++;
        }
    }
    else
    {
        while(s[0] && !_istspace(s[0]))
        {
            s++;
            pos++;
        }
    }
    memcpy(file_name, *command_line, pos * sizeof(WCHAR));
    /* remove the last backslash */
    if (file_name[pos - 1] == _T('\\'))
    {
        file_name[pos - 1] = _T('\0');
    }
    else
    {
        file_name[pos] = _T('\0');
    }

    if (s[0])
    {
        s++;
        pos++;
    }
    while(s[0] && _istspace(s[0]))
    {
        s++;
        pos++;
    }
    (*command_line) += pos;
}

BOOL PerformRegAction(REGEDIT_ACTION action, LPTSTR s, BOOL silent)
{
    switch (action)
    {
        case ACTION_ADD:
        {
            TCHAR szTitle[512], szText[512];
            TCHAR filename[MAX_PATH];
            FILE *fp;

            get_file_name(&s, filename);
            if (!filename[0])
            {
                fprintf(stderr, "%s: No file name is specified\n", getAppName());
                fprintf(stderr, usage);
                exit(4);
            }

            LoadString(hInst, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));

            while (filename[0])
            {
                /* Request import confirmation */
                if (!silent)
                {
                    LoadString(hInst, IDS_IMPORT_PROMPT, szText, COUNT_OF(szText));

                    if (InfoMessageBox(NULL, MB_YESNO | MB_ICONWARNING, szTitle, szText, filename) != IDYES)
                        goto cont;
                }

                fp = _tfopen(filename, _T("r"));
                if (fp != NULL)
                {
                    import_registry_file(fp);

                    /* Show successful import */
                    if (!silent)
                    {
                        LoadString(hInst, IDS_IMPORT_OK, szText, COUNT_OF(szText));
                        InfoMessageBox(NULL, MB_OK | MB_ICONINFORMATION, szTitle, szText, filename);
                    }

                    fclose(fp);
                }
                else
                {
                    LPSTR p = GetMultiByteString(filename);
                    perror("");
                    fprintf(stderr, "%s: Can't open file \"%s\"\n", getAppName(), p);
                    HeapFree(GetProcessHeap(), 0, p);

                    /* Error opening the file */
                    if (!silent)
                    {
                        LoadString(hInst, IDS_IMPORT_ERROR, szText, COUNT_OF(szText));
                        InfoMessageBox(NULL, MB_OK | MB_ICONERROR, szTitle, szText, filename);
                    }
                }

cont:
                get_file_name(&s, filename);
            }
            break;
        }

        case ACTION_DELETE:
        {
            TCHAR reg_key_name[KEY_MAX_LEN];
            get_file_name(&s, reg_key_name);
            if (!reg_key_name[0])
            {
                fprintf(stderr, "%s: No registry key is specified for removal\n", getAppName());
                fprintf(stderr, usage);
                exit(6);
            }
            delete_registry_key(reg_key_name);
            break;
        }

        case ACTION_EXPORT:
        {
            TCHAR filename[MAX_PATH];

            filename[0] = _T('\0');
            get_file_name(&s, filename);
            if (!filename[0])
            {
                fprintf(stderr, "%s: No file name is specified\n", getAppName());
                fprintf(stderr, usage);
                exit(7);
            }

            if (s[0])
            {
                TCHAR reg_key_name[KEY_MAX_LEN];
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
            fprintf(stderr, "%s: Unhandled action!\n", getAppName());
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
        fprintf(stderr, "%s: Undefined switch /%c!\n", getAppName(), chu);
    }
    else
    {
        fprintf(stderr, "%s: Alphabetic character is expected after '%c' "
                        "in switch specification\n", getAppName(), *(s - 1));
    }
    exit(1);
}

BOOL ProcessCmdLine(LPTSTR lpCmdLine)
{
    BOOL silent = FALSE;
    REGEDIT_ACTION action = ACTION_UNDEF;
    LPTSTR s = lpCmdLine;       /* command line pointer */
    TCHAR ch = *s;              /* current character */

    while (ch && ((ch == _T('-')) || (ch == _T('/'))))
    {
        TCHAR chu;
        TCHAR ch2;

        s++;
        ch = *s;
        ch2 = *(s + 1);
        chu = _totupper(ch);
        if (!ch2 || _istspace(ch2))
        {
            if (chu == _T('S'))
            {
                /* Silence dialogs */
                silent = TRUE;
            }
            else if (chu == _T('V'))
            {
                /* Ignore this switch */
            }
            else
            {
                switch (chu)
                {
                    case _T('D'):
                        action = ACTION_DELETE;
                        break;
                    case _T('E'):
                        action = ACTION_EXPORT;
                        break;
                    case _T('?'):
                        fprintf(stderr, usage);
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
            if (ch2 == _T(':'))
            {
                switch (chu)
                {
                    case _T('L'):
                        /* fall through */
                    case _T('R'):
                        s += 2;
                        while (*s && !_istspace(*s))
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
        while (ch && _istspace(ch))
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
