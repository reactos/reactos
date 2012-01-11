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
BOOL PathIsExeW(LPCWSTR lpszPath);

class CFileVersionInfo
{
    public:
        inline CFileVersionInfo():
            m_pInfo(NULL), m_wLang(0), m_wCode(0) {}

        inline ~CFileVersionInfo()
        {
            if (m_pInfo)
                HeapFree(GetProcessHeap(), 0, m_pInfo);
        }

        BOOL Load(LPCWSTR pwszPath)
        {
            ULONG cbBuf = GetFileVersionInfoSizeW(pwszPath, NULL);
            if (!cbBuf)
            {
                WARN("GetFileVersionInfoSize %ls failed\n", pwszPath);
                return FALSE;
            }

            m_pInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBuf);
            if (!m_pInfo)
            {
                ERR("HeapAlloc failed bytes %x\n", cbBuf);
                return FALSE;
            }

            if (!GetFileVersionInfoW(pwszPath, 0, cbBuf, m_pInfo))
            {
                ERR("GetFileVersionInfoW failed\n");
                return FALSE;
            }

            LPLANGANDCODEPAGE lpLangCode;
            UINT cBytes;
            if (!VerQueryValueW(m_pInfo, L"VarFileInfo\\Translation", (LPVOID *)&lpLangCode, &cBytes))
            {
                ERR("VerQueryValueW failed\n");
                return FALSE;
            }

            /* FIXME: find language from current locale / if not available,
             * default to english
             * for now default to first available language
             */
            m_wLang = lpLangCode->lang;
            m_wCode = lpLangCode->code;
            return TRUE;
        }

        LPCWSTR GetString(LPCWSTR pwszName)
        {
            if (!m_pInfo)
                return NULL;

            WCHAR wszBuf[256];
            swprintf(wszBuf, L"\\StringFileInfo\\%04x%04x\\%s", m_wLang, m_wCode, pwszName);

            LPCWSTR pwszResult = NULL;
            UINT cBytes = 0;
            if (!VerQueryValueW(m_pInfo, wszBuf, (LPVOID *)&pwszResult, &cBytes))
                return NULL;

            return pwszResult;
        }
        
        VS_FIXEDFILEINFO *GetFixedInfo()
        {
            if (!m_pInfo)
                return NULL;

            VS_FIXEDFILEINFO *pInfo;
            UINT cBytes;
            if (!VerQueryValueW(m_pInfo, L"\\", (PVOID*)&pInfo, &cBytes))
                return NULL;
            return pInfo;
        }
        

    private:
        PVOID m_pInfo;
        WORD m_wLang, m_wCode;
};

