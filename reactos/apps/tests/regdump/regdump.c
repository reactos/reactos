/* $Id: regdump.c,v 1.3 2003/01/01 11:16:18 robd Exp $
 *
 *  ReactOS regedit
 *
 *  regdump.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "regdump.h"


#ifdef UNICODE
//#define dprintf _tprintf
#define dprintf printf
#else
#define dprintf printf
#endif

void RegKeyPrint(int which);



const char* default_cmd_line1 = "/E HKLM_EXPORT.TXT HKEY_LOCAL_MACHINE";
const char* default_cmd_line2 = "TEST_IMPORT.TXT";
const char* default_cmd_line3 = "/P HKEY_LOCAL_MACHINE\\SYSTEM";
const char* default_cmd_line4 = "/P HKEY_LOCAL_MACHINE\\SOFTWARE";
const char* default_cmd_line5 = "/P HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes";
const char* default_cmd_line6 = "/E HKCR_EXPORT.TXT HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes";
const char* default_cmd_line7 = "/D HKEY_LOCAL_MACHINE\\SYSTEM";
const char* default_cmd_line8 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE";
const char* default_cmd_line9 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes";

/* Show usage */
void usage(const char* appName)
{
    fprintf(stderr, "%s: Dump registry key to console\n", appName);
    fprintf(stderr, "%s HKCR | HKCU | HKLM | HKU | HKCC | HKRR\n", appName);
}

void show_menu(void)
{
    _tprintf(_T("\nchoose test :\n"));
    _tprintf(_T("  0 = Exit\n"));
         printf("  1 = %s\n", default_cmd_line1);
         printf("  2 = %s\n", default_cmd_line2);
         printf("  3 = %s\n", default_cmd_line3);
         printf("  4 = %s\n", default_cmd_line4);
         printf("  5 = %s\n", default_cmd_line5);
         printf("  6 = %s\n", default_cmd_line6);
         printf("  7 = %s\n", default_cmd_line7);
         printf("  8 = %s\n", default_cmd_line8);
         printf("  9 = %s\n", default_cmd_line9);
/*
    _tprintf(_T("  1 = %s\n"), default_cmd_line1);
    _tprintf(_T("  2 = %s\n"), default_cmd_line2);
    _tprintf(_T("  3 = %s\n"), default_cmd_line3);
    _tprintf(_T("  4 = %s\n"), default_cmd_line4);
    _tprintf(_T("  5 = %s\n"), default_cmd_line5);
    _tprintf(_T("  6 = %s\n"), default_cmd_line6);
    _tprintf(_T("  7 = %s\n"), default_cmd_line7);
    _tprintf(_T("  8 = %s\n"), default_cmd_line8);
    _tprintf(_T("  9 = %s\n"), default_cmd_line9);
 */
//        _tprintf(_T("  A = HKEY_CLASSES_ROOT\n"));
//        _tprintf(_T("  B = HKEY_CURRENT_USER\n"));
//        _tprintf(_T("  C = HKEY_LOCAL_MACHINE\n"));
//        _tprintf(_T("  D = HKEY_USERS\n"));
//        _tprintf(_T("  E = HKEY_CURRENT_CONFIG\n"));
//        _tprintf(_T("  F = REGISTRY ROOT\n"));
}

int regdump(int argc, char* argv[])
{
    char Buffer[500];

    if (argc > 1) {
//      if (0 == _tcsstr(argv[1], _T("HKLM"))) {
        if (strstr(argv[1], "help")) {
            usage(argv[0]);
        } else if (strstr(argv[1], "HKCR")) {
            RegKeyPrint('1');
        } else if (strstr(argv[1], "HKCU")) {
            RegKeyPrint('2');
        } else if (strstr(argv[1], "HKLM")) {
            RegKeyPrint('3');
        } else if (strstr(argv[1], "HKU")) {
            RegKeyPrint('4');
        } else if (strstr(argv[1], "HKCC")) {
            RegKeyPrint('5');
        } else if (strstr(argv[1], "HKRR")) {
            RegKeyPrint('6');
        } else {
            dprintf("started with argc = %d, argv[1] = %s (unknown?)\n", argc, argv[1]);
        }
        return 0;
    }
    show_menu();
    while (1) {
        GetInput(Buffer, sizeof(Buffer));
        switch (toupper(Buffer[0])) {
        case '0':
            return(0);
        case '1':
            strcpy(Buffer, default_cmd_line1);
            goto doit;
        case '2':
            strcpy(Buffer, default_cmd_line2);
            goto doit;
        case '3':
            strcpy(Buffer, default_cmd_line3);
            goto doit;
        case '4':
            strcpy(Buffer, default_cmd_line4);
            goto doit;
        case '5':
            strcpy(Buffer, default_cmd_line5);
            goto doit;
        case '6':
            strcpy(Buffer, default_cmd_line6);
            goto doit;
        case '7':
            strcpy(Buffer, default_cmd_line7);
            goto doit;
        case '8':
            strcpy(Buffer, default_cmd_line8);
            goto doit;
        case '9':
            strcpy(Buffer, default_cmd_line9);
            goto doit;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            RegKeyPrint(toupper(Buffer[0]) - 'A' + 1);
            break;
        default: doit:
            if (!ProcessCmdLine(Buffer)) {
                dprintf("invalid input.\n");
                show_menu();
            } else {
                dprintf("done.\n");
            }
            break;
        }
    }
    return 0;
}
