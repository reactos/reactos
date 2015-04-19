#include "precomp.h"

/**************************************************************************
 *  AddToEnumList()
 */
BOOL
CEnumIDList::AddToEnumList(PITEMID_CHILD pidl)
{
    LPENUMLIST pNew;

    if (!pidl)
        return FALSE;

    pNew = static_cast<LPENUMLIST>(SHAlloc(sizeof(ENUMLIST)));
    if (pNew)
    {
        pNew->pNext = NULL;
        pNew->pidl = pidl;

        if (!m_pFirst)
        {
            m_pFirst = pNew;
            m_pCurrent = pNew;
        }

        if (m_pLast)
        {
            /*add the new item to the end of the list */
            m_pLast->pNext = pNew;
        }

        /*update the last item pointer */
        m_pLast = pNew;
        return TRUE;
    }
    return FALSE;
}

CEnumIDList::CEnumIDList() :
    m_ref(0),
    m_pFirst(NULL),
    m_pLast(NULL),
    m_pCurrent(NULL)
{
}

CEnumIDList::~CEnumIDList()
{
    LPENUMLIST pDelete;

    while (m_pFirst)
    {
        pDelete = m_pFirst;
        m_pFirst = pDelete->pNext;
        SHFree(pDelete->pidl);
        SHFree(pDelete);
    }
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
        *ppvObj = static_cast<IEnumIDList*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
WINAPI
CEnumIDList::AddRef()
{
    ULONG refCount = InterlockedIncrement(&m_ref);

    return refCount;
}

ULONG
WINAPI CEnumIDList::Release()
{
    ULONG refCount = InterlockedDecrement(&m_ref);

    if (!refCount)
        delete this;

    return refCount;
}

HRESULT
WINAPI
CEnumIDList::Next(
    ULONG celt,
    PITEMID_CHILD *rgelt,
    ULONG *pceltFetched)
{
    ULONG i;
    HRESULT hr = S_OK;
    PITEMID_CHILD temp;

    if (pceltFetched)
        *pceltFetched = 0;

    if (celt > 1 && !pceltFetched)
    {
        return E_INVALIDARG;
    }

    if (celt > 0 && !m_pCurrent)
    {
        return S_FALSE;
    }

    for (i = 0; i < celt; i++)
    {
        if (!m_pCurrent)
            break;

        temp = ILClone(m_pCurrent->pidl);
        rgelt[i] = temp;
        m_pCurrent = m_pCurrent->pNext;
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
        if (!m_pCurrent)
        {
            hr = S_FALSE;
            break;
        }
        m_pCurrent = m_pCurrent->pNext;
    }

    return hr;
}

HRESULT
WINAPI
CEnumIDList::Reset()
{
    m_pCurrent = m_pFirst;
    return S_OK;
}

HRESULT
WINAPI
CEnumIDList::Clone(
    LPENUMIDLIST * ppenum)
{
    return E_NOTIMPL;
}

LPPIDLDATA _ILGetDataPointer(LPITEMIDLIST pidl)
{
    if (pidl && pidl->mkid.cb != 0x00)
        return reinterpret_cast<LPPIDLDATA>(&pidl->mkid.abID);
    return NULL;
}

LPITEMIDLIST _ILAlloc(BYTE type, unsigned int size)
{
    LPITEMIDLIST pidlOut = NULL;

    pidlOut = static_cast<LPITEMIDLIST>(SHAlloc(size + 5));
    if (pidlOut)
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

PITEMID_CHILD _ILCreateNetConnect()
{
    PITEMID_CHILD pidlOut;

    pidlOut = _ILAlloc(PT_GUID, sizeof(PIDLDATA));
    if (pidlOut)
    {
        LPPIDLDATA pData = _ILGetDataPointer(pidlOut);

        memcpy(&(pData->u.guid.guid), &CLSID_ConnectionFolder, sizeof(GUID));
    }
    return pidlOut;
}

GUID* _ILGetGUIDPointer(LPITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (!pdata)
        return NULL;

    if (pdata->type != PT_GUID)
        return NULL;
    else
        return &(pdata->u.guid.guid);

}

BOOL _ILIsNetConnect(LPCITEMIDLIST pidl)
{
    const IID *piid = _ILGetGUIDPointer(const_cast<LPITEMIDLIST>(pidl));

    if (piid)
        return IsEqualIID(*piid, CLSID_ConnectionFolder);

    return FALSE;
}

PITEMID_CHILD ILCreateNetConnectItem(INetConnection * pItem)
{
    PITEMID_CHILD pidl;
    LPPIDLDATA pdata;

    pidl = _ILAlloc(0x99, sizeof(PIDLDATA));
    pdata = _ILGetDataPointer(pidl);
    pdata->u.value.pItem = pItem;

    return pidl;
}

const VALUEStruct * _ILGetValueStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(const_cast<LPITEMIDLIST>(pidl));

    if (pdata && pdata->type==0x99)
        return reinterpret_cast<const VALUEStruct*>(&pdata->u.value);

    return NULL;
}
