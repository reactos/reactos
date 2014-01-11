#include "precomp.h"

/**************************************************************************
 *  AddToEnumList()
 */
BOOL 
CEnumIDList::AddToEnumList(LPITEMIDLIST pidl)
{
    LPENUMLIST pNew;

    if (!pidl)
        return FALSE;

    pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
    if(pNew)
    {
        pNew->pNext = NULL;
        pNew->pidl = pidl;

        if(!mpFirst)
        {
            mpFirst = pNew;
            mpCurrent = pNew;
        }

        if(mpLast)
        {
            /*add the new item to the end of the list */
            mpLast->pNext = pNew;
        }

        /*update the last item pointer */
        mpLast = pNew;
        return TRUE;
    }
    return FALSE;
}

CEnumIDList::CEnumIDList()
{
    ref = 0;
    mpCurrent = NULL;
    mpLast = NULL;
    mpFirst = NULL;
}

HRESULT
WINAPI
CEnumIDList::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumIDList))
    {
        *ppvObj = (IEnumIDList*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
CEnumIDList::AddRef()
{
    ULONG refCount = InterlockedIncrement(&ref);

    return refCount;
}

ULONG
WINAPI CEnumIDList::Release()
{
    LPENUMLIST pDelete;
    ULONG refCount = InterlockedDecrement(&ref);

    if (!refCount) 
    {
        while (mpFirst)
        {
            pDelete = mpFirst;
            mpFirst = pDelete->pNext;
            SHFree(pDelete->pidl);
            SHFree(pDelete);
        }
        delete this;
    }
    return refCount;
}

HRESULT
WINAPI
CEnumIDList::Next(
    ULONG celt,
    LPITEMIDLIST * rgelt,
    ULONG *pceltFetched)
{
    ULONG i;
    HRESULT hr = S_OK;
    LPITEMIDLIST temp;

    if(pceltFetched)
        *pceltFetched = 0;

    *rgelt=0;

    if (celt > 1 && !pceltFetched)
    {
        return E_INVALIDARG;
    }

    if (celt > 0 && !mpCurrent)
    {
        return S_FALSE;
    }

    for (i = 0; i < celt; i++)
    {
        if (!mpCurrent)
            break;

        temp = ILClone(mpCurrent->pidl);
        rgelt[i] = temp;
        mpCurrent = mpCurrent->pNext;
    }

    if (pceltFetched)
        *pceltFetched = i;

    return hr;
}

HRESULT
WINAPI
CEnumIDList::Skip(ULONG celt)
{
    DWORD dwIndex;
    HRESULT hr = S_OK;

    for (dwIndex = 0; dwIndex < celt; dwIndex++)
    {
        if (!mpCurrent)
        {
            hr = S_FALSE;
            break;
        }
        mpCurrent = mpCurrent->pNext;
    }

    return hr;
}

HRESULT
WINAPI
CEnumIDList::Reset()
{
    mpCurrent = mpFirst;
    return S_OK;
}

HRESULT
WINAPI
CEnumIDList::Clone(
    LPENUMIDLIST * ppenum)
{
    //IEnumIDListImpl *This = (IEnumIDListImpl *)iface;

    return E_NOTIMPL;
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

    pidlOut = (LPITEMIDLIST)SHAlloc(size + 5);
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
    IID *piid = _ILGetGUIDPointer(pidl);

    if (piid)
        return IsEqualIID(*piid, CLSID_NetworkConnections);

    return FALSE;
}

LPITEMIDLIST ILCreateNetConnectItem(INetConnection * pItem)
{
    LPITEMIDLIST pidl;
    LPPIDLDATA pdata;

    pidl = _ILAlloc(0x99, sizeof(PIDLDATA));
    pdata = _ILGetDataPointer(pidl);
    pdata->u.value.pItem = (INetConnection*)pItem;

    return pidl;
}

VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==0x99)
        return (VALUEStruct*)&(pdata->u.value);

    return NULL;
}
