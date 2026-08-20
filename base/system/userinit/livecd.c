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

WCHAR Installer[MAX_PATH];

typedef struct _LIVECD_UNATTEND
{
    BOOL bEnabled;
    LCID LocaleID;
} LIVECD_UNATTEND;


/*
 * Taken and adapted from dll/cpl/sysdm/general.c
 */
static VOID
InitLogo(PIMGINFO pImgInfo, HWND hwndDlg)
{
    BITMAP logoBitmap;
    BITMAP maskBitmap;
    BITMAPINFO bmpi;
    HDC hDC, hDCLogo, hDCMask;
    HBITMAP hMask = NULL, hLogo = NULL;
    HBITMAP hAlphaLogo = NULL;
    COLORREF *pBits;
    INT line, column;

    hDC = GetDC(hwndDlg);
    hDCLogo = CreateCompatibleDC(NULL);
    hDCMask = CreateCompatibleDC(NULL);

    if (hDC == NULL || hDCLogo == NULL || hDCMask == NULL)
        goto Cleanup;

    ZeroMemory(pImgInfo, sizeof(*pImgInfo));
    ZeroMemory(&bmpi, sizeof(bmpi));

    hLogo = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_ROSLOGO), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    hMask = (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_ROSMASK), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

    if (hLogo == NULL || hMask == NULL)
        goto Cleanup;

    GetObject(hLogo, sizeof(logoBitmap), &logoBitmap);
    GetObject(hMask, sizeof(maskBitmap), &maskBitmap);

    if (logoBitmap.bmHeight != maskBitmap.bmHeight || logoBitmap.bmWidth != maskBitmap.bmWidth)
        goto Cleanup;

    bmpi.bmiHeader.biSize = sizeof(BITMAPINFO);
    bmpi.bmiHeader.biWidth = logoBitmap.bmWidth;
    bmpi.bmiHeader.biHeight = logoBitmap.bmHeight;
    bmpi.bmiHeader.biPlanes = 1;
    bmpi.bmiHeader.biBitCount = 32;
    bmpi.bmiHeader.biCompression = BI_RGB;
    bmpi.bmiHeader.biSizeImage = 4 * logoBitmap.bmWidth * logoBitmap.bmHeight;

    /* Create a premultiplied bitmap */
    hAlphaLogo = CreateDIBSection(hDC, &bmpi, DIB_RGB_COLORS, (PVOID*)&pBits, 0, 0);
    if (!hAlphaLogo)
        goto Cleanup;

    SelectObject(hDCLogo, hLogo);
    SelectObject(hDCMask, hMask);

    for (line = logoBitmap.bmHeight - 1; line >= 0; line--)
    {
        for (column = 0; column < logoBitmap.bmWidth; column++)
        {
            COLORREF alpha = GetPixel(hDCMask, column, line) & 0xFF;
            COLORREF Color = GetPixel(hDCLogo, column, line);
            DWORD r, g, b;

            r = GetRValue(Color) * alpha / 255;
            g = GetGValue(Color) * alpha / 255;
            b = GetBValue(Color) * alpha / 255;

            *pBits++ = b | (g << 8) | (r << 16) | (alpha << 24);
        }
    }

    pImgInfo->hBitmap = hAlphaLogo;
    pImgInfo->cxSource = logoBitmap.bmWidth;
    pImgInfo->cySource = logoBitmap.bmHeight;
    pImgInfo->iBits = logoBitmap.bmBitsPixel;
    pImgInfo->iPlanes = logoBitmap.bmPlanes;

Cleanup:
    if (hMask != NULL) DeleteObject(hMask);
    if (hLogo != NULL) DeleteObject(hLogo);
    if (hDCMask != NULL) DeleteDC(hDCMask);
    if (hDCLogo != NULL) DeleteDC(hDCLogo);
    if (hDC != NULL) ReleaseDC(hwndDlg, hDC);
}

/**
 * @brief   Check whether we are running in MiniNT mode (e.g. live medium).
 **/
