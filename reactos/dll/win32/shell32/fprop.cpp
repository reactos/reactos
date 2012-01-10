/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_PROPERTY_SHEET_PAGE 32

typedef struct _LANGANDCODEPAGE_
{
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

static LONG
SH_GetAssociatedApplication(WCHAR *pwszFileExt, WCHAR *pwszAssocApp)
{
    WCHAR wszBuf[MAX_PATH] = {0};
    LONG result;
    DWORD dwSize = sizeof(wszBuf);

    result = RegGetValueW(HKEY_CLASSES_ROOT, pwszFileExt, L"", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize);

    if (result == ERROR_SUCCESS)
    {
        StringCbCat(wszBuf, sizeof(wszBuf), L"\\shell\\open\\command");
        dwSize = MAX_PATH * sizeof(WCHAR);
        result = RegGetValueW(HKEY_CLASSES_ROOT, wszBuf, L"", RRF_RT_REG_SZ, NULL, pwszAssocApp, &dwSize);
        /* FIXME: Make it return full path instead of
           notepad.exe "%1"
           %systemroot%\notepad.exe "%1"
           etc
           Maybe there is code to do that somewhere?
           dll\win32\shell32\shlexec.c for example? */
    }

    if (result != ERROR_SUCCESS)
        pwszAssocApp[0] = '\0';

    return result;
}

static LONG
SH_FileGeneralOpensWith(HWND hwndDlg, WCHAR *fileext)
{
    LONG result;
    WCHAR wAppName[MAX_PATH] = {0};
    WCHAR wAssocApp[MAX_PATH] = {0};

    result = SH_GetAssociatedApplication(fileext, wAssocApp);

    if (result == ERROR_SUCCESS)
    {
        _wsplitpath(wAssocApp, NULL, NULL, wAppName, NULL);

        SetDlgItemTextW(hwndDlg, 14007, wAppName);
    }

    return result;
}

/*************************************************************************
 *
 * SH_FormatFileSizeWithBytes
 *
 * Format a size in bytes to string.
 *
 * lpQwSize = Pointer to 64bit large integer to format
 * pszBuf   = Buffer to fill with output string
 * cchBuf   = size of pszBuf in characters
 *
 */

LPWSTR
SH_FormatFileSizeWithBytes(PULARGE_INTEGER lpQwSize, LPWSTR pszBuf, UINT cchBuf)
{
    NUMBERFMTW nf;
    WCHAR      szNumber[24];
    WCHAR      szDecimalSep[8];
    WCHAR      szThousandSep[8];
    WCHAR      szGrouping[12];
    int        Len, cchFormatted, i;
    size_t     cchRemaining;
    LPWSTR     Ptr;

    // Try to build first Format byte string
    if (StrFormatByteSizeW(lpQwSize->QuadPart, pszBuf, cchBuf) == NULL)
        return NULL;

    // If there is less bytes than 1KB, we have nothing to do
    if (lpQwSize->QuadPart < 1024)
        return pszBuf;

    // Print the number in uniform mode
    swprintf(szNumber, L"%I64u", lpQwSize->QuadPart);

    // Get system strings for decimal and thousand separators.
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, sizeof(szDecimalSep) / sizeof(*szDecimalSep));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, sizeof(szThousandSep) / sizeof(*szThousandSep));

    // Initialize format for printing the number in bytes
    ZeroMemory(&nf, sizeof(nf));
    nf.NumDigits     = 0;
    nf.LeadingZero   = 0;
    nf.Grouping      = 0;
    nf.lpDecimalSep  = szDecimalSep;
    nf.lpThousandSep = szThousandSep;
    nf.NegativeOrder = 0;

    // Get system string for groups separator
    Len = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                         LOCALE_SGROUPING,
                         szGrouping,
                         sizeof(szGrouping) / sizeof(*szGrouping));

    // Convert grouping specs from string to integer
    for (i = 0; i < Len; i++)
    {
        WCHAR wch = szGrouping[i];

        if (wch >= L'0' && wch <= L'9')
            nf.Grouping = nf.Grouping * 10 + (wch - L'0');
        else if (wch != L';')
            break;
    }

    if ((nf.Grouping % 10) == 0)
        nf.Grouping /= 10;
    else
        nf.Grouping *= 10;

    // Concate " (" at the end of buffer
    Len = wcslen(pszBuf);
    Ptr = pszBuf + Len;
    cchRemaining = cchBuf - Len;
    StringCchCopyExW(Ptr, cchRemaining, L" (", &Ptr, &cchRemaining, 0);

    // Save formatted number of bytes in buffer
    cchFormatted = GetNumberFormatW(LOCALE_USER_DEFAULT,
                                    0,
                                    szNumber,
                                    &nf,
                                    Ptr,
                                    cchRemaining);

    if (cchFormatted == 0)
        return NULL;

    // cchFormatted is number of characters including NULL - make it a real length
    --cchFormatted;

    // Copy ' ' to buffer
    Ptr += cchFormatted;
    cchRemaining -= cchFormatted;
    StringCchCopyExW(Ptr, cchRemaining, L" ", &Ptr, &cchRemaining, 0);

    // Copy 'bytes' string and remaining ')'
    Len = LoadStringW(shell32_hInstance, IDS_BYTES_FORMAT, Ptr, cchRemaining);
    Ptr += Len;
    cchRemaining -= Len;
    StringCchCopy(Ptr, cchRemaining, L")");

    return pszBuf;
}

