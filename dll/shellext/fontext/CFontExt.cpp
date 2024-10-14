/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontExt implementation
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "undocgdi.h" // for GetFontResourceInfoW

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

#ifndef SHCIDS_ALLFIELDS
#define SHCIDS_ALLFIELDS 0x80000000L
#endif

struct FolderViewColumns
{
    int iResource;
    DWORD dwDefaultState;
    int cxChar;
    int fmt;
};

enum font_columns
{
    FONTEXT_COL_NAME,
    FONTEXT_COL_FILENAME,
    FONTEXT_COL_SIZE,
    FONTEXT_COL_MODIFIED,
    FONTEXT_COL_ATTR,
};

static FolderViewColumns g_ColumnDefs[] =
{
    { IDS_COL_NAME,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   25, LVCFMT_LEFT },
    { IDS_COL_FILENAME,  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   20, LVCFMT_LEFT },
    { IDS_COL_SIZE,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   10, LVCFMT_RIGHT },
    { IDS_COL_MODIFIED,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT,  15, LVCFMT_LEFT },
    { IDS_COL_ATTR,      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,   12, LVCFMT_RIGHT },
};


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

    // Name, ReactOS specific?
    if (iColumn == FONTEXT_COL_NAME)
        return GetDisplayNameOf(pidl, 0, &psd->str);

    const FontPidlEntry* fontEntry = _FontFromIL(pidl);
    if (!fontEntry)
    {
        ERR("ERROR, not a font PIDL!\n");
        return E_FAIL;
    }

    // If we got here, we are in details view!
    auto info = g_FontCache->Find(fontEntry);
    if (info == nullptr)
    {
        ERR("Unable to query info about %S\n", fontEntry->Name);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    int ret;
    CStringA AttrLetters;
    DWORD dwAttributes;
    SYSTEMTIME time;
    switch (iColumn)
    {
    case FONTEXT_COL_FILENAME:
        return SHSetStrRet(&psd->str, PathFindFileNameW(info->File()));
    case FONTEXT_COL_SIZE:
        psd->str.uType = STRRET_CSTR;
        StrFormatKBSizeA(info->FileSize().QuadPart, psd->str.cStr, MAX_PATH);
        return S_OK;
    case FONTEXT_COL_MODIFIED:
        psd->str.uType = STRRET_CSTR;
        FileTimeToSystemTime(&info->FileWriteTime(), &time);
        ret = GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, psd->str.cStr, MAX_PATH);
        if (ret < 1)
        {
            ERR("GetDateFormatA failed\n");
            return E_FAIL;
        }
        psd->str.cStr[ret-1] = ' ';
        GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, &psd->str.cStr[ret], MAX_PATH - ret);
        return S_OK;
    case FONTEXT_COL_ATTR:
        AttrLetters.LoadString(IDS_COL_ATTR_LETTERS);
        if (AttrLetters.GetLength() != 5)
        {
            ERR("IDS_COL_ATTR_LETTERS does not contain 5 letters!\n");
            return E_FAIL;
        }
        psd->str.uType = STRRET_CSTR;
        dwAttributes = info->FileAttributes();
        ret = 0;
        if (dwAttributes & FILE_ATTRIBUTE_READONLY)
            psd->str.cStr[ret++] = AttrLetters[0];
        if (dwAttributes & FILE_ATTRIBUTE_HIDDEN)
            psd->str.cStr[ret++] = AttrLetters[1];
        if (dwAttributes & FILE_ATTRIBUTE_SYSTEM)
            psd->str.cStr[ret++] = AttrLetters[2];
        if (dwAttributes & FILE_ATTRIBUTE_ARCHIVE)
            psd->str.cStr[ret++] = AttrLetters[3];
        if (dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
            psd->str.cStr[ret++] = AttrLetters[4];
        psd->str.cStr[ret] = '\0';
        return S_OK;
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

    int result;
    DWORD sortMode = lParam & 0xFFFF0000;
    DWORD column = lParam & 0x0000FFFF;
    if (sortMode == SHCIDS_ALLFIELDS)
    {
        UNIMPLEMENTED;
        result = (int)fontEntry1->Index - (int)fontEntry2->Index;
    }
    else
    {
        auto info1 = g_FontCache->Find(fontEntry1);
        auto info2 = g_FontCache->Find(fontEntry2);

        if (!info1 || !info2)
        {
            ERR("Unable to find font %S or %S in cache!\n", fontEntry1->Name, fontEntry2->Name);
            return E_INVALIDARG;
        }

        switch (column)
        {
        case 0xffff:
            /* ROS bug? */
        case FONTEXT_COL_NAME:
            // These items are already ordered by name
            result = (int)fontEntry1->Index - (int)fontEntry2->Index;
            break;
        case FONTEXT_COL_FILENAME:
            result = wcsicmp(PathFindFileNameW(info1->File()), PathFindFileNameW(info2->File()));
            break;
        case FONTEXT_COL_SIZE:
            result = (int)info1->FileSize().HighPart - info2->FileSize().HighPart;
            if (result == 0)
                result = (int)info1->FileSize().LowPart - info2->FileSize().LowPart;
            break;
        case FONTEXT_COL_MODIFIED:
            result = CompareFileTime(&info1->FileWriteTime(), &info2->FileWriteTime());
            break;
        case FONTEXT_COL_ATTR:
            // FIXME: how to compare attributes?
            result = (int)info1->FileAttributes() - info2->FileAttributes();
            break;
        default:
            ERR("Unimplemented column %u\n", column);
            return E_INVALIDARG;
        }
    }

    return MAKE_COMPARE_HRESULT(result);
}

