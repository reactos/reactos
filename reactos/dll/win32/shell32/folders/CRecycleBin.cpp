/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright (C) 2009 Andrew Hill
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

#include <ntquery.h>

#define MAX_PROPERTY_SHEET_PAGE 32

WINE_DEFAULT_DEBUG_CHANNEL(CRecycleBin);

typedef struct
{
    int column_name_id;
    const GUID *fmtId;
    DWORD pid;
    int pcsFlags;
    int fmt;
    int cxChars;
} columninfo;

static const columninfo RecycleBinColumns[] =
{
    {IDS_SHV_COLUMN1,        &FMTID_Storage,   PID_STG_NAME,       SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELFROM, &FMTID_Displaced, PID_DISPLACED_FROM, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELDATE, &FMTID_Displaced, PID_DISPLACED_DATE, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN2,        &FMTID_Storage,   PID_STG_SIZE,       SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 20},
    {IDS_SHV_COLUMN3,        &FMTID_Storage,   PID_STG_STORAGETYPE, SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN4,        &FMTID_Storage,   PID_STG_WRITETIME,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
    /*    {"creation time",  &FMTID_Storage,   PID_STG_CREATETIME, SHCOLSTATE_TYPE_DATE,                        LVCFMT_LEFT,  20}, */
    /*    {"attribs",        &FMTID_Storage,   PID_STG_ATTRIBUTES, SHCOLSTATE_TYPE_STR,                         LVCFMT_LEFT,  20},       */
};

#define COLUMN_NAME    0
#define COLUMN_DELFROM 1
#define COLUMN_DATEDEL 2
#define COLUMN_SIZE    3
#define COLUMN_TYPE    4
#define COLUMN_MTIME   5

#define COLUMNS_COUNT  6

/*
 * Recycle Bin folder
 */

class CRecycleBinEnum :
    public CEnumIDListBase
{
    private:
    public:
        CRecycleBinEnum();
        ~CRecycleBinEnum();
        HRESULT WINAPI Initialize(DWORD dwFlags);
        static BOOL WINAPI CBEnumRecycleBin(IN PVOID Context, IN HANDLE hDeletedFile);
        BOOL WINAPI CBEnumRecycleBin(IN HANDLE hDeletedFile);

        BEGIN_COM_MAP(CRecycleBinEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

class CRecycleBinItemContextMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2
{
    private:
        LPITEMIDLIST                        apidl;
    public:
        CRecycleBinItemContextMenu();
        ~CRecycleBinItemContextMenu();
        HRESULT WINAPI Initialize(LPCITEMIDLIST pidl);

        // IContextMenu
        virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

        // IContextMenu2
        virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

        BEGIN_COM_MAP(CRecycleBinItemContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        END_COM_MAP()
};

typedef struct
{
    PIDLRecycleStruct *pFileDetails;
    HANDLE hDeletedFile;
    BOOL bFound;
} SEARCH_CONTEXT, *PSEARCH_CONTEXT;

typedef struct
{
    DWORD dwNukeOnDelete;
    DWORD dwSerial;
    DWORD dwMaxCapacity;
} DRIVE_ITEM_CONTEXT, *PDRIVE_ITEM_CONTEXT;

BOOL WINAPI CBSearchRecycleBin(IN PVOID Context, IN HANDLE hDeletedFile)
{
    PSEARCH_CONTEXT pContext = (PSEARCH_CONTEXT)Context;

    PDELETED_FILE_DETAILS_W pFileDetails;
    DWORD dwSize;
    BOOL ret;

    if (!GetDeletedFileDetailsW(hDeletedFile,
                                0,
                                NULL,
                                &dwSize) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("GetDeletedFileDetailsW failed\n");
        return FALSE;
    }

    pFileDetails = (DELETED_FILE_DETAILS_W *)SHAlloc(dwSize);
    if (!pFileDetails)
    {
        ERR("No memory\n");
        return FALSE;
    }

    if (!GetDeletedFileDetailsW(hDeletedFile,
                                dwSize,
                                pFileDetails,
                                NULL))
    {
        ERR("GetDeletedFileDetailsW failed\n");
        SHFree(pFileDetails);
        return FALSE;
    }

    ret = memcmp(pFileDetails, pContext->pFileDetails, dwSize);
    if (!ret)
    {
        pContext->hDeletedFile = hDeletedFile;
        pContext->bFound = TRUE;
    }
    else
        CloseRecycleBinHandle(hDeletedFile);

    SHFree(pFileDetails);
    return ret;
}

static PIDLRecycleStruct * _ILGetRecycleStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type == 0x00)
        return (PIDLRecycleStruct*) & (pdata->u.crecycle);

    return NULL;
}

CRecycleBinEnum::CRecycleBinEnum()
{
}

CRecycleBinEnum::~CRecycleBinEnum()
{
}

HRESULT WINAPI CRecycleBinEnum::Initialize(DWORD dwFlags)
{
    static LPCWSTR szDrive = L"C:\\";

    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        TRACE("Starting Enumeration\n");

        if (!EnumerateRecycleBinW(szDrive /* FIXME */ , CBEnumRecycleBin, (PVOID)this))
        {
            WARN("Error: EnumerateCRecycleBinW failed\n");
            return E_FAIL;
        }
    }
    else
    {
        // do nothing
    }
    return S_OK;
}

static LPITEMIDLIST _ILCreateRecycleItem(PDELETED_FILE_DETAILS_W pFileDetails)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLRecycleStruct * p;
    int size0 = (char*)&tmp.u.crecycle.szName - (char*)&tmp.u.crecycle;
    int size = size0;

    tmp.type = 0x00;
    size += (wcslen(pFileDetails->FileName) + 1) * sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size + 2;
    memcpy(pidl->mkid.abID, &tmp, 2 + size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.crecycle;
    RtlCopyMemory(p, pFileDetails, sizeof(DELETED_FILE_DETAILS_W));
    wcscpy(p->szName, pFileDetails->FileName);
    *(WORD*)((char*)pidl + (size + 2)) = 0;
    return pidl;
}

BOOL WINAPI CRecycleBinEnum::CBEnumRecycleBin(IN PVOID Context, IN HANDLE hDeletedFile)
{
    return static_cast<CRecycleBinEnum *>(Context)->CBEnumRecycleBin(hDeletedFile);
}

BOOL WINAPI CRecycleBinEnum::CBEnumRecycleBin(IN HANDLE hDeletedFile)
{
    PDELETED_FILE_DETAILS_W pFileDetails;
    DWORD dwSize;
    LPITEMIDLIST pidl = NULL;
    BOOL ret;

    if (!GetDeletedFileDetailsW(hDeletedFile,
                                0,
                                NULL,
                                &dwSize) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("GetDeletedFileDetailsW failed\n");
        return FALSE;
    }

    pFileDetails = (DELETED_FILE_DETAILS_W *)SHAlloc(dwSize);
    if (!pFileDetails)
    {
        ERR("No memory\n");
        return FALSE;
    }

    if (!GetDeletedFileDetailsW(hDeletedFile,
                                dwSize,
                                pFileDetails,
                                NULL))
    {
        ERR("GetDeletedFileDetailsW failed\n");
        SHFree(pFileDetails);
        return FALSE;
    }

    pidl = _ILCreateRecycleItem(pFileDetails);
    if (!pidl)
    {
        SHFree(pFileDetails);
        return FALSE;
    }

    ret = AddToEnumList(pidl);

    if (!ret)
        SHFree(pidl);
    SHFree(pFileDetails);
    TRACE("Returning %d\n", ret);
    CloseRecycleBinHandle(hDeletedFile);
    return ret;
}

/**************************************************************************
* IContextMenu2 Bitbucket Item Implementation
*/

CRecycleBinItemContextMenu::CRecycleBinItemContextMenu()
{
    apidl = NULL;
}

CRecycleBinItemContextMenu::~CRecycleBinItemContextMenu()
{
    ILFree(apidl);
}

HRESULT WINAPI CRecycleBinItemContextMenu::Initialize(LPCITEMIDLIST pidl)
{
    apidl = ILClone(pidl);
    if (apidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT WINAPI CRecycleBinItemContextMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    WCHAR szBuffer[30] = {0};
    ULONG Count = 1;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n", this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (LoadStringW(shell32_hInstance, IDS_RESTORE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
        Count++;
    }

    if (LoadStringW(shell32_hInstance, IDS_CUT, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_DELETE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_PROPERTIES, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_DEFAULT);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}

HRESULT WINAPI CRecycleBinItemContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    SEARCH_CONTEXT Context;
    static LPCWSTR szDrive = L"C:\\";

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", this, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(1) || lpcmi->lpVerb == MAKEINTRESOURCEA(5))
    {
        Context.pFileDetails = _ILGetRecycleStruct(apidl);
        Context.bFound = FALSE;

        EnumerateRecycleBinW(szDrive, CBSearchRecycleBin, (PVOID)&Context);
        if (!Context.bFound)
            return E_FAIL;

        if (lpcmi->lpVerb == MAKEINTRESOURCEA(1))
        {
            /* restore file */
            if (RestoreFile(Context.hDeletedFile))
                return S_OK;
            else
                return E_FAIL;
        }
        else
        {
            DeleteFileHandleToRecycleBin(Context.hDeletedFile);
            return E_NOTIMPL;
        }
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(3))
    {
        FIXME("implement cut\n");
        return E_NOTIMPL;
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(7))
    {
        FIXME("implement properties\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT WINAPI CRecycleBinItemContextMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    return E_FAIL;
}

HRESULT WINAPI CRecycleBinItemContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("CRecycleBin_IContextMenu2Item_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n", this, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

/**************************************************************************
* registers clipboardformat once
*/
void CRecycleBin::SF_RegisterClipFmt()
{
    TRACE ("(%p)\n", this);

    if (!cfShellIDList)
        cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
}

CRecycleBin::CRecycleBin()
{
    pidl = NULL;
    iIdEmpty = 0;
    cfShellIDList = 0;
    SF_RegisterClipFmt();
    fAcceptFmt = FALSE;
}

CRecycleBin::~CRecycleBin()
{
    /*    InterlockedDecrement(&objCount);*/
    SHFree(pidl);
}

/*************************************************************************
 * RecycleBin IPersistFolder2 interface
 */

HRESULT WINAPI CRecycleBin::GetClassID(CLSID *pClassID)
{
    TRACE("(%p, %p)\n", this, pClassID);
    if (pClassID == NULL)
        return E_INVALIDARG;
    memcpy(pClassID, &CLSID_RecycleBin, sizeof(CLSID));
    return S_OK;
}

HRESULT WINAPI CRecycleBin::Initialize(LPCITEMIDLIST pidl)
{
    TRACE("(%p, %p)\n", this, pidl);

    SHFree((LPVOID)this->pidl);
    this->pidl = ILClone(pidl);
    if (this->pidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetCurFolder(LPITEMIDLIST *ppidl)
{
    TRACE("\n");
    *ppidl = ILClone(pidl);
    return S_OK;
}

/*************************************************************************
 * RecycleBin IShellFolder2 interface
 */

HRESULT WINAPI CRecycleBin::ParseDisplayName(HWND hwnd, LPBC pbc,
        LPOLESTR pszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl,
        ULONG *pdwAttributes)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}


PDELETED_FILE_DETAILS_W
UnpackDetailsFromPidl(LPCITEMIDLIST pidl)
{
    return (PDELETED_FILE_DETAILS_W)&pidl->mkid.abID;
}

HRESULT WINAPI CRecycleBin::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CRecycleBinEnum>(dwFlags, IID_IEnumIDList, ppEnumIDList);
}

HRESULT WINAPI CRecycleBin::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", this, pidl, pbc, debugstr_guid(&riid), ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", this, pidl, pbc, debugstr_guid(&riid), ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    /* TODO */
    TRACE("(%p, %p, %p, %p)\n", this, (void *)lParam, pidl1, pidl2);
    if (pidl1->mkid.cb != pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, pidl1->mkid.cb - pidl2->mkid.cb);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (unsigned short)memcmp(pidl1->mkid.abID, pidl2->mkid.abID, pidl1->mkid.cb));
}

HRESULT WINAPI CRecycleBin::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_NOINTERFACE;

    TRACE("(%p, %p, %s, %p)\n", this, hwndOwner, debugstr_guid(&riid), ppv);

    if (!ppv)
        return hr;

    *ppv = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        hr = this->QueryInterface (IID_IDropTarget, ppv);
    }
    else if (IsEqualIID (riid, IID_IContextMenu) || IsEqualIID (riid, IID_IContextMenu2))
    {
        hr = this->QueryInterface(riid, ppv);
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        hr = IShellView_Constructor ((IShellFolder *)this, &pShellView);
        if (pShellView)
        {
            hr = pShellView->QueryInterface(riid, ppv);
        }
    }
    else
        return hr;
    TRACE ("-- (%p)->(interface=%p)\n", this, ppv);
    return hr;

}