/*************************************************************************
 *
 * SH_CreatePropertySheetPage [Internal]
 *
 * creates a property sheet page from an resource name
 *
 */

HPROPSHEETPAGE
SH_CreatePropertySheetPage(LPCSTR pszResName, DLGPROC pfnDlgProc, LPARAM lParam, LPWSTR pwszTitle)
{
    HRSRC hRes;

    if (pszResName == NULL)
        return (HPROPSHEETPAGE)0;

    hRes = FindResourceA(shell32_hInstance, pszResName, (LPSTR)RT_DIALOG);

    if (hRes == NULL)
    {
        ERR("failed to find resource name\n");
        return (HPROPSHEETPAGE)0;
    }

    LPVOID pTemplate = LoadResource(shell32_hInstance, hRes);

    if (pTemplate == NULL)
        return (HPROPSHEETPAGE)0;

    PROPSHEETPAGEW Page;
    memset(&Page, 0x0, sizeof(PROPSHEETPAGEW));
    Page.dwSize = sizeof(PROPSHEETPAGEW);
    Page.dwFlags = PSP_DLGINDIRECT;
    Page.pResource = (DLGTEMPLATE *)pTemplate;
    Page.pfnDlgProc = pfnDlgProc;
    Page.lParam = lParam;
    Page.pszTitle = pwszTitle;

    if (pwszTitle)
        Page.dwFlags |= PSP_USETITLE;

    return CreatePropertySheetPageW(&Page);
}

/*************************************************************************
 *
 * SH_FileGeneralFileType [Internal]
 *
 * retrieves file extension description from registry and sets it in dialog
 *
 * TODO: retrieve file extension default icon and load it
 *       find executable name from registry, retrieve description from executable
 */