STDMETHODIMP CFontExt::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        ERR("IDropTarget not implemented\n");
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
                CStringW File = g_FontCache->Filename(g_FontCache->Find(fontEntry));
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

    if (dwFlags == SHGDN_FORPARSING)
    {
        CStringW File = g_FontCache->Filename(g_FontCache->Find(fontEntry), true);
        if (!File.IsEmpty())
        {
            return SHSetStrRet(strRet, File);
        }
    }

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
    WCHAR PidlPath[MAX_PATH + 1] = {0}, FontsDir[MAX_PATH + 1];
    if (!SHGetPathFromIDListW(pidl, PidlPath))
    {
        ERR("Unable to extract path from pidl\n");
        return E_FAIL;
    }

    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, FontsDir);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("Unable to get fonts path (0x%x)\n", hr);
        return hr;
    }

    if (_wcsicmp(PidlPath, FontsDir))
    {
        ERR("CFontExt View initializing on unexpected folder: '%S'\n", PidlPath);
        return E_FAIL;
    }

    m_Folder.Attach(ILClone(pidl));
    StringCchCatW(FontsDir, _countof(FontsDir), L"\\");
    g_FontCache->SetFontDir(FontsDir);

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
    *pdwEffect = DROPEFFECT_NONE;

    CDataObjectHIDA cida(pDataObj);
    if (FAILED_UNEXPECTEDLY(cida.hr()))
        return cida.hr();

    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
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
    *pdwEffect = DROPEFFECT_NONE;

    CDataObjectHIDA cida(pDataObj);
    if (!cida)
        return E_UNEXPECTED;

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(cida);
    if (!pidlParent)
    {
        ERR("pidlParent is NULL\n");
        return E_FAIL;
    }

    BOOL bOK = TRUE;
    CAtlArray<CStringW> FontPaths;
    for (UINT n = 0; n < cida->cidl; ++n)
    {
        PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(cida, n);
        if (!pidlRelative)
            continue;

        PIDLIST_ABSOLUTE pidl = ILCombine(pidlParent, pidlRelative);
        if (!pidl)
        {
            ERR("ILCombine failed\n");
            bOK = FALSE;
            break;
        }

        WCHAR szPath[MAX_PATH];
        BOOL ret = SHGetPathFromIDListW(pidl, szPath);
        ILFree(pidl);

        if (!ret)
        {
            ERR("SHGetPathFromIDListW failed\n");
            bOK = FALSE;
            break;
        }

        if (PathIsDirectoryW(szPath))
        {
            ERR("PathIsDirectory\n");
            bOK = FALSE;
            break;
        }

        LPCWSTR pchDotExt = PathFindExtensionW(szPath);
        if (!IsFontDotExt(pchDotExt))
        {
            ERR("'%S' is not supported\n", pchDotExt);
            bOK = FALSE;
            break;
        }

        FontPaths.Add(szPath);
    }

    if (!bOK)
        return E_FAIL;

    CRegKey keyFonts;
    if (keyFonts.Open(FONT_HIVE, FONT_KEY, KEY_WRITE) != ERROR_SUCCESS)
    {
        ERR("keyFonts.Open failed\n");
        return E_FAIL;
    }

    for (size_t iItem = 0; iItem < FontPaths.GetCount(); ++iItem)
    {
        HRESULT hr = DoInstallFontFile(FontPaths[iItem], g_FontCache->FontPath(), keyFonts.m_hKey);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            bOK = FALSE;
            break;
        }
    }

    // Invalidate our cache
    g_FontCache->Read();

    // Notify the system that a font was added
    SendMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

    // Notify the shell that the folder contents are changed
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, g_FontCache->FontPath().GetString(), NULL);

    // TODO: Show message

    return bOK ? S_OK : E_FAIL;
}

HRESULT CFontExt::DoInstallFontFile(LPCWSTR pszFontPath, LPCWSTR pszFontsDir, HKEY hkeyFonts)
{
    WCHAR szDestFile[MAX_PATH];

    // Add this font to the font list, so we can query the name
    if (!AddFontResourceW(pszFontPath))
    {
        ERR("AddFontResourceW('%S') failed\n", pszFontPath);
        DeleteFileW(szDestFile);
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

    AddFontResourceW(szDestFile);

    return S_OK;
}

HRESULT
CFontExt::DoGetFontTitle(IN LPCWSTR pszFontPath, OUT CStringW& strFontName)
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
