/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CNetworkConnections Shell Folder items enumerator
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

PNETCONIDSTRUCT ILGetConnData(PCITEMID_CHILD pidl)
{
    if (!pidl || !pidl->mkid.cb || pidl->mkid.abID[0] != 0x99)
        return NULL;
    return (PNETCONIDSTRUCT)(&pidl->mkid.abID[0]);
}

PWCHAR ILGetConnName(PCITEMID_CHILD pidl)
{
    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
        return NULL;
    return (PWCHAR)&pidl->mkid.abID[pdata->uNameOffset];
}

PWCHAR ILGetDeviceName(PCITEMID_CHILD pidl)
{
    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
        return NULL;
    return (PWCHAR)&pidl->mkid.abID[pdata->uDeviceNameOffset];
}

PITEMID_CHILD ILCreateNetConnectItem(INetConnection * pItem)
{
    PITEMID_CHILD pidl;
    ULONG_PTR size;
    NETCON_PROPERTIES * pProperties;
    PNETCONIDSTRUCT pnetid;
    PWCHAR pwchName;

    if (pItem->GetProperties(&pProperties) != S_OK)
        return NULL;

    size = sizeof(WORD); /* nr of bytes in this item */
    size += sizeof(NETCONIDSTRUCT);
    size += (wcslen(pProperties->pszwName) + 1) * sizeof(WCHAR);
    size += (wcslen(pProperties->pszwDeviceName) + 1) * sizeof(WCHAR);

    /* Allocate enough memory for the trailing id which will indicate that this is a simple id */
    pidl = static_cast<LPITEMIDLIST>(SHAlloc(size + sizeof(SHITEMID)));
    pidl->mkid.cb = (WORD)size;
    pidl->mkid.abID[0] = 0x99;

    /* Copy the connection properties */
    pnetid = ILGetConnData(pidl);
    pnetid->guidId = pProperties->guidId;
    pnetid->Status = pProperties->Status;
    pnetid->MediaType = pProperties->MediaType;
    pnetid->dwCharacter = pProperties->dwCharacter;
    pnetid->uNameOffset = sizeof(NETCONIDSTRUCT);
    pnetid->uDeviceNameOffset = pnetid->uNameOffset + (wcslen(pProperties->pszwName) + 1) * sizeof(WCHAR);

    pwchName = ILGetConnName(pidl);
    wcscpy(pwchName, pProperties->pszwName);

    pwchName = ILGetDeviceName(pidl);
    wcscpy(pwchName, pProperties->pszwDeviceName);

    /* Set the trailing id to null */
    memset((void*)((ULONG_PTR)pidl + size), 0, sizeof(SHITEMID));

    NcFreeNetconProperties(pProperties);

    return pidl;
}

HRESULT ILGetConnection(PCITEMID_CHILD pidl, INetConnection ** pItem)
{
    HRESULT hr;
    CComPtr<INetConnectionManager> pNetConMan;
    CComPtr<IEnumNetConnection> pEnumCon;
    CComPtr<INetConnection> INetCon;
    ULONG Count;
    NETCON_PROPERTIES * pProperties;

    PNETCONIDSTRUCT pdata = ILGetConnData(pidl);
    if (!pdata)
        return E_FAIL;

    /* get an instance to of IConnectionManager */
    hr = CNetConnectionManager_CreateInstance(IID_PPV_ARG(INetConnectionManager, &pNetConMan));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pNetConMan->EnumConnections(NCME_DEFAULT, &pEnumCon);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    while (TRUE)
    {
        hr = pEnumCon->Next(1, &INetCon, &Count);
        if (hr != S_OK)
            return E_FAIL;

        hr = INetCon->GetProperties(&pProperties);
        if (FAILED_UNEXPECTEDLY(hr))
            continue;

        BOOL bSame = !memcmp(&pProperties->guidId, &pdata->guidId, sizeof(GUID));

        NcFreeNetconProperties(pProperties);

        if (bSame)
        {
            *pItem = INetCon.Detach();
            return S_OK;
        }
    }

    return E_FAIL;
}

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
    CComPtr<INetConnectionManager> pNetConMan;
    CComPtr<IEnumNetConnection> pEnumCon;
    ULONG Count;
    PITEMID_CHILD pidl;

    /* get an instance to of IConnectionManager */
    hr = CNetConnectionManager_CreateInstance(IID_PPV_ARG(INetConnectionManager, &pNetConMan));
    if (FAILED_UNEXPECTEDLY(hr))
        return S_OK;

    hr = pNetConMan->EnumConnections(NCME_DEFAULT, &pEnumCon);
    if (FAILED_UNEXPECTEDLY(hr))
        return S_OK;

    while (TRUE)
    {
        CComPtr<INetConnection> INetCon;

        hr = pEnumCon->Next(1, &INetCon, &Count);
        if (hr != S_OK)
            break;

        pidl = ILCreateNetConnectItem(INetCon);
        if (pidl)
        {
            AddToEnumList(pidl);
        }
    }

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

HRESULT CEnumIDList_CreateInstance(HWND hwndOwner, DWORD dwFlags, REFIID riid, LPVOID * ppv)
{
    return ShellObjectCreatorInit<CEnumIDList>(riid, ppv);
}
