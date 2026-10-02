// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------------
//

//
//  Description:
//
//      The Render3DContext renders the 3D Visual subtree.  Note that 3D rendering
//      requires 2-passes. Use the Prerender3DContext to initialize the lights
//      and camera.
//
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------

class CMilVisual3D;

//----------------------------------------------------------------------------------
// Meters
//----------------------------------------------------------------------------------

MtExtern(CRender3DContext);

//----------------------------------------------------------------------------------
// class CRender3DContext
//----------------------------------------------------------------------------------

class CRender3DContext : IGraphIteratorSink
{
    friend CDrawingContext;
    
private:
    // Ctor inaccessible: Use the Create method to create a CRender3DContext.
    CRender3DContext()    {}

public:
    virtual ~CRender3DContext();

    static HRESULT Create(
        __deref_out_ecount(1) CRender3DContext** ppPrerender3DContext
        );

    HRESULT Render(
        __in_ecount(1) CMilVisual3D *pRoot,
        __in_ecount(1) CDrawingContext *pDrawingContext,
        __in_ecount(1) CContextState *pContextState,
        __in_ecount(1) IRenderTargetInternal *pRenderTarget,
        float fWidth,
        float fHeight
        );

    //------------------------------------------------------------------------------
    // IGraphIteratorSink interface
    //------------------------------------------------------------------------------

    HRESULT PreSubgraph(
        __out_ecount(1) BOOL* pfVisitChildren
        );

    HRESULT PostSubgraph();

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CRender3DContext));

private:    
    CGraphIterator         *m_pGraphIterator;
    CGenericMatrixStack     m_transformStack;
    CDrawingContext        *m_pDrawingContext;
    CContextState          *m_pContextState;
    IRenderTargetInternal  *m_pRenderTarget;
    float                   m_fWidth;
    float                   m_fHeight;
};

