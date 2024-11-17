/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shell folder implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "cabview.h"
#include "util.h"

enum FOLDERCOLUMNS
{
    COL_NAME,   // PKEY_ItemNameDisplay
    COL_SIZE,   // PKEY_Size
    COL_TYPE,   // PKEY_ItemTypeText
    COL_MDATE,  // PKEY_DateModified
    COL_PATH,   // PKEY_?: Archive-relative path
    COL_ATT,    // PKEY_FileAttributes
    COLCOUNT
};

static const struct FOLDERCOLUMN
{
    BYTE TextId;
    BYTE LvcFmt;
    BYTE LvcChars;
    BYTE ColFlags;
    const GUID *pkg;
    BYTE pki;
} g_Columns[] =
{
    { IDS_COL_NAME, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, &FMTID_Storage, PID_STG_NAME },
    { IDS_COL_SIZE, LVCFMT_RIGHT, 16, SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT, &FMTID_Storage, PID_STG_SIZE },
    { IDS_COL_TYPE, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, &FMTID_Storage, PID_STG_STORAGETYPE },
    { IDS_COL_MDATE, LVCFMT_LEFT, 20, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, &FMTID_Storage, PID_STG_WRITETIME },
    { IDS_COL_PATH, LVCFMT_LEFT, 30, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, &CLSID_CabFolder, 0 },
    { IDS_COL_ATT, LVCFMT_RIGHT, 10, SHCOLSTATE_TYPE_STR, &FMTID_Storage, PID_STG_ATTRIBUTES },
};

#include <pshpack1.h>
struct CABITEM
{
    WORD cb;
    WORD Unknown; // Not sure what Windows uses this for, we always store 0
    UINT Size;
    WORD Date, Time; // DOS
    WORD Attrib;
    WORD NameOffset;
    WCHAR Path[ANYSIZE_ARRAY];

#if FLATFOLDER
    inline bool IsFolder() const { return false; }
#else
    inline BOOL IsFolder() const { return Attrib & FILE_ATTRIBUTE_DIRECTORY; }
#endif
    enum { FSATTS = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
                    FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DIRECTORY };
    WORD GetFSAttributes() const { return Attrib & FSATTS; }
    LPCWSTR GetName() const { return Path + NameOffset; }

    template<class PIDL> static CABITEM* Validate(PIDL pidl)
    {
        CABITEM *p = (CABITEM*)pidl;
        return p && p->cb > FIELD_OFFSET(CABITEM, Path[1]) && p->Unknown == 0 ? p : NULL;
    }
};
#include <poppack.h>

static CABITEM* CreateItem(LPCWSTR Path, UINT Attrib, UINT Size, UINT DateTime)
{
    const SIZE_T len = lstrlenW(Path), cb = FIELD_OFFSET(CABITEM, Path[len + 1]);
    if (cb > 0xffff)
        return NULL;
    CABITEM *p = (CABITEM*)SHAlloc(cb + sizeof(USHORT));
    if (p)
    {
        p->cb = (USHORT)cb;
        p->Unknown = 0;
        p->Size = Size;
        p->Attrib = Attrib;
        p->Date = HIWORD(DateTime);
        p->Time = LOWORD(DateTime);
        p->NameOffset = 0;
        for (UINT i = 0;; ++i)
        {
            WCHAR c = Path[i];
            if (c == L':') // Don't allow absolute paths
                c = L'_';
            if (c == L'/') // Normalize
                c = L'\\';
            if (c == '\\')
                p->NameOffset = i + 1;
            p->Path[i] = c;
            if (!c)
                break;
        }
        ((SHITEMID*)((BYTE*)p + cb))->cb = 0;
    }
    return p;
}

