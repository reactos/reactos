/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
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

#define MAX_PROPERTY_SHEET_PAGE 32

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(recyclebin);

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
    {IDS_SHV_COLUMN1,        &FMTID_Storage,   PID_STG_NAME,       SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELFROM, &FMTID_Displaced, PID_DISPLACED_FROM, SHCOLSTATE_TYPE_STR|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  30},
    {IDS_SHV_COLUMN_DELDATE, &FMTID_Displaced, PID_DISPLACED_DATE, SHCOLSTATE_TYPE_DATE|SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN2,        &FMTID_Storage,   PID_STG_SIZE,       SHCOLSTATE_TYPE_INT|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 20},
    {IDS_SHV_COLUMN3,        &FMTID_Storage,   PID_STG_STORAGETYPE,SHCOLSTATE_TYPE_INT|SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  20},
    {IDS_SHV_COLUMN4,        &FMTID_Storage,   PID_STG_WRITETIME,  SHCOLSTATE_TYPE_DATE|SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  20},
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

typedef struct tagRecycleBin
{
    const IShellFolder2Vtbl *lpVtbl;
    const IPersistFolder2Vtbl *lpPersistFolderVtbl;
    const IContextMenu2Vtbl *lpContextMenu2;
    const IShellExtInitVtbl *lpSEI;
    LONG refCount;
    INT iIdEmpty;
    LPITEMIDLIST pidl;
    LPCITEMIDLIST apidl;
} RecycleBin, *LPRecycleBin;

typedef struct
{
    PIDLRecycleStruct *pFileDetails;
    HANDLE hDeletedFile;
    BOOL bFound;
}SEARCH_CONTEXT, *PSEARCH_CONTEXT;

typedef struct
{
    DWORD dwNukeOnDelete;
    DWORD dwSerial;
    DWORD dwMaxCapacity;
}DRIVE_ITEM_CONTEXT, *PDRIVE_ITEM_CONTEXT;


static const IContextMenu2Vtbl recycleBincmVtblFolder;
static const IContextMenu2Vtbl recycleBincmVtblBitbucketItem;
static const IShellFolder2Vtbl recycleBinVtbl;
static const IPersistFolder2Vtbl recycleBinPersistVtbl;
static const IShellExtInitVtbl eivt;

static LPRecycleBin __inline impl_from_IContextMenu2(IContextMenu2 *iface)
{
    return (RecycleBin *)((char *)iface - FIELD_OFFSET(RecycleBin, lpContextMenu2));
}

static LPRecycleBin __inline impl_from_IPersistFolder(IPersistFolder2 *iface)
{
    return (RecycleBin *)((char *)iface - FIELD_OFFSET(RecycleBin, lpPersistFolderVtbl));
}

static  LPRecycleBin __inline impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (RecycleBin *)((char*)iface - FIELD_OFFSET(RecycleBin, lpSEI));
}

static void RecycleBin_Destructor(RecycleBin *This);

