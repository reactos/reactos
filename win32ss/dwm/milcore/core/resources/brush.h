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
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilBrushDuce);

// Class: CMilBrushDuce
class CMilBrushDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilBrushDuce));

    CMilBrushDuce(__in_ecount(1) CComposition*)
    {
        SetDirty(TRUE);
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_BRUSH;
    }

public:
    virtual HRESULT GetRealizer(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount(1) CBrushRealizer** ppBrushRealizer
        );

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      RealizationMayNeedNonPow2Tiling
    //
    //  Synopsis:
    //      Returns whether the brush needs non-pow2 tiling. Now-pow2 tiling is
    //      not implemented in hardware text rendering, so text uses this query
    //      to determine if software should be used instead.
    //
    //--------------------------------------------------------------------------
    virtual bool RealizationMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        UNREFERENCED_PARAMETER(pBrushContext);

        return false;
    }

    virtual bool RealizationWillHaveSourceClip() const
    {
        return false;
    }
    
    virtual bool RealizationSourceClipMayBeEntireSource(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        UNREFERENCED_PARAMETER(pBrushContext);
        Assert(RealizationWillHaveSourceClip());
        return true;
    }

    virtual BOOL HasRealizationContextChanged(
        IN const BrushContext *pBrushContext
        ) const
    {
        return TRUE;
    }

    HRESULT GetBrushRealizationNoRef(
        __in_ecount(1) const BrushContext *pBrushContext, 
        __deref_out_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );
        
    virtual bool IsConstantOpaque()
    {
        return false;
    }

    virtual bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const = 0;

    virtual ~CMilBrushDuce();

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      CMilBrushDuce::FreeRealizationResources
    //
    //  Synopsis:
    //      Frees realized resource that shouldn't last longer than a single
    //      primitive.  That is currently true for intermediate RTs.  It is up
    //      to derivatives to override and free.
    //
    //--------------------------------------------------------------------------

    virtual void FreeRealizationResources() 
    {
    };

protected:

     override BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        )
    {
        SetDirty(TRUE);
        return TRUE;
    }
    
    virtual HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        ) = 0;

    DOUBLE GetDouble(DOUBLE rBase, const CMilSlaveDouble *pResource)
    {
        if (NULL != pResource)
        {
            pResource->GetValue(&rBase);
        }

        return rBase;
    }

    static HRESULT GetOpacity(
        DOUBLE rBase,
        __in_ecount(1) CMilSlaveDouble *pResource,
        __out_ecount(1) FLOAT *pOpacity
        )
    {
        HRESULT hr = S_OK;
        DOUBLE opacityD;

        Assert(pOpacity);
        
        IFC(GetDoubleCurrentValue(
            &rBase,
            pResource,
            &opacityD
            ));

        *pOpacity = static_cast<FLOAT>(ClampAlpha(opacityD));
    
    Cleanup:        
        RRETURN(hr);
    }

    MilColorF *GetColor(MilColorF *pclBase, CMilSlaveColor *pResource)
    {
        if (NULL != pResource)
        {
            return pResource->GetValue();
        }
        else
        {
            return pclBase;
        }
    }

    MilPoint2D *GetPoint(MilPoint2D *ppt, CMilSlavePoint *pResource)
    {
        if (NULL != pResource)
        {
            return pResource->GetValue();
        }
        else
        {
            return ppt;
        }
    }

    //
    // Reference to an object which can be used to obtain a realization of this brush
    //

    CBrushRealizer *m_pBrushRealizer;

private:
    //
    // Pointer to the cached current brush realization. The reference is
    // actually kept by the brush subclass.
    //

    CMILBrush *m_pCurrentRealizationNoRef;  
};



