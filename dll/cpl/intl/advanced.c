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

static BOOL
GetSupportedCP(VOID)
{
    UINT uiCPage, Count, Number;
    INFCONTEXT infCont;
    LPCPAGE lpCPage;
    HANDLE hCPage;
    CPINFOEX cpInfEx;
    TCHAR Section[MAX_PATH];

    Count = (UINT) SetupGetLineCount(hIntlInf, _T("CodePages"));
    if (Count <= 0) return FALSE;

    for (Number = 0; Number < Count; Number++)
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

            _stprintf(Section, _T("%s%d"), _T("CODEPAGE_REMOVE_"), uiCPage);
            if ((uiCPage == GetACP()) || (uiCPage == GetOEMCP()) || 
                (!SetupFindFirstLine(hIntlInf, Section, _T("AddReg"), &infCont)))
            {
                lpCPage->Status |= (0x0001 | 0x0002);
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
InitLangList(HWND hwndDlg)
{
    LPCPAGE lpCPage;
    INT ItemIndex;
    HWND hList;
    LV_COLUMN column;
    LV_ITEM item;
    RECT ListRect;

    hList = GetDlgItem(hwndDlg, IDC_CONV_TABLES);

    hIntlInf = SetupOpenInfFile(_T("intl.inf"), NULL, INF_STYLE_WIN4, NULL);

    if (hIntlInf == INVALID_HANDLE_VALUE) return;

    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        hIntlInf = NULL;
        return;
    }

    if (!GetSupportedCP()) return;

    SetupCloseInfFile(hIntlInf);

    if (!EnumSystemCodePages(InstalledCPProc, CP_INSTALLED)) return;

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
            InitLangList(hwndDlg);
        }
        break;
    }

    return FALSE;
}

/* EOF */
