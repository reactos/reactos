/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Userinit Logon Application
 * FILE:        base/system/userinit/livecd.c
 * PROGRAMMERS: Eric Kohl
 */

#include "userinit.h"

HWND hList;
HWND hLocaleList;
BOOL bSpain = FALSE;

static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hInstance,
                                 MAKEINTRESOURCE(IDB_ROSLOGO),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}


BOOL
IsLiveCD(VOID)
{
    HKEY ControlKey = NULL;
    LPWSTR SystemStartOptions = NULL;
    LPWSTR CurrentOption, NextOption; /* Pointers into SystemStartOptions */
    LONG rc;
    BOOL ret = FALSE;

    TRACE("IsLiveCD()\n");

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      REGSTR_PATH_CURRENT_CONTROL_SET,
                      0,
                      KEY_QUERY_VALUE,
                      &ControlKey);
    if (rc != ERROR_SUCCESS)
    {
        WARN("RegOpenKeyEx() failed with error %lu\n", rc);
        goto cleanup;
    }

    rc = ReadRegSzKey(ControlKey, L"SystemStartOptions", &SystemStartOptions);
    if (rc != ERROR_SUCCESS)
    {
        WARN("ReadRegSzKey() failed with error %lu\n", rc);
        goto cleanup;
    }

    /* Check for CONSOLE switch in SystemStartOptions */
    CurrentOption = SystemStartOptions;
    while (CurrentOption)
    {
        NextOption = wcschr(CurrentOption, L' ');
        if (NextOption)
            *NextOption = L'\0';
        if (_wcsicmp(CurrentOption, L"MININT") == 0)
        {
            TRACE("Found 'MININT' boot option\n");
            ret = TRUE;
            goto cleanup;
        }
        CurrentOption = NextOption ? NextOption + 1 : NULL;
    }

cleanup:
    if (ControlKey != NULL)
        RegCloseKey(ControlKey);
    HeapFree(GetProcessHeap(), 0, SystemStartOptions);

    TRACE("IsLiveCD() returning %d\n", ret);

    return ret;
}


static BOOL CALLBACK
LocalesEnumProc(LPTSTR lpLocale)
{
    LCID lcid;
    WCHAR lang[255];
    INT index;
    BOOL bNoShow = FALSE;

    lcid = wcstoul(lpLocale, NULL, 16);

    /* Display only languages with installed support */
    if (!IsValidLocale(lcid, LCID_INSTALLED))
        return TRUE;

    if (lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) ||
        lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT))
    {
        if (bSpain == FALSE)
        {
            LoadStringW(hInstance, IDS_SPAIN, lang, 255);
            bSpain = TRUE;
        }
        else
        {
            bNoShow = TRUE;
        }
    }
    else
    {
        GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang)/sizeof(WCHAR));
    }

    if (bNoShow == FALSE)
    {
        index = SendMessageW(hList,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)lang);

        SendMessageW(hList,
                     CB_SETITEMDATA,
                     index,
                     (LPARAM)lcid);
    }

    return TRUE;
}


static VOID
CreateLanguagesList(HWND hwnd)
{
    WCHAR langSel[255];

    hList = hwnd;
    bSpain = FALSE;
    EnumSystemLocalesW(LocalesEnumProc, LCID_SUPPORTED);

    /* Select current locale */
    /* or should it be System and not user? */
    GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, sizeof(langSel)/sizeof(WCHAR));

    SendMessageW(hList,
                 CB_SELECTSTRING,
                 -1,
                 (LPARAM)langSel);
}


