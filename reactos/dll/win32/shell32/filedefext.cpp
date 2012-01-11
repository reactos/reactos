/*
 * provides new shell item service
 *
 * Copyright 2007 Johannes Anderwald (janderwald@reactos.org)
 * Copyright 2009 Andrew Hill
 * Copyright 2012 Rafal Harabien
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

BOOL PathIsExeW(LPCWSTR lpszPath);

BOOL CFileVersionInfo::Load(LPCWSTR pwszPath)
{
    ULONG cbInfo = GetFileVersionInfoSizeW(pwszPath, NULL);
    if (!cbInfo)
    {
        WARN("GetFileVersionInfoSize %ls failed\n", pwszPath);
        return FALSE;
    }

    m_pInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbInfo);
    if (!m_pInfo)
    {
        ERR("HeapAlloc failed bytes %x\n", cbInfo);
        return FALSE;
    }

    if (!GetFileVersionInfoW(pwszPath, 0, cbInfo, m_pInfo))
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

LPCWSTR CFileVersionInfo::GetString(LPCWSTR pwszName)
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
        
VS_FIXEDFILEINFO *CFileVersionInfo::GetFixedInfo()
{
    if (!m_pInfo)
        return NULL;

    VS_FIXEDFILEINFO *pInfo;
    UINT cBytes;
    if (!VerQueryValueW(m_pInfo, L"\\", (PVOID*)&pInfo, &cBytes))
        return NULL;
    return pInfo;
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
    if (pszResName == NULL)
        return NULL;

    HRSRC hRes = FindResourceA(shell32_hInstance, pszResName, (LPSTR)RT_DIALOG);
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

VOID
CFileDefExt::InitOpensWithField(HWND hwndDlg)
{
    WCHAR wszBuf[MAX_PATH] = L"";
    WCHAR wszPath[MAX_PATH] = L"";
    DWORD dwSize = sizeof(wszBuf);
    BOOL bUnknownApp = TRUE;
    LPCWSTR pwszExt = PathFindExtensionW(m_wszPath);

    if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, L"", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) == ERROR_SUCCESS)
    {
        bUnknownApp = FALSE;
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
                bUnknownApp = TRUE;
        } else
            WARN("RegGetValueW %ls failed\n", wszBuf);
    } else
        WARN("RegGetValueW %ls failed\n", pwszExt);

    if (bUnknownApp)
    {
        /* Unknown application */
        LoadStringW(shell32_hInstance, IDS_UNKNOWN_APP, wszBuf, _countof(wszBuf));
        SetDlgItemTextW(hwndDlg, 14007, wszBuf);
    }
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

BOOL
CFileDefExt::InitFileType(HWND hwndDlg)
{
    TRACE("path %s\n", debugstr_w(m_wszPath));

    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14005);
    if (hDlgCtrl == NULL)
        return FALSE;

    /* Get file information */
    SHFILEINFO fi;
    if (!SHGetFileInfoW(m_wszPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME|SHGFI_ICON))
    {
        ERR("SHGetFileInfoW failed for %ls (%lu)\n", m_wszPath, GetLastError());
        fi.szTypeName[0] = L'\0';
        fi.hIcon = NULL;
    }

    LPCWSTR pwszExt = PathFindExtensionW(m_wszPath);
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
        ERR("No icon %ls\n", m_wszPath);

    return TRUE;
}

/*************************************************************************
 *
 * SHFileGeneralGetFileTimeString [Internal]
 *
 * formats a given LPFILETIME struct into readable user format
 */

BOOL
CFileDefExt::GetFileTimeString(LPFILETIME lpFileTime, WCHAR *lpResult)
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