static BOOL
SH_FileGeneralSetFileType(HWND hwndDlg, WCHAR *filext)
{
    WCHAR name[MAX_PATH];
    WCHAR value[MAX_PATH];
    DWORD lname = MAX_PATH;
    DWORD lvalue = MAX_PATH;
    HKEY hKey;
    LONG result;
    HWND hDlgCtrl;

    TRACE("fileext %s\n", debugstr_w(filext));

    if (filext == NULL)
        return FALSE;

    hDlgCtrl = GetDlgItem(hwndDlg, 14005);

    if (hDlgCtrl == NULL)
        return FALSE;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, filext, &hKey) != ERROR_SUCCESS)
    {
        /* the file extension is unknown, so default to string "FileExtension File" */
        SendMessageW(hDlgCtrl, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)value);
        swprintf(name, L"%s %s", &filext[1], value);
        SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)name);
        return TRUE;
    }

    result = RegEnumValueW(hKey, 0, name, &lname, NULL, NULL, (LPBYTE)value, &lvalue);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyW(HKEY_CLASSES_ROOT, value, &hKey) == ERROR_SUCCESS)
    {
        if (RegLoadMUIStringW(hKey, L"FriendlyTypeName", value, MAX_PATH, NULL, 0, NULL) != ERROR_SUCCESS)
        {
            lvalue = lname = MAX_PATH;
            result = RegEnumValueW(hKey, 0, name, &lname, NULL, NULL, (LPBYTE)value, &lvalue);
        }

        lname = MAX_PATH;

        if (RegGetValueW(hKey, L"DefaultIcon", NULL, RRF_RT_REG_SZ, NULL, name, &lname) == ERROR_SUCCESS)
        {
            UINT IconIndex;
            WCHAR szBuffer[MAX_PATH];
            WCHAR *Offset;
            HICON hIcon = 0;
            HRSRC hResource;
            LPVOID pResource = NULL;
            HGLOBAL hGlobal;
            HINSTANCE hLibrary;
            Offset = wcsrchr(name, L',');

            if (Offset)
            {
                IconIndex = _wtoi(Offset + 2);
                *Offset = L'\0';
                name[MAX_PATH - 1] = L'\0';

                if (ExpandEnvironmentStringsW(name, szBuffer, MAX_PATH))
                {
                    szBuffer[MAX_PATH - 1] = L'\0';
                    hLibrary = LoadLibraryExW(szBuffer, NULL, LOAD_LIBRARY_AS_DATAFILE);
                    if (hLibrary)
                    {
                        hResource = FindResourceW(hLibrary, MAKEINTRESOURCEW(IconIndex), (LPCWSTR)RT_ICON);
                        if (hResource)
                        {
                            hGlobal = LoadResource(shell32_hInstance, hResource);
                            if (hGlobal)
                            {
                                pResource = LockResource(hGlobal);
                                if (pResource != NULL)
                                {
                                    hIcon = CreateIconFromResource((LPBYTE)pResource, SizeofResource(shell32_hInstance, hResource), TRUE, 0x00030000);
                                    TRACE("hIcon %p,- szBuffer %s IconIndex %u error %u icon %p hResource %p pResource %p\n",
                                          hIcon,
                                          debugstr_w(szBuffer),
                                          IconIndex,
                                          MAKEINTRESOURCEW(IconIndex),
                                          hResource,
                                          pResource);
                                    SendDlgItemMessageW(hwndDlg, 14000, STM_SETICON, (WPARAM)hIcon, 0);
                                }
                            }
                        }
                        FreeLibrary(hLibrary);
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    /* file extension type */
    value[MAX_PATH - 1] = L'\0';
    lvalue = wcslen(value);
    lname = wcslen(filext);
    if (MAX_PATH - lvalue - lname - 3 > 0)
    {
        wcscat(value, L" (");
        wcscat(value, filext);
        wcscat(value, L")");
    }

    SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)value);

    return TRUE;
}

/*************************************************************************
 *
 * SHFileGeneralGetFileTimeString [Internal]
 *
 * formats a given LPFILETIME struct into readable user format
 */

static BOOL
SHFileGeneralGetFileTimeString(LPFILETIME lpFileTime, WCHAR *lpResult)
{
    FILETIME ft;
    SYSTEMTIME dt;

    if (lpFileTime == NULL || lpResult == NULL)
        return FALSE;

    if (!FileTimeToLocalFileTime(lpFileTime, &ft))
        return FALSE;

    FileTimeToSystemTime(&ft, &dt);

    /* ddmmyy */
    swprintf(lpResult, L"%02hu/%02hu/%04hu  %02hu:%02hu", dt.wDay, dt.wMonth, dt.wYear, dt.wHour, dt.wMinute);

    TRACE("result %s\n", debugstr_w(lpResult));
    return TRUE;
}

/*************************************************************************
 *
 * SH_FileGeneralSetText [Internal]
 *
 * sets file path string and filename string
 *
 */

static BOOL
SH_FileGeneralSetText(HWND hwndDlg, WCHAR *lpstr)
{
    int flength;
    int plength;
    WCHAR *lpdir;
    WCHAR buff[MAX_PATH];

    if (lpstr == NULL)
        return FALSE;

    lpdir = wcsrchr(lpstr, L'\\');        /* find the last occurence of '\\' */

    plength = wcslen(lpstr);
    flength = wcslen(lpdir);

    if (lpdir)
    {
        /* location text field */
        wcsncpy(buff, lpstr, plength - flength);
        buff[plength - flength] = UNICODE_NULL;

        if (wcslen(buff) == 2)
        {
            wcscat(buff, L"\\");
        }

        SetDlgItemTextW(hwndDlg, 14009, buff);
    }

    if (flength > 1)
    {
        /* text filename field */
        wcsncpy(buff, &lpdir[1], flength);
        SetDlgItemTextW(hwndDlg, 14001, buff);
    }

    return TRUE;
}

/*************************************************************************
 *
 * SH_FileGeneralSetFileSizeTime [Internal]
 *
 * retrieves file information from file and sets in dialog
 *
 */

static BOOL
SH_FileGeneralSetFileSizeTime(HWND hwndDlg, WCHAR *lpfilename, PULARGE_INTEGER lpfilesize)
{
    BOOL result;
    HANDLE hFile;
    FILETIME create_time;
    FILETIME accessed_time;
    FILETIME write_time;
    WCHAR resultstr[MAX_PATH];
    LARGE_INTEGER file_size;

    if (lpfilename == NULL)
        return FALSE;

    hFile = CreateFileW(lpfilename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("failed to open file %s\n", debugstr_w(lpfilename));
        return FALSE;
    }

    result = GetFileTime(hFile, &create_time, &accessed_time, &write_time);

    if (!result)
    {
        WARN("GetFileTime failed\n");
        return FALSE;
    }

    if (SHFileGeneralGetFileTimeString(&create_time, resultstr))
    {
        SetDlgItemTextW(hwndDlg, 14015, resultstr);
    }

    if (SHFileGeneralGetFileTimeString(&accessed_time, resultstr))
    {
        SetDlgItemTextW(hwndDlg, 14019, resultstr);
    }

    if (SHFileGeneralGetFileTimeString(&write_time, resultstr))
    {
        SetDlgItemTextW(hwndDlg, 14017, resultstr);
    }

    if (!GetFileSizeEx(hFile, &file_size))
    {
        WARN("GetFileSize failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    if (!SH_FormatFileSizeWithBytes((PULARGE_INTEGER)&file_size,
                                    resultstr,
                                    sizeof(resultstr) / sizeof(WCHAR)))
        return FALSE;

    TRACE("result size %u resultstr %s\n", file_size.QuadPart, debugstr_w(resultstr));
    SetDlgItemTextW(hwndDlg, 14011, resultstr);

    if (lpfilesize)
        lpfilesize->QuadPart = (ULONGLONG)file_size.QuadPart;

    return TRUE;
}

/*************************************************************************
 *
 * SH_SetFileVersionText [Internal]
 *
 *
 */

static BOOL
SH_FileVersionQuerySetText(HWND hwndDlg, DWORD idCtrl, LPVOID pInfo, LPCWSTR pwszProp)
{
    UINT cbResult;
    WCHAR *pwszResult = NULL;

    if (hwndDlg == NULL || pwszProp == NULL)
        return FALSE;

    if (VerQueryValueW(pInfo, pwszProp, (LPVOID *)&pwszResult, &cbResult))
    {
        /* file description property */
        TRACE("%s :: %s\n", debugstr_w(pwszProp), debugstr_w(pwszResult));
        SetDlgItemTextW(hwndDlg, idCtrl, pwszResult);
        return TRUE;
    }

    return FALSE;
}

/*************************************************************************
 *
 * SH_FileVersionQuerySetListText [Internal]
 *
 * retrieves a version string and adds it to listbox
 *
 */

static BOOL
SH_FileVersionQuerySetListText(HWND hwndDlg, LPVOID pInfo, LPCWSTR pwszProp, WORD wLang, WORD wCode)
{
    HWND hDlgCtrl;
    UINT Index;
    UINT cbResult;
    WCHAR buff[256], *pwszResult = NULL;

    TRACE("pwszProp %s, hwndDlg %p\n", debugstr_w(pwszProp), hwndDlg);

    if (hwndDlg == NULL || pwszProp == NULL)
        return FALSE;

    swprintf(buff, L"\\StringFileInfo\\%04x%04x\\%s", wLang, wCode, pwszProp);

    if (VerQueryValueW(pInfo, buff, (LPVOID *)&pwszResult, &cbResult))
    {
        /* listbox name property */
        hDlgCtrl = GetDlgItem(hwndDlg, 14009);
        TRACE("%s :: %s\n", debugstr_w(pwszProp), debugstr_w(pwszResult));
        Index = SendMessageW(hDlgCtrl, LB_ADDSTRING, (WPARAM) -1, (LPARAM)pwszProp);
        SendMessageW(hDlgCtrl, LB_SETITEMDATA, (WPARAM)Index, (LPARAM)(WCHAR *)pwszResult);
        return TRUE;
    }

    return FALSE;
}

/*************************************************************************
 *
 * SH_FileVersionInitialize [Internal]
 *
 * sets all file version properties in dialog
 */

static BOOL
SH_FileVersionInitialize(HWND hwndDlg, LPCWSTR pwszFilename)
{
    LPVOID pBuf;
    DWORD cbBuf;
    VS_FIXEDFILEINFO *pInfo = NULL;
    UINT cBytes;
    WCHAR buff[256];
    HWND hDlgCtrl;
    WORD wLang = 0, wCode = 0;
    LPLANGANDCODEPAGE lpLangCode;

    if (pwszFilename == 0)
        return FALSE;

    if (!(cbBuf = GetFileVersionInfoSizeW(pwszFilename, NULL)))
    {
        WARN("GetFileVersionInfoSize failed\n");
        return FALSE;
    }

    if (!(pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBuf)))
    {
        WARN("HeapAlloc failed bytes %x\n", cbBuf);
        return FALSE;
    }

    if (!GetFileVersionInfoW(pwszFilename, 0, cbBuf, pBuf))
    {
        HeapFree(GetProcessHeap(), 0, pBuf);
        return FALSE;
    }

    if (VerQueryValueW(pBuf, L"\\", (PVOID*)&pInfo, &cBytes))
    {
        swprintf(buff, L"%u.%u.%u.%u", HIWORD(pInfo->dwFileVersionMS),
                 LOWORD(pInfo->dwFileVersionMS),
                 HIWORD(pInfo->dwFileVersionLS),
                 LOWORD(pInfo->dwFileVersionLS));
        TRACE("MS %x LS %x res %s \n", pInfo->dwFileVersionMS, pInfo->dwFileVersionLS, debugstr_w(buff));
        SetDlgItemTextW(hwndDlg, 14001, buff);
    }

    if (VerQueryValueW(pBuf, L"VarFileInfo\\Translation", (LPVOID *)&lpLangCode, &cBytes))
    {
        /* FIXME find language from current locale / if not available,
         * default to english
         * for now default to first available language
         */
        wLang = lpLangCode->lang;
        wCode = lpLangCode->code;
    }

    swprintf(buff, L"\\StringFileInfo\\%04x%04x\\FileDescription", wLang, wCode);
    SH_FileVersionQuerySetText(hwndDlg, 14003, pBuf, buff);

    swprintf(buff, L"\\StringFileInfo\\%04x%04x\\LegalCopyright", wLang, wCode);
    SH_FileVersionQuerySetText(hwndDlg, 14005, pBuf, buff);

    /* listbox properties */
    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"CompanyName", wLang, wCode);
    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"FileVersion", wLang, wCode);
    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"InternalName", wLang, wCode);

    /* FIXME insert language identifier */

    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"OriginalFilename", wLang, wCode);
    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"ProductName", wLang, wCode);
    SH_FileVersionQuerySetListText(hwndDlg, pBuf, L"ProductVersion", wLang, wCode);
    SetWindowLongPtr(hwndDlg, DWL_USER, (LONG_PTR)pBuf);

    /* select first item */
    hDlgCtrl = GetDlgItem(hwndDlg, 14009);
    SendMessageW(hDlgCtrl, LB_SETCURSEL, 0, 0);
    LPCWSTR pwszText = (WCHAR *)SendMessageW(hDlgCtrl, LB_GETITEMDATA, (WPARAM)0, (LPARAM)NULL);
    SetDlgItemTextW(hwndDlg, 14010, pwszText);

    return TRUE;
}