static VOID
SH_FileGeneralOpensWith(HWND hwndDlg, LPCWSTR pwszExt)
{
    WCHAR wszBuf[MAX_PATH] = L"";
    WCHAR wszPath[MAX_PATH] = L"";
    DWORD dwSize = sizeof(wszBuf);

    if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, L"", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) == ERROR_SUCCESS)
    {
        StringCbCatW(wszBuf, sizeof(wszBuf), L"\\shell\\open\\command");
        dwSize = sizeof(wszPath);
        if (RegGetValueW(HKEY_CLASSES_ROOT, wszBuf, L"", RRF_RT_REG_SZ, NULL, wszPath, &dwSize) == ERROR_SUCCESS)
        {
            /* Get path from command line */
            ExpandEnvironmentStringsW(wszPath, wszBuf, _countof(wszBuf));
            PathRemoveArgs(wszBuf);
            PathUnquoteSpacesW(wszBuf);
            PathSearchAndQualify(wszBuf, wszPath, _countof(wszPath));

            if (PathFileExistsW(wszPath))
            {
                /* Get file description */
                CFileVersionInfo VerInfo;
                VerInfo.Load(wszPath);
                LPCWSTR pwszDescr = VerInfo.GetString(L"FileDescription");
                if (pwszDescr)
                    SetDlgItemTextW(hwndDlg, 14007, pwszDescr);
                else
                {
                    /* File has no description - display filename */
                    LPWSTR pwszFilename = PathFindFileNameW(wszPath);
                    PathRemoveExtension(pwszFilename);
                    pwszFilename[0] = towupper(pwszFilename[0]);
                    SetDlgItemTextW(hwndDlg, 14007, pwszFilename);
                }
            }
            else
            {
                /* Unknown application */
                LoadStringW(shell32_hInstance, IDS_UNKNOWN_APP, wszBuf, _countof(wszBuf));
                SetDlgItemTextW(hwndDlg, 14007, wszBuf);
            }
        } else
            WARN("RegGetValueW %ls failed\n", wszBuf);
    } else
        WARN("RegGetValueW %ls failed\n", pwszExt);

    
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
SH_CreatePropertySheetPage(LPCSTR pwszResName, DLGPROC pfnDlgProc, LPARAM lParam, LPWSTR pwszTitle)
{
    if (pwszResName == NULL)
        return NULL;

    HRSRC hRes = FindResourceA(shell32_hInstance, pwszResName, (LPSTR)RT_DIALOG);
    if (hRes == NULL)
    {
        ERR("failed to find resource name\n");
        return NULL;
    }

    LPVOID pTemplate = LoadResource(shell32_hInstance, hRes);
    if (pTemplate == NULL)
    {
        ERR("failed to load resource\n");
        return NULL;
    }

    PROPSHEETPAGEW Page;
    memset(&Page, 0x0, sizeof(PROPSHEETPAGEW));
    Page.dwSize = sizeof(PROPSHEETPAGEW);
    Page.dwFlags = PSP_DLGINDIRECT;
    Page.pResource = (DLGTEMPLATE*)pTemplate;
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
SH_FileGeneralSetFileType(HWND hwndDlg, LPCWSTR pwszPath)
{
    TRACE("path %s\n", debugstr_w(pwszPath));

    if (pwszPath == NULL || !pwszPath[0])
        return FALSE;

    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14005);
    if (hDlgCtrl == NULL)
        return FALSE;

    /* Get file information */
    SHFILEINFO fi;
    if (!SHGetFileInfoW(pwszPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME|SHGFI_ICON))
    {
        ERR("SHGetFileInfoW failed for %ls (%lu)\n", pwszPath, GetLastError());
        fi.szTypeName[0] = L'\0';
        fi.hIcon = NULL;
    }

    LPCWSTR pwszExt = PathFindExtensionW(pwszPath);
    if (pwszExt[0])
    {
        WCHAR wszBuf[256];

        if (!fi.szTypeName[0])
        {
            /* The file type is unknown, so default to string "FileExtension File" */
            size_t cchRemaining = 0;
            LPWSTR pwszEnd = NULL;

            StringCchPrintfExW(wszBuf, _countof(wszBuf), &pwszEnd, &cchRemaining, 0, L"%s ", pwszExt + 1);
            SendMessageW(hDlgCtrl, WM_GETTEXT, (WPARAM)cchRemaining, (LPARAM)pwszEnd);

            SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)wszBuf);
        }
        else
        {
            /* Update file type */
            StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s (%s)", fi.szTypeName, pwszExt);
            SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)wszBuf);
        }
    }

    /* Update file icon */
    if (fi.hIcon)
        SendDlgItemMessageW(hwndDlg, 14000, STM_SETICON, (WPARAM)fi.hIcon, 0);
    else
        ERR("No icon %ls\n", pwszPath);

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
    SYSTEMTIME st;

    if (lpFileTime == NULL || lpResult == NULL)
        return FALSE;

    if (!FileTimeToLocalFileTime(lpFileTime, &ft))
        return FALSE;

    FileTimeToSystemTime(&ft, &st);

    /* ddmmyy */
    swprintf(lpResult, L"%02hu/%02hu/%04hu  %02hu:%02hu", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);

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
SH_FileGeneralSetText(HWND hwndDlg, LPCWSTR pwszPath)
{
    if (pwszPath == NULL)
        return FALSE;

    /* Find the filename */
    WCHAR *pwszFilename = PathFindFileNameW(pwszPath);

    if (pwszFilename > pwszPath)
    {
        /* Location field */
        WCHAR wszLocation[MAX_PATH];
        StringCchCopyNW(wszLocation, _countof(wszLocation), pwszPath, pwszFilename - pwszPath);
        PathRemoveBackslashW(wszLocation);

        SetDlgItemTextW(hwndDlg, 14009, wszLocation);
    }

    /* Filename field */
    SetDlgItemTextW(hwndDlg, 14001, pwszFilename);

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
SH_FileGeneralSetFileSizeTime(HWND hwndDlg, LPCWSTR pwszPath)
{
    HANDLE hFile;
    FILETIME CreateTime;
    FILETIME AccessedTime;
    FILETIME WriteTime;
    WCHAR wszBuf[MAX_PATH];
    LARGE_INTEGER FileSize;

    if (pwszPath == NULL)
        return FALSE;

    TRACE("SH_FileGeneralSetFileSizeTime %ls\n", pwszPath);

    hFile = CreateFileW(pwszPath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("failed to open file %s\n", debugstr_w(pwszPath));
        return FALSE;
    }

    if (!GetFileTime(hFile, &CreateTime, &AccessedTime, &WriteTime))
    {
        WARN("GetFileTime failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    if (!GetFileSizeEx(hFile, &FileSize))
    {
        WARN("GetFileSize failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    if (SHFileGeneralGetFileTimeString(&CreateTime, wszBuf))
        SetDlgItemTextW(hwndDlg, 14015, wszBuf);

    if (SHFileGeneralGetFileTimeString(&AccessedTime, wszBuf))
        SetDlgItemTextW(hwndDlg, 14019, wszBuf);

    if (SHFileGeneralGetFileTimeString(&WriteTime, wszBuf))
        SetDlgItemTextW(hwndDlg, 14017, wszBuf);

    if (SH_FormatFileSizeWithBytes((PULARGE_INTEGER)&FileSize,
                                    wszBuf,
                                    sizeof(wszBuf) / sizeof(WCHAR)))
    {
        SetDlgItemTextW(hwndDlg, 14011, wszBuf);
    }

    return TRUE;
}

/*************************************************************************
 *
 * SH_SetFileVersionText [Internal]
 *
 *
 */

static BOOL
SH_FileVersionQuerySetText(HWND hwndDlg, DWORD idCtrl, CFileVersionInfo *pVerInfo, LPCWSTR pwszName)
{
    if (hwndDlg == NULL || pwszName == NULL)
        return FALSE;

    LPCWSTR pwszValue = pVerInfo->GetString(pwszName);
    if (pwszValue)
    {
        /* file description property */
        TRACE("%s :: %s\n", debugstr_w(pwszName), debugstr_w(pwszValue));
        SetDlgItemTextW(hwndDlg, idCtrl, pwszValue);
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
SH_FileVersionQuerySetListText(HWND hwndDlg, CFileVersionInfo *pVerInfo, LPCWSTR pwszName)
{
    TRACE("pwszName %s, hwndDlg %p\n", debugstr_w(pwszName), hwndDlg);

    if (hwndDlg == NULL || pwszName == NULL)
        return FALSE;

    LPCWSTR pwszValue = pVerInfo->GetString(pwszName);
    if (pwszValue)
    {
        /* listbox name property */
        HWND hDlgCtrl = GetDlgItem(hwndDlg, 14009);
        TRACE("%s :: %s\n", debugstr_w(pwszName), debugstr_w(pwszValue));
        UINT Index = SendMessageW(hDlgCtrl, LB_ADDSTRING, (WPARAM) -1, (LPARAM)pwszName);
        SendMessageW(hDlgCtrl, LB_SETITEMDATA, (WPARAM)Index, (LPARAM)(WCHAR *)pwszValue);
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
SH_FileVersionInitialize(HWND hwndDlg, LPCWSTR pwszPath)
{
    if (pwszPath == NULL)
        return FALSE;

    /* Get file version info */
    CFileVersionInfo *pVerInfo;
    pVerInfo = new CFileVersionInfo;
    if (!pVerInfo || !pVerInfo->Load(pwszPath))
        return FALSE;

    /* Get fixed info */
    VS_FIXEDFILEINFO *pInfo = pVerInfo->GetFixedInfo();
    if (pInfo)
    {
        WCHAR wszVersion[256];
        swprintf(wszVersion, L"%u.%u.%u.%u", HIWORD(pInfo->dwFileVersionMS),
                 LOWORD(pInfo->dwFileVersionMS),
                 HIWORD(pInfo->dwFileVersionLS),
                 LOWORD(pInfo->dwFileVersionLS));
        TRACE("MS %x LS %x ver %s \n", pInfo->dwFileVersionMS, pInfo->dwFileVersionLS, debugstr_w(wszVersion));
        SetDlgItemTextW(hwndDlg, 14001, wszVersion);
    }

    /* Update labels */
    SH_FileVersionQuerySetText(hwndDlg, 14003, pVerInfo, L"FileDescription");
    SH_FileVersionQuerySetText(hwndDlg, 14005, pVerInfo, L"LegalCopyright");

    /* Add items to listbox */
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"CompanyName");
    /* FIXME insert language identifier */
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"ProductName");
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"InternalName");
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"OriginalFilename");
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"FileVersion");
    SH_FileVersionQuerySetListText(hwndDlg, pVerInfo, L"ProductVersion");

    /* Attach file version to dialog window */
    SetWindowLongPtr(hwndDlg, DWL_USER, (LONG_PTR)pVerInfo);

    /* Select first item */
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14009);
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
            CFileVersionInfo *pVerInfo = (CFileVersionInfo*)GetWindowLongPtr(hwndDlg, DWL_USER);
            if (pVerInfo)
                delete pVerInfo;
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

            LPCWSTR pwszPath = (WCHAR *)ppsp->lParam;
            if (pwszPath == NULL)
            {
                ERR("no path\n");
                break;
            }

            /* Set general text properties filename filelocation and icon */
            SH_FileGeneralSetText(hwndDlg, pwszPath);

            /* Set file type and icon */
            SH_FileGeneralSetFileType(hwndDlg, pwszPath);

            /* Set open with application */
            if (!PathIsExeW(pwszPath))
                SH_FileGeneralOpensWith(hwndDlg, PathFindExtensionW(pwszPath));

            /* Set file created/modfied/accessed time */
            SH_FileGeneralSetFileSizeTime(hwndDlg, pwszPath);

            return TRUE;
        }
        default:
            break;
    }

    return FALSE;
}

static BOOL CALLBACK
AddPropSheetPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pHeader = (PROPSHEETHEADERW *)lParam;

    if (pHeader->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pHeader->phpage[pHeader->nPages++] = hPage;
        return TRUE;
    }

    return FALSE;
}

