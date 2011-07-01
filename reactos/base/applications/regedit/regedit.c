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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <regedit.h>


static const char *usage =
    "Usage:\n"
    "    regedit filename\n"
    "    regedit /E filename [regpath]\n"
    "    regedit /D regpath\n"
    "\n"
    "filename - registry file name\n"
    "regpath - name of the registry key\n"
    "\n"
    "When is called without any switches adds contents of the specified\n"
    "registry file to the registry\n"
    "\n"
    "Switches:\n"
    "    /E - exports contents of the specified registry key to the specified\n"
    "	file. Exports the whole registry if no key is specified.\n"
    "    /D - deletes specified registry key\n"
    "    /S - silent execution, can be used with any other switch.\n"
    "	The only existing mode, exists for compatibility with Windows regedit.\n"
    "    /V - advanced mode, can be used with any other switch.\n"
    "	Ignored, exists for compatibility with Windows regedit.\n"
    "    /L - location of system.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /R - location of user.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /? - print this help. Any other switches are ignored.\n"
    "    /C - create registry from. Not implemented.\n"
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
void get_file_name(LPWSTR *command_line, LPWSTR file_name)
{
    WCHAR *s = *command_line;
    int pos = 0;                /* position of pointer "s" in *command_line */
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
                fprintf(stderr, "%s: Unexpected end of file name!\n", getAppName());
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
    memcpy(file_name, *command_line, pos * sizeof((*command_line)[0]));
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

BOOL PerformRegAction(REGEDIT_ACTION action, LPWSTR s)
{
    TCHAR szTitle[256], szText[256];
    switch (action)
    {
    case ACTION_ADD:
    {
        WCHAR filename[MAX_PATH];
        FILE *fp;

        get_file_name(&s, filename);
        if (!filename[0])
        {
            fprintf(stderr, "%s: No file name is specified\n", getAppName());
            fprintf(stderr, usage);
            exit(4);
        }

        while(filename[0])
        {
            fp = _wfopen(filename, L"r");
            if (fp == NULL)
            {
                LPSTR p = GetMultiByteString(filename);
                perror("");
                fprintf(stderr, "%s: Can't open file \"%s\"\n", getAppName(), p);
                HeapFree(GetProcessHeap(), 0, p);
                exit(5);
            }
            import_registry_file(fp);
            get_file_name(&s, filename);
            LoadString(hInst, IDS_APP_TITLE, szTitle, sizeof(szTitle));
            LoadString(hInst, IDS_IMPORTED_OK, szText, sizeof(szTitle));
            /* show successful import */
            MessageBox(NULL, szText, szTitle, MB_OK);
        }
        break;
    }
    case ACTION_DELETE:
    {
        WCHAR reg_key_name[KEY_MAX_LEN];
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
        WCHAR filename[MAX_PATH];

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
                "in swit ch specification\n", getAppName(), *(s - 1));
    }
    exit(1);
}

BOOL ProcessCmdLine(LPWSTR lpCmdLine)
{
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
        chu = (WCHAR)towupper(ch);
        if (!ch2 || iswspace(ch2))
        {
            if (chu == L'S' || chu == L'V')
            {
                /* ignore these switches */
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
	    {
         TCHAR szTitle[256], szText[256];
         LoadString(hInst, IDS_APP_TITLE, szTitle, sizeof(szTitle));
         LoadString(hInst, IDS_IMPORT_PROMPT, szText, sizeof(szTitle));	
         /* request import confirmation */
	     if (MessageBox(NULL, szText, szTitle, MB_YESNO) == IDYES) 
	     {
          action = ACTION_ADD;
         }
		 else return TRUE;
        }
	if (action == ACTION_UNDEF)
        return FALSE;

    return PerformRegAction(action, s);
}
