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
//      Contains the definition of the radial gradient UCE resource.
//
//      This resource references the constant & animate properties of a radial
//      gradient brush defined at our API, and is able to resolve those
//      properties into a procedural or texture color source.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
MtExtern(CMilRadialGradientBrushDuce);

class CMilRadialGradientBrushDuce : public CMilGradientBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilRadialGradientBrushDuce));

    CMilRadialGradientBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGradientBrushDuce(pComposition)
    {
        SetDirty(TRUE);

        // members initialized by METERHEAP_CLEAR
        Assert(m_fProceduralBrushRealizedAsIntermediate == false);
        Assert(m_pIntermediateBrushRealizer == NULL);
    }

    virtual ~CMilRadialGradientBrushDuce();

public:

    override bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_RADIALGRADIENTBRUSH || CMilGradientBrushDuce::IsOfType(type);
    }

    override HRESULT GetRealizer(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount(1) CBrushRealizer** ppBrushRealizer
        );

    override void FreeRealizationResources();
    
    override bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        // Shape bounds are needed when mapping mode is relative to the
        // bounding box, when a relative transform is used, or when the brush
        // is being realized into an intermediate surface (i.e. for 3D).
        return     (m_data.m_MappingMode == MilBrushMappingMode::RelativeToBoundingBox)
                || (m_data.m_pRelativeTransform != NULL)
                || pBrushContext->fRealizeProceduralBrushesAsIntermediates;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_RADIALGRADIENTBRUSH* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );

    override bool IsConstantOpaque()
    {
        return IsConstantOpaqueInternal(this);
    }

    override BOOL HasRealizationContextChanged(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const;

    HRESULT UpdateGradientRealization(
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __in_ecount(1) CGradientColorData *pColorData,
        __inout_ecount(1) CMILBrushRadialGradient *pGradientBrush
        );

    HRESULT RealizeGradientPoints(
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __out_ecount(1) MilPoint2F *pCenter,
        __out_ecount(1) MilPoint2F *pRightExtent,
        __out_ecount(1) MilPoint2F *pTopExtent,
        __out_ecount(1) MilPoint2F *pFocus,
        __out_ecount(1) BOOL *pfHasSeparateOriginFromCenter
        );

    //
    // Bounding box used to create the last realization.  We store this
    // to compare against future bounding boxes so we can avoid re-creating
    // the realization when the brush's bounding box hasn't changed.
    //
    // Ideally typed as CRect*<CoordinateSpace::BaseSampling>
    MilPointAndSizeD m_cachedBrushSizingBounds;

    CMilRadialGradientBrushDuce_Data m_data;

private:
    BOOL m_fProceduralBrushRealizedAsIntermediate;
    
private:
    HRESULT GetIntermediateSurfaceRealization(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount(1) CMILBrush **ppBrushRealizationNoRef
        );

    LocalMILObject<CMILBrushSolid> m_realizedSolidBrush;
    LocalMILObject<CMILBrushRadialGradient> m_realizedGradientBrush;

    CBrushRealizer *m_pIntermediateBrushRealizer;
    CMILBrushBitmap *m_pRealizedBitmapBrush;
};