HRESULT WINAPI CRecycleBin::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut)
{
    TRACE("(%p, %d, {%p, ...}, {%x})\n", this, cidl, apidl ? apidl[0] : NULL, (unsigned int)*rgfInOut);
    *rgfInOut &= SFGAO_FOLDER|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT *prgfInOut, void **ppv)
{
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p, %p %p)\n", this,
           hwndOwner, cidl, apidl, prgfInOut, ppv);

    if (!ppv)
        return hr;

    *ppv = NULL;

    if ((IsEqualIID (riid, IID_IContextMenu) || IsEqualIID(riid, IID_IContextMenu2)) && (cidl >= 1))
    {
        hr = ShellObjectCreatorInit<CRecycleBinItemContextMenu>(apidl[0], riid, &pObj);
    }
    else if (IsEqualIID (riid, IID_IDropTarget) && (cidl == 1))
    {
        IDropTarget * pDt = NULL;
        hr = QueryInterface(IID_PPV_ARG(IDropTarget, &pDt));
        pObj = pDt;
    }
    else if(IsEqualIID(riid, IID_IExtractIconA) && (cidl == 1))
    {
        LPITEMIDLIST pidlItem = ILCombine(pidl, apidl[0]);
        pObj = IExtractIconA_Constructor(pidlItem);
        SHFree(pidlItem);
        hr = S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtractIconW) && (cidl == 1))
    {
        LPITEMIDLIST pidlItem = ILCombine(pidl, apidl[0]);
        pObj = IExtractIconW_Constructor(pidlItem);
        SHFree(pidlItem);
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppv = pObj;
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

HRESULT WINAPI CRecycleBin::GetDisplayNameOf(PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName)
{
    PIDLRecycleStruct *pFileDetails;
    LPWSTR pFileName;

    TRACE("(%p, %p, %x, %p)\n", this, pidl, (unsigned int)uFlags, pName);


    if (_ILIsBitBucket (pidl))
    {
        WCHAR pszPath[100];

        if (HCR_GetClassNameW(CLSID_RecycleBin, pszPath, MAX_PATH))
        {
            pName->uType = STRRET_WSTR;
            pName->pOleStr = StrDupW(pszPath);
            return S_OK;
        }
    }

    pFileDetails = _ILGetRecycleStruct(pidl);
    if (!pFileDetails)
    {
        pName->cStr[0] = 0;
        pName->uType = STRRET_CSTR;
        return E_INVALIDARG;
    }

    pFileName = wcsrchr(pFileDetails->szName, L'\\');
    if (!pFileName)
    {
        pName->cStr[0] = 0;
        pName->uType = STRRET_CSTR;
        return E_UNEXPECTED;
    }

    pName->pOleStr = StrDupW(pFileName + 1);
    if (pName->pOleStr == NULL)
        return E_OUTOFMEMORY;

    pName->uType = STRRET_WSTR;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pszName,
                                      SHGDNF uFlags, PITEMID_CHILD *ppidlOut)
{
    TRACE("\n");
    return E_FAIL; /* not supported */
}

HRESULT WINAPI CRecycleBin::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::EnumSearches(IEnumExtraSearch **ppEnum)
{
    FIXME("stub\n");
    *ppEnum = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p, %x, %p, %p)\n", this, (unsigned int)dwReserved, pSort, pDisplay);
    *pSort = 0;
    *pDisplay = 0;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    TRACE("(%p, %d, %p)\n", this, iColumn, pcsFlags);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    *pcsFlags = RecycleBinColumns[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT FormatDateTime(LPWSTR buffer, int size, FILETIME * ft)
{
    FILETIME lft;
    SYSTEMTIME time;
    int ret;

    FileTimeToLocalFileTime(ft, &lft);
    FileTimeToSystemTime(&lft, &time);

    ret = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, buffer, size);
    if (ret > 0 && ret < size)
    {
        /* Append space + time without seconds */
        buffer[ret-1] = ' ';
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, &buffer[ret], size - ret);
    }

    return (ret != 0 ? E_FAIL : S_OK);
}

