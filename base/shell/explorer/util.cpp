#include "precomp.h"
#include <winver.h>
#include <psapi.h>

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
              IN LPCWSTR lpMenuName)
{
    HMENU hMenu, hSubMenu = NULL;

    hMenu = LoadMenuW(hInstance, lpMenuName);
    if (hMenu != NULL)
    {
        hSubMenu = GetSubMenu(hMenu, 0);
        if ((hSubMenu != NULL) &&
            !RemoveMenu(hMenu, 0, MF_BYPOSITION))
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
    MENUITEMINFOW mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;

    if (GetMenuItemInfoW(hMenu, uItem, fByPosition, &mii))
    {
        return mii.hSubMenu;
    }

    return NULL;
}

BOOL
GetCurrentLoggedOnUserName(OUT LPWSTR szBuffer,
                           IN DWORD dwBufferSize)
{
    DWORD dwType;
    DWORD dwSize;

    /* Query the user name from the registry */
    dwSize = (dwBufferSize * sizeof(WCHAR)) - 1;
    if (RegQueryValueExW(hkExplorer,
                         L"Logon User Name",
                         0,
                         &dwType,
                         (LPBYTE)szBuffer,
                         &dwSize) == ERROR_SUCCESS &&
        (dwSize / sizeof(WCHAR)) > 1 &&
        szBuffer[0] != L'\0')
    {
        szBuffer[dwSize / sizeof(WCHAR)] = L'\0';
        return TRUE;
    }

    /* Fall back to GetUserName() */
    dwSize = dwBufferSize;
    if (!GetUserNameW(szBuffer, &dwSize))
    {
        szBuffer[0] = L'\0';
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
    MENUITEMINFOW mii;
    WCHAR szBuf[128];
    WCHAR szBufFmt[128];

    /* Find the menu item and read the formatting string */
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = szBufFmt;
    mii.cch = _countof(szBufFmt);
    if (GetMenuItemInfoW(hMenu, uPosition, uFlags, &mii))
    {
        /* Format the string */
        va_start(vl, uFlags);
        _vsntprintf(szBuf,
                    _countof(szBuf) - 1,
                    szBufFmt,
                    vl);
        va_end(vl);
        szBuf[_countof(szBuf) - 1] = L'\0';

        /* Update the menu item */
        mii.dwTypeData = szBuf;
        if (SetMenuItemInfo(hMenu, uPosition, uFlags, &mii))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL GetRegBool(IN LPCWSTR pszSubKey, IN LPCWSTR pszValueName, IN BOOL bDefaultValue)
{
    return SHRegGetBoolUSValueW(pszSubKey, pszValueName, FALSE, bDefaultValue);
}

BOOL SetRegDword(IN LPCWSTR pszSubKey, IN LPCWSTR pszValueName, IN DWORD dwValue)
{
    return (SHRegSetUSValueW(pszSubKey, pszValueName, REG_DWORD, &dwValue,
                             sizeof(dwValue), SHREGSET_FORCE_HKCU) == ERROR_SUCCESS);
}

#define REGKEY_ADVANCED L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"

BOOL GetAdvancedBool(IN LPCWSTR pszValueName, IN BOOL bDefaultValue)
{
    return GetRegBool(REGKEY_ADVANCED, pszValueName, bDefaultValue);
}

BOOL SetAdvancedDword(IN LPCWSTR pszValueName, IN DWORD dwValue)
{
    return SetRegDword(REGKEY_ADVANCED, pszValueName, dwValue);
}

BOOL
GetVersionInfoString(IN LPCWSTR szFileName,
                     IN LPCWSTR szVersionInfo,
                     OUT LPWSTR szBuffer,
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
    UINT i;

    dwLen = GetFileVersionInfoSizeW(szFileName, &dwHandle);

    if (dwLen > 0)
    {
        lpData = HeapAlloc(hProcessHeap, 0, dwLen);

        if (lpData != NULL)
        {
            if (GetFileVersionInfoW(szFileName,
                                    0,
                                    dwLen,
                                    lpData) != 0)
            {
                UserLangId = GetUserDefaultLangID();

                VerQueryValueW(lpData,
                               L"\\VarFileInfo\\Translation",
                               (LPVOID*)&lpTranslate,
                               &cbTranslate);

                for (i = 0; i < cbTranslate / sizeof(LANGCODEPAGE); i++)
                {
                    /* If the bottom eight bits of the language id's
                    match, use this version information (since this
                    means that the version information and the users
                    default language are the same). */
                    if (LOBYTE(lpTranslate[i].wLanguage) == LOBYTE(UserLangId))
                    {
                        wnsprintf(szSubBlock,
                                  _countof(szSubBlock),
                                  L"\\StringFileInfo\\%04X%04X\\%s",
                                  lpTranslate[i].wLanguage,
                                  lpTranslate[i].wCodePage,
                                  szVersionInfo);

                        if (VerQueryValueW(lpData,
                                           szSubBlock,
                                           (LPVOID*)&lpszLocalBuf,
                                           &cbLen) != 0)
                        {
                            wcsncpy(szBuffer, lpszLocalBuf, cbBufLen / sizeof(*szBuffer));

                            bRet = TRUE;
                            break;
                        }
                    }
                }
                if(bRet == FALSE && cbTranslate >= sizeof(LANGCODEPAGE)) {
                    //Try to use the first language as a fallback

                    wnsprintf(szSubBlock,
                                _countof(szSubBlock),
                                L"\\StringFileInfo\\%04X%04X\\%s",
                                lpTranslate[0].wLanguage,
                                lpTranslate[0].wCodePage,
                                szVersionInfo);

                    if (VerQueryValueW(lpData,
                                        szSubBlock,
                                        (LPVOID*)&lpszLocalBuf,
                                        &cbLen) != 0)
                    {
                        wcsncpy(szBuffer, lpszLocalBuf, cbBufLen / sizeof(*szBuffer));

                        bRet = TRUE;
                    }
                }
            }

            HeapFree(hProcessHeap, 0, lpData);
            lpData = NULL;
        }
    }

    return bRet;
}

BOOL GetProcessPath(IN DWORD dwProcessId,
                    OUT LPWSTR szBuffer,
                    IN DWORD cbBufLen)
{
    HANDLE hTaskProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
    BOOL bRet = FALSE;
    if(hTaskProc != NULL) {
        if(GetModuleFileNameExW(hTaskProc, NULL, szBuffer, cbBufLen)) {
            bRet = TRUE;
        }

        CloseHandle(hTaskProc);
    }

    return bRet;
}