/*************************************************************************
 *
 * SH_FileVersionDlgProc
 *
 * wnd proc of 'Version' property sheet page
 */

INT_PTR CALLBACK
SH_FileVersionDlgProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

            if (ppsp == NULL)
                break;

            TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %x\n", hwndDlg, lParam, ppsp->lParam);

            LPCWSTR pwszFilename = (LPCWSTR)ppsp->lParam;

            if (pwszFilename == NULL)
                break;

            return SH_FileVersionInitialize(hwndDlg, pwszFilename);
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == 14009 && HIWORD(wParam) == LBN_SELCHANGE)
            {
                HWND hDlgCtrl = (HWND)lParam;

                LRESULT Index = SendMessageW(hDlgCtrl, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
                if (Index == LB_ERR)
                    break;

                LPCWSTR pwszData = (LPCWSTR)SendMessageW(hDlgCtrl, LB_GETITEMDATA, (WPARAM)Index, (LPARAM)NULL);
                if (pwszData == NULL)
                    break;

                TRACE("hDlgCtrl %x string %s\n", hDlgCtrl, debugstr_w(pwszData));
                SetDlgItemTextW(hwndDlg, 14010, pwszData);

                return TRUE;
            }
            break;
        case WM_DESTROY:
        {
            LPVOID pBuf = (LPVOID)GetWindowLongPtr(hwndDlg, DWL_USER);
            HeapFree(GetProcessHeap(), 0, pBuf);
            break;
        }
        default:
            break;
    }

    return FALSE;
}

