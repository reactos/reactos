/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontExt implementation
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);


struct FolderViewColumns
{
    int iResource;
    DWORD dwDefaultState;
    int cxChar;
    int fmt;
};

static FolderViewColumns g_ColumnDefs[] =
{
    { IDS_COL_NAME,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   25, LVCFMT_LEFT },
};



// Should fix our headers..
EXTERN_C HRESULT WINAPI SHCreateFileExtractIconW(LPCWSTR pszPath, DWORD dwFileAttributes, REFIID riid, void **ppv);


// Helper functions to translate a guid to a readable name
bool GetInterfaceName(const WCHAR* InterfaceString, WCHAR* buf, size_t size)
{
    WCHAR LocalBuf[100];
    DWORD dwType = 0, dwDataSize = size * sizeof(WCHAR);

    if (!SUCCEEDED(StringCchPrintfW(LocalBuf, _countof(LocalBuf), L"Interface\\%s", InterfaceString)))
        return false;

    return RegGetValueW(HKEY_CLASSES_ROOT, LocalBuf, NULL, RRF_RT_REG_SZ, &dwType, buf, &dwDataSize) == ERROR_SUCCESS;
}

WCHAR* g2s(REFCLSID iid)
{
    static WCHAR buf[2][300];
    static int idx = 0;

    idx ^= 1;

    LPOLESTR tmp;
    HRESULT hr = ProgIDFromCLSID(iid, &tmp);
    if (SUCCEEDED(hr))
    {
        wcscpy(buf[idx], tmp);
        CoTaskMemFree(tmp);
        return buf[idx];
    }
    StringFromGUID2(iid, buf[idx], _countof(buf[idx]));
    if (GetInterfaceName(buf[idx], buf[idx], _countof(buf[idx])))
    {
        return buf[idx];
    }
    StringFromGUID2(iid, buf[idx], _countof(buf[idx]));

    return buf[idx];
}


CFontExt::CFontExt()
{
    InterlockedIncrement(&g_ModuleRefCnt);
}

CFontExt::~CFontExt()
{
    InterlockedDecrement(&g_ModuleRefCnt);
}

// *** IShellFolder2 methods ***
STDMETHODIMP CFontExt::GetDefaultSearchGUID(GUID *lpguid)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::EnumSearches(IEnumExtraSearch **ppenum)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    if (!pcsFlags || iColumn >= _countof(g_ColumnDefs))
        return E_INVALIDARG;

    *pcsFlags = g_ColumnDefs[iColumn].dwDefaultState;
    return S_OK;
}

STDMETHODIMP CFontExt::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (iColumn >= _countof(g_ColumnDefs))
        return E_FAIL;

    psd->cxChar = g_ColumnDefs[iColumn].cxChar;
    psd->fmt = g_ColumnDefs[iColumn].fmt;

    // No item requested, so return the column name
    if (pidl == NULL)
    {
        return SHSetStrRet(&psd->str, _AtlBaseModule.GetResourceInstance(), g_ColumnDefs[iColumn].iResource);
    }

    // Validate that this pidl is the last one
    PCUIDLIST_RELATIVE curpidl = ILGetNext(pidl);
    if (curpidl->mkid.cb != 0)
    {
        ERR("ERROR, unhandled PIDL!\n");
        return E_FAIL;
    }

    switch (iColumn)
    {
    case 0: /* Name, ReactOS specific? */
        return GetDisplayNameOf(pidl, 0, &psd->str);
    default:
        break;
    }

    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    //ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

// *** IShellFolder2 methods ***
STDMETHODIMP CFontExt::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return _CEnumFonts_CreateInstance(this, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

STDMETHODIMP CFontExt::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    ERR("%s(riid=%S) UNIMPLEMENTED\n", __FUNCTION__, g2s(riid));
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    const FontPidlEntry* fontEntry1 = _FontFromIL(pidl1);
    const FontPidlEntry* fontEntry2 = _FontFromIL(pidl2);

    if (!fontEntry1 || !fontEntry2)
        return E_INVALIDARG;

    int result = (int)fontEntry1->Index - (int)fontEntry2->Index;

    return MAKE_COMPARE_HRESULT(result);
}

STDMETHODIMP CFontExt::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        *ppvOut = static_cast<IDropTarget *>(this);
        AddRef();
        hr = S_OK;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        ERR("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        // Just create a default shell folder view, and register ourself as folder
        SFV_CREATE sfv = { sizeof(SFV_CREATE) };
        sfv.pshf = this;
        hr = SHCreateShellFolderView(&sfv, (IShellView**)ppvOut);
    }

    return hr;
}