BOOL
IsMiniNT(VOID)
{
    HKEY hKey;
    LONG rc;

    /* Check for the presence of the "MiniNT" registry key */
    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SYSTEM\\CurrentControlSet\\Control\\MiniNT",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc == ERROR_SUCCESS)
        RegCloseKey(hKey);

    return (rc == ERROR_SUCCESS);
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

    // See http://archives.miloush.net/michkap/archive/2006/09/23/768178.html for why we handle spain differently
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
CreateLanguagesList(HWND hwnd, PSTATE pState)
{
    WCHAR langSel[255];
    LCID Locale = 0;

    hList = hwnd;
    bSpain = FALSE;
    EnumSystemLocalesW(LocalesEnumProc, LCID_SUPPORTED);

    if (pState->Unattend->bEnabled)
        Locale = pState->Unattend->LocaleID;

    if (!Locale)
    {
        /* Select current locale */
        /* or should it be System and not user? */
        Locale = GetUserDefaultLCID();
    }
    GetLocaleInfoW(Locale, LOCALE_SLANGUAGE, langSel, ARRAYSIZE(langSel));

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

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
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
    WCHAR szKLID[KL_NAMELENGTH];

    iCurSel = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
    if (iCurSel == CB_ERR)
        return;

    ulLayoutId = (ULONG)SendMessageW(hwnd, CB_GETITEMDATA, iCurSel, 0);
    if (ulLayoutId == (ULONG)CB_ERR)
        return;

    _swprintf(szKLID, L"%08lx", ulLayoutId);

    hKl = LoadKeyboardLayoutW(szKLID, KLF_ACTIVATE | KLF_REPLACELANG | KLF_SETFORPROCESS);
    SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG, 0, &hKl, SPIF_SENDCHANGE);
}

static
VOID
SelectKeyboardForLanguage(
    HWND hwnd,
    LCID lcid)
{
    INT i, nCount;
    ULONG ulLayoutId;

    TRACE("LCID: %08lx, LangID: %04lx\n", lcid, LANGIDFROMLCID(lcid));

    nCount = SendMessageW(hwnd, CB_GETCOUNT, 0, 0);

    for (i = 0; i < nCount; i++)
    {
        ulLayoutId = (ULONG)SendMessageW(hwnd, CB_GETITEMDATA, i, 0);
        TRACE("Layout: %08lx\n", ulLayoutId);

        if (LANGIDFROMLCID(ulLayoutId) == LANGIDFROMLCID(lcid))
        {
            TRACE("Found 1: %08lx --> %08lx\n", ulLayoutId, lcid);
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return;
        }
    }

    for (i = 0; i < nCount; i++)
    {
        ulLayoutId = (ULONG)SendMessageW(hwnd, CB_GETITEMDATA, i, 0);
        TRACE("Layout: %08lx\n", ulLayoutId);

        if (PRIMARYLANGID(ulLayoutId) == PRIMARYLANGID(lcid))
        {
            TRACE("Found 2: %08lx --> %08lx\n", ulLayoutId, lcid);
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return;
        }
    }

    TRACE("No match found!\n");
    /* No match found, try selecting English (United States) */
    for (i = 0; i < nCount; i++)
    {
        ulLayoutId = (DWORD)SendMessageW(hwnd, CB_GETITEMDATA, i, 0);
        if (ulLayoutId == 0x00000409)
        {
            SendMessageW(hwnd, CB_SETCURSEL, i, 0);
            return;
        }
    }
}