/*************************************************************************
 *
 * SH_FileGeneralDlgProc
 *
 * wnd proc of 'General' property sheet page
 *
 */

INT_PTR CALLBACK
SH_FileGeneralDlgProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;

            if (ppsp == NULL)
                break;

            TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %S\n", hwndDlg, lParam, ppsp->lParam);

            WCHAR *pwszFilename = (WCHAR *)ppsp->lParam;

            if (pwszFilename == NULL)
            {
                ERR("no filename\n");
                break;
            }

            /* set general text properties filename filelocation and icon */
            SH_FileGeneralSetText(hwndDlg, pwszFilename);

            /* enumerate file extension from registry and application which opens it */
            SH_FileGeneralSetFileType(hwndDlg, wcsrchr(pwszFilename, L'.'));

            /* set opens with */
            SH_FileGeneralOpensWith(hwndDlg, wcsrchr(pwszFilename, L'.'));

            /* set file time create/modfied/accessed */
            SH_FileGeneralSetFileSizeTime(hwndDlg, pwszFilename, NULL);

            return TRUE;
        }
        default:
            break;
    }

    return FALSE;
}

static BOOL CALLBACK
AddShellPropSheetExCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pInfo = (PROPSHEETHEADERW *)lParam;

    if (pInfo->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pInfo->phpage[pInfo->nPages++] = hPage;
        return TRUE;
    }

    return FALSE;
}

