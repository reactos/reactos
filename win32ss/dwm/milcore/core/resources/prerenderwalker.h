// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    PrerenderWalker.h
//
//---------------------------------------------------------------------------------

// Meter declarations -------------------------------------------------------------

MtExtern(CPrerenderWalker);

//---------------------------------------------------------------------------------
// class CPrerenderWalker
//---------------------------------------------------------------------------------

class CPrerenderWalker : private IModelIteratorSink
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CPrerenderWalker));

    CPrerenderWalker();

    HRESULT RenderLightsAndPossiblyComputeDepthSpan(
        __in_ecount(1) CMilModel3DDuce *pRoot,
        __in_ecount_opt(1) const CMILMatrix* pWorldTransform,
        __in_ecount(1) const CMILMatrix &viewTransform,
        __in_ecount(1) CMILLightData *pLightData,
        bool fComputeDepthSpan
        );
    
    const float *GetSpan() const
    {
        return &(m_depthSpan[0]);
    }

    void AddLight(__in_ecount(1) const CMILLightAmbient *pLight)
    {
        m_pLightData->AddAmbientLight(pLight);
    }

    void AddLight(__in_ecount(1) CMILLightDirectional *pLight)
    {
        m_pLightData->AddDirectionalLight(pLight);
    }

    void AddLight(__in_ecount(1) CMILLightPoint *pLight)
    {
        m_pLightData->AddPointLight(pLight);
    }

    void AddLight(__in_ecount(1) CMILLightSpot *pLight)
    {
        m_pLightData->AddSpotLight(pLight);
    }

private:

    // IModelIteratorSink interface implementation
    virtual HRESULT PreSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel,
        __out_ecount(1) bool *pfVisitChildren
        );
    virtual HRESULT PostSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel
        );

    // Helpers that do most of the pre-subgraph work.
    HRESULT AddDepthSpan(__in_ecount(1) CMilModel3DDuce *pModel);

private:
    // For depth span
    bool                        m_needDepthSpan;
    CGenericMatrixStack         m_transformStack;
    float                       m_depthSpan[2];

    // For both
    CModelIterator              m_iterator;
    CMILLightData              *m_pLightData;
};


