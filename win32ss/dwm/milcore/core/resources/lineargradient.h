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
//      Contains the definition of the linear gradient UCE resource.
//
//      This resource references the constant & animate properties of a linear
//      gradient brush defined at our API, and is able to resolve those
//      properties into a procedural color source.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
MtExtern(CMilLinearGradientBrushDuce);

class CMilLinearGradientBrushDuce : public CMilGradientBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilLinearGradientBrushDuce));

    CMilLinearGradientBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGradientBrushDuce(pComposition)
    {
    }

    virtual ~CMilLinearGradientBrushDuce();

public:

    override bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_LINEARGRADIENTBRUSH || CMilGradientBrushDuce::IsOfType(type);
    }

    override bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        // Shape bounds are needed when mapping mode is relative to the
        // bounding box or when a relative transform is used.
        return    (m_data.m_MappingMode == MilBrushMappingMode::RelativeToBoundingBox)
               || (m_data.m_pRelativeTransform != NULL);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_LINEARGRADIENTBRUSH* pCmd,
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
        ) const
    {
                    // If a mapping mode is relative to the brush sizing bounds *and* those
                    // bounds have changed, then the realization context has changed            
        return  (   (m_data.m_MappingMode == MilBrushMappingMode::RelativeToBoundingBox) &&

                    // Return true if the brush sizing bounds have changed
                    //
                    // We use exact equality here because fuzzy checks are expensive, coming up 
                    // with a fuzzy threshold that defines the point at which visible changes
                    // occur isn't straightforward (i.e., the brush sizing bounds aren't
                    // in device space), and exact equality handles the case we need to optimize
                    // for where a brush fills the exact same geometry more than once.                
                    !(IsExactlyEqualRectD(pBrushContext->rcWorldBrushSizingBounds, m_cachedBrushSizingBounds) ));

    }

    HRESULT GetLinearGradientRealization(
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __in_ecount(1) CGradientColorData *pColorData,
        __inout_ecount(1) CMILBrushLinearGradient *pLinearGradientRealization
        );

    HRESULT RealizeGradientPoints(
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __out_ecount(1) MilPoint2F *pStartPoint,
        __out_ecount(1) MilPoint2F *pEndPoint,
        __out_ecount(1) MilPoint2F *pDirectionPoint
        );

    //
    // Sizing bounds used to create the last realization.  We store this
    // to compare against future bounds so we can avoid re-creating
    // the realization when the brush's sizing bounds haven't changed.
    //
    // Ideally typed as CRect*<CoordinateSpace::BaseSampling>
    MilPointAndSizeD m_cachedBrushSizingBounds;

    CMilLinearGradientBrushDuce_Data m_data;

    LocalMILObject<CMILBrushLinearGradient> m_realizedGradientBrush;
    LocalMILObject<CMILBrushSolid> m_realizedSolidBrush;
};

