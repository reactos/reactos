/*
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/sort.c
 * PURPOSE:         Sorting property page
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

static BOOL bSortPage = FALSE;
static LCID userLcid;

static HWND hWndSortList = NULL;


static BOOL CALLBACK
SortTestEnumProc(PWSTR lpLocale)
{
    LCID lcid;

    lcid = wcstoul(lpLocale, NULL, 16);

    if ((LANGIDFROMLCID(lcid) == LANGIDFROMLCID(userLcid)) &&
        (SORTIDFROMLCID(lcid) != SORTIDFROMLCID(userLcid)))
        bSortPage = TRUE;

    return TRUE;
}


BOOL
IsSortPageNeeded(LCID lcid)
{
    /* Handle special case for Spanish (Spain) */
    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
        return TRUE;

    userLcid = lcid;
    bSortPage = FALSE;

    EnumSystemLocalesW(SortTestEnumProc, LCID_ALTERNATE_SORTS);

    return bSortPage;
}


static BOOL CALLBACK
SortEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    WCHAR lang[255];
    INT index;

    lcid = wcstoul(lpLocale, NULL, 16);

    if ((LANGIDFROMLCID(lcid) == LANGIDFROMLCID(userLcid)) &&
        (SORTIDFROMLCID(lcid) != SORTIDFROMLCID(userLcid)))
    {
        GetLocaleInfoW(lcid, LOCALE_SSORTNAME, lang, sizeof(lang)/sizeof(WCHAR));

        index = SendMessageW(hWndSortList,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)lang);

        SendMessageW(hWndSortList,
                     CB_SETITEMDATA,
                     index,
                     (LPARAM)lcid);
    }

    return TRUE;
}

static VOID
CreateSortList(HWND hwnd, LCID lcid)
{
    WCHAR lang[255];
    INT index;

    hWndSortList = hwnd;

    /* Handle special case for Spainish (Spain) */
    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        /* Add traditional sorting */
        GetLocaleInfoW(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH),
                       LOCALE_SSORTNAME, lang, sizeof(lang)/sizeof(TCHAR));

        index = SendMessageW(hwnd,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)lang);

        SendMessageW(hwnd,
                     CB_SETITEMDATA,
                     index,
                     (LPARAM)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH));

        /* Add modern sorting */
        GetLocaleInfoW(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN),
                       LOCALE_SSORTNAME, lang, sizeof(lang)/sizeof(TCHAR));

        index = SendMessageW(hwnd,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)lang);

        SendMessageW(hwnd,
                     CB_SETITEMDATA,
                     index,
                     (LPARAM)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN));
    }
    else
    {
        userLcid = lcid;

        GetLocaleInfoW(lcid & 0xFFFF, LOCALE_SSORTNAME, lang, sizeof(lang)/sizeof(WCHAR));

        index = SendMessageW(hWndSortList,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)lang);

        SendMessageW(hWndSortList,
                     CB_SETITEMDATA,
                     index,
                     (LPARAM)lcid & 0xFFFF);

        EnumSystemLocalesW(SortEnumProc, LCID_ALTERNATE_SORTS);
    }

    /* Select current locale */
    /* or should it be System and not user? */
    GetLocaleInfoW(lcid, LOCALE_SSORTNAME, lang, sizeof(lang)/sizeof(WCHAR));

    SendMessageW(hwnd,
                 CB_SELECTSTRING,
                 -1,
                 (LPARAM)lang);
}

/* Property page dialog callback */
INT_PTR CALLBACK
SortPageProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    PGLOBALDATA pGlobalData;

    pGlobalData = (PGLOBALDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBALDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            CreateSortList(GetDlgItem(hwndDlg, IDC_SORTLIST_COMBO), pGlobalData->UserLCID);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SORTLIST_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                LCID NewLcid;
                INT iCurSel;

                iCurSel = SendDlgItemMessage(hwndDlg,
                                             IDC_SORTLIST_COMBO,
                                             CB_GETCURSEL,
                                             0,
                                             0);
                if (iCurSel == CB_ERR)
                    break;

                NewLcid = SendDlgItemMessage(hwndDlg,
                                             IDC_SORTLIST_COMBO,
                                             CB_GETITEMDATA,
                                             iCurSel,
                                             0);
                if (NewLcid == (LCID)CB_ERR)
                    break;

                /* Save the new LCID */
                pGlobalData->UserLCID = NewLcid;
                pGlobalData->fUserLocaleChanged = TRUE;
            }
            break;
    }

  return FALSE;
}

/* EOF */
