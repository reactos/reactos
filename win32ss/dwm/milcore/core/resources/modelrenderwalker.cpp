// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of model renderer scene graph walker for 3D
//
//------------------------------------------------------------------------

#include "precomp.hpp"

//---------------------------------------------------------------------------------
//  Meter declarations
//---------------------------------------------------------------------------------

MtDefine(CModelRenderWalker, MILRender, "CModelRenderWalker");
#define NULL_HANDLE NULL

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::ctor
//
//  Synopsis:  Constructor taking render context
//------------------------------------------------------------------------
CModelRenderWalker::CModelRenderWalker(__inout_ecount(1) CDrawingContext *pRC)
{
    m_pRC = pRC;
    m_pCtxState = NULL;
    m_pRenderTarget = NULL;
    m_viewportWidth = 0;
    m_viewportHeight = 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::RenderModels
//
//  Synopsis:  Render scene graph to render target
//
//  Notes:     pCtxState->WorldTransform should already be initialized
//             with the Visual3D's world-to-model transform if any.
//
//  Returns:   Success if the rendering completed without errors
//
//------------------------------------------------------------------------
HRESULT CModelRenderWalker::RenderModels(
    __in_ecount(1) CMilModel3DDuce *pRoot,
    __in_ecount(1) IRenderTargetInternal* pRenderTarget,
    __in_ecount(1) CContextState *pCtxState,
    float viewportWidth,
    float viewportHeight
    )
{
    HRESULT hr = S_OK;

    m_pCtxState = pCtxState;
    m_pRenderTarget = pRenderTarget;
    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;

    // Initialize the stack
    m_transformStack.Clear();
    IFC(m_transformStack.Push(&(pCtxState->WorldTransform3D)));

    // run the iterator
    IFC(m_iterator.Walk(pRoot, this));

Cleanup:
    m_pCtxState = NULL;
    m_pRenderTarget = NULL;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::PreSubgraph
//
//  Synopsis:  Overrides IModelIteratorSink::PreSubgraph
//             PreSubgraph is called before the sub-graph of a node is
//             visited. With the output argument fVisitChildren the
//             implementor can control if the sub-graph of this node
//             should be visited at all.
//
//  Returns:   Success if rendering completed without errors
//
//------------------------------------------------------------------------
HRESULT
CModelRenderWalker::PreSubgraph(
    __in_ecount(1) CMilModel3DDuce *pModel,
    __out_ecount(1) bool *pfVisitChildren
    )
{
    HRESULT hr = S_OK;

    *pfVisitChildren = TRUE;

    IFC(pModel->Render(this));

Cleanup:

    // Note that in case of a failure the graph walker will stop immediately. More importantly
    // there is nothing that is equivalent to the stack unwinding in the recursive case.
    // So cleaning out the stacks has to happen in a different place.

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::RenderGeometryModel3D
//
//  Synopsis:  Renders a GeometryModel3D
//
//  Returns:   Success if rendering completed without errors
//
//------------------------------------------------------------------------
HRESULT
CModelRenderWalker::RenderGeometryModel3D(
    __in_ecount(1) CMilGeometryModel3DDuce *pModel
    )
{
    HRESULT hr = S_OK;
    CMILMesh3D *pMesh = NULL;

    BOOL fPopTransform = FALSE;

    Assert(m_pCtxState->CullMode3D == D3DCULL_CW || m_pCtxState->CullMode3D == D3DCULL_CCW);
    
    // Early exit with S_OK if the primitive has no viable materials or no geometry
    if (!pModel->m_data.m_pGeometry || 
        ((!pModel->m_data.m_pMaterial || !pModel->m_data.m_pMaterial->ShouldRender())
         && (!pModel->m_data.m_pBackMaterial || !pModel->m_data.m_pBackMaterial->ShouldRender()))
        )
    {
        goto Cleanup;
    }

    //
    //  Retrieve the Mesh3D from the primitive's geometry
    //

    IFC(pModel->m_data.m_pGeometry->GetRealization(&pMesh));

    // Early exit with S_OK if the primitive has a mesh, but it is empty (i.e., has no vertices)
    if (!pMesh)
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    //
    //  Adjust the world transform to transform us to model space
    //
    if (pModel->m_data.m_pTransform)
    {
        CMILMatrix matrix;

        IFC(pModel->m_data.m_pTransform->GetRealization(&matrix));

        IFC(m_transformStack.Push(&matrix));
        fPopTransform = TRUE;

        m_transformStack.Top(&(m_pCtxState->WorldTransform3D));
    }

    if (!m_pRC->IsBounding())
    {
        // This is not a bounds calc pass so do a full back to front render
        IFC(ProcessMaterialAndRender(pModel->m_data.m_pBackMaterial, pMesh, true));
        IFC(ProcessMaterialAndRender(pModel->m_data.m_pMaterial, pMesh, false));
    }                  
    else
    {
        // We need to draw something for the bounds calc
        // to work but we don't need lighting or materials. 
        // Furthermore, a bounds DrawMesh3D doesn't cull so
        // we don't have to worry about BackMaterial.
        IFC(m_pRenderTarget->DrawMesh3D(
            m_pCtxState,
            NULL,
            pMesh, 
            NULL, 
            NULL
            ));
    }   

Cleanup:

    //
    //  Restore the world transform (only if we modified it.)
    //

    if (fPopTransform)
    {
        m_transformStack.Pop();
        fPopTransform = FALSE;
        m_transformStack.Top(&m_pCtxState->WorldTransform3D);
    }

    ReleaseInterfaceNoNULL(pMesh);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::ProcessMaterialAndRender
//
//  Synopsis:  Gathers all of the Materials in pMaterial, sets up 
//             lighting, and then renders with each Material
//                       
//  Returns:   Success if rendering succeeded
//
//------------------------------------------------------------------------
HRESULT
CModelRenderWalker::ProcessMaterialAndRender(
    __in_ecount(1) CMilMaterialDuce *pMaterial,
    __in_ecount(1) CMILMesh3D *pMesh3D,
    bool fFlipCullMode
    )
{
    HRESULT hr = S_OK;

    // Cache the original cull mode so we can restore it on cleanup.
    D3DCULL originalCullMode = m_pCtxState->CullMode3D;

    if (pMaterial && pMaterial->ShouldRender())
    {
        bool fDiffuseMaterialFound = false;
        bool fSpecularMaterialFound = false;
        float flFirstSpecularPower;
        MilColorF firstAmbientColor;
        MilColorF firstDiffuseColor;
        MilColorF firstSpecularColor;
        DynArray<CMilMaterialDuce *> rgMaterialList;
        
        IFC(pMaterial->Flatten(
            &rgMaterialList, 
            &fDiffuseMaterialFound, 
            &fSpecularMaterialFound, 
            &flFirstSpecularPower,
            &firstAmbientColor,
            &firstDiffuseColor,
            &firstSpecularColor
            ));

        CMILLightData &lightData = m_pCtxState->LightData;

        lightData.EnableDiffuseAndSpecularCalculation(
            fDiffuseMaterialFound, 
            fSpecularMaterialFound
            );

        if (fDiffuseMaterialFound)
        {
            lightData.SetMaterialAmbientColor(firstAmbientColor); 
            lightData.SetMaterialDiffuseColor(firstDiffuseColor); 
        }
        
        if (fSpecularMaterialFound)
        {
            lightData.SetMaterialSpecularPower(flFirstSpecularPower); 
            lightData.SetMaterialSpecularColor(firstSpecularColor); 
        }

        // The WPF specifies that the winding order of triangles is determined in the
        // mesh's local space (before transformation).  Because reflections change the
        // winding order of the triangle we need to flip the current cull mode if the
        // worldToDevice transform has a negative determinant.
        // 
        // m_pCtxState->CullMode3D was initialized in render3Dcontext.cpp to CW or CCW
        // depending on det(View * Projection * ViewportToDevice).  We still need to
        // take the current world transform into account.  The original cull mode is
        // restored in Cleanup:
        {
            bool fCullIsCW = 
                (m_pCtxState->CullMode3D == D3DCULL_CW) ^ 
                (m_pCtxState->WorldTransform3D.GetDeterminant3D() < 0) ^ 
                fFlipCullMode;
            
            m_pCtxState->CullMode3D = fCullIsCW ? D3DCULL_CW : D3DCULL_CCW;

            // CMILMesh3D & CMILLightData handle the transforms (including flipping if necessary)
            // But they do need to know if we're flipping which side of the mesh we're rendering
            // because of back face or 
            lightData.SetReflectNormals(fFlipCullMode);

            // Later on in the shader, if software lighting is needed, this 
            // invalidation will force a recomputation of the lighting
            pMesh3D->InvalidateColorCache();
        }

        IFC(RealizeMaterialAndRender(
            &rgMaterialList,
            pMesh3D
            )); 
    }
    
Cleanup:
    //  Restore the original cull mode
    m_pCtxState->CullMode3D = originalCullMode;
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::RealizeMaterialAndRender
//
//  Synopsis:  Loops through pMaterialList Realize()-ing and rendering
//             each material
//           
//  Returns:   Success if the realizations and render calls succeed
//
//------------------------------------------------------------------------

HRESULT
CModelRenderWalker::RealizeMaterialAndRender(
    __in_ecount(1) const DynArray<CMilMaterialDuce *> *pMaterialList,
    __in_ecount(1) CMILMesh3D *pMesh3D
    )
{
    HRESULT hr = S_OK;
    BrushContext *pBrushContext = NULL;
    CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> matBrushSpaceToSampleSpace;
    CRectF<CoordinateSpace::BaseSampling> rcTextureCoordinateBoundsInBrushSpace;

    MilBitmapInterpolationMode::Enum oldInterpolationMode = m_pCtxState->RenderState->InterpolationMode;

    // This method is a lot of work and it's not required for a bound
    // pass. This is to make sure we don't do this in a bound pass.
    Assert(!m_pRC->IsBounding());

    // Determine whether this mesh lies within the clip region. 
    {
        //
        //  - These infinite values are actually the invalid
        // values of the rect, not the maximum extent. The reason for this is that
        // INT_MAX is already at the extreme boundary of the range. 
        // 
        const CMILSurfaceRect LARGEST_MILSURFACERECT(
            CMILSurfaceRect::sc_rcInfinite.left + 1, 
            CMILSurfaceRect::sc_rcInfinite.top + 1, 
            CMILSurfaceRect::sc_rcInfinite.right - 1, 
            CMILSurfaceRect::sc_rcInfinite.bottom - 1,
            LTRB_Parameters
            );
            
        CMILSurfaceRect rcClip;

        bool fMeshVisible = false;

        // Convert m_pCtxState->AliasedClip to a CMILSurfaceRect via IntersectCAliasedClipWithSurfaceRect
        // We use the largest surface rect possible to ensure that the clip is just converted and isn't
        // reduced.
        //
        // Using this function ensures that the rect is always converted by the same code that
        // CBaseRenderTarget::UpdateCurrentClip uses. 
        if (false == IntersectCAliasedClipWithSurfaceRect(
            &m_pCtxState->AliasedClip,
            LARGEST_MILSURFACERECT,
            &rcClip
            ))
        {
            // Early out of the converted clip is empty
            goto Cleanup;
        }

        //
        // Calculate the rendering bounds & transform
        //

        IFC(ApplyProjectedMeshTo2DState(
            m_pCtxState,
            pMesh3D,
            rcClip,
            OUT &matBrushSpaceToSampleSpace,
            OUT NULL,
            OUT &fMeshVisible,
            OUT &rcTextureCoordinateBoundsInBrushSpace
            ));

        if (!fMeshVisible)
        {
            goto Cleanup;
        }

    }

    m_pRC->Get3DBrushContext(
        rcTextureCoordinateBoundsInBrushSpace,
        &matBrushSpaceToSampleSpace,
        &pBrushContext
        );

    m_pCtxState->RenderState->InterpolationMode = MilBitmapInterpolationMode::Anisotropic;

    for (UINT i = 0; i < pMaterialList->GetCount(); i++)
    {
        CMilMaterialDuce *pMaterial = (*pMaterialList)[i];
        CMILShader *pShader = NULL;

        IFCSUB1(pMaterial->Realize(
            pMesh3D,
            m_pRC,
            m_pCtxState,
            pBrushContext,
            &pShader
            ));
        //
        //  Render that puppy!
        //

        IFCSUB1(m_pRenderTarget->DrawMesh3D(
            m_pCtxState,
            pBrushContext,
            pMesh3D, 
            pShader, 
            NULL
            ));


    SubCleanup1:
        // This frees all brush realizations in the realizers
        if (pShader)
        {
            pShader->FreeBrushRealizations();
            pShader->Release();
        }

        if (FAILED(hr)) { goto Cleanup; }
    }

Cleanup:
    m_pCtxState->RenderState->InterpolationMode = oldInterpolationMode;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CModelRenderWalker::PostSubgraph
//
//  Synopsis:  Overrides IGraphIteratorSink::PostSubgraph
//             PostSubgraph is called after the sub-graph of a node
//             was visited.
//
//  Returns:   Success upon completion without errors
//
//------------------------------------------------------------------------

HRESULT CModelRenderWalker::PostSubgraph(__in_ecount(1) CMilModel3DDuce *pModel)
{
    pModel->PostRender(this);

    return S_OK;
}