static CABITEM* CreateItem(LPCSTR Path, UINT Attrib, UINT Size = 0, UINT DateTime = 0)
{
    WCHAR buf[MAX_PATH * 2];
    UINT codepage = (Attrib & _A_NAME_IS_UTF) ? CP_UTF8 : CP_ACP;
    if (MultiByteToWideChar(codepage, 0, Path, -1, buf, _countof(buf)))
        return CreateItem(buf, Attrib, Size, DateTime);
    return NULL;
}

static HRESULT CALLBACK ItemMenuCallback(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
                                         UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum { IDC_EXTRACT, IDC_EXTRACTALL };
    HRESULT hr = E_NOTIMPL;
    const BOOL Background = !pdtobj;

    switch (uMsg)
    {
        case DFM_MODIFYQCMFLAGS:
        {
            *((UINT*)lParam) = wParam | CMF_NOVERBS | (Background ? 0 : CMF_VERBSONLY);
            return S_OK;
        }

        case DFM_MERGECONTEXTMENU:
        {
            QCMINFO &qcmi = *(QCMINFO*)lParam;
            UINT pos = qcmi.indexMenu, id = 0;
            if (Background || SUCCEEDED(hr = InsertMenuItem(qcmi, pos, id, IDC_EXTRACT, IDS_EXTRACT, MFS_DEFAULT)))
            {
                hr = InsertMenuItem(qcmi, pos, id, IDC_EXTRACTALL, IDS_EXTRACTALL);
                if (SUCCEEDED(hr) && !Background)
                {
                    --pos;
                    InsertMenuItem(qcmi, pos, id, 0, -1); // Separator
                }
            }
            if (SUCCEEDED(hr))
            {
                qcmi.idCmdFirst = id + 1;
                hr = S_FALSE; // Don't add verbs
            }
            break;
        }

        case DFM_INVOKECOMMAND:
        {
            hr = S_FALSE;
            CCabFolder *pCabFolder = static_cast<CCabFolder*>(psf);
            switch (wParam)
            {
                case IDC_EXTRACT:
                case IDC_EXTRACTALL:
                    hr = pCabFolder->ExtractFilesUI(hwnd, wParam == IDC_EXTRACT ? pdtobj : NULL);
                    break;
            }
            break;
        }
    }
    return hr;
}

static HRESULT CALLBACK FolderBackgroundMenuCallback(IShellFolder *psf, HWND hwnd,
                                                     IDataObject *pdtobj, UINT uMsg,
                                                     WPARAM wParam, LPARAM lParam)
{
    return ItemMenuCallback(psf, hwnd, NULL, uMsg, wParam, lParam);
}

int CEnumIDList::FindNamedItem(PCUITEMID_CHILD pidl) const
{
    CABITEM *needle = (CABITEM*)pidl;
    for (ULONG i = 0, c = GetCount(); i < c; ++i)
    {
        CABITEM *item = (CABITEM*)DPA_FastGetPtr(m_Items, i);
        if (!lstrcmpiW(needle->Path, item->Path))
            return i;
    }
    return -1;
}

struct FILLCALLBACKDATA
{
    CEnumIDList *pEIDL;
    SHCONTF ContF;
};

static HRESULT CALLBACK EnumFillCallback(EXTRACTCALLBACKMSG msg, const EXTRACTCALLBACKDATA &ecd, LPVOID cookie)
{
    FILLCALLBACKDATA &data = *(FILLCALLBACKDATA*)cookie;

    switch ((UINT)msg)
    {
        case ECM_FILE:
        {
            const FDINOTIFICATION &fdin = *ecd.pfdin;
            HRESULT hr = S_FALSE;
            SFGAOF attr = MapFSToSFAttributes(fdin.attribs & CABITEM::FSATTS);
            if (IncludeInEnumIDList(data.ContF, attr))
            {
                UINT datetime = MAKELONG(fdin.time, fdin.date);
                CABITEM *item = CreateItem(fdin.psz1, fdin.attribs, fdin.cb, datetime);
                if (!item)
                    return E_OUTOFMEMORY;
                if (FAILED(hr = data.pEIDL->Append((LPCITEMIDLIST)item)))
                    SHFree(item);
            }
            return SUCCEEDED(hr) ? S_FALSE : hr; // Never extract
        }
    }
    return E_NOTIMPL;
}

