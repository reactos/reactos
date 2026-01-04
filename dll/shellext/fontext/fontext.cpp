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

HRESULT
InstallFontFiles(
    _Out_ CStringW& strMsg,
    _In_ PCUIDLIST_ABSOLUTE pidlParent,
    _In_ UINT cidl,
    _In_ PCUITEMID_CHILD_ARRAY apidl)
{
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
        HRESULT hr = DoInstallFontFile(strMsg, FontPaths[iItem], g_FontCache->FontPath(), keyFonts);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

HRESULT
DoInstallFontFile(
    _Out_ CStringW& strMsg,
    _In_ PCWSTR pszFontPath,
    _In_ PCWSTR pszFontsDir,
    _In_ HKEY hkeyFonts)
{
    WCHAR szDestFile[MAX_PATH];

    // Add this font to the font list, so we can query the name
    if (!AddFontResourceW(pszFontPath))
    {
        ERR("AddFontResourceW('%S') failed\n", pszFontPath);
        return E_FAIL;
    }

    CStringW strFontName;
    HRESULT hr = DoGetFontTitle(pszFontPath, strFontName);

    // We got the name, remove it again
    RemoveFontResourceW(pszFontPath);

    if (!SUCCEEDED(hr))
    {
        ERR("DoGetFontTitle failed (err=0x%x)!\n", hr);
        return hr;
    }

    StringCchCopyW(szDestFile, sizeof(szDestFile), pszFontsDir);

    LPCWSTR pszFileTitle = PathFindFileName(pszFontPath);
    PathAppendW(szDestFile, pszFileTitle);
    if (!CopyFileW(pszFontPath, szDestFile, FALSE))
    {
        ERR("CopyFileW('%S', '%S') failed\n", pszFontPath, szDestFile);
        return E_FAIL;
    }

    DWORD cbData = (wcslen(pszFileTitle) + 1) * sizeof(WCHAR);
    LONG nError = RegSetValueExW(hkeyFonts, strFontName, 0, REG_SZ,
                                 (const BYTE *)pszFileTitle, cbData);
    if (nError)
    {
        ERR("RegSetValueExW failed with %ld\n", nError);
        DeleteFileW(szDestFile);
        return E_FAIL;
    }

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
    if (ret)
    {
        TRACE("pszFontName: %S\n", (LPCWSTR)strFontName);
        return S_OK;
    }

    ERR("GetFontResourceInfoW failed (err: %u)\n", dwErr);
    return E_FAIL;
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
