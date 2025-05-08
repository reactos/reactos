// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//     Implementation of the content bounder.  
//
//      2005/01/19
//      Moved GetVisualInnerBounds from CMilRenderContext to this 
//      class.  Added lazy initialization of the render context & target.
//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CContentBounder, Mem, "CContentBounder");

//+-----------------------------------------------------------------------
//
//  Member:     CContentBounder::CContentBounder
//
//  Synopsis:   Constructor.
//
//------------------------------------------------------------------------
CContentBounder::CContentBounder(
    __in_ecount(1) CComposition *pComposition
    )
{
    m_pBoundsRenderTarget = NULL;
    m_pDrawingContext = NULL;
    
    // Save pComposition so the render context can be lazily initialized if & when
    // GetContentBounds is called.
    m_pComposition = pComposition;

    WHEN_DBG(m_fInUse = FALSE);
}

//+-----------------------------------------------------------------------
//
//  Member:     CContentBounder::CContentBounder
//
//  Synopsis:   Destructor
//
//------------------------------------------------------------------------
CContentBounder::~CContentBounder()
{
    ReleaseInterfaceNoNULL(m_pDrawingContext);
    ReleaseInterfaceNoNULL(m_pBoundsRenderTarget);
}

//+-----------------------------------------------------------------------
//
//  Member:     CContentBounder::Create
//
//  Synopsis:   Instantiates and initializes a CContentBounder
//              instance.
//
//------------------------------------------------------------------------
HRESULT 
CContentBounder::Create(
    __in_ecount(1) CComposition *pComposition, 
    __deref_out_ecount(1) CContentBounder **ppContentBounder
    )
{
    Assert(ppContentBounder);
    
    HRESULT hr = S_OK;
    
    CContentBounder *pNewInstance = NULL;

    // Initialize out-param to NULL
    *ppContentBounder = NULL;
        
    // Instantiate the content bounder
    pNewInstance = new CContentBounder(pComposition);
    IFCOOM(pNewInstance);

    // Set out-param to initialized instance
    *ppContentBounder = pNewInstance;

    // Avoid deletion during Cleanup
    pNewInstance = NULL;

Cleanup:

    // Free any allocations that weren't set to NULL due to failure
    delete pNewInstance;

    RRETURN(hr);    
}
    
//+----------------------------------------------------------------------------------
//
//  Member:     CContentBounder::Initialize
//
//  Synopsis:   Initializes this object by instantiating the bounds
//              render target and render context.
//
//----------------------------------------------------------------------------------
HRESULT 
CContentBounder::Initialize(
    __in_ecount(1) CComposition *pComposition   // Composition to associate with the content
    )
{  
    // Initialize shouldn't be called successfully twice
    Assert(NULL == m_pBoundsRenderTarget);
    Assert(NULL == m_pDrawingContext);

    HRESULT hr = S_OK;

    // Instantiate bounds render target & render context
    IFC(CSwRenderTargetGetBounds::Create(&m_pBoundsRenderTarget));
    IFC(CDrawingContext::Create(pComposition, &m_pDrawingContext));
    
Cleanup:

    // Release any resources obtained upon failure
    if (FAILED(hr))
    {
        ReleaseInterface(m_pDrawingContext);
        
        ReleaseInterface(m_pBoundsRenderTarget);
    }

    RRETURN(hr);    
}