HRESULT CEnumIDList::Fill(LPCWSTR path, HWND hwnd, SHCONTF contf)
{
    FILLCALLBACKDATA data = { this, contf };
    return ExtractCabinet(path, NULL, EnumFillCallback, &data);
}

HRESULT CEnumIDList::Fill(PCIDLIST_ABSOLUTE pidl, HWND hwnd, SHCONTF contf)
{
    WCHAR path[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, path))
        return Fill(path, hwnd, contf);
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

IFACEMETHODIMP CCabFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = COL_NAME;
    if (pDisplay)
        *pDisplay = COL_NAME;
    return S_OK;
}

IFACEMETHODIMP CCabFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    if (!pcsFlags || iColumn >= _countof(g_Columns))
        return E_INVALIDARG;
    *pcsFlags = g_Columns[iColumn].ColFlags;
    return S_OK;
}

IFACEMETHODIMP CCabFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName)
{
    CABITEM *item = CABITEM::Validate(pidl);
    if (!item || !pName)
        return E_INVALIDARG;

    if (dwFlags & SHGDN_FORPARSING)
    {
        if (dwFlags & SHGDN_INFOLDER)
            return StrTo(FLATFOLDER ? item->Path : item->GetName(), *pName);

        WCHAR parent[MAX_PATH];
        if (!SHGetPathFromIDListW(m_CurDir, parent))
            return E_FAIL;
        UINT cch = lstrlenW(parent) + 1 + lstrlenW(item->Path) + 1;
        pName->uType = STRRET_WSTR;
        pName->pOleStr = (LPWSTR)SHAlloc(cch * sizeof(WCHAR));
        if (!pName->pOleStr)
            return E_OUTOFMEMORY;
        lstrcpyW(pName->pOleStr, parent);
        PathAppendW(pName->pOleStr, item->Path);
        return S_OK;
    }

    SHFILEINFO fi;
    DWORD attr = item->IsFolder() ? FILE_ATTRIBUTE_DIRECTORY : 0;
    UINT flags = SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES;
    if (SHGetFileInfo(item->GetName(), attr, &fi, sizeof(fi), flags))
        return StrTo(fi.szDisplayName, *pName);
    return StrTo(item->GetName(), *pName);
}