HRESULT WINAPI CRecycleBin::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    PIDLRecycleStruct * pFileDetails;
    WCHAR buffer[MAX_PATH];
    WCHAR szTypeName[100];
    LPWSTR pszBackslash;
    UINT Length;

    TRACE("(%p, %p, %d, %p)\n", this, pidl, iColumn, pDetails);
    if (iColumn >= COLUMNS_COUNT)
        return E_FAIL;
    pDetails->fmt = RecycleBinColumns[iColumn].fmt;
    pDetails->cxChar = RecycleBinColumns[iColumn].cxChars;
    if (pidl == NULL)
    {
        pDetails->str.uType = STRRET_WSTR;
        LoadStringW(shell32_hInstance, RecycleBinColumns[iColumn].column_name_id, buffer, MAX_PATH);
        return SHStrDupW(buffer, &pDetails->str.pOleStr);
    }

    if (iColumn == COLUMN_NAME)
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &pDetails->str);

    pFileDetails = _ILGetRecycleStruct(pidl);
    switch (iColumn)
    {
        case COLUMN_DATEDEL:
            FormatDateTime(buffer, MAX_PATH, &pFileDetails->DeletionTime);
            break;
        case COLUMN_DELFROM:
            pszBackslash = wcsrchr(pFileDetails->szName, L'\\');
            Length = (pszBackslash - pFileDetails->szName);
            memcpy((LPVOID)buffer, pFileDetails->szName, Length * sizeof(WCHAR));
            buffer[Length] = L'\0';
            break;
        case COLUMN_SIZE:
            StrFormatKBSizeW(pFileDetails->FileSize.QuadPart, buffer, MAX_PATH);
            break;
        case COLUMN_MTIME:
            FormatDateTime(buffer, MAX_PATH, &pFileDetails->LastModification);
            break;
        case COLUMN_TYPE:
            szTypeName[0] = L'\0';
            wcscpy(buffer, PathFindExtensionW(pFileDetails->szName));
            if (!( HCR_MapTypeToValueW(buffer, buffer, sizeof(buffer) / sizeof(WCHAR), TRUE) &&
                    HCR_MapTypeToValueW(buffer, szTypeName, sizeof(szTypeName) / sizeof(WCHAR), FALSE )))
            {
                wcscpy (szTypeName, PathFindExtensionW(pFileDetails->szName));
                wcscat(szTypeName, L"-");
                Length = wcslen(szTypeName);
                if (LoadStringW(shell32_hInstance, IDS_SHV_COLUMN1, &szTypeName[Length], (sizeof(szTypeName) / sizeof(WCHAR)) - Length))
                    szTypeName[(sizeof(szTypeName)/sizeof(WCHAR))-1] = L'\0';
            }
            pDetails->str.uType = STRRET_WSTR;
            return SHStrDupW(szTypeName, &pDetails->str.pOleStr);
            break;
        default:
            return E_FAIL;
    }

    pDetails->str.uType = STRRET_WSTR;
    return SHStrDupW(buffer, &pDetails->str.pOleStr);
}

