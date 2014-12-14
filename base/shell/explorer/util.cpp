#include "precomp.h"
#include <winver.h>

typedef struct _LANGCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGCODEPAGE, *PLANGCODEPAGE;

HRESULT
IsSameObject(IN IUnknown *punk1, IN IUnknown *punk2)
{
    HRESULT hRet;

    hRet = punk1->QueryInterface(IID_PPV_ARG(IUnknown, &punk1));

    if (!SUCCEEDED(hRet))
        return hRet;

    hRet = punk2->QueryInterface(IID_PPV_ARG(IUnknown, &punk2));

    punk1->Release();

    if (!SUCCEEDED(hRet))
        return hRet;

    punk2->Release();

    /* We're dealing with the same object if the IUnknown pointers are equal */
    return (punk1 == punk2) ? S_OK : S_FALSE;
}

HMENU
LoadPopupMenu(IN HINSTANCE hInstance,
              IN LPCTSTR lpMenuName)
{
    HMENU hMenu, hSubMenu = NULL;

    hMenu = LoadMenu(hInstance,
                     lpMenuName);

    if (hMenu != NULL)
    {
        hSubMenu = GetSubMenu(hMenu,
                              0);
        if (hSubMenu != NULL &&
            !RemoveMenu(hMenu,
            0,
            MF_BYPOSITION))
        {
            hSubMenu = NULL;
        }

        DestroyMenu(hMenu);
    }

    return hSubMenu;
}

HMENU
FindSubMenu(IN HMENU hMenu,
            IN UINT uItem,
            IN BOOL fByPosition)
{
    MENUITEMINFO mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;

    if (GetMenuItemInfo(hMenu,
        uItem,
        fByPosition,
        &mii))
    {
        return mii.hSubMenu;
    }

    return NULL;
}

BOOL
GetCurrentLoggedOnUserName(OUT LPTSTR szBuffer,
                           IN DWORD dwBufferSize)
{
    DWORD dwType;
    DWORD dwSize;

    /* Query the user name from the registry */
    dwSize = (dwBufferSize * sizeof(WCHAR)) - 1;
    if (RegQueryValueEx(hkExplorer,
        TEXT("Logon User Name"),
        0,
        &dwType,
        (LPBYTE) szBuffer,
        &dwSize) == ERROR_SUCCESS &&
        (dwSize / sizeof(WCHAR)) > 1 &&
        szBuffer[0] != _T('\0'))
    {
        szBuffer[dwSize / sizeof(WCHAR)] = _T('\0');
        return TRUE;
    }

    /* Fall back to GetUserName() */
    dwSize = dwBufferSize;
    if (!GetUserName(szBuffer,
        &dwSize))
    {
        szBuffer[0] = _T('\0');
        return FALSE;
    }

    return TRUE;
}

BOOL
FormatMenuString(IN HMENU hMenu,
                 IN UINT uPosition,
                 IN UINT uFlags,
...)
{
    va_list vl;
    MENUITEMINFO mii;
    WCHAR szBuf[128];
    WCHAR szBufFmt[128];

    /* Find the menu item and read the formatting string */
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = (LPTSTR) szBufFmt;
    mii.cch = sizeof(szBufFmt) / sizeof(szBufFmt[0]);
    if (GetMenuItemInfo(hMenu,
        uPosition,
        uFlags,
        &mii))
    {
        /* Format the string */
        va_start(vl, uFlags);
        _vsntprintf(szBuf,
                    (sizeof(szBuf) / sizeof(szBuf[0])) - 1,
                    szBufFmt,
                    vl);
        va_end(vl);
        szBuf[(sizeof(szBuf) / sizeof(szBuf[0])) - 1] = _T('\0');

        /* Update the menu item */
        mii.dwTypeData = (LPTSTR) szBuf;
        if (SetMenuItemInfo(hMenu,
            uPosition,
            uFlags,
            &mii))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
GetExplorerRegValueSet(IN HKEY hKey,
                       IN LPCTSTR lpSubKey,
                       IN LPCTSTR lpValue)
{
    WCHAR szBuffer[MAX_PATH];
    HKEY hkSubKey;
    DWORD dwType, dwSize;
    BOOL Ret = FALSE;

    StringCbCopy(szBuffer, sizeof(szBuffer),
                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"));
    if (FAILED_UNEXPECTEDLY(StringCbCat(szBuffer, sizeof(szBuffer),
        _T("\\"))))
        return FALSE;
    if (FAILED_UNEXPECTEDLY(StringCbCat(szBuffer, sizeof(szBuffer),
        lpSubKey)))
        return FALSE;

    dwSize = sizeof(szBuffer);
    if (RegOpenKeyEx(hKey,
        szBuffer,
        0,
        KEY_QUERY_VALUE,
        &hkSubKey) == ERROR_SUCCESS)
    {
        ZeroMemory(szBuffer,
                   sizeof(szBuffer));

        if (RegQueryValueEx(hkSubKey,
            lpValue,
            0,
            &dwType,
            (LPBYTE) szBuffer,
            &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD && dwSize == sizeof(DWORD))
                Ret = *((PDWORD) szBuffer) != 0;
            else if (dwSize > 0)
                Ret = *((PCHAR) szBuffer) != 0;
        }

        RegCloseKey(hkSubKey);
    }
    return Ret;
}

BOOL
GetVersionInfoString(IN WCHAR *szFileName,
IN WCHAR *szVersionInfo,
OUT WCHAR *szBuffer,
IN UINT cbBufLen)
{
    LPVOID lpData = NULL;
    WCHAR szSubBlock[128];
    WCHAR *lpszLocalBuf = NULL;
    LANGID UserLangId;
    PLANGCODEPAGE lpTranslate = NULL;
    DWORD dwLen;
    DWORD dwHandle;
    UINT cbTranslate;
    UINT cbLen;
    BOOL bRet = FALSE;
    unsigned int i;

    dwLen = GetFileVersionInfoSize(szFileName, &dwHandle);

    if (dwLen > 0)
    {
        lpData = HeapAlloc(hProcessHeap, 0, dwLen);

        if (lpData != NULL)
        {
            if (GetFileVersionInfo(szFileName,
                0,
                dwLen,
                lpData) != 0)
            {
                UserLangId = GetUserDefaultLangID();

                VerQueryValue(lpData,
                              TEXT("\\VarFileInfo\\Translation"),
                              (LPVOID *) &lpTranslate,
                              &cbTranslate);

                for (i = 0; i < cbTranslate / sizeof(LANGCODEPAGE); i++)
                {
                    /* If the bottom eight bits of the language id's
                    match, use this version information (since this
                    means that the version information and the users
                    default language are the same). */
                    if ((lpTranslate[i].wLanguage & 0xFF) ==
                        (UserLangId & 0xFF))
                    {
                        wnsprintf(szSubBlock,
                                  sizeof(szSubBlock) / sizeof(szSubBlock[0]),
                                  TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                                  lpTranslate[i].wLanguage,
                                  lpTranslate[i].wCodePage,
                                  szVersionInfo);

                        if (VerQueryValue(lpData,
                            szSubBlock,
                            (LPVOID *) &lpszLocalBuf,
                            &cbLen) != 0)
                        {
                            _tcsncpy(szBuffer, lpszLocalBuf, cbBufLen / sizeof(*szBuffer));

                            bRet = TRUE;
                            break;
                        }
                    }
                }
            }
            HeapFree(hProcessHeap, 0, lpData);
            lpData = NULL;
        }
    }

    return bRet;
}
