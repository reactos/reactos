/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shell extension entry point
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"
#include "undocgdi.h" // for GetFontResourceInfoW

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

const GUID CLSID_CFontExt = { 0xbd84b380, 0x8ca2, 0x1069, { 0xab, 0x1d, 0x08, 0x00, 0x09, 0x48, 0xf5, 0x34 } };

class CFontExtModule : public CComModule
{
public:
    void Init(_ATL_OBJMAP_ENTRY *p, HINSTANCE h, const GUID *plibid)
    {
        g_FontCache = new CFontCache();
        CComModule::Init(p, h, plibid);
    }

    void Term()
    {
        delete g_FontCache;
        g_FontCache = NULL;
        CComModule::Term();
    }
};

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CFontExt, CFontExt)
END_OBJECT_MAP()

LONG g_ModuleRefCnt;
CFontExtModule gModule;

STDAPI DllCanUnloadNow()
{
    if (g_ModuleRefCnt)
        return S_FALSE;
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    WCHAR Path[MAX_PATH] = { 0 };
    static const char DesktopIniContents[] = "[.ShellClassInfo]\r\n"
        "CLSID={BD84B380-8CA2-1069-AB1D-08000948F534}\r\n"
        "IconResource=%SystemRoot%\\system32\\shell32.dll,-39\r\n"; // IDI_SHELL_FONTS_FOLDER
    HANDLE hFile;
    HRESULT hr;

    hr = gModule.DllRegisterServer(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, Path);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Make this a system folder:
    // Ideally this should not be done here, but when installing
    // Otherwise, livecd won't have this set properly
    DWORD dwAttributes = GetFileAttributesW(Path);
    if (dwAttributes != INVALID_FILE_ATTRIBUTES)
    {
        dwAttributes |= FILE_ATTRIBUTE_SYSTEM;
        SetFileAttributesW(Path, dwAttributes);
    }
    else
    {
        ERR("Unable to get attributes for fonts folder (%d)\n", GetLastError());
    }

    if (!PathAppendW(Path, L"desktop.ini"))
        return E_FAIL;

    hFile = CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD BytesWritten, BytesToWrite = strlen(DesktopIniContents);
    if (WriteFile(hFile, DesktopIniContents, (DWORD)BytesToWrite, &BytesWritten, NULL))
        hr = (BytesToWrite == BytesWritten) ? S_OK : E_FAIL;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    return hr;
}

STDAPI DllUnregisterServer()
{
    return gModule.DllUnregisterServer(FALSE);
}

void CloseRegKeyArray(HKEY* array, UINT cKeys)
{
    for (UINT i = 0; i < cKeys; ++i)
        RegCloseKey(array[i]);
}

LSTATUS AddClassKeyToArray(const WCHAR* szClass, HKEY* array, UINT* cKeys)
{
    if (*cKeys >= 16)
        return ERROR_MORE_DATA;

    HKEY hkey;
    LSTATUS result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, KEY_READ | KEY_QUERY_VALUE, &hkey);
    if (result == ERROR_SUCCESS)
    {
        array[*cKeys] = hkey;
        *cKeys += 1;
    }
    return result;
}

HRESULT
InstallFontFiles(
    _Inout_ PINSTALL_FONT_DATA pData)
{
    PCUIDLIST_ABSOLUTE pidlParent = pData->pidlParent;
    UINT cidl = pData->cSteps;
    PCUITEMID_CHILD_ARRAY apidl = pData->apidl;

    CAtlArray<CStringW> FontPaths;
    for (UINT n = 0; n < cidl; ++n)
    {
        CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidl;
        pidl.Attach(ILCombine(pidlParent, apidl[n]));
        if (!pidl)
        {
            ERR("Out of memory\n");
            return E_OUTOFMEMORY;
        }

        WCHAR szPath[MAX_PATH];
        if (!SHGetPathFromIDListW(pidl, szPath) || PathIsDirectoryW(szPath) ||
            !IsFontDotExt(PathFindExtensionW(szPath)))
        {
            ERR("Not font file: %s\n", wine_dbgstr_w(szPath));
            return E_FAIL;
        }

        FontPaths.Add(szPath);
    }

    CRegKey keyFonts;
    if (keyFonts.Open(FONT_HIVE, FONT_KEY, KEY_WRITE) != ERROR_SUCCESS)
    {
        ERR("CRegKey::Open failed\n");
        return E_FAIL;
    }

    for (SIZE_T iItem = 0; iItem < FontPaths.GetCount(); ++iItem)
    {
        if (pData->bCanceled)
        {
            WARN("Canceled\n");
            return E_ABORT;
        }

        HRESULT hr = DoInstallFontFile(FontPaths[iItem], g_FontCache->FontPath(), keyFonts);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (pData->hwnd)
            ::PostMessageW(pData->hwnd, WM_COMMAND, IDCONTINUE, 0);
    }

    return S_OK;
}