HRESULT WINAPI CRecycleBin::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    TRACE("(%p, %d, %p)\n", this, iColumn, pscid);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    pscid->fmtid = *RecycleBinColumns[iColumn].fmtId;
    pscid->pid = RecycleBinColumns[iColumn].pid;
    return S_OK;
}

/*************************************************************************
 * RecycleBin IContextMenu interface
 */

HRESULT WINAPI CRecycleBin::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    WCHAR szBuffer[100];
    MENUITEMINFOW mii;
    int id = 1;

    TRACE("QueryContextMenu %p %p %u %u %u %u\n", this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if (!hMenu)
        return E_INVALIDARG;

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fState = MFS_ENABLED;
    szBuffer[0] = L'\0';
    LoadStringW(shell32_hInstance, IDS_EMPTY_BITBUCKET, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    mii.dwTypeData = szBuffer;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdFirst + id++;
    mii.fType = MFT_STRING;
    iIdEmpty = 1;

    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, id);
}

HRESULT WINAPI CRecycleBin::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr;
    LPSHELLBROWSER lpSB;
    IShellView * lpSV = NULL;

    TRACE("%p %p verb %p\n", this, lpcmi, lpcmi->lpVerb);

    if (LOWORD(lpcmi->lpVerb) == iIdEmpty)
    {
        // FIXME
        // path & flags
        hr = SHEmptyRecycleBinW(lpcmi->hwnd, L"C:\\", 0);
        TRACE("result %x\n", hr);
        if (hr != S_OK)
            return hr;

        lpSB = (LPSHELLBROWSER)SendMessageA(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
        if (lpSB && SUCCEEDED(lpSB->QueryActiveShellView(&lpSV)))
            lpSV->Refresh();
    }
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    FIXME("%p %lu %u %p %p %u\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    return E_NOTIMPL;
}

/*************************************************************************
 * RecycleBin IShellPropSheetExt interface
 */

HRESULT WINAPI CRecycleBin::AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    FIXME("%p %p %lu\n", this, pfnAddPage, lParam);

    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::ReplacePage(EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    FIXME("%p %lu %p %lu\n", this, uPageID, pfnReplaceWith, lParam);

    return E_NOTIMPL;
}

/*************************************************************************
 * RecycleBin IShellExtInit interface
 */

HRESULT WINAPI CRecycleBin::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    TRACE("%p %p %p %p\n", this, pidlFolder, pdtobj, hkeyProgID );
    return S_OK;
}

