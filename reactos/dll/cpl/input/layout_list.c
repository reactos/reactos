/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/layout_list.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "layout_list.h"


static LAYOUT_LIST_NODE *_LayoutList = NULL;


static LAYOUT_LIST_NODE*
LayoutList_AppendNode(DWORD dwId, DWORD dwSpecialId, const WCHAR *pszName)
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

    pNew->pszName = _wcsdup(pszName);
    if (pNew->pszName == NULL)
    {
        free(pNew);
        return NULL;
    }

    pNew->dwId = dwId;
    pNew->dwSpecialId = dwSpecialId;

    if (pCurrent == NULL)
    {
        _LayoutList = pNew;
    }
    else
    {
        while (pCurrent->pNext != NULL)
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

    if (_LayoutList == NULL)
        return;

    pCurrent = _LayoutList;

    while (pCurrent != NULL)
    {
        LAYOUT_LIST_NODE *pNext = pCurrent->pNext;

        free(pCurrent->pszName);
        free(pCurrent);

        pCurrent = pNext;
    }

    _LayoutList = NULL;
}


VOID
LayoutList_Create(VOID)
{
    WCHAR szSystemDirectory[MAX_PATH];
    WCHAR szLayoutId[MAX_PATH];
    DWORD dwIndex = 0;
    DWORD dwSize;
    HKEY hKey;

    if (!GetSystemDirectoryW(szSystemDirectory, ARRAYSIZE(szSystemDirectory)))
    {
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts",
                      0,
                      KEY_ENUMERATE_SUB_KEYS,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    dwSize = ARRAYSIZE(szLayoutId);

    while (RegEnumKeyExW(hKey, dwIndex, szLayoutId, &dwSize,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        HKEY hLayoutKey;

        if (RegOpenKeyExW(hKey,
                          szLayoutId,
                          0,
                          KEY_QUERY_VALUE,
                          &hLayoutKey) == ERROR_SUCCESS)
        {
            WCHAR szBuffer[MAX_PATH];

            dwSize = sizeof(szBuffer);

            if (RegQueryValueExW(hLayoutKey,
                                 L"Layout File",
                                 NULL, NULL,
                                 (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
            {
                WCHAR szFilePath[MAX_PATH];

                StringCchPrintfW(szFilePath, ARRAYSIZE(szFilePath),
                                 L"%s\\%s", szSystemDirectory, szBuffer);

                if (GetFileAttributesW(szFilePath) != INVALID_FILE_ATTRIBUTES)
                {
                    DWORD dwSpecialId = 0;

                    dwSize = sizeof(szBuffer);

                    if (RegQueryValueExW(hLayoutKey,
                                         L"Layout Id",
                                         NULL, NULL,
                                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
                    {
                        dwSpecialId = DWORDfromString(szBuffer);
                    }

                    dwSize = sizeof(szBuffer);

                    if (RegQueryValueExW(hLayoutKey,
                                         L"Layout Display Name",
                                         NULL, NULL,
                                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS &&
                        szBuffer[0] == L'@')
                    {
                        WCHAR *pBuffer;
                        WCHAR *pIndex;

                        /* Move to the position after the character "@" */
                        pBuffer = szBuffer + 1;

                        /* Get a pointer to the beginning ",-" */
                        pIndex = wcsstr(pBuffer, L",-");

                        if (pIndex != NULL)
                        {
                            WCHAR szPath[MAX_PATH];
                            INT iIndex;

                            /* Convert the number in the string after the ",-" */
                            iIndex = _wtoi(pIndex + 2);

                            pIndex[0] = 0;

                            if (ExpandEnvironmentStringsW(pBuffer, szPath, ARRAYSIZE(szPath)) != 0)
                            {
                                HANDLE hHandle;

                                hHandle = LoadLibraryW(szPath);
                                if (hHandle != NULL)
                                {
                                    INT iLength = LoadStringW(hHandle, iIndex, szBuffer, ARRAYSIZE(szBuffer));

                                    FreeLibrary(hHandle);

                                    if (iLength != 0)
                                    {
                                        DWORD dwLayoutId = DWORDfromString(szLayoutId);

                                        LayoutList_AppendNode(dwLayoutId, dwSpecialId, szBuffer);
                                    }
                                    else
                                    {
                                        goto NotTranslated;
                                    }
                                }
                                else
                                {
                                    goto NotTranslated;
                                }
                            }
                            else
                            {
                                goto NotTranslated;
                            }
                        }
                        else
                        {
                            goto NotTranslated;
                        }
                    }
                    else
                    {
NotTranslated:
                        dwSize = sizeof(szBuffer);

                        if (RegQueryValueExW(hLayoutKey,
                                             L"Layout Text",
                                             NULL, NULL,
                                             (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
                        {
                            DWORD dwLayoutId = DWORDfromString(szLayoutId);

                            LayoutList_AppendNode(dwLayoutId, dwSpecialId, szBuffer);
                        }
                    }
                }
            }

            RegCloseKey(hLayoutKey);
        }

        dwSize = ARRAYSIZE(szLayoutId);
        ++dwIndex;
    }

    RegCloseKey(hKey);
}


LAYOUT_LIST_NODE*
LayoutList_GetByHkl(HKL hkl)
{
    LAYOUT_LIST_NODE *pCurrent;

    if ((HIWORD(hkl) & 0xF000) == 0xF000)
    {
        DWORD dwSpecialId = (HIWORD(hkl) & 0x0FFF);

        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (dwSpecialId == pCurrent->dwSpecialId)
            {
                return pCurrent;
            }
        }
    }
    else
    {
        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (HIWORD(hkl) == LOWORD(pCurrent->dwId))
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
