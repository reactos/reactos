// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore render state. Contains renderstate methods accessible to
//  product code
//

#include "precomp.hpp"

MtDefine(CMILRenderState, MILApi, "CMILRenderState");

/*=========================================================================*\
    CMILRenderState - Render State for MIL
\*=========================================================================*/

CMILRenderState::CMILRenderState(__in_ecount_opt(1) CMILFactory *pFactory)
    : CMILObject(pFactory)
{
}

CMILRenderState::~CMILRenderState()
{
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILRenderState::Create
//
//  Synopsis:  Creation method for CMILRenderState object
//
//-----------------------------------------------------------------------------
__checkReturn HRESULT 
CMILRenderState::Create(
    __in_ecount_opt(1) CMILFactory *pFactory,
    __deref_out_ecount(1) CMILRenderState ** const ppRenderState
    )
{
    HRESULT hr = S_OK;

    CMILRenderState *pRenderState = new CMILRenderState(
        pFactory
        );
    IFCOOM(pRenderState);

    *ppRenderState = pRenderState;
    pRenderState->AddRef();

Cleanup:
    RRETURN(hr);
}

HRESULT CMILRenderState::SetSourceRectangle(__in_ecount_opt(1) const MilPointAndSizeL *prc)
{
    HRESULT hr = S_OK;

    if (prc == NULL)
    {
        m_RenderState.Options.SourceRectValid = FALSE;
    }
    else
    {
        m_RenderState.SourceRect = *prc;
        m_RenderState.Options.SourceRectValid = TRUE;
    }

    UpdateUniqueCount();

    RRETURN(hr);
}

// error C4995: 'D3DMATRIX': name was marked as #pragma deprecated
//
// Ignore deprecation of D3DMATRIX for this prototype because
// it is defined in the interface this class is implementing
#pragma warning (push)
#pragma warning (disable : 4995)
HRESULT CMILRenderState::SetLocalTransform(
    __in_ecount(1) const D3DMATRIX *pMatrix
    )
{
    HRESULT hr = S_OK;

    if (pMatrix == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        m_RenderState.LocalTransform = pMatrix;

        UpdateUniqueCount();
    }

    RRETURN(hr);
}

#pragma warning (pop)


HRESULT CMILRenderState::SetInterpolationMode(
    MilBitmapInterpolationMode::Enum interpolationMode)
{
    HRESULT hr = S_OK;

    if (m_RenderState.InterpolationMode != interpolationMode)
    {
        m_RenderState.InterpolationMode = interpolationMode;

        UpdateUniqueCount();
    }

    RRETURN(hr);
}

HRESULT CMILRenderState::SetAntiAliasMode(
    MilAntiAliasMode::Enum antiAliasMode)
{
    HRESULT hr = S_OK;

    if (m_RenderState.AntiAliasMode != antiAliasMode)
    {
        m_RenderState.AntiAliasMode = antiAliasMode;

        UpdateUniqueCount();
    }

    RRETURN(hr);
}


__out_ecount(1) CRenderState *
CMILRenderState::GetRenderStateInternal()
{
    return &m_RenderState;
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT CMILRenderState::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        // Call our base object's HrFindInterface
        hr = CMILObject::HrFindInterface(riid, ppvObject);
    }

    return hr;
}




