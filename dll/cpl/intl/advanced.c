#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

typedef struct CPStruct
{
    WORD    Status;
    UINT    CPage;
    HANDLE  hCPage;
    TCHAR   Name[MAX_PATH];
    struct  CPStruct *NextItem;
} CPAGE, *LPCPAGE;

static LPCPAGE PCPage = NULL;
static HINF hIntlInf;
static BOOL bSpain = FALSE;
static HWND hLangList;

static BOOL
GetSupportedCP(VOID)
{
    UINT uiCPage, Number;
    LONG Count;
    INFCONTEXT infCont;
    LPCPAGE lpCPage;
    HANDLE hCPage;
    CPINFOEX cpInfEx;
    //TCHAR Section[MAX_PATH];

    Count = SetupGetLineCount(hIntlInf, _T("CodePages"));
    if (Count <= 0) return FALSE;

    for (Number = 0; Number < (UINT)Count; Number++)
    {
        if (SetupGetLineByIndex(hIntlInf, _T("CodePages"), Number, &infCont) &&
            SetupGetIntField(&infCont, 0, (PINT)&uiCPage))
        {
            if (!(hCPage = GlobalAlloc(GHND, sizeof(CPAGE)))) return FALSE;

            lpCPage            = GlobalLock(hCPage);
            lpCPage->CPage     = uiCPage;
            lpCPage->hCPage    = hCPage;
            lpCPage->Status    = 0;
            (lpCPage->Name)[0] = 0;

            if (GetCPInfoEx(uiCPage, 0, &cpInfEx))
            {
                _tcscpy(lpCPage->Name, cpInfEx.CodePageName);
            }
            else if (!SetupGetStringField(&infCont, 1, lpCPage->Name, MAX_PATH, NULL))
            {
                GlobalUnlock(hCPage);
                GlobalFree(hCPage);
                continue;
            }

            lpCPage->NextItem = PCPage;
            PCPage = lpCPage;
        }
    }

    return TRUE;
}

static BOOL CALLBACK
InstalledCPProc(LPTSTR lpStr)
{
    LPCPAGE lpCP;
    UINT uiCP;

    lpCP = PCPage;
    uiCP = _ttol(lpStr);

    for (;;)
    {
        if (!lpCP) break;
        if (lpCP->CPage == uiCP)
        {
            lpCP->Status |= 0x0001;
            break;
        }
        lpCP = lpCP->NextItem;
    }

    return TRUE;
}

static VOID
InitCodePagesList(HWND hwndDlg)
{
    LPCPAGE lpCPage;
    INT ItemIndex;
    HWND hList;
    LV_COLUMN column;
    LV_ITEM item;
    RECT ListRect;

    hList = GetDlgItem(hwndDlg, IDC_CONV_TABLES);

    hIntlInf = SetupOpenInfFile(_T("intl.inf"), NULL, INF_STYLE_WIN4, NULL);

    if (hIntlInf == INVALID_HANDLE_VALUE)
        return;

    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        hIntlInf = NULL;
        return;
    }

    if (!GetSupportedCP())
        return;

    SetupCloseInfFile(hIntlInf);

    if (!EnumSystemCodePages(InstalledCPProc, CP_INSTALLED))
        return;

    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
    column.fmt  = LVCFMT_LEFT;
    GetClientRect(hList, &ListRect);
    column.cx   = ListRect.right - GetSystemMetrics(SM_CYHSCROLL);
    (VOID) ListView_InsertColumn(hList, 0, &column);

    (VOID) ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);

    lpCPage = PCPage;

    for (;;)
    {
        if (!lpCPage) break;
        ZeroMemory(&item, sizeof(LV_ITEM));
        item.mask      = LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
        item.state     = 0;
        item.stateMask = LVIS_STATEIMAGEMASK;
        item.pszText   = lpCPage->Name;
        item.lParam    = (LPARAM)lpCPage;

        ItemIndex = ListView_InsertItem(hList, &item);

        if (ItemIndex >= 0)
        {
            if (lpCPage->Status & 0x0001)
            {
                ListView_SetItemState(hList, ItemIndex,
                                      INDEXTOSTATEIMAGEMASK(LVIS_SELECTED),
                                      LVIS_STATEIMAGEMASK);
            }
            else
            {
                ListView_SetItemState(hList, ItemIndex,
                                      INDEXTOSTATEIMAGEMASK(LVIS_FOCUSED),
                                      LVIS_STATEIMAGEMASK);
            }
        }

        lpCPage = lpCPage->NextItem;
    }
}