STDMETHODIMP CFontExt::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    if (!rgfInOut || !cidl || !apidl)
        return E_INVALIDARG;

    DWORD rgf = 0;
    while (cidl > 0 && *apidl)
    {
        const FontPidlEntry* fontEntry = _FontFromIL(*apidl);
        if (fontEntry)
        {
            // We don't support delete yet
            rgf |= (/*SFGAO_CANDELETE |*/ SFGAO_HASPROPSHEET | SFGAO_CANCOPY | SFGAO_FILESYSTEM);
        }
        else
        {
            rgf = 0;
            break;
        }

        apidl++;
        cidl--;
    }

    *rgfInOut = rgf;
    return S_OK;
}


STDMETHODIMP CFontExt::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    if (riid == IID_IContextMenu ||
        riid == IID_IContextMenu2 ||
        riid == IID_IContextMenu3)
    {
        return _CFontMenu_CreateInstance(hwndOwner, cidl, apidl, this, riid, ppvOut);
    }
    else if (riid == IID_IExtractIconA || riid == IID_IExtractIconW)
    {
        if (cidl == 1)
        {
            const FontPidlEntry* fontEntry = _FontFromIL(*apidl);
            if (fontEntry)
            {
                DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL;
                CStringW File = g_FontCache->Filename(fontEntry);
                // Just create a default icon extractor based on the filename
                // We might want to create a preview with the font to get really fancy one day.
                return SHCreateFileExtractIconW(File, dwAttributes, riid, ppvOut);
            }
        }
        else
        {
            ERR("IID_IExtractIcon with cidl != 1 UNIMPLEMENTED\n");
        }
    }
    else if (riid == IID_IDataObject)
    {
        if (cidl >= 1)
        {
            return _CDataObject_CreateInstance(m_Folder, cidl, apidl, riid, ppvOut);
        }
        else
        {
            ERR("IID_IDataObject with cidl == 0 UNIMPLEMENTED\n");
        }
    }

    //ERR("%s(riid=%S) UNIMPLEMENTED\n", __FUNCTION__, g2s(riid));
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!pidl)
        return E_NOTIMPL;

    // Validate that this pidl is the last one
    PCUIDLIST_RELATIVE curpidl = ILGetNext(pidl);
    if (curpidl->mkid.cb != 0)
    {
        ERR("ERROR, unhandled PIDL!\n");
        return E_FAIL;
    }

    const FontPidlEntry* fontEntry = _FontFromIL(pidl);
    if (!fontEntry)
        return E_FAIL;

    return SHSetStrRet(strRet, fontEntry->Name);
}

STDMETHODIMP CFontExt::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    ERR("%s() UNIMPLEMENTED\n", __FUNCTION__);
    return E_NOTIMPL;
}

// *** IPersistFolder2 methods ***
STDMETHODIMP CFontExt::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (ppidl && m_Folder)
    {
        *ppidl = ILClone(m_Folder);
        return S_OK;
    }

    return E_POINTER;
}


// *** IPersistFolder methods ***
STDMETHODIMP CFontExt::Initialize(LPCITEMIDLIST pidl)
{
    WCHAR PidlPath[MAX_PATH + 1] = {0}, Expected[MAX_PATH + 1];
    if (!SHGetPathFromIDListW(pidl, PidlPath))
    {
        ERR("Unable to extract path from pidl\n");
        return E_FAIL;
    }

    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, Expected);
    if (!SUCCEEDED(hr))
    {
        ERR("Unable to get fonts path (0x%x)\n", hr);
        return hr;
    }

    if (_wcsicmp(PidlPath, Expected))
    {
        ERR("CFontExt View initializing on unexpected folder: '%S'\n", PidlPath);
        return E_FAIL;
    }

    m_Folder.Attach(ILClone(pidl));

    return S_OK;
}


// *** IPersist methods ***
STDMETHODIMP CFontExt::GetClassID(CLSID *lpClassId)
{
    *lpClassId = CLSID_CFontExt;
    return S_OK;
}

// *** IDropTarget methods ***
STDMETHODIMP CFontExt::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    FORMATETC formatETC;
    HRESULT hr = DoGetFormatETC(pDataObj, formatETC);
    if (hr != S_OK)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return DRAGDROP_S_CANCEL;
    }

    // TODO: Check filename extension

#if 1   // Please implement DoGetFontTitle
    *pdwEffect = DROPEFFECT_NONE;
    return DRAGDROP_S_CANCEL;
#else
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
#endif
}