void toggleNukeOnDeleteOption(HWND hwndDlg, BOOL bEnable)
{
    if (bEnable)
    {
        SendDlgItemMessage(hwndDlg, 14001, BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, 14002), FALSE);
        SendDlgItemMessage(hwndDlg, 14003, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendDlgItemMessage(hwndDlg, 14001, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, 14002), TRUE);
        SendDlgItemMessage(hwndDlg, 14003, BM_SETCHECK, BST_UNCHECKED, 0);
    }
}


static VOID
InitializeRecycleBinDlg(HWND hwndDlg, WCHAR DefaultDrive)
{
    WCHAR CurDrive = L'A';
    WCHAR szDrive[] = L"A:\\";
    DWORD dwDrives;
    WCHAR szName[100];
    WCHAR szVolume[100];
    DWORD MaxComponent, Flags;
    DWORD dwSerial;
    LVCOLUMNW lc;
    HWND hDlgCtrl;
    LVITEMW li;
    INT itemCount;
    ULARGE_INTEGER TotalNumberOfFreeBytes, TotalNumberOfBytes, FreeBytesAvailable;
    RECT rect;
    int columnSize;
    int defIndex = 0;
    DWORD dwSize;
    PDRIVE_ITEM_CONTEXT pItem = NULL, pDefault = NULL, pFirst = NULL;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_LOCATION, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
        szVolume[0] = 0;

    GetClientRect(hDlgCtrl, &rect);

    memset(&lc, 0, sizeof(LV_COLUMN) );
    lc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;

    columnSize = 140; //FIXME
    lc.iSubItem   = 0;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    lc.cx         = columnSize;
    lc.cchTextMax = wcslen(szVolume);
    lc.pszText    = szVolume;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&lc);

    if (!LoadStringW(shell32_hInstance, IDS_RECYCLEBIN_DISKSPACE, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
        szVolume[0] = 0;

    lc.iSubItem   = 1;
    lc.cx         = rect.right - rect.left - columnSize;
    lc.cchTextMax = wcslen(szVolume);
    lc.pszText    = szVolume;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 1, (LPARAM)&lc);

    dwDrives = GetLogicalDrives();
    itemCount = 0;
    do
    {
        if ((dwDrives & 0x1))
        {
            UINT Type = GetDriveTypeW(szDrive);
            if (Type == DRIVE_FIXED) //FIXME
            {
                if (!GetVolumeInformationW(szDrive, szName, sizeof(szName) / sizeof(WCHAR), &dwSerial, &MaxComponent, &Flags, NULL, 0))
                {
                    szName[0] = 0;
                    dwSerial = -1;
                }

                swprintf(szVolume, L"%s (%c)", szName, szDrive[0]);
                memset(&li, 0x0, sizeof(LVITEMW));
                li.mask = LVIF_TEXT | LVIF_PARAM;
                li.iSubItem = 0;
                li.pszText = szVolume;
                li.iItem = itemCount;
                SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
                if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable , &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
                {
                    if (StrFormatByteSizeW(TotalNumberOfFreeBytes.QuadPart, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
                    {

                        pItem = (DRIVE_ITEM_CONTEXT *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRIVE_ITEM_CONTEXT));
                        if (pItem)
                        {
                            swprintf(szName, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume\\%04X-%04X", LOWORD(dwSerial), HIWORD(dwSerial));
                            dwSize = sizeof(DWORD);
                            RegGetValueW(HKEY_CURRENT_USER, szName, L"MaxCapacity", RRF_RT_DWORD, NULL, &pItem->dwMaxCapacity, &dwSize);
                            dwSize = sizeof(DWORD);
                            RegGetValueW(HKEY_CURRENT_USER, szName, L"NukeOnDelete", RRF_RT_DWORD, NULL, &pItem->dwNukeOnDelete, &dwSize);
                            pItem->dwSerial = dwSerial;
                            li.mask = LVIF_PARAM;
                            li.lParam = (LPARAM)pItem;
                            (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                            if (CurDrive == DefaultDrive)
                            {
                                defIndex = itemCount;
                                pDefault = pItem;
                            }
                        }
                        if (!pFirst)
                            pFirst = pItem;

                        li.mask = LVIF_TEXT;
                        li.iSubItem = 1;
                        li.pszText = szVolume;
                        li.iItem = itemCount;
                        (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
                    }
                }
                itemCount++;
            }
        }
        CurDrive++;
        szDrive[0] = CurDrive;
        dwDrives = (dwDrives >> 1);
    } while(dwDrives);

    if (!pDefault)
        pDefault = pFirst;
    if (pDefault)
    {
        toggleNukeOnDeleteOption(hwndDlg, pDefault->dwNukeOnDelete);
        SetDlgItemInt(hwndDlg, 14002, pDefault->dwMaxCapacity, FALSE);
    }
    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_STATE;
    li.stateMask = (UINT) - 1;
    li.state = LVIS_FOCUSED | LVIS_SELECTED;
    li.iItem = defIndex;
    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);

}

static BOOL StoreDriveSettings(HWND hwndDlg)
{
    int iCount, iIndex;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    LVITEMW li;
    PDRIVE_ITEM_CONTEXT pItem;
    HKEY hKey, hSubKey;
    WCHAR szSerial[20];
    DWORD dwSize;


    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return FALSE;

    iCount = ListView_GetItemCount(hDlgCtrl);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM;

    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
            swprintf(szSerial, L"%04X-%04X", LOWORD(pItem->dwSerial), HIWORD(pItem->dwSerial));
            if (RegCreateKeyExW(hKey, szSerial, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
            {
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&pItem->dwNukeOnDelete, dwSize);
                dwSize = sizeof(DWORD);
                RegSetValueExW(hSubKey, L"MaxCapacity", 0, REG_DWORD, (LPBYTE)&pItem->dwMaxCapacity, dwSize);
                RegCloseKey(hSubKey);
            }
        }
    }
    RegCloseKey(hKey);
    return TRUE;

}

static VOID FreeDriveItemContext(HWND hwndDlg)
{
    int iCount, iIndex;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    LVITEMW li;

    iCount = ListView_GetItemCount(hDlgCtrl);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_PARAM;

    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        li.iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&li))
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)li.lParam);
        }
    }
}

