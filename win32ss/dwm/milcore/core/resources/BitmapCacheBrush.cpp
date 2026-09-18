// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      The BitmapCacheBrush CSlaveResource is responsible for maintaining
//      the current base values & animation resources for all  
//      BitmapCacheBrush properties, and for the registration of the cache
//      texture that serves as this brush's realization.
//
//------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(BitmapCacheBrushResource, MILRender, "BitmapCacheBrushDuce Resource");

MtDefine(CMilBitmapCacheBrushDuce, BitmapCacheBrushResource, "CMilBitmapCacheBrushDuce");

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilBitmapCacheBrushDuce::~CMilBitmapCacheBrushDuce
//
//  Synopsis:  
//      Class destructor.
//
//-------------------------------------------------------------------------  
CMilBitmapCacheBrushDuce::~CMilBitmapCacheBrushDuce()
{
    delete m_pPreComputeContext;

    if (m_data.m_pInternalTarget != NULL)
    {
        m_data.m_pInternalTarget->UnRegisterCache(m_data.m_pBitmapCache);
    }
    
    UnRegisterNotifiers();
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilBitmapCacheBrushDuce::ProcessUpdate
//
//  Synopsis:  
//      Registers itself with the target Visual to create (or re-use)
//      the brush's underlying cache, specified by its CacheMode.
//
//-------------------------------------------------------------------------
HRESULT 
CMilBitmapCacheBrushDuce::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_BITMAPCACHEBRUSH* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the previous cache mode and Visual and hold onto them.
    CMilBitmapCacheDuce *pOldBitmapCacheMode = m_data.m_pBitmapCache;
    CMilVisual *pOldTarget = m_data.m_pInternalTarget;
    if (pOldBitmapCacheMode != NULL)
    {
        pOldBitmapCacheMode->AddRef();
    }

    if (pOldTarget != NULL)
    {
        pOldTarget->AddRef();
    }
    
    IFC(GeneratedProcessUpdate(pHandleTable, pCmd));

    // If the cache mode or Visual has changed, ensure we have
    // unregistered the previous CacheMode/Visual pair and
    // registered the new pair.
    CMilBitmapCacheDuce *pNewBitmapCacheMode = m_data.m_pBitmapCache;
    CMilVisual *pNewTarget = m_data.m_pInternalTarget;
    if (pOldBitmapCacheMode != pNewBitmapCacheMode || pOldTarget != pNewTarget)
    {
        if (pOldTarget != NULL)
        {
            pOldTarget->UnRegisterCache(pOldBitmapCacheMode);
        }

        if (pNewTarget != NULL)
        {
            IFC(pNewTarget->RegisterCache(pNewBitmapCacheMode));
        }
    }
    
Cleanup:
    ReleaseInterface(pOldBitmapCacheMode);
    ReleaseInterface(pOldTarget);
    
    RRETURN(hr);
}
    
//+------------------------------------------------------------------------
//
//  Member:  
//      CMilBitmapCacheBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      Returns a CMILBrushBitmap wrapping the cache texture.
//
//-------------------------------------------------------------------------
HRESULT 
CMilBitmapCacheBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    IWGXBitmapSource *pBitmapSource = NULL;

    if (m_data.m_pInternalTarget == NULL)
    {
        *ppBrushRealizationNoRef = NULL;
    }
    else
    {
        CMILMatrix matSurfaceToSamplingSpace = CMILMatrix(true);
        
        // PreCompute must be called to ensure cached content is visited, since the target
        // visual might not be attached to the Visual tree elsewhere.
        IFC(PreCompute(pBrushContext->pBrushDeviceNoRef)); 

        // Ensure caches are up-to-date.
        IFC(pBrushContext->pBrushDeviceNoRef->GetVisualCacheManagerNoRef()->UpdateCaches());

        IRenderTargetInternal *pIRT = static_cast<IRenderTargetInternal*>(pBrushContext->pRenderTargetCreator);

        CMilVisualCacheSet *pCacheSet = m_data.m_pInternalTarget->GetCacheSet();
        Assert(pCacheSet != NULL);
        
        IFC(pCacheSet->GetBitmapSource(m_data.m_pBitmapCache, pIRT, &pBitmapSource));

        // If we have no cache bitmap we have nothing to render.
        if (pBitmapSource == NULL)
        {
            *ppBrushRealizationNoRef = NULL;
            goto Cleanup;
        }
        
        m_brushRealization.SetBitmap(pBitmapSource);

        UINT uWidth, uHeight;
        IFC(pBitmapSource->GetSize(&uWidth, &uHeight));

        matSurfaceToSamplingSpace.Scale(
            static_cast<REAL>(pBrushContext->rcWorldBrushSizingBounds.Width / static_cast<float>(uWidth)),
            static_cast<REAL>(pBrushContext->rcWorldBrushSizingBounds.Height / static_cast<float>(uHeight))
            );
        
        m_brushRealization.SetBitmapToXSpaceTransform(
            &matSurfaceToSamplingSpace,
            XSpaceIsWorldSpace
            DBG_COMMA_PARAM(NULL)
            );
    
        *ppBrushRealizationNoRef = &m_brushRealization;
    }

Cleanup:
    ReleaseInterface(pBitmapSource);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilBitmapCacheBrushDuce::GetRenderTargetBitmap
//
//  Synopsis:
//      Returns the underlying cache bitmap.
//
//-------------------------------------------------------------------------
HRESULT 
CMilBitmapCacheBrushDuce::GetRenderTargetBitmap(
    __in CComposition *pComposition,
    __in IRenderTargetInternal *pDestRT,
    __deref_out_opt IMILRenderTargetBitmap **ppRTB
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    if (m_data.m_pInternalTarget == NULL)
    {
        *ppRTB = NULL;
    }
    else
    {
        CMILMatrix matSurfaceToSamplingSpace = CMILMatrix(true);
        
        // PreCompute must be called to ensure cached content is visited, since the target
        // visual might not be attached to the Visual tree elsewhere.
        IFC(PreCompute(pComposition));

        // Ensure caches are up-to-date.
        IFC(pComposition->GetVisualCacheManagerNoRef()->UpdateCaches());

        CMilVisualCacheSet *pCacheSet = m_data.m_pInternalTarget->GetCacheSet();
        Assert(pCacheSet != NULL);
        
        IFC(pCacheSet->GetRenderTargetBitmap(
            m_data.m_pBitmapCache,
            OUT ppRTB, 
            pDestRT 
            DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilBitmapCacheBrushDuce::PreCompute
//
//  Synopsis:  
//      Calls PreCompute on the current Visual content
//
//  Notes:
//      CPreComputeContext::PreCompute avoids a full traversal if a
//      PreCompute has already been done and isn't needed, so it
//      is acceptable to call PreCompute multiple times.  This
//      fact allows us to avoid writing logic which would avoid
//      calling PreCompute twice (once potentially during GetContentBounds,
//      and again during DrawIntoBaseTile).
//
//-------------------------------------------------------------------------
HRESULT
CMilBitmapCacheBrushDuce::PreCompute(
    __in_ecount(1) CComposition *pComposition
    ) const
{
    HRESULT hr = S_OK;

    if (m_pPreComputeContext == NULL)
    {
        IFC(CPreComputeContext::Create(
            pComposition,
            &m_pPreComputeContext
            ));
    }

    Assert(m_data.m_pInternalTarget != NULL);

    IFC(CMilVisualBrushDuce::PreComputeHelper(
        m_pPreComputeContext,
        m_data.m_pInternalTarget
        ));

Cleanup:
    RRETURN(hr);
}

