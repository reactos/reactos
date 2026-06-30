// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Declare base surface render target class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once


#include "BaseRT.h"

#include "RTLayer.h"

#define MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS 4

class CD3DSurface;

#if DBG_STEP_RENDERING

class ISteppedRenderingSurfaceRT
{
public:

    virtual void DbgGetSurfaceBitmapNoRef(
        __deref_out_ecount_opt(1) IWGXBitmap **ppSurfaceBitmap
        ) const PURE;
    virtual void DbgGetTargetSurface(
        __deref_out_ecount_opt(1) CD3DSurface **ppD3DSurface
        ) const PURE;
    virtual UINT DbgTargetWidth() const PURE;
    virtual UINT DbgTargetHeight() const PURE;
};

class ISteppedRenderingDisplayRT
{
public:

    virtual void ShowSteppedRendering(
        __in LPCTSTR pszRenderDesc,
        __in_ecount(1) const ISteppedRenderingSurfaceRT *pRT
        ) PURE;
};
#endif

template <typename TRenderTargetLayerData>
class CBaseSurfaceRenderTarget :
    public CBaseRenderTarget
    DBG_STEP_RENDERING_COMMA_PARAM(public ISteppedRenderingSurfaceRT)
{
protected:

    typedef CRenderTargetLayer<CMILSurfaceRect, TRenderTargetLayerData> CRenderTargetLayer;
    typedef CRenderTargetLayerStack<CMILSurfaceRect, TRenderTargetLayerData> CRenderTargetLayerStack;

protected:

    CBaseSurfaceRenderTarget(DisplayId associatedDisplay) : m_associatedDisplay(associatedDisplay) 
    {
        m_forceClearType = false;
    }
    
    virtual ~CBaseSurfaceRenderTarget() {}

public:

    DisplayId GetDisplayId() const
    {
        return m_associatedDisplay;
    }
    
    //
    // IRenderTargetInternal methods
    //

    STDMETHOD(BeginLayer)(
        __in_ecount(1) MilRectF const &LayerBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        __in_ecount_opt(1) IShapeData const *pGeometricMask,
        __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
        FLOAT flAlphaScale,
        __in_ecount_opt(1) CBrushRealizer *pAlphaMask
        );

    STDMETHOD(EndLayer)(
        );

    STDMETHOD_(void, EndAndIgnoreAllLayers)(
        );
    
    STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        ) override;
    
    // This method is used to allow a developer to force ClearType use in
    // intermediate render targets with alpha channels.
    STDMETHODIMP SetClearTypeHint(
        __in bool forceClearType
        )
    {
        m_forceClearType = forceClearType;
        RRETURN(S_OK);
    }

    virtual bool HasAlpha() const = 0;
protected:

    //
    // Additional methods required
    //

    virtual HRESULT BeginLayerInternal(
        __inout_ecount(1) CRenderTargetLayer *pNewLayer
        ) PURE;

    virtual HRESULT EndLayerInternal() PURE;

    __success(return)
    bool GetPartialLayerCaptureRects(
        __inout_ecount(1) CRenderTargetLayer *pNewLayer,
        __out_ecount_part(MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS, *pcCopyRects) CMILSurfaceRect *rgCopyRects,
        __deref_out_range(0,MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS) UINT *pcCopyRects
        );

protected:
#if DBG
    void DbgAssertBoundsState();
#else
    void DbgAssertBoundsState() {}
#endif

protected:

    //
    // RenderTarget State
    //

    CRenderTargetLayerStack m_LayerStack;

    // Associated display
    //  If set to None, this render target does not know which display its
    //  content will be drawn to.  If set to anything else, we regard this
    //  render target as safe to draw content which is restricted to this
    //  display.
    const DisplayId m_associatedDisplay;

    // Force ClearType rendering of text on this surface if the display
    // supports it, regardless of the pixel format.
    bool m_forceClearType;
};