HRESULT CCabFolder::GetItemDetails(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd, VARIANT *pv)
{
    HRESULT hr = E_FAIL;
    STRRET *psr = &psd->str, srvar;
    CABITEM *item = CABITEM::Validate(pidl);
    if (!item)
        return E_INVALIDARG;

    switch (iColumn)
    {
        case COL_NAME:
        {
            hr = GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, pv ? &srvar : psr);
            return SUCCEEDED(hr) && pv ? StrRetToVariantBSTR(&srvar, *pv) : hr;
        }

        case COL_SIZE:
        {
            UINT data = item->Size;
            if (pv)
            {
                V_VT(pv) = VT_UI4;
                V_UI4(pv) = data;
            }
            else
            {
                psr->uType = STRRET_CSTR;
                StrFormatByteSizeA(data, psr->cStr, _countof(psr->cStr));
            }
            return S_OK;
        }

        case COL_TYPE:
        {
            SHFILEINFO fi;
            LPCWSTR data = fi.szTypeName;
            DWORD attr = item->GetFSAttributes();
            UINT flags = SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES;
            if (SHGetFileInfo(item->GetName(), attr, &fi, sizeof(fi), flags))
                return pv ? StrTo(data, *pv) : StrTo(data, *psr);
            break;
        }

        case COL_MDATE:
        {
            if (pv)
            {
                if (DosDateTimeToVariantTime(item->Date, item->Time, &V_DATE(pv)))
                {
                    V_VT(pv) = VT_DATE;
                    return S_OK;
                }
            }
            else
            {
                FILETIME utc, loc;
                if (DosDateTimeToFileTime(item->Date, item->Time, &utc) && FileTimeToLocalFileTime(&utc, &loc))
                {
                    psr->uType = STRRET_CSTR;
                    if (SHFormatDateTimeA(&loc, NULL, psr->cStr, _countof(psr->cStr)))
                    {
                        return S_OK;
                    }
                }
            }
            break;
        }

        case COL_PATH:
        {
            UINT len = item->NameOffset ? item->NameOffset - 1 : 0;
            return pv ? StrTo(item->Path, len, *pv) : StrTo(item->Path, len, *psr);
        }

        case COL_ATT:
        {
            UINT data = item->GetFSAttributes();
            if (pv)
            {
                V_VT(pv) = VT_UI4;
                V_UI4(pv) = data;
            }
            else
            {
                UINT i = 0;
                psr->uType = STRRET_CSTR;
                if (data & FILE_ATTRIBUTE_READONLY) psr->cStr[i++] = 'R';
                if (data & FILE_ATTRIBUTE_HIDDEN) psr->cStr[i++] = 'H';
                if (data & FILE_ATTRIBUTE_SYSTEM) psr->cStr[i++] = 'S';
                if (data & FILE_ATTRIBUTE_ARCHIVE) psr->cStr[i++] = 'A';
                psr->cStr[i++] = '\0';
            }
            return S_OK;
        }
    }
    return hr;
}

IFACEMETHODIMP CCabFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    if (!pscid || !pv)
        return E_INVALIDARG;

    CABITEM *item;
    int col = MapSCIDToColumn(*pscid);
    if (col >= 0)
    {
        return GetItemDetails(pidl, col, NULL, pv);
    }
    else if ((item = CABITEM::Validate(pidl)) == NULL)
    {
        return E_INVALIDARG;
    }
    else if (IsEqual(*pscid, FMTID_ShellDetails, PID_FINDDATA))
    {
        WIN32_FIND_DATA wfd;
        ZeroMemory(&wfd, sizeof(wfd));
        wfd.dwFileAttributes = item->GetFSAttributes();
        wfd.nFileSizeLow = item->Size;
        DosDateTimeToFileTime(item->Date, item->Time, &wfd.ftLastWriteTime);
        lstrcpyn(wfd.cFileName, item->GetName(), MAX_PATH);
        return InitVariantFromBuffer(&wfd, sizeof(wfd), pv);
    }
    return E_FAIL;
}

IFACEMETHODIMP CCabFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (!psd || iColumn >= _countof(g_Columns))
    {
        return E_INVALIDARG;
    }
    else if (!pidl)
    {
        psd->fmt = g_Columns[iColumn].LvcFmt;
        psd->cxChar = g_Columns[iColumn].LvcChars;
        WCHAR buf[MAX_PATH];
        if (LoadStringW(_AtlBaseModule.GetResourceInstance(), g_Columns[iColumn].TextId, buf, _countof(buf)))
            return StrTo(buf, psd->str);
        return E_FAIL;
    }
    return GetItemDetails(pidl, iColumn, psd, NULL);
}

int CCabFolder::MapSCIDToColumn(const SHCOLUMNID &scid)
{
    for (UINT i = 0; i < _countof(g_Columns); ++i)
    {
        if (g_Columns[i].pkg && IsEqual(scid, *g_Columns[i].pkg, g_Columns[i].pki))
            return i;
    }
    return -1;
}

IFACEMETHODIMP CCabFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    if (column < _countof(g_Columns) && g_Columns[column].pkg)
    {
        pscid->fmtid = *g_Columns[column].pkg;
        pscid->pid = g_Columns[column].pki;
        return S_OK;
    }
    return E_FAIL;
}

