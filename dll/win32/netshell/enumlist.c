#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct tagGUIDStruct
{
    BYTE dummy; /* offset 01 is unknown */
    GUID guid;  /* offset 02 */
} GUIDStruct;

#define PT_GUID		0x1F

typedef struct tagPIDLDATA
{
    BYTE type;			/*00*/
    union
    {
        struct tagGUIDStruct guid;
        struct tagVALUEStruct value;
    }u;
} PIDLDATA, *LPPIDLDATA;

typedef struct tagENUMLIST
{
    struct tagENUMLIST	*pNext;
    LPITEMIDLIST		pidl;

}ENUMLIST, *LPENUMLIST;

typedef struct
{
    const IEnumIDListVtbl *lpVtbl;
    LONG        ref;
    LPENUMLIST  mpFirst;
    LPENUMLIST  mpLast;
    LPENUMLIST  mpCurrent;

} IEnumIDListImpl;

/**************************************************************************
 *  AddToEnumList()
 */
BOOL 
AddToEnumList(
    IEnumIDList * iface,
    LPITEMIDLIST pidl)
{
    LPENUMLIST  pNew;
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;


    if (!iface || !pidl)
        return FALSE;

    pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
    if(pNew)
    {
        pNew->pNext = NULL;
        pNew->pidl = pidl;

        if(!This->mpFirst)
        {
            This->mpFirst = pNew;
            This->mpCurrent = pNew;
        }

        if(This->mpLast)
        {
            /*add the new item to the end of the list */
            This->mpLast->pNext = pNew;
        }

        /*update the last item pointer */
        This->mpLast = pNew;
        return TRUE;
    }
    return FALSE;
}

static
HRESULT
WINAPI
IEnumIDList_fnQueryInterface(
    IEnumIDList * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumIDList))
    {
        *ppvObj = This;
        IEnumIDList_AddRef((IEnumIDList*)*ppvObj);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
IEnumIDList_fnAddRef(
    IEnumIDList * iface)
{
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI IEnumIDList_fnRelease(
    IEnumIDList * iface)
{
    LPENUMLIST  pDelete;
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        while(This->mpFirst)
        {
            pDelete = This->mpFirst;
            This->mpFirst = pDelete->pNext;
            SHFree(pDelete->pidl);
            SHFree(pDelete);
        }
        CoTaskMemFree(This);
    }
    return refCount;
}

static 
HRESULT
WINAPI
IEnumIDList_fnNext(
    IEnumIDList * iface,
    ULONG celt,
    LPITEMIDLIST * rgelt,
    ULONG *pceltFetched)
{
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    ULONG    i;
    HRESULT  hr = S_OK;
    LPITEMIDLIST  temp;

    if(pceltFetched)
        *pceltFetched = 0;

    *rgelt=0;

    if(celt > 1 && !pceltFetched)
    {
        return E_INVALIDARG;
    }

    if(celt > 0 && !This->mpCurrent)
    {
        return S_FALSE;
    }

    for(i = 0; i < celt; i++)
    {
        if(!(This->mpCurrent))
            break;

        temp = ILClone(This->mpCurrent->pidl);
        rgelt[i] = temp;
        This->mpCurrent = This->mpCurrent->pNext;
    }

    if(pceltFetched)
    {
        *pceltFetched = i;
    }

    return hr;
}

static
HRESULT
WINAPI
IEnumIDList_fnSkip(
    IEnumIDList * iface,ULONG celt)
{
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    DWORD    dwIndex;
    HRESULT  hr = S_OK;

    for(dwIndex = 0; dwIndex < celt; dwIndex++)
    {
        if(!This->mpCurrent)
        {
            hr = S_FALSE;
            break;
        }
        This->mpCurrent = This->mpCurrent->pNext;
    }
    return hr;
}

static
HRESULT
WINAPI
IEnumIDList_fnReset(
    IEnumIDList * iface)
{
    IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    This->mpCurrent = This->mpFirst;
    return S_OK;
}

static
HRESULT
WINAPI
IEnumIDList_fnClone(
    IEnumIDList * iface,
    LPENUMIDLIST * ppenum)
{
    //IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    return E_NOTIMPL;
}

static const IEnumIDListVtbl eidlvt =
{
    IEnumIDList_fnQueryInterface,
    IEnumIDList_fnAddRef,
    IEnumIDList_fnRelease,
    IEnumIDList_fnNext,
    IEnumIDList_fnSkip,
    IEnumIDList_fnReset,
    IEnumIDList_fnClone,
};

IEnumIDList * IEnumIDList_Constructor(void)
{
    IEnumIDListImpl *lpeidl = CoTaskMemAlloc(sizeof(IEnumIDListImpl));

    if (lpeidl)
    {
        lpeidl->ref = 1;
        lpeidl->lpVtbl = &eidlvt;
        lpeidl->mpCurrent = NULL;
        lpeidl->mpLast = NULL;
        lpeidl->mpFirst = NULL;
    }

    return (IEnumIDList*)lpeidl;
}

LPPIDLDATA _ILGetDataPointer(LPCITEMIDLIST pidl)
{
    if(pidl && pidl->mkid.cb != 0x00)
        return (LPPIDLDATA) &(pidl->mkid.abID);
    return NULL;
}

LPITEMIDLIST _ILAlloc(BYTE type, unsigned int size)
{
    LPITEMIDLIST pidlOut = NULL;

    pidlOut = SHAlloc(size + 5);
    if(pidlOut)
    {
        LPPIDLDATA pData;

        ZeroMemory(pidlOut, size + 5);
        pidlOut->mkid.cb = size + 3;
        pData = _ILGetDataPointer(pidlOut);
        if (pData)
            pData->type = type;

    }

    return pidlOut;
}

LPITEMIDLIST _ILCreateNetConnect()
{
    LPITEMIDLIST pidlOut;

    pidlOut = _ILAlloc(PT_GUID, sizeof(PIDLDATA));
    if (pidlOut)
    {
        LPPIDLDATA pData = _ILGetDataPointer(pidlOut);

        memcpy(&(pData->u.guid.guid), &CLSID_NetworkConnections, sizeof(GUID));
    }
    return pidlOut;
}

IID* _ILGetGUIDPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata =_ILGetDataPointer(pidl);

    if (!pdata)
        return NULL;

    if (pdata->type != PT_GUID)
        return NULL;
    else
        return &(pdata->u.guid.guid);

}

BOOL _ILIsNetConnect(LPCITEMIDLIST pidl)
{
    REFIID iid = _ILGetGUIDPointer(pidl);

    if (iid)
        return IsEqualIID(iid, &CLSID_NetworkConnections);
    return FALSE;
}

LPITEMIDLIST ILCreateNetConnectItem(INetConnection * pItem)
{
    LPITEMIDLIST pidl;
    LPPIDLDATA pdata;

    pidl = _ILAlloc(0x99, sizeof(PIDLDATA));
    pdata = _ILGetDataPointer(pidl);
    pdata->u.value.pItem = (PVOID)pItem;

    return pidl;
}

VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==0x99)
        return (VALUEStruct*)&(pdata->u.value);

    return NULL;
}
