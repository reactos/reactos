#include <windows.h>
#include <tchar.h>
#include <stdio.h>

TCHAR szADT[MAX_PATH];

BOOL ParseCommandLine (int argc, char *argv[]);
UINT CreateNestedDirectory(LPCTSTR lpDirectory);
LPTSTR CheckSlash (LPTSTR lpDir);


int __cdecl main( int argc, char *argv[])
{
    LPTSTR lpEnd, lpRoot;
    HANDLE hFile;
    INT i;

    if (!ParseCommandLine (argc, argv)) {
        return 1;
    }

    if (!CreateNestedDirectory (szADT)) {
        return 1;
    }


    lpRoot = CheckSlash (szADT);

    lstrcpy (lpRoot, TEXT("GPT.ini"));
    if (!WritePrivateProfileString (TEXT("General"),
                                    TEXT("GUID"),
                                    TEXT("<GUID goes here>"),
                                    szADT)) {
        return 1;
    }

    if (!WritePrivateProfileString (TEXT("General"),
                                    TEXT("Class Store"),
                                    TEXT(" <Path to a Class Store goes here>"),
                                    szADT)) {
        return 1;
    }

    *(lpRoot - 1) = TEXT('\0');

    for (i=0; i < 2; i++ ) {

        lpRoot = CheckSlash (szADT);

        if (i == 0) {
            lstrcpy (lpRoot, TEXT("User"));
        } else {
            lstrcpy (lpRoot, TEXT("Machine"));
        }

        lpEnd = CheckSlash (szADT);

        lstrcpy (lpEnd, TEXT("Applications\\Assigned\\Alpha"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Applications\\Assigned\\x86\\WinNT"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Applications\\Assigned\\x86\\Win95"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }


        lstrcpy (lpEnd, TEXT("Applications\\Published\\Alpha"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Applications\\Published\\x86\\WinNT"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Applications\\Published\\x86\\Win95"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }


        if (i > 0) {
            lstrcpy (lpEnd, TEXT("OS Upgrades"));
            if (!CreateNestedDirectory (szADT)) {
                return 1;
            }
        }

        lstrcpy (lpEnd, TEXT("Profile"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Power Schemes"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }


        lstrcpy (lpEnd, TEXT("Scripts"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Registry.txt"));
        hFile = CreateFile (szADT, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle (hFile);
        }


        lstrcpy (lpEnd, TEXT("Profile"));
        lpEnd = CheckSlash (szADT);

        lstrcpy (lpEnd, TEXT("Application Data"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Desktop\\My Documents"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Favorites"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("NetHood"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("PrintHood"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Recent"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("SendTo"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Templates"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        lstrcpy (lpEnd, TEXT("Start Menu\\Programs\\Startup"));
        if (!CreateNestedDirectory (szADT)) {
            return 1;
        }

        if (i == 0) {
            lstrcpy (lpRoot, TEXT("User\\Scripts\\Scripts.ini"));
         } else {
            lstrcpy (lpRoot, TEXT("Machine\\Scripts\\Scripts.ini"));
         }

        if (i == 0) {
            if (!WritePrivateProfileString (TEXT("Logon"),
                                            TEXT("CommandLine"),
                                            TEXT(" <Command line goes here>"),
                                            szADT)) {
                return 1;
            }

            if (!WritePrivateProfileString (TEXT("Logoff"),
                                            TEXT("CommandLine"),
                                            TEXT(" <Command line goes here>"),
                                            szADT)) {
                return 1;
            }
        } else {
            if (!WritePrivateProfileString (TEXT("Boot"),
                                            TEXT("CommandLine"),
                                            TEXT(" <Command line goes here>"),
                                            szADT)) {
                return 1;
            }

            if (!WritePrivateProfileString (TEXT("Shutdown"),
                                            TEXT("CommandLine"),
                                            TEXT(" <Command line goes here>"),
                                            szADT)) {
                return 1;
            }
        }


        *(lpRoot - 1) = TEXT('\0');
    }

    _tprintf (TEXT("\r\nThe %s Group Policy Template (GPT) was successfully created.\r\n\r\n"), szADT);
    _tprintf (TEXT("Add this directory to the %%SystemRoot%%\\DS.INI file on each client machine.\r\n"));
    _tprintf (TEXT("The Path entry can have multiple ADT paths each separated by a semi-colon.\r\n"));
    _tprintf (TEXT("The file format is:\r\n\r\n"));
    _tprintf (TEXT("[User ADT]\r\nPath=%s\r\n\r\n"), szADT);

    return 0;
}


BOOL ParseCommandLine (int argc, char *argv[])
{

    if (argc != 2) {
        goto usage;
    }

    if (!lstrcmpi(argv[1], TEXT("/?"))) {
        goto usage;
    }

    if (!lstrcmpi(argv[1], TEXT("-?"))) {
        goto usage;
    }

    lstrcpy (szADT, argv[1]);

    return TRUE;

usage:
    _tprintf (TEXT("\r\nusage:  creategpt <gptpath>\r\n\r\n"));

    return FALSE;
}


UINT CreateNestedDirectory(LPCTSTR lpDirectory)
{
    TCHAR szDirectory[MAX_PATH];
    LPTSTR lpEnd;


    if (CreateDirectory (lpDirectory, NULL)) {
        return 1;
    }


    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    lstrcpy (szDirectory, lpDirectory);


    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {
        lpEnd += 2;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    if (CreateDirectory (szDirectory, NULL)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    return 0;

}

LPTSTR CheckSlash (LPTSTR lpDir)
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}
