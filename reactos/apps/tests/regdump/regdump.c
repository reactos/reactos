#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <ddk/ntddk.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


#define MAX_NAME_LEN    500
/*
BOOL DumpRegKey(TCHAR* KeyPath, HKEY hKey)
{ 
    TCHAR keyPath[1000];
    int keyPathLen = 0;

    keyPath[0] = _T('\0');

	dprintf("\n[%s]\n", KeyPath);

    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
				HKEY hSubKey;
                DWORD dwCount = 0L;

				dprintf("\n[%s\\%s]\n", KeyPath, Name);

                errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hSubKey);
                if (errCode == ERROR_SUCCESS) {
                    TCHAR SubName[MAX_NAME_LEN];
                    DWORD cSubName = MAX_NAME_LEN;
//                    if (RegEnumKeyEx(hSubKey, 0, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    while (RegEnumKeyEx(hSubKey, dwCount, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                        dprintf("\t%s (%d)\n", SubName, dwCount);
						cSubName = MAX_NAME_LEN;
                        ++dwCount;
                    }
                }
                RegCloseKey(hSubKey);

                //AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwCount);
                cName = MAX_NAME_LEN;
                ++dwIndex;
            }
            RegCloseKey(hNewKey);
        }
    } else {
    }
	dprintf("\n");
    return TRUE;
} 
 */

BOOL _DumpRegKey(TCHAR* KeyPath, HKEY hKey)
{ 
    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode = RegOpenKeyEx(hKey, NULL, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
			TCHAR* pKeyName = &KeyPath[_tcslen(KeyPath)];
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
				HKEY hSubKey;
                DWORD dwCount = 0L;
				_tcscat(KeyPath, _T("\\"));
				_tcscat(KeyPath, Name);
				dprintf("\n[%s]\n", KeyPath);
                errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hSubKey);
                if (errCode == ERROR_SUCCESS) {
#if 1
                    _DumpRegKey(KeyPath, hSubKey);
#else
                    TCHAR SubName[MAX_NAME_LEN];
                    DWORD cSubName = MAX_NAME_LEN;
//                    if (RegEnumKeyEx(hSubKey, 0, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    while (RegEnumKeyEx(hSubKey, dwCount, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                        dprintf("\t%s (%d)\n", SubName, dwCount);
						cSubName = MAX_NAME_LEN;
                        ++dwCount;
                    }
#endif
                }
                RegCloseKey(hSubKey);
                cName = MAX_NAME_LEN;
				*pKeyName = _T('\0');
				++dwIndex;
            }
            RegCloseKey(hNewKey);
        }
    } else {
    }
    return TRUE;
} 

BOOL DumpRegKey(TCHAR* KeyPath, HKEY hKey)
{ 
	dprintf("\n[%s]\n", KeyPath);
    return _DumpRegKey(KeyPath, hKey);
}

void RegKeyPrint(int which)
{
	TCHAR szKeyPath[1000];

        switch (which) {
        case '1':
			strcpy(szKeyPath, _T("HKEY_CLASSES_ROOT"));
            DumpRegKey(szKeyPath, HKEY_CLASSES_ROOT);
            break;
        case '2':
			strcpy(szKeyPath, _T("HKEY_CURRENT_USER"));
            DumpRegKey(szKeyPath, HKEY_CURRENT_USER);
            break;
        case '3':
			strcpy(szKeyPath, _T("HKEY_LOCAL_MACHINE"));
            DumpRegKey(szKeyPath, HKEY_LOCAL_MACHINE);
            break;
        case '4':
			strcpy(szKeyPath, _T("HKEY_USERS"));
            DumpRegKey(szKeyPath, HKEY_USERS);
            break;
        case '5':
			strcpy(szKeyPath, _T("HKEY_CURRENT_CONFIG"));
            DumpRegKey(szKeyPath, HKEY_CURRENT_CONFIG);
            break;
        case '6':
//            DumpRegKey(szKeyPath, HKEY_CLASSES_ROOT);
//            DumpRegKey(szKeyPath, HKEY_CURRENT_USER);
//            DumpRegKey(szKeyPath, HKEY_LOCAL_MACHINE);
//            DumpRegKey(szKeyPath, HKEY_USERS);
//            DumpRegKey(szKeyPath, HKEY_CURRENT_CONFIG);
            dprintf("unimplemented...\n");
            break;
		}

}

int main(int argc, char* argv[])
{
    char Buffer[10];
	TCHAR szKeyPath[1000];
    DWORD Result;

    AllocConsole();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

	if (argc > 1) {

//		if (0 == _tcsstr(argv[1], _T("HKLM"))) {

		if (strstr(argv[1], _T("help"))) {

		} else if (strstr(argv[1], _T("HKCR"))) {
			RegKeyPrint('1');
		} else if (strstr(argv[1], _T("HKCU"))) {
			RegKeyPrint('2');
		} else if (strstr(argv[1], _T("HKLM"))) {
			RegKeyPrint('3');
		} else if (strstr(argv[1], _T("HKU"))) {
			RegKeyPrint('4');
		} else if (strstr(argv[1], _T("HKCC"))) {
			RegKeyPrint('5');
		} else if (strstr(argv[1], _T("HKRR"))) {
			RegKeyPrint('6');
		} else {
            dprintf("started with argc = %d, argv[1] = %s (unknown?)\n", argc, argv[1]);
		}

	} else while (1) {
        dprintf("choose test :\n");
        dprintf("  0 = Exit\n");
        dprintf("  1 = HKEY_CLASSES_ROOT\n");
        dprintf("  2 = HKEY_CURRENT_USER\n");
        dprintf("  3 = HKEY_LOCAL_MACHINE\n");
        dprintf("  4 = HKEY_USERS\n");
        dprintf("  5 = HKEY_CURRENT_CONFIG\n");
        dprintf("  6 = REGISTRY ROOT\n");
        ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
        switch (Buffer[0]) {
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
		default:
			dprintf("invalid input.\n");
			break;
		}
	}
    return 0;
}
/*
[HKEY_LOCAL_MACHINE]

[HKEY_LOCAL_MACHINE\HARDWARE]

[HKEY_LOCAL_MACHINE\HARDWARE\ACPI]

[HKEY_LOCAL_MACHINE\HARDWARE\ACPI\DSDT]

[HKEY_LOCAL_MACHINE\HARDWARE\ACPI\DSDT\VIA694]

[HKEY_LOCAL_MACHINE\HARDWARE\ACPI\DSDT\VIA694\AWRDACPI]

 */