static
VOID
InitializeDefaultUserLocale(
    PLCID pNewLcid)
{
    WCHAR szBuffer[80];
    PWSTR ptr;
    HKEY hLocaleKey;
    DWORD ret;
    DWORD dwSize;
    LCID lcid;
    INT i;

    struct {LCTYPE LCType; PWSTR pValue;} LocaleData[] = {
        /* Number */
        {LOCALE_SDECIMAL, L"sDecimal"},
        {LOCALE_STHOUSAND, L"sThousand"},
        {LOCALE_SNEGATIVESIGN, L"sNegativeSign"},
        {LOCALE_SPOSITIVESIGN, L"sPositiveSign"},
        {LOCALE_SGROUPING, L"sGrouping"},
        {LOCALE_SLIST, L"sList"},
        {LOCALE_SNATIVEDIGITS, L"sNativeDigits"},
        {LOCALE_INEGNUMBER, L"iNegNumber"},
        {LOCALE_IDIGITS, L"iDigits"},
        {LOCALE_ILZERO, L"iLZero"},
        {LOCALE_IMEASURE, L"iMeasure"},
        {LOCALE_IDIGITSUBSTITUTION, L"NumShape"},

        /* Currency */
        {LOCALE_SCURRENCY, L"sCurrency"},
        {LOCALE_SMONDECIMALSEP, L"sMonDecimalSep"},
        {LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep"},
        {LOCALE_SMONGROUPING, L"sMonGrouping"},
        {LOCALE_ICURRENCY, L"iCurrency"},
        {LOCALE_INEGCURR, L"iNegCurr"},
        {LOCALE_ICURRDIGITS, L"iCurrDigits"},

        /* Time */
        {LOCALE_STIMEFORMAT, L"sTimeFormat"},
        {LOCALE_STIME, L"sTime"},
        {LOCALE_S1159, L"s1159"},
        {LOCALE_S2359, L"s2359"},
        {LOCALE_ITIME, L"iTime"},
        {LOCALE_ITIMEMARKPOSN, L"iTimePrefix"},
        {LOCALE_ITLZERO, L"iTLZero"},

        /* Date */
        {LOCALE_SLONGDATE, L"sLongDate"},
        {LOCALE_SSHORTDATE, L"sShortDate"},
        {LOCALE_SDATE, L"sDate"},
        {LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek"},
        {LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear"},
        {LOCALE_IDATE, L"iDate"},
        {LOCALE_ICALENDARTYPE, L"iCalendarType"},

        /* Misc */
        {LOCALE_SCOUNTRY, L"sCountry"},
        {LOCALE_SLANGUAGE, L"sLanguage"},
        {LOCALE_ICOUNTRY, L"iCountry"},
        {0, NULL}};

    ret = RegOpenKeyExW(HKEY_USERS,
                        L".DEFAULT\\Control Panel\\International",
                        0,
                        KEY_READ | KEY_WRITE,
                        &hLocaleKey);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

    if (pNewLcid == NULL)
    {
        dwSize = 9 * sizeof(WCHAR);
        ret = RegQueryValueExW(hLocaleKey,
                               L"Locale",
                               NULL,
                               NULL,
                               (PBYTE)szBuffer,
                               &dwSize);
        if (ret != ERROR_SUCCESS)
            goto done;

        lcid = (LCID)wcstoul(szBuffer, &ptr, 16);
        if (lcid == 0)
            goto done;
    }
    else
    {
        lcid = *pNewLcid;

        swprintf(szBuffer, L"%08lx", lcid);
        RegSetValueExW(hLocaleKey,
                       L"Locale",
                       0,
                       REG_SZ,
                       (PBYTE)szBuffer,
                       (wcslen(szBuffer) + 1) * sizeof(WCHAR));
    }

    i = 0;
    while (LocaleData[i].pValue != NULL)
    {
        if (GetLocaleInfo(lcid,
                          LocaleData[i].LCType | LOCALE_NOUSEROVERRIDE,
                          szBuffer,
                          sizeof(szBuffer) / sizeof(WCHAR)))
        {
            RegSetValueExW(hLocaleKey,
                           LocaleData[i].pValue,
                           0,
                           REG_SZ,
                           (PBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));
        }

        i++;
    }

done:
    RegCloseKey(hLocaleKey);
}


VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
                 HWND_TOP,
                 ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
                 ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0,
                 0,
                 SWP_NOSIZE);
}


static
VOID
OnDrawItem(
     LPDRAWITEMSTRUCT lpDrawItem,
     PSTATE pState,
     UINT uCtlID)
{
    HDC hdcMem;
    LONG left;

    if (lpDrawItem->CtlID == uCtlID)
    {
        /* position image in centre of dialog */
        left = (lpDrawItem->rcItem.right - pState->ImageInfo.cxSource) / 2;

        hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
        if (hdcMem != NULL)
        {
            SelectObject(hdcMem, pState->ImageInfo.hBitmap);
            BitBlt(lpDrawItem->hDC,
                   left,
                   lpDrawItem->rcItem.top,
                   lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                   lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                   hdcMem,
                   0,
                   0,
                   SRCCOPY);
            DeleteDC(hdcMem);
        }
    }
}


static
INT_PTR
CALLBACK
LocaleDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSTATE pState;

    /* Retrieve pointer to the state */
    pState = (PSTATE)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global state */
            pState = (PSTATE)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pState);

            /* Center the dialog window */
            CenterWindow (hwndDlg);
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));

            EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
            return FALSE;

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam,
                       pState,
                       IDC_LOCALELOGO);
            return TRUE;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDOK:
                        {
                        LCID NewLcid;
                        INT iCurSel;

                        iCurSel = SendDlgItemMessageW(hwndDlg,
                                                     IDC_LANGUAGELIST,
                                                     CB_GETCURSEL,
                                                     0,
                                                     0);
                if (iCurSel == CB_ERR)
                    break;

                NewLcid = SendDlgItemMessageW(hwndDlg,
                                              IDC_LANGUAGELIST,
                                              CB_GETITEMDATA,
                                              iCurSel,
                                              0);
                if (NewLcid == (LCID)CB_ERR)
                    break;

                            NtSetDefaultLocale(TRUE, NewLcid);
                            InitializeDefaultUserLocale(&NewLcid);
                        }

                        pState->NextPage = STARTPAGE;
                        EndDialog(hwndDlg, 0);
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }

    return FALSE;
}


static
INT_PTR
CALLBACK
StartDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSTATE pState;

    /* Retrieve pointer to the state */
    pState = (PSTATE)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the state */
            pState = (PSTATE)lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pState);

            /* Center the dialog window */
            CenterWindow(hwndDlg);

            EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
            return FALSE;

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam,
                       pState,
                       IDC_STARTLOGO);
            return TRUE;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_RUN:
                        pState->NextPage = DONE;
                        pState->Run = SHELL;
                        EndDialog(hwndDlg, 0);
                        break;

                    case IDC_INSTALL:
                        pState->NextPage = DONE;
                        pState->Run = INSTALLER;
                        EndDialog(hwndDlg, 0);
                        break;

                    case IDOK:
                        pState->NextPage = LOCALEPAGE;
                        EndDialog(hwndDlg, 0);
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }

    return FALSE;
}


VOID
RunLiveCD(
    PSTATE pState)
{
    InitImageInfo(&pState->ImageInfo);

    while (pState->NextPage != DONE)
    {
        switch (pState->NextPage)
        {
            case LOCALEPAGE:
                DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_LOCALEPAGE),
                               NULL,
                               LocaleDlgProc,
                               (LPARAM)pState);
                break;

            case STARTPAGE:
                DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_STARTPAGE),
                               NULL,
                               StartDlgProc,
                               (LPARAM)pState);
                break;

            default:
                break;
        }
    }

    DeleteObject(pState->ImageInfo.hBitmap);
}

/* EOF */
