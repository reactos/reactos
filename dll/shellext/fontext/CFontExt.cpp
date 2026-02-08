/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontExt implementation
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2019-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

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

static HRESULT FONTEXT_GetAttributeString(DWORD dwAttributes, LPWSTR pszOut, UINT cchMax)
{
    CStringW AttrLetters;
    AttrLetters.LoadString(IDS_COL_ATTR_LETTERS);

    if (AttrLetters.GetLength() != 5)
    {
        ERR("IDS_COL_ATTR_LETTERS does not contain 5 letters!\n");
        return E_FAIL;
    }

    UINT ich = 0;
    if ((dwAttributes & FILE_ATTRIBUTE_READONLY) && ich < cchMax)
        pszOut[ich++] = AttrLetters[0];
    if ((dwAttributes & FILE_ATTRIBUTE_HIDDEN) && ich < cchMax)
        pszOut[ich++] = AttrLetters[1];
    if ((dwAttributes & FILE_ATTRIBUTE_SYSTEM) && ich < cchMax)
        pszOut[ich++] = AttrLetters[2];
    if ((dwAttributes & FILE_ATTRIBUTE_ARCHIVE) && ich < cchMax)
        pszOut[ich++] = AttrLetters[3];
    if ((dwAttributes & FILE_ATTRIBUTE_COMPRESSED) && ich < cchMax)
        pszOut[ich++] = AttrLetters[4];
    if (ich < cchMax)
    {
        pszOut[ich] = UNICODE_NULL;
        return S_OK;
    }
    ERR("Buffer too short: %u\n", cchMax);
    return E_FAIL;
}

CFontExt::CFontExt()
{
    InterlockedIncrement(&g_ModuleRefCnt);
}

CFontExt::~CFontExt()
{
    InterlockedDecrement(&g_ModuleRefCnt);
}

void CFontExt::SetViewWindow(HWND hwndView)
{
    m_hwndView = hwndView;
}

void CFontExt::DeleteItems()
{
    DoDeleteFontFiles(m_hwndView, m_cidl, m_apidl);
}