HRESULT WINAPI RecycleBin_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput)
{
    RecycleBin *obj;
    HRESULT ret;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = SHAlloc(sizeof(RecycleBin));
    if (obj == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(obj, sizeof(RecycleBin));
    obj->lpVtbl = &recycleBinVtbl;
    obj->lpSEI = &eivt;
    obj->lpPersistFolderVtbl = &recycleBinPersistVtbl;
    obj->lpContextMenu2 = NULL;
    if (FAILED(ret = IUnknown_QueryInterface((IUnknown *)obj, riid, ppOutput)))
    {
        RecycleBin_Destructor(obj);
        return ret;
    }
/*    InterlockedIncrement(&objCount);*/
    return S_OK;
}

static void RecycleBin_Destructor(RecycleBin *This)
{
/*    InterlockedDecrement(&objCount);*/
    SHFree(This->pidl);
    SHFree(This);
}

static HRESULT WINAPI RecycleBin_QueryInterface(IShellFolder2 *iface, REFIID riid, void **ppvObject)
{
    RecycleBin *This = (RecycleBin *)iface;
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IShellFolder)
            || IsEqualGUID(riid, &IID_IShellFolder2))
        *ppvObject = This;

    if (IsEqualGUID(riid, &IID_IPersist) || IsEqualGUID(riid, &IID_IPersistFolder)
            || IsEqualGUID(riid, &IID_IPersistFolder2))
        *ppvObject = &This->lpPersistFolderVtbl;

	else if (IsEqualIID(riid, &IID_IContextMenu) || IsEqualGUID(riid, &IID_IContextMenu2))
    {
        This->lpContextMenu2 = &recycleBincmVtblFolder;
        *ppvObject = &This->lpContextMenu2;
    }
	else if(IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObject = &(This->lpSEI);
    }

    if (*ppvObject != NULL)
    {
        IUnknown_AddRef((IUnknown *)*ppvObject);
        return S_OK;
    }
    WARN("no interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI RecycleBin_AddRef(IShellFolder2 *iface)
{
    RecycleBin *This = (RecycleBin *)iface;
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI RecycleBin_Release(IShellFolder2 *iface)
{
    RecycleBin *This = (RecycleBin *)iface;
    LONG result;

    TRACE("(%p)\n", This);
    result = InterlockedDecrement(&This->refCount);
    if (result == 0)
    {
        TRACE("Destroy object\n");
        RecycleBin_Destructor(This);
    }
    return result;
}

static HRESULT WINAPI RecycleBin_ParseDisplayName(IShellFolder2 *This, HWND hwnd, LPBC pbc,
            LPOLESTR pszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl,
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

static LPITEMIDLIST _ILCreateRecycleItem(PDELETED_FILE_DETAILS_W pFileDetails)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLRecycleStruct * p;
    int size0 = (char*)&tmp.u.crecycle.szName-(char*)&tmp.u.crecycle;
    int size = size0;

    tmp.type = 0x00;
    size += (wcslen(pFileDetails->FileName) + 1) * sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size+2;
    memcpy(pidl->mkid.abID, &tmp, 2+size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.crecycle;
    RtlCopyMemory(p, pFileDetails, sizeof(DELETED_FILE_DETAILS_W));
    wcscpy(p->szName, pFileDetails->FileName);
    *(WORD*)((char*)pidl+(size+2)) = 0;
    return pidl;
}

static PIDLRecycleStruct * _ILGetRecycleStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==0x00)
        return (PIDLRecycleStruct*)&(pdata->u.crecycle);

    return NULL;
}

BOOL
WINAPI
CBSearchBitBucket(IN PVOID Context, IN HANDLE hDeletedFile)
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

    pFileDetails = SHAlloc(dwSize);
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



BOOL
WINAPI
CBEnumBitBucket(IN PVOID Context, IN HANDLE hDeletedFile)
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

    pFileDetails = SHAlloc(dwSize);
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

    ret = AddToEnumList((IEnumIDList*)Context, pidl);

    if (!ret)
        SHFree(pidl);
    SHFree(pFileDetails);
    TRACE("Returning %d\n", ret);
    CloseRecycleBinHandle(hDeletedFile);
    return ret;
}

static HRESULT WINAPI RecycleBin_EnumObjects(IShellFolder2 *iface, HWND hwnd, SHCONTF grfFlags, IEnumIDList **ppenumIDList)
{
    RecycleBin *This = (RecycleBin *)iface;
    IEnumIDList *list;
    static LPCWSTR szDrive = L"C:\\";

    TRACE("(%p, %p, %x, %p)\n", This, hwnd, (unsigned int)grfFlags, ppenumIDList);

    if (grfFlags & SHCONTF_NONFOLDERS)
    {
        TRACE("Starting Enumeration\n");
        *ppenumIDList = NULL;
        list = IEnumIDList_Constructor();
        if (list == NULL)
            return E_OUTOFMEMORY;

        if (!EnumerateRecycleBinW(szDrive, //FIXME
                                  CBEnumBitBucket,
                                  (PVOID)list))
        {
            WARN("Error: EnumerateRecycleBinW failed\n");
        }
        *ppenumIDList = list;
    }
    else
    {
        *ppenumIDList = IEnumIDList_Constructor();
        if (*ppenumIDList == NULL)
            return E_OUTOFMEMORY;
    }

    return S_OK;

}

