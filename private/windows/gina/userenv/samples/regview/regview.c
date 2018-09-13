#include <windows.h>
#include <tchar.h>
#include <stdio.h>


#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512

//
// Verison number for the registry file format
//

#define REGISTRY_FILE_VERSION       1


//
// File signature
//

#define REGFILE_SIGNATURE  0x67655250


BOOL DisplayRegistryData (LPTSTR lpRegistry);


int __cdecl main( int argc, char *argv[])
{
    WCHAR szPath[MAX_PATH * 2];

    if (argc != 2) {
        _tprintf(TEXT("usage:  regview <pathname>\registry.pol"));
        _tprintf(TEXT("example:  regview d:\registry.pol"));
        return 1;
    }


    if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[1], -1, szPath,
                            (MAX_PATH * 2))) {
        _tprintf(TEXT("Failed to convert path to unicode"));
        return 1;
    }

    DisplayRegistryData(szPath);

    return 0;
}


//*************************************************************
//
//  DisplayRegistryData()
//
//  Purpose:    Displays the registry data
//
//  Parameters: lpRegistry  -   Path to registry.pol
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL DisplayRegistryData (LPTSTR lpRegistry)
{
    HANDLE hFile;
    BOOL bResult = FALSE;
    DWORD dwTemp, dwBytesRead, dwType, dwDataLength, dwIndex, dwCount;
    LPWSTR lpKeyName, lpValueName, lpTemp;
    LPBYTE lpData = NULL, lpIndex;
    WCHAR  chTemp;
    INT i;
    CHAR szString[20];


    //
    // Open the registry file
    //

    hFile = CreateFile (lpRegistry, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);


    if (hFile == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            return TRUE;
        }
        else
        {
            _tprintf(TEXT("DisplayRegistryData: CreateFile failed with %d"),
                     GetLastError());
            return FALSE;
        }
    }


    //
    // Allocate buffers to hold the keyname, valuename, and data
    //

    lpKeyName = (LPWSTR) LocalAlloc (LPTR, MAX_KEYNAME_SIZE * sizeof(WCHAR));

    if (!lpKeyName)
    {
        _tprintf(TEXT("DisplayRegistryData: Failed to allocate memory with %d"),
                 GetLastError());
        return FALSE;
    }


    lpValueName = (LPWSTR) LocalAlloc (LPTR, MAX_VALUENAME_SIZE * sizeof(WCHAR));

    if (!lpValueName)
    {
        _tprintf(TEXT("DisplayRegistryData: Failed to allocate memory with %d"),
                 GetLastError());
        LocalFree (lpKeyName);
        return FALSE;
    }


    //
    // Read the header block
    //
    // 2 DWORDS, signature (PReg) and version number and 2 newlines
    //

    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        _tprintf(TEXT("DisplayRegistryData: Failed to read signature with %d"),
                 GetLastError());
        goto Exit;
    }


    if (dwTemp != REGFILE_SIGNATURE)
    {
        _tprintf(TEXT("DisplayRegistryData: Invalid file signature"));
        goto Exit;
    }


    if (!ReadFile (hFile, &dwTemp, sizeof(dwTemp), &dwBytesRead, NULL) ||
        dwBytesRead != sizeof(dwTemp))
    {
        _tprintf(TEXT("DisplayRegistryData: Failed to read version number with %d"),
                 GetLastError());
        goto Exit;
    }

    if (dwTemp != REGISTRY_FILE_VERSION)
    {
        _tprintf(TEXT("DisplayRegistryData: Invalid file version"));
        goto Exit;
    }


    //
    // Read the data
    //

    while (TRUE)
    {

        //
        // Read the first character.  It will either be a [ or the end
        // of the file.
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                _tprintf(TEXT("DisplayRegistryData: Failed to read first character with %d"),
                         GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L'['))
        {
            break;
        }


        //
        // Read the keyname
        //

        lpTemp = lpKeyName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                _tprintf(TEXT("DisplayRegistryData: Failed to read keyname character with %d"),
                         GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                _tprintf(TEXT("DisplayRegistryData: Failed to read first character with %d"),
                         GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the valuename
        //

        lpTemp = lpValueName;

        while (TRUE)
        {

            if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
            {
                _tprintf(TEXT("DisplayRegistryData: Failed to read valuename character with %d"),
                         GetLastError());
                goto Exit;
            }

            *lpTemp++ = chTemp;

            if (chTemp == TEXT('\0'))
                break;
        }


        //
        // Read the semi-colon
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                _tprintf(TEXT("DisplayRegistryData: Failed to read first character with %d"),
                         GetLastError());
                goto Exit;
            }
            break;
        }

        if ((dwBytesRead == 0) || (chTemp != L';'))
        {
            break;
        }


        //
        // Read the type
        //

        if (!ReadFile (hFile, &dwType, sizeof(DWORD), &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to read type with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to skip semicolon with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Read the data length
        //

        if (!ReadFile (hFile, &dwDataLength, sizeof(DWORD), &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to data length with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Skip semicolon
        //

        if (!ReadFile (hFile, &dwTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to skip semicolon with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Allocate memory for data
        //

        lpData = (LPBYTE) LocalAlloc (LPTR, dwDataLength);

        if (!lpData)
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to allocate memory for data with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Read data
        //

        if (!ReadFile (hFile, lpData, dwDataLength, &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to read data with %d"),
                     GetLastError());
            goto Exit;
        }


        //
        // Skip closing bracket
        //

        if (!ReadFile (hFile, &chTemp, sizeof(WCHAR), &dwBytesRead, NULL))
        {
            _tprintf(TEXT("DisplayRegistryData: Failed to skip closing bracket with %d"),
                     GetLastError());
            goto Exit;
        }

        if (chTemp != L']')
        {
            _tprintf(TEXT("DisplayRegistryData: Expected to find ], but found %c"),
                     chTemp);
            goto Exit;
        }

        //
        // Print out the entry
        //

        _tprintf (TEXT("\nKeyName:\t%s\n"), lpKeyName);
        _tprintf (TEXT("ValueName:\t%s\n"), lpValueName);


        switch (dwType) {

            case REG_DWORD:
                _tprintf (TEXT("ValueType:\tREG_DWORD\n"));
                _tprintf (TEXT("Value:\t\t0x%08x\n"), *((LPDWORD)lpData));
                break;

            case REG_SZ:
                _tprintf (TEXT("ValueType:\tREG_SZ\n"));
                _tprintf (TEXT("Value:\t%s\n"), (LPTSTR)lpData);
                break;

            case REG_EXPAND_SZ:
                _tprintf (TEXT("ValueType:\tREG_EXPAND_SZ\n"));
                _tprintf (TEXT("Value:\t%s\n"), (LPTSTR)lpData);
                break;

            case REG_MULTI_SZ:
                _tprintf (TEXT("ValueType:\tREG_MULTI_SZ\n"));
                _tprintf (TEXT("Value:\n\t\t"));
                lpTemp = (LPWSTR) lpData;

                while (*lpTemp) {
                    _tprintf (TEXT("%s\n\t\t"), lpTemp);
                    lpTemp += lstrlen(lpTemp) + 1;
                }
                break;

            case REG_BINARY:
                _tprintf (TEXT("ValueType:\tREG_BINARY\n"));
                _tprintf (TEXT("Value:\n\t"));

                dwIndex = 0;
                dwCount = 0;
                lpIndex = lpData;
                ZeroMemory(szString, sizeof(szString));

                while (dwIndex <= dwDataLength) {
                    _tprintf (TEXT("%02x "), *lpIndex);

                    if ((*lpIndex > 32) && (*lpIndex < 127)) {
                        szString[dwCount] = *lpIndex;
                    } else {
                        szString[dwCount] = '.';
                    }

                    if (dwCount < 15) {
                        dwCount++;
                    } else {
                        printf (" %s", szString);
                        _tprintf (TEXT("\n\t"));
                        ZeroMemory(szString, sizeof(szString));
                        dwCount = 0;
                    }

                    dwIndex++;
                    lpIndex++;
                }

                if (dwCount > 0) {
                    while (dwCount < 16) {
                        _tprintf (TEXT("   "));
                        dwCount++;
                    }
                    printf (" %s\n", szString);
                }

                _tprintf (TEXT("\n"));

                break;

            case REG_NONE:
                _tprintf (TEXT("ValueType:\tREG_NONE\n"));
                _tprintf (TEXT("Value:\t\tThis key contains no values\n"), *lpData);
                break;


            default:
                _tprintf (TEXT("ValueType:\tUnknown\n"));
                _tprintf (TEXT("ValueSize:\t%d\n"), dwDataLength);
                break;
        }

        LocalFree (lpData);
        lpData = NULL;

    }

    bResult = TRUE;

Exit:

    //
    // Finished
    //

    if (lpData) {
        LocalFree (lpData);
    }
    CloseHandle (hFile);
    LocalFree (lpKeyName);
    LocalFree (lpValueName);

    return bResult;
}
