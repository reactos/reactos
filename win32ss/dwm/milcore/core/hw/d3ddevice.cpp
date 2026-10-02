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
//      Implementation of CD3DDeviceLevel1
//
//      Abstracts the core D3D device to provide the following functionality:
//         1. Restrict access to methods of IDirect3DDevice9 to those
//            available on level 1 graphics cards. (Level1 is the base support
//            we require to hw accelerate)
//         2. Provide correct information for GetDeviceCaps
//         3. Centralize resource creation so that it can be tracked.
//            Tracking created resources is important for responding to mode
//            changes
//         4. Respond to mode changes on present call
//         5. Provide testing functionality for determining if a graphics
//            card meets the level1 criteria for hw acceleration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
#include "HwGraphicsCards.h"

using namespace dxlayer;

//   What depth buffer should be used
//  since we no longer need the stencil.  Is 16 or 32 better than 24?

const D3DFORMAT kD3DDepthFormat = D3DFMT_D24S8;

MtDefine(CD3DDeviceLevel1, MILRender, "CD3DDeviceLevel1");
MtDefine(NormalTraceVectors, MILRender, "Normal vectors drawn for testing");
MtDefine(MILHwMetrics, Metrics, "MIL HW");
MtDefine(VerticesPerFrame, MILHwMetrics, "Vertices per frame");
MtDefine(TrianglesPerFrame, MILHwMetrics, "Triangles per frame");

MtDefine(D3DAllocatedData, MILRender, "Data Allocated by D3D");

MtDefine(D3DRGBRasterizer,        D3DAllocatedData, "D3D RGB rasterizer");
MtDefine(D3DSwapChain,            D3DAllocatedData, "Data Allocated for a D3D swap chain");
MtDefine(D3DColorFill,              D3DAllocatedData, "Data Allocated for a D3D ColorFill");
MtDefine(D3DTexture,              D3DAllocatedData, "Data Allocated for a D3D Texture");
MtDefine(D3DDevice,               D3DAllocatedData, "Data Allocated for a D3D Device");
MtDefine(D3DDrawPrimitiveUP,      D3DAllocatedData, "Data Allocated for a D3D DrawPrimitiveUP Call");
MtDefine(D3DStencilSurface,       D3DAllocatedData, "Data Allocated for a D3D Stencil Surface");
MtDefine(D3DDrawIndexedPrimitive, D3DAllocatedData, "Data Allocated for a D3D DrawIndexedPrimitive call");
MtDefine(D3DDrawIndexedPrimitiveUP, D3DAllocatedData, "Data Allocated for a D3D DrawIndexedPrimitiveUP call");
MtDefine(D3DDrawPrimitive,        D3DAllocatedData, "Data Allocated for a D3D DrawPrimitive call");
MtDefine(D3DVertexBuffer,         D3DAllocatedData, "Data Allocated for a D3D Vertex Buffer");
MtDefine(D3DIndexBuffer,          D3DAllocatedData, "Data Allocated for a D3D Index Buffer");
MtDefine(matrix_t_get_scaling,    D3DAllocatedData, "Data Allocated for Matrix Scaling");
MtDefine(D3DRenderTarget,         D3DAllocatedData, "Data Allocated for a D3D RenderTarget");
MtDefine(D3DStateBlock,           D3DAllocatedData, "Data Allocated for a D3D StateBlock");

MtDefine(D3DCompiler,             D3DAllocatedData, "Data Allocated for a compiled shader");

DeclareTag(tagD3DStats, "MIL-HW", "Output d3d stats");
DeclareTag(tagPixelZoomMode, "MIL-HW", "Pixel zoom mode");
DeclareTag(tagLowPrimitiveCount, "MIL-HW", "Lower primitive count limit");
DeclareTag(tagInjectDIE, "MIL-HW", "Inject D3DERR_DRIVERINTERNALERROR failures");

void DbgInjectDIE(__inout_ecount(1) HRESULT * const pHR);

#define GPU_MARKERS_MAX_ARRAY_SIZE 35

#if !DBG
MIL_FORCEINLINE void DbgInjectDIE(__inout_ecount(1) HRESULT *pHR)
{
}
#endif



extern HINSTANCE g_DllInstance;
extern volatile DWORD g_dwTextureUpdatesPerFrame;
LONG g_lPixelsFilledPerFrame = 0;

//+-----------------------------------------------------------------------------
//
//  D3DDevice allocation macros
//
//  Synopsis:
//      Put these around any D3D interface call that can result in
//      D3DERR_OUTOFVIDEOMEMORY. These macros assume that "hr" contains the
//      HRRESULT from the device call.
//
//      For example:
//
//           BEGIN_DEVICE_ALLOCATION
//
//           MIL_THR(m_pD3DDevice->CreateTexture(...));
//
//           END_DEVICE_ALLOCATION
//
//------------------------------------------------------------------------------

#define BEGIN_DEVICE_ALLOCATION \
    do \
    { \

#define END_DEVICE_ALLOCATION \
    } while (m_resourceManager.FreeSomeVideoMemory((hr))); \

//+-----------------------------------------------------------------------------
//
//  Function:
//      DbgIsPixelZoomMode()
//
//  Synopsis:
//      Is pixel zoom mode enabled
//
//------------------------------------------------------------------------------

#if DBG
// Pick an odd number for the zoom factor so that it is clear what
// side of the center of the pixel an edge lies on.
int c_dbgPixelZoomModeScale = 11;

// Top left corner zoom mode
POINT g_dbgMousePosition = {0, 0};

// HWND from last present
HWND g_dbgHWND;

bool
DbgIsPixelZoomMode()
{
    wpf::util::DpiAwarenessScope<HWND> dpiScope(g_dbgHWND);

    if (IsTagEnabled(tagPixelZoomMode))
    {
        short keyState = GetKeyState(MK_RBUTTON);

        // The high order bit of keyState indicates that the button is down, so
        // check it here.

        if (keyState & 8000)
        {
            //
            // Mouse button is down, so capture the mouse position and return false.
            //
            // Note that this code assumes that everything succeeds which is ok for
            // our special trace tag
            //

            GetCursorPos(&g_dbgMousePosition);
            ScreenToClient(g_dbgHWND, &g_dbgMousePosition);
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Create
//
//  Synopsis:
//      Create the d3ddevice and test for level1
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Create(
    __inout_ecount(1) IDirect3DDevice9 *pID3DDevice,
    __in_ecount(1) const CDisplay *pPrimaryDisplay,
    __in_ecount(1) IMILPoolManager *pManager,
    DWORD dwBehaviorFlags,
    __deref_out_ecount(1) CD3DDeviceLevel1 ** const ppDevice
    )
{
    HRESULT hr = S_OK;

    *ppDevice = NULL;

    //
    // Create CD3DDeviceLevel1
    //

    *ppDevice = new CD3DDeviceLevel1(pManager, dwBehaviorFlags);
    IFCOOM(*ppDevice);

    //
    // Call init
    //

    IFC((*ppDevice)->Init(pID3DDevice, pPrimaryDisplay));

Cleanup:
    if (FAILED(hr))
    {
        // We need to delete here since we've partially initialized the device
        // and the Release call relies on the pool manager being hooked up.
        delete (*ppDevice);
        *ppDevice = NULL;
    }
    else
    {
        (*ppDevice)->AddRef(); // CD3DDeviceLevel1::ctor sets ref count == 0
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CD3DDeviceLevel1
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DDeviceLevel1::CD3DDeviceLevel1(
    __in_ecount(1) IMILPoolManager *pManager,
    DWORD dwBehaviorFlags
    ) : CMILPoolResource(pManager),
        m_Tier(MIL_TIER(0,0)),
        m_dwD3DBehaviorFlags(dwBehaviorFlags),
        m_uFrameNumber(0),
        m_ullLastMarkerId(0),
        m_ullLastConsumedMarkerId(0),
        m_uNumSuccessfulPresentsSinceMarkerFlush(0),
        m_fHWVBlankTested(false),
        m_fHWVBlank(true),
        m_fMultisampleFailed(false),
        m_ManagedPool(D3DPOOL_MANAGED),
        m_pCachedAnisoFilterMode(NULL)
{
    m_cEntry = 0;
    m_dwThreadId = 0;

    m_pD3DDevice = NULL;
    m_pD3DDeviceEx = NULL;

    m_RenderTargetTestStatusX8R8G8B8.hrTest = WGXERR_NOTINITIALIZED;
    m_RenderTargetTestStatusX8R8G8B8.hrTestGetDC = WGXERR_NOTINITIALIZED;
    m_RenderTargetTestStatusA8R8G8B8.hrTest = WGXERR_NOTINITIALIZED;
    m_RenderTargetTestStatusA8R8G8B8.hrTestGetDC = WGXERR_NOTINITIALIZED;
    m_RenderTargetTestStatusA2R10G10B10.hrTest = WGXERR_NOTINITIALIZED;
    m_RenderTargetTestStatusA2R10G10B10.hrTestGetDC = WGXERR_NOTINITIALIZED;

    m_pCurrentRenderTargetNoRef = NULL;
    m_pDepthStencilBufferForCurrentRTNoRef = NULL;

    m_fInScene = false;

    m_fDeviceLostProcessed = false;
    m_hrDisplayInvalid = S_OK;

    m_dwGPUMarkerFlags = 0;

    m_fmtSupportFor32bppBGR =
        m_fmtSupportFor32bppPBGRA =
        m_fmtSupportFor32bppBGR101010 =
        m_fmtSupportFor128bppRGBFloat =
        m_fmtSupportFor128bppPRGBAFloat = MilPixelFormat::Undefined;

    if (g_pMediaControl)
    {
        m_dwMetricsVerticesPerFrame = 0;
        m_dwMetricsTrianglesPerFrame = 0;
    }

    ZeroMemory(&m_desc, sizeof(m_desc));

    m_uCacheIndex = CMILResourceCache::InvalidToken;

    m_pD3DDummyBackBuffer = NULL;

    m_matSurfaceToClip.reset_to_identity();

    m_pHwIndexBuffer = NULL;
    m_pHwVertexBuffer = NULL;

#if DBG_STEP_RENDERING
    m_fDbgInStepRenderingPresent = false;
    m_pDbgSaveSurface = NULL;
#endif DBG_STEP_RENDERING
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Init
//
//  Synopsis:
//      1. Creates a D3D device
//      2. Tests it for level1 support
//      3. Initializes this class
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Init(
    __inout_ecount(1) IDirect3DDevice9 *pID3DDevice,
    __in_ecount(1) const CDisplay *pDisplay
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice == NULL);
    Assert(m_pD3DDeviceEx == NULL);

    IDirect3D9* pd3d9 = NULL;

    // Initialize the resource manager as early as possible since the resource manager asserts on
    // shutdown that it has a valid device associated. If not, failures in the hardware detection code
    // below will lead to asserts firing in the D3DResourceManager code on shutdown.
    m_resourceManager.Init(this);

    //
    // Initialize basic members
    //

    m_luidD3DAdapter = pDisplay->GetLUID();

    MIL_THR(pID3DDevice->GetDeviceCaps(
        &m_caps
        ));

    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to get device caps", hr);
        goto Cleanup;
    }

    //
    // Starting with WPF 4.0, WPF will no longer support pre DX9 class hardware and hardware that
    // does not at least support PS2.0. We also require VS 2.0. In the case of the hardware not supporting
    // VS2.0 we fall back to software vertex processing (e.g. for Intel 945G).
    //
    // Hence, if we do not find a device with PS2.0 support, we fail the device creation here. Higher up in the
    // stack that will cause us to create a software renderer. The exception is 3D: For 3D software rendering we
    // are using RGBRast. RGBRast only supports fixed function and therefore we will allow the creation of a software
    // DX device for 3D here, even though it does not support PS2.0.
    //

    if (m_caps.PixelShaderVersion < D3DPS_VERSION(2,0)
        && (m_caps.DeviceType != D3DDEVTYPE_SW)
        )
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Adapter does not support PS2.0", WGXERR_INSUFFICIENT_GPU_CAPS);
        // If PixelShaderVersion is less than 2.0, fall back to software rendering.
        IFC(WGXERR_INSUFFICIENT_GPU_CAPS);
    }

    //
    // It appears that some devices (Pixomatic) can return 0 here for their Max
    // Aniso.  0 is an invalid value and the default aniso set by d3d is 1, so
    // if they return 0, overwrite it with 1.  It's possible that this could
    // fail later when we try to set it to 1, but it seems safer than trying to
    // set it to 0.
    //
    if (m_caps.MaxAnisotropy == 0)
    {
        m_caps.MaxAnisotropy = 1;
    }

    bool fSupportsMagAniso = ((m_caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0);
    bool fSupportsMinAniso = ((m_caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0);

    if (fSupportsMagAniso && fSupportsMinAniso)
    {
        m_pCachedAnisoFilterMode = &CD3DRenderState::sc_fmAnisotropic;
    }
    else if (fSupportsMinAniso)
    {
        m_pCachedAnisoFilterMode = &CD3DRenderState::sc_fmMinOnlyAnisotropic;
    }
    else
    {
        //
        // It's unlikely that a card supports Mag but not Min, and Mag aniso
        // doesn't buy us much anyway, so we just default to linear.
        //
        m_pCachedAnisoFilterMode = &CD3DRenderState::sc_fmLinear;
    }

    //
    // There is only ever one software device and it is shared so we must
    // protect it with critical section.
    //

    if (m_caps.DeviceType == D3DDEVTYPE_SW)
    {
        IFC(m_csDeviceEntry.Init());
    }

    //
    // Determine Graphics Acceleration Tier
    //

    if (m_caps.DeviceType != D3DDEVTYPE_SW)
    {
        m_Tier = pDisplay->GetTier();
    }

    {
    #pragma warning(push)
    // error C4328: 'CGuard<LOCK>::CGuard(LOCK &)' : indirection alignment of formal parameter 1 (16) is greater than the actual argument alignment (8)
    #pragma warning(disable : 4328)
    ENTER_DEVICE_FOR_SCOPE(*this);
    #pragma warning(pop)

    m_pD3DDevice = pID3DDevice;
    m_pD3DDevice->AddRef();

    IGNORE_HR(m_pD3DDevice->QueryInterface(IID_IDirect3DDevice9Ex, (void**)&m_pD3DDeviceEx));

    m_ManagedPool = IsExtendedDevice() ? D3DPOOL_MANAGED_INTERNAL : D3DPOOL_MANAGED;

    //
    // If we are rendering with a SW Rasterizer we don't need to check the drivers.
    //
    if (   CD3DRegistryDatabase::ShouldSkipDriverCheck() == false
        && m_caps.DeviceType != D3DDEVTYPE_SW
        && m_caps.DeviceType != D3DDEVTYPE_REF)
    {
        IFC(CheckBadDeviceDrivers(pDisplay));
    }

    //
    // Check for primitive count limiting trace tag
    //

#if DBG
    if (IsTagEnabled(tagLowPrimitiveCount))
    {
        m_caps.MaxPrimitiveCount = 8;
    }
#endif

    //
    // Get the implicit back buffer as the dummy
    //
    IFC(m_pD3DDevice->GetBackBuffer(
        0,
        0,
        D3DBACKBUFFER_TYPE_MONO,
        &m_pD3DDummyBackBuffer
        ));

    //
    // Detect supported target formats
    //

    MIL_THR(pDisplay->GetMode(&m_d3ddm, NULL));
    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICELOST)
        {
            MIL_THR(WGXERR_DISPLAYSTATEINVALID);
        }

        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to get adapter display mode",  hr);
        goto Cleanup;
    }

    //
    // Get IDirect3D for remaing support detection
    //

    IFC( m_pD3DDevice->GetDirect3D(&pd3d9) );

    //
    // Detect supported texture formats
    //

    GatherSupportedTextureFormats(pd3d9);

    //
    // Detect supported multisample types
    //

    GatherSupportedMultisampleTypes(pd3d9);

    //
    // Request a global cache index
    //

    IGNORE_HR(AcquireIndex());

    //
    // Initialize render state object
    //

    IFC(CD3DRenderState::Init(
        this,
        m_pD3DDevice
        ));


    //
    // Initialize hw surface render target shared data
    //

    IFC(InitSharedData(this));

    //
    // Initialize glyph bank
    //

    IFC(m_glyphBank.Init(this, &m_resourceManager));

    //
    // Create our fast path IB/VB
    //

    //
    // Size of the vertex and index buffers.  Since we're filling the buffers with
    // sets of 3 indices and sometimes sets of 3 vertices, keeping them both a
    // multiple of 3 helps the math and efficiency of the buffers slightly.
    //
    // The size of these buffers can be explored for performance characteristics.
    // Making the index buffer small will hurt performance because we will have to
    // discard it and retrieve a new one more often.  Making the vertex buffer
    // smaller will probably have a more dramatic performance impact as it causes
    // us to render non-indexed primitives.
    //
    // 20001 was picked because it's around 625kb for the vertex buffer.  This is
    // fairly large, but real scenario testing should be done to find out our
    // optimal size.
    //

    //
    // Note that all cards we support can accept VB's of 64k vertices or
    // more.  If we happen to try this creation on a card that doesn't support
    // these sizes, we end up failing to create the device and falling
    // back to software as expected.
    //


    const UINT uHwVertexBufferSize = 20001*sizeof(CD3DVertexXYZDUV2);
    const UINT uHwIndexBufferSize = 20001*3*sizeof(WORD);

    IFC(CHwD3DIndexBuffer::Create(
        &m_resourceManager,
        this,
        uHwIndexBufferSize,
        &m_pHwIndexBuffer
        ));

    IFC(CHwD3DVertexBuffer::Create(
        &m_resourceManager,
        this,
        uHwVertexBufferSize,
        &m_pHwVertexBuffer
        ));

    // Do basic device tests
    IFC(TestLevel1Device());

    m_presentFailureWindowMessage = RegisterWindowMessage(L"NeedsRePresentOnWake");

    } // Leave device scope

Cleanup:
    ReleaseInterfaceNoNULL(pd3d9);

    if (FAILED(hr))
    {
        ReleaseInterface(m_pD3DDevice);
        ReleaseInterface(m_pD3DDeviceEx);
    }
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GatherSupportedTextureFormats
//
//  Synopsis:
//      Inspect texture formats supported by device and setup our mappings from
//      given MilPixelFormat::Enum to supported MilPixelFormat::Enum
//

void
CD3DDeviceLevel1::GatherSupportedTextureFormats(
    __in_ecount(1) IDirect3D9 *pd3d9
    )
{
    //
    // Check on support for one of three 8 bit formats for use with text
    //

    m_fSupportsD3DFMT_A8 = SUCCEEDED( pd3d9->CheckDeviceFormat(
        m_caps.AdapterOrdinal,
        m_caps.DeviceType,
        m_d3ddm.Format,
        0,
        D3DRTYPE_TEXTURE,
        D3DFMT_A8
        ) );
    m_fSupportsD3DFMT_P8 = SUCCEEDED( pd3d9->CheckDeviceFormat(
        m_caps.AdapterOrdinal,
        m_caps.DeviceType,
        m_d3ddm.Format,
        0,
        D3DRTYPE_TEXTURE,
        D3DFMT_P8
        ) );
    m_fSupportsD3DFMT_L8 = SUCCEEDED( pd3d9->CheckDeviceFormat(
        m_caps.AdapterOrdinal,
        m_caps.DeviceType,
        m_d3ddm.Format,
        0,
        D3DRTYPE_TEXTURE,
        D3DFMT_L8
        ) );

    //
    // Check for support for general rendering formats.
    //
    // Check higher precision formats first; so that lower precision formats
    // can map to higher precision ones if needed.
    //

    m_fmtSupportFor128bppPRGBAFloat =
        SUCCEEDED( pd3d9->CheckDeviceFormat(
            m_caps.AdapterOrdinal,
            m_caps.DeviceType,
            m_d3ddm.Format,
            0,
            D3DRTYPE_TEXTURE,
            D3DFMT_A32B32G32R32F
            ) ) ?
        MilPixelFormat::PRGBA128bppFloat : MilPixelFormat::Undefined;

    // There is no specific support in D3D for 128bpp w/o alpha; so,
    // use the alpha form.
    m_fmtSupportFor128bppRGBFloat = m_fmtSupportFor128bppPRGBAFloat;

    m_fmtSupportFor32bppBGR101010 =
        SUCCEEDED( pd3d9->CheckDeviceFormat(
            m_caps.AdapterOrdinal,
            m_caps.DeviceType,
            m_d3ddm.Format,
            0,
            D3DRTYPE_TEXTURE,
            D3DFMT_A2R10G10B10
            ) ) ?
        MilPixelFormat::BGR32bpp101010 : m_fmtSupportFor128bppRGBFloat;

    m_fmtSupportFor32bppPBGRA =
        SUCCEEDED( pd3d9->CheckDeviceFormat(
            m_caps.AdapterOrdinal,
            m_caps.DeviceType,
            m_d3ddm.Format,
            0,
            D3DRTYPE_TEXTURE,
            D3DFMT_A8R8G8B8
            ) ) ?
        MilPixelFormat::PBGRA32bpp : m_fmtSupportFor128bppPRGBAFloat;

    m_fmtSupportFor32bppBGR =
        SUCCEEDED( pd3d9->CheckDeviceFormat(
            m_caps.AdapterOrdinal,
            m_caps.DeviceType,
            m_d3ddm.Format,
            0,
            D3DRTYPE_TEXTURE,
            D3DFMT_X8R8G8B8
            ) ) ?
        MilPixelFormat::BGR32bpp :
        ((m_fmtSupportFor32bppPBGRA != MilPixelFormat::Undefined) ?
            // First try PBGRA since converions is probably easier
            m_fmtSupportFor32bppPBGRA :
            // then go for B10G10R10
            m_fmtSupportFor32bppBGR101010
         );

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetMaxMultisampleTypeWithDepthSupport
//
//  Synopsis:
//      Search for highest multisample type supported for given target surface
//      and depth.  If none are supported D3DMULTISAMPLE_NONE is returned.
//
//  Note:
//      d3dfmtDepth argument maybe be set to d3dfmtTarget to avoid a secondary
//      check.  Both arguments accept any D3DFORMAT.
//

D3DMULTISAMPLE_TYPE
CD3DDeviceLevel1::GetMaxMultisampleTypeWithDepthSupport(
    __in_ecount(1) IDirect3D9 *pd3d,
    D3DFORMAT d3dfmtTarget,
    D3DFORMAT d3dfmtDepth,
    D3DMULTISAMPLE_TYPE MaxMultisampleType
    ) const
{
    Assert(MaxMultisampleType <= D3DMULTISAMPLE_16_SAMPLES);

    while (MaxMultisampleType >= D3DMULTISAMPLE_2_SAMPLES)
    {
        if (SUCCEEDED(pd3d->CheckDeviceMultiSampleType(
            m_caps.AdapterOrdinal,
            m_caps.DeviceType,
            d3dfmtTarget,
            true,
            MaxMultisampleType,
            NULL
           )))
        {
            if (   d3dfmtTarget == d3dfmtDepth
                   // Depth format isn't same as target so check it too
                || SUCCEEDED(pd3d->CheckDeviceMultiSampleType(
                    m_caps.AdapterOrdinal,
                    m_caps.DeviceType,
                    d3dfmtDepth,
                    true,
                    MaxMultisampleType,
                    NULL
                    ))
               )
            {
                break;
            }
        }

        reinterpret_cast<DWORD &>(MaxMultisampleType)--;
    }

    if (MaxMultisampleType < D3DMULTISAMPLE_2_SAMPLES)
    {
        MaxMultisampleType = D3DMULTISAMPLE_NONE;
    }

    return MaxMultisampleType;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GatherSupportedMultisampleTypes
//
//  Synopsis:
//      Inspect target formats supported by device and setup our mappings from
//      given MilPixelFormat::Enum to mutlisample type
//

void
CD3DDeviceLevel1::GatherSupportedMultisampleTypes(
    __in_ecount(1) IDirect3D9 *pd3d
    )
{
    //
    // Check for multisample support for general rendering formats.
    //

    D3DMULTISAMPLE_TYPE MaxMultisampleType =
        IsLDDMDevice() ? D3DMULTISAMPLE_4_SAMPLES : D3DMULTISAMPLE_NONE;

    // Get default multi-sample max from the registry
    CDisplayRegKey keyDisplay(HKEY_LOCAL_MACHINE, _T(""));

    keyDisplay.ReadDWORD(
        _T("MaxMultisampleType"),
        reinterpret_cast<DWORD *>(&MaxMultisampleType)
        );

    // Filter first by maximum depth buffer support
    MaxMultisampleType = GetMaxMultisampleTypeWithDepthSupport(
        pd3d,
        kD3DDepthFormat,
        kD3DDepthFormat,
        MaxMultisampleType
        );

    m_multisampleTypeFor32bppBGR = GetMaxMultisampleTypeWithDepthSupport(
        pd3d,
        D3DFMT_X8R8G8B8,
        kD3DDepthFormat,
        MaxMultisampleType
        );

    m_multisampleTypeFor32bppPBGRA = GetMaxMultisampleTypeWithDepthSupport(
        pd3d,
        D3DFMT_A8R8G8B8,
        kD3DDepthFormat,
        MaxMultisampleType
        );

    m_multisampleTypeFor32bppBGR101010 = GetMaxMultisampleTypeWithDepthSupport(
        pd3d,
        D3DFMT_A2R10G10B10,
        kD3DDepthFormat,
        MaxMultisampleType
        );

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::~CD3DDeviceLevel1
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DDeviceLevel1::~CD3DDeviceLevel1()
{
    Assert(m_cEntry == 0);
    Assert(m_dwThreadId == 0);
    Enter();

    ResetMarkers();

    ReleaseInterface(m_pEffectPipelineVertexShader20);
    ReleaseInterface(m_pEffectPipelineVertexShader30);
    ReleaseInterface(m_pEffectPipelineVertexBuffer);
    ReleaseInterface(m_pEffectPipelinePassThroughPixelShader);
    ReleaseInterface(m_pD3DDummyBackBuffer);
    ReleaseInterface(m_pD3DDeviceEx);
    m_pCurrentRenderTargetNoRef = NULL;   // No longer care about what is set.
                                          // This makes sure
                                          // ReleaseUseOfRenderTarget does
                                          // nothing.

    ReleaseInterface(m_pHwIndexBuffer);
    ReleaseInterface(m_pHwVertexBuffer);

    m_resourceManager.DestroyAllResources();

    // m_pDepthStencilBufferForCurrentRTNoRef should have been released from
    // use at this point
    Assert(m_pDepthStencilBufferForCurrentRTNoRef == NULL);

    ReleaseInterface(m_pD3DDevice);

#if DBG
    // Only need to leave for entry assert checks in ~CAssertEntry
    Leave();
#endif

}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Enter and Leave
//
//  Synopsis:
//      Thread protection marking methods
//
//      Call Enter when device is about to used in a way that requires exclusive
//      access and Leave when leaving that context.  This is most commonly done
//      when handling a drawing routine, which the caller is required to provide
//      protection for.
//
//      Enter and Leave must be paired exactly.
//

void
CD3DDeviceLevel1::Enter(
    )
{
    if (IsEnsuringCorrectMultithreadedRendering())
    {
        m_csDeviceEntry.Enter();
    }

    // This call should be protected from double thread entry by the caller.

#if DBG
    // Attempt to catch simultaneous entry from two threads
    m_oDbgEntryCheck.Enter();
#endif

    m_cEntry++;
    m_dwThreadId = GetCurrentThreadId();

    Assert(m_cEntry > 0);
}

void
CD3DDeviceLevel1::Leave(
    )
{
    // This call should be protected from double thread entry by the caller
    // just like Enter was.

    Assert(m_cEntry > 0);

    // Should leave using same thread we entered on.
    Assert(m_dwThreadId == GetCurrentThreadId());

    if (--m_cEntry == 0)
    {
        m_dwThreadId = 0;
    }

#if DBG
    // Attempt to catch simultaneous entry from two threads
    m_oDbgEntryCheck.Leave();
#endif

    if (IsEnsuringCorrectMultithreadedRendering())
    {
        m_csDeviceEntry.Leave();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsProtected
//
//  Synopsis:
//      Return true if this context is protected
//

bool
CD3DDeviceLevel1::IsProtected(
    bool fForceEntryConfirmation    // Ignore lack of multithreaded usage flag
                                    // when checking for protection.
    ) const
{
    // Check if we are always protected (and entry confirmation isn't required)
    // or if not if this thread has been marked/entered.
    bool fProtected = (   (   !fForceEntryConfirmation
                           && !(m_dwD3DBehaviorFlags & D3DCREATE_MULTITHREADED))
                       || (m_dwThreadId == GetCurrentThreadId()));

    if (fProtected)
    {
        AssertEntry(m_oDbgEntryCheck);
        if (   (m_dwD3DBehaviorFlags & D3DCREATE_MULTITHREADED)
            || fForceEntryConfirmation)
        {
            Assert(m_cEntry > 0);
            Assert(m_dwThreadId == GetCurrentThreadId());
        }
    }

    return fProtected;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsEntered
//
//  Synopsis:
//      Return true if this thread has been marked/entered as protected
//

bool
CD3DDeviceLevel1::IsEntered(
    ) const
{
    // Calling this method implies that either this thread is the only thread
    // that could have entered the device protection.  Therefore we should use
    // the entry check assert.
    AssertEntry(m_oDbgEntryCheck);

    bool fEntered = (m_cEntry > 0);

    if (fEntered)
    {
        // If entered this should be the marked thread
        Assert(m_dwThreadId == GetCurrentThreadId());
    }
    else
    {
        // If not entered there should be no thread ID marked
        Assert(m_dwThreadId == 0);
    }

    return fEntered;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::TestRenderTargetFormat
//
//  Synopsis:
//      Test the device to see if it is usable with this render target format
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::TestRenderTargetFormat(
    D3DFORMAT fmtRenderTarget,
    __inout_ecount(1) TargetFormatTestStatus *pFormatTestEntry
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    CD3DSurface *pD3DSurface = NULL;
    CD3DLockableTexture *pLockableTexture = NULL;
    D3DDEVICE_CREATION_PARAMETERS d3dCreateParams;
    D3DSURFACE_DESC d3dsd;
    CD3DSwapChain *pD3DSwapChain = NULL;
    IDirect3D9 *pD3D = NULL;

    //
    // Get d3d object and adapter
    //

    MIL_THR(m_pD3DDevice->GetDirect3D(&pD3D));
    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to get d3d object",  hr);
        goto Cleanup;
    }

    MIL_THR(m_pD3DDevice->GetCreationParameters(&d3dCreateParams));
    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to get creation parameters",  hr);
        goto Cleanup;
    }

    if (m_caps.DeviceType == D3DDEVTYPE_HAL)
    {
        //
        // Check for our depth buffer
        //

        MIL_THR(pD3D->CheckDepthStencilMatch(
            d3dCreateParams.AdapterOrdinal,
            D3DDEVTYPE_HAL,
            m_d3ddm.Format,
            fmtRenderTarget,
            kD3DDepthFormat
            ));

        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Can't get 24-bit z-buffer",  hr);
            goto Cleanup;
        }
    }

    // Render Target create routines are about to be used and they expect this
    // format to have been successfully tested.  So, set status to success now
    // with the expectation that the real status will be set later.
    // WGXHR_INTERNALTEMPORARYSUCCESS is used to indicate success, but also
    // note that this value should not last.
    WHEN_DBG_ANALYSIS(pFormatTestEntry->hrTest = WGXHR_INTERNALTEMPORARYSUCCESS);

    if (!d3dCreateParams.hFocusWindow)
    {
        //
        // Try to create a lockable render target
        //

        hr = CreateRenderTarget(
            128,
            128,
            fmtRenderTarget,
            D3DMULTISAMPLE_NONE,
            0,
            true,
            &pD3DSurface
            );

        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to create render target",  hr);
            goto Cleanup;
        }
    }
    else
    {
        //
        // Try to create a lockable secondary swap chain
        //
        D3DPRESENT_PARAMETERS d3dpp;

        ZeroMemory(&d3dpp, sizeof(d3dpp));
        d3dpp.Windowed = true;
        d3dpp.BackBufferWidth = 128;
        d3dpp.BackBufferHeight = 128;
        d3dpp.BackBufferFormat = fmtRenderTarget;
        d3dpp.BackBufferCount = 1;
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
        d3dpp.hDeviceWindow = d3dCreateParams.hFocusWindow;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

        MIL_THR(CreateAdditionalSwapChain(
            NULL, //pMILDC
            &d3dpp,
            &pD3DSwapChain
            ));
        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to create swap chain",  hr);
            goto Cleanup;
        }

        MIL_THR(pD3DSwapChain->GetBackBuffer(0,  &pD3DSurface));
        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to get swap chain back buffer",  hr);
            goto Cleanup;
        }

        TestGetDC(pD3DSurface, IN OUT pFormatTestEntry);
    }

    MIL_THR(SetRenderTarget(pD3DSurface));
    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to set render target",  hr);
        goto Cleanup;
    }

    MIL_THR(SetDepthStencilSurface(NULL));
    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to reset depth stencil surface",  hr);
        goto Cleanup;
    }

    //
    // Check that we can get our favorite texture format and
    // try to render it
    //

    ZeroMemory(&d3dsd, sizeof(d3dsd));
    d3dsd.Format = PixelFormatToD3DFormat(MilPixelFormat::BGRA32bpp);
    d3dsd.Type = D3DRTYPE_TEXTURE;
    d3dsd.Usage = 0;
    d3dsd.Pool = m_ManagedPool;
    d3dsd.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dsd.MultiSampleQuality = 0;
    d3dsd.Width = 128;
    d3dsd.Height = 128;

    MIL_THR(CreateLockableTexture(
        &d3dsd,
        &pLockableTexture
        ));

    if (FAILED(hr))
    {
        TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to create 32-bit ARGB texture",  hr);
        goto Cleanup;
    }

    {
        MilPointAndSizeL rc = {0, 0, 128, 128};
        MIL_THR(RenderTexture(pLockableTexture, rc, TBM_DEFAULT/* premultiplied */));
        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to draw texture",  hr);
            goto Cleanup;
        }
    }

Cleanup:

    //
    // Process test results.
    //
    // If the failure is due to a lack of capability, then don't bother to
    // create future targets of this format for this device.  Out of memory or
    // driverinternal errors can all be context dependent so we need to
    // evaluate each time.  For example, D3DERR_DRIVERINTERNALERROR has been
    // seen when we are low on video memory and try to lock a surface.
    //
    // Note: These settings should only persist until the device capabilities
    //  can change.  Display mode change gives us this notification and these
    //  settings will be cleared.
    //

    pFormatTestEntry->hrTest = hr;

    if (FAILED(hr))
    {
        if (   (hr == D3DERR_OUTOFVIDEOMEMORY)
            || (hr == E_OUTOFMEMORY)
            || (hr == D3DERR_DRIVERINTERNALERROR))
        {
            // This case doesn't actually determine usability; so reset back to
            // untested (not initialized).
            pFormatTestEntry->hrTest = WGXERR_NOTINITIALIZED;
        }
        else
        {
            // If there is a failure and GetDC test status is not initialized
            // then update it with the general failure status.
            if (pFormatTestEntry->hrTestGetDC == WGXERR_NOTINITIALIZED)
            {
                pFormatTestEntry->hrTestGetDC = pFormatTestEntry->hrTest;
            }
        }
    }

    ReleaseInterfaceNoNULL(pD3DSurface);
    ReleaseInterfaceNoNULL(pLockableTexture);
    ReleaseInterfaceNoNULL(pD3DSwapChain);
    ReleaseInterfaceNoNULL(pD3D);

    RRETURN(hr); // D3DERR_DRIVERINTERNALERROR OK here
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::TestLevel1Device
//
//  Synopsis:
//      Test the device to see if it is basically usable
//
//------------------------------------------------------------------------------

HRESULT
CD3DDeviceLevel1::TestLevel1Device(
    )
{
    HRESULT hr = S_OK;

    //
    // Check Device Caps
    //

    IFC(HwCaps::CheckDeviceLevel1(m_caps));

    //
    // Test some render states
    //

    // This is a shotgun approach. We can't really enumerate all combinations of
    // state that might cause the driver to return failure. The driver might
    // only fail later, when it's sure we intend to use a particular combination
    // of state (e.g. at DrawPrimitive). And, testing for a failed return code,
    // isn't enough.

    //   Do better HW caps testing
    //   Ultimately, we need to be confident that any state we choose to set
    //   later on, will succeed, produce correct rendering, and not cause system
    //   instability.

#define IFC_RSCHECK(expr) {MIL_THR(expr); if (FAILED(hr)) {                              \
    TRACE_DEVICECREATE_FAILURE(m_caps.AdapterOrdinal, "Failed to set render states", hr); \
    goto Cleanup;}}

    IFC_RSCHECK( SetRenderState_AlphaSolidBrush() );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::NearestNeighbor, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::Linear, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::TriLinear, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::NearestNeighbor, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::Linear, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::TriLinear, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::NearestNeighbor, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::Linear, 0) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::TriLinear, 0) );

    IFC_RSCHECK( SetRenderState_AlphaSolidBrush() );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::NearestNeighbor, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::Linear, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_None, MilBitmapInterpolationMode::TriLinear, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::NearestNeighbor, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::Linear, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Diffuse, MilBitmapInterpolationMode::TriLinear, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::NearestNeighbor, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::Linear, 1) );
    IFC_RSCHECK( SetRenderState_Texture( TBM_DEFAULT, TBA_Specular, MilBitmapInterpolationMode::TriLinear, 1) );
