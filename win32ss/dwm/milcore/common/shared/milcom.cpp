// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#include "precomp.hpp"

/*=========================================================================*\
    CMILCOMBase - Base COM object for MIL
\*=========================================================================*/

CMILCOMBase::CMILCOMBase()
{
    m_cRef = 0;
}

CMILCOMBase::~CMILCOMBase()
{
}

ULONG CMILCOMBase::InternalAddRef()
{
    if (m_cRef < 0)
    {
        //
        // See comments in CMILComBase::InternalRelease
        //        
        FreRIPW(L"Tried to AddRef an object which has previously been freed (refcount went to 0).");
    }

    return InterlockedIncrement(&m_cRef);
}

ULONG CMILCOMBase::InternalRelease()
{
    AssertConstMsgW(
        m_cRef > 0,
        L"Attempt to release an object with 0 or less references! Possible memory leak."
        );

    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
        // Setting m_cRef count here
        // before destruction allows us to catch the stack attempting the AddRef in CMILCOMBase::InternalAddRef()
        //
        m_cRef--;
        delete this;
    }

    return cRef;
}

HRESULT CMILCOMBase::InternalQueryInterface(
    __in_ecount(1) REFIID riid,
    __deref_opt_out VOID **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IUnknown)
        {
            *ppvObject = static_cast<IUnknown*>(this);

            hr = S_OK;
        }
        else 
        {
            hr = HrFindInterface(riid, ppvObject);
        }
        
        if (SUCCEEDED(hr))
        {
            // This is necessary because the stream object is
            // implemented using a proxy wrapper and it uses the QI mechanism
            // to retrieve the internal stream interface (for historical 
            // reasons). For normal objects, ppvObject should be the correct
            // pointer anyway, so this should work, however it would be cleaner
            // if we were able to simply call AddRef() for this object.
            
            static_cast<IUnknown*>(*ppvObject)->AddRef();
        }
        else
        {
            // Make sure ppvObject is always set, per QI specification.
            *ppvObject = NULL;
        }
    }
    
    return hr;
}