HRESULT
DoInstallFontFile(
    _In_ PCWSTR pszFontPath,
    _In_ PCWSTR pszFontsDir,
    _In_ HKEY hkeyFonts)
{
    ATLASSERT(pszFontPath);
    ATLASSERT(pszFontsDir);
    ATLASSERT(hkeyFonts);

    // Add this font to the font list, so we can query the name
    if (!AddFontResourceW(pszFontPath))
    {
        ERR("AddFontResourceW('%S') failed\n", pszFontPath);
        return E_FAIL;
    }

    // Get the font name
    CStringW strFontName;
    HRESULT hr = DoGetFontTitle(pszFontPath, strFontName);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Remove it now
    // WINDOWS BUG: Removing once is not enough
    for (INT iTry = 0; iTry < 3; ++iTry)
    {
        if (!RemoveFontResourceW(pszFontPath) &&
            !RemoveFontResourceExW(pszFontPath, FR_PRIVATE, NULL))
        {
            break;
        }
    }

    // Delete font entry in registry
    RegDeleteValueW(hkeyFonts, strFontName);

    LPCWSTR pszFileTitle = PathFindFileName(pszFontPath);
    ATLASSERT(pszFileTitle);

    // Build destination path
    CStringW szDestFile(pszFontsDir); // pszFontsDir has backslash at back
    szDestFile += pszFileTitle;
    TRACE("szDestFile: '%S'\n", (PCWSTR)szDestFile);

    if (!StrCmpIW(szDestFile, pszFontPath)) // Same file?
    {
        ERR("Wrongly same: %S\n", pszFontPath);
        return E_FAIL;
    }

    // Copy file
    if (!CopyFileW(pszFontPath, szDestFile, FALSE))
    {
        ERR("CopyFileW('%S', '%S') failed\n", pszFontPath, (PCWSTR)szDestFile);
        return E_FAIL;
    }

    // Write registry for font entry
    DWORD cbData = (wcslen(pszFileTitle) + 1) * sizeof(WCHAR);
    LONG nError = RegSetValueExW(hkeyFonts, strFontName, 0, REG_SZ,
                                 (const BYTE *)pszFileTitle, cbData);
    if (nError)
    {
        ERR("RegSetValueExW failed with %ld\n", nError);
        DeleteFileW(szDestFile);
        return E_FAIL;
    }

    // Notify file creation
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, (PCWSTR)szDestFile, NULL);

    return AddFontResourceW(szDestFile) ? S_OK : E_FAIL;
}

HRESULT
DoGetFontTitle(
    _In_ LPCWSTR pszFontPath,
    _Out_ CStringW& strFontName)
{
    DWORD cbInfo = 0;
    BOOL ret = GetFontResourceInfoW(pszFontPath, &cbInfo, NULL, 1);
    if (!ret || !cbInfo)
    {
        ERR("GetFontResourceInfoW failed (err: %u)\n", GetLastError());
        return E_FAIL;
    }

    LPWSTR pszBuffer = strFontName.GetBuffer(cbInfo / sizeof(WCHAR));
    ret = GetFontResourceInfoW(pszFontPath, &cbInfo, pszBuffer, 1);
    DWORD dwErr = GetLastError();;
    strFontName.ReleaseBuffer();
    if (!ret)
    {
        ERR("GetFontResourceInfoW failed (err: %u)\n", dwErr);
        return E_FAIL;
    }

    LPCWSTR pchDotExt = PathFindExtensionW(pszFontPath);
    if (!StrCmpIW(pchDotExt, L".ttf") || !StrCmpIW(pchDotExt, L".ttc") ||
        !StrCmpIW(pchDotExt, L".otf") || !StrCmpIW(pchDotExt, L".otc"))
    {
        strFontName += L" (TrueType)";
    }

    TRACE("pszFontName: %S\n", (LPCWSTR)strFontName);
    return S_OK;
}