static UINT
LoadPropSheetHandlers(LPCWSTR pwszPath, PROPSHEETHEADERW *pHeader, UINT cMaxPages, HPSXA *phpsxa, IDataObject *pDataObj)
{
    WCHAR wszBuf[MAX_PATH];
    UINT cPages = 0, i = 0;

    LPWSTR pwszFilename = PathFindFileNameW(pwszPath);

    if (pwszFilename[0] == L'{')
    {
        /* Load class property sheet handlers */
        StringCbPrintfW(wszBuf, sizeof(wszBuf), L"CLSID\\%s", pwszFilename);
        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszBuf, cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);
    }
    else
    {
        /* Load property sheet handlers from ext key */
        LPWSTR pwszExt = PathFindExtensionW(pwszFilename);
        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, pwszExt, cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);

        /* Load property sheet handlers from prog id key */
        DWORD cbBuf = sizeof(wszBuf);
        if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, L"", RRF_RT_REG_SZ, NULL, wszBuf, &cbBuf) == ERROR_SUCCESS)
        {
            TRACE("EnumPropSheetExt wszBuf %s, pwszExt %s\n", debugstr_w(wszBuf), debugstr_w(pwszExt));
            phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszBuf, cMaxPages - cPages, pDataObj);
            cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);
        }
    }

    /* Add property sheet handlers from "*" key */
    phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"*", cMaxPages - cPages, pDataObj);
    cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);

    return cPages;
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
    HPSXA hpsxa[3] = {NULL, NULL, NULL};

    TRACE("SH_ShowPropertiesDialog entered filename %s\n", debugstr_w(pwszPath));

    if (pwszPath == NULL || !wcslen(pwszPath))
        return FALSE;

    memset(hppages, 0x0, sizeof(HPROPSHEETPAGE) * MAX_PROPERTY_SHEET_PAGE);

    /* Make a copy of path */
    WCHAR wszPath[MAX_PATH];
    StringCbCopyW(wszPath, sizeof(wszPath), pwszPath);

    /* remove trailing \\ at the end of path */
    PathRemoveBackslashW(wszPath);

    /* Handle drives */
    if (PathIsRootW(wszPath))
        return SH_ShowDriveProperties(wszPath, pidlFolder, apidl);

    /* Handle folders */
    if (PathIsDirectoryW(wszPath))
        return SH_ShowFolderProperties(wszPath, pidlFolder, apidl);

    /* Handle files */
    PROPSHEETHEADERW Header;
    memset(&Header, 0x0, sizeof(PROPSHEETHEADERW));
    Header.dwSize = sizeof(PROPSHEETHEADERW);
    Header.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    Header.phpage = hppages;
    Header.pszCaption = PathFindFileNameW(wszPath);

    hppages[Header.nPages] =
        SH_CreatePropertySheetPage("SHELL_FILE_GENERAL_DLG",
                                   SH_FileGeneralDlgProc,
                                   (LPARAM)wszPath,
                                   NULL);

    if (hppages[Header.nPages])
        Header.nPages++;

    if (GetFileVersionInfoSizeW(wszPath, NULL) && Header.nPages < _countof(hppages))
    {
        hppages[Header.nPages] =
            SH_CreatePropertySheetPage("SHELL_FILE_VERSION_DLG",
                                       SH_FileVersionDlgProc,
                                       (LPARAM)wszPath,
                                       NULL);
        if (hppages[Header.nPages])
            Header.nPages++;
    }

    CComPtr<IDataObject> pDataObj;
    hr = SHCreateDataObject(pidlFolder, 1, apidl, NULL, IID_IDataObject, (LPVOID *)&pDataObj);

    if (SUCCEEDED(hr))
        LoadPropSheetHandlers(wszPath, &Header, MAX_PROPERTY_SHEET_PAGE - 1, hpsxa, pDataObj);

    INT_PTR Result = PropertySheetW(&Header);

    for (UINT i = 0; i < 3; ++i)
        if (hpsxa[i])
            SHDestroyPropSheetExtArray(hpsxa[i]);

    return (Result != -1);
}

/*EOF */
