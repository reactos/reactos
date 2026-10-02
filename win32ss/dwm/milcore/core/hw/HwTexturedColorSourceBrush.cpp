// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTexturedColorSourceBrush implementation
//

#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Function:  CHwTexturedColorSourceBrush::CHwTexturedColorSourceBrush
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CHwTexturedColorSourceBrush::CHwTexturedColorSourceBrush(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwTexturedColorSource *pTexturedSource
    ) : CHwBrush(pDevice),
        m_pTexturedSource(pTexturedSource)
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwTexturedColorSourceBrush::~CHwTexturedColorSourceBrush
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CHwTexturedColorSourceBrush::~CHwTexturedColorSourceBrush()
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwTexturedColorSourceBrush::AddRef
//
//  Synopsis:  Illegal method for this type of brush
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CHwTexturedColorSourceBrush::AddRef()
{
    RIP("CHwTexturedColorSourceBrush should not be AddRef'ed.");
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwTexturedColorSourceBrush::Release
//
//  Synopsis:  Illegal method for this type of brush
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CHwTexturedColorSourceBrush::Release()
{
    RIP("CHwTexturedColorSourceBrush should not be Release'd.");
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSourceBrush::SendOperations
//
//  Synopsis:  Send primary blend operations color source(s) to builder
//

HRESULT
CHwTexturedColorSourceBrush::SendOperations(
    __inout_ecount(1) CHwPipelineBuilder *pBuilder
    )
{
    HRESULT hr = S_OK;

    CHwBoxColorSource *pMaskColorSource = NULL;
    
    Assert(m_pTexturedSource);

    IFC(pBuilder->Set_Texture(m_pTexturedSource));
    
    IFC(m_pTexturedSource->GetMaskColorSource(&pMaskColorSource));
    
    if (pMaskColorSource)
    {
        pMaskColorSource->ResetAlphaScaleFactor();

        IFC(pBuilder->Mul_AlphaMask(
            pMaskColorSource
            ));
    }
Cleanup:
    ReleaseInterfaceNoNULL(pMaskColorSource);
    
    RRETURN(hr);
}



