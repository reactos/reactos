/* $Id: regdump.c,v 1.2 2002/11/24 19:13:40 robd Exp $
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


#define MAX_REGKEY_NAME_LEN    500
#define MAX_REG_DATA_SIZE 500
#define REG_FILE_HEX_LINE_LEN   76

#ifdef UNICODE
//#define dprintf _tprintf
#define dprintf printf
#else
#define dprintf printf
#endif

/*
char dprintf_buffer[1024];

void dprintf(char* fmt, ...)
{
    int msg_len;
    va_list args;

    va_start(args, fmt);
    vsprintf(dprintf_buffer, fmt, args);
    //_vstprintf(dprintf_buffer, fmt, args);
    msg_len = strlen(dprintf_buffer)
    WriteConsoleA(OutputHandle, dprintf_buffer, msg_len, NULL, NULL);
    va_end(args);
}
 */
BOOL _DumpRegKey(TCHAR* KeyPath, HKEY hKey)
{ 
    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode;

        //_tprintf(_T("_DumpRegKey() - Calling RegOpenKeyEx(%x)\n"), hKey);
        //
        // TODO: this is freezing ROS in bochs hard
        errCode = RegOpenKeyEx(hKey, NULL, 0, KEY_READ, &hNewKey);
        //
        //errCode = RegOpenKeyExA(hKey, NULL, 0, KEY_READ, &hNewKey);

        //_tprintf(_T("RegOpenKeyEx returned %x\n"), errCode);

        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_REGKEY_NAME_LEN];
            DWORD cName = MAX_REGKEY_NAME_LEN;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
			TCHAR* pKeyName = &KeyPath[_tcslen(KeyPath)];

            //_tprintf(_T("Calling RegEnumKeyEx(%x)\n"), hNewKey);

            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
				HKEY hSubKey;
                DWORD dwCount = 0L;
                //int len;
				_tcscat(KeyPath, _T("\\"));
				_tcscat(KeyPath, Name);
                _tprintf(_T("[%s]\n"), KeyPath);
#if 1
                //_tprintf(_T("Calling RegOpenKeyEx\n"));

                errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hSubKey);
                if (errCode == ERROR_SUCCESS) {
#if 1
                    BYTE Data[MAX_REG_DATA_SIZE];
                    DWORD cbData = MAX_REG_DATA_SIZE;
                    TCHAR ValueName[MAX_REGKEY_NAME_LEN];
                    DWORD cValueName = MAX_REGKEY_NAME_LEN;
                    DWORD Type;

                    //_tprintf(_T("Calling RegEnumValue\n"));

                    while (RegEnumValue(hSubKey, dwCount, ValueName, &cValueName, NULL, &Type, Data, &cbData) == ERROR_SUCCESS) {
                        //dprintf("\t%S (%d) %d data bytes\n", ValueName, Type, cbData);
                        _tprintf(_T("\t%s = "), ValueName);
////
            switch (Type) {
            case REG_EXPAND_SZ:
                _tprintf(_T("expand:"));
            case REG_SZ:
                _tprintf(_T("\"%s\"\n"), Data);
                break;
            case REG_DWORD:
                _tprintf(_T("dword:%08lx\n"), *((DWORD *)Data));
                break;
            default:
                _tprintf(_T("warning - unsupported registry format '%ld', ") \
                         _T("treat as binary\n"), Type);
                _tprintf(_T("key name: \"%s\"\n"), Name);
                _tprintf(_T("value name:\"%s\"\n\n"), ValueName);
                /* falls through */
            case REG_MULTI_SZ:
                /* falls through */
            case REG_BINARY:
            {
                DWORD i1;
                TCHAR *hex_prefix;
                TCHAR buf[20];
                int cur_pos;

                if (Type == REG_BINARY) {
                    hex_prefix = _T("hex:");
                } else {
                    hex_prefix = buf;
                    _stprintf(buf, _T("hex(%ld):"), Type);
                }

                /* position of where the next character will be printed */
                /* NOTE: yes, _tcslen("hex:") is used even for hex(x): */
                cur_pos = _tcslen(_T("\"\"=")) + _tcslen(_T("hex:")) + _tcslen(ValueName);

                //dprintf(hex_prefix);
                //_tprintf(hex_prefix);

                for (i1 = 0; i1 < cbData; i1++) {
                    _tprintf(_T("%02x"), (unsigned int)(Data)[i1]);
                    if (i1 + 1 < cbData) {
                        _tprintf(_T(","));
                }
                    cur_pos += 3;
                    /* wrap the line */
                    if (cur_pos > REG_FILE_HEX_LINE_LEN) {
                        _tprintf(_T("\\\n  "));
                        cur_pos = 2;
                    }
                }
                _tprintf(_T("\n"));
                break;
            }
            }
////
                        cValueName = MAX_REGKEY_NAME_LEN;
                        cbData = MAX_REG_DATA_SIZE;
                        ++dwCount;
                    }
#endif
#if 1
                    _DumpRegKey(KeyPath, hSubKey);
#endif
                    RegCloseKey(hSubKey);
                }
#endif
                cName = MAX_REGKEY_NAME_LEN;
				*pKeyName = _T('\0');
				++dwIndex;
            }
            RegCloseKey(hNewKey);
        } else {
            _tprintf(_T("_DumpRegKey(...) RegOpenKeyEx() failed.\n\n"));
        }
    } else {
        _tprintf(_T("_DumpRegKey(...) - NULL key parameter passed.\n\n"));
    }
    return TRUE;
} 

