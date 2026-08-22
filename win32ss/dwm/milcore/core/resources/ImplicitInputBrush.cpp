// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Implicit input brush resource header.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(ImplicitInputBrushResource, MILRender, "ImplicitInputBrush Resource");

MtDefine(CMilImplicitInputBrushDuce, ImplicitInputBrushResource, "CMilImplicitInputBrushDuce");

CMilImplicitInputBrushDuce::~CMilImplicitInputBrushDuce()
{
    UnRegisterNotifiers();
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CMilImplicitInputBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      Realizes each property of the brush and sets it on the
//      cached realization
//
//-----------------------------------------------------------------------------

HRESULT
CMilImplicitInputBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    UNREFERENCED_PARAMETER(pBrushContext);
    
    // This brush is currently only used as a placeholder for Effects running on
    // the node they are attached to, so the brush itself should never have been allowed
    // to be used elsewhere.  If it is, we'll fill with an obnoxious pink color to make 
    // it more obvious that something is amiss.
    const MilColorF realizedColor = { 0.7f, 0, 0.7f, 0.7f };
   
    m_solidBrushRealization.SetColor(&realizedColor);

    *ppBrushRealizationNoRef = &m_solidBrushRealization;

    RRETURN(S_OK);
}



