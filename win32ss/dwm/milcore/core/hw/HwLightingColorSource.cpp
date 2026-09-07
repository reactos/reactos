// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $Description:
//      Contains CHwLightingColorSource implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(CHwLightingColorSource, MILRender, "CHwLightingColorSource");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwLightingColorSource::Create
//
//  Synopsis:
//      Creates an instance.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwLightingColorSource::Create(
    __in_ecount(1) CMILLightData const *pLightData,
    __deref_out_ecount(1) CHwLightingColorSource ** const ppHwLightingColorSource
    )
{
    HRESULT hr = S_OK;
    CHwLightingColorSource *pNewHwLightColorSource = NULL;

    pNewHwLightColorSource = new CHwLightingColorSource(pLightData);
    IFCOOM(pNewHwLightColorSource);

    pNewHwLightColorSource->AddRef();

    *ppHwLightingColorSource = pNewHwLightColorSource;
    pNewHwLightColorSource = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewHwLightColorSource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwLightingColorSource::SendShaderData
//
//  Synopsis:
//      Send necessary data to the shader.
//
//------------------------------------------------------------------------------
HRESULT
CHwLightingColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pHwShader
    )
{
    HRESULT hr = S_OK;

    Assert(m_hParameter != MILSP_INVALID_HANDLE);

    IFC(m_pLightDataNoRef->SendShaderData(
        m_hParameter,
        pHwShader
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwLightingColorSource::ctor
//
//  Synopsis:
//      Initializes members.
//
//------------------------------------------------------------------------------
CHwLightingColorSource::CHwLightingColorSource(
    __in_ecount(1) CMILLightData const *pLightData
    )
    : m_pLightDataNoRef(pLightData)
{
    ResetForPipelineReuse();
}



