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
#include "shri.hxx"


CSharesEnum::CSharesEnum(
    IN PWSTR pszMachine,
    IN DWORD level
    )
    :
    m_pszMachine(pszMachine),
    m_level(level),
    m_uFlags(0),
    m_bDoSmb(TRUE),
    m_bDoFpnw(FALSE),
    m_bDoSfm(FALSE),
    m_pShares(NULL),
    m_cShares(0),
    m_pSfmShares(NULL),
    m_cSfmShares(0),
    m_pFpnwShares(NULL),
    m_cFpnwShares(0),
    m_ulRefs(0),
	m_pShareList(NULL),
	m_pShareCurrent(NULL)
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
    DWORD svtype = 0;
    DWORD err;

    m_uFlags = uFlags;

    // See what servers are running: SMB, FPNW, and/or SFM

    PSERVER_INFO_101 pServerInfo;
    ret = NetServerGetInfo(m_pszMachine, 101, (LPBYTE*)&pServerInfo);
    if (NERR_Success != ret)
    {
		return HRESULT_FROM_WIN32(ret);
    }
    else
    {
        svtype = pServerInfo->sv101_type;
        NetApiBufferFree(pServerInfo);

        if (svtype & SV_TYPE_AFP && LoadSFMSupportLibrary())
        {
            m_bDoSfm = TRUE;
        }
        if (svtype & SV_TYPE_SERVER_MFPN && LoadFPNWSupportLibrary())
        {
            m_bDoFpnw = TRUE;
        }
    }

    //
    // Enumerate shares from all file servers. Do SMB first.
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
        appDebugOut((DEB_ERROR, "NetShareEnum(%ws...) failed! Error = 0x%08lx\n", m_pszMachine, ret));
        hr = HRESULT_FROM_WIN32(ret);
    }
    else
    {
        appAssert(entriesread == totalentries);
        appAssert(0 == entriesread || NULL != pBuf);
        m_pShares = (SHARE_INFO_2*)pBuf;    // possibly level one info
        m_cShares = entriesread;
    }

    //
    // Do SFM shares
    //

    if (m_bDoSfm)
    {
        AFP_SERVER_HANDLE hServer;
        err = (*g_pfnAfpAdminConnect)(m_pszMachine, &hServer);
        if (NO_ERROR == err)
        {
            err = (*g_pfnAfpAdminVolumeEnum)(
                            hServer,
                            &pBuf,
                            MAX_PREFERRED_LENGTH,
                            &entriesread,
                            &totalentries,
                            NULL);
            if (NO_ERROR == err)
            {
                appAssert(entriesread == totalentries);
                appAssert(0 == entriesread || NULL != pBuf);
                m_pSfmShares = (AFP_VOLUME_INFO*)pBuf;
                m_cSfmShares = entriesread;

                (*g_pfnAfpAdminDisconnect)(hServer);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(err);
                appDebugOut((DEB_ERROR, "AfpAdminVolumeEnum(%ws...) failed! Error = 0x%08lx\n", m_pszMachine, err));
            }
        }
        else
        {
            appDebugOut((DEB_ERROR, "AfpAdminConnect(%ws...) failed! Error = 0x%08lx\n", m_pszMachine, err));
            hr = HRESULT_FROM_WIN32(err);
        }
    }

    //
    // Do FPNW shares
    //

    if (m_bDoFpnw)
    {
        // BUGBUG: do level 1 or level 2 based on security?

        err = (*g_pfnFpnwVolumeEnum)(
                        m_pszMachine,
                        1,   // level
                        &pBuf,
                        &entriesread,
                        NULL);
        if (NO_ERROR == err)
        {
            appAssert(0 == entriesread || NULL != pBuf);
            m_pFpnwShares = (FPNWVOLUMEINFO*)pBuf;
            m_cFpnwShares = entriesread;
        }
        else
        {
            appDebugOut((DEB_ERROR, "FpnwVolumeEnum(%ws...) failed! Error = 0x%08lx\n", m_pszMachine, err));
            hr = HRESULT_FROM_WIN32(err);
        }
    }

	// Now create a linked list of shares. This just makes them easier to
	// handle, though is arguably a waste of memory.

	m_pShareList = new CShare();	// dummy head node
	if (NULL == m_pShareList)
	{
		return E_OUTOFMEMORY;
	}

	DWORD i;
	for (i = 0; i < m_cShares; i++)
	{
		CShare* ptmp = new CShare();
		if (NULL == ptmp)
		{
			return E_OUTOFMEMORY;
		}

		ptmp->AddSmb(&m_pShares[i]);
		ptmp->InsertBefore(m_pShareList);
	}

	for (i = 0; i < m_cSfmShares; i++)
	{
		CShare* ptmp = new CShare();
		if (NULL == ptmp)
		{
			return E_OUTOFMEMORY;
		}

		ptmp->AddSfm(&m_pSfmShares[i]);
		ptmp->InsertBefore(m_pShareList);
	}

	for (i = 0; i < m_cFpnwShares; i++)
	{
		CShare* ptmp = new CShare();
		if (NULL == ptmp)
		{
			return E_OUTOFMEMORY;
		}

		ptmp->AddFpnw(&m_pFpnwShares[i]);
		ptmp->InsertBefore(m_pShareList);
	}

	m_pShareCurrent = (CShare*)m_pShareList->Next();
    return S_OK;
}


CSharesEnum::~CSharesEnum()
{
    if (NULL != m_pShares)
    {
        NetApiBufferFree(m_pShares);
    }
    if (NULL != m_pSfmShares)
    {
        (*g_pfnAfpAdminBufferFree)(m_pSfmShares);
    }
    if (NULL != m_pFpnwShares)
    {
        DWORD err = (*g_pfnFpnwApiBufferFree)(m_pFpnwShares);
        if (NO_ERROR != err)
        {
            appDebugOut((DEB_ERROR, "FpnwApiBufferFree() failed! Error = 0x%08lx\n", err));
        }
    }

	CShare* pCurrent = (CShare*) m_pShareList->Next();
	while (pCurrent != m_pShareList)
	{
		CShare* pTmp = pCurrent;
		pCurrent = (CShare*) pCurrent->Next();
		pTmp->Unlink();
		delete pTmp;
	}
	delete m_pShareList;
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
    CShare* pShare;

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

    // Do we have anything else left?

    if (m_pShareCurrent == m_pShareList)	// we're done iterating
    {
        if (NULL != pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        return S_FALSE; // didn't copy celt
    }

    // Get the next share
    pShare = m_pShareCurrent;
    appAssert(NULL != pShare);
    pShare->FillID(&ids);
	m_pShareCurrent = (CShare*) m_pShareCurrent->Next();

// CopyOne:

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
    return E_NOTIMPL;
}


STDMETHODIMP
CSharesEnum::Reset(
    VOID
    )
{
    return E_NOTIMPL;
}


STDMETHODIMP
CSharesEnum::Clone(
    IEnumIDList** ppenum
    )
{
    return E_NOTIMPL;
}
