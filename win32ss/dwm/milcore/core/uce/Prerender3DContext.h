// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------------
//

//
//  Description:
//
//      The Prerender3DContext walks the 3D Visual subtree collecting lights
//      and (optionally) computing the near and far camera planes.
//
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------

class CMilVisual3D;

//----------------------------------------------------------------------------------
// Meters
//----------------------------------------------------------------------------------

MtExtern(CPrerender3DContext);

//----------------------------------------------------------------------------------
// class CPrerender3DContext
//----------------------------------------------------------------------------------

class CPrerender3DContext : IGraphIteratorSink
{
    friend CDrawingContext;
    
private:
    // Ctor inaccessible: Use the Create method to create a CPrerender3DContext.
    CPrerender3DContext()    {}

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CPrerender3DContext));

public:
    virtual ~CPrerender3DContext();

    static HRESULT Create(
        __deref_out_ecount(1) CPrerender3DContext** ppPrerender3DContext
        );

    HRESULT Compute(
        __in_ecount(1) CMilVisual3D *pRoot,
        __in_ecount(1) CMILMatrix *pViewTransform,
        __inout_ecount(1) CMILLightData *pLightData,
        __inout bool &fRenderRequired
        )
    {
        float unusedNearPlane;
        float unusedFarPlane;
        
        RRETURN(Compute(
            pRoot,
            pViewTransform,
            pLightData,
            /* fComputeClipPlanes = */ false,
            unusedNearPlane,
            unusedFarPlane,
            fRenderRequired));
    }

    HRESULT Compute(
        __in_ecount(1) CMilVisual3D *pRoot,
        __in_ecount(1) CMILMatrix *pViewTransform,
        __inout_ecount(1) CMILLightData *pLightData,
        float &flNearPlane,
        float &flFarPlane,
        __inout bool &fRenderRequired
        )
    {
        RRETURN(Compute(
            pRoot,
            pViewTransform,
            pLightData,
            /* fComputeClipPlanes = */ true,
            flNearPlane,
            flFarPlane,
            fRenderRequired));
    }

    HRESULT Compute(
        __in_ecount(1) CMilVisual3D *pRoot,
        __in_ecount(1) CMILMatrix *pViewTransform,
        __inout_ecount(1) CMILLightData *pLightData,
        bool fComputeClipPlanes,
        float &flNearPlane,
        float &flFarPlane,
        __inout bool &fRenderRequired
        );


    //------------------------------------------------------------------------------
    // IGraphIteratorSink interface
    //------------------------------------------------------------------------------

    HRESULT PreSubgraph(
        __out_ecount(1) BOOL* pfVisitChildren
        );

    HRESULT PostSubgraph();

private:    
    CGraphIterator   *m_pGraphIterator;
    bool              m_needDepthSpan;
    CMILMatrix       *m_pViewTransform;
    CGenericMatrixStack
                      m_transformStack;
    float             m_depthSpan[2];
    bool              m_fRenderRequired;
    bool              m_fComputeClipPlanes;
    CMILLightData    *m_pLightData;
};