static INT
GetDefaultItem(HWND hwndDlg, LVITEMW * li)
{
    HWND hDlgCtrl;
    UINT iItemCount, iIndex;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    if (!hDlgCtrl)
        return -1;

    iItemCount = ListView_GetItemCount(hDlgCtrl);
    if (!iItemCount)
        return -1;

    ZeroMemory(li, sizeof(LVITEMW));
    li->mask = LVIF_PARAM | LVIF_STATE;
    li->stateMask = (UINT) - 1;
    for (iIndex = 0; iIndex < iItemCount; iIndex++)
    {
        li->iItem = iIndex;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)li))
        {
            if (li->state & LVIS_SELECTED)
                return iIndex;
        }
    }
    return -1;

}

static INT_PTR CALLBACK
RecycleBinDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPSHNOTIFY lppsn;
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PDRIVE_ITEM_CONTEXT pItem;
    BOOL bSuccess;
    UINT uResult;
    PROPSHEETPAGE * page;
    DWORD dwStyle;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            InitializeRecycleBinDlg(hwndDlg, (WCHAR)page->lParam);
            dwStyle = (DWORD) SendDlgItemMessage(hwndDlg, 14000, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
            SendDlgItemMessage(hwndDlg, 14000, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
            if (GetDlgCtrlID((HWND)wParam) != 14000)
            {
                SetFocus(GetDlgItem(hwndDlg, 14000));
                return FALSE;
            }
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14001:
                    toggleNukeOnDeleteOption(hwndDlg, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                case 14003:
                    toggleNukeOnDeleteOption(hwndDlg, TRUE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                case 14004:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;
        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            lppl = (LPNMLISTVIEW) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                if (GetDefaultItem(hwndDlg, &li) > -1)
                {
                    pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
                    if (pItem)
                    {
                        uResult = GetDlgItemInt(hwndDlg, 14002, &bSuccess, FALSE);
                        if (bSuccess)
                            pItem->dwMaxCapacity = uResult;
                        if (SendDlgItemMessageW(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            pItem->dwNukeOnDelete = TRUE;
                        else
                            pItem->dwNukeOnDelete = FALSE;
                    }
                }
                if (StoreDriveSettings(hwndDlg))
                {
                    SetWindowLongPtr( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            else if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                ZeroMemory(&li, sizeof(li));
                li.mask = LVIF_PARAM;
                li.iItem = lppl->iItem;
                if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&li))
                    return TRUE;

                pItem = (PDRIVE_ITEM_CONTEXT)li.lParam;
                if (!pItem)
                    return TRUE;

                if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                {
                    /* new focused item */
                    toggleNukeOnDeleteOption(lppl->hdr.hwndFrom, pItem->dwNukeOnDelete);
                    SetDlgItemInt(hwndDlg, 14002, pItem->dwMaxCapacity, FALSE);
                }
                else if ((lppl->uOldState & LVIS_FOCUSED) && !(lppl->uNewState & LVIS_FOCUSED))
                {
                    /* kill focus */
                    uResult = GetDlgItemInt(hwndDlg, 14002, &bSuccess, FALSE);
                    if (bSuccess)
                        pItem->dwMaxCapacity = uResult;
                    if (SendDlgItemMessageW(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        pItem->dwNukeOnDelete = TRUE;
                    else
                        pItem->dwNukeOnDelete = FALSE;
                }
                return TRUE;

            }
            break;
        case WM_DESTROY:
            FreeDriveItemContext(hwndDlg);
            break;
    }
    return FALSE;
}

BOOL SH_ShowRecycleBinProperties(WCHAR sDrive)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE hprop;

    BOOL ret;


    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSP_DEFAULT | PSH_PROPTITLE;
    psh.pszCaption = MAKEINTRESOURCEW(IDS_RECYCLEBIN_FOLDER_NAME);
    psh.hwndParent = NULL;
    psh.phpage = hpsp;
    psh.hInstance = shell32_hInstance;

    hprop = SH_CreatePropertySheetPage(IDD_RECYCLE_BIN_PROPERTIES, RecycleBinDlg, (LPARAM)sDrive, NULL);
    if (!hprop)
    {
        ERR("Failed to create property sheet\n");
        return FALSE;
    }
    hpsp[psh.nPages] = hprop;
    psh.nPages++;


    ret = PropertySheetW(&psh);
    if (ret < 0)
        return FALSE;
    else
        return TRUE;
}

BOOL
TRASH_CanTrashFile(LPCWSTR wszPath)
{
    LONG ret;
    DWORD dwNukeOnDelete, dwType, VolSerialNumber, MaxComponentLength;
    DWORD FileSystemFlags, dwSize, dwDisposition;
    HKEY hKey;
    WCHAR szBuffer[10];
    WCHAR szKey[150] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume\\";

    if (wszPath[1] != L':')
    {
        /* path is UNC */
        return FALSE;
    }

    if (GetDriveTypeW(wszPath) != DRIVE_FIXED)
    {
        /* no bitbucket on removable media */
        return FALSE;
    }

    if (!GetVolumeInformationW(wszPath, NULL, 0, &VolSerialNumber, &MaxComponentLength, &FileSystemFlags, NULL, 0))
    {
        ERR("GetVolumeInformationW failed with %u\n", GetLastError());
        return FALSE;
    }

    swprintf(szBuffer, L"%04X-%04X", LOWORD(VolSerialNumber), HIWORD(VolSerialNumber));
    wcscat(szKey, szBuffer);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW failed\n");
        return FALSE;
    }

    if (dwDisposition  & REG_CREATED_NEW_KEY)
    {
        /* per default move to bitbucket */
        dwNukeOnDelete = 0;
        RegSetValueExW(hKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&dwNukeOnDelete, sizeof(DWORD));
        /* per default unlimited size */
        dwSize = -1;
        RegSetValueExW(hKey, L"MaxCapacity", 0, REG_DWORD, (LPBYTE)&dwSize, sizeof(DWORD));
        RegCloseKey(hKey);
        return TRUE;
    }
    else
    {
        dwSize = sizeof(dwNukeOnDelete);
        ret = RegQueryValueExW(hKey, L"NukeOnDelete", NULL, &dwType, (LPBYTE)&dwNukeOnDelete, &dwSize);
        if (ret != ERROR_SUCCESS)
        {
            if (ret ==  ERROR_FILE_NOT_FOUND)
            {
                /* restore key and enable bitbucket */
                dwNukeOnDelete = 0;
                RegSetValueExW(hKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&dwNukeOnDelete, sizeof(DWORD));
            }
            RegCloseKey(hKey);
            return TRUE;
        }
        else if (dwNukeOnDelete)
        {
            /* do not delete to bitbucket */
            RegCloseKey(hKey);
            return FALSE;
        }
        /* FIXME
         * check if bitbucket is full
         */
        RegCloseKey(hKey);
        return TRUE;
    }
}

BOOL
TRASH_TrashFile(LPCWSTR wszPath)
{
    TRACE("(%s)\n", debugstr_w(wszPath));
    return DeleteFileToRecycleBin(wszPath);
}

/*************************************************************************
 * SHUpdateCRecycleBinIcon                                [SHELL32.@]
 *
 * Undocumented
 */
EXTERN_C HRESULT WINAPI SHUpdateRecycleBinIcon(void)
{
    FIXME("stub\n");



    return S_OK;
}

/****************************************************************************
 * IDropTarget implementation
 */
BOOL CRecycleBin::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    /* TODO on shift we should delete, we should update the cursor manager to show this. */

    DWORD dwEffect = DROPEFFECT_COPY;

    *pdwEffect = DROPEFFECT_NONE;

    if (fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);

        if (*pdwEffect == DROPEFFECT_NONE)
            *pdwEffect = dwEffect;

        /* ... matches the desired effect ? */
        if (dwEffect & *pdwEffect) {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT WINAPI CRecycleBin::DragEnter(IDataObject *pDataObject,
                                    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("Recycle bin drag over (%p)\n", this);
    /* The recycle bin accepts pretty much everything, and sets a CSIDL flag. */
    fAcceptFmt = TRUE;

    QueryDrop(dwKeyState, pdwEffect);
    return S_OK;
}

HRESULT WINAPI CRecycleBin::DragOver(DWORD dwKeyState, POINTL pt,
                                   DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pdwEffect)
        return E_INVALIDARG;

    QueryDrop(dwKeyState, pdwEffect);

    return S_OK;
}

HRESULT WINAPI CRecycleBin::DragLeave()
{
    TRACE("(%p)\n", this);

    fAcceptFmt = FALSE;

    return S_OK;
}

HRESULT WINAPI CRecycleBin::Drop(IDataObject *pDataObject,
                               DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p) object dropped on recycle bin, effect %u\n", this, *pdwEffect);
    
    /* TODO: pdwEffect should be read and make the drop object be permanently deleted in the move case (shift held) */

    FORMATETC fmt;
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);

    /* Handle cfShellIDList Drop objects here, otherwise send the approriate message to other software */
    if (SUCCEEDED(pDataObject->QueryGetData(&fmt))) {
        IStream *s;
        CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObject, &s);
        SHCreateThread(DoDeleteThreadProc, s, NULL, NULL);
    }
    else
    {
        /* 
         * TODO call SetData on the data object with format CFSTR_TARGETCLSID
         * set to the Recycle Bin's class identifier CLSID_RecycleBin.
         */
    }
    return S_OK;
}