STDMETHODIMP CFontExt::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    return S_OK;
}

STDMETHODIMP CFontExt::DragLeave()
{
    return S_OK;
}

STDMETHODIMP CFontExt::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    FORMATETC formatETC;
    HRESULT hr = DoGetFormatETC(pDataObj, formatETC);
    if (hr != S_OK)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return DRAGDROP_S_CANCEL;
    }

    STGMEDIUM medium;
    hr = pDataObj->GetData(&formatETC, &medium);
    if (FAILED(hr))
    {
        *pdwEffect = DROPEFFECT_NONE;
        return DRAGDROP_S_CANCEL;
    }

    BOOL bOK = TRUE;
    CAtlArray<CStringW> FontPaths;
    HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
    if (hDrop)
    {
        WCHAR szPath[MAX_PATH];
        UINT iFile, cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        for (iFile = 0; iFile < cFiles; ++iFile)
        {
            DragQueryFile(hDrop, iFile, szPath, _countof(szPath));

            if (PathIsDirectoryW(szPath))
            {
                bOK = FALSE;
                break;
            }

            LPCWSTR pchDotExt = PathFindExtensionW(szPath);
            if (_wcsicmp(pchDotExt, L".ttf") != 0 &&
                _wcsicmp(pchDotExt, L".ttc") != 0 &&
                _wcsicmp(pchDotExt, L".otf") != 0 &&
                _wcsicmp(pchDotExt, L".otc") != 0 &&
                _wcsicmp(pchDotExt, L".fon") != 0 &&
                _wcsicmp(pchDotExt, L".fnt") != 0)
            {
                bOK = FALSE;
                break;
            }

            FontPaths.Add(szPath);
        }

        GlobalUnlock(medium.hGlobal);
    }
    ReleaseStgMedium(&medium);

    if (!bOK)
        return E_FAIL;

    WCHAR szFontsDir[MAX_PATH];
    GetWindowsDirectoryW(szFontsDir, _countof(szFontsDir));
    PathAppendW(szFontsDir, L"Fonts");

    HKEY hkeyFonts = NULL;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                  0,
                  KEY_READ | KEY_WRITE,
                  &hkeyFonts);
    if (!hkeyFonts)
        return E_FAIL;

    for (size_t iItem = 0; iItem < FontPaths.GetCount(); ++iItem)
    {
        HRESULT hr = DoInstallFontFile(FontPaths[iItem], szFontsDir, hkeyFonts);
        if (FAILED(hr))
        {
            bOK = FALSE;
            break;
        }
    }

    RegCloseKey(hkeyFonts);

    if (bOK)
    {
        SendMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    }

    // TODO: Show message

    return bOK ? S_OK : E_FAIL;
}

HRESULT CFontExt::DoGetFormatETC(IDataObject* pDataObj, FORMATETC& formatETC)
{
    if (!pDataObj)
        return E_POINTER;

    formatETC.cfFormat = CF_HDROP;
    formatETC.ptd = NULL;
    formatETC.dwAspect = DVASPECT_CONTENT;
    formatETC.lindex = -1;
    formatETC.tymed = TYMED_HGLOBAL;

    HRESULT hr = pDataObj->QueryGetData(&formatETC);
    return hr;
}

HRESULT CFontExt::DoInstallFontFile(LPCWSTR pszFontPath, LPCWSTR pszFontsDir, HKEY hkeyFonts)
{
    WCHAR szDestFile[MAX_PATH];
    LPCWSTR pszFileTitle = PathFindFileName(pszFontPath);

    WCHAR szFontName[512];
    if (!DoGetFontTitle(pszFontPath, szFontName))
        return E_FAIL;

    RemoveFontResourceW(pszFileTitle);

    StringCchCopyW(szDestFile, sizeof(szDestFile), pszFontsDir);
    PathAppendW(szDestFile, pszFileTitle);
    if (!CopyFileW(pszFontPath, szDestFile, FALSE))
        return E_FAIL;

    if (!AddFontResourceW(pszFileTitle))
        return E_FAIL;

    DWORD cbData = (wcslen(pszFileTitle) + 1) * sizeof(WCHAR);
    LONG nError = RegSetValueExW(hkeyFonts, szFontName, 0, REG_SZ, (const BYTE *)szFontName, cbData);
    if (nError)
    {
        return E_FAIL;
    }

    return E_FAIL;
}

HRESULT CFontExt::DoGetFontTitle(LPCWSTR pszFontPath, LPCWSTR pszFontName)
{
    // TODO:
    return E_FAIL;
}
