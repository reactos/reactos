// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//    It contains the alpha mask and the bounds of the node in inner space
//    which has this alpha mask. We only want to store these bounds if a 
//    node has alpha mask and so we have this wrapper class.
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilAlphaMaskWrapper, Mem, "CMilAlphaMaskWrapper");

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::CMilAlphaMaskWrapper
//
//  Synopsis:   Constructor.
//
//------------------------------------------------------------------------
CMilAlphaMaskWrapper::CMilAlphaMaskWrapper(
    )
{
    m_previousInnerBounds = CMilRectF::sc_rcEmpty;
    m_previousOuterBounds = CMilRectF::sc_rcEmpty;    
    m_pAlphaMask = NULL;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::~CMilAlphaMaskWrapper
//
//  Synopsis:   Destructor
//
//------------------------------------------------------------------------
CMilAlphaMaskWrapper::~CMilAlphaMaskWrapper()
{
    ReleaseInterfaceNoNULL(m_pAlphaMask);
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::Create
//
//  Synopsis:   Instantiates and initializes a CMilAlphaMaskWrapper
//              instance.
//
//------------------------------------------------------------------------
HRESULT 
CMilAlphaMaskWrapper::Create(
    __deref_out_ecount(1) CMilAlphaMaskWrapper **ppAlphaMask
    )
{
    Assert(ppAlphaMask);
    
    HRESULT hr = S_OK;
    
    CMilAlphaMaskWrapper *pNewInstance = NULL;
        
    // Instantiate the wrapper
    pNewInstance = new CMilAlphaMaskWrapper();
    IFCOOM(pNewInstance);

    // Set out-param to initialized instance
    *ppAlphaMask = pNewInstance;

    // Avoid deletion during Cleanup
    pNewInstance = NULL;

Cleanup:

    // Free any allocations that weren't set to NULL due to failure
    delete pNewInstance;

    RRETURN(hr);    
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::GetVisualPreviousInnerBounds
//
//  Synopsis:   Get the bounds of the node which has alpha mask
//
//------------------------------------------------------------------------
void 
CMilAlphaMaskWrapper::GetVisualPreviousInnerBounds(
    __out_ecount(1) CMilRectF *prcBounds
    ) const
{
    *prcBounds = m_previousInnerBounds;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::SetVisualPreviousInnerBounds
//
//  Synopsis:   Set the bounds of the node which has alpha mask
//
//------------------------------------------------------------------------
void 
CMilAlphaMaskWrapper::SetVisualPreviousInnerBounds(
    __in_ecount(1) const CMilRectF &prcBounds
    )
{
    m_previousInnerBounds = prcBounds;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::GetVisualPreviousOuterBounds
//
//  Synopsis:   Get the bounds of the node which has alpha mask
//
//------------------------------------------------------------------------
void 
CMilAlphaMaskWrapper::GetVisualPreviousOuterBounds(
    __out_ecount(1) CMilRectF *prcBounds
    ) const
{
    *prcBounds = m_previousOuterBounds;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::SetVisualPreviousOuterBounds
//
//  Synopsis:   Set the bounds of the node which has alpha mask
//
//------------------------------------------------------------------------
void 
CMilAlphaMaskWrapper::SetVisualPreviousOuterBounds(
    __in_ecount(1) const CMilRectF &prcBounds
    )
{
    m_previousOuterBounds = prcBounds;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMilAlphaMaskWrapper::SetAlphaMask
//
//  Synopsis:   Set the alpha mask for this node
//
//------------------------------------------------------------------------
void 
CMilAlphaMaskWrapper::SetAlphaMask(
    __in_ecount_opt(1) CMilBrushDuce *pAlphaMask
    )
{
    // decrease the refcount for the previous alpha mask
    ReleaseInterfaceNoNULL(m_pAlphaMask);

    m_pAlphaMask = pAlphaMask;

    // add ref count for the new alpha mask
    if (m_pAlphaMask)
    {
        m_pAlphaMask->AddRef();
    }
}