static BOOL CALLBACK
LocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    TCHAR lang[255];
    INT index;
    BOOL bNoShow = FALSE;

    lcid = _tcstoul(lpLocale, NULL, 16);

    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        if (bSpain == FALSE)
        {
            LoadString(hApplet, IDS_SPAIN, lang, 255);
            bSpain = TRUE;
        }
        else
        {
            bNoShow = TRUE;
        }
    }
    else
    {
        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang)/sizeof(TCHAR));
    }

    if (bNoShow == FALSE)
    {
    index = SendMessage(hLangList,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)lang);

    SendMessage(hLangList,
                CB_SETITEMDATA,
                index,
                (LPARAM)lcid);
    }

    return TRUE;
}

static VOID
InitLanguagesList(HWND hwndDlg)
{
    TCHAR langSel[255];

    hLangList = GetDlgItem(hwndDlg, IDC_LANGUAGE_COMBO);

    bSpain = FALSE;
    EnumSystemLocales(LocalesEnumProc, LCID_SUPPORTED);

    /* Select current locale */
    GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, sizeof(langSel)/sizeof(TCHAR));

    SendMessage(hLangList, CB_SELECTSTRING, -1, (LPARAM)langSel);
}

static VOID
GetCurrentDPI(LPTSTR szDPI)
{
    DWORD dwType, dwSize, dwDPI, dwDefDPI = 0x00000060; // Default 96 DPI
    HKEY hKey;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontDPI"), 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        _tcscpy(szDPI, _T("96"));
        return;
    }

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);

    if (RegQueryValueEx(hKey, _T("LogPixels"), NULL, &dwType, (LPBYTE)&dwDPI, &dwSize) != ERROR_SUCCESS)
    {
        if (RegSetValueEx(hKey, _T("LogPixels"), 0, REG_DWORD, (LPBYTE)&dwDefDPI, sizeof(DWORD)) == ERROR_SUCCESS)
        {
            _tcscpy(szDPI, _T("96"));
            RegCloseKey(hKey);
            return;
        }
    }
    else wsprintf(szDPI, _T("%d"), dwDPI);

    RegCloseKey(hKey);
}

VOID
SetNonUnicodeLang(HWND hwnd, LCID lcid)
{
    TCHAR szDefCP[5 + 1], szSection[MAX_PATH], szDPI[3 + 1];
    HINF hFontInf;
    UINT Count;

    GetLocaleInfo(MAKELCID(lcid, SORT_DEFAULT), LOCALE_IDEFAULTCODEPAGE, szDefCP, sizeof(szDefCP) / sizeof(TCHAR));
    GetCurrentDPI(szDPI);

    wsprintf(szSection, _T("Font.CP%s.%s"), szDefCP, szDPI);

    hFontInf = SetupOpenInfFile(_T("font.inf"), NULL, INF_STYLE_WIN4, NULL);

    if (hFontInf == INVALID_HANDLE_VALUE)
        return;

    if (!SetupOpenAppendInfFile(NULL, hFontInf, NULL))
    {
        SetupCloseInfFile(hFontInf);
        return;
    }

    Count = (UINT) SetupGetLineCount(hFontInf, szSection);
    if (Count <= 0) return;

    if (!SetupInstallFromInfSection(hwnd, hFontInf, szSection, SPINST_REGISTRY & ~SPINST_FILES,
                                    NULL, NULL, 0, NULL, NULL, NULL, NULL))
        MessageBox(hwnd, _T("Unable to install a new language for programs don't support unicode!"),
                   NULL, MB_ICONERROR | MB_OK);

    SetupCloseInfFile(hFontInf);
}

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            InitLanguagesList(hwndDlg);
            InitCodePagesList(hwndDlg);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_LANGUAGE_COMBO:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            if (lpnm->code == (UINT)PSN_APPLY)
            {
                LCID lcid;
                INT iIndex;

                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

                iIndex = SendMessage(hLangList, CB_GETCURSEL, 0, 0);
                if (iIndex == CB_ERR)
                    break;

                lcid = SendMessage(hLangList, CB_GETITEMDATA, iIndex, 0);
                if (lcid == (LCID)CB_ERR)
                    break;

                SetNonUnicodeLang(hwndDlg, lcid);
            }
        }
        break;
    }

    return FALSE;
}

/* EOF */