#undef IFC_RSCHECK

    //
    // Let people know that everything is ok!
    //

    if (m_caps.DeviceType == D3DDEVTYPE_SW)
    {
        TraceTag((tagError, "MIL-HW(adapter=%d): d3d software device tested successfully. (For SW 3D use only.)", m_caps.AdapterOrdinal));
    }
    else
    {
        TraceTag((tagError, "MIL-HW(adapter=%d): d3d device tested successfully.", m_caps.AdapterOrdinal));
    }

Cleanup:
    //
    // If the failure is due to a lack of capability, then don't
    // bother to create future devices for this adapter.  Out of
    // memory or driverinternal errors can all be context dependent
    // so we need to evaluate each time.  For example,
    // D3DERR_DRIVERINTERNALERROR has been seen when we are low on
    // video memory and try to lock a surface.
    //
    // Note: These settings should only persist until the device capabilities
    //  can change.  Display mode change gives us this notification and these
    //  settings will be cleared.
    //

    if (FAILED(hr) &&
        (hr != D3DERR_OUTOFVIDEOMEMORY) &&
        (hr != E_OUTOFMEMORY) &&
        (hr != D3DERR_DRIVERINTERNALERROR) &&
        (m_caps.DeviceType == D3DDEVTYPE_HAL))
    {
        IGNORE_HR(CD3DRegistryDatabase::DisableAdapter(m_caps.AdapterOrdinal));
    }

    RRETURN(hr); // D3DERR_DRIVERINTERNALERROR OK here
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::TestGetDC
//
//  Synopsis:
//      Test to see if we can obtain a DC from this surface.  Record the result
//      into the device (TargetFormatTestStatus *), so we can avoid creating
//      surfaces that will require a DC.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::TestGetDC(
    __inout_ecount(1) CD3DSurface * const pD3DSurface,
    __inout_ecount(1) TargetFormatTestStatus * const pFormatTestEntry
    )
{
    HDC hTestDC = NULL;
    HRESULT hrGetDC;

    Assert(pFormatTestEntry->hrTestGetDC == WGXERR_NOTINITIALIZED);

    // Make a test GetDC call
    MIL_THRX(hrGetDC, pD3DSurface->GetDC(&hTestDC));

    Assert(hrGetDC != WGXERR_NOTINITIALIZED);

    pFormatTestEntry->hrTestGetDC = hrGetDC;

    if (hTestDC)
    {
        pD3DSurface->ReleaseDC(hTestDC);
    }

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      HwCaps::CheckDeviceLevel1
//
//  Synopsis:
//      Check the caps for a device- fail if the device does not support caps
//      that we need.
//
//------------------------------------------------------------------------------
HRESULT
HwCaps::CheckDeviceLevel1(
    __in_ecount(1) const D3DCAPS9 &caps
    )
{
    HRESULT hr = S_OK;

    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2 &&
        !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
    {
        hr = E_FAIL;
        TRACE_DEVICECREATE_FAILURE(caps.AdapterOrdinal, "Non power of 2 textures support must be present for hw acceleration",  hr);
        goto Cleanup;
    }

    //
    // Check for non square textures
    //
    if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
    {
        hr = E_FAIL;
        TRACE_DEVICECREATE_FAILURE(caps.AdapterOrdinal, "Non square texture support must be present for hw acceleration",  hr);
        goto Cleanup;
    }

    //
    // Check for multi-texturing and color masking
    //

    if (caps.MaxTextureBlendStages < 2
        || caps.MaxSimultaneousTextures < 2)
    {
        hr = E_FAIL;
        TRACE_DEVICECREATE_FAILURE(caps.AdapterOrdinal, "We need at least 2 texture stages",  hr);
        goto Cleanup;
    }

    // Since we intend on only using D3DDEVTYPE_SW for 3D, we don't
    // care about color masking which is only used for text.

    if (!IsSWDevice(caps))
    {
        if (!CanMaskColorChannels(caps))
        {
            hr = E_FAIL;
            TRACE_DEVICECREATE_FAILURE(caps.AdapterOrdinal, "Color masking support must be present for hw acceleration",  hr);
            goto Cleanup;
        }
    }

    //
    // Check for blending capabilities
    //

    {
        UINT requiredSrcCaps = 0
            | D3DPBLENDCAPS_ZERO
            | D3DPBLENDCAPS_ONE
/*          | D3DPBLENDCAPS_SRCCOLOR                unused*/
/*          | D3DPBLENDCAPS_INVSRCCOLOR             unused*/
            | D3DPBLENDCAPS_SRCALPHA
/*          | D3DPBLENDCAPS_INVSRCALPHA             unused*/
/*          | D3DPBLENDCAPS_DESTALPHA               unused*/
            | D3DPBLENDCAPS_INVDESTALPHA
/*          | D3DPBLENDCAPS_DESTCOLOR               unused*/
/*          | D3DPBLENDCAPS_INVDESTCOLOR            unused*/
/*          | D3DPBLENDCAPS_SRCALPHASAT             unused*/
/*          | D3DPBLENDCAPS_BOTHSRCALPHA            unused*/
/*          | D3DPBLENDCAPS_BOTHINVSRCALPHA         unused*/
/*          | D3DPBLENDCAPS_BLENDFACTOR      used but we allow unsupported blend factor*/
        ;

        UINT requiredDestCaps = 0
            | D3DPBLENDCAPS_ZERO
            | D3DPBLENDCAPS_ONE
/*          | D3DPBLENDCAPS_SRCCOLOR                unused*/
            | D3DPBLENDCAPS_INVSRCCOLOR
/*          | D3DPBLENDCAPS_SRCALPHA                unused*/
            | D3DPBLENDCAPS_INVSRCALPHA
/*          | D3DPBLENDCAPS_DESTALPHA               unused*/
/*          | D3DPBLENDCAPS_INVDESTALPHA            unused*/
/*          | D3DPBLENDCAPS_DESTCOLOR               unused*/
/*          | D3DPBLENDCAPS_INVDESTCOLOR            unused*/
/*          | D3DPBLENDCAPS_SRCALPHASAT             unused*/
/*          | D3DPBLENDCAPS_BOTHSRCALPHA            unused*/
/*          | D3DPBLENDCAPS_BOTHINVSRCALPHA         unused*/
/*          | D3DPBLENDCAPS_BLENDFACTOR      used but we allow unsupported blend factor*/
        ;

        if ((caps.SrcBlendCaps  & requiredSrcCaps ) != requiredSrcCaps ||
            (caps.DestBlendCaps & requiredDestCaps) != requiredDestCaps)
        {
            hr = E_FAIL;
            TRACE_DEVICECREATE_FAILURE(caps.AdapterOrdinal, "Device doesn't support all the required blending modes",  hr);
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetRenderTargetFormatTestEntry
//
//  Synopsis:
//      Get the test status entry of a particular render target format.
//
//------------------------------------------------------------------------------

HRESULT
CD3DDeviceLevel1::GetRenderTargetFormatTestEntry(
    D3DFORMAT fmtRenderTarget,
    __deref_out_ecount(1) TargetFormatTestStatus * &pFormatTestEntry
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*this);

    pFormatTestEntry = NULL;

    switch (fmtRenderTarget)
    {
    case D3DFMT_X8R8G8B8:
        pFormatTestEntry = &m_RenderTargetTestStatusX8R8G8B8;
        break;

    case D3DFMT_A8R8G8B8:
        pFormatTestEntry = &m_RenderTargetTestStatusA8R8G8B8;
        break;

    case D3DFMT_A2R10G10B10:
        pFormatTestEntry = &m_RenderTargetTestStatusA2R10G10B10;
        break;

    default:
        RIP("Unsupported render target format.");
        IFC(E_INVALIDARG);
        break;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CheckRenderTargetFormat
//
//  Synopsis:
//      Check the device to see if it is usable with this render target format.
//      Testing will only happen the first time the format is introduced to the
//      device.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceLevel1::CheckRenderTargetFormat(
    D3DFORMAT fmtRenderTarget,
    __deref_opt_out_ecount(1) HRESULT const ** const pphrTestGetDC
    )
{
    #pragma warning(push)
    // error C4328: 'CGuard<LOCK>::CGuard(LOCK &)' : indirection alignment of formal parameter 1 (16) is greater than the actual argument alignment (8)
    #pragma warning(disable : 4328)
    ENTER_DEVICE_FOR_SCOPE(*this);
    #pragma warning(pop)

    HRESULT hr = S_OK;
    TargetFormatTestStatus *pRenderTargetTestStatus = NULL;

    IFC(GetRenderTargetFormatTestEntry(
        fmtRenderTarget,
        OUT pRenderTargetTestStatus
        ));

    // Return pointer to GetDC test status if requested.
    if (pphrTestGetDC)
    {
        *pphrTestGetDC = &pRenderTargetTestStatus->hrTestGetDC;
    }

    if (pRenderTargetTestStatus->hrTest != WGXERR_NOTINITIALIZED)
    {
        IFC(pRenderTargetTestStatus->hrTest);
        goto Cleanup;
    }

    IFC(TestRenderTargetFormat(fmtRenderTarget, pRenderTargetTestStatus));

Cleanup:
    if (FAILED(hr))
    {
        TraceTag((tagError, "MIL-HW(adapter=%d): d3d device failed testing.", m_caps.AdapterOrdinal));
    }

    RRETURN(hr); // Let DIE through
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::AssertRenderFormatIsTestedSuccessfully
//
//  Synopsis:
//      Trigger an assert if given format has not been tested as a render target
//      or that test failed.
//
//------------------------------------------------------------------------------

void
CD3DDeviceLevel1::AssertRenderFormatIsTestedSuccessfully(
    D3DFORMAT fmtRenderTarget
    )
{
#if DBG_ANALYSIS
    #pragma warning(push)
    // error C4328: 'CGuard<LOCK>::CGuard(LOCK &)' : indirection alignment of formal parameter 1 (16) is greater than the actual argument alignment (8)
    #pragma warning(disable : 4328)
    ENTER_DEVICE_FOR_SCOPE(*this);
    #pragma warning(pop)

    TargetFormatTestStatus *pRenderTargetTestStatus = NULL;

    Assert(SUCCEEDED(GetRenderTargetFormatTestEntry(
        fmtRenderTarget,
        OUT pRenderTargetTestStatus
        )));

    if (pRenderTargetTestStatus)
    {
        Assert(pRenderTargetTestStatus->hrTest != WGXERR_NOTINITIALIZED);
        Assert(SUCCEEDED(pRenderTargetTestStatus->hrTest));
    }
#endif
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CheckBadDeviceDrivers
//
//  Synopsis:
//      Modifies caps to disable buggy features of bad device drivers
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CheckBadDeviceDrivers(
    __in_ecount(1) const CDisplay *pDisplay
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    Assert(m_caps.AdapterOrdinal == pDisplay->GetDisplayIndex());

    if (!pDisplay->IsRecentDriver())
    {
        IFC(E_FAIL);
    }
    else if (pDisplay->IsDeviceDriverBad())
    {
        MIL_THR(E_FAIL);

        if (pDisplay->GetVendorId() == GraphicsCardVendorIntel
            && pDisplay->GetDeviceId() == GraphicsCardIntel_845G)
        {
            TRACE_DEVICECREATE_FAILURE(
                m_caps.AdapterOrdinal,
                "Intel 845 disabled due to performance problems and a GPU bug.",
                hr);
        }
        else
        {
            TRACE_DEVICECREATE_FAILURE(
                m_caps.AdapterOrdinal,
                "Device has been disabled due to driver problems.",
                hr);
        }

        goto Cleanup;
    }

    {
        //
        //
        // On many pieces of nvidia hardware, scissor rect has artifacts.  For
        // right now we are disabling it.
        //
        //
        // Disabling for all hardware because we don't have enough test time
        // to verify that all issues are gone. This is fine because the
        // optimization doesn't make any measurable improvement to DWM or
        // WPF perf tests.
        //
        m_caps.RasterCaps &= ~D3DPRASTERCAPS_SCISSORTEST;

        Assert((m_caps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST) == 0);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::MarkUnusable
//
//  Synopsis:
//      Mark this device as unusable and notify manager. Future calls to Present
//      will return device lost.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::MarkUnusable(
    bool fMayBeMultithreadedCall
    )
{
    // No entry check as this method is thread safe

    if (m_hrDisplayInvalid == D3DERR_DRIVERINTERNALERROR)
    {
        IGNORE_HR(CD3DRegistryDatabase::HandleAdapterUnexpectedError(m_caps.AdapterOrdinal));
    }

    //
    // Future calls to Present will return display invalid
    //

    m_hrDisplayInvalid = WGXERR_DISPLAYSTATEINVALID;

    //
    // We can only safely access this device's m_resourceManager and the
    // m_pManager when on this device's rendering thread.  If we're on a
    // different thread defer those operations.
    //

    if (   !m_fDeviceLostProcessed
        && IsProtected(fMayBeMultithreadedCall)
#if DBG_STEP_RENDERING
           // Don't process this if within stepped rendering because the
           // primitive may be using cached resources w/o a reference to them.
           // For example CHwSurfaceRenderTarget::DrawBitmap does that with the
           // m_pDrawBitmapScratchBrush.
        && !DbgInStepRenderingPresent()
#endif DBG_STEP_RENDERING
       )
    {
        // Destroy all GPUMarkers created using this device
        ResetMarkers();

        m_fDeviceLostProcessed = true;

        //
        // Notify the manager this device is unusable
        //

        m_pManager->UnusableNotification(this);

        //
        // Future Consideration:   Have state manager release resources
        // since we know what is set we can set all state to NULL and eliminate
        // any internal D3D references.  The need is not pressing as we expect
        // the device will be fully released soon enough and will truely free
        // all associated resources when it does.  Note that we expect this,
        // but haven't validated D3D behavior.
        //

        //
        // Attempt to destroy all resources that are now also lost/unusable
        //
        // There is a slim chance that the only thing keeping this device alive
        // is an outstanding CD3DResource and since DestroyAllResources could
        // eliminate that reference make sure this is the last call of this
        // method.  Note what makes this unlikely is that we check for device
        // protection above and currently the only way I know to get that is
        // through a RT which is not currently a resource, but does hold a
        // reference to the device.
        //

        //
        // *** NOTE: UnusableNotification is depending upon this for D3DImage
        //

        m_resourceManager.DestroyAllResources();

    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetSwapChain
//
//  Synopsis:
//      delegate to IDirect3DDevice9::GetSwapChain
//      Note: we always create a new wrapper object
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::GetSwapChain(
    UINT uGroupAdapterOrdinal,
    __deref_out_ecount(1) CD3DSwapChain ** const ppSwapChain
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    IDirect3DSwapChain9 *pD3DSwapChain = NULL;

    *ppSwapChain = NULL;

    //
    // Delegate to GetSwapChain
    //

    IFC(m_pD3DDevice->GetSwapChain(uGroupAdapterOrdinal, &pD3DSwapChain));

    //
    // Create swap chain wrapper
    //

    IFC(CD3DSwapChain::Create(
        &m_resourceManager,
        pD3DSwapChain,
        0,
        NULL, // pPresentContext- NULL indicates normal GetDC behavior
        ppSwapChain
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pD3DSwapChain);
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateRenderTarget
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateRenderTargetUntracked, then place
//      a resource wrapper around it
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateRenderTarget(
    UINT uiWidth,
    UINT uiHeight,
    D3DFORMAT d3dfmtSurface,
    D3DMULTISAMPLE_TYPE d3dMultiSampleType,
    DWORD dwMultisampleQuality,
    bool fLockable,
    __deref_out_ecount(1) CD3DSurface ** const ppD3DSurface
    )
{
    HRESULT hr = S_OK;
    IDirect3DSurface9 *pID3DSurface = NULL;

    IFC(CreateRenderTargetUntracked(
        uiWidth,
        uiHeight,
        d3dfmtSurface,
        d3dMultiSampleType,
        dwMultisampleQuality,
        fLockable,
        &pID3DSurface
        ));

    IFC(CD3DSurface::Create(&m_resourceManager, pID3DSurface, ppD3DSurface));

Cleanup:
    ReleaseInterface(pID3DSurface);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateRenderTargetUntracked
//
//  Synopsis:
//      Delegate to IDirect3DDevice9::CreateRenderTarget. This method is called
//      "Untracked" because the surface created is not tracked by our resource
//      management system. This version of CreateRenderTarget should only be
//      called if absolutely necessary.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateRenderTargetUntracked(
    UINT uiWidth,
    UINT uiHeight,
    D3DFORMAT d3dfmtSurface,
    D3DMULTISAMPLE_TYPE d3dMultiSampleType,
    DWORD dwMultisampleQuality,
    bool fLockable,
    __deref_out_ecount(1) IDirect3DSurface9 ** const ppD3DSurface
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pID3DSurface = NULL;

    *ppD3DSurface = NULL;

    AssertDeviceEntry(*this);

    AssertRenderFormatIsTestedSuccessfully(d3dfmtSurface);

    BEGIN_DEVICE_ALLOCATION;

    {
        MtSetDefault(Mt(D3DRenderTarget));

        hr = m_pD3DDevice->CreateRenderTarget(
            uiWidth,
            uiHeight,
            d3dfmtSurface,
            d3dMultiSampleType,
            dwMultisampleQuality,
            fLockable,
            &pID3DSurface,
            NULL
            );
    }

    END_DEVICE_ALLOCATION;

    IFC(hr); // placed here to avoid stack capturing multiple times in allocation loop

    *ppD3DSurface = pID3DSurface; // Steal ref
    pID3DSurface = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pID3DSurface);

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetRenderTargetData
//
//  Synopsis:
//      Delegate to IDirect3DDevice9::GetRenderTargetData, Copies data from a
//      render target source surface to a system memory destination surface.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::GetRenderTargetData(
    __in_ecount(1) IDirect3DSurface9 *pSourceSurface,
    __in_ecount(1) IDirect3DSurface9 *pDestinationSurface
    )
{
    HRESULT hr = S_OK;

    IFC(m_pD3DDevice->GetRenderTargetData(
        pSourceSurface,
        pDestinationSurface
        ));

Cleanup:
    if (hr == D3DERR_DEVICELOST)
    {
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateAdditionalSwapChain
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateAdditionalSwapChain
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateAdditionalSwapChain(
    __in_ecount_opt(1) CMILDeviceContext const *pMILDC,
    __in_ecount(1) D3DPRESENT_PARAMETERS *pD3DPresentParams,
    __deref_out_ecount(1) CD3DSwapChain ** const ppSwapChain
    )
{
    AssertDeviceEntry(*this);

    // Assert format has been tested, for swap chains.
    AssertRenderFormatIsTestedSuccessfully(pD3DPresentParams->BackBufferFormat);

    HRESULT hr = S_OK;
    IDirect3DSwapChain9 *pD3DSwapChain = NULL;

    *ppSwapChain = NULL;

    //
    // It is possible for caller to try creating a render target even though
    // the device is now invalid.  Check here for such a situation.
    //

    IFC(m_hrDisplayInvalid);

    //
    // Workaround : HW RT artifacts on large windows
    //
    // Some drivers can't handle creating swap chains larger than their texture
    // limit.  Until these are fixed we will mock OOVM so that we can fallback
    // to SW.
    //

    if (pD3DPresentParams->BackBufferWidth > m_caps.MaxTextureWidth ||
        pD3DPresentParams->BackBufferHeight > m_caps.MaxTextureHeight)
    {
        // Any error except WGXERR_DISPLAYSTATEINVALID will try fallback to SW.
        IFC(D3DERR_OUTOFVIDEOMEMORY);
    }

    //
    // Delegate to CreateAdditionalSwapChain
    //

    {
        MtSetDefault(Mt(D3DSwapChain));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateAdditionalSwapChain(pD3DPresentParams, &pD3DSwapChain);

        END_DEVICE_ALLOCATION;

        IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM
    }

    //
    // Create swap chain wrapper
    //

    if (   pMILDC
        && (   pMILDC->PresentWithHAL()
            || !IsExtendedDevice()
            || (   pD3DPresentParams->BackBufferFormat != D3DFMT_A8R8G8B8
                && pD3DPresentParams->BackBufferFormat != D3DFMT_X8R8G8B8
               )
           )
       )
    {
        //
        // The MILDC, if passed to CD3DSwapChain::Create, will cause us to
        // implement GetDC ourselves by copying the swap chain surface to a
        // software bitmap. We only want to do this in WDDM since XPDM can
        // hardware accelerate GetDC on its own.
        //
        // Additionally, non-32bpp formats are not currently allowed when a
        // MILDC is supplied.
        //
        pMILDC = NULL;
    }

    IFC(CD3DSwapChain::Create(
        &m_resourceManager,
        pD3DSwapChain,
        pD3DPresentParams->BackBufferCount,
        pMILDC,
        ppSwapChain
        ));

Cleanup:
    // Can't use HandleDIE because if we can't create swap chain
    // present won't be called.

    switch (hr)
    {
    case D3DERR_DRIVERINTERNALERROR:
        m_hrDisplayInvalid = D3DERR_DRIVERINTERNALERROR;
        __fallthrough;

    case D3DERR_DEVICELOST:
        hr = WGXERR_DISPLAYSTATEINVALID;
        MarkUnusable(false /* This call is already entry protected. */);
    }

    ReleaseInterface(pD3DSwapChain);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexBuffer
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateVertexBuffer
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateVertexBuffer(
    UINT Length,
    DWORD Usage,
    DWORD FVF,
    D3DPOOL Pool,
    __deref_out_ecount(1) IDirect3DVertexBuffer9 ** const ppVertexBuffer
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    MtSetDefault(Mt(D3DVertexBuffer));

    //
    // Allocate the D3D vertex buffer
    //

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreateVertexBuffer(
        Length,
        Usage,
        FVF,
        Pool,
        ppVertexBuffer,
        NULL // HANDLE* pSharedHandle
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

HRESULT
CD3DDeviceLevel1::CreateIndexBuffer(
    UINT Length,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    __deref_out_ecount(1) IDirect3DIndexBuffer9 ** const ppIndexBuffer
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    MtSetDefault(Mt(D3DIndexBuffer));

    //
    // Allocate the D3D vertex buffer
    //

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreateIndexBuffer(
        Length,
        Usage,
        Format,
        Pool,
        ppIndexBuffer,
        NULL // HANDLE* pSharedHandle
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ComposeRects
//
//  Synopsis:
//      delegate to IDirect3DDevice9::ComposeRects
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ComposeRects(
        __in_ecount(1) IDirect3DSurface9* pSource,
        __inout_ecount(1) IDirect3DSurface9* pDestination,
        __in_ecount(1) IDirect3DVertexBuffer9* pSrcRectDescriptors,
        UINT NumRects,
        __in_ecount(1) IDirect3DVertexBuffer9* pDstRectDescriptors,
        D3DCOMPOSERECTSOP Operation
        )
{
    AssertDeviceEntry(*this);
    Assert(m_pD3DDeviceEx);

    HRESULT hr = S_OK;

    //
    // Compose overscaled glyph run bitmap
    //

    MIL_THR(m_pD3DDeviceEx->ComposeRects(
        pSource,
        pDestination,
        pSrcRectDescriptors,
        NumRects,
        pDstRectDescriptors,
        Operation,
        0,  //OffsetX
        0   //OffsetY
        ));

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateTexture
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateTexture
//
//  Notes:
//      Shared handle support is a D3D9.L only feature.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateTexture(
    __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
    UINT uLevels,
    __deref_out_ecount(1) IDirect3DTexture9 ** const ppD3DTexture,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    //
    // If we've already processed a mode change for this device but haven't recreated
    // it for this window, we should notify the caller.  If we allow the device to create
    // a texture that new texture will be valid and the rendering stack could
    // attempt to draw into it even though the rest of the device's resources have
    // already been released.
    //
    if (m_fDeviceLostProcessed == true)
    {
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
        RRETURN(hr);
    }

    //
    // Allocate the D3D texture
    //

    {
        MtSetDefault(Mt(D3DTexture));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateTexture(
            pSurfDesc->Width,
            pSurfDesc->Height,
            uLevels,
            pSurfDesc->Usage,
            pSurfDesc->Format,
            pSurfDesc->Pool,
            ppD3DTexture,
            pSharedHandle
            );

        END_DEVICE_ALLOCATION;

        DbgInjectDIE(&hr);

        MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateStateBlock
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateStateBlock
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateStateBlock(
    D3DSTATEBLOCKTYPE d3dStateBlockType,
    __deref_out_ecount(1) IDirect3DStateBlock9 ** const ppStateBlock
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    {
        MtSetDefault(Mt(D3DStateBlock));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateStateBlock(
            d3dStateBlockType,
            ppStateBlock
            );

        END_DEVICE_ALLOCATION;

        MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateLockableTexture
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateTexture
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateLockableTexture(
    __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
    __deref_out_ecount(1) CD3DLockableTexture ** const ppLockableTexture
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    IDirect3DTexture9 *pD3DTexture = NULL;
    UINT uLevels = 1;

    Assert((pSurfDesc->Pool == m_ManagedPool) ||
           (pSurfDesc->Pool == D3DPOOL_SYSTEMMEM));

    *ppLockableTexture = NULL;

    if (pSurfDesc->Usage & D3DUSAGE_AUTOGENMIPMAP)
    {
        uLevels = 0;
    }

    //
    // Allocate the D3D texture
    //

    {
        MtSetDefault(Mt(D3DTexture));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateTexture(
            pSurfDesc->Width,
            pSurfDesc->Height,
            uLevels,
            pSurfDesc->Usage,
            pSurfDesc->Format,
            pSurfDesc->Pool,
            &pD3DTexture,
            NULL // HANDLE* pSharedHandle
            );

        END_DEVICE_ALLOCATION;

        IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM
    }

    //
    // Create the texture wrapper
    //

    IFC(CD3DLockableTexture::Create(
        &m_resourceManager,
        pD3DTexture,
        ppLockableTexture
        ));

Cleanup:
    ReleaseInterface(pD3DTexture);
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateSysMemUpdateSurface
//
//  Synopsis:
//         delegate to IDirect3DDevice9::CreateSurface
//
//      *** WARNING *** WARNING *** WARNING *** WARNING ***
//
//         CreateSysMemUpdateSurface only allows a non-NULL pvPixels on
//         Longhorn. Passing a non-NULL pSharedHandle to
//         CreateOffscreenPlainSurface will return E_NOTIMPL on XP and Server
//         2003.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateSysMemUpdateSurface(
    UINT uWidth,
    UINT uHeight,
    D3DFORMAT fmtTexture,
    __in_xcount_opt(uWidth * uHeight * D3DFormatSize(fmtTexture)) void *pvPixels,
    __deref_out_ecount(1) IDirect3DSurface9 ** const ppD3DSysMemSurface
    )
{
    HRESULT hr = S_OK;
    IDirect3DTexture9 *pD3DTexture = NULL;

    *ppD3DSysMemSurface = NULL;

    if (pvPixels)
    {
        // pvPixel may be non-NULL only if we have LDDM
        Assert(IsLDDMDevice());
    }

    if (IsLDDMDevice())
    {
        //
        // Allocate the D3D surface. Passing the pixels this way creates the
        // surface by referencing these pixels.
        //

        {
            MtSetDefault(Mt(D3DTexture));

            BEGIN_DEVICE_ALLOCATION;

            hr = m_pD3DDevice->CreateOffscreenPlainSurface(
                uWidth,
                uHeight,
                fmtTexture,
                D3DPOOL_SYSTEMMEM,
                ppD3DSysMemSurface,
                pvPixels ? reinterpret_cast<HANDLE*>(&pvPixels) : NULL
                );

            END_DEVICE_ALLOCATION;

            MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

            IFC(HandleDIE(hr));
        }
    }
    else
    {
        //
        // In XPDM, offscreen plain surfaces do not
        // work correctly. D3D9: SystemMemory Resource Lock
        // doesn't synchronize with command stream.
        //
        // The code in this "else" block is a workaround. The Locking mechanism
        // does work on textures.
        //

        //
        // There is one caveat to using a texture instead of an offscreen
        // surface. Textures must respect max texture size, while offscreen
        // surfaces do not. Fortunately our code does not attempt to do such a
        // thing, so we are okay Asserting here.
        //
        Assert(uWidth <= GetMaxTextureWidth());
        Assert(uHeight <= GetMaxTextureHeight());

        D3DSURFACE_DESC d3dsdSysMemTexture;

        d3dsdSysMemTexture.Format = fmtTexture;
        d3dsdSysMemTexture.Type = D3DRTYPE_TEXTURE;
        d3dsdSysMemTexture.Usage = 0;
        d3dsdSysMemTexture.Pool = D3DPOOL_SYSTEMMEM;
        d3dsdSysMemTexture.MultiSampleType = D3DMULTISAMPLE_NONE;
        d3dsdSysMemTexture.MultiSampleQuality = 0;
        d3dsdSysMemTexture.Width = uWidth;
        d3dsdSysMemTexture.Height = uHeight;

        IFC(CreateTexture(
            &d3dsdSysMemTexture,
            1,
            &pD3DTexture
            ));

        IFC(pD3DTexture->GetSurfaceLevel(
            0,
            ppD3DSysMemSurface
            ));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pD3DTexture);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateSysMemReferenceTexture
//
//  Synopsis:
//         delegate to IDirect3DDevice9::CreateTexture
//
//      *** WARNING *** WARNING *** WARNING *** WARNING ***
//
//         CreateSysMemReferenceTexture only works on Longhorn. Passing a
//         non-NULL pSharedHandle to CreateTexture will return E_NOTIMPL on XP
//         and Server 2003.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateSysMemReferenceTexture(
    __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
    __in_xcount(
        pSurfDesc->Width * pSurfDesc->Height
        * D3DFormatSize(pSurfDesc->Format)
        ) void *pvPixels,
    __deref_out_ecount(1) IDirect3DTexture9 ** const ppD3DSysMemTexture
    )
{
    HRESULT hr = S_OK;

    // Mip mapping for this special kind of texture
    // is not supported by us or D3D.
    UINT uiLevels = 1;

    Assert(pSurfDesc->Pool == D3DPOOL_SYSTEMMEM);

    // this function cannot be called if we are not LDDM
    Assert(IsLDDMDevice());

    *ppD3DSysMemTexture = NULL;

    //
    // Allocate the D3D texture. Passing the pixels
    // this way creates the texture by referencing
    // these pixels.
    //

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreateTexture(
        pSurfDesc->Width,
        pSurfDesc->Height,
        uiLevels,
        pSurfDesc->Usage,
        pSurfDesc->Format,
        pSurfDesc->Pool,
        ppD3DSysMemTexture,
        reinterpret_cast<HANDLE*>(&pvPixels)
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateSurface
//
//  Synopsis:
//      delegate to IDirect3DDevice9::UpdateSurface
//
//  Note:
//      There is no check for this, but the src texture must be in system memory
//      and the dest texture must be in pool default.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::UpdateSurface(
    __in_ecount(1) IDirect3DSurface9 *pD3DSysMemSrcSurface,
    __in_ecount_opt(1) const RECT *pSourceRect,
    __inout_ecount(1) IDirect3DSurface9 *pD3DPoolDefaultDestSurface,
    __in_ecount_opt(1) const POINT *pDestPoint
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*this);

    IFC(m_pD3DDevice->UpdateSurface(
        pD3DSysMemSrcSurface,
        pSourceRect,
        pD3DPoolDefaultDestSurface,
        pDestPoint
        ));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateTexture
//
//  Synopsis:
//      delegate to IDirect3DDevice9::UpdateTexture
//      Note: There is no check for this, but the src texture
//      must be in system memory and the dest texture must be in
//      pool default.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceLevel1::UpdateTexture(
    __in_ecount(1) IDirect3DTexture9 *pD3DSysMemSrcTexture,
    __inout_ecount(1) IDirect3DTexture9 *pD3DPoolDefaultDestTexture
    )
{
    HRESULT hr = S_OK;

    IFC(m_pD3DDevice->UpdateTexture(
        pD3DSysMemSrcTexture,
        pD3DPoolDefaultDestTexture
        ));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::StretchRect
//
//  Synopsis:
//      delegate to IDirect3DDevice9::StretchRect
//      Note: There are restrictions on which types of surfaces
//      may be used with this function. See the D3D docs for
//      specifics.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceLevel1::StretchRect(
    __in_ecount(1) CD3DSurface *pSourceSurface,
    __in_ecount_opt(1) const RECT *pSourceRect,
    __inout_ecount(1) IDirect3DSurface9 *pDestSurface,
    __in_ecount_opt(1) const RECT *pDestRect,
    D3DTEXTUREFILTERTYPE Filter
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*this);

    if (g_pMediaControl)
    {
        if (pSourceRect)
        {
            g_lPixelsFilledPerFrame += (pSourceRect->right - pSourceRect->left) * (pSourceRect->bottom - pSourceRect->top);
        }
        else if (pDestRect)
        {
            g_lPixelsFilledPerFrame += (pDestRect->right - pDestRect->left) * (pDestRect->bottom - pDestRect->top);
        }
    }

    IFC(m_pD3DDevice->StretchRect(
        pSourceSurface->ID3DSurface(),
        pSourceRect,
        pDestSurface,
        pDestRect,
        Filter
        ));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetRenderTarget
//
//  Synopsis:
//      1. Call EndScene
//      2. Set the render target
//      3. Call BeginScene
//      4. Set the view and projection matrices.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetRenderTargetForEffectPipeline(
    __in_ecount(1) CD3DSurface *pD3DSurface
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    //
    // If the render target hasn't changed, don't do anything.
    //

    if (pD3DSurface == m_pCurrentRenderTargetNoRef)
    {
        goto Cleanup;
    }

    //
    // Call EndScene
    //

    if (m_fInScene)
    {
        IFC(EndScene());
    }

    //
    // Set the render target
    //

    m_desc = pD3DSurface->Desc();

    IFC(m_pD3DDevice->SetRenderTarget(0, pD3DSurface->ID3DSurface()));
    m_pCurrentRenderTargetNoRef = pD3DSurface;

    //
    // SetRenderTarget resets the Viewport and ScissorClip
    //  for RT index 0; so remember that.
    //

    //
    // Our clip being set is tracked by our CHwRenderStateManager through the
    // CD3DRenderstate object.
    //
    // We have to let it know to set the clip to false.
    //
    SetClipSet(false);


    //
    // Call BeginScene
    //

    IFC(BeginScene());

Cleanup:

    //
    // If any part of the above fails, there is no valid state.
    // Release the use of the current render target is there is one.
    //

    if (FAILED(hr) && m_pCurrentRenderTargetNoRef)
    {
        ReleaseUseOfRenderTarget(m_pCurrentRenderTargetNoRef);
    }

    RRETURN(HandleDIE(hr));
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetRenderTarget
//
//  Synopsis:
//      1. Call EndScene
//      2. Set the render target
//      3. Call BeginScene
//      4. Set the view and projection matrices.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetRenderTarget(
    __in_ecount(1) CD3DSurface *pD3DSurface
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    CMILMatrix matView;
    CMILMatrix matProj;

    //
    // If the render target hasn't changed, don't do anything.
    //

    if (pD3DSurface == m_pCurrentRenderTargetNoRef)
    {
        goto Cleanup;
    }

    //
    // Call EndScene
    //

    if (m_fInScene)
    {
        IFC(EndScene());
    }

    //
    // Set the render target
    //

    m_desc = pD3DSurface->Desc();

    // There was a Watson report where D3D returned a failure code
    //                 indicating the D3D9 surface ptr was NULL.  From inspection
    //                 this seems impossible - we want this to break in retail at
    //                 the point of failure to aid future investigation of the issue.
    FreAssert(pD3DSurface->ID3DSurface() != NULL);

    IFC(m_pD3DDevice->SetRenderTarget(0, pD3DSurface->ID3DSurface()));
    m_pCurrentRenderTargetNoRef = pD3DSurface;

    //
    // SetRenderTarget resets the Viewport and ScissorClip
    //  for RT index 0; so remember that.
    //

    //
    // Our clip being set is tracked by our CHwRenderStateManager through the
    // CD3DRenderstate object.
    //
    // We have to let it know to set the clip to false.
    //
    SetClipSet(false);

    {
        MilPointAndSizeL rcViewport;

        rcViewport.X = 0;
        rcViewport.Y = 0;
        rcViewport.Width = m_desc.Width;
        rcViewport.Height = m_desc.Height;

        //
        // We must call ScissorRectChanged because IDirect3DDevice9::SetRenderTarget
        // resets the scissor rect to the viewport.
        //
        if (SupportsScissorRect())
        {
            ScissorRectChanged(&rcViewport);
        }

        //
        // Set the viewport since it has inherently changed.
        //
        IFC(SetViewport(&rcViewport));

        //
        // Set the surface to clipping matrix
        //

        IFC(SetSurfaceToClippingMatrix(
            &rcViewport
            ));
    }

    //
    // Call BeginScene
    //

    IFC(BeginScene());

Cleanup:

    //
    // If any part of the above fails, there is no valid state.
    // Release the use of the current render target is there is one.
    //

    if (FAILED(hr) && m_pCurrentRenderTargetNoRef)
    {
        ReleaseUseOfRenderTarget(m_pCurrentRenderTargetNoRef);
    }

    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ReleaseUseOfRenderTarget
//
//  Synopsis:
//       Releases any cached use the device may have of the given
//      render target, thereby enabling the D3D surface to be cleaned up
//      when it is truely no longer in use.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ReleaseUseOfRenderTarget(
    __in_ecount(1) const CD3DSurface *pD3DSurface     // [IN] a render target that
                                                      // will no longer be valid
    )
{
    AssertDeviceEntry(*this);

    if (pD3DSurface == m_pCurrentRenderTargetNoRef)
    {
        //
        // The pd3dSurface we need to release is currently
        //  set as the D3D render target.  In order to
        //  completely release it we must call SetRenderTarget
        //  with a different RT.  NULL is not acceptable.
        //

        m_pCurrentRenderTargetNoRef = NULL;

        AssertMsg(m_fInScene,
                  "m_fInScene expected to be true.\r\n"
                  "This may be ignored only if caller is SetRenderTarget."
                  );
        if (m_fInScene)
        {
            IGNORE_HR(EndScene());
        }

        IGNORE_HR(m_pD3DDevice->SetRenderTarget(0, m_pD3DDummyBackBuffer));

        ReleaseUseOfDepthStencilSurface(m_pDepthStencilBufferForCurrentRTNoRef);

        // Note: We've set the RT to a dummy so there is no point in
        //       beginning a scene now.  The scene will begin once
        //       another RT has been set.
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Clear
//
//  Synopsis:
//      delegate to IDirect3DDevice9::Clear
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Clear(
    DWORD dwCount,
    __in_ecount_opt(dwCount) const D3DRECT* pRects,
    DWORD dwFlags,
    D3DCOLOR d3dColor,
    FLOAT rZValue,
    int iStencilValue
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    Assert((dwCount > 0) == (pRects != NULL));
    Assert(rZValue >= 0.0f && rZValue <= 1.0f);
    Assert(iStencilValue >= 0);

    //
    // There is a bug in checked D3D that will cause them to fail if we clear
    // the target and the depth stencil surface is not at least as big as the
    // rendertarget, even if we're not clearing the depth or stencil.
    //
    // Some drivers don't deal with this well either, as they've never, been
    // expected to.
    //

    if (IsDepthStencilSurfaceSmallerThan(m_desc.Width, m_desc.Height))
    {
        //
        // Too small; so unset the depth-stencil buffer to work around bugs.
        //

        // We shouldn't be clearing z or stencil, with an inappropriately
        // sized buffer.
        Assert((dwFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) ) == 0);

        IFC(SetDepthStencilSurfaceInternal(NULL));
    }

    //
    // Do the clear
    //

    IFC(m_pD3DDevice->Clear(
        dwCount,
        pRects,
        dwFlags,
        d3dColor,
        rZValue,
        iStencilValue
        ));

#if DBG
    //
    // Draw zoom mode grid (if it is enabled)
    //

    if (DbgIsPixelZoomMode())
    {
        D3DRECT d3dRect;
        MilColorB colorGrid = MIL_COLOR(255, 190, 190, 190);

        if (dwFlags == D3DCLEAR_TARGET
            && pRects == NULL
            && m_pCurrentRenderTargetNoRef != NULL)
        {
            for (unsigned x = 0; x < m_desc.Width; x += c_dbgPixelZoomModeScale)
            {
                for (unsigned y = 0; y < m_desc.Height; y += c_dbgPixelZoomModeScale)
                {
                    if (((x + y) % (2*c_dbgPixelZoomModeScale)) == 0)
                    {
                        d3dRect.x1 = x;
                        d3dRect.y1 = y;
                        d3dRect.x2 = x + c_dbgPixelZoomModeScale;
                        d3dRect.y2 = y + c_dbgPixelZoomModeScale;

                        IGNORE_HR(m_pD3DDevice->Clear(1, &d3dRect, D3DCLEAR_TARGET, colorGrid, rZValue, 0));
                    }
                }
            }
        }
        else
        {
            //  at some point, we can handle rect clears
        }
    }
#endif

Cleanup:
    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ColorFill
//
//  Synopsis:
//      delegate to ColorFill
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ColorFill(
    __inout_ecount(1) IDirect3DSurface9 *pSurface,
    __in_ecount_opt(1) const RECT *pRect,
    D3DCOLOR color
    )
{
    {
        MtSetDefault(Mt(D3DColorFill));

        return m_pD3DDevice->ColorFill(
            pSurface,
            pRect,
            color
            );
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CleanupFreedResources
//
//  Synopsis:
//      Free, without delay, unused resources hanging off the device.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::CleanupFreedResources()
{
    m_resourceManager.DestroyReleasedResourcesFromLastFrame();
    m_resourceManager.DestroyResources(CD3DResourceManager::WithoutDelay);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      FillCurrentCumulativeMax
//
//  Synopsis:
//      Take points to three DWORDs for Current, Cumulative, Max, and a new
//      value.  Use atomic operations to fill in the first three based on the
//      new value
//
//      Allow the cumulative ones to overflow and wraparound. Expectation is
//      that they will be reset by a monitoring tool before then, and it's ok if
//      they're not.
//
//------------------------------------------------------------------------------

static void
FillCurrentCumulativeMax(
    __out_ecount(1) DWORD *pCurrent,
    __inout_ecount(1) DWORD *pCumulative,
    __inout_ecount(1) DWORD *pMax,
    DWORD newValue)
{
    InterlockedExchange(reinterpret_cast<LONG *>(pCurrent), newValue);
    InterlockedExchangeAdd(reinterpret_cast<LONG *>(pCumulative), newValue);
    DWORD maxValue = *pMax > newValue ? *pMax : newValue;
    InterlockedExchange(reinterpret_cast<LONG *>(pMax), maxValue);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Present
//
//  Synopsis:
//      1. Call EndScene
//      2. Delegate to CD3DSwapChain::Present
//      3. Call BeginScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Present(
    __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
    __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
    __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
    DWORD dwD3DPresentFlags
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr;

    if (FAILED(m_hrDisplayInvalid))
    {
        // Call MarkUnusable to check if we still need to handle loss now that
        // we have the proper protection.
        MarkUnusable(false /* This call is already entry protected. */);

        // MarkUnusable may have changed the HR but it's still a failure.
        Assert(FAILED(m_hrDisplayInvalid));

        MIL_THR(m_hrDisplayInvalid);
        goto Cleanup;
    }

    Assert(pD3DSwapChain->m_pD3DSwapChain);
    Assert(pD3DSwapChain->IsValid());

#if DBG==1
    if (IsTagEnabled(tagPixelZoomMode))
    {
        //
        // Get the HWND so that we can do screen to client coordinate transforms.  This code means
        // that you actually need to wait for a frame before using the mouse and doesn't work with
        // multiple windows.
        //
        // However, since this is a tracetag only debugging feature, this code is sufficient.
        //
        g_dbgHWND = pMILDC->GetHWND();
    }
#endif

    //if (m_bNeedGarbageCollection)  --- always for now
    {
        m_glyphBank.CollectGarbage();
        //m_bNeedGarbageCollection = false;
    }

    //
    // Update metrics
    //

    if (g_pMediaControl)
    {
        CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();

        // Update metrics, including cumulative and maxmetrics.


        if (m_dwMetricsTrianglesPerFrame > 0)
        {
            //
            // Only update if we've drawn something
            //

            FillCurrentCumulativeMax(&pFile->TrianglesPerFrame,
                                     &pFile->TrianglesPerFrameCumulative,
                                     &pFile->TrianglesPerFrameMax,
                                     m_dwMetricsTrianglesPerFrame);
        }

        // Texture Updates
        FillCurrentCumulativeMax(&pFile->TextureUpdatesPerFrame,
                                 &pFile->TextureUpdatesPerFrameCumulative,
                                 &pFile->TextureUpdatesPerFrameMax,
                                 g_dwTextureUpdatesPerFrame);


        // Pixels
        FillCurrentCumulativeMax(&pFile->PixelsFilledPerFrame,
                                 &pFile->PixelsFilledPerFrameCumulative,
                                 &pFile->PixelsFilledPerFrameMax,
                                 g_lPixelsFilledPerFrame);

        InterlockedExchange(reinterpret_cast<volatile LONG *>(&g_dwTextureUpdatesPerFrame), 0);
        InterlockedExchange(static_cast<LONG *>(&g_lPixelsFilledPerFrame), 0);

        MtSet(Mt(VerticesPerFrame), m_dwMetricsVerticesPerFrame, 0);
        m_dwMetricsVerticesPerFrame = 0;

        MtSet(Mt(TrianglesPerFrame), m_dwMetricsTrianglesPerFrame, 0);
        m_dwMetricsTrianglesPerFrame = 0;
    }

    //
    // Call EndScene
    //

    bool fRestoreScene = m_fInScene;
    bool fPresentProcessed = false;

    if (m_fInScene)
    {
        IFC(EndScene());
    }

#if DBG
    if (IsTagEnabled(tagD3DStats))
    {
        // Query stats
        m_d3dStats.OnPresent(m_pD3DDevice);
    }
#endif
    IF_D3DLOG(m_log.OnPresent();)

    if (pMILDC->PresentWithHAL())
    {
        IFC(PresentWithD3D(
            pD3DSwapChain->m_pD3DSwapChain,
            prcSource,
            prcDest,
            pMILDC,
            pDirtyRegion,
            dwD3DPresentFlags,
            &fPresentProcessed
            ));
    }
    else
    {
        IFC(PresentWithGDI(
            pD3DSwapChain,
            prcSource,
            prcDest,
            pMILDC,
            pDirtyRegion,
            &fPresentProcessed
            ));
    }

    if (fRestoreScene)
    {
        MIL_THR_SECONDARY(BeginScene());
    }

    if (fPresentProcessed)
    {
        if (!IsLDDMDevice())
        {
            if (!CCommonRegistryData::GPUThrottlingDisabled())
            {
                ULONGLONG ullPresentTime;

                m_uNumSuccessfulPresentsSinceMarkerFlush++;

                IFCW32(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&ullPresentTime)));

                IFC(InsertGPUMarker(ullPresentTime));
            }
        }
    }

Cleanup:

    RRETURN1(hr, S_PRESENT_OCCLUDED); // DIE already handled
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::PresentWithD3D
//
//  Synopsis:
//      Use D3D to present from the swap chain.
//

HRESULT
CD3DDeviceLevel1::PresentWithD3D(
    __inout_ecount(1) IDirect3DSwapChain9 *pD3DSwapChain,
    __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
    __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
    DWORD dwD3DPresentFlags,
    __out_ecount(1) bool *pfPresentProcessed
    )
{
    HRESULT hr = S_OK;

    Assert( pMILDC->PresentWithHAL() );

    *pfPresentProcessed = false;

    //
    // Call present and check for mode change
    //

    BEGIN_DEVICE_ALLOCATION;

    hr = pD3DSwapChain->Present(
        prcSource,
        prcDest,
        pMILDC->GetHWND(),
        (prcSource == NULL) ? NULL : pDirtyRegion,
        dwD3DPresentFlags
        );

    END_DEVICE_ALLOCATION;


    DbgInjectDIE(&hr);

    if (hr == S_OK)
    {
        *pfPresentProcessed = true;
    }
    else if (hr == S_PRESENT_MODE_CHANGED)
    {
        //
        // Desktop Display Mode has  changed.  (LH Only, pre-LH systems will
        // return D3DERR_DEVICELOST instead.
        //
        // Currently we want to handle this identically to D3DERR_DEVICELOST
        //
        // Future Consideration:  May want to optimize mode change
        //
        // We could optimize this scenario so we don't recreate the device
        // and instead check the new display parameters.
        //

        hr = D3DERR_DEVICELOST;
    }
    else if (hr == S_PRESENT_OCCLUDED)
    {
        // Device is in a normal state but isn't visible.  This is LH Only
        // and can be because of:
        //
        // 1.  Presentation window is minimized.
        //
        // 2.  Another Device entered fullscreen mode on the same monitor,
        //     and this window is completely on that monitor.
        //
        // 3.  The monitor on which the window is displayed is asleep.
        //     (This final case we handle in the UI thread, by listening for
        //      power events and appropriately invalidating the window when
        //      the monitor comes back on). This is facilitated by sending
        //      a custom window message that the UI thread is looking for.

        //
        // In the windowed case we can't keep checking our device state until
        // we're valid again before we render, since all rendering will
        // stop.  If a window is straddling 2 monitors and one side gets
        // occluded the other won't render.  So if we're windowed, we keep
        // rendering as if nothing has happened.
        //

        // To avoid overloading the CPU with repeated failures, we sleep briefly here. If we are failing to present,
        // this is not a problem because the UI will be unresponsive anyway.
        Sleep(100);
        PostMessage(pMILDC->GetHWND(), m_presentFailureWindowMessage, NULL, NULL);

        hr = S_OK;
    }

    //
    // !!! Critical Note: After this point hr may not be S_OK.  Make sure not
    //                    to change to S_OK when making other calls.
    //

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    if (FAILED(hr))
    {
        hr = HandlePresentFailure(pMILDC, hr);
    }

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::HandlePresentFailure
//
//  Synopsis:
//      This method handles hresult failures for the present methods, e.g. PresentWithGDI or
//      PresentWithD3D.
//
//      To avoid unnecessary overhead this function should only be called if FAILED(hr) is true,
//      i.e. only with a valid HRESULT failure and not a success code.
//------------------------------------------------------------------------------

HRESULT
CD3DDeviceLevel1::HandlePresentFailure(
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    HRESULT hr
    )
{
    Assert(FAILED(hr));

    //
    //  Release what resources we can.
    //  Do not use ReleaseUseOfRenderTarget since it calls EndScene.
    //  There should normally be a valid RT at this point, but
    //  there are some rare corner cases that we should protect
    //  against.
    //

    if (m_pCurrentRenderTargetNoRef)
    {
        m_pCurrentRenderTargetNoRef = NULL;

        IGNORE_HR(m_pD3DDevice->SetRenderTarget(0, m_pD3DDummyBackBuffer));

        ReleaseUseOfDepthStencilSurface(m_pDepthStencilBufferForCurrentRTNoRef);
    }


    if (!IsWindow(pMILDC->GetHWND()))
    {
        //
        // There can be a variety of failure codes returned when a window is
        // destroyed while we are trying to draw to it with GDI.  To simplify
        // the callers error handling check for an invalid window and return
        // a failure code indicating such.  Otherwise just return whatever
        // we could discern so far.
        //

        MIL_THR(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
    }
    else if (hr == E_FAIL || hr == D3DERR_DRIVERINTERNALERROR)
    {
        //
        // The code might end up in here for various reasons:
        //
        //    CD3DSurface::GetDC can fail with D3DERR_DRIVERINTERNALERROR, if
        //    this happens we treat this as a D3DERR_DEVICELOST which is handled below.
        //
        //    IDirect3DSwapChain9::Present may return E_FAIL (though it isn't supposed to),
        //    so treat like D3DERR_DRIVERINTERNALERROR & convert to D3DERR_DEVICELOST
        //    which is handled below.
        //
        hr = D3DERR_DEVICELOST;
        IGNORE_HR(CD3DRegistryDatabase::HandleAdapterUnexpectedError(m_caps.AdapterOrdinal));
    }
    else if (hr == E_INVALIDARG && IsLDDMDevice())
    {
        //
        //  DWM DX redirection resize
        //  synchronization can return E_INVALIDARG or E_FAIL (handled above).
        //
        RIPW(L"LDDM Present returned E_INVALIDARG");
        MIL_THR(WGXERR_NEED_RECREATE_AND_PRESENT);
    }



    if (hr == D3DERR_DEVICELOST ||
        hr == D3DERR_DEVICEHUNG ||  // Hw Adapter timed out and has
                                    // been reset by the OS (LH Only)
                                    //
        hr == D3DERR_DEVICEREMOVED  // Hw Adapter has been removed (LH
                                    // Only)
                                    //
        )
    {
        hr = WGXERR_DISPLAYSTATEINVALID;
        MarkUnusable(false /* This call is already entry protected. */);
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::PresentWithGDI
//
//  Synopsis:
//      Presents the backbuffer using a gdi bit blt. Currently this should only
//      be used with rendertargets that have specified a right to left layout.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::PresentWithGDI(
    __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
    __in_ecount_opt(1) CMILSurfaceRect const * prcSource,
    __in_ecount_opt(1) CMILSurfaceRect const * prcDest,
    __in_ecount(1) CMILDeviceContext const * pMILDC,
    __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
    __out_ecount(1) bool *pfPresentProcessed
    )
{
    HRESULT hr = S_OK;
    HDC hdcFrontBuffer = NULL;
    HDC hdcBackBuffer = NULL;

    CD3DSurface *pBackBufferSurface = NULL;

    CMilRectU rcSource;
    RECT rcDest;

    UINT uBufferWidth;
    UINT uBufferHeight;

    Assert( !pMILDC->PresentWithHAL() );

    //
    // We don't handle the case where the swap chain has more than 1 entry.
    //
#if DBG
    Assert(pD3DSwapChain->DbgGetNumBackBuffers() == 1);
#endif

    *pfPresentProcessed = false;

    IFC(pD3DSwapChain->GetBackBuffer(
        0,
        &pBackBufferSurface
        ));

    pBackBufferSurface->GetSurfaceSize(
        &uBufferWidth,
        &uBufferHeight
        );

    //
    // If a source and destination rect weren't specified, set them to be the
    // full size of the buffer.
    //
    // The source and dest pointers are linked, they should either both be
    // NULL, or both be non-NULL.
    //
    if (prcSource)
    {
        Assert(prcDest);

        Assert(prcSource->Width() == prcDest->Width());
        Assert(prcSource->Height() == prcDest->Height());

        rcDest = *prcDest;
        rcSource.left = prcSource->left;
        rcSource.top = prcSource->top;
        rcSource.right = prcSource->right;
        rcSource.bottom = prcSource->bottom;
    }
    else
    {
        Assert(!prcDest);

        rcSource.left = 0;
        rcSource.top = 0;
        rcSource.right = uBufferWidth;
        rcSource.bottom = uBufferWidth;

        rcDest.left   = 0;
        rcDest.top    = 0;
        rcDest.right  = uBufferWidth;
        rcDest.bottom = uBufferHeight;
    }

    Assert(rcDest.right > rcDest.left);
    Assert(rcDest.bottom > rcDest.top);

    IFC(pD3DSwapChain->GetDC(
        0,
        rcSource,
        OUT &hdcBackBuffer
        ));

    *pfPresentProcessed = true;


    if ((pMILDC->GetRTInitializationFlags() & MilRTInitialization::PresentUsingMask) == MilRTInitialization::PresentUsingUpdateLayeredWindow)
    {
        SIZE sz = { uBufferWidth, uBufferHeight };
        POINT ptSrc = { 0, 0 };
        HWND hWnd = pMILDC->GetHWND();

        hr = UpdateLayeredWindowEx(
            hWnd,
            NULL, // front buffer
            &pMILDC->GetPosition(),
            &sz,
            hdcBackBuffer,
            &ptSrc,
            pMILDC->GetColorKey(), // colorkey
            &pMILDC->GetBlendFunction(), // blendfunction
            pMILDC->GetULWFlags(), // flags
            prcSource
            );
        // If we get this error, then UpdateLayeredWindow
        // probably failed because the size in sz didn't exactly match the
        // window size.  Ignore this error (rather than crash).
        if (hr == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE))
        {
            hr = S_OK;
        }
        IFC(hr);
    }
    else if ((pMILDC->GetRTInitializationFlags() & MilRTInitialization::PresentUsingMask) == MilRTInitialization::PresentUsingBitBlt)
    {
        IFC(pMILDC->BeginRendering(
            &hdcFrontBuffer
            ));

        IFCW32_CHECKSAD(BitBlt(
            hdcFrontBuffer,
            rcDest.left,
            rcDest.top,
            rcDest.right - rcDest.left,
            rcDest.bottom - rcDest.top,
            hdcBackBuffer,
            rcSource.left,
            rcSource.top,
            SRCCOPY
            ));
    }
    else
    {
        // No support for AlphaBlend yet.
        IFC(E_NOTIMPL);
    }

Cleanup:

    if (FAILED(hr))
    {
        hr = HandlePresentFailure(pMILDC, hr);
    }

    if (hdcBackBuffer)
    {
        //
        // Need to release the DC we're holding onto
        //
        MIL_THR_SECONDARY(pD3DSwapChain->ReleaseDC(
            0,
            hdcBackBuffer
            ));
    }

    ReleaseInterfaceNoNULL(pBackBufferSurface);

    if (hdcFrontBuffer != NULL)
    {
        pMILDC->EndRendering(hdcFrontBuffer);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetTexture
//
//  Synopsis:
//      Sets the texture for a particular stage
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetTexture(
    DWORD dwTextureStage,
    __inout_ecount_opt(1) CD3DTexture *pD3DTexture
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;


    IDirect3DBaseTexture9 *pd3dBaseTextureNoRef = NULL;

    // 2/14/2003 chrisra - Changed the function so if a NULL texture
    // was passed in the stage would be set to NULL

    if (pD3DTexture != NULL)
    {
        Assert(pD3DTexture->IsValid());

        Use(*pD3DTexture);

        //
        // Get IDirect3DBaseTexture
        //

        pd3dBaseTextureNoRef = pD3DTexture->GetD3DTextureNoRef();

        //
        // Set base texture at specified stage
        //
    }

    IFC(CD3DRenderState::SetTexture(dwTextureStage, pd3dBaseTextureNoRef));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetD3DTexture
//
//  Synopsis:
//      Sets the texture for a particular stage
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetD3DTexture(
    DWORD dwTextureStage,
    __in_ecount_opt(1) IDirect3DTexture9 *pD3DTexture
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    IFC(CD3DRenderState::SetTexture(dwTextureStage, pD3DTexture));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DisableTextureTransform
//
//  Synopsis:
//      Disables texture transformation for given stage
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DisableTextureTransform(DWORD dwTextureStage)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    IFC(CD3DRenderState::DisableTextureTransform(dwTextureStage));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::BeginScene
//
//  Synopsis:
//      delegate to IDirect3DDevice9::BeginScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::BeginScene()
{
    HRESULT hr = S_OK;

    Assert(!m_fInScene);

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->BeginScene();

    END_DEVICE_ALLOCATION;

    IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM

    m_fInScene = true;

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::EndScene
//
//  Synopsis:
//      delegate to IDirect3DDevice9::EndScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::EndScene()
{
    HRESULT hr = S_OK;

    Assert(m_fInScene);

    IFC(m_pD3DDevice->EndScene());
    m_fInScene = false;

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::FlushBufferFan
//
//  Synopsis:
//      Draws the vertexbuffer assuming the primitive type is a fan. Does not
//      clear out the vertex information that may be reused in multi-pass
//      schemes
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::FlushBufferFan(
    __in_ecount(1) CD3DVertexBuffer *pBuffer
    )
{
    HRESULT hr = S_OK;
    UINT cVertices = pBuffer->GetNumVertices();

    Assert(m_fInScene);

    //
    // It's possible for the tessellator to output 0 triangles.  For example,
    // if we get a zero area rectangle, this will occur.
    //

    //
    // The number of triangles is equal to the number of vertices - 2, but we
    // don't want to do that operation and then check for cTriangles > 0,
    // because the unsigned subtraction operation could cause wrapping,
    // resulting in us attempting to render approximately UINT_MAX.  So we just
    // check for cVertices > 2 and then calculate the number of triangles
    // after.
    //

    if (cVertices > 2)
    {
        UINT cTriangles = cVertices - 2;

        IFC(DrawPrimitiveUP(
            D3DPT_TRIANGLEFAN,
            cTriangles,
            pBuffer->GetVertices(),
            pBuffer->GetVertexStride()
            ));
    }

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Set3DTransforms
//
//  Synopsis:
//      Sends what the current transforms on the card should be to our
//      CD3DRenderState for 3D.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Set3DTransforms(
    __in_ecount(1) CMILMatrix const *pWorldTransform3D,
    __in_ecount(1) CMILMatrix const *pViewTransform3D,
    __in_ecount(1) CMILMatrix const *pProjectionTransform3D,
    __in_ecount(1) CMatrix<CoordinateSpace::Projection3D,CoordinateSpace::Device> const &matHomogeneousTo2DDevice
    )
{
    HRESULT hr = S_OK;
    CMILMatrix mat2DDeviceToHomogeneous;
    CMILMatrix matProjectionModifier;
    CMILMatrix mat3DViewportProjection;

    IFC(SetWorldTransform(
        pWorldTransform3D
        ));

    IFC(SetNonWorldTransform(
        D3DTS_VIEW,
        pViewTransform3D
        ));

    matProjectionModifier = matHomogeneousTo2DDevice * m_matSurfaceToClip;


    // We now have the transform to take us from Homogeneous Clipping Space to
    // the local space of the viewport passed to us.  We apply this to our
    // projection transform, so now our projection transform will take all
    // objects to local space of the viewport.

    mat3DViewportProjection = *pProjectionTransform3D * matProjectionModifier;

    IFC(SetNonWorldTransform(
        D3DTS_PROJECTION,
        &mat3DViewportProjection
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetSurfaceToClippingMatrix
//
//  Synopsis:
//      Calculate surface space to homogeneous clipping (~viewport) matrix for
//      2D and 3D rendering
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetSurfaceToClippingMatrix(
    __in_ecount(1) const MilPointAndSizeL *prcViewport
    )
{
    HRESULT hr = S_OK;

    m_matSurfaceToClip.reset_to_identity();

    //
    // The SurfaceToClip matrix is used to change our coordinate system to
    // the one DX needs...
    //
    // DX one is this (with integer pixel center at edges):
    //
    //             +1.0
    //              ^ +y
    //              |
    //              |
    //              |
    //       <------O------> +x  +1.0
    //              |
    //              |
    //              |
    //              v
    //
    // Ours is this with integers at upper-left of pixel (half-integer pixel
    //  center):
    //
    //     O------------> +x  +Width
    //     |
    //     |
    //     |
    //     |
    //     |
    //     |
    //     v +y
    //   +Height

    //
    // This diagram of the surface and homogeneous in a single dimension (x) may
    // be more helpful.  From this diagram we can directly derive the required
    // surface to homogeneous clipping transform.  The viewport (V) is given in
    // surface coordinate space.  In homogeneous clipping space -1 is exactly
    // 1/2 pixel in (right) from the viewport left (V.L).  +1 is exactly 1/2
    // pixel right of the viewport right (V.R).  The lightable (writeable) area
    // is filled with a \/\/ pattern.
    //
    //           V.L+1/2               V.R+1/2
    //              |                     |         W-1/2  W+1/2
    //        1/2   |<-- V.R-V.L = V.W -->|           |     |
    //     0  |     |                     |           |  W  |
    //     +--+--+--+--+-- ... --+-----+--+--+-----+--+--+  +  +
    //     |     |\/\/\|/\     /\|/\/\/|     |     |     |
    //     |  *  |\/*/\|/\     /\|/\/\/|  *  |     |  *  |  * (Imaginary)
    //     |     |\/\/\|/\     /\|/\/\/|     |     |     |
    //     +-----+--+--+-- ... --+-----+-----+-----+--+--+  +  +
    //              |                     |
    //             -1   <---- +2 ---->   +1
    //
    //
    // Now lets find a scale and translation matrix that maps surface space to
    // clipping space.
    //
    //     SurfaceToClip = Sx * Tx
    //
    // The scale portion of the transform is found by matching up the clipping
    // range with the viewport range:
    //
    //          Sx = 2 / V.W
    //

    FLOAT flRecipViewWidth = 1.0f / static_cast<FLOAT>(prcViewport->Width);

    m_matSurfaceToClip._11 = 2.0f * flRecipViewWidth;

    //
    // Now we can solve a linear equation to find Tx.  In matrix form:
    //
    //          <x'> = <x> * Sx * Tx
    //
    // But since we only have a single dimension and specifically a scale and
    // translate matrix we can express this as:
    //
    //          x' = x * Sx + Tx
    //
    // Solving for Tx
    //
    //          Tx = x' - x * Sx
    //
    // Using the left matching coordinates and already solved Sx we have
    //
    //          Tx = -1 -(V.L+1/2) * 2/V.W                  (substitution)
    //             = -1 -2V.L/V.W - 1/V.W                   (distribution)
    //             = -V.L*2/V.W - 1 - 1/V.W
    //             = -V.L*Sx - 1 - 1/V.W
    //             = - (V.L*Sx + 1 + 1/V.W)
    //

    m_matSurfaceToClip._41 =
        - (  static_cast<FLOAT>(prcViewport->X) * m_matSurfaceToClip._11
           + 1
           + flRecipViewWidth);

    //
    // Computing the Y components is very similar except that in homongenous
    // clipping space +Y is up instead of down as in surface space.  Scaling
    // the Y components by -1 corrects for this.
    //
    //          Sy = -2 / V.H
    //

    FLOAT flRecipViewHeight = 1.0f / static_cast<FLOAT>(prcViewport->Height);

    m_matSurfaceToClip._22 = -2.0f * flRecipViewHeight;

    //
    //          Ty = - (-V.T*2/V.H - 1 - 1/V.H)
    //             = (-V.T)*(-2/V.H) + 1 + 1/V.H
    //             = -V.T*Sy + 1 + 1/V.H
    //

    m_matSurfaceToClip._42 =
        - static_cast<FLOAT>(prcViewport->Y) * m_matSurfaceToClip._22
        + 1
        + flRecipViewHeight;

    //
    // Set the 2D transforms for the state manager.  The world and view
    // matrices are identity.  The m_matSurfaceToClip matrix is concatentation
    // of two matrices that can be thought of as the view and projection
    // matrices.
    //
    // The other important propery about a projection matix is what is set in
    // z-scale, translate, and reciprical w as they can affect z-clipping.
    // Note that these are all the same as they'd be in an identity matrix.
    //

    IFC(Define2DTransforms(
        &m_matSurfaceToClip
        ));

    // 3/18/2003 chrisra - In my 3D checkin I removed the explicit setting of the
    //  transform, assuming that every rendering call would be preceded by a call
    //  to EnsureState, which would call this.  Unfortunately, we have code in
    //  TestLevel1Device which calls rendertexture after this function without
    //  any call to EnsureState.  So there is currently a requirement for this
    //  function to exit with the transforms set in D3D.
    IFC(Set2DTransformForFixedFunction());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreatePixelShaderFromResource
//
//  Synopsis:
//      Read precompiled shader binary data from the resource of this
//      executable, pointed by argument uResourceId. Create pixel shader from
//      these data.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreatePixelShaderFromResource(
    UINT uResourceId,
    __deref_out_ecount(1) IDirect3DPixelShader9 ** const ppPixelShader
    )
{
    HRESULT hr = S_OK;
    HGLOBAL hResource = NULL;
    HRSRC hResourceInfo = NULL;
    const DWORD *pData = NULL;

    // This routine should not be called if the shader exists.
    // Use EnsurePixelShader() instead, if it might happen.
    Assert(*ppPixelShader == NULL);

    IFCW32(hResourceInfo = FindResource(
        g_DllInstance,
        MAKEINTRESOURCE(uResourceId),
        RT_RCDATA
        ));

    IFCW32(hResource = LoadResource(
        g_DllInstance,
        hResourceInfo
        ));

    // This method is nothing more than a cast, so we don't have to worry about error checking here
    pData = reinterpret_cast<const DWORD *>(LockResource(hResource));

    if (pData == NULL)
    {
        AssertMsg(0, "Error-couldn't load shader resource");
        IFC(E_FAIL);
    }

    IFC(CreatePixelShader(pData, ppPixelShader));

Cleanup:
    if (hResource)
    {
        GlobalUnlock(hResource);
    }
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexShaderFromResource
//
//  Synopsis:
//      Reads precompiled shader binary data from the resource of this
//      executable, pointed by argument uResourceId and creates a vertex
//      shader from the binary data.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateVertexShaderFromResource(
    UINT uResourceId,
    __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppVertexShader
    )
{
    HRESULT hr = S_OK;
    HGLOBAL hResource = NULL;
    HRSRC hResourceInfo = NULL;
    const DWORD *pData = NULL;

    Assert(*ppVertexShader == NULL);

    IFCW32(hResourceInfo = FindResource(g_DllInstance, MAKEINTRESOURCE(uResourceId), RT_RCDATA));
    IFCW32(hResource = LoadResource(g_DllInstance, hResourceInfo));

    // This method is nothing more than a cast, so we don't have to worry about error checking here
    pData = reinterpret_cast<const DWORD *>(LockResource(hResource));

    if (pData == NULL)
    {
        AssertMsg(0, "Error-couldn't load shader resource");
        IFC(E_FAIL);
    }

    IFC(CreateVertexShader(pData, ppVertexShader));

Cleanup:
    if (hResource)
    {
        GlobalUnlock(hResource);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CompilePipelineVertexShader
//
//  Synopsis:
//      Compiles a vertex shader from a string.
//

HRESULT
CD3DDeviceLevel1::CompilePipelineVertexShader(
    __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
    UINT cbHLSLSource,
    __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppVertexShader
)
{
    MtSetDefault(Mt(D3DCompiler));

    std::shared_ptr<buffer> pShader;
    std::shared_ptr<buffer> pErr;

    std::string profile_name = 
        shader::get_vertex_shader_profile_name(m_pD3DDevice);

    HRESULT hr =
        shader::compile(
            std::string(pHLSLSource, pHLSLSource + cbHLSLSource),
            "VertexShaderImpl",
            profile_name,
            0, 0,
            pShader, pErr);

    if (SUCCEEDED(hr))
    {
        hr = CreateVertexShader(reinterpret_cast<DWORD*>(pShader->get_buffer_data().buffer), ppVertexShader);
    }

    if (FAILED(hr))
    {
        hr = shader::handle_errors_and_transform_hresult(hr, pErr);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CompilePipelinePixelShader
//
//  Synopsis:
//      Compiles a pixel shader from a string.
//

HRESULT
CD3DDeviceLevel1::CompilePipelinePixelShader(
    __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
    UINT cbHLSLSource,
    __deref_out_ecount(1) IDirect3DPixelShader9 ** const ppPixelShader
    )
{
    MtSetDefault(Mt(D3DCompiler));

    std::shared_ptr<buffer> pShader;
    std::shared_ptr<buffer> pErr;

    std::string profile_name 
        = shader::get_pixel_shader_profile_name(m_pD3DDevice);

    HRESULT hr =
        shader::compile(
            std::string(pHLSLSource, pHLSLSource + cbHLSLSource),
            "PixelShaderImpl",
            profile_name,
            0, 0,
            pShader, pErr);

    if (SUCCEEDED(hr))
    {
        hr = CreatePixelShader(reinterpret_cast<DWORD*>(pShader->get_buffer_data().buffer), ppPixelShader);
    }

    if (!SUCCEEDED(hr))
    {
        hr = shader::handle_errors_and_transform_hresult(hr, pErr);
    }

    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexShader
//
//  Synopsis:
//      Creates a vertex shader.
//

HRESULT
CD3DDeviceLevel1::CreateVertexShader(
    __in const DWORD *pdwfnVertexShader,
    __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppOutShader
    )
{
    HRESULT hr = S_OK;

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreateVertexShader(
        pdwfnVertexShader,
        ppOutShader
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreatePixelShader
//
//  Synopsis:
//      Creates a pixel shader.
//

HRESULT
CD3DDeviceLevel1::CreatePixelShader(
    __in const DWORD *pdwfnPixelShader,
    __deref_out_ecount(1) IDirect3DPixelShader9 ** const ppOutShader
    )
{
    HRESULT hr = S_OK;

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreatePixelShader(
        pdwfnPixelShader,
        ppOutShader
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::RenderTexture
//
//  Synopsis:
//      Renders the upper-left portion of the given texture 1:1 on the current
//      render target
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::RenderTexture(
    __in_ecount(1) CD3DTexture *pD3DTexture,
    __in_ecount(1) const MilPointAndSizeL &rcDestination,
    TextureBlendMode blendMode
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    CD3DVertexXYZDUV2 *pVertex;
    FLOAT rLeft, rTop, rRight, rBottom;
    FLOAT ruRight, rvBottom;

    Assert(rcDestination.X >= 0);
    Assert(rcDestination.Y >= 0);
    Assert(rcDestination.Width > 0);
    Assert(rcDestination.Height > 0);
    Assert(static_cast<UINT>(rcDestination.X) < m_desc.Width);
    Assert(static_cast<UINT>(rcDestination.Y) < m_desc.Height);
    Assert(static_cast<UINT>(rcDestination.X) +
            static_cast<UINT>(rcDestination.Width) <= m_desc.Width);
    Assert(static_cast<UINT>(rcDestination.Y) +
            static_cast<UINT>(rcDestination.Height) <= m_desc.Height);

    //
    // Get source information
    //

    UINT uTexWidth, uTexHeight;
    pD3DTexture->GetTextureSize(&uTexWidth, &uTexHeight);

    Assert(uTexWidth > 0);
    Assert(uTexHeight > 0);

    Assert(static_cast<UINT>(rcDestination.Width) <= uTexWidth);
    Assert(static_cast<UINT>(rcDestination.Height) <= uTexHeight);

    //
    // Compute coordinates at corners
    //

    rLeft   = static_cast<FLOAT>(rcDestination.X);
    rTop    = static_cast<FLOAT>(rcDestination.Y);
    rRight  = rLeft + static_cast<FLOAT>(rcDestination.Width);
    rBottom = rTop + static_cast<FLOAT>(rcDestination.Height);

    ruRight = static_cast<FLOAT>(rcDestination.Width) / static_cast<FLOAT>(uTexWidth);
    rvBottom = static_cast<FLOAT>(rcDestination.Height) / static_cast<FLOAT>(uTexHeight);

    //
    // Set device state
    //

    Assert(m_fInScene);

    IFC(SetTexture(0, pD3DTexture));

    IFC(SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1));
    IFC(SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR1));

    // No need to set sampler state since we won't sample beyond texture bounds
    IFC(SetRenderState_Texture(
        blendMode,
        TBA_Diffuse,
        MilBitmapInterpolationMode::NearestNeighbor,
        0
        ));

    CD3DVertexBufferDUV2 *pBuffer;

    IFC(StartPrimitive(&pBuffer));

    IFC(pBuffer->GetNewVertices(
        4,            // Number of new vertices
        &pVertex      // Pointer to vertices requested
        ));

    //
    // Generate vertices and triangle fan
    //
    //    0-------3
    //    |\      ^
    //    |  \    |
    //    |    \  |
    //    |      \|
    //    1 ----> 2
    //
    // Future Consideration:   Move to use 'normal' vertex buffers
    //  This only uses fan at the moment because that is what is exposed for
    //  this pattern.  If text rendering moves to use Hw Pipeline then this
    //  should move too.
    //

    pVertex[0].SetXYDUV0(rLeft, rTop, 0xffffffff, 0.0f, 0.0f);
    pVertex[1].SetXYDUV0(rLeft, rBottom, 0xffffffff, 0.0f, rvBottom);
    pVertex[2].SetXYDUV0(rRight, rBottom, 0xffffffff, ruRight, rvBottom);
    pVertex[3].SetXYDUV0(rRight, rTop, 0xffffffff, ruRight, 0.0f);

    //
    // Finish up
    //

    IFC(EndPrimitiveFan(&m_vbBufferDUV2));

Cleanup:
    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetSupportedTextureFormat
//
//  Synopsis:
//      Given a source format and destination format select a format for a
//      texture
//
//      Return WGXERR_UNSUPPORTEDPIXELFORMAT if no acceptable format was found
//      to be supported.  See GatherSupportedTextureFormats for more details on
//      what is acceptable.
//

HRESULT
CD3DDeviceLevel1::GetSupportedTextureFormat(
    MilPixelFormat::Enum fmtBitmapSource,       // Current format of bitmap

    MilPixelFormat::Enum fmtDestinationSurface, // Format of surface onto which
                                          // the bitmap will be drawn
    bool fForceAlpha,                     // Use alpha in texture regardless
                                          // of whether the input used alpha.
    __out_ecount(1)
    MilPixelFormat::Enum &fmtTextureSource      // Format of the texture to
                                          // hold the bitmap
    ) const
{
    //
    // If the destination surface has higher precision than 8 bits per channel,
    // use high precision textures.  Typically, we expect 32bppBGR101010.
    //

    bool fUseAlpha = fForceAlpha || HasAlphaChannel(fmtBitmapSource);

    if (fmtDestinationSurface == MilPixelFormat::BGR32bpp101010)
    {
        //
        // Break down texture format based on source format
        //
        //  There are three possibilities for texture format:
        //      MilPixelFormat::BGR32bpp101010
        //      MilPixelFormat::RGB128bppFloat
        //      MilPixelFormat::PRGBA128bppFloat
        //

        if (fmtBitmapSource == MilPixelFormat::RGB128bppFloat)
        {
            // Special case MilPixelFormat::RGB128bppFloat
            fmtTextureSource = fUseAlpha
                               ? m_fmtSupportFor128bppPRGBAFloat
                               : m_fmtSupportFor128bppRGBFloat;
        }
        else
        {
            //
            // Check if MilPixelFormat::BGR32bpp101010 can handle the source
            // format.  There are two requirements:
            //  1) Pixel format size must be less than 32bppBGR101010
            //  2) There must not be an alpha channel
            //

            Assert(GetPixelFormatSize(MilPixelFormat::BGR32bpp101010) == 32);
            if ((GetPixelFormatSize(fmtBitmapSource) <= 32) && !fUseAlpha)
            {
                //
                // Convert to MilPixelFormat::BGR32bpp101010 as it has enough
                // precision and there is no alpha channel.
                //
                fmtTextureSource = m_fmtSupportFor32bppBGR101010;
            }
            else
            {
                //
                // Convert to MilPixelFormat::PRGBA128bppFloat so that we can retain
                // the alpha channel and/or enough precision.
                //
                //
                fmtTextureSource = m_fmtSupportFor128bppPRGBAFloat;
            }
        }

        Assert(   (fmtTextureSource == MilPixelFormat::BGR32bpp101010)
               || (fmtTextureSource == MilPixelFormat::RGB128bppFloat)
               || (fmtTextureSource == MilPixelFormat::PRGBA128bppFloat)
               || (fmtTextureSource == MilPixelFormat::Undefined));
    }
    else
    {

        //
        // Convert formats to a 32bpp BGR format
        //   Note that this will cause precision loss if the source format has
        //   more that 8 bits per channel.
        //

        if (!fUseAlpha)
        {
            // No alpha channel => MilPixelFormat::BGR32bpp
            fmtTextureSource = m_fmtSupportFor32bppBGR;
        }
        else
        {
            // Alpha channel => MilPixelFormat::PBGRA32bpp
            fmtTextureSource = m_fmtSupportFor32bppPBGRA;
        }

        Assert(   (fmtTextureSource == MilPixelFormat::BGR32bpp)
               || (fmtTextureSource == MilPixelFormat::PBGRA32bpp)
               || (fmtTextureSource == MilPixelFormat::BGR32bpp101010)
               || (fmtTextureSource == MilPixelFormat::RGB128bppFloat)
               || (fmtTextureSource == MilPixelFormat::PRGBA128bppFloat)
               || (fmtTextureSource == MilPixelFormat::Undefined));
    }

    HRESULT hr = S_OK;

    if (fmtTextureSource == MilPixelFormat::Undefined)
    {
        MIL_THR(WGXERR_UNSUPPORTEDPIXELFORMAT);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetSupportedMultisampleType
//
//  Synopsis:
//      Given a destination format select a multisample format
//

D3DMULTISAMPLE_TYPE
CD3DDeviceLevel1::GetSupportedMultisampleType(
    MilPixelFormat::Enum fmtDestinationSurface        // Format of target surface
    ) const
{
    D3DMULTISAMPLE_TYPE MultisampleType;

    if (fmtDestinationSurface == MilPixelFormat::BGR32bpp)
    {
        MultisampleType = m_multisampleTypeFor32bppBGR;
    }
    else if (fmtDestinationSurface == MilPixelFormat::PBGRA32bpp)
    {
        MultisampleType = m_multisampleTypeFor32bppPBGRA;
    }
    else
    {
        Assert(fmtDestinationSurface == MilPixelFormat::BGR32bpp101010);
        MultisampleType = m_multisampleTypeFor32bppBGR101010;
    }


    return MultisampleType;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetClipRect
//
//  Synopsis:
//      Provide access aligned clipping using SetViewport.
//      Note that the vieport will be reset on SetRenderTarget.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetClipRect(
    __in_ecount_opt(1) const CMILSurfaceRect *prcClip
    )
{
    HRESULT hr = S_OK;
    MilPointAndSizeL rcTargetSurface = {0, 0, m_desc.Width, m_desc.Height};
    MilPointAndSizeL rcSurfaceIntersectClip;
    const MilPointAndSizeL *prcNewClip = NULL;

    if (prcClip)
    {
        MilPointAndSizeL rcClip = {
            prcClip->left,
            prcClip->top,
            prcClip->right - prcClip->left,
            prcClip->bottom - prcClip->top
        };

        //
        // Determine the correct new clip rect by intersecting it with
        //  the target bounds and the given clip rect.
        //

        if (!IntersectRect(OUT &rcSurfaceIntersectClip, &rcTargetSurface, &rcClip))
        {
            hr = WGXHR_CLIPPEDTOEMPTY;
            goto Cleanup;
        }

        if (!IsClipSet(&rcSurfaceIntersectClip))
        {
            prcNewClip = &rcSurfaceIntersectClip;
        }
    }
    else

    {
        //
        // If clipping has been previously set then reset it to
        //  the full extents of the target.
        //

        if (IsClipSet())
        {
            prcNewClip = &rcTargetSurface;
        }
    }

    if (prcNewClip)
    {
        if (SupportsScissorRect())
        {
            if (prcNewClip == &rcTargetSurface ||
                RtlEqualMemory(prcNewClip, &rcTargetSurface, sizeof(*prcNewClip)))
            {
                // This optimization removes the scissor rect when rectangular
                // clipping is turned off.
                IFC(SetScissorRect(NULL));
            }
            else
            {
                IFC(SetScissorRect(prcNewClip));
            }
        }
        else
        {
            IFC(SetViewport(prcNewClip));

            IFC(SetSurfaceToClippingMatrix(
                prcNewClip
                ));
        }

        SetClipSet(prcClip != NULL);
        SetClip(*prcNewClip); // don't really need this if !m_fClipSet

        EventWriteSetClipInfo(prcNewClip->X,
                              prcNewClip ->Y,
                              prcNewClip->Width,
                              prcNewClip->Height);
    }

Cleanup:
    RRETURN1(hr, WGXHR_CLIPPEDTOEMPTY);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetClipRect
//
//  Synopsis:
//      Return the current clip rect.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::GetClipRect(
    __out_ecount(1) MilPointAndSizeL * const prcClipRect
    ) const
{
    if (IsClipSet())
    {
        *prcClipRect = GetClip();
    }
    else
    {
        //
        // If not set then there is no clip which is equivalent
        // to a clip exactly the size of the target.
        //

        prcClipRect->X = 0;
        prcClipRect->Y = 0;
        prcClipRect->Width = m_desc.Width;
        prcClipRect->Height = m_desc.Height;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DbgTraceDeviceCreationFailure
//
//  Synopsis:
//      Output information about the device creation failure
//
//------------------------------------------------------------------------------
#if DBG==1
void
CD3DDeviceLevel1::DbgTraceDeviceCreationFailure(
    UINT uAdapter,
    __in PCSTR szMessage,
    HRESULT hrError
    )
{
    TraceTag((tagError, "MIL-HW(adapter=%d): Can't create d3d rendering device.", uAdapter));
    TraceTag((tagError, "MIL-HW(adapter=%d): %s (hr = 0x%x).", uAdapter, szMessage, hrError));
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateDepthBuffer
//
//  Synopsis:
//      delegate to IDirect3DDevice9::CreateDepthStencilBuffer
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateDepthBuffer(
    UINT uWidth,
    UINT uHeight,
    D3DMULTISAMPLE_TYPE MultisampleType,
    __deref_out_ecount(1) CD3DSurface ** const ppSurface
    )
{
    HRESULT hr = S_OK;
    IDirect3DSurface9* pD3DSurface = NULL;


    //   What depth buffer should be used
    //  since we no longer need the stencil.  Is 16 or 32 better than 24?

    {
        MtSetDefault(Mt(D3DStencilSurface));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateDepthStencilSurface(
            uWidth,
            uHeight,
            kD3DDepthFormat,
            MultisampleType,
            0,
            false, /* fDiscard */
            &pD3DSurface,
            NULL // HANDLE* pSharedHandle
            );

        // In the event that we've failed because we're out of video memory and we're attempting
        // to multisample, we should break out of the BEGIN_DEVICE_ALLOCATION loop.
        // CHwSurfaceRenderTarget::Begin3DInternal will reduce the multisample level and retry.
        //
        // Rationale: We only enable 3D AA on WDDM which has virtualized video memory.  If
        //            WDDM is reporting OOVM we're in really bad shape.
        //
        if ((hr == D3DERR_OUTOFVIDEOMEMORY) && (MultisampleType != D3DMULTISAMPLE_NONE))
        {
            break;
        }

        END_DEVICE_ALLOCATION;

        IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM
    }

    IFC(CD3DSurface::Create(&m_resourceManager, pD3DSurface, ppSurface));

Cleanup:
    ReleaseInterfaceNoNULL(pD3DSurface);
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetDepthStencilSurface
//
//  Synopsis:
//      Turns ZEnable on and off and delegates to
//      IDirect3DDevice9::SetDepthStencilSurface if pSurface is non null
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetDepthStencilSurface(
    __in_ecount_opt(1) CD3DSurface *pSurface
    )
{
    HRESULT hr = S_OK;

    //
    // Enable or disable z-buffer
    //

    if (pSurface)
    {
        IFC(SetRenderState(
            D3DRS_ZENABLE,
            D3DZB_TRUE
            ));
    }
    else
    {
        IFC(SetRenderState(
            D3DRS_ZENABLE,
            D3DZB_FALSE
            ));

        IFC(SetRenderState(
            D3DRS_STENCILENABLE,
            false
            ));
    }

    //
    // Set the z-buffer if necessary
    //

    Assert(m_pCurrentRenderTargetNoRef);
    m_pDepthStencilBufferForCurrentRTNoRef = pSurface;

    IFC(SetDepthStencilSurfaceInternal(pSurface));

Cleanup:
    if (FAILED(hr))
    {
        // We might as well try to turn off clipping in case of failure
        // Otherwise who knows what the clipping state will be
        IGNORE_HR(SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE)); // HRESULT ignored
        IGNORE_HR(SetRenderState(D3DRS_STENCILENABLE, D3DZB_FALSE)); // HRESULT ignored
        IGNORE_HR(SetDepthStencilSurfaceInternal(NULL));
    }
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ReleaseUseOfDepthStencilSurface
//
//  Synopsis:
//       Releases any cached use the device may have of the given
//      depth stencil buffer, thereby enabling the D3D surface to be cleaned up
//      when it is truely no longer in use.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ReleaseUseOfDepthStencilSurface(
    __in_ecount_opt(1) CD3DSurface *pSurface    // a surface that will no
                                                // longer be valid
    )
{
    AssertDeviceEntry(*this);

    if (pSurface)
    {
        //
        // It's possible for the state manager to have a different
        // depth/stencil buffer than what this has tracked as d/s buffer for
        // current RT.  For example SetDepthStencilSurface can fail and leave
        // m_pDepthStencilBufferForCurrentRTNoRef in an inaccurate state.  So
        // always call to state manager.
        //

        IGNORE_HR(ReleaseUseOfDepthStencilSurfaceInternal(pSurface));

        //
        // If "current" depth/stencil is released then note that there is no
        // d/s buffer for current RT.
        //

        if (m_pDepthStencilBufferForCurrentRTNoRef == pSurface)
        {
            m_pDepthStencilBufferForCurrentRTNoRef = NULL;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetLinearPalette
//
//  Synopsis:
//      setup identical palette to the device. Intended to save video memory on
//      alpha-only textures: when D3DFMT_A8 is not supported, we can use
//      D3DFMT_P8 instead and save 75% of VM in comparison with D3DFMT_A8R8G8B8.
//      We assume that palette is not used for another purposes, so this routine
//      need to be called just once, and fixed palette number == 0 is used.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetLinearPalette()
{
    HRESULT hr = S_OK;
    PALETTEENTRY* pPal = (PALETTEENTRY*)WPFAlloc(
        ProcessHeap,
        Mt(CD3DDeviceLevel1),
        sizeof(PALETTEENTRY)*256
        );

    IFCOOM(pPal);
    for (unsigned int i = 0 ; i < 256; i++)
         ((unsigned int*)pPal)[i] = i*0x01010101;

    IFC( m_pD3DDevice->SetPaletteEntries(0, pPal) );
    IFC( m_pD3DDevice->SetCurrentTexturePalette(0) );

Cleanup:
    if (pPal) WPFFree(ProcessHeap, pPal);
    RRETURN(HandleDIE(hr));
}


#if DBG_STEP_RENDERING
HRESULT
CD3DDeviceLevel1::DbgSaveSurface(
    __in_ecount(1) CD3DSurface *pD3DSurface,
    __in_ecount(1) const MilPointAndSizeL &rcSave
    )
{
    HRESULT hr;

    Assert(rcSave.X >= 0);
    Assert(rcSave.Y >= 0);
    Assert(rcSave.Width > 0);
    Assert(rcSave.Height > 0);

    Assert(!m_pDbgSaveSurface);

    D3DSURFACE_DESC const &d3dsd = pD3DSurface->Desc();

    MIL_THR(CreateRenderTarget(
        rcSave.Width,
        rcSave.Height,
        d3dsd.Format,
        D3DMULTISAMPLE_NONE,
        0,
        false,
        &m_pDbgSaveSurface
        ));

    if (SUCCEEDED(hr))
    {
        RECT rcSrc = {
            rcSave.X, rcSave.Y,
            rcSave.X + rcSave.Width, rcSave.Y + rcSave.Height
            };

        RECT rcDst = { 0, 0, rcSave.Width, rcSave.Height };

        MIL_THR(StretchRect(
            pD3DSurface,
            &rcSrc,
            m_pDbgSaveSurface,
            &rcDst,
            D3DTEXF_NONE
            ));
    }

    if (FAILED(hr))
    {
        ReleaseInterface(m_pDbgSaveSurface);
    }

    RRETURN(HandleDIE(hr));
}


HRESULT
CD3DDeviceLevel1::DbgRestoreSurface(
    __inout_ecount(1) CD3DSurface *pD3DSurface,
    __in_ecount(1) const MilPointAndSizeL &rcRestore
    )
{
    HRESULT hr;

    Assert(rcRestore.X >= 0);
    Assert(rcRestore.Y >= 0);
    Assert(rcRestore.Width > 0);
    Assert(rcRestore.Height > 0);

    Assert(m_pDbgSaveSurface);

    RECT rcSrc = { 0, 0, rcRestore.Width, rcRestore.Height };
    RECT rcDst = {
        rcRestore.X, rcRestore.Y,
        rcRestore.X + rcRestore.Width, rcRestore.Y + rcRestore.Height
        };

    hr = THR(StretchRect(
        m_pDbgSaveSurface,
        &rcSrc,
        pD3DSurface,
        &rcDst,
        D3DTEXF_NONE
        ));

    m_pDbgSaveSurface->Release();
    m_pDbgSaveSurface = NULL;

    RRETURN(HandleDIE(hr));
}
#endif DBG_STEP_RENDERING


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawBox
//
//  Synopsis:
//      Takes a MilPointAndSize3F, generates a box primitive from that and
//      renders it with the given fillmode and color.  It restores the fill mode
//      when it's done rendering.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawBox(
    __in_ecount(1) const MilPointAndSize3F *pbox,
    const D3DFILLMODE d3dFillMode,
    const DWORD dwColor
    )
{
    HRESULT hr = S_OK;
    CD3DVertexXYZDUV2 vecMeshBounds[8];
    WORD wMeshTriangles[36];
    D3DFILLMODE d3dOrigFillMode = D3DFILL_SOLID;
    D3DCMPFUNC d3dOrigDepthTest = D3DCMP_LESSEQUAL;
    bool fFillModeRetrieved = false;

    IFC(GetFillMode(
        &d3dOrigFillMode
        ));

    IFC(GetDepthTestFunction(
        &d3dOrigDepthTest
        ));

    fFillModeRetrieved = true;

    // Lower Left

    vecMeshBounds[0].X = pbox->X;
    vecMeshBounds[0].Y = pbox->Y;
    vecMeshBounds[0].Z = pbox->Z;


    // Lower Right

    vecMeshBounds[1].X = pbox->X + pbox->LengthX;
    vecMeshBounds[1].Y = pbox->Y;
    vecMeshBounds[1].Z = pbox->Z;


    // Upper Right

    vecMeshBounds[2].X = pbox->X + pbox->LengthX;
    vecMeshBounds[2].Y = pbox->Y + pbox->LengthY;
    vecMeshBounds[2].Z = pbox->Z;


    // Upper Left

    vecMeshBounds[3].X = pbox->X;
    vecMeshBounds[3].Y = pbox->Y + pbox->LengthY;
    vecMeshBounds[3].Z = pbox->Z;


    // Copy the 4 corners shifted by the length of z for the other half
    // of the cube.

    for (int i = 4; i < 8; i++)
    {
        vecMeshBounds[i].X = vecMeshBounds[i-4].X;
        vecMeshBounds[i].Y = vecMeshBounds[i-4].Y;
        vecMeshBounds[i].Z = vecMeshBounds[i-4].Z + pbox->LengthZ;
    }

    for (int i = 0; i < 8; i++)
    {
        vecMeshBounds[i].Diffuse = dwColor;
        vecMeshBounds[i].U0 = 0.0f;
        vecMeshBounds[i].V0 = 0.0f;
        vecMeshBounds[i].U1 = 0.0f;
        vecMeshBounds[i].V1 = 0.0f;
    }

    // Bottom Face

    wMeshTriangles[0] = 0;
    wMeshTriangles[1] = 2;
    wMeshTriangles[2] = 1;

    wMeshTriangles[3] = 0;
    wMeshTriangles[4] = 3;
    wMeshTriangles[5] = 2;

    // Top Face

    wMeshTriangles[6] = 4;
    wMeshTriangles[7] = 5;
    wMeshTriangles[8] = 6;

    wMeshTriangles[9] = 4;
    wMeshTriangles[10] = 6;
    wMeshTriangles[11] = 7;


    // Left Face

    wMeshTriangles[12] = 0;
    wMeshTriangles[13] = 4;
    wMeshTriangles[14] = 3;

    wMeshTriangles[15] = 3;
    wMeshTriangles[16] = 4;
    wMeshTriangles[17] = 7;


    // Right Face

    wMeshTriangles[18] = 1;
    wMeshTriangles[19] = 2;
    wMeshTriangles[20] = 5;

    wMeshTriangles[21] = 2;
    wMeshTriangles[22] = 6;
    wMeshTriangles[23] = 5;


    // Upper Face

    wMeshTriangles[24] = 2;
    wMeshTriangles[25] = 3;
    wMeshTriangles[26] = 6;

    wMeshTriangles[27] = 3;
    wMeshTriangles[28] = 7;
    wMeshTriangles[29] = 6;


    // Lower Face

    wMeshTriangles[30] = 0;
    wMeshTriangles[31] = 1;
    wMeshTriangles[32] = 4;

    wMeshTriangles[33] = 1;
    wMeshTriangles[34] = 5;
    wMeshTriangles[35] = 4;


    // Set Fill Mode

    IFC(SetRenderState(
        D3DRS_FILLMODE,
        d3dFillMode
        ));

    IFC(SetRenderState(
        D3DRS_ZFUNC,
        D3DCMP_ALWAYS
        ));

    // Set FVF

    IFC(SetFVF(vecMeshBounds[0].Format));

    IFC(SetRenderState_AlphaSolidBrush());

    // Draw the Mesh

    IFC(DrawIndexedTriangleListUP(
        8,
        12,
        wMeshTriangles,
        vecMeshBounds,
        sizeof(vecMeshBounds[0])
        ));

    IFC(SetRenderState(
        D3DRS_FILLMODE,
        d3dOrigFillMode
        ));

    IFC(SetRenderState(
        D3DRS_ZFUNC,
        d3dOrigDepthTest
        ));

Cleanup:
    if (FAILED(hr) && fFillModeRetrieved)
    {
        IGNORE_HR(SetRenderState(
            D3DRS_FILLMODE,
            d3dOrigFillMode
            ));

        IGNORE_HR(SetRenderState(
            D3DRS_ZFUNC,
            d3dOrigDepthTest
            ));
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawIndexedTriangleListUP
//
//  Synopsis:
//      This method creates a fast path for our XYZDUV2 vertices which will use
//      custom VB/IB code instead of DrawIndexedPrimUP.
//
//      Note that the main advantage here is that we bypass the
//      lowvertexcount limit in d3d9 which is set too low at 96.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawIndexedTriangleListUP(
    __range(1, UINT_MAX) UINT uNumVertices,
    __range(1, UINT_MAX) UINT uPrimitiveCount,
    __in_xcount(sizeof(WORD) * uPrimitiveCount * 3) const WORD* pIndexData,
    __in const void* pVertexStreamZeroData,
    __range(1, UINT_MAX) UINT uVertexStreamZeroStride
    )
{
    HRESULT hr = S_OK;
    UINT uiNumIndices = uPrimitiveCount*3;
    void *pvDataVertices = NULL;
    void *pvDataIndices = NULL;
    bool fVBLockAcquired = false;
    bool fIBLockAcquired = false;
    UINT uiCurrentVertex = 0;
    UINT uiCurrentIndex = 0;

    Assert(uNumVertices > 0);
    Assert(uPrimitiveCount > 0);
    Assert(uVertexStreamZeroStride > 0);

    //
    // Add bandwidth contribution
    //

    if (g_pMediaControl)
    {
        const WORD *pIndex = static_cast<const WORD *>(pIndexData);
        const CD3DVertexXYZDUV2 *pVertices = static_cast<const CD3DVertexXYZDUV2 *>(pVertexStreamZeroData);

        for (UINT i = 0; i < uPrimitiveCount; i++)
        {
            CD3DVertexXYZDUV2 v1 = pVertices[pIndex[0]];
            CD3DVertexXYZDUV2 v2 = pVertices[pIndex[1]];
            CD3DVertexXYZDUV2 v3 = pVertices[pIndex[2]];

            //  Area = abs((xB*yA-xA*yB)+(xC*yB-xB*yC)+(xA*yC-xC*yA))/2
            float rArea = fabs((v2.X*v1.Y-v1.X*v2.Y) + (v3.X*v2.Y-v2.X*v3.Y) + (v1.X*v3.Y-v3.X*v1.Y))/2.0f;
            g_lPixelsFilledPerFrame += CFloatFPU::Ceiling(rArea);

            pIndex += 3;
        }
    }

    //
    // Try to lock both the IB/VB for the fast path case
    //

    hr = THR(m_pHwVertexBuffer->Lock(
        uNumVertices,
        uVertexStreamZeroStride,
        &pvDataVertices,
        &uiCurrentVertex
        ));

    if (SUCCEEDED(hr))
    {
        fVBLockAcquired = true;

        hr = THR(m_pHwIndexBuffer->Lock(
            uiNumIndices,
            reinterpret_cast<WORD **>(&pvDataIndices),
            &uiCurrentIndex
            ));

        fIBLockAcquired = SUCCEEDED(hr);
    }

    if (!fVBLockAcquired || !fIBLockAcquired)
    {
        MtSetDefault(Mt(D3DDrawIndexedPrimitiveUP));

        //
        // Fall back to to the d3d version
        //

        //
        // Whenever we call DrawPrimitiveUP, D3D resets the first stream
        // and the index source to NULL vertex & index streams.  In order to
        // keep our cached stream value identical to D3D's we need to set our
        // stream and index sources to NULL as well.
        //
        IFC(SetStreamSource(
            0,
            NULL
            ));

        IFC(SetIndices(NULL));

        IFC(m_pD3DDevice->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST,
            0,
            uNumVertices,
            uPrimitiveCount,
            pIndexData,
            D3DFMT_INDEX16,
            pVertexStreamZeroData,
            uVertexStreamZeroStride
            ));

        UpdateMetrics(uNumVertices, uPrimitiveCount);

        goto Cleanup;
    }

    //
    // Update vertices
    //

    RtlCopyMemory(pvDataVertices, pVertexStreamZeroData, uNumVertices*uVertexStreamZeroStride);

    IFC(m_pHwVertexBuffer->Unlock(uNumVertices));
    fVBLockAcquired = false;

    //
    // Update indices
    //

    RtlCopyMemory(pvDataIndices, pIndexData, sizeof(short)*uiNumIndices);

    IFC(m_pHwIndexBuffer->Unlock());
    fIBLockAcquired = false;

    //
    // Lots of dx device methods (like DrawPrimUp/DrawIndexedPrimUp)
    // will change the current IB/VB.  So, we either need to track all of
    // those or set it on every draw call.
    //
    // It turns out that d3d has a fast path when the IB/VB don't change,
    // so setting it on each draw isn't costing us much.
    //

    IFC(SetStreamSource(
        m_pHwVertexBuffer->GetD3DBuffer(),
        uVertexStreamZeroStride
        ));

    IFC(SetIndices(
        m_pHwIndexBuffer->GetD3DBuffer()
        ));

    //
    // Call DrawIndexedPrimitive
    //

    {
        MtSetDefault(Mt(D3DDrawIndexedPrimitive));

        IFC(m_pD3DDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            uiCurrentVertex,
            0,
            uNumVertices,
            uiCurrentIndex,
            uPrimitiveCount
            ));
    }

    UpdateMetrics(uNumVertices, uPrimitiveCount);

Cleanup:
    if (fVBLockAcquired)
    {
        IGNORE_HR(m_pHwVertexBuffer->Unlock(uNumVertices));
    }
    if (fIBLockAcquired)
    {
        IGNORE_HR(m_pHwIndexBuffer->Unlock());
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawIndexedTriangleList
//
//  Synopsis:
//      Draw whatever is set in the current stream as an indexed triangle list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawIndexedTriangleList(
    UINT uBaseVertexIndex,
    UINT uMinIndex,
    UINT cVertices,
    UINT uStartIndex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawIndexedPrimitive));

    IFC(m_pD3DDevice->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        uBaseVertexIndex,
        uMinIndex,
        cVertices,
        uStartIndex,
        cPrimitives
        ));

    UpdateMetrics(cVertices, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawTriangleList
//
//  Synopsis:
//      Draw whatever is set in the current stream as a triangle list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawTriangleList(
    UINT uStartVertex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawPrimitive));

    IFC(m_pD3DDevice->DrawPrimitive(
        D3DPT_TRIANGLELIST,
        uStartVertex,
        cPrimitives
        ));

    UpdateMetrics(cPrimitives*3, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawTriangleStrip
//
//  Synopsis:
//      Draw whatever is set in the current stream as a triangle strip.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawTriangleStrip(
    UINT uStartVertex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawPrimitive));

    IFC(m_pD3DDevice->DrawPrimitive(
        D3DPT_TRIANGLESTRIP,
        uStartVertex,
        cPrimitives
        ));

    UpdateMetrics(cPrimitives + 2, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawLargePrimitiveUP
//
//  Synopsis:
//      Draw a primitive that exceeds the max primitive count on the device by
//      calling DrawPrimUP multiple times.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawLargePrimitiveUP(
    D3DPRIMITIVETYPE primitiveType,
    UINT uPrimitiveCount,
    __in_xcount(
        //
        // Vertex counts are:
        //
        // D3DPT_LINELIST - PrimitiveCount*2
        // D3DPT_TRIANGLESTRIP - PrimitiveCount+2
        // D3DPT_TRIANGLEFAN - PrimitiveCount+2    <- Although, fans are not currently supported by this method.
        //
        uVertexStreamZeroStride * ((primitiveType == D3DPT_LINELIST) ? uPrimitiveCount*2 : uPrimitiveCount+2)
        ) const void* pVertexStreamZeroData,
    UINT uVertexStreamZeroStride
    )
{
    HRESULT hr = S_OK;
    const BYTE *pbVertexStreamPosition = static_cast<const BYTE *>(pVertexStreamZeroData);

    UINT uMaxPrimitiveCount = 0;

    //
    // Check for supported primitive types and compute max primitives for the 0xffff case
    //
    // (From the d3d docs) MaxPrimitiveCount (is the) Maximum number of primitives for each
    // IDirect3DDevice9::DrawPrimitive call. There are two cases:
    //
    // - If MaxPrimitiveCount is not equal to 0xffff, you can draw at most MaxPrimitiveCount
    //   primitives with each draw call.
    //
    // - However, if MaxPrimitiveCount equals 0xffff, you can still draw at most MaxPrimitiveCount
    //   primitive, but you may also use no more than MaxPrimitiveCount unique vertices (since each
    //   primitive can potentially use three different vertices).


    switch (primitiveType)
    {
    case D3DPT_LINELIST:
        uMaxPrimitiveCount = 0xffff / 2;
        break;

    case D3DPT_TRIANGLELIST:
        uMaxPrimitiveCount = 0xffff / 3;
        break;

    case D3DPT_TRIANGLESTRIP:
        uMaxPrimitiveCount = 0xffff - 2;
        break;

    default:
        //
        // For completeness, we should also implement triangle fans here.  However,
        // this is unused by trapezoidal AA and we only draw triangle fans
        // with 4 vertices (which is always within the primitive count allowed on our
        // supported cards), so the triangle fan case is left unimplemented for the
        // moment since it is not currently needed.
        //
        AssertMsg(0, "Unsupported primitive type");
        IFC(E_NOTIMPL);
    }

    if (m_caps.MaxPrimitiveCount != 0xffff)
    {
        uMaxPrimitiveCount = static_cast<UINT>(m_caps.MaxPrimitiveCount);
    }

    //
    // Call DrawPrimitiveUP multiple times
    //

    while (uPrimitiveCount > 0)
    {
        MtSetDefault(Mt(D3DDrawPrimitiveUP));
        UINT uDrawPrimCount = min(uPrimitiveCount, uMaxPrimitiveCount);

        //
        // Call draw primitive UP
        //

        //
        // Whenever we call DrawPrimitiveUP, D3D resets the first stream source
        // to a NULL vertex stream.  In order to keep our cached stream value
        // identical to D3D's we need to set our stream source to NULL as well.
        //
        IFC(SetStreamSource(
            0,
            NULL
            ));

        IFC(m_pD3DDevice->DrawPrimitiveUP(
            primitiveType,
            uDrawPrimCount,
            pbVertexStreamPosition,
            uVertexStreamZeroStride
            ));

        UpdateMetrics(0, uDrawPrimCount);

        //
        // Advance
        //

        uPrimitiveCount -= uDrawPrimCount;

        switch (primitiveType)
        {
        case D3DPT_LINELIST:
            // We used uDrawPrimCount*2 vertices (because linelists have 2 vertices per
            // primitive), so advance by that amount now.
            pbVertexStreamPosition += uVertexStreamZeroStride*2*uDrawPrimCount;
            UpdateMetrics(2 * uDrawPrimCount, 0);
            break;

        case D3DPT_TRIANGLELIST:
            pbVertexStreamPosition += uVertexStreamZeroStride*3*uDrawPrimCount;
            UpdateMetrics(3 * uDrawPrimCount, 0);
            break;

        case D3DPT_TRIANGLESTRIP:
            // Each vertex after the first 2 defines a new triangle, so the total
            // vertex count we used is uDrawPrimCount+2.  However, since
            // we need to duplicate the last two vertices to ensure we continue
            // the strip properly, we only advance uPrimitiveCount vertices.
            pbVertexStreamPosition += uVertexStreamZeroStride*uDrawPrimCount;
            UpdateMetrics(uDrawPrimCount + 2, 0);
            break;
        }
    }

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CopyD3DTexture
//
//  Synopsis:
//      Does a non-filtered copy of all a source texture contents into all of a
//      destination texture contents.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CopyD3DTexture(
    __in_ecount(1) IDirect3DTexture9 *pD3DSourceTexture,
    __inout_ecount(1) IDirect3DTexture9 *pD3DDestinationTexture
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pD3DSurfaceSource      = NULL;
    IDirect3DSurface9 *pD3DSurfaceDestination = NULL;

    IFC(pD3DSourceTexture->GetSurfaceLevel(0, &pD3DSurfaceSource));
    IFC(pD3DDestinationTexture->GetSurfaceLevel(0, &pD3DSurfaceDestination));

    IFC(m_pD3DDevice->StretchRect(
        pD3DSurfaceSource,
        NULL,
        pD3DSurfaceDestination,
        NULL,
        D3DTEXF_NONE
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pD3DSurfaceSource);
    ReleaseInterfaceNoNULL(pD3DSurfaceDestination);

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawPrimitiveUP
//
//  Synopsis:
//      This method creates a fast path for our XYZDUV2 vertices which will use
//      custom VB code instead of DrawPrimUP.
//
//      Note that the main advantage here is that we bypass the
//      lowvertexcount limit in d3d9 which is set too low at 96.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawPrimitiveUP(
    D3DPRIMITIVETYPE primitiveType,
    UINT uPrimitiveCount,
    __in_xcount(
        //
        // Vertex counts are:
        //
        // D3DPT_LINELIST - PrimitiveCount*2
        // D3DPT_TRIANGLESTRIP - PrimitiveCount+2
        // D3DPT_TRIANGLEFAN - PrimitiveCount+2
        //
        uVertexStreamZeroStride * ((primitiveType == D3DPT_LINELIST) ? uPrimitiveCount*2 : uPrimitiveCount+2)
        ) const void* pVertexStreamZeroData,
    UINT uVertexStreamZeroStride
    )
{
    HRESULT hr = S_OK;
    void *pvData = NULL;
    UINT uNumVertices = 0;
    bool fLockAcquired = false;
    UINT uiCurrentVertex = 0;

    Assert(uPrimitiveCount > 0);
    Assert(pVertexStreamZeroData);
    Assert(uVertexStreamZeroStride > 0);

    //
    // Compute number of vertices for supported types
    //

    switch (primitiveType)
    {
    case D3DPT_LINELIST:
        uNumVertices = uPrimitiveCount*2;
        break;

    case D3DPT_TRIANGLELIST:
        uNumVertices = uPrimitiveCount*3;
        break;

    case D3DPT_TRIANGLEFAN:
        {
            uNumVertices = uPrimitiveCount+2;

            //
            // Add bandwidth contribution
            //
            if (g_pMediaControl)
            {

                const CD3DVertexXYZDUV2 *pVertices =
                    static_cast<const CD3DVertexXYZDUV2 *>(pVertexStreamZeroData);

                CD3DVertexXYZDUV2 v1 = pVertices[0];

                for (UINT i = 0; i < uPrimitiveCount; i++)
                {
                    CD3DVertexXYZDUV2 v2 = pVertices[1];
                    CD3DVertexXYZDUV2 v3 = pVertices[2];

                    //  Area = abs((xB*yA-xA*yB)+(xC*yB-xB*yC)+(xA*yC-xC*yA))/2
                    float rArea = fabs((v2.X*v1.Y-v1.X*v2.Y) + (v3.X*v2.Y-v2.X*v3.Y) + (v1.X*v3.Y-v3.X*v1.Y))/2.0f;
                    g_lPixelsFilledPerFrame += CFloatFPU::Ceiling(rArea);

                    pVertices += 1;
                }
            }
            break;
        }

    case D3DPT_TRIANGLESTRIP:
        {
            uNumVertices = uPrimitiveCount+2;

            //
            // Add bandwidth contribution
            //
            if (g_pMediaControl)
            {
                const CD3DVertexXYZDUV2 *pVertices =
                    static_cast<const CD3DVertexXYZDUV2 *>(pVertexStreamZeroData);

                for (UINT i = 0; i < uPrimitiveCount; i++)
                {
                    CD3DVertexXYZDUV2 v1 = pVertices[0];
                    CD3DVertexXYZDUV2 v2 = pVertices[1];
                    CD3DVertexXYZDUV2 v3 = pVertices[2];

                    //  Area = abs((xB*yA-xA*yB)+(xC*yB-xB*yC)+(xA*yC-xC*yA))/2
                    float rArea = fabs((v2.X*v1.Y-v1.X*v2.Y) + (v3.X*v2.Y-v2.X*v3.Y) + (v1.X*v3.Y-v3.X*v1.Y))/2.0f;
                    g_lPixelsFilledPerFrame += CFloatFPU::Ceiling(rArea);

                    pVertices += 1;
                }
            }
            break;
        }

    default:
        AssertMsg(0, "Unsupported primitive type");
        IFC(E_INVALIDARG);
    }

    //
    // If the primitive count exceeds the max available on the device,
    // use multiple DrawPrimitiveUP calls.
    //
    // See DrawLargePrimitiveUP for explanation of the 0xffff case.
    //

    if (   uPrimitiveCount > m_caps.MaxPrimitiveCount
        || (m_caps.MaxPrimitiveCount == 0xffff && uNumVertices > 0xffff))
    {
        IFC(DrawLargePrimitiveUP(
            primitiveType,
            uPrimitiveCount,
            pVertexStreamZeroData,
            uVertexStreamZeroStride
            ));

        goto Cleanup;
    }

    //
    // Try to lock both the IB/VB for the fast path case
    //

    hr = THR(m_pHwVertexBuffer->Lock(
        uNumVertices,
        uVertexStreamZeroStride,
        &pvData,
        &uiCurrentVertex
        ));

    fLockAcquired = SUCCEEDED(hr);

    if (!fLockAcquired)
    {
        MtSetDefault(Mt(D3DDrawPrimitiveUP));

        //
        // Fall back to to the d3d version
        //

        //
        // Whenever we call DrawPrimitiveUP, D3D resets the first stream source
        // to a NULL vertex stream.  In order to keep our cached stream value
        // identical to D3D's we need to set our stream source to NULL as well.
        //
        IFC(SetStreamSource(
            NULL,
            0
            ));

        IFC(m_pD3DDevice->DrawPrimitiveUP(
            primitiveType,
            uPrimitiveCount,
            pVertexStreamZeroData,
            uVertexStreamZeroStride
            ));

        UpdateMetrics(uNumVertices, uPrimitiveCount);

        goto Cleanup;
    }

    //
    // Update vertices
    //

    RtlCopyMemory(pvData, pVertexStreamZeroData, uNumVertices*uVertexStreamZeroStride);

    IFC(m_pHwVertexBuffer->Unlock(uNumVertices));
    fLockAcquired = false;

    //
    // Lots of dx device methods (like DrawPrimUp/DrawIndexedPrimUp)
    // wil change the current set IB/VB.  So, we either need to track all of
    // those or set it on every draw call.
    //
    // It turns out that d3d has a fast path when the IB/VB don't change,
    // so setting it on each draw isn't costing us much.
    //

    IFC(SetStreamSource(
        m_pHwVertexBuffer->GetD3DBuffer(),
        uVertexStreamZeroStride
        ));

    //
    // Call DrawPrimitive
    //

    {
        MtSetDefault(Mt(D3DDrawPrimitive));

        IFC(m_pD3DDevice->DrawPrimitive(
            primitiveType,
            uiCurrentVertex,
            uPrimitiveCount
            ));
    }

    UpdateMetrics(uNumVertices, uPrimitiveCount);

Cleanup:
    if (fLockAcquired)
    {
        IGNORE_HR(m_pHwVertexBuffer->Unlock(uNumVertices));
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawVideoToSurface
//
//  Synopsis:
//      Takes an IAVSurfaceRenderer and gives it the D3D device that it can use
//      to Draw the Video frame. In special cases, it draws directly to the
//      backbuffer (CD3DSwapChain) in the specified destination rectangle.
//      Otherwise it uses an intermediate surface which is returned in
//      IWGXBitmapSource. Saves device state before calling to get the frame.
//      Restores state after call is done.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawVideoToSurface(
    __in_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __deref_opt_out_ecount(1) IWGXBitmapSource ** const ppBitmapSource
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice);

    AssertDeviceEntry(*this);

    //
    // Get the next frame here
    // This MUST be the very last HResult altering call in DrawVideoToSurface,
    // since the caller of this assumes that it must call EndRender if this
    // succeeds.
    //
    IFC(pSurfaceRenderer->BeginRender(this, ppBitmapSource));

Cleanup:

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CheckDeviceState
//
//  Synopsis:
//      Checks current state of the device for the hwnd.
//

HRESULT
CD3DDeviceLevel1::CheckDeviceState(
    __in_opt HWND hwnd
    )
{
    HRESULT hr = S_OK;

    //
    // CheckDeviceState call and subsequent logic are only valid with D3D9.L
    //

    if (!m_pD3DDeviceEx)
    {
        IFC(E_NOTIMPL);
    }

    MIL_THR(m_pD3DDeviceEx->CheckDeviceState(hwnd));

    if (hr == S_PRESENT_MODE_CHANGED)
    {
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }
    else if (hr == S_PRESENT_OCCLUDED)
    {
        //
        // In the windowed case we can't keep checking our device state until
        // we're valid again before we render, since all rendering will
        // stop.  If a window is straddling 2 monitors and one side gets
        // occluded the other won't render.  So if we're windowed, we keep
        // rendering as if nothing has happened.
        //

        hr = S_OK;
    }
    else if (   hr == D3DERR_DEVICELOST
             || hr == D3DERR_DEVICEHUNG     // Hw Adapter timed out and has
                                            // been reset by the OS
             || hr == D3DERR_DEVICEREMOVED  // Hw Adapter has been removed
        )
    {
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
        MarkUnusable(false /* This call is already entry protected. */);
    }

Cleanup:
    EventWriteWClientUceCheckDeviceStateInfo(hwnd, hr);

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::WaitForVBlank
//
//  Synopsis:
//      Waits until a vblank occurs on the specified swap chain
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::WaitForVBlank(UINT uSwapChainIndex)
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = WGXERR_NO_HARDWARE_DEVICE;

    if (m_pD3DDeviceEx)
    {
        // The first time WaitForVBlank is called make
        // sure the device driver has support
        // DX has a 100ms timeout in WaitForVBlank so it will return even when
        // the device does not support VBlank event.
        // If it takes >= 100ms to return from the call we
        // know the driver doesn't support waiting on vblank
        if (!m_fHWVBlankTested)
        {
            m_fHWVBlankTested = true;
            ULONGLONG ullStart;
            ULONGLONG ullEnd;
            ULONGLONG ullFreq;
            IFCW32(QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&ullFreq)));
            IFCW32(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&ullStart)));
            MIL_THR(m_pD3DDeviceEx->WaitForVBlank(uSwapChainIndex));
            IFCW32(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&ullEnd)));

            ullEnd -= ullStart;
            ullEnd = (ullEnd * 1000) / ullFreq;

            // Precision on the timeout is not as good as
            // QPC so reduce the test value about one quantum
            // or 75ms
            m_fHWVBlank = SUCCEEDED(hr) && (ullEnd < 75);
            TraceTag((0, "WFVB %I64u %d\n", ullEnd, m_fHWVBlank));
            if (!m_fHWVBlank)
            {
                hr = WGXERR_NO_HARDWARE_DEVICE;
            }
        }
        else if (m_fHWVBlank)
        {
            MIL_THR(m_pD3DDeviceEx->WaitForVBlank(uSwapChainIndex));
        }
        if (FAILED(hr))
        {
            hr = WGXERR_NO_HARDWARE_DEVICE;
        }
    }

  Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ResetMarkers
//
//  Synopsis:
//      Deletes all markers. called when device is lost
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ResetMarkers()
{
    m_ullLastConsumedMarkerId = m_ullLastMarkerId;
    UINT uIndex;
    UINT count = m_rgpMarkerFree.GetCount();
    for (uIndex = 0; uIndex < count ; uIndex++)
    {
        delete m_rgpMarkerFree[uIndex];
    }
    m_rgpMarkerFree.Reset();

    count = m_rgpMarkerActive.GetCount();
    for (uIndex = 0; uIndex < count;  uIndex++)
    {
        delete m_rgpMarkerActive[uIndex];
    }
    m_rgpMarkerActive.Reset();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ConsumePresentMarkers
//
//  Synopsis:
//      Walk through the array of markers testing and freeing markers that have
//      been consumed.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ConsumePresentMarkers(
    bool fForceFlush
    )
{
    HRESULT hr = S_OK;

    Assert(m_rgpMarkerActive.GetCount() < INT_MAX);

    for (int i = static_cast<int>(m_rgpMarkerActive.GetCount()) - 1; i >= 0; i--)
    {
        bool fMarkerConsumed = false;

        Assert(i < static_cast<int>(m_rgpMarkerActive.GetCount()));

        IFC(IsConsumedGPUMarker(
            i,
            fForceFlush,
            &fMarkerConsumed
            ));

        if (fMarkerConsumed)
        {
            IFC(FreeMarkerAndItsPredecessors(i));

            //
            // We've found the most recent marker that's been consumed and freed
            // all those before it.  We don't need to walk through the list anymore.
            //
            break;
        }

        //
        // Once a flush has occurred, another won't give us any more information,
        // so don't pay for its cost more than once.
        //
        if (fForceFlush)
        {
            fForceFlush = false;
            m_uNumSuccessfulPresentsSinceMarkerFlush = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::AdvanceFrame
//
//  Synopsis:
//      If given a frame counter different from the last one then tells resource
//      manager to reclaim frame resources.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::AdvanceFrame(UINT uFrameNumber)
{
    if (m_uFrameNumber != uFrameNumber)
    {
        m_uFrameNumber = uFrameNumber;

        m_resourceManager.EndFrame();
        m_resourceManager.DestroyReleasedResourcesFromLastFrame();
        m_resourceManager.DestroyResources(CD3DResourceManager::WithDelay);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetNumQueuedPresents
//
//  Synopsis:
//      If GPUMarkers are enabled it walks through the array of markers testing
//      if they've been consumed and returns the number of outstanding markers.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;
    bool fForceFlush = (m_uNumSuccessfulPresentsSinceMarkerFlush >= NUM_PRESENTS_BEFORE_GPU_MARKER_FLUSH);

    *puNumQueuedPresents = 0;

    if (   !AreGPUMarkersTested()
        || !AreGPUMarkersEnabled()
        ||  IsLDDMDevice()
            )
    {
        goto Cleanup;
    }

    IFC(ConsumePresentMarkers(
        fForceFlush
        ));

    //
    // If we're over our queue limit, and we didn't flush before, try with a flush.
    //
    if (   m_rgpMarkerActive.GetCount() > 2
        && fForceFlush == false
           )
    {
        IFC(ConsumePresentMarkers(
            true
            ));
    }

    //
    // We only return a queue of presents if we know that markers are working,
    // so if we haven't seen a marker consumed, don't return the size of the array.
    //
    if (HaveGPUMarkersBeenConsumed())
    {
        *puNumQueuedPresents = m_rgpMarkerActive.GetCount();
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::InsertGPUMarker
//
//  Synopsis:
//      Inserts a marker into the GPU command stream The marker ID must be
//      greater than any previous id used.  QPC timer values are expected to be
//      used
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::InsertGPUMarker(
    ULONGLONG ullMarkerId
    )
{
    HRESULT hr = S_OK;
    CGPUMarker *pMarker = NULL;

    //
    // Our Markers should always be in increasing order, but there are occasions
    // when we can receive a marker out of order.  These include coming back from
    // a locked desktop, where QueryPerformanceCounter doesn't behave properly.
    //
    if (ullMarkerId < m_ullLastMarkerId)
    {
        TraceTag((tagError, "Warning: GPUMarker Received out of increasing order.  Ignoring it."));
        goto Cleanup;
    }

    if (!m_pD3DDevice)
    {
        goto Cleanup;
    }

    //
    // If markers haven't been tested do it.
    //
    if (!AreGPUMarkersTested())
    {
        MIL_THR(m_pD3DDevice->CreateQuery(
            D3DQUERYTYPE_EVENT,
            NULL
            ));

        //
        // If we fail in the creation of a query, disable markers.
        //
        if (SUCCEEDED(hr))
        {
            SetGPUMarkersAsEnabled();
        }

        SetGPUMarkersAsTested();
    }

    Assert(AreGPUMarkersTested());

    //
    // If markers are disabled on this device just return
    //
    if (!AreGPUMarkersEnabled())
    {
        m_ullLastMarkerId = ullMarkerId;
        goto Cleanup;
    }

    if (m_rgpMarkerFree.GetCount() > 0)
    {
        UINT uIndex = m_rgpMarkerFree.GetCount()-1;
        pMarker = m_rgpMarkerFree[uIndex];
        m_rgpMarkerFree.SetCount(uIndex);
        pMarker->Reset(ullMarkerId);
    }
    else
    {
        IDirect3DQuery9 *pQuery = NULL;
        IFC(m_pD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pQuery));
        pMarker = new CGPUMarker(pQuery, ullMarkerId);
        IFCOOM(pMarker);
    }

    IFC(pMarker->InsertIntoCommandStream());
    IFC(m_rgpMarkerActive.Add(pMarker));

    pMarker = NULL;

  Cleanup:
    delete pMarker;

    //
    // If we have a backlog of active markers it probably means the hardware isn't reporting
    // the queries properly.  Turn off markers for this device.
    //
    if (m_rgpMarkerActive.GetCount() > GPU_MARKERS_MAX_ARRAY_SIZE)
    {
        TraceTag((tagError, "Backlog of unconsumed markers in the device, turning marking checking off."));

        DisableGPUMarkers();
    }

    if (D3DERR_DEVICELOST == hr
     || D3DERR_NOTAVAILABLE == hr)
    {
        hr = S_OK;
    }

    //
    //  No other HRESULTs are expected, but this code is fairly new,
    // and no harm is done by ignoring them (will just shut ourselves off).
    //
    if (FAILED(hr))
    {
        hr = S_OK;
        DisableGPUMarkers();
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsConsumedGPUMarker
//
//  Synopsis:
//      Determines if the marker has been consumed.
//
//  Returns:
//      If the marker was consumed or we can't confirm it was explicitly
//         NOT consumed *pfMarkersConsumed will be true.  If we can
//         confirm it wasn't consumed, it will be false.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceLevel1::IsConsumedGPUMarker(
    UINT uMarkerIndex,
    bool fFlushMarkers,
    __out_ecount(1) bool *pfMarkerConsumed
    )
{
    HRESULT hr = S_OK;

    Assert(m_rgpMarkerActive.GetCount() > 0);
    Assert(AreGPUMarkersEnabled());
    Assert(uMarkerIndex < m_rgpMarkerActive.GetCount());

    MIL_THR(m_rgpMarkerActive[uMarkerIndex]->CheckStatus(
        fFlushMarkers,
        pfMarkerConsumed
        ));

    //
    // If we recieve device lost then the card is no longer rendering our content.
    // Assume the marker has been consumed, and reinterpret the HRESULT.
    //
    if (hr == D3DERR_DEVICELOST)
    {
        *pfMarkerConsumed = true;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (*pfMarkerConsumed == true)
        {
            SetGPUMarkersAsConsumed();
        }
    }

    //
    //  No other HRESULTs are expected, but this code is fairly new,
    // and no harm is done by ignoring them (will just shut ourselves off).
    //
    if (FAILED(hr))
    {
        hr = S_OK;
        DisableGPUMarkers();
        *pfMarkerConsumed = true;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::FreeMarkerAndItsPredecessors
//
//  Synopsis:
//      Move the marker, and all markers below it into the Free list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::FreeMarkerAndItsPredecessors(
    UINT uIndex
    )
{
    HRESULT hr = S_OK;

    UINT cConsumed = uIndex+1;
    UINT cNew  = m_rgpMarkerActive.GetCount() - cConsumed;
    UINT ui;

    // this marker was consumed
    // Update the last consumed id
    m_ullLastConsumedMarkerId = m_rgpMarkerActive[uIndex]->GetId();

    // remove it and all those with lower ids to the free list
    for (ui = 0; ui < cConsumed; ui++)
    {
        IFC(m_rgpMarkerFree.Add(m_rgpMarkerActive[ui]));
        m_rgpMarkerActive[ui] = NULL;
    }
    // Shift the unconsumed entries to the begining.
    for (ui = 0; ui < cNew; ui++)
    {
        m_rgpMarkerActive[ui] = m_rgpMarkerActive[cConsumed + ui];
    }
    m_rgpMarkerActive.SetCount(cNew);

Cleanup:
    RRETURN(hr);
}

void
CD3DDeviceLevel1::InitializeIMediaDeviceConsumer(
    __in_ecount(1) IMediaDeviceConsumer *pIMediaDeviceConsumer
    )
{
    pIMediaDeviceConsumer->SetIDirect3DDevice9(m_pD3DDevice);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Function:
//      DbgInjectDIE
//
//  Synopsis:
//      If tagInjectDIE is enabled sometimes change S_OK to
//      D3DERR_DRIVERINTERNALERROR for fault testing purposes.
//
//------------------------------------------------------------------------------
void
DbgInjectDIE(
    __inout_ecount(1) HRESULT * const pHR
    )
{
    if (IsTagEnabled(tagInjectDIE))
    {
        const unsigned c_uInjectionRate = 200;
        static unsigned s_count = 0;

        if (    SUCCEEDED(*pHR)
                && !(++s_count%c_uInjectionRate))
        {
            TraceTag((tagError, "MIL-HW: Injecting D3DERR_DRIVERINTERNALERROR!."));
            *pHR = D3DERR_DRIVERINTERNALERROR;
        }
    }
}
#endif


//+-----------------------------------------------------------------------------
//
//  Function:
//      PrepareShaderEffectPipeline
//
//  Synopsis:
//      Configures the device for running a pixel shader effect.
//      (See ImageEffect).
//
//      Returns a scratch vertex buffer to use for the shader pipeline with
//      this device.
//
//------------------------------------------------------------------------------


HRESULT
CD3DDeviceLevel1::PrepareShaderEffectPipeline(bool useVS30)
{
    HRESULT hr = S_OK;
    CD3DVertexXYZDUV2* pVertices = NULL;

    // We have a vs_2_0 and vs_3_0 copy of the vertex shader in order to work around an issue with
    // ATI cards.  ATI cards would fail to render ps_3_0 effects when using a vs_2_0 vertex shader.
    // Both vertex shaders are identical outside of the version number.
    // We still need a vs_2_0 version for machines with only shader model 2.0 support.
    IDirect3DVertexShader9 *pVertexShader = NULL;
    if (useVS30)
    {
        if (m_pEffectPipelineVertexShader30 == NULL)
        {
            IFC(CreateVertexShaderFromResource(VS_ShaderEffects30, &m_pEffectPipelineVertexShader30));
        }

        pVertexShader = m_pEffectPipelineVertexShader30;
    }
    else
    {
        if (m_pEffectPipelineVertexShader20 == NULL)
        {
            IFC(CreateVertexShaderFromResource(VS_ShaderEffects20, &m_pEffectPipelineVertexShader20));
        }

        pVertexShader = m_pEffectPipelineVertexShader20;
    }

    if (m_pEffectPipelineVertexBuffer == NULL)
    {
        IFC(CreateVertexBuffer(
            4 * sizeof(CD3DVertexXYZDUV2),
            D3DUSAGE_WRITEONLY,
            CD3DVertexXYZDUV2::Format,
            D3DPOOL_DEFAULT,
            &m_pEffectPipelineVertexBuffer));
    }

    IFC(SetVertexShader(pVertexShader));
    IFC(SetFVF(CD3DVertexXYZDUV2::Format));

    IFC(m_pEffectPipelineVertexBuffer->Lock( 0, 0, reinterpret_cast<void**>(&pVertices), 0));

    //   1---3
    //   | \ |
    //   |  \|
    //   0---2
    pVertices[0].SetXYUV0(0, 1.0f, 0, 1.0f);
    pVertices[1].SetXYUV0(0, 0, 0, 0);
    pVertices[2].SetXYUV0(1.0f, 1.0f, 1.0f, 1.0f);
    pVertices[3].SetXYUV0(1.0f, 0, 1.0f, 0);

    hr = m_pEffectPipelineVertexBuffer->Unlock();
    pVertices = NULL;
    IFC(hr);

    IFC(SetStreamSource(m_pEffectPipelineVertexBuffer, sizeof(CD3DVertexXYZDUV2)));

Cleanup:
    if (pVertices)
    {
        IGNORE_HR(m_pEffectPipelineVertexBuffer->Unlock());
    }

    RRETURN(HandleDIE(hr));
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      SetPassThroughPixelShader
//
//  Synopsis:
//      Sets the shader effect pipeline pixel shader to the
//      default implementation (pass-through).
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetPassThroughPixelShader()
{
    HRESULT hr = S_OK;

    if (m_pEffectPipelinePassThroughPixelShader == NULL)
    {
        IFC(CreatePixelShaderFromResource(PS_PassThroughShaderEffect, &m_pEffectPipelinePassThroughPixelShader));
    }

    IFC(SetPixelShader(m_pEffectPipelinePassThroughPixelShader));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CheckDeviceFloatingPointRenderTargetFormat
//
//  Synopsis:
//      Returns true if the device supports A32R32B32G32F for render target
//      textures.  Needed by built-in blur effect.
//
//------------------------------------------------------------------------------
bool
CD3DDeviceLevel1::Is128BitFPTextureSupported() const
{
    return (m_fmtSupportFor128bppPRGBAFloat == MilPixelFormat::PRGBA128bppFloat);
}