//+-----------------------------------------------------------------------
//
//  Member:     CContentBounder::GetContentBounds
//
//  Synopsis:   Retrieves the bounds of the passed in content using a
//              bounds render target & context.
//
//------------------------------------------------------------------------
HRESULT 
CContentBounder::GetContentBounds(
    __in_ecount_opt(1) CMilSlaveResource *pContent,  // Content to retrieve bounds for
    __out_ecount(1) CMilRectF* prcBounds     // Output bounds of content
    )
{   
    HRESULT hr = S_OK;

    // Assert that this object isn't already in a GetContentBounds call
    WHEN_DBG(Assert(!m_fInUse));

    // Set debug-only in-use flag
    WHEN_DBG(m_fInUse = TRUE);

    // Always initialize out-param to 0,0,0,0
    prcBounds->SetEmpty();

    // Retrieve the bounds if the content is non-NULL
    if (NULL != pContent)
    {               
        // Lazily allocate the render target & context.  
        //
        // This avoids maintaining a full render context & target in-memory until
        // the object is used. More importantly, this also breaks cyclic dependendies
        // caused by the fact a CMilRenderContext contains a CContentBounder
        // and vice versa.
        if (NULL == m_pDrawingContext)
        {
            // We assume that both the render context & target are initialized together,
            // so we only need to check one variable in fre builds.  Guard this assumption
            // in chk builds.
            Assert (NULL == m_pBoundsRenderTarget);

            IFC(Initialize(m_pComposition));   
        }        

        // Initialize must be called successfully 
        Assert(m_pBoundsRenderTarget && m_pDrawingContext); 

        //
        // Draw content into bounds render target
        //
        
        IFC(m_pDrawingContext->BeginFrame(
            m_pBoundsRenderTarget
            DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::PageInPixels)
            ));

        if (pContent->IsOfType(TYPE_RENDERDATA))
        {
            // Cast to RenderData & Draw
            CMilSlaveRenderData *pRenderData = DYNCAST(CMilSlaveRenderData, pContent);
            Assert(pRenderData);

            IFC(pRenderData->Draw(m_pDrawingContext));
        }
        else if (pContent->IsOfType(TYPE_DRAWING))
        {
            // Cast to Drawing & Draw
            CMilDrawingDuce *pDrawing = DYNCAST(CMilDrawingDuce, pContent);           
            Assert(pDrawing);

            IFC(pDrawing->Draw(m_pDrawingContext));                 
        }
        else if (pContent->IsOfType(TYPE_VIEWPORT3DVISUAL))
        {
            // Cast to a Viewport3DVisual & Render
            CMilViewport3DVisual* pViewport = DYNCAST(CMilViewport3DVisual, pContent);
            Assert(pViewport);

            IFC(pViewport->RenderContent(m_pDrawingContext));              
        }
        else
        {
            // Invalid content type            
            Assert(FALSE);
            IFC(E_INVALIDARG);
        }

        *prcBounds = m_pBoundsRenderTarget->GetAccumulatedBounds();
        if (!(prcBounds->IsWellOrdered()))
        {
            *prcBounds = CMilRectF::sc_rcInfinite;
        }

    }

Cleanup:

    //
    // Always reset the DrawingContext & RenderTarget, if they were successfully allocated
    //
    
    if (m_pDrawingContext)
    {
        m_pDrawingContext->EndFrame();
    }

    if (m_pBoundsRenderTarget)
    {
        m_pBoundsRenderTarget->ResetBounds();
    }

    WHEN_DBG(m_fInUse = FALSE);    

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CMilRenderContext::GetVisualInnerBounds
//
//  Synopsis:   Obtains the inner (non-transformed) bounds of a visual's
//              content, combined with the bounds of it's subgraph
//
//------------------------------------------------------------------------------
HRESULT
CContentBounder::GetVisualInnerBounds(
    __in_ecount(1) CMilVisual* pNode,    // Node to obtain bounds for
    __out_ecount(1) CMilRectF *prcBounds         // Output bounds of pNode
    )
{
    HRESULT hr = S_OK;

    CMilRectF rcBounds;

    //
    // Calculate the inner bounds of the content.  pNode->m_Bounds contains the bounds
    // of the node transformed into it's parent's coordinate system (i.e., it's 
    // outer bounds), but we need the inner-bounds of the content.  Thus, we can't 
    // use m_Bounds and must walk the content bounds
    //
    IFC(pNode->GetContentBounds(this, &rcBounds));

    // Walk the children and union in their bounds.
    // 
    // The cached bounds (m_Bounds) of the children contain their subgraph bounds
    // transformed into this node's coordinate inner space.
    UINT childCount = static_cast<UINT>(pNode->m_rgpChildren.GetCount());
    for (UINT i = 0; i < childCount; i++)
    {
        const CMilVisual *pChild = pNode->m_rgpChildren[i];
        Assert(pChild);

        rcBounds.Union(pChild->m_Bounds);
    }

    // Set out-param upon success
    *prcBounds = rcBounds;

Cleanup:

    RRETURN(hr);
}


