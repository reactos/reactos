/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/network.c
 * PURPOSE:     ReactX diagnosis network page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

#include <winver.h>

typedef struct
{
    WCHAR Guid[40];
    UINT ResourceID;
}DIRECTPLAY_GUID;

typedef struct _LANGANDCODEPAGE_
  {
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

static DIRECTPLAY_GUID DirectPlay8SP[] =
{
    {
        L"{6D4A3650-628D-11D2-AE0F-006097B01411}",
        IDS_DIRECTPLAY8_MODEMSP
    },
    {
        L"{743B5D60-628D-11D2-AE0F-006097B01411}",
        IDS_DIRECTPLAY8_SERIALSP
    },
    {
        L"{53934290-628D-11D2-AE0F-006097B01411}",
        IDS_DIRECTPLAY8_IPXSP
    },
    {
        L"{EBFE7BA0-628D-11D2-AE0F-006097B01411}",
        IDS_DIRECTPLAY8_TCPSP
    }
};

static DIRECTPLAY_GUID DirectPlaySP[] =
{
    {
        L"{36E95EE0-8577-11cf-960C-0080C7534E82}",
        IDS_DIRECTPLAY_TCPCONN
    },
    {
        L"685BC400-9D2C-11cf-A9CD-00AA006886E3",
        IDS_DIRECTPLAY_IPXCONN
    },
    {
        L"{44EAA760-CB68-11cf-9C4E-00A0C905425E}",
        IDS_DIRECTPLAY_MODEMCONN
    },
    {
        L"{0F1D6860-88D9-11cf-9C4E-00A0C905425E}",
        IDS_DIRECTPLAY_SERIALCONN
    }
};

static
VOID
InitListViewColumns(HWND hDlgCtrl)
{
    WCHAR szText[256];
    LVCOLUMNW lvcolumn;
    INT Index;

    ZeroMemory(&lvcolumn, sizeof(LVCOLUMNW));
    lvcolumn.pszText = szText;
    lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvcolumn.fmt = LVCFMT_LEFT;
    lvcolumn.cx = 200;

    for(Index = 0; Index < 4; Index++)
    {
        szText[0] = L'\0';
        LoadStringW(hInst, IDS_DIRECTPLAY_COL_NAME1 + Index, szText, sizeof(szText) / sizeof(WCHAR));
        szText[(sizeof(szText) / sizeof(WCHAR))-1] = L'\0';
        if (Index)
            lvcolumn.cx = 98;
        if (SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, Index, (LPARAM)&lvcolumn) == -1)
            return;
    }
}

UINT
FindProviderIndex(LPCWSTR szGuid, DIRECTPLAY_GUID * PreDefProviders)
{
    UINT Index;
    for(Index = 0; Index < 4; Index++)
    {
        if (!wcsncmp(PreDefProviders[Index].Guid, szGuid, 40))
            return Index;
    }
    return UINT_MAX;
}

