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
        result = StrCmpIW(fontEntry1->Name, fontEntry2->Name);
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
            result = StrCmpIW(fontEntry1->Name, fontEntry2->Name);
            break;
        case FONTEXT_COL_FILENAME:
            result = StrCmpIW(PathFindFileNameW(info1->File()), PathFindFileNameW(info2->File()));
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
    m_bDragAccepted = FALSE;

    STGMEDIUM stg;
    HDROP hDrop = GetDropFromDataObject(stg, pDataObj);
    if (!hDrop)
    {
        *pdwEffect = DROPEFFECT_NONE;
        DragLeave();
        return E_FAIL;
    }

    m_bDragAccepted = CheckDropFontFiles(hDrop);
    ::ReleaseStgMedium(&stg);

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

DWORD WINAPI CFontExt::InstallThreadProc(LPVOID lpParameter)
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

INT_PTR CALLBACK
CFontExt::InstallDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, PINSTALL_FONT_DATA pData)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pData->hwnd = hwnd;
            ATLASSERT(pData->cSteps >= 0);
            SendDlgItemMessageW(hwnd, IDC_INSTALL_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, pData->cSteps));
            if (!SHCreateThread(CFontExt::InstallThreadProc, pData, CTF_COINIT, NULL))
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

INT_PTR CALLBACK
CFontExt::InstallDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PINSTALL_FONT_DATA pData = (PINSTALL_FONT_DATA)GetWindowLongPtrW(hwnd, DWLP_USER);
    if (uMsg == WM_INITDIALOG)
    {
        pData = (PINSTALL_FONT_DATA)lParam;
        SetWindowLongPtrW(hwnd, DWLP_USER, lParam);
    }

    ATLASSERT(pData);
    ATLASSERT(pData->pFontExt);
    return pData->pFontExt->InstallDlgProc(hwnd, uMsg, wParam, lParam, pData);
}

STDMETHODIMP CFontExt::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    // NOTE: Getting cida in the other thread fails
    CDataObjectHIDA cida(pDataObj);
    if (!cida || cida->cidl <= 0)
    {
        ERR("E_UNEXPECTED\n");
        return E_UNEXPECTED;
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
    data.pFontExt = this;
    data.pDataObj = pDataObj;
    data.pidlParent = pidlParent;
    data.apidl = &apidl[0];
    data.cSteps = cida->cidl;
    pDataObj->AddRef();
    DialogBoxParamW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDD_INSTALL),
                    m_hwndView, CFontExt::InstallDialogProc, (LPARAM)&data);
    if (data.bCanceled)
        return E_ABORT;

    CStringW text, title;
    title.LoadStringW(IDS_REACTOS_FONTS_FOLDER);
    if (SUCCEEDED(data.hrResult))
    {
        // Invalidate our cache
        g_FontCache->Read();

        // Notify the system that a font was added
        SendMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

        // Show successful message
        text.LoadStringW(IDS_INSTALL_OK);
        MessageBoxW(m_hwndView, text, title, MB_ICONINFORMATION);
    }
    else
    {
        // Show error message
        text.LoadStringW(IDS_INSTALL_FAILED);
        MessageBoxW(m_hwndView, text, title, MB_ICONERROR);
    }

    return data.hrResult;
}
