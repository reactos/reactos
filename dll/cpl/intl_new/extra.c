/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/extra.c
 * PURPOSE:         Extra parameters page
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

HWND hLanguageList;

/* FUNCTIONS ****************************************************************/

/* Language enumerate procedure */
BOOL
CALLBACK
LanguagesEnumProc(LPTSTR lpLanguage)
{
    LCID Lcid;
    TCHAR Lang[MAX_STR_SIZE];
    int Index;

    Lcid = wcstoul(lpLanguage, NULL, 16);

    GetLocaleInfo(Lcid, LOCALE_SLANGUAGE, Lang, sizeof(Lang));
    Index = (int) SendMessageW(hLanguageList,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)Lang);

    SendMessageW(hLanguageList,
                 CB_SETITEMDATA,
                 Index,
                 (LPARAM)Lcid);

    return TRUE;
}

/* Enumerate all installed language identifiers */
static
VOID
CreateLanguagesList(HWND hWnd)
{
    TCHAR LangSel[MAX_STR_SIZE];

    hLanguageList = hWnd;

    EnumSystemLocalesW(LanguagesEnumProc, LCID_INSTALLED);

    // Select current locale
    GetLocaleInfo(GetUserDefaultLCID(),
                  LOCALE_SLANGUAGE,
                  LangSel,
                  sizeof(LangSel));

    SendMessageW(hLanguageList,
                 CB_SELECTSTRING,
                (WPARAM) -1,
                (LPARAM)LangSel);
}


/* Extra Parameters page dialog callback */
INT_PTR
CALLBACK
ExtraOptsProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch(uMsg)
    {
        case WM_INITDIALOG:
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGE_COMBO));
        break;

        case WM_COMMAND:

        break;

        case WM_NOTIFY:
        {
            LPNMHDR Lpnm = (LPNMHDR)lParam;
            /* If push apply button */
            if (Lpnm->code == (UINT)PSN_APPLY)
            {
                // TODO: Implement
            }
        }
        break;
    }
    return FALSE;
}

/* EOF */