static
VOID
CreateKeyboardLayoutList(
    HWND hItemsList)
{
    HKEY hKey;
    WCHAR szCurrentKLID[KL_NAMELENGTH], szKLID[KL_NAMELENGTH];
    WCHAR KeyName[MAX_PATH];
    DWORD dwIndex;
    DWORD dwSize;
    INT iIndex;
    ULONG ulLayoutId;

    if (!GetKeyboardLayoutNameW(szCurrentKLID))
        wcscpy(szCurrentKLID, L"00000409");

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\Keyboard Layouts",
                      0, KEY_ENUMERATE_SUB_KEYS, &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    for (dwIndex = 0; ; ++dwIndex)
    {
        dwSize = ARRAYSIZE(szKLID);
        if (RegEnumKeyExW(hKey, dwIndex, szKLID, &dwSize,
                          NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            break;
        }

        GetLayoutName(szKLID, KeyName);

        iIndex = (INT)SendMessageW(hItemsList, CB_ADDSTRING, 0, (LPARAM)KeyName);

        ulLayoutId = wcstoul(szKLID, NULL, 16);
        SendMessageW(hItemsList, CB_SETITEMDATA, iIndex, (LPARAM)ulLayoutId);

        if (_wcsicmp(szKLID, szCurrentKLID) == 0)
            SendMessageW(hItemsList, CB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0);
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
        {LOCALE_SABBREVLANGNAME, L"sLanguage"},
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

        _swprintf(szBuffer, L"%08lx", lcid);
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
            static BLENDFUNCTION BlendFunc = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

            SelectObject(hdcMem, pState->ImageInfo.hBitmap);
            GdiAlphaBlend(lpDrawItem->hDC,
                          left,
                          lpDrawItem->rcItem.top,
                          pState->ImageInfo.cxSource,
                          pState->ImageInfo.cySource,
                          hdcMem,
                          0, 0,
                          pState->ImageInfo.cxSource,
                          pState->ImageInfo.cySource,
                          BlendFunc);
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
        {
            /* Save pointer to the state */
            pState = (PSTATE)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pState);

            /* Center the dialog window */
            CenterWindow(hwndDlg);

            /* Fill the language and keyboard layout lists */
            CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST), pState);
            CreateKeyboardLayoutList(GetDlgItem(hwndDlg, IDC_LAYOUTLIST));

            /* In unattended mode, advance to the next page */
            if (pState->Unattend->bEnabled)
                SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
            return TRUE;
        }

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam, pState, IDC_LOCALELOGO);
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

                        /* Store the locale settings in the registry */
                        InitializeDefaultUserLocale(&NewLcid);

                        /* Set UI language for this thread */
                        SetThreadLocale(NewLcid);

                        SetKeyboardLayout(GetDlgItem(hwndDlg, IDC_LAYOUTLIST));

                        pState->NextPage = STARTPAGE;
                        EndDialog(hwndDlg, LOWORD(wParam));
                    }
                    break;

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        static WCHAR szMsg[RC_STRING_MAX_SIZE];
                        INT ret;
                        LoadStringW(GetModuleHandle(NULL), IDS_CANCEL_CONFIRM, szMsg, ARRAYSIZE(szMsg));
                        ret = MessageBoxW(hwndDlg, szMsg, L"ReactOS LiveCD", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
                        if (ret == IDOK || ret == IDYES)
                        {
                            pState->NextPage = DONE;
                            pState->Run = REBOOT;
                            EndDialog(hwndDlg, LOWORD(wParam));
                        }
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
        {
            /* Save pointer to the state */
            pState = (PSTATE)lParam;
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pState);

            /* Center the dialog window */
            CenterWindow(hwndDlg);

            /* If the ReactOS Installer could not be located, directly start
             * the LiveCD. Otherwise, directly start the installer if we are
             * in unattended mode. */
            if (!*Installer)
            {
                /* Disable the "Install" button and click on "Run LiveCD" */
                EnableWindow(GetDlgItem(hwndDlg, IDC_INSTALL), FALSE);
                SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_RUN, BN_CLICKED), 0);
            }
            else if (pState->Unattend->bEnabled)
            {
                /* Click on "Install" */
                SendMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INSTALL, BN_CLICKED), 0);
            }
            return TRUE;
        }

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam, pState, IDC_STARTLOGO);
            return TRUE;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_RUN:
                        pState->NextPage = DONE;
                        pState->Run = SHELL;
                        EndDialog(hwndDlg, LOWORD(wParam));
                        break;

                    case IDC_INSTALL:
                        pState->NextPage = DONE;
                        pState->Run = INSTALLER;
                        EndDialog(hwndDlg, LOWORD(wParam));
                        break;

                    case IDOK:
                        pState->NextPage = LOCALEPAGE;
                        EndDialog(hwndDlg, LOWORD(wParam));
                        break;

                    case IDCANCEL:
                        if (HIWORD(wParam) == BN_CLICKED)
                        {
                            static WCHAR szMsg[RC_STRING_MAX_SIZE];
                            INT ret;
                            LoadStringW(GetModuleHandle(NULL), IDS_CANCEL_CONFIRM, szMsg, ARRAYSIZE(szMsg));
                            ret = MessageBoxW(hwndDlg, szMsg, L"ReactOS LiveCD", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
                            if (ret == IDOK || ret == IDYES)
                            {
                                pState->NextPage = DONE;
                                pState->Run = REBOOT;
                                EndDialog(hwndDlg, LOWORD(wParam));
                            }
                        }
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

static VOID
ParseUnattend(
    _In_ LPCWSTR UnattendInf,
    _Out_ LIVECD_UNATTEND* pUnattend)
{
    WCHAR Buffer[MAX_PATH];

    pUnattend->bEnabled = FALSE;

    if (!GetPrivateProfileStringW(L"Unattend", L"Signature", L"", Buffer, _countof(Buffer), UnattendInf))
    {
        ERR("Unable to parse Signature\n");
        return;
    }

    if (_wcsicmp(Buffer, L"$ReactOS$") && _wcsicmp(Buffer, L"$Windows NT$"))
    {
        TRACE("Unknown signature: %S\n", Buffer);
        return;
    }

    if (!GetPrivateProfileStringW(L"Unattend", L"UnattendSetupEnabled", L"", Buffer, _countof(Buffer), UnattendInf))
    {
        ERR("Unable to parse UnattendSetupEnabled\n");
        return;
    }

    if (_wcsicmp(Buffer, L"yes") != 0)
    {
        TRACE("Unattended setup is not enabled\n");
        return;
    }
    /* If the user presses Ctrl+Shift+F10, disable unattended setup */
    if ((GetKeyState(VK_CONTROL) & GetKeyState(VK_SHIFT) & GetKeyState(VK_F10)) < 0)
    {
        WARN("Unattended setup is disabled by keypress\n");
        return;
    }

    pUnattend->bEnabled = TRUE;
    pUnattend->LocaleID = 0;

    if (GetPrivateProfileStringW(L"Unattend", L"LocaleID", L"", Buffer, _countof(Buffer), UnattendInf) && Buffer[0])
    {
        pUnattend->LocaleID = wcstol(Buffer, NULL, 16);
    }
}


/**
 * @brief
 * Expands the path for the ReactOS Installer "reactos.exe".
 * See also base/setup/welcome/welcome.c!ExpandInstallerPath()
 **/
static BOOL
ExpandInstallerPath(
    _In_ PCWSTR pInstallerName,
    _Out_writes_z_(PathSize) PWSTR pInstallerPath,
    _In_ SIZE_T PathSize)
{
    SYSTEM_INFO SystemInfo;
    SIZE_T cchInstallerNameLen;
    PWSTR ptr;
    DWORD dwAttribs;

    cchInstallerNameLen = wcslen(pInstallerName);
    if (PathSize < cchInstallerNameLen)
    {
        /* The buffer is not large enough to contain the installer file name */
        *pInstallerPath = UNICODE_NULL;
        return FALSE;
    }

    /*
     * First, try to find the installer using the default drive, under
     * the directory whose name corresponds to the currently-running
     * CPU architecture.
     */
    GetSystemInfo(&SystemInfo);

    *pInstallerPath = UNICODE_NULL;
    /* Alternatively one can use SharedUserData->NtSystemRoot */
    GetSystemWindowsDirectoryW(pInstallerPath, PathSize - cchInstallerNameLen - 1);
    ptr = wcschr(pInstallerPath, L'\\');
    if (ptr)
        *++ptr = UNICODE_NULL;
    else
        *pInstallerPath = UNICODE_NULL;

    /* Append the corresponding CPU architecture */
    switch (SystemInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            StringCchCatW(pInstallerPath, PathSize, L"I386");
            break;

        case PROCESSOR_ARCHITECTURE_MIPS:
            StringCchCatW(pInstallerPath, PathSize, L"MIPS");
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            StringCchCatW(pInstallerPath, PathSize, L"ALPHA");
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            StringCchCatW(pInstallerPath, PathSize, L"PPC");
            break;

        case PROCESSOR_ARCHITECTURE_SHX:
            StringCchCatW(pInstallerPath, PathSize, L"SHX");
            break;

        case PROCESSOR_ARCHITECTURE_ARM:
            StringCchCatW(pInstallerPath, PathSize, L"ARM");
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            StringCchCatW(pInstallerPath, PathSize, L"IA64");
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA64:
            StringCchCatW(pInstallerPath, PathSize, L"ALPHA64");
            break;

        case PROCESSOR_ARCHITECTURE_AMD64:
            StringCchCatW(pInstallerPath, PathSize, L"AMD64");
            break;

        // case PROCESSOR_ARCHITECTURE_MSIL: /* .NET CPU-independent code */
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            WARN("Unknown processor architecture %lu\n", SystemInfo.wProcessorArchitecture);
            SystemInfo.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
            break;
    }

    if (SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_UNKNOWN)
        StringCchCatW(pInstallerPath, PathSize, L"\\");
    StringCchCatW(pInstallerPath, PathSize, pInstallerName);

    dwAttribs = GetFileAttributesW(pInstallerPath);
    if ((dwAttribs != INVALID_FILE_ATTRIBUTES) &&
        !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        /* We have found the installer */
        return TRUE;
    }

    WARN("Couldn't find the installer '%s', trying alternative.\n", debugstr_w(pInstallerPath));

    /*
     * We failed. Try to find the installer from either the current
     * ReactOS installation directory, or from our current directory.
     */
    *pInstallerPath = UNICODE_NULL;
    /* Alternatively one can use SharedUserData->NtSystemRoot */
    if (GetSystemWindowsDirectoryW(pInstallerPath, PathSize - cchInstallerNameLen - 1))
        StringCchCatW(pInstallerPath, PathSize, L"\\");
    StringCchCatW(pInstallerPath, PathSize, pInstallerName);

    dwAttribs = GetFileAttributesW(pInstallerPath);
    if ((dwAttribs != INVALID_FILE_ATTRIBUTES) &&
        !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        /* We have found the installer */
        return TRUE;
    }

    /* Installer not found */
    ERR("Couldn't find the installer '%s'\n", debugstr_w(pInstallerPath));
    *pInstallerPath = UNICODE_NULL;
    return FALSE;
}

VOID
RunLiveCD(
    PSTATE pState)
{
    LIVECD_UNATTEND Unattend = {0};
    WCHAR UnattendInf[MAX_PATH];

    /* Try to locate the ReactOS Installer */
    if (!ExpandInstallerPath(L"reactos.exe", Installer, _countof(Installer)))
        *Installer = UNICODE_NULL;
    if (*Installer)
        TRACE("ReactOS Installer: '%S'\n", Installer);
    else
        WARN("Could not find the ReactOS Installer\n");

    /* If the ReactOS Installer was located, use its path for the
     * unattended file; otherwise, use the current ReactOS directory. */
    StringCchCopyW(UnattendInf, _countof(UnattendInf), Installer);
    if (*UnattendInf)
    {
        /* Find the last path separator and truncate the path there.
         * If there is none, NUL the path. */
        PWCHAR ptr = wcsrchr(UnattendInf, L'\\');
        if (!ptr)
            ptr = UnattendInf;
        *ptr = UNICODE_NULL;
    }
    if (!*UnattendInf)
    {
        /* No actual path was found, fall back to the ReactOS directory */
        GetWindowsDirectoryW(UnattendInf, _countof(UnattendInf));
    }
    StringCchCatW(UnattendInf, _countof(UnattendInf), L"\\unattend.inf");
    ParseUnattend(UnattendInf, &Unattend);
    pState->Unattend = &Unattend;

    InitLogo(&pState->ImageInfo, NULL);

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