void CFontExt::PreviewItems()
{
    for (UINT i = 0; i < m_cidl; ++i)
    {
        const FontPidlEntry* fontEntry = _FontFromIL(m_apidl[i]);
        if (fontEntry)
            RunFontViewer(m_hwndView, fontEntry);
    }
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
        ERR("Unable to query info about %S\n", fontEntry->Name());
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    int ret;
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
        {
            WCHAR szAttr[8];
            HRESULT hr = FONTEXT_GetAttributeString(info->FileAttributes(), szAttr, _countof(szAttr));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            return SHSetStrRet(&psd->str, szAttr);
        }
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
    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;
    if (pchEaten) *pchEaten = 0;

    // Load font cache
    if (g_FontCache->Size() == 0)
        g_FontCache->Read();

    if (!PathIsRelativeW(lpszDisplayName)) // Full path?
    {
        for (SIZE_T iFont = 0; iFont < g_FontCache->Size(); ++iFont)
        {
            CStringW filePath = g_FontCache->GetFontFilePath(g_FontCache->File(iFont));
            if (filePath.CompareNoCase(lpszDisplayName) == 0)
            {
                CStringW fontName = g_FontCache->Name(iFont), fileName = g_FontCache->File(iFont);
                if (fontName.IsEmpty())
                {
                    ERR("Why is fileName empty?\n");
                    return E_FAIL;
                }
                if (fileName.IsEmpty())
                {
                    ERR("Why is fileName empty?\n");
                    return E_FAIL;
                }

                // Create a PIDL
                *ppidl = _ILCreate(fontName, fileName);
                if (*ppidl == NULL)
                    return E_OUTOFMEMORY;

                if (pchEaten)
                    *pchEaten = wcslen(lpszDisplayName);

                if (pdwAttributes && *pdwAttributes)
                    *pdwAttributes &= (SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_CANCOPY |
                                       SFGAO_FILESYSTEM);

                return S_OK;
            }
        }
    }

    // Search font name
    for (SIZE_T iFont = 0; iFont < g_FontCache->Size(); ++iFont)
    {
        CStringW fontName = g_FontCache->Name(iFont);
        if (fontName.IsEmpty())
        {
            ERR("Why is fontName empty?\n");
            continue;
        }

        if (fontName.CompareNoCase(lpszDisplayName) == 0) // Found?
        {
            CStringW fileName = g_FontCache->File(iFont);
            if (fileName.IsEmpty())
            {
                ERR("Why is fileName empty?\n");
                continue;
            }

            // Create a PIDL
            *ppidl = _ILCreate(fontName, fileName);
            if (*ppidl == NULL)
                return E_OUTOFMEMORY;

            if (pchEaten)
                *pchEaten = wcslen(lpszDisplayName);

            if (pdwAttributes && *pdwAttributes)
                *pdwAttributes &= (SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_CANCOPY |
                                   SFGAO_FILESYSTEM);

            return S_OK;
        }
    }

    // Not found
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
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
        result = StrCmpIW(fontEntry1->Name(), fontEntry2->Name());
    }
    else
    {
        auto info1 = g_FontCache->Find(fontEntry1);
        auto info2 = g_FontCache->Find(fontEntry2);

        if (!info1 || !info2)
        {
            ERR("Unable to find font %S or %S in cache!\n", fontEntry1->Name(), fontEntry2->Name());
            return E_INVALIDARG;
        }

        switch (column)
        {
        case 0xffff:
            /* ROS bug? */
        case FONTEXT_COL_NAME:
            result = StrCmpIW(fontEntry1->Name(), fontEntry2->Name());
            break;
        case FONTEXT_COL_FILENAME:
            result = StrCmpIW(PathFindFileNameW(info1->File()), PathFindFileNameW(info2->File()));
            break;
        case FONTEXT_COL_SIZE:
            {
                ULONGLONG size1 = info1->FileSize().QuadPart, size2 = info2->FileSize().QuadPart;
                result = (size1 < size2) ? -1 : ((size1 > size2) ? 1 : 0);
            }
            break;
        case FONTEXT_COL_MODIFIED:
            result = CompareFileTime(&info1->FileWriteTime(), &info2->FileWriteTime());
            break;
        case FONTEXT_COL_ATTR:
            {
                HRESULT hr;
                WCHAR szAttr1[8], szAttr2[8];
                hr = FONTEXT_GetAttributeString(info1->FileAttributes(), szAttr1, _countof(szAttr1));
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
                hr = FONTEXT_GetAttributeString(info2->FileAttributes(), szAttr2, _countof(szAttr2));
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
                result = _wcsicmp(szAttr1, szAttr2);
            }
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
        *ppvOut = static_cast<IDropTarget *>(this);
        AddRef();
        hr = S_OK;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        TRACE("IContextMenu\n");
        return CFontBackgroundMenu_Create(this, hwndOwner, this, (IContextMenu**)ppvOut);
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        CComPtr<CFontFolderViewCB> sfviewcb;
        if (SUCCEEDED(hr = ShellObjectCreator(sfviewcb)))
        {
            SFV_CREATE create = { sizeof(create), this, NULL, sfviewcb };
            hr = SHCreateShellFolderView(&create, (IShellView**)ppvOut);
            if (SUCCEEDED(hr))
                sfviewcb->Initialize(this, (IShellView*)*ppvOut, m_Folder);
        }
    }

    return hr;
}

STDMETHODIMP CFontExt::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    if (!rgfInOut || !cidl || !apidl)
        return E_INVALIDARG;

    DWORD rgf = (SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_CANCOPY | SFGAO_FILESYSTEM);
    while (cidl > 0 && *apidl)
    {
        const FontPidlEntry* fontEntry = _FontFromIL(*apidl);
        if (!fontEntry)
        {
            rgf = 0;
            break;
        }

        apidl++;
        cidl--;
    }

    *rgfInOut &= rgf;
    return S_OK;
}

