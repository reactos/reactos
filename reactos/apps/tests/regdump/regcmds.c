/* $Id: regcmds.c,v 1.1 2002/11/24 19:13:40 robd Exp $
 *
 *  ReactOS regedit
 *
 *  regcmds.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  Original Work Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#ifdef WIN32_REGDBG
#else
#include <ctype.h>
#endif

#include "regproc.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static char *usage =
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
"regedit. The difference with Windows regedit - this application has\n"
"command-line interface only.\n";

typedef enum {
    ACTION_UNDEF, ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;

/**
 * Process unknown switch.
 *
 * Params:
 *   chu - the switch character in upper-case.
 *   s - the command line string where s points to the switch character.
 */
void error_unknown_switch(char chu, char *s)
{
    if (isalpha(chu)) {
        printf("Undefined switch /%c!\n", chu);
    } else {
        printf("Alphabetic character is expected after '%c' "
               "in switch specification\n", *(s - 1));
    }
    //exit(1);
}

BOOL PerformRegAction(REGEDIT_ACTION action, LPSTR s)
{
    TCHAR filename[MAX_PATH];
    TCHAR reg_key_name[KEY_MAX_LEN];

    switch (action) {
    case ACTION_ADD:
        get_file_name(&s, filename, MAX_PATH);
        if (!filename[0]) {
            printf("No file name is specified\n%s", usage);
            return FALSE;
            //exit(1);
        }
        while (filename[0]) {
            if (!import_registry_file(filename)) {
                perror("");
                printf("Can't open file \"%s\"\n", filename);
                return FALSE;
                //exit(1);
            }
            get_file_name(&s, filename, MAX_PATH);
        }
        break;
    case ACTION_DELETE:
        get_file_name(&s, reg_key_name, KEY_MAX_LEN);
        if (!reg_key_name[0]) {
            printf("No registry key is specified for removal\n%s", usage);
            return FALSE;
            //exit(1);
        }
        delete_registry_key(reg_key_name);
        break;
    case ACTION_EXPORT:
        filename[0] = '\0';
        get_file_name(&s, filename, MAX_PATH);
        if (!filename[0]) {
            printf("No file name is specified\n%s", usage);
            return FALSE;
            //exit(1);
        }
        if (s[0]) {
            get_file_name(&s, reg_key_name, KEY_MAX_LEN);
            export_registry_key(filename, reg_key_name);
        } else {
            export_registry_key(filename, NULL);
        }
        break;
    default:
        printf("Unhandled action!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL ProcessCmdLine(LPSTR lpCmdLine)
{
    REGEDIT_ACTION action = ACTION_UNDEF;
    LPSTR s = lpCmdLine;        /* command line pointer */
    CHAR ch = *s;               /* current character */

    while (ch && ((ch == '-') || (ch == '/'))) {
        char chu;
        char ch2;

        s++;
        ch = *s;
        ch2 = *(s+1);
        chu = toupper(ch);
        if (!ch2 || isspace(ch2)) {
            if (chu == 'S' || chu == 'V') {
                /* ignore these switches */
            } else {
                switch (chu) {
                case 'D':
                    action = ACTION_DELETE;
                    break;
                case 'E':
                    action = ACTION_EXPORT;
                    break;
                case '?':
                    printf(usage);
                    return FALSE;
                    //exit(0);
                    break;
                default:
                    error_unknown_switch(chu, s);
                    return FALSE;
                    break;
                }
            }
            s++;
        } else {
            if (ch2 == ':') {
                switch (chu) {
                case 'L':
                    /* fall through */
                case 'R':
                    s += 2;
                    while (*s && !isspace(*s)) {
                        s++;
                    }
                    break;
                default:
                    error_unknown_switch(chu, s);
                    return FALSE;
                    break;
                }
            } else {
                /* this is a file name, starting from '/' */
                s--;
                break;
            }
        }
        /* skip spaces to the next parameter */
        ch = *s;
        while (ch && isspace(ch)) {
            s++;
            ch = *s;
        }
    }
    if (action == ACTION_UNDEF) {
        action = ACTION_ADD;
    }
    return PerformRegAction(action, s);
}
