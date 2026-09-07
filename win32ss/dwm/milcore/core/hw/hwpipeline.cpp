// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains implementation for CHwPipeline class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::CHwPipeline
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwPipeline::CHwPipeline(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
    : m_pDevice(pDevice)
{
    m_pABM = NULL;              // Set in Builder::SetupCompositionMode
    m_dwFirstUnusedStage = 0;   // Set in Builder::FinalizeBlendOperations
    m_pVBB = NULL;
    m_pGG = NULL;
    m_pVB = NULL;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::~CHwPipeline
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CHwPipeline::~CHwPipeline()
{
    for (UINT uItem = 0; uItem < m_rgItem.GetCount(); uItem++)
    {
        ReleaseInterfaceNoNULL(m_rgItem[uItem].pHwColorSource);
    }

    delete m_pVBB;
    // The lifetime of m_pGG and m_pVB are not controlled by pipeline.
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::Execute
//
//  Synopsis:
//      Realizes color sources, sets device state and then sends geometry from
//      the IGeometryGenerator to the device.
//
//      IMPORTANT state is set lazily and if the IGeometryGenerator returns
//      WGXHR_EMPTYFILL resources and state are not processed. The
//      CHw3DGeometryRenderer sends no geometry but returns S_OK so the state is
//      still set.
//

HRESULT
CHwPipeline::Execute()
{
    HRESULT hr = S_OK;

    Assert(m_pVBB || m_pVB);
    Assert(m_pGG);

    if (m_pVB)
    {
        // If we have a vertex buffer pointer that means that we already
        // went through the rendering once & were left with a vertex buffer
        // sufficient to re-render (meaning that it contains all of the
        // geometry.
        IFC(SendDeviceStates(m_pVB));
        IFC(m_pVB->DrawPrimitive(m_pDevice));
    }
    else
    {
        // This is probably the first time we're executing but maybe we
        // executed before and we couldn't cache the vertex buffer
        // because the geometry was too large.

        //
        // Populate the vertex buffer
        //

        // Reset buffer to be empty
        IFC(m_pVBB->BeginBuilding());

        // Request geometry data from geometry generate be sent to vertex builder
        IFC(m_pGG->SendGeometry(m_pVBB));
        if (hr == WGXHR_EMPTYFILL)
        {
            // Note that WGXHR_EMPTYFILL is a success code, so it will survive
            // the IFC above.
            hr = S_OK;
            if (!m_pVBB->HasOutsideBounds())
            {
                goto Cleanup;
            }
        }

        IFC(m_pVBB->FlushTryGetVertexBuffer(&m_pVB));
    
        // The Vertex buffer builder is of no more use.
        delete m_pVBB;
        m_pVBB = NULL;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::ReleaseExpensiveResources
//
//  Synopsis:
//      Release expensive resource the pipeline has accumulated
//
//      If a pipeline is to be reused, this method must be called before another
//      call to InitializeForRendering.
//

void
CHwPipeline::ReleaseExpensiveResources(
    )
{
    for (UINT uItem = 0; uItem < m_rgItem.GetCount(); uItem++)
    {
        ReleaseInterface(m_rgItem[uItem].pHwColorSource);
    }

    m_rgItem.SetCount(0);

    if (m_pVBB)
    {
        delete m_pVBB;
        m_pVBB = NULL;
    }

    // The Geometry Generator and Vertex Buffer are only used in the context of
    // a single call to the pipeline for rendering and the pipeline assumes the
    // caller or some other mechanism sufficiently controls their lifetime.
    // There is no reference from the pipeline class.
    m_pGG = NULL;   // Caller controlled lifetime.
    m_pVB = NULL;   // Device abstraction controlled lifetime.
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::SetupCompositionMode
//
//  Synopsis:
//      Setup compositing mode
//

void
CHwPipeline::SetupCompositionMode(
    MilCompositingMode::Enum eCompositingMode
    )
{
    switch (eCompositingMode)
    {
    case MilCompositingMode::SourceCopy:
        m_pABM = &CD3DRenderState::sc_abmSrcCopy;
        break;

    case MilCompositingMode::SourceOver:
        m_pABM = &CD3DRenderState::sc_abmSrcOverPremultiplied;
        break;

    case MilCompositingMode::SourceAdd:
        m_pABM = &CD3DRenderState::sc_abmAddSourceColor;
        break;

    case MilCompositingMode::SourceUnder:
        m_pABM = &CD3DRenderState::sc_abmSrcUnderPremultiplied;
        break;

    case MilCompositingMode::SourceAlphaMultiply:
        m_pABM = &CD3DRenderState::sc_abmSrcAlphaMultiply;
        break;

    case MilCompositingMode::SourceInverseAlphaMultiply:
        m_pABM = &CD3DRenderState::sc_abmSrcInverseAlphaMultiply;
        break;

    case MilCompositingMode::SourceOverNonPremultiplied:
        m_pABM = &CD3DRenderState::sc_abmSrcOver_SrcNonPremultiplied;
        break;

    case MilCompositingMode::SourceInverseAlphaOverNonPremultiplied:
        m_pABM = &CD3DRenderState::sc_abmSrcOver_InverseAlpha_SrcNonPremultiplied;
        break;

    case MilCompositingMode::DestInvert:
        m_pABM = &CD3DRenderState::sc_abmSrcAlphaWithInvDestColor;
        break;

    default:
        NO_DEFAULT("Unrecognized Compositing Mode");
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::RealizeColorSources
//
//  Synopsis:
//      Realize pipeline color sources.
//

HRESULT
CHwPipeline::RealizeColorSources()
{
    HRESULT hr = S_OK;

    UINT uResource;

    for (uResource = 0; uResource < m_rgItem.GetCount(); uResource++)
    {
        if (m_rgItem[uResource].pHwColorSource)
        {
            switch (static_cast<int>(m_rgItem[uResource].pHwColorSource->GetSourceType()))
            {
            case CHwColorSource::PrecomputedComponent:
            case CHwColorSource::Constant:
                break;

            case CHwColorSource::Texture:
            case CHwColorSource::Texture | CHwColorSource::Constant:
                IFC(m_rgItem[uResource].pHwColorSource->Realize());
                break;

            case CHwColorSource::Programmatic:
                break;

            default:
                AssertMsg(false, "Unrecognized HW Color Source type");
                IFC(WGXERR_INTERNALERROR);
                break;
            }
        }
    }

Cleanup:

    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::RealizeColorSourcesAndSendState
//
//  Synopsis:
//      Realize pipeline color sources and send device states.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipeline::RealizeColorSourcesAndSendState(
    __in_ecount_opt(1) const CHwVertexBuffer *pVB
    )
{
    HRESULT hr = S_OK;
    
    IFC(RealizeColorSources());
    IFC(SendDeviceStates(pVB));

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Table:
//      sc_tsoFromPipeOp
//
//  Synopsis:
//      Table of texture stage operations for each of the valid combinations of
//      pipeline operations
//
//------------------------------------------------------------------------------

static const TextureStageOperation* sc_tsoFromPipeOp[HBO_Total][HBA_Total][HBA_Total] =
{   // HBO_SelectSource
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Diffuse*/{ &CD3DRenderState::sc_tsoDiffuse,
                        NULL,       NULL,       NULL,       NULL},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ &CD3DRenderState::sc_tsoSelectTexture,
                        NULL,       NULL,       NULL,       NULL},
    },
    // HBO_Multiply
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoPremulTextureXCurrent},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoPremulTextureXDiffuse},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ NULL,     &CD3DRenderState::sc_tsoPremulTextureXCurrent,
                                    &CD3DRenderState::sc_tsoPremulTextureXDiffuse,
                                                NULL,       NULL},
    },
    // HBO_SelectSourceColorIgnoreAlpha
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ &CD3DRenderState::sc_tsoOpaqueTextureXCurrent,
                        NULL,       NULL,       NULL,       NULL},
    },
    // HBO_MultiplyColorIgnoreAlpha
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoOpaqueTextureXCurrent},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoOpaqueTextureXDiffuse},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ NULL,     &CD3DRenderState::sc_tsoOpaqueTextureXCurrent,
                                    &CD3DRenderState::sc_tsoOpaqueTextureXDiffuse,
                                                NULL,       NULL},
    },
    // HBO_BumpMap
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoBumpMapTexture},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoBumpMapTexture},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoBumpMapTexture},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ &CD3DRenderState::sc_tsoBumpMapTexture,
                        &CD3DRenderState::sc_tsoBumpMapTexture,
                                    &CD3DRenderState::sc_tsoBumpMapTexture,
                                                NULL,       NULL},
    },
    
    // HBO_MultiplyByAlpha
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ NULL,     &CD3DRenderState::sc_tsoMaskTextureXCurrent,
                                    NULL,     NULL,       NULL},
    },
    
    // HBO_MultiplyAlphaOnly
    {
//   hbaSrc2  None      Current     Diffuse     Specular    Texture
// hbaSrc1
/* None   */{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Current*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoColorSelectTextureAlphaMultiplyCurrent},
/* Diffuse*/{ NULL,     NULL,       NULL,       NULL,       &CD3DRenderState::sc_tsoColorSelectTextureAlphaMultiplyDiffuse},
/*Specular*/{ NULL,     NULL,       NULL,       NULL,       NULL},
/* Texture*/{ NULL,     &CD3DRenderState::sc_tsoColorSelectCurrentAlphaMultiplyTexture,
                                    &CD3DRenderState::sc_tsoColorSelectDiffuseAlphaMultiplyTexture,
                                                NULL,       NULL},
    },
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipeline::SendStageStates
//
//  Synopsis:
//      Send all states needed to render to the device
//