BOOL DumpRegKey(TCHAR* KeyPath, HKEY hKey)
{ 
    _tprintf(_T("\n[%s]\n"), KeyPath);
    //_tprintf(_T("Calling _DumpRegKey(..., %x))\n", hKey);
    return _DumpRegKey(KeyPath, hKey);
}

void RegKeyPrint(int which)
{
	TCHAR szKeyPath[1000];

        switch (which) {
        case '1':
            _tcscpy(szKeyPath, _T("HKEY_CLASSES_ROOT"));
            DumpRegKey(szKeyPath, HKEY_CLASSES_ROOT);
            break;
        case '2':
            _tcscpy(szKeyPath, _T("HKEY_CURRENT_USER"));
            DumpRegKey(szKeyPath, HKEY_CURRENT_USER);
            break;
        case '3':
            _tcscpy(szKeyPath, _T("HKEY_LOCAL_MACHINE"));
            DumpRegKey(szKeyPath, HKEY_LOCAL_MACHINE);
            break;
        case '4':
            _tcscpy(szKeyPath, _T("HKEY_USERS"));
            DumpRegKey(szKeyPath, HKEY_USERS);
            break;
        case '5':
            _tcscpy(szKeyPath, _T("HKEY_CURRENT_CONFIG"));
            DumpRegKey(szKeyPath, HKEY_CURRENT_CONFIG);
            break;
        case '6':
//            DumpRegKey(szKeyPath, HKEY_CLASSES_ROOT);
//            DumpRegKey(szKeyPath, HKEY_CURRENT_USER);
//            DumpRegKey(szKeyPath, HKEY_LOCAL_MACHINE);
//            DumpRegKey(szKeyPath, HKEY_USERS);
//            DumpRegKey(szKeyPath, HKEY_CURRENT_CONFIG);
            _tprintf(_T("unimplemented...\n"));
            break;
		}

}

const char* default_cmd_line1 = "/E HKLM_EXPORT.TXT HKEY_LOCAL_MACHINE";
const char* default_cmd_line2 = "TEST_IMPORT.TXT";
const char* default_cmd_line3 = "/D HKEY_LOCAL_MACHINE\\SYSTEM";
const char* default_cmd_line4 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE";
const char* default_cmd_line5 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes";
const char* default_cmd_line6 = "/E HKCR_EXPORT.TXT HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes";
const char* default_cmd_line7 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE";
const char* default_cmd_line8 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE";
const char* default_cmd_line9 = "/D HKEY_LOCAL_MACHINE\\SOFTWARE";

void show_menu(void)
{
        _tprintf(_T("\nchoose test :\n"));
        _tprintf(_T("  0 = Exit\n"));
        _tprintf(_T("  1 = HKEY_CLASSES_ROOT\n"));
        _tprintf(_T("  2 = HKEY_CURRENT_USER\n"));
        _tprintf(_T("  3 = HKEY_LOCAL_MACHINE\n"));
        _tprintf(_T("  4 = HKEY_USERS\n"));
        _tprintf(_T("  5 = HKEY_CURRENT_CONFIG\n"));
        _tprintf(_T("  6 = REGISTRY ROOT\n"));
        printf("  7 = %s\n", default_cmd_line1);
        printf("  8 = %s\n", default_cmd_line2);
        printf("  9 = %s\n", default_cmd_line3);
        printf("  A = %s\n", default_cmd_line4);
        printf("  B = %s\n", default_cmd_line5);
        printf("  C = %s\n", default_cmd_line6);
        printf("  D = %s\n", default_cmd_line7);
        printf("  E = %s\n", default_cmd_line8);
        printf("  F = %s\n", default_cmd_line9);
/*
        _tprintf(_T("  7 = %s\n"), default_cmd_line1);
        _tprintf(_T("  8 = %s\n"), default_cmd_line2);
        _tprintf(_T("  9 = %s\n"), default_cmd_line3);
        _tprintf(_T("  A = %s\n"), default_cmd_line4);
        _tprintf(_T("  B = %s\n"), default_cmd_line5);
        _tprintf(_T("  C = %s\n"), default_cmd_line6);
        _tprintf(_T("  D = %s\n"), default_cmd_line7);
        _tprintf(_T("  E = %s\n"), default_cmd_line8);
        _tprintf(_T("  F = %s\n"), default_cmd_line9);
 */
}

int regdump(int argc, char* argv[])
{
    char Buffer[500];

	if (argc > 1) {
//		if (0 == _tcsstr(argv[1], _T("HKLM"))) {
        if (strstr(argv[1], "help")) {

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
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
			RegKeyPrint(Buffer[0]/* - '0'*/);
            break;
        case '7':
            strcpy(Buffer, default_cmd_line1);
            goto doit;
        case '8':
            strcpy(Buffer, default_cmd_line2);
            goto doit;
        case '9':
            strcpy(Buffer, default_cmd_line3);
            goto doit;
        case 'A':
            strcpy(Buffer, default_cmd_line4);
            goto doit;
        case 'B':
            strcpy(Buffer, default_cmd_line5);
            goto doit;
        case 'C':
            strcpy(Buffer, default_cmd_line6);
            goto doit;
        case 'D':
            strcpy(Buffer, default_cmd_line7);
            goto doit;
        case 'E':
            strcpy(Buffer, default_cmd_line8);
            goto doit;
        case 'F':
            strcpy(Buffer, default_cmd_line9);
            goto doit;
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