HRESULT CALLBACK CFontExt::MenuCallback(
    IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
    UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFontExt* pThis = static_cast<CFontExt*>(psf);
    if (!pThis)
        return E_FAIL;

    pThis->m_pDataObj = pdtobj;

    switch (uMsg)
    {
        case DFM_MERGECONTEXTMENU:
        {
            QCMINFO* pqcminfo = (QCMINFO*)lParam;
            // Insert [Preview] menu item
            CString strPreview(MAKEINTRESOURCEW(IDS_FONT_PREVIEW));
            ::InsertMenuW(pqcminfo->hmenu, 0, MF_BYPOSITION | MF_STRING,
                          pqcminfo->idCmdFirst++, strPreview); // Command 0
            // Make it default
            ::SetMenuDefaultItem(pqcminfo->hmenu, pqcminfo->idCmdFirst - 1, FALSE);
            return S_OK;
        }
        case DFM_GETVERBA:
        case DFM_GETVERBW:
        {
            // Replace default "open" command action
            UINT idCmd = LOWORD(wParam), cchMax = HIWORD(wParam);
            if (idCmd == 0)
            {
                if (uMsg == DFM_GETVERBA)
                    lstrcpynA((LPSTR)lParam, "open", cchMax);
                else
                    lstrcpynW((LPWSTR)lParam, L"open", cchMax);
                return S_OK;
            }
            break;
        }
        case DFM_INVOKECOMMANDEX:
            return E_NOTIMPL;
        case DFM_INVOKECOMMAND:
        {
            if (wParam == 0)
            {
                pThis->PreviewItems();
                return S_OK;
            }
            if (wParam == DFM_CMD_COPY)
                return S_FALSE;
            if (wParam == DFM_CMD_DELETE)
            {
                pThis->DeleteItems();
                return S_OK;
            }
            if (wParam == DFM_CMD_PASTE)
                return S_FALSE;
            if (wParam == DFM_CMD_PROPERTIES)
                return S_FALSE;
            ERR("wParam: %p\n", wParam);
            return E_FAIL;
        }
        case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
            return S_FALSE;
        case DFM_WM_INITMENUPOPUP:
        {
            HMENU hMenu = (HMENU)wParam;
            // Delete default [Open] menu item
            ::DeleteMenu(hMenu, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);
            // Disable [Paste link] menu item
            ::EnableMenuItem(hMenu, FCIDM_SHVIEW_INSERTLINK, MF_BYCOMMAND | MF_GRAYED);
            break;
        }
    }
    return E_NOTIMPL;
}

STDMETHODIMP CFontExt::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    if (riid == IID_IContextMenu ||
        riid == IID_IContextMenu2 ||
        riid == IID_IContextMenu3)
    {
        if (cidl <= 0)
            return E_FAIL;
        m_cidl = cidl;
        m_apidl = apidl;
        return CDefFolderMenu_Create2(NULL, hwndOwner, cidl, apidl, this, MenuCallback,
                                      0, NULL, (IContextMenu**)ppvOut);
    }
    else if (riid == IID_IExtractIconA || riid == IID_IExtractIconW)
    {
        if (cidl == 1)
        {
            const FontPidlEntry* fontEntry = _FontFromIL(*apidl);
            if (fontEntry)
            {
                DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL;
                CStringW strFileName = g_FontCache->GetFontFilePath(fontEntry->FileName());
                return SHCreateFileExtractIconW(strFileName, dwAttributes, riid, ppvOut);
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
            CComPtr<IDataObject> pDataObj;
            HRESULT hr = OleGetClipboard(&pDataObj);
            if (FAILED_UNEXPECTEDLY(hr))
                return E_FAIL;
            *ppvOut = pDataObj.Detach();
            return S_OK;
        }
    }
    else if (riid == IID_IDropTarget)
    {
        *ppvOut = static_cast<IDropTarget*>(this);
        AddRef();
        return S_OK;
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

    if (dwFlags & SHGDN_FORPARSING)
    {
        CStringW File = g_FontCache->Filename(g_FontCache->Find(fontEntry), true);
        if (!File.IsEmpty())
        {
            return SHSetStrRet(strRet, File);
        }
    }

    return SHSetStrRet(strRet, fontEntry->Name());
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

    if (StrCmpIW(PidlPath, FontsDir))
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
    m_bDragAccepted = CheckDataObject(pDataObj);
    if (!m_bDragAccepted)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return E_FAIL;
    }

    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CFontExt::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!m_bDragAccepted)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return E_FAIL;
    }
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CFontExt::DragLeave()
{
    return S_OK;
}

STDMETHODIMP CFontExt::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    ATLASSERT(m_hwndView);
    HRESULT hr = InstallFontsFromDataObject(m_hwndView, pDataObj);

    CStringW text, title;
    title.LoadStringW(IDS_REACTOS_FONTS_FOLDER);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        // Show error message
        text.LoadStringW(IDS_INSTALL_FAILED);
        MessageBoxW(m_hwndView, text, title, MB_ICONERROR);
    }
    else if (hr == S_OK)
    {
        // Refresh font cache and notify the system about the font change
        if (g_FontCache)
            g_FontCache->Read();

        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"fonts", SMTO_ABORTIFHUNG, 1000, NULL);
        SendMessageTimeoutW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0, SMTO_ABORTIFHUNG, 1000, NULL);

        // Show successful message
        text.LoadStringW(IDS_INSTALL_OK);
        MessageBoxW(m_hwndView, text, title, MB_ICONINFORMATION);
    }

    return hr;
}
