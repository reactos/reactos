// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics
//      $Keywords:
//
//  $Description:
//      Implementation of the bound-calculating render target.  This class
//      accumulates the bounding rectangle of whatever is "rendered" to it.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSwRenderTargetGetBounds, MILRender, "CSwRenderTargetGetBounds");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::Create
//
//  Synopsis:
//      Instantiates, initializes, & ref-counts a CSwRenderTargetGetBounds
//      object
//
//------------------------------------------------------------------------------

HRESULT CSwRenderTargetGetBounds::Create(
    __deref_out_ecount(1) CSwRenderTargetGetBounds **ppRT
    )
{
    HRESULT hr = S_OK;

    CSwRenderTargetGetBounds *pRT = new CSwRenderTargetGetBounds;
    IFCOOM(pRT);

    pRT->AddRef();

    IFC(pRT->HrInit());

    *ppRT = pRT;
    pRT = NULL;

 Cleanup:

    ReleaseInterface(pRT);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::HrFindInterface
//
//  Synopsis:
//      CSwRenderTargetGetBounds QI helper routine
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IRenderTargetInternal || riid == IID_IMILRenderTarget)
        {
            *ppvObject = static_cast<IRenderTargetInternal*>(this);

            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::GetNumQueuedPresents
//
//  Synopsis:
//      The BoundsRenderTarget doesn't queue up any rendering calls, so it
//      always returns 0.
//
//------------------------------------------------------------------------------
HRESULT
CSwRenderTargetGetBounds::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    *puNumQueuedPresents = 0;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::CSwRenderTargetGetBounds
//
//  Synopsis:
//      Default constructor.
//
//------------------------------------------------------------------------------

CSwRenderTargetGetBounds::CSwRenderTargetGetBounds()
{
    m_DeviceTransform.SetToIdentity();
    ResetBounds();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::~CSwRenderTargetGetBounds
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------

CSwRenderTargetGetBounds::~CSwRenderTargetGetBounds()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::HrInit
//
//  Synopsis:
//      Initializes this render target.
//
//------------------------------------------------------------------------------

HRESULT CSwRenderTargetGetBounds::HrInit()
{
    UpdateUniqueCount();

    m_DeviceTransform.SetToIdentity();

    ResetBounds();

    RRETURN(S_OK);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::GetBounds
//
//  Synopsis:
//      Return bounds representing limit of bounds accumulation
//

STDMETHODIMP_(VOID) CSwRenderTargetGetBounds::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    RIP("Currently unused.");
    *pBounds = CMilRectF::sc_rcInfinite;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::Clear
//
//  Synopsis:
//      Not implemented. There should be no need to call this method.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    UNREFERENCED_PARAMETER(pColor);
    UNREFERENCED_PARAMETER(pAliasedClip);

    RIP("Currently unused.");

    RRETURN(THR(E_NOTIMPL));
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::Begin3D
//
//  Synopsis:
//      Do nothing
//
//------------------------------------------------------------------------------

STDMETHODIMP
CSwRenderTargetGetBounds::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    //
    // Even though bounds are given we ignore them since this render target is
    // used to compute the bounds of the subsequent 3D rendering.
    //

    UNREFERENCED_PARAMETER(rcBounds);
    UNREFERENCED_PARAMETER(AntiAliasMode);
    UNREFERENCED_PARAMETER(fUseZBuffer);
    UNREFERENCED_PARAMETER(rZ);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::End3D
//
//  Synopsis:
//      Do nothing
//
//------------------------------------------------------------------------------

STDMETHODIMP
CSwRenderTargetGetBounds::End3D(
    )
{
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::GetDeviceTransform
//
//  Synopsis:
//      Returns the identity device transform.
//
//------------------------------------------------------------------------------

__outro_ecount(1) const CMILMatrix *CSwRenderTargetGetBounds::GetDeviceTransform() const
{
    return &m_DeviceTransform;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::DrawBitmap
//
//  Synopsis:
//      Accumulates the bounds of the draw bitmap call.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    CRectF<CoordinateSpace::PageInPixels> rcBounds;
    CRectF<CoordinateSpace::LocalRendering> rcSource;

    // figure out the source rect

    if (pContextState->RenderState->Options.SourceRectValid)
    {
        rcSource.left = static_cast<FLOAT>(pContextState->RenderState->SourceRect.X);
        rcSource.top = static_cast<FLOAT>(pContextState->RenderState->SourceRect.Y);
        rcSource.right = static_cast<FLOAT>(pContextState->RenderState->SourceRect.X+pContextState->RenderState->SourceRect.Width);
        rcSource.bottom = static_cast<FLOAT>(pContextState->RenderState->SourceRect.Y+pContextState->RenderState->SourceRect.Height);
    }
    else
    {
        // Default source rect covers the bounds of the source, which
        // is 1/2 beyond the extreme sample points in each direction.

        UINT width;
        UINT height;

        IFC(pIBitmap->GetSize(&width, &height));

        rcSource.left = 0.0f;
        rcSource.top = 0.0f;
        rcSource.right = static_cast<FLOAT>(width);
        rcSource.bottom = static_cast<FLOAT>(height);
    }

    // Compute the bounding rectangle.

    pContextState->WorldToDevice.Transform2DBounds(
        rcSource,
        OUT rcBounds
        );

    AddBounds(
        rcBounds,
        pContextState->AliasedClip
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwSurfaceTargetSurface::DrawMesh3D
//
//  Synopsis:
//      Accumulates the projected bounds of the Mesh3D.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;
    CMilPointAndSize3F boxBounds3D;
    CRectF<CoordinateSpace::PageInPixels> rcMeshTargetBounds;
    CMultiOutSpaceMatrix<CoordinateSpace::Local3D> full3DTransform;

    // The model render walker properly does not call DrawMesh3D if we would
    // not render the mesh. In fact, it always gives us a NULL pShader. It
    // would therefore be unappropriate to check for a NULL shader here (as we
    // do in DrawPath).
    UNREFERENCED_PARAMETER(pShader);

    CombineContextState3DTransforms(
        pContextState,
        &full3DTransform);

    IFC(pMesh3D->GetBounds(&boxBounds3D));

    // This method (CSwRenderTargetGetBounds::DrawMesh3D) is 
    // declared as __declspec(nothrow). CalcProjectedBounds 
    // is not guaranteed to return - it calls into 
    // dxlayer::extensions<dxapi>::transform_array which can in turn call 
    // into std::terminate. Though std::terminate is not the same as 
    // throwing an exception, we nevertheless get warning C4702 
    // (unreachable code). 
    //
    // __declspec(nothrow) should imply that no exception is thrown by called 
    // functions - and not that control is always guaranteed to return. 
    // We are encountering this warning nevertheless. Normally, suppression 
    // using #pragma warning(suppress: 4702) would suffice in this situation, 
    // but link-time code generation that is triggered by whole-program optimization
    // doesn't seem to respect this pragma. So we resort to the following hack - 
    // of using a try-catch block - to let the compiler know that the
    // __declspec(nothrow) guarantee will be satisfied. 
    try
    {
        CalcProjectedBounds<CoordinateSpace::PageInPixels>(
            full3DTransform,
            &boxBounds3D,
            OUT &rcMeshTargetBounds
            );
    } 
    catch (...)
    {
        IFC(E_FAIL);
    }

    AddBounds(
        rcMeshTargetBounds,
        pContextState->AliasedClip
        );

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::DrawPath
//
//  Synopsis:
//      Accumulates the bounds of the path.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pBrushContext);

    Assert(pContextState);
    Assert(pShape);

    if (pFillBrush || pStrokeBrush)
    {
        CMilRectF rcBounds;

        if (pPen && pPen->GetDashCap() == MilPenCap::Flat)
        {
            CPlainPen solid(*pPen);
            IFC(pShape->GetRelativeTightBoundsNoBadNumber(
                    OUT rcBounds,
                    pStrokeBrush ? &solid: NULL,
                    // NOTE: Converting CMultiOutSpaceMatrix to CBaseMatrix w/o space check
                    CMILMatrix::ReinterpretBase(&pContextState->WorldToDevice)
                    ));
        }
        else
        {
            IFC(pShape->GetRelativeTightBoundsNoBadNumber(
                    OUT rcBounds,
                    pStrokeBrush ? pPen : NULL,
                    // NOTE: Converting CMultiOutSpaceMatrix to CBaseMatrix w/o space check
                    CMILMatrix::ReinterpretBase(&pContextState->WorldToDevice)
                    ));
        }

        AddBounds(
            rcBounds,
            pContextState->AliasedClip
            );
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CSwRenderTargetGetBounds::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    // It is not necessary to compute the bounds of this rendering operation because
    // it is not publically exposed.
    RIP("DrawInfinitePath not part of public API so we shouldn't be computing bounds.");
    return E_NOTIMPL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::DrawGlyphs
//
//  Synopsis:
//      Accumulates the bounds of the glyphrun.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    Assert(!pars.rcBounds.PageInPixels().IsEmpty());

    if (pars.pBrushRealizer != NULL)
    {
        AddBounds(
            pars.rcBounds.PageInPixels(),
            pars.pContextState->AliasedClip
            );
    }

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::CreateRenderTargetBitmap
//
//  Synopsis:
//      Create a bitmap compatible with this RenderTarget and wrap a new
//      RenderTarget around it.
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::CreateRenderTargetBitmap(
    UINT width,
    UINT height,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    // I guess we'll want to create a "GetBounds" bitmap object, which looks
    // like a bitmap but only accumulates bounds.
    FreAssert(FALSE);
    RRETURN(E_NOTIMPL);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::BeginLayer
//
//  Synopsis:
//      Fail call
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::BeginLayer(
    __in_ecount(1) MilRectF const &LayerBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    __in_ecount_opt(1) IShapeData const *pGeometricMask,
    __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
    FLOAT flAlphaScale,
    __in_ecount_opt(1) CBrushRealizer *pAlphaMask
    )
{
    UNREFERENCED_PARAMETER(LayerBounds);
    UNREFERENCED_PARAMETER(AntiAliasMode);
    UNREFERENCED_PARAMETER(pGeometricMask);
    UNREFERENCED_PARAMETER(pGeometricMaskToTarget);
    UNREFERENCED_PARAMETER(flAlphaScale);
    UNREFERENCED_PARAMETER(pAlphaMask);

    RRETURN(E_NOTIMPL);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::EndLayer
//
//  Synopsis:
//      Fail call
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetGetBounds::EndLayer(
    )
{
    RRETURN(E_NOTIMPL);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::EndAndIgnoreAllLayers
//
//  Synopsis:
//      Nothing to do
//
//------------------------------------------------------------------------------

STDMETHODIMP_(void) CSwRenderTargetGetBounds::EndAndIgnoreAllLayers(
    )
{
    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::ReadEnabledDisplays
//
//  Synopsis:
//      Fail call, only used during Render pass.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CSwRenderTargetGetBounds::ReadEnabledDisplays (
    __inout DynArray<bool> *pEnabledDisplays
    )
{
    UNREFERENCED_PARAMETER(pEnabledDisplays);

    FreAssert(FALSE);
    RRETURN(E_NOTIMPL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::GetRealizationCacheIndex
//
//  Synopsis:
//      Currently unused.
//

UINT
CSwRenderTargetGetBounds::GetRealizationCacheIndex()
{
    RIP("Currently unused.");

    return CMILResourceCache::InvalidToken;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::DrawVideo
//
//  Synopsis:
//      Accumulates the bounds occupied by the video.
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetGetBounds::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount_opt(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;
    // We require that this is checked by the API proxy class.

    Assert(pSurfaceRenderer || pBitmapSource);

    CRectF<CoordinateSpace::PageInPixels> rcBounds;
    CRectF<CoordinateSpace::LocalRendering> rcSource;

    // figure out the source rect

    if (pContextState->RenderState->Options.SourceRectValid)
    {
        rcSource.left = static_cast<FLOAT>(pContextState->RenderState->SourceRect.X);
        rcSource.top  = static_cast<FLOAT>(pContextState->RenderState->SourceRect.Y);
        rcSource.right  = static_cast<FLOAT>(rcSource.left + pContextState->RenderState->SourceRect.Width);
        rcSource.bottom = static_cast<FLOAT>(rcSource.top + pContextState->RenderState->SourceRect.Height);
    }
    else
    {
        MilPointAndSizeF rcSurface = { 0, 0 };

        if (NULL != pSurfaceRenderer)
        {
            IFC(pSurfaceRenderer->GetContentRectF(&rcSurface));
        }
        else
        {
            UINT    width = 0;
            UINT    height = 0;

            IFC(pBitmapSource->GetSize(&width, &height));

            rcSurface.Width = static_cast<FLOAT>(width);
            rcSurface.Height = static_cast<FLOAT>(height);
        }

        rcSource.left = rcSurface.X;
        rcSource.top = rcSurface.Y;
        rcSource.right = rcSurface.X + rcSurface.Width;
        rcSource.bottom = rcSurface.Y + rcSurface.Height;
    }

    //
    // Compute the bounding rectangle.
    //
    pContextState->WorldToDevice.Transform2DBounds(
        rcSource,
        OUT rcBounds
        );

    AddBounds(
        rcBounds,
        pContextState->AliasedClip
        );

Cleanup:

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetGetBounds::AddBounds
//
//  Synopsis:
//      Adds the bounding rectangle to the accumulated bounds, intersecting it
//      with the clip.
//

void
CSwRenderTargetGetBounds::AddBounds(
    __in_ecount(1) const CMilRectF &rcBounds,
    __in_ecount(1) const CAliasedClip &aliasedClip
    )
{
    // Ignore the bounds if they are not well-ordered. If we encounter numerical error
    // in computing the bounds now, we'll almost certainly encounter the same error when
    // rasterizing later, and the draw call will be a noop.
    if (rcBounds.IsWellOrdered())
    {
        if (aliasedClip.IsNullClip())
        {
            m_rcResult.Union(rcBounds);
        }
        else
        {
            CMilRectF rcIntersectedBounds;

            aliasedClip.GetAsCMilRectF(
                &rcIntersectedBounds
                );

            rcIntersectedBounds.Intersect(
                rcBounds
                );

            m_rcResult.Union(rcIntersectedBounds);
        }
    }
}



