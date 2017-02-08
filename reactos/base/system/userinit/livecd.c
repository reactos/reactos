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

    ImgInfo->hBitmap = LoadImageW(hInstance,
                                  MAKEINTRESOURCEW(IDB_ROSLOGO),
                                  IMAGE_BITMAP,
                                  0,
                                  0,
                                  LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(bitmap), &bitmap);

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

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
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
            LoadStringW(hInstance, IDS_SPAIN, lang, ARRAYSIZE(lang));
            bSpain = TRUE;
        }
        else
        {
            bNoShow = TRUE;
        }
    }
    else
    {
        GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, lang, ARRAYSIZE(lang));
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
    GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, ARRAYSIZE(langSel));

    SendMessageW(hList,
                 CB_SELECTSTRING,
                 -1,
                 (LPARAM)langSel);
}


static
BOOL
GetLayoutName(
    LPCWSTR szLCID,
    LPWSTR szName)
{
    HKEY hKey;
    DWORD dwBufLen;
    WCHAR szBuf[MAX_PATH], szDispName[MAX_PATH], szIndex[MAX_PATH], szPath[MAX_PATH];
    HANDLE hLib;
    UINT i, j, k;

    wsprintf(szBuf, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s", szLCID);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szDispName);

        if (RegQueryValueExW(hKey, L"Layout Display Name", NULL, NULL, (LPBYTE)szDispName, &dwBufLen) == ERROR_SUCCESS)
        {
            if (szDispName[0] == '@')
            {
                for (i = 0; i < wcslen(szDispName); i++)
                {
                    if ((szDispName[i] == ',') && (szDispName[i + 1] == '-'))
                    {
                        for (j = i + 2, k = 0; j < wcslen(szDispName)+1; j++, k++)
                        {
                            szIndex[k] = szDispName[j];
                        }
                        szDispName[i - 1] = '\0';
                        break;
                    }
                    else
                        szDispName[i] = szDispName[i + 1];
                }

                if (ExpandEnvironmentStringsW(szDispName, szPath, ARRAYSIZE(szPath)))
                {
                    hLib = LoadLibraryW(szPath);
                    if (hLib)
                    {
                        if (LoadStringW(hLib, _wtoi(szIndex), szPath, ARRAYSIZE(szPath)) != 0)
                        {
                            wcscpy(szName, szPath);
                            RegCloseKey(hKey);
                            return TRUE;
                        }
                        FreeLibrary(hLib);
                    }
                }
            }
        }

        dwBufLen = sizeof(szBuf);

        if (RegQueryValueExW(hKey, L"Layout Text", NULL, NULL, (LPBYTE)szName, &dwBufLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}


static
VOID
SetKeyboardLayout(
    HWND hwnd)
{
    INT iCurSel;
    ULONG ulLayoutId;
    HKL hKl;
    WCHAR szLayoutId[9];

    iCurSel = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
    if (iCurSel == CB_ERR)
        return;

    ulLayoutId = (ULONG)SendMessageW(hwnd, CB_GETITEMDATA, iCurSel, 0);
    if (ulLayoutId == (ULONG)CB_ERR)
        return;

    swprintf(szLayoutId, L"%08lx", ulLayoutId);

    hKl = LoadKeyboardLayoutW(szLayoutId, KLF_ACTIVATE | KLF_REPLACELANG | KLF_SETFORPROCESS);
    SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG, 0, &hKl, SPIF_SENDWININICHANGE);
}


static
VOID
SelectKeyboardForLanguage(
    HWND hwnd,
    LCID lcid)
{
    INT i, nCount;
    LCID LayoutId;

    TRACE("LCID: %08lx\n", lcid);
    TRACE("LangID: %04lx\n", LANGIDFROMLCID(lcid));

    nCount = SendMessageW(hwnd, CB_GETCOUNT, 0, 0);

    for (i = 0; i < nCount; i++)
    {
        LayoutId = (LCID)SendMessageW(hwnd, CB_GETITEMDATA, i, 0);
        TRACE("Layout: %08lx\n", LayoutId);

        if (LANGIDFROMLCID(LayoutId) == LANGIDFROMLCID(lcid))
        {
            TRACE("Found 1: %08lx --> %08lx\n", LayoutId, lcid);
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return;
        }
    }

    for (i = 0; i < nCount; i++)
    {
        LayoutId = (LCID)SendMessageW(hwnd, CB_GETITEMDATA, i, 0);
        TRACE("Layout: %08lx\n", LayoutId);

        if (PRIMARYLANGID(LayoutId) == PRIMARYLANGID(lcid))
        {
            TRACE("Found 2: %08lx --> %08lx\n", LayoutId, lcid);
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return;
        }
    }

    TRACE("No match found!\n");
}


static
VOID
CreateKeyboardLayoutList(
    HWND hItemsList)
{
    HKEY hKey;
    WCHAR szLayoutId[9], szCurrentLayoutId[9];
    WCHAR KeyName[MAX_PATH];
    DWORD dwIndex = 0;
    DWORD dwSize;
    INT iIndex;
    LONG lError;
    ULONG ulLayoutId;

    if (!GetKeyboardLayoutNameW(szCurrentLayoutId))
        wcscpy(szCurrentLayoutId, L"00000409");

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\CurrentControlSet\\Control\\Keyboard Layouts",
                           0,
                           KEY_ENUMERATE_SUB_KEYS,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return;

    while (TRUE)
    {
        dwSize = ARRAYSIZE(szLayoutId);

        lError = RegEnumKeyExW(hKey,
                               dwIndex,
                               szLayoutId,
                               &dwSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS)
            break;

        GetLayoutName(szLayoutId, KeyName);

        iIndex = (INT)SendMessageW(hItemsList, CB_ADDSTRING, 0, (LPARAM)KeyName);

        ulLayoutId = wcstoul(szLayoutId, NULL, 16);
        SendMessageW(hItemsList, CB_SETITEMDATA, iIndex, (LPARAM)ulLayoutId);

        if (wcscmp(szLayoutId, szCurrentLayoutId) == 0)
        {
            SendMessageW(hItemsList, CB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0);
        }

        dwIndex++;
    }

    RegCloseKey(hKey);
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
        if (GetLocaleInfoW(lcid,
                           LocaleData[i].LCType | LOCALE_NOUSEROVERRIDE,
                           szBuffer,
                           ARRAYSIZE(szBuffer)))
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
        /* Position image in centre of dialog */
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
    pState = (PSTATE)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global state */
            pState = (PSTATE)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pState);

            /* Center the dialog window */
            CenterWindow(hwndDlg);

            /* Fill the language and keyboard layout lists */
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));
            CreateKeyboardLayoutList(GetDlgItem(hwndDlg, IDC_LAYOUTLIST));

            /* Disable the 'Cancel' button*/
            EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
            return FALSE;

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam,
                       pState,
                       IDC_LOCALELOGO);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_LANGUAGELIST:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
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

                        TRACE("LCID: 0x%08lx\n", NewLcid);
                        SelectKeyboardForLanguage(GetDlgItem(hwndDlg, IDC_LAYOUTLIST),
                                                  NewLcid);
                    }
                    break;

                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
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

                        /* Set the locale for the current thread */
                        NtSetDefaultLocale(TRUE, NewLcid);

                        /* Store the locale setings in the registry */
                        InitializeDefaultUserLocale(&NewLcid);

                        SetKeyboardLayout(GetDlgItem(hwndDlg, IDC_LAYOUTLIST));

                        pState->NextPage = STARTPAGE;
                        EndDialog(hwndDlg, 0);
                    }
                    break;

                default:
                    break;
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
    pState = (PSTATE)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the state */
            pState = (PSTATE)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pState);

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
                DialogBoxParamW(hInstance,
                                MAKEINTRESOURCEW(IDD_LOCALEPAGE),
                                NULL,
                                LocaleDlgProc,
                                (LPARAM)pState);
                break;

            case STARTPAGE:
                DialogBoxParamW(hInstance,
                                MAKEINTRESOURCEW(IDD_STARTPAGE),
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