IFACEMETHODIMP CCabFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CEnumIDList *p = CEnumIDList::CreateInstance();
    *ppEnumIDList = static_cast<LPENUMIDLIST>(p);
    return p ? p->Fill(m_CurDir, hwndOwner, dwFlags) : E_OUTOFMEMORY;
}

IFACEMETHODIMP CCabFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT CCabFolder::CompareID(LPARAM lParam, PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2)
{
    CABITEM *p1 = (CABITEM*)pidl1, *p2 = (CABITEM*)pidl2;
    HRESULT hr = S_OK;
    int ret = 0;

    if (lParam & (SHCIDS_ALLFIELDS | SHCIDS_CANONICALONLY))
    {
        ret = lstrcmpiW(p1->Path, p2->Path);
        if (ret && (lParam & SHCIDS_ALLFIELDS))
        {
            for (UINT i = 0; ret && SUCCEEDED(hr) && i < COLCOUNT; ++i)
            {
                hr = (i == COL_NAME) ? 0 : CompareID(i, pidl1, pidl2);
                ret = (short)HRESULT_CODE(hr);
            }
        }
    }
    else
    {
        UINT col = lParam & SHCIDS_COLUMNMASK;
        switch (col)
        {
            case COL_NAME:
                ret = StrCmpLogicalW(p1->GetName(), p2->GetName());
                break;

            case COL_SIZE:
                ret = p1->Size - p2->Size;
                break;

            case COL_MDATE:
                ret = MAKELONG(p1->Time, p1->Date) - MAKELONG(p2->Time, p2->Date);
                break;

            default:
            {
                if (col < COLCOUNT)
                {
                    PWSTR str1, str2;
                    if (SUCCEEDED(hr = ::GetDetailsOf(*this, pidl1, col, str1)))
                    {
                        if (SUCCEEDED(hr = ::GetDetailsOf(*this, pidl2, col, str2)))
                        {
                            ret = StrCmpLogicalW(str1, str2);
                            SHFree(str2);
                        }
                        SHFree(str1);
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
        }
    }
    return SUCCEEDED(hr) ? MAKE_COMPARE_HRESULT(ret) : hr;
}

IFACEMETHODIMP CCabFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    C_ASSERT(FLATFOLDER);
    if (!pidl1 || !ILIsSingle(pidl1) || !pidl2 || !ILIsSingle(pidl2))
        return E_UNEXPECTED;

    return CompareID(lParam, pidl1, pidl2);
}

IFACEMETHODIMP CCabFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvc = { sizeof(SFV_CREATE), static_cast<IShellFolder*>(this) };
        return SHCreateShellFolderView(&sfvc, (IShellView**)ppv);
    }
    if (riid == IID_IContextMenu)
    {
        LPFNDFMCALLBACK func = FolderBackgroundMenuCallback;
        IContextMenu **ppCM = (IContextMenu**)ppv;
        return CDefFolderMenu_Create2(m_CurDir, hwndOwner, 0, NULL, this, func, 0, NULL, ppCM);
    }
    return E_NOINTERFACE;
}

IFACEMETHODIMP CCabFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
    if (!cidl)
    {
        const SFGAOF ThisFolder = (SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_CANLINK);
        *rgfInOut = *rgfInOut & ThisFolder;
        return S_OK;
    }
    else if (!apidl)
    {
        return E_INVALIDARG;
    }
    HRESULT hr = S_OK;
    const SFGAOF filemask = SFGAO_READONLY | SFGAO_HIDDEN | SFGAO_SYSTEM | SFGAO_ISSLOW;
    SFGAOF remain = *rgfInOut & filemask, validate = *rgfInOut & SFGAO_VALIDATE;
    CComPtr<CEnumIDList> list;
    for (UINT i = 0; i < cidl && (remain || validate); ++i)
    {
        CABITEM *item = CABITEM::Validate(apidl[i]);
        if (!item)
        {
            hr = E_INVALIDARG;
            break;
        }
        else if (validate)
        {
            if (!list && FAILED_UNEXPECTEDLY(hr = CreateEnum(&list)))
                return hr;
            if (list->FindNamedItem((PCUITEMID_CHILD)item) == -1)
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        SFGAOF att = MapFSToSFAttributes(item->GetFSAttributes()) | SFGAO_ISSLOW;
        remain &= att & ~(FLATFOLDER ? SFGAO_FOLDER : 0);
    }
    *rgfInOut = remain;
    return hr;
}