BOOL CheckDropFontFiles(HDROP hDrop)
{
    UINT cFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
    if (cFiles == 0)
        return FALSE;

    for (UINT iFile = 0; iFile < cFiles; ++iFile)
    {
        WCHAR szFile[MAX_PATH];
        if (!DragQueryFileW(hDrop, iFile, szFile, _countof(szFile)))
            return FALSE;
        LPCWSTR pchDotExt = PathFindExtensionW(szFile);
        if (!IsFontDotExt(pchDotExt))
            return FALSE;
    }

    return TRUE;
}

BOOL CheckDataObject(IDataObject *pDataObj)
{
    STGMEDIUM stg;
    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pDataObj->GetData(&etc, &stg);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;
    HDROP hDrop = reinterpret_cast<HDROP>(stg.hGlobal);
    BOOL bOK = CheckDropFontFiles(hDrop);
    ReleaseStgMedium(&stg);
    return bOK;
}

static DWORD WINAPI InstallThreadProc(LPVOID lpParameter)
{
    PINSTALL_FONT_DATA pData = (PINSTALL_FONT_DATA)lpParameter;
    ATLASSERT(pData);
    pData->hrResult = InstallFontFiles(pData);
    if (pData->bCanceled)
        pData->hrResult = S_FALSE;
    TRACE("hrResult: 0x%08X\n", pData->hrResult);
    ::PostMessageW(pData->hwnd, WM_COMMAND, IDOK, 0);
    pData->pDataObj->Release();
    return 0;
}

static INT_PTR CALLBACK
InstallDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, PINSTALL_FONT_DATA pData)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pData->hwnd = hwnd;
            ATLASSERT(pData->cSteps >= 0);
            SendDlgItemMessageW(hwnd, IDC_INSTALL_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, pData->cSteps));
            if (!SHCreateThread(InstallThreadProc, pData, CTF_COINIT, NULL))
            {
                WARN("!SHCreateThread\n");
                pData->pDataObj->Release();
                pData->hrResult = E_ABORT;
                EndDialog(hwnd, IDABORT);
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    pData->bCanceled = TRUE;
                    EndDialog(hwnd, IDCANCEL);
                    break;
                case IDCONTINUE:
                    pData->iStep += 1;
                    ATLASSERT(pData->iStep <= pData->cSteps);
                    SendDlgItemMessageW(hwnd, IDC_INSTALL_PROGRESS, PBM_SETPOS, pData->iStep, 0);
                    break;
            }
            break;
        }
    }
    return 0;
}

static INT_PTR CALLBACK
InstallDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PINSTALL_FONT_DATA pData = (PINSTALL_FONT_DATA)GetWindowLongPtrW(hwnd, DWLP_USER);
    if (uMsg == WM_INITDIALOG)
    {
        pData = (PINSTALL_FONT_DATA)lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, lParam);
    }

    ATLASSERT(pData);
    return InstallDlgProc(hwnd, uMsg, wParam, lParam, pData);
}

HRESULT InstallFontsFromDataObject(HWND hwndView, IDataObject* pDataObj)
{
    if (!CheckDataObject(pDataObj))
    {
        ERR("!CheckDataObject\n");
        return E_FAIL;
    }

    CDataObjectHIDA cida(pDataObj);
    if (!cida || cida->cidl <= 0)
    {
        ERR("E_UNEXPECTED\n");
        return E_FAIL;
    }

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(cida);
    if (!pidlParent)
    {
        ERR("pidlParent is NULL\n");
        return E_FAIL;
    }

    CAtlArray<PCUIDLIST_RELATIVE> apidl;
    for (UINT n = 0; n < cida->cidl; ++n)
    {
        PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(cida, n);
        if (!pidlRelative)
        {
            ERR("!pidlRelative\n");
            return E_FAIL;
        }
        apidl.Add(pidlRelative);
    }

    // Show progress dialog
    INSTALL_FONT_DATA data;
    data.pDataObj = pDataObj;
    data.pidlParent = pidlParent;
    data.apidl = &apidl[0];
    data.cSteps = cida->cidl;
    pDataObj->AddRef();
    DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD_INSTALL),
                    hwndView, InstallDialogProc, (LPARAM)&data);
    if (data.bCanceled)
        return S_FALSE;

    return FAILED_UNEXPECTEDLY(data.hrResult) ? E_FAIL : S_OK;
}

EXTERN_C
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        gModule.Init(ObjectMap, hInstance, NULL);
        break;
    case DLL_PROCESS_DETACH:
        gModule.Term();
        break;
    }

    return TRUE;
}
