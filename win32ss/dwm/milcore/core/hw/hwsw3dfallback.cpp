// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//
//      CHw3DSoftwareFallback implementation, inherits from CHwSurfaceRenderTarget
//
//      This object will take 3D only drawing functions and implements them
//      using a D3D RGB Rast software device.  This is then drawn back onto a
//      Software Rendertarget
//

#include "precomp.hpp"

MtDefine(CHw3DSoftwareSurface, MILRender, "CHw3DSoftwareSurface");

const size_t kTargetPixelSize = sizeof(ARGB);

//+------------------------------------------------------------------------
//
//  Function:  CHw3DSoftwareSurface::ctor
//
//  Synopsis:  Initializes the CHw3DSoftwareSurface and
//              CHwSurfaceRenderTarget members
//
//-------------------------------------------------------------------------
CHw3DSoftwareSurface::CHw3DSoftwareSurface(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    MilPixelFormat::Enum fmtTarget,
    D3DFORMAT d3dfmtTarget,
    DisplayId associatedDisplay,
    bool fComposeWithCopy
    ) :
    CHwSurfaceRenderTarget(
        pDevice,
        fmtTarget,
        d3dfmtTarget,
        associatedDisplay
        ),
    m_fComposeWithCopy(fComposeWithCopy)
{
    m_fEnableRendering = false;
    m_fSurfaceDirty = false;

    Assert(kTargetPixelSize == D3DFormatSize(m_d3dfmtTargetSurface));

    m_pWrapperBitmap = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHw3DSoftwareSurface::~CHw3DSoftwareSurface
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------

CHw3DSoftwareSurface::~CHw3DSoftwareSurface()
{
    ReleaseInterfaceNoNULL(m_pWrapperBitmap);

    CleanupFreedResources();
}

//+----------------------------------------------------------------------------
//
//  Function:  CHw3DSoftwareSurface::Create
//
//  Synopsis:  Creates a new HW3D Software Fallback object with a
//             CD3DDeviceLevel1 object that will support RGB RAST.
//
//-----------------------------------------------------------------------------
HRESULT
CHw3DSoftwareSurface::Create(
    MilPixelFormat::Enum fmtTarget,
    DisplayId associatedDisplay,
    UINT uWidth,
    UINT uHeight,
    __deref_out_ecount(1) CHw3DSoftwareSurface ** const ppHw3DFallbackRT
    )
{
    HRESULT hr = S_OK;
    CHw3DSoftwareSurface *pHw3DFallback = NULL;

    *ppHw3DFallbackRT = NULL;

    CD3DDeviceLevel1 *pD3DDevice = NULL;

    //
    // Grab the D3DDeviceManager and then get an RGBRast Device
    //

    CD3DDeviceManager * const pD3DDeviceManager = CD3DDeviceManager::Get();

    IFC(pD3DDeviceManager->GetSWDevice(&pD3DDevice));

    //
    // Only two formats are supported by copy optimization: 32bpp BGR and PBGRA
    //
    bool const fComposeWithCopy = (   fmtTarget == MilPixelFormat::BGR32bpp
                                   || fmtTarget == MilPixelFormat::PBGRA32bpp);

    if (!fComposeWithCopy)
    {
        // Select a format that we can source over blend with
        fmtTarget = MilPixelFormat::PBGRA32bpp;
    }

    // Make sure device is capable of using this target format
    D3DFORMAT d3dfmtTarget = PixelFormatToD3DFormat(fmtTarget);
    IFC(pD3DDevice->CheckRenderTargetFormat(d3dfmtTarget));

    pHw3DFallback = new CHw3DSoftwareSurface(
        pD3DDevice,
        fmtTarget,
        d3dfmtTarget,
        associatedDisplay,
        fComposeWithCopy
        );
    IFCOOM(pHw3DFallback);
    pHw3DFallback->AddRef();

    IFC(pHw3DFallback->Resize(
        uWidth,
        uHeight
        ));

    *ppHw3DFallbackRT = pHw3DFallback;

    pHw3DFallback = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pHw3DFallback);
    ReleaseInterfaceNoNULL(pD3DDevice);
    pD3DDeviceManager->Release();

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:  CHw3DSoftwareSurface::BeginSw3D
//
//  Synopsis:  Prepare D3D surface for next set of DrawMesh3D calls
//
//-------------------------------------------------------------------------
HRESULT
CHw3DSoftwareSurface::BeginSw3D(
    // NOTE that the next annotation is currently accurate but doesn't
    // account for future use of sparse 2D allocations.
    __in_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
    UINT cbTargetStride,
    __in_ecount(1) CMILSurfaceRect const &rcBounds,
    bool fUseZBuffer,
    __in_ecount_opt(1) FLOAT *prZ   // Optional to support stepped rendering
    )
{
    HRESULT hr = S_OK;

    if (m_fIn3D)
    {
        IFGOTO(Exit, WGXERR_INVALIDCALL);
    }

    //
    // Must call EnsureSurface before adjusting bounds since if surface needs
    // allocates EnsureSurface will set bounds itself.
    //
    // Future Consideration:   Change EnsureSurface to accept bounds
    //  and allocate only what is really needed for this operations.  This
    //  is Sw 3D so we expect less use as perf is somewhat less that Hw.
    //

    IFGOTO(Exit, EnsureSurface());

    m_rcBoundsPre3D = m_rcBounds;

    //
    // This intersect is really used to check that rcBounds isn't empty. This
    // could just check IsEmpty and assign, but it does intersect to be extra
    // careful.
    //

    if (m_rcBounds.Intersect(rcBounds))
    {
        D3DLOCKED_RECT d3dLock;

        //
        // Lock everything within bounds
        //

        IFC(m_pD3DTargetSurface->LockRect(
            &d3dLock,
            &m_rcBounds,
            0
            ));

        //
        // Initialize relevant pixels of D3D surface.
        //

        size_t cbWidth = kTargetPixelSize * m_rcBounds.Width();

        BYTE *pbDestPixels = reinterpret_cast<BYTE*>(d3dLock.pBits);

        if (m_fComposeWithCopy)
        {
            //
            // Copying the surface forward versus clearing to transparent
            // enables us to do a simple copy back when done versus a slow
            // blend.
            //

            BYTE *pbSourcePixels = reinterpret_cast<BYTE*>(pvTargetPixels)
                                 + kTargetPixelSize * static_cast<UINT>(m_rcBounds.left)
                                 + cbTargetStride * static_cast<UINT>(m_rcBounds.top);

            for (int y = m_rcBounds.top; y < m_rcBounds.bottom; y++)
            {
#if DBG_ANALYSIS
                Assert(pbSourcePixels + cbWidth <=
                       reinterpret_cast<BYTE*>(pvTargetPixels) + cbDbgAnalysisTargetBufferSize
                       );
#endif

                CopyMemory(pbDestPixels, pbSourcePixels, cbWidth);
                pbDestPixels += d3dLock.Pitch;
                pbSourcePixels += cbTargetStride;
            }
        }
        else
        {
            //
            // Formats differ - clear 3D surface to transparent and blend later
            //

            Assert(m_fmtTarget == MilPixelFormat::PBGRA32bpp);

            for (int y = m_rcBounds.top; y < m_rcBounds.bottom; y++)
            {
                ZeroMemory(pbDestPixels, cbWidth);
                pbDestPixels += d3dLock.Pitch;
            }
        }

        //
        // Unlock D3D target
        //

        IFC(m_pD3DTargetSurface->UnlockRect());

        //
        // Finally ready to clear the depth buffer
        //

        if (prZ)
        {
            D3DMULTISAMPLE_TYPE MultisampleType = D3DMULTISAMPLE_NONE;

            IFC(CHwSurfaceRenderTarget::Begin3DInternal(
                *prZ,
                fUseZBuffer,
                MultisampleType
                ));

            // We should not change MultisampleType if D3DMULTISAMPLE_NONE is requested
            Assert(MultisampleType == D3DMULTISAMPLE_NONE);
        }

    }

    //
    // Remember state is now in 3D context, but nothing yet is dirty
    //

    m_fIn3D = true;
    m_fSurfaceDirty = false;

Cleanup:

    if (FAILED(hr))
    {
        m_rcBounds = m_rcBoundsPre3D;
    }

Exit:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHw3DSoftwareSurface::EndSw3D
//
//  Synopsis:  Complete D3D 3D rendering and composite result back into Sw
//             surface
//

HRESULT
CHw3DSoftwareSurface::EndSw3D(
    __inout_ecount(1) CSpanSink *pSwSink,
    // NOTE that the next annotation is currently accurate but doesn't
    // account for future use of sparse 2D allocations.
    __inout_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
    UINT cbTargetStride,
    __inout_ecount(1) CSoftwareRasterizer *pSwRast
    )
{
    HRESULT hr = S_OK;

    //
    // Snag current bounds before they may be restored
    //

    CMILSurfaceRect rc3DBounds = m_rcBounds;

    IFC(CHwSurfaceRenderTarget::End3D());

    //
    // Composite the results back onto the SW RenderTarget
    //

    if (m_fSurfaceDirty)
    {
        IFC(CompositeWithSwRenderTarget(
            rc3DBounds,
            pSwSink,
            pvTargetPixels,
            DBG_ANALYSIS_PARAM_COMMA(cbDbgAnalysisTargetBufferSize)
            cbTargetStride,
            pSwRast
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHw3DSoftwareSurface::DrawMesh3D
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget when enabled and track the
//             update
//

STDMETHODIMP
CHw3DSoftwareSurface::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    if (!m_rcBounds.IsEmpty())
    {
        IFC(CHwSurfaceRenderTarget::DrawMesh3D(
            pContextState,
            pBrushContext,
            pMesh3D,
            pShader,
            pIEffect
            ));

        //
        // Remember that some changes have been applied and Sw target will need
        // update.
        //
        // Future Consideration:   Account for success w/o render for Sw 3D
        //  dirty tracking.
        //

        m_fSurfaceDirty = true;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHw3DSoftwareSurface::IsValid
//
//  Synopsis:  Return FALSE when rendering with this render target or any use
//             is no longer allowed.  Mode change is a common cause of of
//             invalidation, but this class doesn't currently pay attention to
//             mode change; so, it is expected to always be valid.  The
//             DrawMesh3D method also assumes CHwSurfaceRenderTarget::Clear and
//             ::DrawMesh3D will actually affect the surface.  Naturally a
//             valid surface is required for this.
//
//-----------------------------------------------------------------------------

bool
CHw3DSoftwareSurface::IsValid() const
{
    Assert(m_fEnableRendering);
    Assert(m_pD3DTargetSurface->IsValid());

    return true;
}


//+------------------------------------------------------------------------
//
//  Function:  CHw3DSoftwareSurface::EnsureSurface
//
//  Synopsis:  Makes sure our surface is set up for rendering
//
//-------------------------------------------------------------------------
HRESULT
CHw3DSoftwareSurface::EnsureSurface()
{
    HRESULT hr = S_OK;

    //
    // Should always have a device
    //

    Assert(m_pD3DDevice);

    //
    // Release old resources
    //

    if (m_pD3DTargetSurface)
    {
        if (m_uiNewTargetWidth  != m_uWidth ||
            m_uiNewTargetHeight != m_uHeight)
        {
            ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

            ReleaseInterface(m_pD3DTargetSurface);
        }
    }

    //
    // Create our TargetSurface if it doesn't exist
    //

    if (!m_pD3DTargetSurface)
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        IFC(m_pD3DDevice->CreateRenderTarget(
            m_uiNewTargetWidth,
            m_uiNewTargetHeight,
            m_d3dfmtTargetSurface,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            &m_pD3DTargetSurface
            ));

        m_uWidth = m_uiNewTargetWidth;
        m_uHeight = m_uiNewTargetHeight;

        // Update bounds and min alpha only have we have a size
        IFC(CBaseRenderTarget::Init());
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CHw3DSoftwareSurface::CleanupFreedResources
//
//  Synopsis:   Call CD3DDeviceLevel1 CleanupFreedResources
//
//-------------------------------------------------------------------------
void
CHw3DSoftwareSurface::CleanupFreedResources()
{
    if (m_pD3DDevice)
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        m_pD3DDevice->CleanupFreedResources();
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CHw3DSoftwareSurface::CompositeWithSwRenderTarget
//
//  Synopsis:   Composites the current rendertarget with the Sw Surface
//              We need to render only the area that we filled on the hw
//              surface, because that's the only area that's guaranteed to
//              have correct bits.
//
//-------------------------------------------------------------------------
HRESULT
CHw3DSoftwareSurface::CompositeWithSwRenderTarget(
    __in_ecount(1) const CMILSurfaceRect &rc3DBounds,
    __inout_ecount(1) CSpanSink *pSwSink,
    // NOTE that the next annotation is currently accurate but doesn't
    // account for future use of sparse 2D allocations.
    __inout_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
    UINT cbTargetStride,
    __inout_ecount(1) CSoftwareRasterizer *pSwRast
    )
{
    HRESULT hr = S_OK;
    D3DLOCKED_RECT d3dLock;

    BOOL fSurfaceLocked = false;

    //
    // Lock the D3D SW Surface so we can grab it's bits.
    //

    IFC(m_pD3DTargetSurface->LockRect(
        &d3dLock,
        &rc3DBounds,
        0
        ));

    fSurfaceLocked = true;

    if (m_fComposeWithCopy)
    {
        //
        // Fast path - copy bits back to target
        //

        size_t cbWidth = kTargetPixelSize * rc3DBounds.Width();

#if DBG_ANALYSIS
        Assert(cbTargetStride*(rc3DBounds.bottom-rc3DBounds.top-1) + cbWidth <= cbDbgAnalysisTargetBufferSize);
#endif

        BYTE *pbDestPixels = static_cast<BYTE *>(pvTargetPixels)
                               + kTargetPixelSize * static_cast<UINT>(rc3DBounds.left)
                               + cbTargetStride * static_cast<UINT>(rc3DBounds.top);

        BYTE *pbSourcePixels = static_cast<BYTE *>(d3dLock.pBits);

        for (int y = rc3DBounds.top; y < rc3DBounds.bottom; y++)
        {
            CopyMemory(pbDestPixels, pbSourcePixels, cbWidth);
            pbDestPixels += cbTargetStride;
            pbSourcePixels += d3dLock.Pitch;
        }
    }
    else
    {
        //
        // Slow path - blend bits back to target
        //
        // Prepare our settings for rendering to the Sw Surface
        //

        CRenderState bltRenderState;

        // Set source rect as updated portion of surface.
        bltRenderState.Options.SourceRectValid = TRUE;
        bltRenderState.SourceRect.X = 0;
        bltRenderState.SourceRect.Y = 0;
        bltRenderState.SourceRect.Width = rc3DBounds.Width();
        bltRenderState.SourceRect.Height = rc3DBounds.Height();

        // One-to-one transfer
        bltRenderState.InterpolationMode = MilBitmapInterpolationMode::NearestNeighbor;
        bltRenderState.PrefilterEnable = false;
        bltRenderState.AntiAliasMode = MilAntiAliasMode::None;


        CContextState bltContextState(TRUE /* => Intialize 2D state only */);
        bltContextState.RenderState = &bltRenderState;

        // Restrict composite to surface and bounds -- see Begin3D
        CRectClipper Clipper;
        Clipper.SetClip(rc3DBounds);


        //
        // Create a bitmap that wraps the surface bits
        //

        if (!m_pWrapperBitmap)
        {
            m_pWrapperBitmap = new CClientMemoryBitmap;
            IFCOOM(m_pWrapperBitmap);
            m_pWrapperBitmap->AddRef();
        }

        //
        // Initialize as a bitmap of the updated portion of the surface.
        // 

        IFC(m_pWrapperBitmap->HrInit(
            bltRenderState.SourceRect.Width,
            bltRenderState.SourceRect.Height,
            m_fmtTarget,
            m_uHeight * d3dLock.Pitch,
            static_cast<BYTE *>(d3dLock.pBits),
            d3dLock.Pitch
            ));

        // Translate source into position
        bltContextState.WorldToDevice.SetDx(static_cast<REAL>(rc3DBounds.left));
        bltContextState.WorldToDevice.SetDy(static_cast<REAL>(rc3DBounds.top));


        //
        // Draw the bitmap on the Sw RenderTarget with the surface bits as the
        // input bitmap.
        //

        IFC(pSwRast->DrawBitmap(
            pSwSink,            // Span Sink
            &Clipper,           // Span Clipper
            &bltContextState,   // Context State
            m_pWrapperBitmap,   // Bitmap Source
            NULL                // Effect List
            ));

    }

Cleanup:

    //
    // Unlock the D3D SW Surface
    //

    if (fSurfaceLocked)
    {
        MIL_THR_SECONDARY(m_pD3DTargetSurface->UnlockRect());
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::HrFindInterface
//
//  Synopsis:  HrFindInterface implementation that responds to render
//             target QI's.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHw3DSoftwareSurface::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    // HWND classes are protected by CMetaRenderTarget and never
    // need to be QI'ed, therefore never needing to call HrFindInterface
    AssertMsg(false, "CHw3DSoftwareSurface is not allowed to be QI'ed.");
    RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Function:  CHw3DSoftwareSurface::Resize
//
//  Synopsis:  Resizes the Surface
//
//-------------------------------------------------------------------------
HRESULT
CHw3DSoftwareSurface::Resize(
    UINT uWidth,
    UINT uHeight
    )
{
    HRESULT hr = S_OK;

    //
    // Don't render when minimized or empty
    //

    if (uWidth == 0 || uHeight == 0)
    {
        m_fEnableRendering = FALSE;
    }
    else
    {
        //
        // Update our new size
        //

        m_uiNewTargetWidth = uWidth;
        m_uiNewTargetHeight = uHeight;

        m_fEnableRendering = TRUE;
    }

    RRETURN(hr);
}