IFACEMETHODIMP CCabFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *prgfInOut, LPVOID *ppvOut)
{
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IExtractIconA || riid == IID_IExtractIconW)
    {
        if (cidl != 1)
            return E_INVALIDARG;
        CABITEM *item = CABITEM::Validate(apidl[0]);
        if (!item)
            return E_INVALIDARG;

        DWORD attr = item->GetFSAttributes();
        return SHCreateFileExtractIconW(item->GetName(), attr, riid, ppvOut);
    }
    else if (riid == IID_IContextMenu && cidl)
    {
        LPFNDFMCALLBACK func = ItemMenuCallback;
        IContextMenu **ppCM = (IContextMenu**)ppvOut;
        return CDefFolderMenu_Create2(NULL, hwndOwner, cidl, apidl, this, func, 0, NULL, ppCM);
    }
    else if (riid == IID_IDataObject && cidl)
    {
        // Note: This IDataObject is only compatible with IContextMenu, it cannot handle drag&drop of virtual items!
        return CIDLData_CreateFromIDArray(m_CurDir, cidl, apidl, (IDataObject**)ppvOut);
    }
    return hr;
}

IFACEMETHODIMP CCabFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_WINDOWCREATED:
            m_ShellViewWindow = (HWND)wParam;
            return S_OK;
        case SFVM_WINDOWCLOSING:
            m_ShellViewWindow = NULL;
            return S_OK;
    }
    return E_NOTIMPL;
}

IFACEMETHODIMP CCabFolder::GetIconOf(PCUITEMID_CHILD pidl, UINT flags, int *pIconIndex)
{
    if (CABITEM *item = CABITEM::Validate(pidl))
    {
        int index = MapPIDLToSystemImageListIndex(this, pidl, flags);
        if (index == -1 && item->IsFolder())
            index = (flags & GIL_OPENICON) ? SIID_FOLDEROPEN : SIID_FOLDER;
        if (index != -1)
        {
            *pIconIndex = index;
            return S_OK;
        }
    }
    return S_FALSE;
}

static HRESULT GetFsPathFromIDList(PCIDLIST_ABSOLUTE pidl, PWSTR pszPath)
{
    BOOL ret = SHGetPathFromIDListW(pidl, pszPath);
    if (!ret && ILIsEmpty(pidl))
        ret = SHGetSpecialFolderPathW(NULL, pszPath, CSIDL_DESKTOPDIRECTORY, TRUE);
    return ret ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

static int CALLBACK FolderBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    WCHAR buf[MAX_PATH];
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            if (LoadStringW(_AtlBaseModule.GetResourceInstance(), IDS_EXTRACT, buf, _countof(buf)))
            {
                // Remove leading and trailing dots
                WCHAR *s = buf, *e = s + lstrlenW(s);
                while (*s == '.') ++s;
                while (e > s && e[-1] == '.') *--e = UNICODE_NULL;
                SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)s);
                SendMessageW(GetDlgItem(hwnd, IDOK), WM_SETTEXT, 0, (LPARAM)s);
            }
            if (lpData)
            {
                SendMessageW(hwnd, BFFM_SETEXPANDED, FALSE, lpData);
                SendMessageW(hwnd, BFFM_SETSELECTION, FALSE, lpData);
            }
            break;
        }

        case BFFM_SELCHANGED:
        {
            SFGAOF wanted = SFGAO_FILESYSTEM | SFGAO_FOLDER, query = wanted | SFGAO_STREAM;
            PCIDLIST_ABSOLUTE pidl = (PCIDLIST_ABSOLUTE)lParam;
            BOOL enable = ILIsEmpty(pidl); // Allow the desktop
            PCUITEMID_CHILD child;
            IShellFolder *pSF;
            if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &pSF), &child)))
            {
                SFGAOF attrib = query;
                if (SUCCEEDED(pSF->GetAttributesOf(1, &child, &attrib)))
                    enable = (attrib & query) == wanted;
                pSF->Release();
            }
            if (enable)
            {
                // We don't trust .zip folders, check the file-system to make sure
                UINT attrib = SUCCEEDED(GetFsPathFromIDList(pidl, buf)) ? GetFileAttributesW(buf) : 0;
                enable = (attrib & FILE_ATTRIBUTE_DIRECTORY) && attrib != INVALID_FILE_ATTRIBUTES;
            }
            PostMessageW(hwnd, BFFM_ENABLEOK, 0, enable);
            break;
        }
    }
    return 0;
}