BOOL
GetFileVersion(LPCWSTR szAppName, WCHAR * szVer, DWORD szVerSize)
{
    UINT VerSize;
    DWORD DummyHandle;
    LPVOID pBuf;
    WORD lang = 0;
    WORD code = 0;
    LPLANGANDCODEPAGE lplangcode;
    WCHAR szBuffer[100];
    WCHAR * pResult;
    BOOL bResult = FALSE;
    BOOL bVer;

    static const WCHAR wFormat[] = L"\\StringFileInfo\\%04x%04x\\FileVersion";
    static const WCHAR wTranslation[] = L"VarFileInfo\\Translation";

    /* query version info size */
    VerSize = GetFileVersionInfoSizeW(szAppName, &DummyHandle);
    if (!VerSize)
        return FALSE;


    /* allocate buffer */
    pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, VerSize);
    if (!pBuf)
        return FALSE;


    /* query version info */
    if(!GetFileVersionInfoW(szAppName, 0, VerSize, pBuf))
    {
        HeapFree(GetProcessHeap(), 0, pBuf);
        return FALSE;
    }

    /* query lang code */
    if(VerQueryValueW(pBuf, wTranslation, (LPVOID *)&lplangcode, &VerSize))
    {
       /* FIXME find language from current locale / if not available,
        * default to english
        * for now default to first available language
        */
       lang = lplangcode->lang;
       code = lplangcode->code;
    }
    /* set up format */
    wsprintfW(szBuffer, wFormat, lang, code);
    /* query manufacturer */
     pResult = NULL;
    bVer = VerQueryValueW(pBuf, szBuffer, (LPVOID *)&pResult, &VerSize);

    if (VerSize < szVerSize && bVer && pResult)
    {
        wcscpy(szVer, pResult);
        pResult = wcschr(szVer, L' ');
        if (pResult)
        {
            /* cut off build info */
            VerSize = (pResult - szVer);
        }
        if (GetLocaleInfoW(MAKELCID(lang, SORT_DEFAULT), LOCALE_SLANGUAGE, &szVer[VerSize], szVerSize-VerSize))
        {
            szVer[VerSize-1] = L' ';
            szVer[szVerSize-1] = L'\0';
        }
        bResult = TRUE;
    }

    HeapFree(GetProcessHeap(), 0, pBuf);
    return bResult;
}