static HRESULT WINAPI RecycleBin_BindToObject(IShellFolder2 *This, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", This, pidl, pbc, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_BindToStorage(IShellFolder2 *This, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", This, pidl, pbc, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_CompareIDs(IShellFolder2 *iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    RecycleBin *This = (RecycleBin *)iface;

    /* TODO */
    TRACE("(%p, %p, %p, %p)\n", This, (void *)lParam, pidl1, pidl2);
    if (pidl1->mkid.cb != pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, pidl1->mkid.cb - pidl2->mkid.cb);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (unsigned short)memcmp(pidl1->mkid.abID, pidl2->mkid.abID, pidl1->mkid.cb));
}

static HRESULT WINAPI RecycleBin_CreateViewObject(IShellFolder2 *iface, HWND hwndOwner, REFIID riid, void **ppv)
{
    RecycleBin *This = (RecycleBin *)iface;
    LPSHELLVIEW pShellView;
    HRESULT hr = E_NOINTERFACE;

    TRACE("(%p, %p, %s, %p)\n", This, hwndOwner, debugstr_guid(riid), ppv);

    if (!ppv)
        return hr;

    *ppv = NULL;

    if (IsEqualIID (riid, &IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IContextMenu) || IsEqualIID (riid, &IID_IContextMenu2))
    {
        This->lpContextMenu2 = &recycleBincmVtblFolder;
        *ppv = &This->lpContextMenu2;
        This->refCount++;
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IShellView))
    {
        pShellView = IShellView_Constructor ((IShellFolder *) iface);
        if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppv);
            IShellView_Release (pShellView);
        }
    }
    else
        return hr;
    TRACE ("-- (%p)->(interface=%p)\n", This, ppv);
    return hr;

}

