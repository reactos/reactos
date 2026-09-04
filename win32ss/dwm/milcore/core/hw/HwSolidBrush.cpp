// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidBrush implementation
//

#include "precomp.hpp"

MtDefine(CHwSolidBrush, MILRender, "CHwSolidBrush");


//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidBrush::CHwSolidBrush
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CHwSolidBrush::CHwSolidBrush(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : CHwBrush(pDevice),
        CHwConstantMilColorFColorSource(pDevice)
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidBrush::SetColor
//
//  Synopsis:  Called at the beginning of a rendering operation to set color.
//
//-----------------------------------------------------------------------------
void
CHwSolidBrush::SetColor(
    __in_ecount(1) MilColorF const &color
    )
{
    m_color = color;
}

//+---------------------------------------------------------------------------
//
//  Member:    CHwSolidBrush::SendOperations
//
//  Synopsis:  Send primary blend operations color source(s) to builder
//

HRESULT
CHwSolidBrush::SendOperations(
    __inout_ecount(1) CHwPipelineBuilder *pBuilder
    )
{
    HRESULT hr = S_OK;

    IFC(pBuilder->Set_Constant(this));

Cleanup:
    RRETURN(hr);
}