DWORD WINAPI DoDeleteThreadProc(LPVOID lpParameter) 
{
    CoInitialize(NULL);
    CComPtr<IDataObject> pDataObject;
    HRESULT hr = CoGetInterfaceAndReleaseStream (static_cast<IStream*>(lpParameter), IID_PPV_ARG(IDataObject, &pDataObject));
    if (SUCCEEDED(hr))
    {
        DoDeleteDataObject(pDataObject);
    }
    CoUninitialize();
    return 0;
}

HRESULT WINAPI DoDeleteDataObject(IDataObject *pda) 
{
    TRACE("performing delete");
    HRESULT hr;

    STGMEDIUM medium;
    FORMATETC formatetc;
    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    hr = pda->GetData(&formatetc, &medium);
    if (FAILED(hr))
        return hr;

    /* lock the handle */
    LPIDA lpcida = (LPIDA)GlobalLock(medium.hGlobal);
    if (!lpcida)
    {
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    /* convert the data into pidl */
    LPITEMIDLIST pidl;
    LPITEMIDLIST *apidl = _ILCopyCidaToaPidl(&pidl, lpcida);
    if (!apidl)
    {
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IShellFolder> psfFrom = NULL;

    /* Grab the desktop shell folder */
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
    {
        ERR("SHGetDesktopFolder failed\n");
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    /* Find source folder, this is where the clipboard data was copied from */
    if (_ILIsDesktop(pidl))
    {
        psfFrom = psfDesktop;
    }
    else 
    {
        hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfFrom));
        if (FAILED(hr))
        {
            ERR("no IShellFolder\n");
            SHFree(pidl);
            _ILFreeaPidl(apidl, lpcida->cidl);
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }
    }

    STRRET strTemp;
    hr = psfFrom->GetDisplayNameOf(apidl[0], SHGDN_FORPARSING, &strTemp);
    if (FAILED(hr))
    {
        ERR("IShellFolder_GetDisplayNameOf failed with %x\n", hr);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        return hr;
    }

    WCHAR wszPath[MAX_PATH];
    hr = StrRetToBufW(&strTemp, apidl[0], wszPath, _countof(wszPath));
    if (FAILED(hr))
    {
        ERR("StrRetToBufW failed with %x\n", hr);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        return hr;
    }

    /* Only keep the base path */
    LPWSTR pwszFilename = PathFindFileNameW(wszPath);
    *pwszFilename = L'\0';

    /* Build paths list */
    LPWSTR pwszPaths = BuildPathsList(wszPath, lpcida->cidl, (LPCITEMIDLIST*) apidl);
    if (!pwszPaths)
    {
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    /* Delete them */
    SHFILEOPSTRUCTW FileOp;
    ZeroMemory(&FileOp, sizeof(FileOp));
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = pwszPaths;
    FileOp.fFlags = FOF_ALLOWUNDO;

    if (SHFileOperationW(&FileOp) != 0)
    {
        ERR("SHFileOperation failed with 0x%x for %s\n", GetLastError(), debugstr_w(pwszPaths));
        hr = E_FAIL;
    }

    HeapFree(GetProcessHeap(), 0, pwszPaths);
    SHFree(pidl);
    _ILFreeaPidl(apidl, lpcida->cidl);
    ReleaseStgMedium(&medium);

    return hr;
}