static HRESULT WINAPI RecycleBin_GetAttributesOf(IShellFolder2 *This, UINT cidl, LPCITEMIDLIST *apidl,
                                   SFGAOF *rgfInOut)
{
    TRACE("(%p, %d, {%p, ...}, {%x})\n", This, cidl, apidl[0], (unsigned int)*rgfInOut);
    *rgfInOut &= SFGAO_CANMOVE|SFGAO_CANDELETE|SFGAO_HASPROPSHEET|SFGAO_FILESYSTEM;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetUIObjectOf(IShellFolder2 *iface, HWND hwndOwner, UINT cidl, LPCITEMIDLIST *apidl,
                      REFIID riid, UINT *prgfInOut, void **ppv)
{
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;
    RecycleBin * This = (RecycleBin*)iface;

    TRACE ("(%p)->(%p,%u,apidl=%p, %p %p)\n", This,
            hwndOwner, cidl, apidl, prgfInOut, ppv);

    if (!ppv)
        return hr;

    *ppv = NULL;

    if ((IsEqualIID (riid, &IID_IContextMenu) || IsEqualIID(riid, &IID_IContextMenu2)) && (cidl >= 1))
    {
        This->lpContextMenu2 = &recycleBincmVtblBitbucketItem;
        pObj = (IUnknown*)(&This->lpContextMenu2);
        This->apidl = apidl[0];
        IUnknown_AddRef(pObj);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
        hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppv = pObj;
    TRACE ("(%p)->hr=0x%08x\n", This, hr);
    return hr;
}

static HRESULT WINAPI RecycleBin_GetDisplayNameOf(IShellFolder2 *This, LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET *pName)
{
    PIDLRecycleStruct *pFileDetails;
    LPWSTR pFileName;

    TRACE("(%p, %p, %x, %p)\n", This, pidl, (unsigned int)uFlags, pName);


    if (_ILIsBitBucket (pidl))
    {
       WCHAR pszPath[100];

       if (HCR_GetClassNameW(&CLSID_RecycleBin, pszPath, MAX_PATH))
       {
           pName->uType = STRRET_WSTR;
           pName->u.pOleStr = StrDupW(pszPath);
           return S_OK;
       }
    }

    pFileDetails = _ILGetRecycleStruct(pidl);
    if (!pFileDetails)
    {
        pName->u.cStr[0] = 0;
        pName->uType = STRRET_CSTR;
        return E_INVALIDARG;
    }

    pFileName = wcsrchr(pFileDetails->szName, L'\\');
    if (!pFileName)
    {
        pName->u.cStr[0] = 0;
        pName->uType = STRRET_CSTR;
        return E_UNEXPECTED;
    }

    pName->u.pOleStr = StrDupW(pFileName + 1);
    if (pName->u.pOleStr == NULL)
        return E_OUTOFMEMORY;

    pName->uType = STRRET_WSTR;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_SetNameOf(IShellFolder2 *This, HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName,
            SHGDNF uFlags, LPITEMIDLIST *ppidlOut)
{
    TRACE("\n");
    return E_FAIL; /* not supported */
}

static HRESULT WINAPI RecycleBin_GetDefaultSearchGUID(IShellFolder2 *iface, GUID *pguid)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_EnumSearches(IShellFolder2 *iface, IEnumExtraSearch **ppEnum)
{
    FIXME("stub\n");
    *ppEnum = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI RecycleBin_GetDefaultColumn(IShellFolder2 *iface, DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
    RecycleBin *This = (RecycleBin *)iface;
    TRACE("(%p, %x, %p, %p)\n", This, (unsigned int)dwReserved, pSort, pDisplay);
    *pSort = 0;
    *pDisplay = 0;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetDefaultColumnState(IShellFolder2 *iface, UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    RecycleBin *This = (RecycleBin *)iface;
    TRACE("(%p, %d, %p)\n", This, iColumn, pcsFlags);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    *pcsFlags = RecycleBinColumns[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_GetDetailsEx(IShellFolder2 *iface, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
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
    if (ret>0 && ret<size)
    {
        /* Append space + time without seconds */
        buffer[ret-1] = ' ';
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, &buffer[ret], size - ret);
    }

    return (ret!=0 ? E_FAIL : S_OK);
}

static HRESULT WINAPI RecycleBin_GetDetailsOf(IShellFolder2 *iface, LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    RecycleBin *This = (RecycleBin *)iface;
    PIDLRecycleStruct * pFileDetails;
    WCHAR buffer[MAX_PATH];
    WCHAR szTypeName[100];
    LPWSTR pszBackslash;
    UINT Length;

    TRACE("(%p, %p, %d, %p)\n", This, pidl, iColumn, pDetails);
    if (iColumn >= COLUMNS_COUNT)
        return E_FAIL;
    pDetails->fmt = RecycleBinColumns[iColumn].fmt;
    pDetails->cxChar = RecycleBinColumns[iColumn].cxChars;
    if (pidl == NULL)
    {
        pDetails->str.uType = STRRET_WSTR;
        LoadStringW(shell32_hInstance, RecycleBinColumns[iColumn].column_name_id, buffer, MAX_PATH);
        return SHStrDupW(buffer, &pDetails->str.u.pOleStr);
    }

    if (iColumn == COLUMN_NAME)
        return RecycleBin_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &pDetails->str);

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
            wcscpy(buffer,PathFindExtensionW(pFileDetails->szName));
            if (!( HCR_MapTypeToValueW(buffer, buffer, sizeof(buffer)/sizeof(WCHAR), TRUE) &&
                   HCR_MapTypeToValueW(buffer, szTypeName, sizeof(szTypeName)/sizeof(WCHAR), FALSE )))
            {
                wcscpy (szTypeName, PathFindExtensionW(pFileDetails->szName));
                wcscat(szTypeName, L"-");
                Length = wcslen(szTypeName);
                if (LoadStringW(shell32_hInstance, IDS_SHV_COLUMN1, &szTypeName[Length], (sizeof(szTypeName)/sizeof(WCHAR))- Length))
                    szTypeName[(sizeof(szTypeName)/sizeof(WCHAR))-1] = L'\0';
            }
            pDetails->str.uType = STRRET_WSTR;
            return SHStrDupW(szTypeName, &pDetails->str.u.pOleStr);
            break;
        default:
            return E_FAIL;
    }

    pDetails->str.uType = STRRET_WSTR;
    return SHStrDupW(buffer, &pDetails->str.u.pOleStr);
}

static HRESULT WINAPI RecycleBin_MapColumnToSCID(IShellFolder2 *iface, UINT iColumn, SHCOLUMNID *pscid)
{
    RecycleBin *This = (RecycleBin *)iface;
    TRACE("(%p, %d, %p)\n", This, iColumn, pscid);
    if (iColumn>=COLUMNS_COUNT)
        return E_INVALIDARG;
    pscid->fmtid = *RecycleBinColumns[iColumn].fmtId;
    pscid->pid = RecycleBinColumns[iColumn].pid;
    return S_OK;
}

static const IShellFolder2Vtbl recycleBinVtbl =
{
    /* IUnknown */
    RecycleBin_QueryInterface,
    RecycleBin_AddRef,
    RecycleBin_Release,

    /* IShellFolder */
    RecycleBin_ParseDisplayName,
    RecycleBin_EnumObjects,
    RecycleBin_BindToObject,
    RecycleBin_BindToStorage,
    RecycleBin_CompareIDs,
    RecycleBin_CreateViewObject,
    RecycleBin_GetAttributesOf,
    RecycleBin_GetUIObjectOf,
    RecycleBin_GetDisplayNameOf,
    RecycleBin_SetNameOf,

    /* IShellFolder2 */
    RecycleBin_GetDefaultSearchGUID,
    RecycleBin_EnumSearches,
    RecycleBin_GetDefaultColumn,
    RecycleBin_GetDefaultColumnState,
    RecycleBin_GetDetailsEx,
    RecycleBin_GetDetailsOf,
    RecycleBin_MapColumnToSCID
};

static HRESULT WINAPI RecycleBin_IPersistFolder2_GetClassID(IPersistFolder2 *This, CLSID *pClassID)
{
    TRACE("(%p, %p)\n", This, pClassID);
    if (This == NULL || pClassID == NULL)
        return E_INVALIDARG;
    memcpy(pClassID, &CLSID_RecycleBin, sizeof(CLSID));
    return S_OK;
}

static HRESULT WINAPI RecycleBin_IPersistFolder2_Initialize(IPersistFolder2 *iface, LPCITEMIDLIST pidl)
{
    RecycleBin *This = impl_from_IPersistFolder(iface);
    TRACE("(%p, %p)\n", This, pidl);

    SHFree((LPVOID)This->pidl);
    This->pidl = ILClone(pidl);
    if (This->pidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI RecycleBin_IPersistFolder2_GetCurFolder(IPersistFolder2 *iface, LPITEMIDLIST *ppidl)
{
    RecycleBin *This = impl_from_IPersistFolder(iface);
    TRACE("\n");
    *ppidl = ILClone(This->pidl);
    return S_OK;
}

static HRESULT WINAPI RecycleBin_IPersistFolder2_QueryInterface(IPersistFolder2 *This, REFIID riid, void **ppvObject)
{
    return RecycleBin_QueryInterface((IShellFolder2 *)impl_from_IPersistFolder(This), riid, ppvObject);
}

static ULONG WINAPI RecycleBin_IPersistFolder2_AddRef(IPersistFolder2 *This)
{
    return RecycleBin_AddRef((IShellFolder2 *)impl_from_IPersistFolder(This));
}

static ULONG WINAPI RecycleBin_IPersistFolder2_Release(IPersistFolder2 *This)
{
    return RecycleBin_Release((IShellFolder2 *)impl_from_IPersistFolder(This));
}

static const IPersistFolder2Vtbl recycleBinPersistVtbl =
{
    /* IUnknown */
    RecycleBin_IPersistFolder2_QueryInterface,
    RecycleBin_IPersistFolder2_AddRef,
    RecycleBin_IPersistFolder2_Release,

    /* IPersist */
    RecycleBin_IPersistFolder2_GetClassID,
    /* IPersistFolder */
    RecycleBin_IPersistFolder2_Initialize,
    /* IPersistFolder2 */
    RecycleBin_IPersistFolder2_GetCurFolder
};

/*************************************************************************
 * BitBucket IShellExtInit interface
 */

static HRESULT WINAPI
RecycleBin_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    return RecycleBin_QueryInterface((IShellFolder2 *)impl_from_IShellExtInit(iface), riid, ppvObject);
}

static ULONG WINAPI
RecycleBin_ExtInit_AddRef( IShellExtInit* iface )
{
    return RecycleBin_AddRef((IShellFolder2 *)impl_from_IShellExtInit(iface));
}

static ULONG WINAPI
RecycleBin_ExtInit_Release( IShellExtInit* iface )
{
    return RecycleBin_Release((IShellFolder2 *)impl_from_IShellExtInit(iface));
}

static HRESULT WINAPI
RecycleBin_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    RecycleBin *This = impl_from_IShellExtInit(iface);

    TRACE("%p %p %p %p\n", This, pidlFolder, pdtobj, hkeyProgID );
    return S_OK;
}


static const IShellExtInitVtbl eivt =
{
    RecycleBin_ExtInit_QueryInterface,
    RecycleBin_ExtInit_AddRef,
    RecycleBin_ExtInit_Release,
    RecycleBin_ExtInit_Initialize
};


/*************************************************************************
 * BitBucket context menu
 *
 */

static HRESULT WINAPI
RecycleBin_IContextMenu2Folder_QueryInterface( IContextMenu2* iface, REFIID riid, void** ppvObject )
{
    return RecycleBin_QueryInterface((IShellFolder2 *)impl_from_IContextMenu2(iface), riid, ppvObject);
}

static ULONG WINAPI
RecycleBin_IContextMenu2Folder_AddRef( IContextMenu2* iface )
{
    return RecycleBin_AddRef((IShellFolder2 *)impl_from_IContextMenu2(iface));
}

static ULONG WINAPI
RecycleBin_IContextMenu2Folder_Release( IContextMenu2* iface )
{
    return RecycleBin_Release((IShellFolder2 *)impl_from_IContextMenu2(iface));
}

static HRESULT WINAPI
RecycleBin_IContextMenu2Folder_QueryContextMenu( IContextMenu2* iface, HMENU hmenu, UINT indexMenu,
                            UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{
    WCHAR szBuffer[100];
    MENUITEMINFOW mii;
    int id = 1;
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("%p %p %u %u %u %u\n", This,
          hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if ( !hmenu )
        return E_INVALIDARG;

    memset( &mii, 0, sizeof(mii) );
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fState = MFS_ENABLED;
    szBuffer[0] = L'\0';
    LoadStringW(shell32_hInstance, IDS_EMPTY_BITBUCKET, szBuffer, sizeof(szBuffer)/sizeof(WCHAR));
    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
    mii.dwTypeData = szBuffer;
    mii.cch = wcslen( mii.dwTypeData );
    mii.wID = idCmdFirst + id++;
    mii.fType = MFT_STRING;
    This->iIdEmpty = 1;

    if (!InsertMenuItemW( hmenu, indexMenu, TRUE, &mii ))
        return E_FAIL;

    return MAKE_HRESULT( SEVERITY_SUCCESS, 0, id );
}

static HRESULT WINAPI
RecycleBin_IContextMenu2Folder_InvokeCommand( IContextMenu2* iface, LPCMINVOKECOMMANDINFO lpici )
{
    HRESULT hr;
    LPSHELLBROWSER	lpSB;
    LPSHELLVIEW lpSV = NULL;
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("%p %p verb %p\n", This, lpici, lpici->lpVerb);

    if ( LOWORD(lpici->lpVerb) == This->iIdEmpty)
    {
       // FIXME
       // path & flags
       hr = SHEmptyRecycleBinW(lpici->hwnd, L"C:\\", 0);
       TRACE("result %x\n", hr);
       if (hr != S_OK)
       {
          return hr;
       }

       if((lpSB = (LPSHELLBROWSER)SendMessageA(lpici->hwnd, CWM_GETISHELLBROWSER,0,0)))
       {
          if(SUCCEEDED(IShellBrowser_QueryActiveShellView(lpSB, &lpSV)))
          {
             IShellView_Refresh(lpSV);
          }
       }
    }
    return S_OK;
}

static HRESULT WINAPI
RecycleBin_IContextMenu2Folder_GetCommandString( IContextMenu2* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

/**************************************************************************
* RecycleBin_IContextMenu2Item_HandleMenuMsg()
*/
static HRESULT WINAPI RecycleBin_IContextMenu2Folder_HandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("RecycleBin_IContextMenu2Item_IContextMenu2Folder_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    return E_NOTIMPL;
}


static const IContextMenu2Vtbl recycleBincmVtblFolder =
{
    RecycleBin_IContextMenu2Folder_QueryInterface,
    RecycleBin_IContextMenu2Folder_AddRef,
    RecycleBin_IContextMenu2Folder_Release,
    RecycleBin_IContextMenu2Folder_QueryContextMenu,
    RecycleBin_IContextMenu2Folder_InvokeCommand,
    RecycleBin_IContextMenu2Folder_GetCommandString,
    RecycleBin_IContextMenu2Folder_HandleMenuMsg
};


/**************************************************************************
* IContextMenu2 Bitbucket Item Implementation
*/

/************************************************************************
 * RecycleBin_IContextMenu2Item_QueryInterface
 */
static HRESULT WINAPI RecycleBin_IContextMenu2Item_QueryInterface(IContextMenu2 * iface, REFIID iid, LPVOID * ppvObject)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("(%p)\n", This);

    return RecycleBin_QueryInterface((IShellFolder2*)This, iid, ppvObject);
}

/************************************************************************
 * RecycleBin_IContextMenu2Item_AddRef
 */
static ULONG WINAPI RecycleBin_IContextMenu2Item_AddRef(IContextMenu2 * iface)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("(%p)->(count=%u)\n", This, This->refCount);

    return RecycleBin_AddRef((IShellFolder2*)This);
}

/************************************************************************
 * RecycleBin_IContextMenu2Item_Release
 */
static ULONG WINAPI RecycleBin_IContextMenu2Item_Release(IContextMenu2  * iface)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("(%p)->(count=%u)\n", This, This->refCount);

    return RecycleBin_Release((IShellFolder2*)This);
}

/**************************************************************************
* RecycleBin_IContextMenu2Item_QueryContextMenu()
*/
static HRESULT WINAPI RecycleBin_IContextMenu2Item_QueryContextMenu(
	IContextMenu2 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    WCHAR szBuffer[30] = {0};
    ULONG Count = 1;

    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (LoadStringW(shell32_hInstance, IDS_RESTORE, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
        Count++;
    }

    if (LoadStringW(shell32_hInstance, IDS_CUT, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_DELETE, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_PROPERTIES, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_DEFAULT);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}


/**************************************************************************
* RecycleBin_IContextMenu2Item_InvokeCommand()
*/
static HRESULT WINAPI RecycleBin_IContextMenu2Item_InvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
	
    SEARCH_CONTEXT Context;
    RecycleBin * This = impl_from_IContextMenu2(iface);
    static LPCWSTR szDrive = L"C:\\";

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(1) || lpcmi->lpVerb == MAKEINTRESOURCEA(5)) 
    {
        Context.pFileDetails = _ILGetRecycleStruct(This->apidl);
        Context.bFound = FALSE;

        EnumerateRecycleBinW(szDrive, CBSearchBitBucket, (PVOID)&Context);
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

/**************************************************************************
 *  RecycleBin_IContextMenu2Item_GetCommandString()
 *
 */
static HRESULT WINAPI RecycleBin_IContextMenu2Item_GetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);


	return E_FAIL;
}



/**************************************************************************
* RecycleBin_IContextMenu2Item_HandleMenuMsg()
*/
static HRESULT WINAPI RecycleBin_IContextMenu2Item_HandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    RecycleBin * This = impl_from_IContextMenu2(iface);

    TRACE("RecycleBin_IContextMenu2Item_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

static const IContextMenu2Vtbl recycleBincmVtblBitbucketItem =
{
	RecycleBin_IContextMenu2Item_QueryInterface,
	RecycleBin_IContextMenu2Item_AddRef,
	RecycleBin_IContextMenu2Item_Release,
	RecycleBin_IContextMenu2Item_QueryContextMenu,
	RecycleBin_IContextMenu2Item_InvokeCommand,
	RecycleBin_IContextMenu2Item_GetCommandString,
	RecycleBin_IContextMenu2Item_HandleMenuMsg
};


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


void
InitializeBitBucketDlg(HWND hwndDlg, WCHAR DefaultDrive)
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
            (void)SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&li);
            if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable , &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
            {
                if (StrFormatByteSizeW(TotalNumberOfFreeBytes.QuadPart, szVolume, sizeof(szVolume) / sizeof(WCHAR)))
                {

                    pItem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DRIVE_ITEM_CONTEXT));
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
   }while(dwDrives);

   if (!pDefault)
       pDefault = pFirst;
   if (pDefault)
   {
       toggleNukeOnDeleteOption(hwndDlg, pDefault->dwNukeOnDelete);
       SetDlgItemInt(hwndDlg, 14002, pDefault->dwMaxCapacity, FALSE);
   }
   ZeroMemory(&li, sizeof(li));
   li.mask = LVIF_STATE;
   li.stateMask = (UINT)-1;
   li.state = LVIS_FOCUSED|LVIS_SELECTED;
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


   if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Bitbucket\\Volume", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
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
               RegSetValueExW(hSubKey, L"NukeOnDelete", 0, REG_DWORD, (LPVOID)&pItem->dwNukeOnDelete, dwSize);
               dwSize = sizeof(DWORD);
               RegSetValueExW(hSubKey, L"MaxCapacity", 0, REG_DWORD, (LPVOID)&pItem->dwMaxCapacity, dwSize);
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

INT
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
   li->stateMask = (UINT)-1;
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

INT_PTR
CALLBACK
BitBucketDlg(
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
        InitializeBitBucketDlg(hwndDlg, (WCHAR)page->lParam);
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
                    SetWindowLong( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
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
   psh.u3.phpage = hpsp;
   psh.hInstance = shell32_hInstance;

   hprop = SH_CreatePropertySheetPage("BITBUCKET_PROPERTIES_DLG", BitBucketDlg, (LPARAM)sDrive, NULL);
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
   return DeleteFileToRecycleBinW(wszPath);
}

/*************************************************************************
 * SHUpdateRecycleBinIcon                                [SHELL32.@]
 *
 * Undocumented
 */
HRESULT WINAPI SHUpdateRecycleBinIcon(void)
{
    FIXME("stub\n");



    return S_OK;
}
