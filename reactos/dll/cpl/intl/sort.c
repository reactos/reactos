/* $Id$
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/sort.c
 * PURPOSE:         Sorting property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

static BOOL bSortPage = FALSE;
static LCID userLcid;

static HWND hWndSortList = NULL;


static BOOL CALLBACK
SortTestEnumProc(LPTSTR lpLocale)
{
    LCID lcid;

    lcid = _tcstoul(lpLocale, NULL, 16);

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

    EnumSystemLocales(SortTestEnumProc, LCID_ALTERNATE_SORTS);

    return bSortPage;
}


static BOOL CALLBACK
SortEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    TCHAR lang[255];
    INT index;

    lcid = _tcstoul(lpLocale, NULL, 16);

    if ((LANGIDFROMLCID(lcid) == LANGIDFROMLCID(userLcid)) &&
        (SORTIDFROMLCID(lcid) != SORTIDFROMLCID(userLcid)))
    {
        GetLocaleInfo(lcid, LOCALE_SSORTNAME, lang, sizeof(lang));

        index = SendMessage(hWndSortList,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lang);

        SendMessage(hWndSortList,
                    CB_SETITEMDATA,
                    index,
                    (LPARAM)lcid);
    }

    return TRUE;
}

static VOID
CreateSortList(HWND hwnd, LCID lcid)
{
    TCHAR lang[255];
    INT index;

    hWndSortList = hwnd;

    /* Handle special case for Spainish (Spain) */
    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        /* Add traditional sorting */
        GetLocaleInfo(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH),
                      LOCALE_SSORTNAME, lang, sizeof(lang));

        index = SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                           (LPARAM)lang);

        SendMessage(hwnd,
                    CB_SETITEMDATA,
                    index,
                    (LPARAM)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH));

        /* Add modern sorting */
        GetLocaleInfo(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN),
                      LOCALE_SSORTNAME, lang, sizeof(lang));

        index = SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                           (LPARAM)lang);

        SendMessage(hwnd,
                    CB_SETITEMDATA,
                    index,
                    (LPARAM)MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN));
    }
    else
    {
        userLcid = lcid;

        GetLocaleInfo(lcid & 0xFFFF, LOCALE_SSORTNAME, lang, sizeof(lang));

        index = SendMessage(hWndSortList,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)lang);

        SendMessage(hWndSortList,
                    CB_SETITEMDATA,
                    index,
                    (LPARAM)lcid & 0xFFFF);

        EnumSystemLocales(SortEnumProc, LCID_ALTERNATE_SORTS);
    }

    /* Select current locale */
    /* or should it be System and not user? */
    GetLocaleInfo(lcid, LOCALE_SSORTNAME, lang, sizeof(lang));

    SendMessage(hwnd,
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

            CreateSortList(GetDlgItem(hwndDlg, IDC_SORTLIST_COMBO), pGlobalData->lcid);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SORTLIST_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
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
                        pGlobalData->lcid = NewLcid;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
            {
                /* FIXME: Set locale ID: pGlobalData->lcid */
            }
            break;
    }

  return FALSE;
}

/* EOF */