struct EXTRACTFILESDATA
{
    CCabFolder *pLifetimeCF;
    HWND hWndOwner;
    CIDA *pCIDA;
    STGMEDIUM cidamedium;
    IDataObject *pDO;
    IStream *pMarshalDO;
    IProgressDialog *pPD;
    UINT cabfiles, completed;
    WCHAR path[MAX_PATH], cab[MAX_PATH];
};

static HWND GetUiOwner(const EXTRACTFILESDATA &data)
{
    HWND hWnd;
    if (SUCCEEDED(IUnknown_GetWindow(data.pPD, &hWnd)) && IsWindowVisible(hWnd))
        return hWnd;
    return IsWindowVisible(data.hWndOwner) ? data.hWndOwner : NULL;
}

static HRESULT CALLBACK ExtractFilesCallback(EXTRACTCALLBACKMSG msg, const EXTRACTCALLBACKDATA &ecd, LPVOID cookie)
{
    EXTRACTFILESDATA &data = *(EXTRACTFILESDATA*)cookie;
    switch ((UINT)msg)
    {
        case ECM_BEGIN:
        {
            data.cabfiles = (UINT)(SIZE_T)ecd.pfdin->hf;
            return S_OK;
        }

        case ECM_FILE:
        {
            if (data.pPD && data.pPD->HasUserCancelled())
                return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            HRESULT hr = data.pCIDA ? S_FALSE : S_OK; // Filtering or all items?
            if (hr != S_OK)
            {
                CABITEM *needle = CreateItem(ecd.pfdin->psz1, ecd.pfdin->attribs);
                if (!needle)
                    return E_OUTOFMEMORY;
                for (UINT i = 0; i < data.pCIDA->cidl && hr == S_FALSE; ++i)
                {
                    C_ASSERT(FLATFOLDER);
                    LPCITEMIDLIST pidlChild = ILFindLastID(HIDA_GetPIDLItem(data.pCIDA, i));
                    CABITEM *haystack = CABITEM::Validate(pidlChild);
                    if (!haystack && FAILED_UNEXPECTEDLY(hr = E_FAIL))
                        break;
                    if (!lstrcmpiW(needle->Path, haystack->Path))
                    {
                        if (data.pPD)
                            data.pPD->SetLine(1, needle->Path, TRUE, NULL);
                        hr = S_OK; // Found it in the list of files to extract
                    }
                }
                SHFree(needle);
            }
            if (data.pPD)
                data.pPD->SetProgress(data.completed++, data.cabfiles);
            return hr;
        }

        case ECM_PREPAREPATH:
        {
            UINT flags = SHPPFW_DIRCREATE | SHPPFW_IGNOREFILENAME;
            return SHPathPrepareForWriteW(GetUiOwner(data), NULL, ecd.Path, flags);
        }

        case ECM_ERROR:
        {
            return ErrorBox(GetUiOwner(data), ecd.hr);
        }
    }
    return E_NOTIMPL;
}

