// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_material
//      $Keywords:
//
//  $Description:
//      Contains CHwColorComponentSource implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(CHwColorComponentSource, MILRender, "CHwDiffuseComponentColorSource");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwColorComponentSource::Create
//
//  Synopsis:
//      Creates an instance.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwColorComponentSource::Create(
    CHwColorComponentSource::VertexComponent eComponent,
    __deref_out_ecount(1) CHwColorComponentSource ** const ppHwColorSource
    )
{
    HRESULT hr = S_OK;
    CHwColorComponentSource *pNewColorSource = NULL;

    pNewColorSource = new CHwColorComponentSource;
    IFCOOM(pNewColorSource);

    pNewColorSource->AddRef();

    pNewColorSource->SetComponentLocation(eComponent);

    *ppHwColorSource = pNewColorSource;
    pNewColorSource = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewColorSource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwColorComponentSource::ctor
//
//  Synopsis:
//      Initializes members.
//
//------------------------------------------------------------------------------

CHwColorComponentSource::CHwColorComponentSource()
{
    m_eSourceLocation = CHwColorComponentSource::Total;

    ResetForPipelineReuse();
}



