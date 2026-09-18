// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    File: api_base.cpp

    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"

/*=========================================================================*\
    CMILObject - MIL base object interface
\*=========================================================================*/

CMILObject::CMILObject(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : m_pFactory(pFactory)
{
    CAssertDllInUse::Enter();
}

CMILObject::~CMILObject()
{
    CAssertDllInUse::Leave();
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT CMILObject::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}


