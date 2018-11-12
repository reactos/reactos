/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CNetworkConnections Shell Folder items enumerator
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"


typedef struct tagGUIDStruct
{
    BYTE dummy; /* offset 01 is unknown */
    GUID guid;  /* offset 02 */
} GUIDStruct;

#define PT_GUID 0x1F

typedef struct tagPIDLDATA
{
    BYTE type;			/*00*/
    union
    {
        struct tagGUIDStruct guid;
        struct tagVALUEStruct value;
    } u;
} PIDLDATA, *LPPIDLDATA;

typedef struct tagENUMLIST
{
    struct tagENUMLIST *pNext;
    PITEMID_CHILD pidl;
} ENUMLIST, *LPENUMLIST;

class CEnumIDList:
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
    public:
        CEnumIDList();
        ~CEnumIDList();

        HRESULT Initialize();

        // IEnumIDList
        virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched);
        virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
        virtual HRESULT STDMETHODCALLTYPE Reset();
        virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum);

    private:
        BOOL AddToEnumList(PITEMID_CHILD pidl);

        LPENUMLIST  m_pFirst;
        LPENUMLIST  m_pLast;
        LPENUMLIST  m_pCurrent;

    public:
        DECLARE_NOT_AGGREGATABLE(CEnumIDList)
        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CEnumIDList)
            COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

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
CEnumIDList::Initialize()
{
    HRESULT hr;
    INetConnectionManager *pNetConMan;
    IEnumNetConnection *pEnumCon;
    INetConnection *INetCon;
    ULONG Count;
    PITEMID_CHILD pidl;

    /* get an instance to of IConnectionManager */
    hr = CNetConnectionManager_CreateInstance(IID_INetConnectionManager, (LPVOID*)&pNetConMan);
    if (FAILED(hr))
        return S_OK;

    hr = pNetConMan->EnumConnections(NCME_DEFAULT, &pEnumCon);
    if (FAILED(hr))
    {
        pNetConMan->Release();
        return S_OK;
    }

    do
    {
        hr = pEnumCon->Next(1, &INetCon, &Count);
        if (hr == S_OK)
        {
            pidl = ILCreateNetConnectItem(INetCon);
            if (pidl)
            {
                AddToEnumList(pidl);
            }
        }
        else
        {
            break;
        }
    } while (TRUE);

    pEnumCon->Release();
    pNetConMan->Release();

    return S_OK;    
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

HRESULT CEnumIDList_CreateInstance(HWND hwndOwner, DWORD dwFlags, REFIID riid, LPVOID * ppv)
{
    return ShellObjectCreatorInit<CEnumIDList>(riid, ppv);
}
