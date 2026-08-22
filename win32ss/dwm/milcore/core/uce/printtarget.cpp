// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    printtarget.cpp
//
//     The classes used to support printing are called generic for historical reasons.
//---------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSlaveGenericRenderTarget, MILRender, "CSlaveGenericRenderTarget");

//------------------------------------------------------------------
// CSlaveGenericRenderTarget::ctor
//------------------------------------------------------------------

CSlaveGenericRenderTarget::CSlaveGenericRenderTarget(
    CComposition *pComposition
    ) : CRenderTarget(pComposition)
{
    m_pRenderTarget = NULL;
    m_uiWidth = 0;
    m_uiHeight = 0;
}

//------------------------------------------------------------------
// CSlaveGenericRenderTarget::dtor
//------------------------------------------------------------------

CSlaveGenericRenderTarget::~CSlaveGenericRenderTarget()
{
    ReleaseInterface(m_pRenderTarget);  
}

//------------------------------------------------------------------
// CSlaveGenericRenderTarget::Render
//------------------------------------------------------------------

HRESULT CSlaveGenericRenderTarget::Render(
    __out_ecount(1) bool *pfPresentNeeded
    )
{
    HRESULT hr = S_OK;

    CDrawingContext *pDrawingContext = NULL;
    IFC(GetDrawingContext(&pDrawingContext));

    if (m_pRenderTarget != NULL && m_pRoot != NULL)
    {
        //
        // Render into our render target.
        // (Eventually we will also take dirty regions into account).

        // Don't clear, because if we are drawing to an image, we want to
        // preserve what's already there.
        // IFC(m_pRenderTarget->Clear(&m_clearColor));

        Assert(m_uiWidth <= INT_MAX);
        Assert(m_uiHeight <= INT_MAX);

        CMilRectF rcSurfaceBounds(
            0.0f,
            0.0f,
            static_cast<FLOAT>(m_uiWidth),
            static_cast<FLOAT>(m_uiHeight),
            XYWH_Parameters
            );

        if (!rcSurfaceBounds.IsEmpty())
        {
            BOOL fNeedsFullPresent = FALSE;

            IFC(pDrawingContext->BeginFrame(
                m_pRenderTarget
                DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Device)
                ));

            // Pass in NULL for the clear color so that we don't end up clearing out
            // the contents of the image.
            IFC(pDrawingContext->Render(
                    m_pRoot,
                    m_pRenderTarget, 
                    NULL, 
                    rcSurfaceBounds, 
                    TRUE,
                    0,      // No extra invalid regions
                    NULL,   // No extra invalid regions,
                    false,
                    &fNeedsFullPresent
                    ));

            pDrawingContext->EndFrame();
        }
    }
    else
    {
        Assert(FALSE);
    }

Cleanup:
    if (FAILED(hr))
    {
        ReleaseDrawingContext();
    }
    
    RRETURN(hr);
}
//+-----------------------------------------------------------------------
//
//  Member: CSlaveGenericRenderTarget::Present
//
//  Synopsis: Presents the completed rendering.  NOOP for surfaces
//
//  Returns:  S_OK always
//
//------------------------------------------------------------------------
HRESULT CSlaveGenericRenderTarget::Present()
{
    return S_OK;
}


//------------------------------------------------------------------
// CSlaveGenericRenderTarget::ProcessCreate
//------------------------------------------------------------------

HRESULT
CSlaveGenericRenderTarget::ProcessCreate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_GENERICTARGET_CREATE* pCmd
    )
{
    m_uiWidth = pCmd->width;
    m_uiHeight = pCmd->height;

    ReplaceInterface(m_pRenderTarget, (IMILRenderTarget*)pCmd->pRenderTarget);

    return S_OK;
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveGenericRenderTarget::Present
//
//  Synopsis: Returns the underlying render target.
//
//------------------------------------------------------------------------
HRESULT 
CSlaveGenericRenderTarget::GetBaseRenderTargetInternal(
    __deref_out_opt IRenderTargetInternal **ppIRT
    )
{
    HRESULT hr = S_OK;
    
    if (m_pRenderTarget != NULL)
    {
        IFC(m_pRenderTarget->QueryInterface(IID_IRenderTargetInternal, (void **)ppIRT));
    }
    else
    {
        *ppIRT = NULL;
    }

Cleanup:
    RRETURN(hr);
}