static
BOOL
EnumerateServiceProviders(HKEY hKey, HWND hDlgCtrl, DIRECTPLAY_GUID * PreDefProviders)
{
    DWORD dwIndex = 0;
    LONG result;
    WCHAR szName[50];
    WCHAR szGUID[40];
    WCHAR szTemp[63];
    WCHAR szResult[MAX_PATH+20] = {0};
    DWORD RegProviders = 0;
    DWORD ProviderIndex;
    DWORD dwName;
    LVITEMW Item;
    INT ItemCount;
    LRESULT lResult;


    ItemCount = ListView_GetItemCount(hDlgCtrl);
    ZeroMemory(&Item, sizeof(LVITEMW));
    Item.mask = LVIF_TEXT;
    Item.pszText = szResult;
    Item.iItem = ItemCount;
    /* insert all predefined items first */
    for(dwIndex = 0; dwIndex < 4; dwIndex++)
    {
        Item.iItem = ItemCount + dwIndex;
        Item.iSubItem = 0;
        szResult[0] = L'\0';
        LoadStringW(hInst, PreDefProviders[dwIndex].ResourceID, szResult, sizeof(szResult)/sizeof(WCHAR));
        szResult[(sizeof(szResult)/sizeof(WCHAR))-1] = L'\0';
        Item.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEM, 0, (LPARAM)&Item);
        Item.iSubItem = 1;
        szResult[0] = L'\0';
        LoadStringW(hInst, IDS_REG_FAIL, szResult, sizeof(szResult)/sizeof(WCHAR));
        szResult[(sizeof(szResult)/sizeof(WCHAR))-1] = L'\0';
        SendMessageW(hDlgCtrl, LVM_SETITEM, 0, (LPARAM)&Item);
    }

    dwIndex = 0;
    do
    {
        dwName = sizeof(szName) / sizeof(WCHAR);
        result = RegEnumKeyEx(hKey, dwIndex, szName, &dwName, NULL, NULL, NULL, NULL);
        if (result == ERROR_SUCCESS)
        {
            szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';

            ProviderIndex = UINT_MAX;
            if (GetRegValue(hKey, szName, L"GUID", REG_SZ, szGUID, sizeof(szGUID)))
                ProviderIndex = FindProviderIndex(szGUID, PreDefProviders);

            if (ProviderIndex == UINT_MAX)
            {
                /* a custom service provider was found */
                Item.iItem = ListView_GetItemCount(hDlgCtrl);

                /* FIXME
                 * on Windows Vista we need to use RegLoadMUIString which is not available for older systems
                 */
                if (!GetRegValue(hKey, szName, L"Friendly Name", REG_SZ, szResult, sizeof(szResult)))
                    if (!GetRegValue(hKey, szName, L"DescriptionW", REG_SZ, szResult, sizeof(szResult)))
                        szResult[0] = L'\0';

                /* insert the new provider */
                Item.iSubItem = 0;
                lResult = SendMessageW(hDlgCtrl, LVM_INSERTITEM, 0, (LPARAM)&Item);
                /* adjust index */
                ProviderIndex = lResult - ItemCount;
            }

            szResult[0] = L'\0';
            /* check if the 'Path' key is available */
            if (!GetRegValue(hKey, szName, L"Path", REG_SZ, szResult, sizeof(szResult)))
            {
                /* retrieve the path by lookup the CLSID */
                wcscpy(szTemp, L"CLSID\\");
                wcscpy(&szTemp[6], szGUID);
                wcscpy(&szTemp[44], L"\\InProcServer32");
                if (!GetRegValue(HKEY_CLASSES_ROOT, szTemp, NULL, REG_SZ, szResult, sizeof(szResult)))
                {
                    szResult[0] = L'\0';
                    ProviderIndex = UINT_MAX;
                }
            }
            if (szResult[0])
            {
                /* insert path name */
                Item.iSubItem = 2;
                Item.iItem = ProviderIndex + ItemCount;
                SendMessageW(hDlgCtrl, LVM_SETITEM, 0, (LPARAM)&Item);
                /* retrieve file version */
                if (!GetFileVersion(szResult, szTemp, sizeof(szTemp)/sizeof(WCHAR)))
                {
                    szTemp[0] = L'\0';
                    LoadStringW(hInst, IDS_VERSION_UNKNOWN, szTemp, sizeof(szTemp)/sizeof(WCHAR));
                    szTemp[(sizeof(szTemp)/sizeof(WCHAR))-1] = L'\0';
                }
                Item.iSubItem = 3;
                Item.pszText = szTemp;
                SendMessageW(hDlgCtrl, LVM_SETITEM, 0, (LPARAM)&Item);
                Item.pszText = szResult;
            }

             if (ProviderIndex != UINT_MAX)
                {
                    RegProviders |= (1 << ProviderIndex);
                    szResult[0] = L'\0';
                    LoadStringW(hInst, IDS_REG_SUCCESS, szResult, sizeof(szResult) / sizeof(WCHAR));
                    Item.iSubItem = 1;

                    Item.iItem = ProviderIndex + ItemCount;
                    szResult[(sizeof(szResult)/sizeof(WCHAR))-1] = L'\0';
                    SendMessageW(hDlgCtrl, LVM_SETITEM, 0, (LPARAM)&Item);
                }
        }
        dwIndex++;
    }while(result != ERROR_NO_MORE_ITEMS);

    /* check if all providers have been registered */
//    if (RegProviders == 15)
        return TRUE;
    return FALSE;
}



static
void
InitializeDirectPlayDialog(HWND hwndDlg)
{
    HKEY hKey;
    LONG result;
    HWND hDlgCtrl;

    /* open DirectPlay8 key */
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\DirectPlay8\\Service Providers", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
        return;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LIST_PROVIDER);

    /* Highlights the entire row instead of just the selected item in the first column */
    SendMessage(hDlgCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    /* initialize list ctrl */
    InitListViewColumns(hDlgCtrl);

    /* enumerate providers */
    result = EnumerateServiceProviders(hKey, hDlgCtrl, DirectPlay8SP);
    RegCloseKey(hKey);
    if (!result)
        return;

    /* open DirectPlay key */
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\DirectPlay\\Service Providers", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
        return;

    /* enumerate providers */
    EnumerateServiceProviders(hKey, hDlgCtrl, DirectPlaySP);
    RegCloseKey(hKey);
}

INT_PTR CALLBACK
NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message) {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeDirectPlayDialog(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}
