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
//      Contains declaration for CHwPipelineBuilder class.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CHwColorTransformColorSource;
class CHwRadialGradientColorSource;

#define HWPIPELINE_ANTIALIAS_LOCATION MILVFAttrDiffuse

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPipelineBuilder
//
//  Synopsis:
//      Short-lived builder class that puts together the generic pipeline
//      structure that will be reinterpreted for either fixed function or shader
//      rendering.
//
//------------------------------------------------------------------------------
class CHwPipelineBuilder
{

public:

    HRESULT SetupVertexBuilder(
        __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
        );

    virtual HRESULT Set_Constant(
        __in_ecount(1) CHwConstantColorSource *pConstant
        ) PURE;

    virtual HRESULT Set_Texture(
        __in_ecount(1) CHwTexturedColorSource *pTexture
        ) PURE;

    virtual HRESULT Set_RadialGradient(
        __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient
        ) PURE;

    virtual HRESULT Mul_ConstAlpha(
        CHwConstantAlphaColorSource *pAlphaColorSource
        ) PURE;

    virtual HRESULT Set_BumpMap(
        __in_ecount(1) CHwTexturedColorSource *pBumpMap
        );

    virtual HRESULT Mul_AlphaMask(
        __in_ecount(1) CHwTexturedColorSource *pAlphaMaskColorSource
        );

    HRESULT Mul_BlendColors(
        __in_ecount(1) CHwColorComponentSource *pBlendColorSource
        );

    HRESULT Set_AAColorSource(
        __in_ecount(1) CHwColorComponentSource *pAAColorSource
        );

    virtual HRESULT Add_Lighting(
        __in_ecount(1) CHwLightingColorSource *pLightingSource
        ) PURE;

protected:

    virtual HRESULT Mul_BlendColorsInternal(
        __in_ecount(1) CHwColorComponentSource *pBlendColorSource
        ) PURE;

private:

    HRESULT ProcessEffectList(
        __in_ecount(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext *pEffectContext
        );

    HRESULT ProcessAlphaScaleEffect(
        __in_ecount(1) const IMILEffectList *pIEffects,
        UINT uIndex,
        UINT cbSize,
        UINT cResources
        );

    HRESULT ProcessAlphaMaskEffect(
        __in_ecount(1) const CHwBrushContext *pEffectContext,
        __in_ecount(1) const IMILEffectList *pIEffects,
        UINT uIndex,
        UINT cbSize,
        UINT cResources
        );

    HRESULT ProcessClip(
        );

protected:

    CHwPipelineBuilder(
        __in_ecount(1) CHwPipeline * const pHP,
        HwPipeline::Type oType
        );

    void InitializePipelineMembers(
        MilCompositingMode::Enum eCompositingMode,
        __in_ecount(1) IGeometryGenerator const *pIGeometryGenerator
        );

    HRESULT SendPipelineOperations(
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext,
        __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator
        );

    HRESULT ChooseVertexBuilder(
        __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
        );

    __range(0, INT_MAX) INT ReserveCurrentTextureSampler()
    {
        return ++m_iCurrentSampler;
    }

    __range(0, INT_MAX) INT GetNumReservedSamplers() const
    {
        Assert(m_iCurrentSampler >= INVALID_PIPELINE_SAMPLER); 
        Assert(m_iCurrentSampler < INT_MAX);

        return m_iCurrentSampler + 1;
    }


    __range(0, INT_MAX) INT ReserveCurrentStage()
    {
        Assert(m_iCurrentStage + 1 > INVALID_PIPELINE_STAGE);
        Assert(m_iCurrentStage + 1 < INT_MAX);

        return ++m_iCurrentStage;
    }

    void DecrementNumStages()
    {
        Assert(m_iCurrentStage > INVALID_PIPELINE_STAGE);

        m_iCurrentStage--;
    }

    __range(0,INT_MAX) INT GetNumReservedStages() const
    {
        Assert(m_iCurrentStage + 1 > INVALID_PIPELINE_STAGE);
        Assert(m_iCurrentStage < INT_MAX);

        return m_iCurrentStage + 1;
    }


    void SetLastItemAsEarliestAvailableForAlphaMultiply(
        )
    {
        INT iItemCount = static_cast<INT>(m_pHP->m_rgItem.GetCount());
        Assert(iItemCount > 0);
        // We always expect to see advancement of item.
        Assert(iItemCount > m_iAlphaMultiplyOkayAtItem);

        m_iAlphaMultiplyOkayAtItem = iItemCount-1;
    }

    __range(INVALID_PIPELINE_ITEM, INT_MAX) INT GetEarliestItemAvailableForAlphaMultiply() const
    {
        Assert(m_iAlphaMultiplyOkayAtItem >= INVALID_PIPELINE_ITEM);
        Assert(m_iAlphaMultiplyOkayAtItem <= INT_MAX);

        return m_iAlphaMultiplyOkayAtItem;
    }


    void SetLastItemAsAlphaScalable(
        )
    {
        INT iItemCount = static_cast<INT>(m_pHP->m_rgItem.GetCount());
        Assert(iItemCount > 0);
        m_iLastAlphaScalableItem = iItemCount-1;
    }

    void SetLastAlphaScalableStage(
        __range(0, INT_MAX) INT iItem
        )
    {
        Assert(iItem > INVALID_PIPELINE_ITEM);
        Assert(iItem <= INT_MAX);

        m_iLastAlphaScalableItem = iItem;
    }

    __range(INVALID_PIPELINE_ITEM, INT_MAX) INT GetLastAlphaScalableItem() const
    {
        Assert(m_iLastAlphaScalableItem >= INVALID_PIPELINE_ITEM);
        Assert(m_iLastAlphaScalableItem <= INT_MAX);

        return m_iLastAlphaScalableItem;
    }


    __range(INVALID_PIPELINE_ITEM, INT_MAX) INT GetAAPiggybackItem() const
    {
        Assert(m_iAntiAliasingPiggybackedByItem >= INVALID_PIPELINE_ITEM);
        Assert(m_iAntiAliasingPiggybackedByItem <= INT_MAX);

        return m_iAntiAliasingPiggybackedByItem;
    }

    void SetLastItemAsAAPiggyback(
        )
    {
        // There can be one AA piggyback
        Assert(m_iAntiAliasingPiggybackedByItem == INVALID_PIPELINE_ITEM);

        INT iItemCount = static_cast<INT>(m_pHP->m_rgItem.GetCount());
        Assert(iItemCount > 0);
        m_iAntiAliasingPiggybackedByItem = iItemCount-1;
    }

    MilVertexFormat GetAvailableForGeneration() const
    {
        return m_mvfAvailable;
    }

    MilVertexFormat GetGeneratedComponents() const
    {
        return m_mvfGenerated;
    }

    MilVertexFormat GetAvailableForReference() const
    {
        return m_mvfIn;
    }

    bool VerticesArePreGenerated() const
    {
        // Future Consideration:   Find cleaner way to indicate fixed vertices
        // NOTE
        //   Currently fixed vertices also means that we are doing HW transform
        //   of texture coordinates.  At some point this could be decoupled in
        //   which case uses of this flag would have to be broken out by
        //   what they really care about.
        
        return (GetAvailableForReference() & MILVFAttrUV1) != 0;

        // Not equals zero Works around error C4800 which prefers BOOL to bool.
    }
    

    void GenerateVertexAttribute(
        MilVertexFormatAttribute mvfaReserve
        )
    {
        Assert(GetAvailableForGeneration() & mvfaReserve);

        m_mvfGenerated |= mvfaReserve;
        m_mvfAvailable = m_mvfAvailable & ~mvfaReserve;
    }

    bool TryToMultiplyConstantAlphaToExistingStage(
        __in_ecount(1) const CHwConstantAlphaColorSource *pAlphaColorSource
        );

    HRESULT CheckForBlendAlreadyPresentAtAALocation(
        bool *pfNeedToAddAnotherStageToBlendAntiAliasing
        );

protected:
    // Blend operation for combining alpha in vertices (from AA or alpha multiply)
    // with other textures.
    HwBlendOp m_eAlphaMultiplyOp;

    MilVertexFormat m_mvfIn;         // vertex props send by geometry generator
    MilVertexFormat m_mvfGenerated;

    bool m_fAntiAliasUsed;

private:
    CHwPipeline * const m_pHP;
    HwPipeline::Type m_oPipelineType;

    INT m_iCurrentSampler;
    INT m_iCurrentStage;

    INT m_iAlphaMultiplyOkayAtItem;
    INT m_iLastAlphaScalableItem;

    INT m_iAntiAliasingPiggybackedByItem;

    MilVertexFormat m_mvfAvailable;  // vertex props that are yet unclaimed
};



//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwFFPipelineBuilder
//
//  Synopsis:
//      Short lived fixed function builder class that takes the generic pipeline
//      and constructs a fixed function pipeline from it.
//
//------------------------------------------------------------------------------

class CHwFFPipelineBuilder :
    public CHwPipelineBuilder
{
public:
    CHwFFPipelineBuilder(
        __inout_ecount(1) CHwFFPipeline *pHP
        );

    HRESULT Setup(
        MilCompositingMode::Enum eCompositingMode,
        __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext
        );

    override HRESULT Set_Constant(
        __in_ecount(1) CHwConstantColorSource *pConstant
        );

    override HRESULT Set_Texture(
        __in_ecount(1) CHwTexturedColorSource *pTexture
        );

    override HRESULT Set_RadialGradient(
        __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient
        );

    override HRESULT Mul_ConstAlpha(
        CHwConstantAlphaColorSource *pAlphaColorSource
        );

    override HRESULT Mul_AlphaMask(
        __in_ecount(1) CHwTexturedColorSource *pAlphaMaskColorSource
        );

    override HRESULT Add_Lighting(
        __in_ecount(1) CHwLightingColorSource *pLightingSource
        );

protected:

    override HRESULT Mul_BlendColorsInternal(
        __in_ecount(1) CHwColorComponentSource *pBlendColorSource
        );

private:

    void FinalizeBlendOperations(
        MilCompositingMode::Enum eCompositingMode
        );

    HRESULT AddFFPipelineItem(
        HwBlendOp eBlendOp,
        HwBlendArg hbaSrc1,
        HwBlendArg hbaSrc2,
        MilVertexFormatAttribute mvfaSourceLocation,
        __in_ecount_opt(1) CHwColorSource *pHwColorSource
        );

private:
    CHwFFPipeline *m_pHP;
};


