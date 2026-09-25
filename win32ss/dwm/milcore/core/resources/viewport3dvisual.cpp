// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      Viewport3DVisual.cpp
//
// Abstract: 
//      Viewport3DVisual resource.
//
//---------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilViewport3DVisual, MILRender, "CMilViewport3DVisual");

//---------------------------------------------------------------------------------
//
// class CMilViewport3DVisual
//
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// CMilVisual::dtor
//---------------------------------------------------------------------------------

CMilViewport3DVisual::~CMilViewport3DVisual()
{
    UnRegisterNotifier(m_pCamera);

    if (m_pChild != NULL)
    {
        m_pChild->SetParent(NULL);
        m_pChild->Release();
    }
}

// ----------------------------------------------------------------------------
//
//   Command handlers
//
// ----------------------------------------------------------------------------

override HRESULT
CMilViewport3DVisual::ProcessRemoveAllChildren(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_REMOVEALLCHILDREN* pCmd
    )
{
    RIP("Unexpected MILCMD_VISUAL_REMOVEALLCHILDREN on CMilViewport3DVisual.");
    RRETURN(E_UNEXPECTED);
}

override HRESULT 
CMilViewport3DVisual::ProcessRemoveChild(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_REMOVECHILD* pCmd
    )
{
    RIP("Unexpected MILCMD_VISUAL_REMOVEALLCHILDREN on CMilViewport3DVisual.");
    RRETURN(E_UNEXPECTED);
}

override HRESULT 
CMilViewport3DVisual::ProcessInsertChildAt(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_INSERTCHILDAT* pCmd
    )
{
    RIP("Unexpected MILCMD_VISUAL_REMOVEALLCHILDREN on CMilViewport3DVisual.");
    RRETURN(E_UNEXPECTED);
}

HRESULT 
CMilViewport3DVisual::ProcessSetCamera(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SETCAMERA* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource
    CMilCameraDuce *pCamera = NULL;

    if (pCmd->hCamera != HMIL_RESOURCE_NULL)
    {
        pCamera =
            static_cast<CMilCameraDuce*>(pHandleTable->GetResource(
                pCmd->hCamera, 
                TYPE_CAMERA
                ));

        if (pCamera == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (pCamera != m_pCamera)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pCamera));
        UnRegisterNotifier(m_pCamera);
        m_pCamera = pCamera;

        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(/* pNode = */ this, /* fNeedsBoundingBoxUpdate = */ TRUE, /* fDirtyForRender = */ TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CMilViewport3DVisual::ProcessSetViewport(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT* pCmd
    )
{
    MilPointAndSizeD viewport = pCmd->Viewport;

    if (viewport.X != m_viewport.X
        || viewport.Y != m_viewport.Y
        || viewport.Width != m_viewport.Width
        || viewport.Height != m_viewport.Height)
    {
        m_viewport = viewport;
        CMilVisual::PropagateFlags(/* pNode = */ this, /* fNeedsBoundingBoxUpdate = */ TRUE, /* fDirtyForRender = */ TRUE);
    }            

    return S_OK;
}

HRESULT 
CMilViewport3DVisual::ProcessSet3DChild(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SET3DCHILD* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisual3D *pChild =
        static_cast<CMilVisual3D*>(pHandleTable->GetResource(
            pCmd->hChild, 
            TYPE_VISUAL3D
            ));

    Assert(pChild);
    IFC(CMilVisual3D::ValidateNode(pChild));  

    if (m_pChild != NULL)
    {
        m_pChild->SetParent(NULL);
        m_pChild->Release();
    }

    m_pChild = pChild;

    m_pChild->AddRef();
    m_pChild->SetParent(this);

    CMilVisual::PropagateFlags(this, TRUE, FALSE);
    CMilVisual3D::PropagateFlags(pChild, FALSE, TRUE);

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------------------------
// CMilViewport3DVisual::GetContentBounds
//
// Synopsis:
//     Compute the bounds of the content rendered by this node.
//
//---------------------------------------------------------------------------------

override HRESULT 
CMilViewport3DVisual::GetContentBounds(
    __in_ecount(1) CContentBounder *pContentBounder, 
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;
    
    // initialize out-param to 0,0,0,0
    prcBounds->SetEmpty();    

    // The inner bounds of this visual are accumulated here.
    IFC(pContentBounder->GetContentBounds(
        this, 
        &m_innerBoundingBoxRect
        ));

    *prcBounds = m_innerBoundingBoxRect;

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------------------------
// CMilViewport3DVisual::RenderContent
//
// Synopsis:
//     Render the contents of this node.  
//
//---------------------------------------------------------------------------------

override HRESULT 
CMilViewport3DVisual::RenderContent(
    __in_ecount(1) CDrawingContext *pDrawingContext
    )
{
    HRESULT hr = S_OK;

    // Rendering rules for CMilViewport3DVisual are the same as CMilVisual:
    // content is rendered before children.

    // 1.  Render 2D content
    IFC(CMilVisual::RenderContent(pDrawingContext));

    // 2.  Render 3D children
    // Inner bounding box is used as the bounds of CMilViewport3DVisual 
    IFC(pDrawingContext->Render3D(
        m_pChild,
        m_pCamera,
        &m_viewport,
        CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(m_innerBoundingBoxRect)
        ));

Cleanup:
    RRETURN(hr);
}


