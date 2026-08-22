// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Header for the brush realizer class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

class CMilBrushDuce;
class CMetaBitmapRenderTarget;
class CEffectList;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBrushRealizer
//
//  Synopsis:
//      This class allows the drawing context to pass in a brush that is not yet
//      realized to the internal render targets. The internal render targets can
//      then ask the CBrushRealizer to realize itself.
//
//------------------------------------------------------------------------------

class CBrushRealizer : 
    public CMILRefCountBase
{
protected:

    CBrushRealizer();
    virtual ~CBrushRealizer();

public:
    static HRESULT CreateResourceRealizer(
        __inout_ecount(1) CMilBrushDuce *pBrushResource,
        __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
        );

    static HRESULT CreateImmediateRealizer(
        __in_ecount(1) CMILBrush *pMILBrush,
        __in_ecount_opt(1) IMILEffectList *pIEffect,
        bool fSkipMetaFixups,
        __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
        );

    static HRESULT CreateImmediateRealizer(
        __in_ecount(1) const MilColorF *pColor,
        __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
        );

    static HRESULT CreateNullBrush(
        __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
        );

    virtual bool RealizedBrushMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const PURE;
    virtual bool RealizedBrushWillHaveSourceClip() const PURE;
    virtual bool RealizedBrushSourceClipMayBeEntireSource(
        __in_ecount_opt(1) const BrushContext *pBrushContext
        ) const PURE;

    virtual HRESULT EnsureRealization(
        UINT uAdapterIndex,
        DisplayId realizationDestination,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) CIntermediateRTCreator *pIRenderTargetCreator
        ) PURE;

    virtual void RestoreMetaIntermediates();

    virtual void FreeRealizationResources();

    virtual __out_ecount_opt(1) CMILBrush *GetRealizedBrushNoRef(
        bool fConvertNULLToTransparent
        );
    virtual HRESULT GetRealizedEffectsNoRef(
        __deref_out_ecount_opt(1) IMILEffectList **ppIEffectList
        ) PURE;

    float GetOpacityFromRealizedBrush();

protected:

    virtual __out_ecount_opt(1) const CMILBrush *GetRealizedBrushNoRef() const { return  m_pRealizedBrush; }

    void SetRealizedBrush(
        __inout_ecount_opt(1) CMILBrush *pRealizedBrush,
        bool fSkipMetaFixups
        );

    HRESULT ReplaceBrushMetaIntermedateWithInternalIntermediate(
        IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
        DisplayId realizationDestination
        );

private:

    void PutBackBrushMetaIntermediate();

protected:
    // This member is either used to convert NULL brushes to transparent brushes
    // or it is used to store a cheap solid color brush.
    LocalMILObject<CMILBrushSolid> m_solidColorBrush;

private:
    CMILBrush *m_pRealizedBrush;

    // If non-NULL then we need to adjust the realized brush
    CMetaBitmapRenderTarget *m_pBrushMetaBitmapRT;
};



//+-----------------------------------------------------------------------------
//
//  Class:
//      CBrushResourceRealizer
//
//  Synopsis:
//      brush realizer coming from the UCE
//
//------------------------------------------------------------------------------

MtExtern(CBrushResourceRealizer);

class CBrushResourceRealizer : 
    public CBrushRealizer
{
public:

    CBrushResourceRealizer(
        __inout_ecount(1) CMilBrushDuce *pBrushResource
        );
    virtual ~CBrushResourceRealizer();

    bool RealizedBrushMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const override;
    bool RealizedBrushWillHaveSourceClip() const override;
    bool RealizedBrushSourceClipMayBeEntireSource(
        __in_ecount_opt(1) const BrushContext *pBrushContext
        ) const override;

    HRESULT EnsureRealization(
        UINT uAdapterIndex,
        DisplayId realizationDestination,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) CIntermediateRTCreator *pIRenderTargetCreator
        ) override;

    // override void RestoreMetaIntermediates() ; inherits implmentation
    void FreeRealizationResources() override;

    HRESULT GetRealizedEffectsNoRef(
        __deref_out_ecount_opt(1) IMILEffectList **ppIEffectList
        ) override;

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBrushResourceRealizer));

private:

    // Members used to realize the brush
    CMilBrushDuce *m_pBrushResourceNoRef;

    // Future Consideration:   Remove this effect list by changing interface
    // to internal rendertargets.
    CEffectList *m_pBrushEffects;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CImmediateBrushRealizer
//
//  Synopsis:
//      This class allows us to keep code paths that wish to pass a brush
//      realizer to lower levels, but which already have a CMILBrush. Instances
//      of this class do not really need to be realized. The realize method just
//      returns the brush and effect list that this class has been holding all
//      along.
//
//------------------------------------------------------------------------------

MtExtern(CImmediateBrushRealizer);

class CImmediateBrushRealizer : public CBrushRealizer
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CImmediateBrushRealizer));

    CImmediateBrushRealizer();
    ~CImmediateBrushRealizer();

    void SetMILBrush(
        __inout_ecount(1) CMILBrush *pMILBrush,
        __inout_ecount_opt(1) IMILEffectList *pIEffectList,
        bool fSkipMetaFixups
        );

    void SetSolidColorBrush(
        __in_ecount(1) const MilColorF *pColor
        );

    bool RealizedBrushMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const override;
    bool RealizedBrushWillHaveSourceClip() const override;
    bool RealizedBrushSourceClipMayBeEntireSource(
        __in_ecount_opt(1) const BrushContext *pBrushContext
        ) const override;

    HRESULT EnsureRealization(
        UINT uAdapterIndex,
        DisplayId realizationDestination,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) CIntermediateRTCreator *pIRenderTargetCreator
        ) override;

    void RestoreMetaIntermediates() override;

    HRESULT GetRealizedEffectsNoRef(
        __deref_out_ecount_opt(1) IMILEffectList **ppIEffectList
        ) override
    {
        *ppIEffectList = m_pIEffectList;
        return S_OK;
    }

private:

    void SetEffect(
        __inout_ecount_opt(1) IMILEffectList *pIEffect,
        bool fSkipMetaFixups
        );

    HRESULT ReplaceEffectMetaIntermedateWithInternalIntermediate(
        IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
        DisplayId realizationDestination
        );

     void PutBackEffectMetaIntermediate();

private:

    // If non-NULL then we need to adjust the effect
    CMetaBitmapRenderTarget *m_pEffectMetaBitmapRT;

    IMILEffectList *m_pIEffectList;
};


