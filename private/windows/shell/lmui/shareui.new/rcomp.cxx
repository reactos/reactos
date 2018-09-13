//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       rcomp.cxx
//
//  Contents:   Implementation of IRemoteComputer
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "guids.h"
#include "rcomp.hxx"
#include "shares.hxx"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CSharesRC::Initialize(
    LPWSTR pszMachine,
    BOOL bEnumerating
    )
{
    CShares* This = IMPL(CShares,m_RemoteComputer,this);

    if (NULL == pszMachine)
    {
        return E_INVALIDARG;
    }

    //
    // We only want to show the remote shares folder if it is an NT
    // server and the user has administrative access. That means that
    // NetShareEnum must pass at least level 2.
    //

    if (bEnumerating)
    {
        if (!IsLevelOk(pszMachine, 2))
        {
            return E_FAIL;
        }
    }

    This->m_pszMachine = NewDup(pszMachine);
    if (NULL == This->m_pszMachine)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
