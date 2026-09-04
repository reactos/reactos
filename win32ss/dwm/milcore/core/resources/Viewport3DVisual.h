// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      Viewport3DVisual.h
//
// Abstract: 
//      Viewport3DVisual resource.
//
//---------------------------------------------------------------------------

#pragma once

MtExtern(CMilViewport3DVisual);

//---------------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------------

class CMilVisual3D;
class CMilCameraDuce;

//---------------------------------------------------------------------------------
// class CMilViewport3DVisual
//---------------------------------------------------------------------------------

class CMilViewport3DVisual : public CMilVisual
{
    friend class CResourceFactory;
    friend class CDrawingContext;
    friend class CPreComputeContext;
    friend class CGraphWalker;
    friend class CContentBounder;
    friend class CWindowRenderTarget;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilViewport3DVisual));

    CMilViewport3DVisual(__in_ecount(1) CComposition* pComposition) : CMilVisual(pComposition) {}

    virtual ~CMilViewport3DVisual();

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_VIEWPORT3DVISUAL || CMilVisual::IsOfType(type);
    }

    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessSetCamera(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SETCAMERA* pCmd
        );

    HRESULT ProcessSetViewport(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT* pCmd
        );

    override virtual HRESULT ProcessRemoveAllChildren(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_REMOVEALLCHILDREN* pCmd
        );

    override virtual HRESULT ProcessRemoveChild(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_REMOVECHILD* pCmd
        );

    override virtual HRESULT ProcessInsertChildAt(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_INSERTCHILDAT* pCmd
        );

    HRESULT ProcessSet3DChild(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VIEWPORT3DVISUAL_SET3DCHILD* pCmd
        );

    override HRESULT GetContentBounds(
        __in_ecount(1) CContentBounder *pContentBounder, 
        __out_ecount(1) CMilRectF *prcBounds
        );
    
    override HRESULT RenderContent(
        __in_ecount(1) CDrawingContext* pDrawingContext
        );

private:
    CMilVisual3D *m_pChild;
    CMilCameraDuce *m_pCamera;
    MilPointAndSizeD m_viewport;

    // To store the accumulated inner bounds of this visual
    CMilRectF m_innerBoundingBoxRect;
};