static void Free(EXTRACTFILESDATA &data)
{
    if (data.pPD)
    {
        data.pPD->StopProgressDialog();
        data.pPD->Release();
    }
    CDataObjectHIDA::DestroyCIDA(data.pCIDA, data.cidamedium);
    IUnknown_Set((IUnknown**)&data.pDO, NULL);
    IUnknown_Set((IUnknown**)&data.pMarshalDO, NULL);
    IUnknown_Set((IUnknown**)&data.pLifetimeCF, NULL);
    SHFree(&data);
}

static DWORD CALLBACK ExtractFilesThread(LPVOID pParam)
{
    EXTRACTFILESDATA &data = *(EXTRACTFILESDATA*)pParam;
    HRESULT hr = S_OK;
    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ProgressDialog, NULL, IID_PPV_ARG(IProgressDialog, &data.pPD))))
    {
        // TODO: IActionProgress SPACTION_COPYING
        if (SUCCEEDED(data.pPD->StartProgressDialog(data.hWndOwner, NULL, PROGDLG_NOTIME, NULL)))
        {
            data.pPD->SetTitle(data.cab);
            data.pPD->SetLine(2, data.path, TRUE, NULL);
            data.pPD->SetAnimation(GetModuleHandleW(L"SHELL32"), 161);
            data.pPD->SetProgress(0, 0);
        }
    }
    if (data.pMarshalDO)
    {
        hr = CoGetInterfaceAndReleaseStream(data.pMarshalDO, IID_PPV_ARG(IDataObject, &data.pDO));
        data.pMarshalDO = NULL;
        if (SUCCEEDED(hr))
            hr = CDataObjectHIDA::CreateCIDA(data.pDO, &data.pCIDA, data.cidamedium);
    }
    if (SUCCEEDED(hr))
    {
        ExtractCabinet(data.cab, data.path, ExtractFilesCallback, &data);
    }
    Free(data);
    return 0;
}

HRESULT CCabFolder::ExtractFilesUI(HWND hWnd, IDataObject *pDO)
{
    if (!IsWindowVisible(hWnd) && IsWindowVisible(m_ShellViewWindow))
        hWnd = m_ShellViewWindow;

    EXTRACTFILESDATA *pData = (EXTRACTFILESDATA*)SHAlloc(sizeof(*pData));
    if (!pData)
        return E_OUTOFMEMORY;
    ZeroMemory(pData, sizeof(*pData));
    pData->hWndOwner = hWnd;
    pData->pLifetimeCF = this;
    pData->pLifetimeCF->AddRef();

    HRESULT hr = GetFsPathFromIDList(m_CurDir, pData->cab);
    if (SUCCEEDED(hr) && pDO)
    {
        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDO, &pData->pMarshalDO);
    }
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        LPITEMIDLIST pidlInitial = ILClone(m_CurDir);
        ILRemoveLastID(pidlInitial); // Remove the "name.cab" part (we can't extract into ourselves)
        UINT bif = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
        BROWSEINFO bi = { hWnd, NULL, NULL, pData->cab, bif, FolderBrowseCallback, (LPARAM)pidlInitial };
        if (PIDLIST_ABSOLUTE folder = SHBrowseForFolderW(&bi))
        {
            hr = GetFsPathFromIDList(folder, pData->path);
            ILFree(folder);
            if (SUCCEEDED(hr))
            {
                UINT ctf = CTF_COINIT | CTF_PROCESS_REF | CTF_THREAD_REF | CTF_FREELIBANDEXIT;
                hr = SHCreateThread(ExtractFilesThread, pData, ctf, NULL) ? S_OK : E_OUTOFMEMORY;
            }
        }
        ILFree(pidlInitial);
    }
    if (hr != S_OK)
        Free(*pData);
    return hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) ? S_OK : hr;
}