static int
EnumPropSheetExt(LPCWSTR pwszPath, PROPSHEETHEADERW *pInfo, int NumPages, HPSXA *phpsxa, IDataObject *pDataObj)
{
    WCHAR wszName[MAX_PATH];
    WCHAR *pwszExt;
    UINT cchPath;
    DWORD dwName;
    int Pages;
    CLSID clsid;

    pwszExt = PathFindExtensionW(pwszPath);

    if (!pwszExt[0])
    {
        cchPath = wcslen(pwszPath);

        if (CLSIDFromString(pwszPath, &clsid) == NOERROR)
            StringCbPrintfW(wszName, sizeof(wszName), L"CLSID\\%s", pwszPath);
        else
            StringCbCopyW(wszName, sizeof(wszName), pwszPath);
    }
    else
        StringCbCopyW(wszName, sizeof(wszName), pwszExt);

    TRACE("EnumPropSheetExt wszName %s\n", debugstr_w(wszName));

    phpsxa[0] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszName, NumPages, pDataObj);
    Pages = SHAddFromPropSheetExtArray(phpsxa[0], AddShellPropSheetExCallback, (LPARAM)pInfo);

    phpsxa[1] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"*", NumPages - Pages, pDataObj);
    Pages += SHAddFromPropSheetExtArray(phpsxa[1], AddShellPropSheetExCallback, (LPARAM)pInfo);

    phpsxa[2] = NULL;

    if (pwszExt)
    {
        /* try to load property sheet handlers from prog id key */
        dwName = sizeof(wszName);

        if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, L"", RRF_RT_REG_SZ, NULL, wszName, &dwName) == ERROR_SUCCESS)
        {
            TRACE("EnumPropSheetExt wszName %s, pwszExt %s\n", debugstr_w(wszName), debugstr_w(pwszExt));
            phpsxa[2] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszName, NumPages - Pages, pDataObj);
            Pages += SHAddFromPropSheetExtArray(phpsxa[2], AddShellPropSheetExCallback, (LPARAM)pInfo);
        }
    }

    return Pages;
}

