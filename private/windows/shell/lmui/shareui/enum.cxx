//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       enum.cxx
//
//  Contents:   Implementation of IEnumIDList
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "enum.hxx"
#include "util.hxx"


CSharesEnum::CSharesEnum(
    IN PWSTR pszMachine,
    IN DWORD level
    )
    :
    m_pszMachine(pszMachine),
    m_level(level),
    m_uFlags(0),
    m_pShares(NULL),
    m_cShares(0),
    m_dwEnumFlags(0),
    m_iCurrent(0),
    m_ulRefs(0)
{
    AddRef();
}


HRESULT
CSharesEnum::Init(
    ULONG uFlags
    )
{
    HRESULT hr = S_OK;
    LPBYTE pBuf = NULL;
    DWORD entriesread, totalentries;
    NET_API_STATUS ret = NERR_Success;

    m_uFlags = uFlags;

    //
    // Enumerate shares.
    //

    appAssert(m_level == 1 || m_level == 2);

    ret = NetShareEnum(
                m_pszMachine,
                m_level,
                &pBuf,
                0xffffffff,     // get them all!
                &entriesread,
                &totalentries,
                NULL);
    if (NERR_Success != ret)
    {
        hr = HRESULT_FROM_WIN32(ret);
    }
    else
    {
        appAssert(entriesread == totalentries);

        m_pShares = (SHARE_INFO_2*)pBuf;    // possibly level one info
        m_cShares = entriesread;

#ifdef WIZARDS
        m_dwEnumFlags = EF_SHOW_NEW_WIZARD;

        // Now, see if the machine is running the NetWare or Mac server.

        PSERVER_INFO_101 pServerInfo;
        NET_API_STATUS ret = NetServerGetInfo(m_pszMachine, 101, (LPBYTE*)&pServerInfo);
        if (NERR_Success != ret)
        {
            hr = HRESULT_FROM_WIN32(ret);
        }
        else
        {
            if (pServerInfo->sv101_type & SV_TYPE_AFP)
            {
                m_dwEnumFlags |= EF_SHOW_MAC_WIZARD;
            }
            if (pServerInfo->sv101_type & SV_TYPE_NOVELL)
            {
                m_dwEnumFlags |= EF_SHOW_NW_WIZARD;
            }

            // If *either* of the non-SMB servers are running, then all the
            // special "all" wizard object.

            if (m_dwEnumFlags & (EF_SHOW_MAC_WIZARD | EF_SHOW_NW_WIZARD))
            {
                m_dwEnumFlags |= EF_SHOW_ALL_WIZARD;
            }
        }
        NetApiBufferFree(pServerInfo);
#endif // WIZARDS
    }

    return hr;
}


CSharesEnum::~CSharesEnum()
{
    if (NULL != m_pShares)
    {
        NetApiBufferFree(m_pShares);
    }
}


STDMETHODIMP
CSharesEnum::Next(
    ULONG celt,
    LPITEMIDLIST* ppidlOut,
    ULONG* pceltFetched
    )
{
    HRESULT hr = S_OK;
    IDSHARE ids;
    ULONG celtFetched = 0;

    if (NULL == pceltFetched)
    {
        if (celt != 1)
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        *pceltFetched = 0;
    }

    if (celt == 0)
    {
        return S_OK;
    }

    if (!(m_uFlags & SHCONTF_NONFOLDERS))
    {
        return S_FALSE;
    }

CopyAnother:

    if (celtFetched == celt)
    {
        if (NULL != pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        return S_OK;    // got celt elements
    }

#ifdef WIZARDS
    if (0 != m_dwEnumFlags)
    {
        // We still have some special stuff to enumerate

        if (m_dwEnumFlags & EF_SHOW_NEW_WIZARD)
        {
            FillSpecialID(&ids, SHID_SHARE_NEW, IDS_SHARE_NEW);
            m_dwEnumFlags &= ~EF_SHOW_NEW_WIZARD;
            goto CopyOne;
        }

        if (m_dwEnumFlags & EF_SHOW_NW_WIZARD)
        {
            FillSpecialID(&ids, SHID_SHARE_NW, IDS_SHARE_NW);
            m_dwEnumFlags &= ~EF_SHOW_NW_WIZARD;
            goto CopyOne;
        }

        if (m_dwEnumFlags & EF_SHOW_MAC_WIZARD)
        {
            FillSpecialID(&ids, SHID_SHARE_MAC, IDS_SHARE_MAC);
            m_dwEnumFlags &= ~EF_SHOW_MAC_WIZARD;
            goto CopyOne;
        }

        if (m_dwEnumFlags & EF_SHOW_ALL_WIZARD)
        {
            FillSpecialID(&ids, SHID_SHARE_ALL, IDS_SHARE_ALL);
            m_dwEnumFlags &= ~EF_SHOW_ALL_WIZARD;
            goto CopyOne;
        }
    }
#endif // WIZARDS

    if (m_iCurrent >= m_cShares)
    {
        // already enumerated all of them
        if (NULL != pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        return S_FALSE; // didn't copy celt
    }

    switch (m_level)
    {
    case 1: FillID1(&ids, &(((LPSHARE_INFO_1)m_pShares)[m_iCurrent])); break;
    case 2: FillID2(&ids, &(((LPSHARE_INFO_2)m_pShares)[m_iCurrent])); break;
    default: appAssert(FALSE);
    }

    ++m_iCurrent;

#ifdef WIZARDS
CopyOne:
#endif // WIZARDS

    ppidlOut[celtFetched] = ILClone((LPCITEMIDLIST)(&ids));
    if (NULL == ppidlOut[celtFetched])
    {
        // free up everything so far
        for (ULONG i = 0; i < celtFetched; i++)
        {
            ILFree(ppidlOut[i]);
        }
        return E_OUTOFMEMORY;
    }

    ++celtFetched;
    goto CopyAnother;
}


STDMETHODIMP
CSharesEnum::Skip(
    ULONG celt
    )
{
    if (m_iCurrent >= m_cShares)
    {
        return S_FALSE;
    }

    m_iCurrent += celt;
    return NOERROR;
}


STDMETHODIMP
CSharesEnum::Reset(
    VOID
    )
{
    m_iCurrent = 0;
    return NOERROR;
}


STDMETHODIMP
CSharesEnum::Clone(
    IEnumIDList** ppenum
    )
{
    return E_FAIL;  // not supported
}