BOOL
CFileDefExt::InitFilePath(HWND hwndDlg)
{
    /* Find the filename */
    WCHAR *pwszFilename = PathFindFileNameW(m_wszPath);

    if (pwszFilename > m_wszPath)
    {
        /* Location field */
        WCHAR wszLocation[MAX_PATH];
        StringCchCopyNW(wszLocation, _countof(wszLocation), m_wszPath, pwszFilename - m_wszPath);
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

BOOL
CFileDefExt::InitFileSizeTime(HWND hwndDlg)
{
    WCHAR wszBuf[MAX_PATH];

    TRACE("SH_FileGeneralSetFileSizeTime %ls\n", m_wszPath);

    HANDLE hFile = CreateFileW(m_wszPath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("failed to open file %s\n", debugstr_w(m_wszPath));
        return FALSE;
    }

    FILETIME CreateTime, AccessedTime, WriteTime;
    if (!GetFileTime(hFile, &CreateTime, &AccessedTime, &WriteTime))
    {
        WARN("GetFileTime failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    LARGE_INTEGER FileSize;
    if (!GetFileSizeEx(hFile, &FileSize))
    {
        WARN("GetFileSize failed\n");
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    if (GetFileTimeString(&CreateTime, wszBuf))
        SetDlgItemTextW(hwndDlg, 14015, wszBuf);

    if (GetFileTimeString(&AccessedTime, wszBuf))
        SetDlgItemTextW(hwndDlg, 14019, wszBuf);

    if (GetFileTimeString(&WriteTime, wszBuf))
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
 * CFileDefExt::InitGeneralPage [Internal]
 *
 * sets all file general properties in dialog
 */

BOOL
CFileDefExt::InitGeneralPage(HWND hwndDlg)
{
    /* Set general text properties filename filelocation and icon */
    InitFilePath(hwndDlg);

    /* Set file type and icon */
    InitFileType(hwndDlg);

    /* Set open with application */
    if (!PathIsExeW(m_wszPath))
        InitOpensWithField(hwndDlg);
    else
    {
        LPCWSTR pwszDescr = m_VerInfo.GetString(L"FileDescription");
        if (pwszDescr)
            SetDlgItemTextW(hwndDlg, 14007, pwszDescr);
    }

    /* Set file created/modfied/accessed time */
    InitFileSizeTime(hwndDlg);

    return TRUE;
}

/*************************************************************************
 *
 * CFileDefExt::GeneralPageProc
 *
 * wnd proc of 'General' property sheet page
 *
 */

INT_PTR CALLBACK
CFileDefExt::GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;

            if (ppsp == NULL || !ppsp->lParam)
                break;

            TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %S\n", hwndDlg, lParam, ppsp->lParam);

            CFileDefExt *pFileDefExt = (CFileDefExt*)ppsp->lParam;
            pFileDefExt->InitGeneralPage(hwndDlg);
        }
        default:
            break;
    }

    return FALSE;
}

/*************************************************************************
 *
 * CFileDefExt::InitVersionPage [Internal]
 *
 * sets all file version properties in dialog
 */

BOOL
CFileDefExt::InitVersionPage(HWND hwndDlg)
{
    /* Get fixed info */
    VS_FIXEDFILEINFO *pInfo = m_VerInfo.GetFixedInfo();
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
    SetVersionLabel(hwndDlg, 14003, L"FileDescription");
    SetVersionLabel(hwndDlg, 14005, L"LegalCopyright");

    /* Add items to listbox */
    AddVersionString(hwndDlg, L"CompanyName");
    /* FIXME insert language identifier */
    AddVersionString(hwndDlg, L"ProductName");
    AddVersionString(hwndDlg, L"InternalName");
    AddVersionString(hwndDlg, L"OriginalFilename");
    AddVersionString(hwndDlg, L"FileVersion");
    AddVersionString(hwndDlg, L"ProductVersion");

    /* Attach file version to dialog window */
    SetWindowLongPtr(hwndDlg, DWL_USER, (LONG_PTR)this);

    /* Select first item */
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14009);
    SendMessageW(hDlgCtrl, LB_SETCURSEL, 0, 0);
    LPCWSTR pwszText = (WCHAR *)SendMessageW(hDlgCtrl, LB_GETITEMDATA, (WPARAM)0, (LPARAM)NULL);
    SetDlgItemTextW(hwndDlg, 14010, pwszText);

    return TRUE;
}

/*************************************************************************
 *
 * CFileDefExt::SetVersionLabel [Internal]
 *
 *
 */

BOOL
CFileDefExt::SetVersionLabel(HWND hwndDlg, DWORD idCtrl, LPCWSTR pwszName)
{
    if (hwndDlg == NULL || pwszName == NULL)
        return FALSE;

    LPCWSTR pwszValue = m_VerInfo.GetString(pwszName);
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
 * CFileDefExt::AddVersionString [Internal]
 *
 * retrieves a version string and adds it to listbox
 *
 */

BOOL
CFileDefExt::AddVersionString(HWND hwndDlg, LPCWSTR pwszName)
{
    TRACE("pwszName %s, hwndDlg %p\n", debugstr_w(pwszName), hwndDlg);

    if (hwndDlg == NULL || pwszName == NULL)
        return FALSE;

    LPCWSTR pwszValue = m_VerInfo.GetString(pwszName);
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
 * CFileDefExt::VersionPageProc
 *
 * wnd proc of 'Version' property sheet page
 */

INT_PTR CALLBACK
CFileDefExt::VersionPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

            if (ppsp == NULL || !ppsp->lParam)
                break;

            TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %x\n", hwndDlg, lParam, ppsp->lParam);

            CFileDefExt *pFileDefExt = (CFileDefExt*)ppsp->lParam;
            return pFileDefExt->InitVersionPage(hwndDlg);
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
            break;
        default:
            break;
    }

    return FALSE;
}

CFileDefExt::CFileDefExt()
{
    m_wszPath[0] = L'\0';
}

CFileDefExt::~CFileDefExt()
{
    
}

HRESULT WINAPI
CFileDefExt::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pDataObj, HKEY hkeyProgID)
{
    FORMATETC format;
    STGMEDIUM stgm;
    HRESULT hr;

    TRACE("%p %p %p %p\n", this, pidlFolder, pDataObj, hkeyProgID);

    if (!pDataObj)
        return E_FAIL;

    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    hr = pDataObj->GetData(&format, &stgm);
    if (FAILED(hr))
        return hr;

    if (!DragQueryFileW((HDROP)stgm.hGlobal, 0, m_wszPath, _countof(m_wszPath)))
    {
        ERR("DragQueryFileW failed\n");
        ReleaseStgMedium(&stgm);
        return E_FAIL;
    }

    ReleaseStgMedium(&stgm);
    TRACE("File properties %ls\n", m_wszPath);
    m_VerInfo.Load(m_wszPath);

    return S_OK;
}

HRESULT WINAPI
CFileDefExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CFileDefExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CFileDefExt::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CFileDefExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage;

    hPage = SH_CreatePropertySheetPage("SHELL_FILE_GENERAL_DLG",
                                   GeneralPageProc,
                                   (LPARAM)this,
                                   NULL);
    if (hPage)
        pfnAddPage(hPage, lParam);

    if (GetFileVersionInfoSizeW(m_wszPath, NULL))
    {
        hPage = SH_CreatePropertySheetPage("SHELL_FILE_VERSION_DLG",
                                            VersionPageProc,
                                            (LPARAM)this,
                                            NULL);
        if (hPage)
            pfnAddPage(hPage, lParam);
    }

    return S_OK;
}

HRESULT WINAPI
CFileDefExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CFileDefExt::SetSite(IUnknown *punk)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CFileDefExt::GetSite(REFIID iid, void **ppvSite)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