/*************************************************************************
 *
 * SH_ShowPropertiesDialog
 *
 * called from ShellExecuteExW32
 *
 * pwszPath contains path of folder/file
 *
 * TODO: provide button change application type if file has registered type
 *       make filename field editable and apply changes to filename on close
 */

BOOL
SH_ShowPropertiesDialog(LPCWSTR pwszPath, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST *apidl)
{
    HRESULT hr;
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    HPSXA hpsxa[3] = {};

    TRACE("SH_ShowPropertiesDialog entered filename %s\n", debugstr_w(pwszPath));

    if (pwszPath == NULL || !wcslen(pwszPath))
        return FALSE;

    memset(hppages, 0x0, sizeof(HPROPSHEETPAGE) * MAX_PROPERTY_SHEET_PAGE);

    /* Make a copy of path */
    WCHAR wszPath[MAX_PATH];
    StringCbCopyW(wszPath, sizeof(wszPath), pwszPath);

    //
    // get length
    //
    INT cchPath = wcslen(wszPath);
    if (cchPath > 3 && wszPath[cchPath-1] == L'\\')
    {
        //
        // remove trailing \\ at the end of path
        //
        wszPath[cchPath-1] = L'\0';
    }

    if (PathIsRootW(wszPath))
        return SH_ShowDriveProperties(wszPath, pidlFolder, apidl);

    if (PathIsDirectoryW(wszPath))
        return SH_ShowFolderProperties(wszPath, pidlFolder, apidl);

    LPCWSTR pwszFilename = PathFindFileNameW(wszPath);

    PROPSHEETHEADERW Info;
    memset(&Info, 0x0, sizeof(PROPSHEETHEADERW));
    Info.dwSize = sizeof(PROPSHEETHEADERW);
    Info.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    Info.phpage = hppages;
    Info.pszCaption = pwszFilename;

    hppages[Info.nPages] =
        SH_CreatePropertySheetPage("SHELL_FILE_GENERAL_DLG",
                                   SH_FileGeneralDlgProc,
                                   (LPARAM)wszPath,
                                   NULL);

    if (hppages[Info.nPages])
        Info.nPages++;

    CComPtr<IDataObject> pDataObj;
    hr = SHCreateDataObject(pidlFolder, 1, apidl, NULL, IID_IDataObject, (LPVOID *)&pDataObj);

    if (hr == S_OK)
    {
        if (!EnumPropSheetExt(wszPath, &Info, MAX_PROPERTY_SHEET_PAGE - 1, hpsxa, pDataObj))
        {
            hpsxa[0] = NULL;
            hpsxa[1] = NULL;
            hpsxa[2] = NULL;
        }
    }

    if (GetFileVersionInfoSizeW(wszPath, NULL))
    {
        hppages[Info.nPages] =
            SH_CreatePropertySheetPage("SHELL_FILE_VERSION_DLG",
                                       SH_FileVersionDlgProc,
                                       (LPARAM)wszPath,
                                       NULL);
        if (hppages[Info.nPages])
            Info.nPages++;
    }

    INT_PTR res = PropertySheetW(&Info);

    if (hr == S_OK)
    {
        SHDestroyPropSheetExtArray(hpsxa[0]);
        SHDestroyPropSheetExtArray(hpsxa[1]);
        SHDestroyPropSheetExtArray(hpsxa[2]);
    }

    return (res != -1);
}

/*EOF */
