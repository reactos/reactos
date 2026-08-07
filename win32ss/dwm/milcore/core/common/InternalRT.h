// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Declares internal render target interfaces
//
//----------------------------------------------------------------------------

#pragma once

struct CContextState;
class CMILMesh3D;
class CPlainPen;
class CGlyphRunResource;
class CMILBrushBitmap;
struct DrawGlyphsParameters;
interface IAVSurfaceRenderer;

class CBrushRealizer;
struct BrushContext;
class CMetaRenderTarget;
class CMilEffectDuce;

// This enum is used to compose the return result from
// IRenderTargetInternal::GetType
enum InternalRenderTargetType
{
    RenderTargetRequirementsMask = 0x000000FF,
    RenderTargetTypeMask         = 0x0000FF00,
    BoundsRenderTarget           = 0x00000100,
    DummyRenderTarget            = 0x00000200,
    SWRasterRenderTarget         = 0x00000400,
    HWRasterRenderTarget         = 0X00000800
};

   

class IRenderTargetInternal : public IMILRenderTarget, public CIntermediateRTCreator
{
public:

    // Get the Page to Device transform.

    STDMETHOD_(__outro_ecount(1) const CMILMatrix *, GetDeviceTransform)() const PURE;

    // Draw a surface.

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) PURE;

    // Draw a mesh3D.

    STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D* pMesh3D,
        __inout_ecount_opt(1) CMILShader* pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) PURE;

    // Draw a path.

    STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pPath,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        ) PURE;

    // Fill render target with a brush.

    STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        ) PURE;
    
    STDMETHOD(ComposeEffect)(
        __inout_ecount(1) CContextState *pContextState,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __inout_ecount(1) CMilEffectDuce* pEffect,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IMILRenderTargetBitmap* pImplicitInput
        ) PURE;

    // Draw the glyph run

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        ) PURE;

    // Draw Video

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount_opt(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    IRenderTargetInternal::BeginLayer
    //
    //  Synopsis:  Begin accumulation of rendering into a layer.  Modifications
    //             to layer, as specified in arguments, are handled and result
    //             is applied to render target when the matching EndLayer call
    //             is made.
    //
    //             Calls to BeginLayer may be nested, but other calls that
    //             depend on the current contents, such as Present,
    //             are not allowed until all layers have been resolved with
    //             EndLayer.
    //
    //-------------------------------------------------------------------------

    STDMETHOD(BeginLayer)(
        __in_ecount(1) MilRectF const &LayerBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        __in_ecount_opt(1) IShapeData const *pGeometricMask,
        __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
        FLOAT flAlphaScale = 1.0f,
        __in_ecount_opt(1) CBrushRealizer *pAlphaMask=NULL
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    IRenderTargetInternal::EndLayer
    //
    //  Synopsis:  End accumulation of rendering into current layer.
    //             Modifications to layer, as specified in BeginLayer
    //             arguments, are handled and result is applied to render
    //             target.
    //
    //-------------------------------------------------------------------------

    STDMETHOD(EndLayer)(
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    IRenderTargetInternal::EndAndIgnoreAllLayers
    //
    //  Synopsis:  End accumulation of all layers, but don't apply any
    //             modifications as specified in BeginLayer calls.  This method
    //             should be used to restore render target state when regular
    //             clean up with EndLayer is not meaningful, such as an abort
    //             of rendering.
    //
    //-------------------------------------------------------------------------

    STDMETHOD_(void, EndAndIgnoreAllLayers)(
        ) PURE;
    
    // This method is used to determine if the render target is being
    // used to render, or if it's merely being used for bounds accumulation,
    // hit test, etc.
    STDMETHOD(GetType) (
        __out DWORD *pRenderTargetType
        ) PURE;

    // This method is used to allow a developer to force ClearType use in
    // intermediate render targets with alpha channels.
    STDMETHOD (SetClearTypeHint) (
        __in bool forceClearType
        ) PURE;

    virtual UINT GetRealizationCacheIndex() PURE;

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        ) PURE;

    virtual __out_ecount_opt(1) CMetaRenderTarget *DynCastToMeta() { return NULL; }
      
};

#undef INTERFACE
#define INTERFACE IRenderTargetHWNDInternal

DECLARE_INTERFACE(IRenderTargetHWNDInternal)
{
    virtual void SetPosition(POINT ptOrigin) PURE;

    virtual void UpdatePresentProperties(
        MilTransparency::Flags transparencyFlags,
        BYTE constantAlpha,
        COLORREF colorKey
        ) PURE;

    STDMETHOD(Present)(
        THIS_
        __in_ecount(1) const RECT *pRect
        ) PURE;

    STDMETHOD(ScrollBlt) (
        THIS_
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        ) PURE;        

    STDMETHOD(InvalidateRect)(
        THIS_
        __in_ecount(1) CMILSurfaceRect const *prc
        ) PURE;

    STDMETHOD(ClearInvalidatedRects)(
        THIS
        ) PURE;

    STDMETHOD(Resize)(
        THIS_
        UINT width,
        UINT height
        ) PURE;

    STDMETHOD_(VOID, AdvanceFrame)(
        THIS_
        UINT uFrameNumber
        ) PURE;

    STDMETHOD(WaitForVBlank)(
        THIS_
        ) PURE;
};

struct DrawGlyphsParameters
{
    CContextState *pContextState;
    BrushContext *pBrushContext;
    CGlyphRunResource *pGlyphRun;
    CBrushRealizer *pBrushRealizer;
    CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> rcBounds;
};

