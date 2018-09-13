//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       pfolder.cxx
//
//  Contents:   Implementation of IPersistFolder
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "guids.h"
#include "pfolder.hxx"
#include "shares.hxx"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesPF::GetClassID(
    LPCLSID lpClassID
    )
{
    CShares* This = IMPL(CShares,m_PersistFolder,this);
    *lpClassID = CLSID_CShares;
    return S_OK;
}


STDMETHODIMP
CSharesPF::Initialize(
    LPCITEMIDLIST pidl
    )
{
    CShares* This = IMPL(CShares,m_PersistFolder,this);
    This->m_pidl = ILClone(pidl);
    if (NULL == This->m_pidl)
    {
        return E_OUTOFMEMORY;
    }

    // Determine what share info level to use, based on which level succeeds.
    // NOTE: if this is being invoked remotely, we assume that IRemoteComputer
    // is invoked *before* IPersistFolder.

    // Try 2, then 1.
    if (IsLevelOk(This->m_pszMachine, 2))
    {
        This->m_level = 2;
    }
    else if (IsLevelOk(This->m_pszMachine, 1))
    {
        This->m_level = 1;
    }
    else
    {
        // error: can't enumerate
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    return S_OK;
}
