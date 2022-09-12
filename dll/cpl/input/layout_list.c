/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/layout_list.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "layout_list.h"

static LAYOUT_LIST_NODE *_LayoutList = NULL;

static LAYOUT_LIST_NODE*
LayoutList_AppendNode(DWORD dwKLID, WORD wSpecialId, LPCWSTR pszFile, LPCWSTR pszName,
                      LPCWSTR pszImeFile)
{
    LAYOUT_LIST_NODE *pCurrent;
    LAYOUT_LIST_NODE *pNew;

    if (pszName == NULL)
        return NULL;

    pCurrent = _LayoutList;

    pNew = (LAYOUT_LIST_NODE*)malloc(sizeof(LAYOUT_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    ZeroMemory(pNew, sizeof(LAYOUT_LIST_NODE));

    pNew->dwKLID = dwKLID;
    pNew->wSpecialId = wSpecialId;

    pNew->pszName = _wcsdup(pszName);
    pNew->pszFile = _wcsdup(pszFile);
    pNew->pszImeFile = _wcsdup(pszImeFile);
    if (pNew->pszName == NULL || pNew->pszFile == NULL ||
        (pszImeFile && pNew->pszImeFile == NULL))
    {
        free(pNew->pszName);
        free(pNew->pszFile);
        free(pNew->pszImeFile);
        free(pNew);
        return NULL;
    }

    if (pCurrent == NULL)
    {
        _LayoutList = pNew;
    }
    else
    {
        while (pCurrent->pNext)
        {
            pCurrent = pCurrent->pNext;
        }

        pNew->pPrev = pCurrent;
        pCurrent->pNext = pNew;
    }

    return pNew;
}


VOID
LayoutList_Destroy(VOID)
{
    LAYOUT_LIST_NODE *pCurrent;
    LAYOUT_LIST_NODE *pNext;

    if (_LayoutList == NULL)
        return;

    for (pCurrent = _LayoutList; pCurrent; pCurrent = pNext)
    {
        pNext = pCurrent->pNext;

        free(pCurrent->pszName);
        free(pCurrent->pszFile);
        free(pCurrent->pszImeFile);
        free(pCurrent);
    }

    _LayoutList = NULL;
}

typedef HRESULT (WINAPI *FN_SHLoadRegUIStringW)(HKEY, LPCWSTR, LPWSTR, DWORD);

/* FIXME: Use shlwapi!SHLoadRegUIStringW instead if it had fully implemented */
HRESULT FakeSHLoadRegUIStringW(HKEY hkey, LPCWSTR value, LPWSTR buf, DWORD size)
{
#if 1
    PWCHAR pBuffer, pIndex;
    WCHAR szDllPath[MAX_PATH];
    DWORD dwSize;
    HINSTANCE hDllInst;
    INT iIndex, iLength;

    dwSize = size * sizeof(WCHAR);
    if (RegQueryValueExW(hkey, value, NULL, NULL, (LPBYTE)buf, &dwSize) != ERROR_SUCCESS)
        return E_FAIL;

    if (buf[0] != L'@')
        return S_OK;

    /* Move to the position after the character "@" */
    pBuffer = buf + 1;

    /* Get a pointer to the beginning ",-" */
    pIndex = wcsstr(pBuffer, L",-");
    if (!pIndex)
        return E_FAIL;

    /* Convert the number in the string after the ",-" */
    iIndex = _wtoi(pIndex + 2);

    *pIndex = 0; /* Cut the string */

    if (ExpandEnvironmentStringsW(pBuffer, szDllPath, ARRAYSIZE(szDllPath)) == 0)
        return E_FAIL;

    hDllInst = LoadLibraryW(szDllPath);
    if (!hDllInst)
        return E_FAIL;

    iLength = LoadStringW(hDllInst, iIndex, buf, size);
    FreeLibrary(hDllInst);

    if (iLength <= 0)
        return E_FAIL;

    return S_OK;
#else
    HRESULT hr = E_FAIL;
    HINSTANCE hSHLWAPI = LoadLibraryW(L"shlwapi");
    FN_SHLoadRegUIStringW fn;
    fn = (FN_SHLoadRegUIStringW)GetProcAddress(hSHLWAPI, (LPCSTR)(INT_PTR)439);
    if (fn)
        hr = fn(hkey, value, buf, size);
    FreeLibrary(hSHLWAPI);
    return hr;
#endif
}

static BOOL
LayoutList_ReadLayout(HKEY hLayoutKey, LPCWSTR szKLID, LPCWSTR szSystemDirectory)
{
    WCHAR szFile[80], szImeFile[80], szBuffer[MAX_PATH], szFilePath[MAX_PATH];
    DWORD dwSize, dwKLID = DWORDfromString(szKLID);
    WORD wSpecialId = 0;
    LPWSTR pszImeFile = NULL;

    dwSize = sizeof(szFile);
    if (RegQueryValueExW(hLayoutKey, L"Layout File", NULL, NULL,
                         (LPBYTE)szFile, &dwSize) != ERROR_SUCCESS)
    {
        return FALSE; /* No "Layout File" value */
    }

    if (IS_IME_KLID(dwKLID))
    {
        dwSize = sizeof(szImeFile);
        if (RegQueryValueExW(hLayoutKey, L"IME File", NULL, NULL,
                             (LPBYTE)szImeFile, &dwSize) != ERROR_SUCCESS)
        {
            return FALSE; /* No "IME File" value */
        }
        pszImeFile = szImeFile;
    }

    /* Build the "Layout File" full path and check existence */
    StringCchPrintfW(szFilePath, ARRAYSIZE(szFilePath), L"%s\\%s", szSystemDirectory, szFile);
    if (GetFileAttributesW(szFilePath) == INVALID_FILE_ATTRIBUTES)
        return FALSE; /* No layout file found */

    /* Get the special ID */
    dwSize = sizeof(szBuffer);
    if (RegQueryValueExW(hLayoutKey, L"Layout Id", NULL, NULL,
                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
    {
        wSpecialId = LOWORD(DWORDfromString(szBuffer));
    }

    /* If there is a valid "Layout Display Name", then use it as the entry name */
    if (FakeSHLoadRegUIStringW(hLayoutKey, L"Layout Display Name",
                               szBuffer, ARRAYSIZE(szBuffer)) == S_OK)
    {
        LayoutList_AppendNode(dwKLID, wSpecialId, szFile, szBuffer, pszImeFile);
        return TRUE;
    }

    /* Otherwise, use "Layout Text" value as the entry name */
    dwSize = sizeof(szBuffer);
    if (RegQueryValueExW(hLayoutKey, L"Layout Text", NULL, NULL,
                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
    {
        LayoutList_AppendNode(dwKLID, wSpecialId, szFile, szBuffer, pszImeFile);
        return TRUE;
    }

    return FALSE;
}

VOID
LayoutList_Create(VOID)
{
    WCHAR szSystemDirectory[MAX_PATH], szKLID[MAX_PATH];
    DWORD dwSize, dwIndex;
    HKEY hKey, hLayoutKey;

    if (!GetSystemDirectoryW(szSystemDirectory, ARRAYSIZE(szSystemDirectory)))
        return;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    for (dwIndex = 0; ; ++dwIndex)
    {
        dwSize = ARRAYSIZE(szKLID);
        if (RegEnumKeyExW(hKey, dwIndex, szKLID, &dwSize, NULL, NULL,
                          NULL, NULL) != ERROR_SUCCESS)
        {
            break;
        }

        if (RegOpenKeyExW(hKey, szKLID, 0, KEY_QUERY_VALUE, &hLayoutKey) == ERROR_SUCCESS)
        {
            LayoutList_ReadLayout(hLayoutKey, szKLID, szSystemDirectory);
            RegCloseKey(hLayoutKey);
        }
    }

    RegCloseKey(hKey);
}


LAYOUT_LIST_NODE*
LayoutList_GetByHkl(HKL hkl)
{
    LAYOUT_LIST_NODE *pCurrent;

    if (IS_SPECIAL_HKL(hkl))
    {
        WORD wSpecialId = SPECIALIDFROMHKL(hkl);

        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (wSpecialId == pCurrent->wSpecialId)
            {
                return pCurrent;
            }
        }
    }
    else if (IS_IME_HKL(hkl))
    {
        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (hkl == UlongToHandle(pCurrent->dwKLID))
            {
                return pCurrent;
            }
        }
    }
    else
    {
        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (LANGIDFROMHKL(hkl) == LANGIDFROMKLID(pCurrent->dwKLID))
            {
                return pCurrent;
            }
        }
    }

    return NULL;
}


LAYOUT_LIST_NODE*
LayoutList_GetFirst(VOID)
{
    return _LayoutList;
}
