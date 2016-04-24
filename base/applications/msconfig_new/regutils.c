#include "precomp.h"
#include "utils.h"
#include "regutils.h"

LRESULT
RegQueryRegistryKeys(IN HKEY    hRootKey,
                     IN LPCWSTR KeyName,
                     IN PQUERY_REGISTRY_KEYS_TABLE QueryTable,
                     IN PVOID   Context)
{
    HKEY hSubKey = NULL;

    if (RegOpenKeyExW(hRootKey, KeyName, 0, KEY_ENUMERATE_SUB_KEYS, &hSubKey) == ERROR_SUCCESS)
    {
        HKEY hEntryKey = NULL;

        LRESULT lError = ERROR_SUCCESS;
        DWORD   dwIndex = 0;
        WCHAR   szValueName[MAX_VALUE_NAME] = L"";
        DWORD   dwValueLength = ARRAYSIZE(szValueName);

        while ( (lError = RegEnumKeyExW(hSubKey, dwIndex, szValueName, &dwValueLength, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS )
        {
            if ( (lError == ERROR_SUCCESS) && (RegOpenKeyExW(hSubKey, szValueName, 0, KEY_QUERY_VALUE, &hEntryKey) == ERROR_SUCCESS) )
            {
                PQUERY_REGISTRY_KEYS_TABLE pTable = QueryTable;
                while (pTable && pTable->QueryRoutine)
                {
                    pTable->QueryRoutine(hRootKey, KeyName, szValueName, hEntryKey, Context, pTable->EntryContext);
                    ++pTable;
                }

                RegCloseKey(hEntryKey);
            }

            ++dwIndex;
            dwValueLength  = ARRAYSIZE(szValueName);
            szValueName[0] = L'\0';
        }

        RegCloseKey(hSubKey);
    }

    return ERROR_SUCCESS;
}

//
// Idea taken from RtlQueryRegistryValues (see DDK).
//
LRESULT
RegQueryRegistryValues(IN HKEY    hRootKey,
                       IN LPCWSTR KeyName,
                       IN PQUERY_REGISTRY_VALUES_TABLE QueryTable,
                       IN PVOID   Context)
{
    LRESULT res     = ERROR_SUCCESS;
    HKEY    hSubKey = NULL;

    if ( (res = RegOpenKeyExW(hRootKey, KeyName, 0, KEY_QUERY_VALUE, &hSubKey)) == ERROR_SUCCESS )
    {
        DWORD dwIndex = 0, dwType = 0;
        WCHAR szValueName[MAX_VALUE_NAME] = L"";
        LPBYTE lpData = NULL;
        DWORD dwValueLength = ARRAYSIZE(szValueName), dwDataLength = 0;

        while (RegEnumValueW(hSubKey, dwIndex, szValueName, &dwValueLength, NULL, &dwType, NULL, &dwDataLength) != ERROR_NO_MORE_ITEMS)
        {
            ++dwValueLength;
            lpData = (LPBYTE)MemAlloc(0, dwDataLength);

            if (RegEnumValueW(hSubKey, dwIndex, szValueName, &dwValueLength, NULL, &dwType, lpData, &dwDataLength) == ERROR_SUCCESS)
            {
                PQUERY_REGISTRY_VALUES_TABLE pTable = QueryTable;
                while (pTable && pTable->QueryRoutine)
                {
                    pTable->QueryRoutine(hRootKey, KeyName, szValueName, dwType, lpData, dwDataLength, Context, pTable->EntryContext);
                    ++pTable;
                }
            }

            MemFree(lpData); lpData = NULL;

            ++dwIndex;
            dwValueLength = ARRAYSIZE(szValueName), dwDataLength = 0;
            szValueName[0] = L'\0';
        }

        RegCloseKey(hSubKey);
    }

    return res;
}

LONG
RegGetDWORDValue(IN  HKEY    hKey,
                 IN  LPCWSTR lpSubKey OPTIONAL,
                 IN  LPCWSTR lpValue  OPTIONAL,
                 OUT LPDWORD lpData   OPTIONAL)
{
    LONG lRet      = ERROR_SUCCESS;
    HKEY hEntryKey = NULL;

    //
    // Open the sub-key, if any. Otherwise,
    // use the given registry key handle.
    //
    if (lpSubKey)
    {
        lRet = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hEntryKey);
    }
    else
    {
        if (hKey != INVALID_HANDLE_VALUE)
        {
            // TODO: Ensure that hKey has the KEY_QUERY_VALUE right.
            hEntryKey = hKey;
            lRet = ERROR_SUCCESS;
        }
        else
        {
            lRet = ERROR_INVALID_HANDLE;
        }
    }

    if (lRet == ERROR_SUCCESS)
    {
        DWORD dwType    = 0,
              dwRegData = 0,
              dwBufSize = sizeof(dwRegData /* DWORD */);

        lRet = RegQueryValueExW(hEntryKey, lpValue, NULL, &dwType, (LPBYTE)&dwRegData, &dwBufSize);

        if (lRet == ERROR_SUCCESS)
        {
            if ( (dwType == REG_DWORD) && (dwBufSize == sizeof(DWORD)) )
            {
                if (lpData)
                    *lpData = dwRegData;
            }
            else
            {
                lRet = ERROR_UNSUPPORTED_TYPE;
            }
        }
        else if (lRet == ERROR_MORE_DATA)
        {
            if (dwType != REG_DWORD)
            {
                lRet = ERROR_UNSUPPORTED_TYPE;
            }
        }

        // Close the opened sub-key.
        if (lpSubKey)
            RegCloseKey(hEntryKey);
    }

    return lRet;
}

LONG
RegSetDWORDValue(IN HKEY    hKey,
                 IN LPCWSTR lpSubKey OPTIONAL,
                 IN LPCWSTR lpValue  OPTIONAL,
                 IN BOOL    bCreateKeyIfDoesntExist,
                 IN DWORD   dwData)
{
    LONG lRet      = ERROR_SUCCESS;
    HKEY hEntryKey = NULL;

    //
    // Open (or create) the sub-key, if any.
    // Otherwise, use the given registry key handle.
    //
    if (lpSubKey)
    {
        if (bCreateKeyIfDoesntExist)
            lRet = RegCreateKeyExW(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hEntryKey, NULL);
        else
            lRet = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_SET_VALUE, &hEntryKey);
    }
    else
    {
        if (hKey != INVALID_HANDLE_VALUE)
        {
            // TODO: Ensure that hKey has the KEY_QUERY_VALUE right.
            hEntryKey = hKey;
            lRet = ERROR_SUCCESS;
        }
        else
        {
            lRet = ERROR_INVALID_HANDLE;
        }
    }

    //
    // Opening successful, can set the value now.
    //
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegSetValueExW(hEntryKey, lpValue, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData /* DWORD */));

        // Close the opened (or created) sub-key.
        if (lpSubKey)
            RegCloseKey(hEntryKey);
    }

    return lRet;
}
