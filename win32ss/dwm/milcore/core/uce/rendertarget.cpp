// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Abstract:
//
//     The classes is used as the base class for other render targets like 
//     HwndTarget, SurfTarget, PrintTarget
//---------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CRenderTarget, MILRender, "CRenderTarget");

//------------------------------------------------------------------
// CRenderTarget::ctor
//------------------------------------------------------------------

CRenderTarget::CRenderTarget(
    __in_ecount(1) CComposition *pComposition
    ) : m_pComposition(pComposition)
{
    // The DrawingContext Addrefs and Releases the m_pComposition
    // so we do not AddRef it here.

    m_pDrawingContext = NULL;
 }

//------------------------------------------------------------------
// CRenderTarget::dtor
//------------------------------------------------------------------

CRenderTarget::~CRenderTarget()
{
    // The DrawingContext Addrefs and Releases the m_pComposition
    // so we do not release it here.

    UnRegisterNotifier(m_pRoot);
    ReleaseDrawingContext();
}

//+----------------------------------------------------------------------------------
//
//  Member:     CRenderTarget::Initialize
//
//  Synopsis:   Initializes this object by instantiating the DrawingContext
//
//----------------------------------------------------------------------------------
HRESULT 
CRenderTarget::Initialize(
    __in_ecount(1) CComposition *pComposition   // Composition to associate with the content
    )
{
    HRESULT hr = S_OK;
    
    Assert(m_pDrawingContext == NULL);
    IFC(CDrawingContext::Create(pComposition, &m_pDrawingContext));

Cleanup:
    // Release any resources obtained upon failure
    if (FAILED(hr))
    {
        ReleaseDrawingContext();
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CRenderTarget::ReleaseDrawingContext
//---------------------------------------------------------------------------------
void 
CRenderTarget::ReleaseDrawingContext()
{
    if (m_pDrawingContext != nullptr)
    {
        m_pDrawingContext->UpdateDpiProvider(nullptr);
    }
    ReleaseInterface(m_pDrawingContext);
}

//---------------------------------------------------------------------------------
// CRenderTarget::GetDrawingContext
//---------------------------------------------------------------------------------

HRESULT 
CRenderTarget::GetDrawingContext(
    __deref_out_ecount(1) CDrawingContext **pDrawingContext,
    bool allowCreation
    )
{
    HRESULT hr = S_OK;

    if (m_pDrawingContext == NULL)
    {
        if (allowCreation)
        {
            IFC(Initialize(m_pComposition));   
        }
        else
        {
            //
            // Allow callers to enforce the pre-existence of an
            // initialized CDrawingContext if they desire.
            //
            IFC(E_UNEXPECTED);
        }
    }

    // Sometimes, a render target is also a DpiProvider. 
    // If the current one is such an RT, then pass it along to the Drawing Context
    IDpiProvider* pDpiProvider = nullptr;
    if (SUCCEEDED(QueryInterface(IID_PPV_ARGS(&pDpiProvider))))
    {
        if (m_pDrawingContext != nullptr)
        {
            m_pDrawingContext->UpdateDpiProvider(pDpiProvider);
        }
        ReleaseInterface(pDpiProvider);
    }

    *pDrawingContext = m_pDrawingContext;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CRenderTarget::ProcessSetRoot
//---------------------------------------------------------------------------------

HRESULT
CRenderTarget::ProcessSetRoot(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_ecount(1) const MILCMD_TARGET_SETROOT *pctx
    )
{
    HRESULT hr = S_OK;

    UnRegisterNotifier(m_pRoot);

    if (pctx->hRoot != NULL)
    {
        CMilVisual* pRoot =
            static_cast<CMilVisual*>(pHandleTable->GetResource(
                pctx->hRoot, 
                TYPE_VISUAL
                ));

        if (pRoot == NULL)
        {
            RIP("Invalid composition node handle in MILCMD_TARGET_SETROOT.");
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        IFC(RegisterNotifier(pRoot));
        m_pRoot = pRoot;
    }

Cleanup:
    RRETURN(hr);
}