HRESULT
CHwFFPipeline::SendDeviceStates(
    __in_ecount_opt(1) const CHwVertexBuffer *pVB
    )
{
    HRESULT hr = S_OK;

    for (UINT uFFItem = 0; uFFItem < m_rgItem.GetCount(); uFFItem++)
    {
        IFC(SendFFStageState(
            m_rgItem[uFFItem]
            ));
    }

    IFC(m_pDevice->DisableTextureStage(m_dwFirstUnusedStage));
    
    IFC(pVB->SendVertexFormat(m_pDevice));
    
    //
    // Currently this must be called after the SendShaderData above
    //
    IFC(SendRenderStates());

Cleanup:

    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::SendRenderStates
//
//  Synopsis:
//      Send render states to device
//

HRESULT
CHwFFPipeline::SendRenderStates()
{
    HRESULT hr = S_OK;

    IFC(m_pDevice->SetAlphaBlendMode(m_pABM));

    IFC(m_pDevice->SetPixelShader(NULL));
    IFC(m_pDevice->SetVertexShader(NULL));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::SendFFStageStates
//
//  Synopsis:
//      Send stage and sample states for the given Fixed Function pipeline item
//

HRESULT 
CHwFFPipeline::SendFFStageState(
    __in_ecount(1) HwPipelineItem &oItem
    )
{
    HRESULT hr = S_OK;

    //
    // Check for an operation
    //

    if (oItem.eBlendOp != HBO_Nop)
    {
        const TextureStageOperation *ptso;

        Assert(oItem.eBlendOp >= 0);
        Assert(oItem.eBlendOp < HBO_Total);
        Assert(oItem.oBlendParams.hbaSrc1 >= 0);
        Assert(oItem.oBlendParams.hbaSrc1 < HBA_Total);
        Assert(oItem.oBlendParams.hbaSrc2 >= 0);
        Assert(oItem.oBlendParams.hbaSrc2 < HBA_Total);

        ptso = sc_tsoFromPipeOp[oItem.eBlendOp]
                               [oItem.oBlendParams.hbaSrc1]
                               [oItem.oBlendParams.hbaSrc2];

        // A NULL ptso indicates an unsupported blending operation
        if (!ptso)
        {
            RIP("Unsupported blending operation!"); // Assert to make sure we don't accidentally fall back to SW.
            IFC(E_NOTIMPL);
        }

        IFC(m_pDevice->SetTextureStageOperation(oItem.dwStage, ptso));
    }

    //
    // If this item has a color source request it to send whatever stage states
    // it needs to the device.
    //

    if (oItem.pHwColorSource)
    {
        IFC(oItem.pHwColorSource->SendDeviceStates(
            oItem.dwStage,
            oItem.dwSampler
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipeline::InitializeForRendering
//
//  Synopsis:
//      Work from an empty pipeline to build the device rendering pipeline and
//      prepare a vertex builder to receive geometry data.
//

HRESULT
CHwFFPipeline::InitializeForRendering(
    MilCompositingMode::Enum CompositingMode,
    __inout_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext,
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    HRESULT hr = S_OK;

    CHwFFPipelineBuilder ffBuilder(
        this
        );

    IFC(ffBuilder.Setup(
        CompositingMode,
        pGeometryGenerator,
        pIPCS,
        pIEffects,
        pEffectContext
        ));

    // Use the Builder class to select/create a vertex builder

    IFC(ffBuilder.SetupVertexBuilder(
        &m_pVBB
        ));
    
    if (prcOutsideBounds)
    {
        m_pVBB->SetOutsideBounds(
            prcOutsideBounds,
            fNeedInside
            );
    }

    // Remember the geometry generator for use in RealizeResources

    m_pGG = pGeometryGenerator;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipeline::~CHwShaderPipeline
//
//  Synopsis:
//      Release the shader.
//
//------------------------------------------------------------------------------

CHwShaderPipeline::~CHwShaderPipeline()
{
    ReleaseInterfaceNoNULL(m_pPipelineShader);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipeline::InitializeForRendering
//
//  Synopsis:
//      Work from an empty pipeline to build the device rendering pipeline and
//      prepare a vertex builder to receive geometry data.
//

HRESULT
CHwShaderPipeline::InitializeForRendering(
    MilCompositingMode::Enum CompositingMode,
    __inout_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext,
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    HRESULT hr = S_OK;

    CHwShaderPipelineBuilder shaderBuilder(this);

    // This should have been checked earlier
    Assert(CHwShaderPipeline::CanRunWithDevice(m_pDevice));
    
    IFC(shaderBuilder.Setup(
        m_f2D,
        CompositingMode,
        pGeometryGenerator,
        pIPCS,
        pIEffects,
        pEffectContext
        ));
    
    // Use the Builder class to select/create a vertex builder

    if (m_f2D)
    {
        IFC(shaderBuilder.SetupVertexBuilder(
            &m_pVBB
            ));
    }

    if (prcOutsideBounds)
    {
        m_pVBB->SetOutsideBounds(
            prcOutsideBounds,
            fNeedInside
            );
    }
    
    // Remember the geometry generator for use in RealizeResources

    m_pGG = pGeometryGenerator;

    Assert(m_pPipelineShader == NULL);

    //
    // We want to check for any shaders that fail compiling in 2D, because we don't
    // expect any of these to fail.  So create an instrumentation list, but only
    // break if we're in 2D.
    //
    {
        SET_CONDITIONAL_MILINSTRUMENTATION_FLAGS(m_f2D ?
                                                    (  MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE
                                                     | MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST) :
                                                    MILINSTRUMENTATIONFLAGS_DONOTHING
                                                    );                                                           

        BEGIN_MILINSTRUMENTATION_HRESULT_LIST
            WGXERR_SHADER_COMPILE_FAILED
        END_MILINSTRUMENTATION_HRESULT_LIST

        IFC(shaderBuilder.GetHwShader(
            &m_pPipelineShader
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipeline::SendDeviceStates
//
//  Synopsis:
//      Send all states needed to render to the device
//

HRESULT
CHwShaderPipeline::SendDeviceStates(
    __in_ecount_opt(1) const CHwVertexBuffer *pVB
    )
{
    HRESULT hr = S_OK;

    Assert(m_pPipelineShader);

    for (UINT uShaderItem = 0; uShaderItem < m_rgItem.GetCount(); uShaderItem++)
    {
        HwPipelineItem &oShaderItem = m_rgItem[uShaderItem];

        if (oShaderItem.pHwColorSource)
        {
            IFC(oShaderItem.pHwColorSource->SendDeviceStates(
                oShaderItem.dwSampler,
                oShaderItem.dwSampler
                ));

            IFC(oShaderItem.pHwColorSource->SendShaderData(
                m_pPipelineShader
                ));
        }
    }

    if (m_f2D)
    {
        // pVB should only be NULL for the 3D pipeline
        Assert(pVB);
        
        IFC(pVB->SendVertexFormat(m_pDevice));
    }
    
    IFC(m_pDevice->SetAlphaBlendMode(m_pABM));
    IFC(m_pPipelineShader->SetState(m_f2D));

Cleanup:

    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipeline::ReInitialize
//
//  Synopsis:
//      Change the non-geometry properties of the pipeline and prepare to
//      execute rendering of the same geometry with different state.
//

HRESULT
CHwShaderPipeline::ReInitialize(
    MilCompositingMode::Enum CompositingMode,
    __inout_ecount_opt(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext,
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    HRESULT hr = S_OK;

    CHwShaderPipelineBuilder builder(this);

    // Use the Builder class to construct the pipeline

    IFC(builder.Setup(
        m_f2D,
        CompositingMode,
        m_pGG,
        pIPCS,
        pEffects,
        pEffectContext
        ));

    if (!m_pVB)
    {
        // Use the Builder class to select/create a vertex builder
        IFC(builder.SetupVertexBuilder(
            &m_pVBB
            ));
    }

    if (prcOutsideBounds)
    {
        m_pVBB->SetOutsideBounds(
            prcOutsideBounds,
            fNeedInside
            );
    }
    
    ReleaseInterface(m_pPipelineShader);
    
    IFC(builder.GetHwShader(
        &m_pPipelineShader
        ));

    // Note: no code can go here w/o checking hr && *pfFailedCompile    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipeline::CanRunWithDevice
//
//  Synopsis:
//      Checks the device to see if the shaderpipeline can be run with it.
//
//------------------------------------------------------------------------------
/* static */ bool 
CHwShaderPipeline::CanRunWithDevice(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice
    )
{
    DWORD dwVertexShaderVersion = pDevice->GetVertexShaderVersion();
    DWORD dwPixelShaderVersion  = pDevice->GetPixelShaderVersion();

    if (   dwPixelShaderVersion  < D3DPS_VERSION(2,0) 
        || dwVertexShaderVersion < D3DVS_VERSION(2,0)
           )
    {
        return false;
    }
    else
    {
        return true;
    }
}

//  
//
// The VBB and GG are hard coded into the shader pipeline. The only thing
// they do that the 3D shader path needs is color source realization. We're
// forced to do the below because the VBB can't handle FVFs that don't 
// contain Diffuse.
//
// Refactoring is needed.
HRESULT 
CHwShaderPipeline::Execute()
{
    HRESULT hr = S_OK;

    if (m_f2D)
    {
        IFC(CHwPipeline::Execute());
    }
    else
    {
        IFC(RealizeColorSourcesAndSendState(m_pVB));
    }
    
Cleanup:
    RRETURN(hr);
}





