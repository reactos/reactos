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
//      Contains CD3DDeviceLevel1 implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CD3DSwapChain;
class CD3DDeviceVertexBuffer;
class CD3DDeviceIndexBuffer;
interface IMediaDeviceConsumer;

MtExtern(CD3DDeviceLevel1);

#include "uce\global.h"

#define NUM_PRESENTS_BEFORE_GPU_MARKER_FLUSH 3

#define GPUMARKER_FLAGS_MARKERS_ENABLED             0x1
#define GPUMARKER_FLAGS_MARKERS_TESTED              0x10
#define GPUMARKER_FLAGS_MARKER_CONSUMED             0x100

//------------------------------------------------------------------------------
//
// D3DPOOL_MANAGED is not allowed with IDirect3DDevice9Ex. This define is the back
// door for MilCore to use managed pool behavior until MilCore stops using
// managed resources with IDirect3DDevice9Ex.
//
//------------------------------------------------------------------------------

#define D3DPOOL_MANAGED_INTERNAL ((D3DPOOL)6)

//+-----------------------------------------------------------------------------
//
//  Defines:
//      Device creation failure tracking
//
//------------------------------------------------------------------------------

#if DBG==1
#define TRACE_DEVICECREATE_FAILURE(adapter, msg,  hr) \
    CD3DDeviceLevel1::DbgTraceDeviceCreationFailure(adapter, msg, hr);
#else
#define TRACE_DEVICECREATE_FAILURE(adapter, msg,  hr)
#endif

// Forward declaration for class used for monitor GPU progress
class CGPUMarker;
//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DDeviceLevel1
//
//  Synopsis:
//      Abstracts the core D3D device to provide the following functionality:
//          0. Assert on invalid input
//          1. Restrict access to methods of IDirect3DDevice9 to those
//             available on level 1 graphics cards. (Level1 is the base support
//             we require to hw accelerate)
//          2. Provide correct information for GetDeviceCaps
//          3. Centralize resource creation so that it can be tracked.
//             Tracking created resources is important for responding to mode
//             changes
//          4. Respond to mode changes on present call
//          5. Provide testing functionality for determining if a graphics
//             card meets the level1 criteria for hw acceleration
//
//      Note that CD3DDeviceLevel1 is base class for more capable hw devices.
//
//------------------------------------------------------------------------------

class CD3DDeviceLevel1
      // costly shared data is stored here since some of this data has
      // device affinity.  It also holds the cache index, which needs
      // to be released after all resources and therefore should be the
      // last to be destroyed and making it the first base class.
    : public CHwSurfaceRenderTargetSharedData,
      public CD3DRenderState,
      public CMILPoolResource
{
private:

    struct TargetFormatTestStatus {
        HRESULT hrTest;
        HRESULT hrTestGetDC;
    };

public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DDeviceLevel1));

    static HRESULT Create(
        __inout_ecount(1) IDirect3DDevice9 *pID3DDevice,
        __in_ecount(1) const CDisplay *pPrimaryDisplay,
        __in_ecount(1) IMILPoolManager *pManager,
        DWORD dwBehaviorFlags,
        __deref_out_ecount(1) CD3DDeviceLevel1 ** const ppDevice
        );

    virtual ~CD3DDeviceLevel1();

    //+-------------------------------------------------------------------------
    //
    //  Members:
    //      Enter and Leave
    //
    //  Synopsis:
    //      Thread protection marking methods
    //
    //      Call Enter when device is about to used in a way that requires
    //      exclusive access and Leave when leaving that context.  This is most
    //      commonly done when handling a drawing routine, which the caller is
    //      required to provide protection for.
    //
    //--------------------------------------------------------------------------

    void Enter();
    void Leave();

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsProtected
    //
    //  Synopsis:
    //      Return true if this thread is protected
    //
    //--------------------------------------------------------------------------

    bool IsProtected(
        bool fForceEntryConfirmation    // Ignore lack of multithreaded usage
                                        // flag when checking for protection.
                                        // This is to be used by internal
                                        // callers that may violate caller
                                        // provided protection.
        ) const;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsEntered
    //
    //  Synopsis:
    //      Return true if this thread has been marked/entered as protected
    //
    //--------------------------------------------------------------------------

    bool IsEntered() const;

    bool IsEnsuringCorrectMultithreadedRendering() const
        { return m_csDeviceEntry.IsValid(); }


    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      ENTER_DEVICE_FOR_SCOPE
    //
    //  Synopsis:
    //      Provide simple mechanism to mark device as protected for the
    //      duration of scope
    //
    //--------------------------------------------------------------------------

    #define ENTER_DEVICE_FOR_SCOPE(Device) \
        CGuard<CD3DDeviceLevel1> oDeviceEntry(Device)

    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      ENTER_USE_CONTEXT_FOR_SCOPE
    //
    //  Synopsis:
    //      Provide simple mechanism to enter/exit a use context in the scope
    //
    //--------------------------------------------------------------------------

    #define ENTER_USE_CONTEXT_FOR_SCOPE(Device) \
        CD3DUseContextGuard oUseContext(Device)


    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      AssertDeviceEntry
    //
    //  Synopsis:
    //      Provide simple mechanism to check device protection is taken for the
    //      duration of scope
    //
    //--------------------------------------------------------------------------

    #define AssertDeviceEntry(Device) \
        AssertEntry((Device).m_oDbgEntryCheck); \
        Assert((Device).IsProtected(false /* Do not force entry check */))

    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      AssertNoDeviceEntry
    //
    //  Synopsis:
    //      Check device protection is not been taken at this time nor for any
    //      other thread for the duration of scope; however this thread may
    //      enter the device protection from this scope
    //
    //--------------------------------------------------------------------------

    #define AssertNoDeviceEntry(Device) \
        AssertEntry((Device).m_oDbgEntryCheck); \
        Assert(!((Device).IsEntered()))

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      InitializeIMediaDeviceConsumer
    //
    //  Synopsis:
    //      Provide a clean mechanism for video code to break the abstraction
    //      and get at the underlying device. This is necessary because WMP has
    //      no knowledge of CD3DDeviceLevel1
    //
    //--------------------------------------------------------------------------
    void InitializeIMediaDeviceConsumer(
        __in_ecount(1) IMediaDeviceConsumer *pIMediaDeviceConsumer
        );

    HRESULT CheckRenderTargetFormat(
        D3DFORMAT fmtRenderTarget,
        __deref_opt_out_ecount(1) HRESULT const ** const pphrTestGetDC = NULL
        );

    void AssertRenderFormatIsTestedSuccessfully(
        D3DFORMAT fmtRenderTarget
        );

    HRESULT Set3DTransforms(
        __in_ecount(1) CMILMatrix const *pWorldTransform3D,
        __in_ecount(1) CMILMatrix const *pViewTransform3D,
        __in_ecount(1) CMILMatrix const *pProjectionTransform3D,
        __in_ecount(1) CMatrix<CoordinateSpace::Projection3D,CoordinateSpace::Device> const &matHomogeneousTo2DDevice
        );

    HRESULT SetRenderTarget(__in_ecount(1) CD3DSurface *pD3DSurface);

    void ReleaseUseOfRenderTarget(
        __in_ecount(1) const CD3DSurface *pD3DRenderTargetSurface
        );

    HRESULT GetSwapChain(
        UINT uGroupAdapterOrdinal,
        __deref_out_ecount(1) CD3DSwapChain ** const ppSwapChain
        );

    void MarkUnusable(
        bool fMayBeMultithreadedCall
        );

    HRESULT CreateAdditionalSwapChain(
        __in_ecount_opt(1) CMILDeviceContext const *pMILDC,
        __in_ecount(1) D3DPRESENT_PARAMETERS *pD3DPresentParams,
        __deref_out_ecount(1) CD3DSwapChain ** const ppSwapChain
        );

    HRESULT CreateRenderTarget(
        UINT uiWidth,
        UINT uiHeight,
        D3DFORMAT d3dfmtSurface,
        D3DMULTISAMPLE_TYPE d3dMultiSampleType,
        DWORD dwMultisampleQuality,
        bool fLockable,
        __deref_out_ecount(1) CD3DSurface ** const ppD3DSurface
        );

    HRESULT CreateRenderTargetUntracked(
        UINT uiWidth,
        UINT uiHeight,
        D3DFORMAT d3dfmtSurface,
        D3DMULTISAMPLE_TYPE d3dMultiSampleType,
        DWORD dwMultisampleQuality,
        bool fLockable,
        __deref_out_ecount(1) IDirect3DSurface9 ** const ppD3DSurface
        );

    HRESULT GetRenderTargetData(
        __in_ecount(1) IDirect3DSurface9 *pSourceSurface,
        __in_ecount(1) IDirect3DSurface9 *pDestinationSurface
        );

    HRESULT CreateVertexBuffer(
        UINT Length,
        DWORD Usage,
        DWORD FVF,
        D3DPOOL Pool,
        __deref_out_ecount(1) IDirect3DVertexBuffer9 ** const ppVertexBuffer
        );

    HRESULT CreateIndexBuffer(
        UINT Length,
        DWORD Usage,
        D3DFORMAT Format,
        D3DPOOL Pool,
        __deref_out_ecount(1) IDirect3DIndexBuffer9 ** const ppIndexBuffer
        );

    HRESULT CreateStateBlock(
        D3DSTATEBLOCKTYPE d3dStateBlockType,
        __deref_out_ecount(1) IDirect3DStateBlock9 ** const ppStateBlock
        );

    HRESULT ComposeRects(
        __in_ecount(1) IDirect3DSurface9* pSource,
        __inout_ecount(1) IDirect3DSurface9* pDestination,
        __in_ecount(1) IDirect3DVertexBuffer9* pSrcRectDescriptors,
        UINT NumRects,
        __in_ecount(1) IDirect3DVertexBuffer9* pDstRectDescriptors,
        D3DCOMPOSERECTSOP Operation
        );

    HRESULT CreateTexture(
        __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
        UINT uLevels,
        __deref_out_ecount(1) IDirect3DTexture9 ** const ppD3DTexture,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle = NULL
        );

    HRESULT CreatePixelShaderFromResource(
        UINT uResourceId,
        __deref_out_ecount(1) IDirect3DPixelShader9 ** const ppPixelShader
        );

    HRESULT CreateVertexShaderFromResource(
        UINT uResourceId,
        __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppVertexShader
        );
        
    HRESULT CompilePipelineVertexShader(
        __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
        UINT cbHLSLSource,
        __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppVertexShader
        );

    HRESULT CompilePipelinePixelShader(
        __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
        UINT cbHLSLSource,
        __deref_out_ecount(1) IDirect3DPixelShader9 ** const ppPixelShader
        );

    HRESULT CreateVertexShader(
        __in const DWORD *pdwfnVertexShader,
        __deref_out_ecount(1) IDirect3DVertexShader9 ** const ppOutShader
        );

    HRESULT CreatePixelShader(
        __in const DWORD *pdwfnPixelShader,
        __deref_out_ecount(1) IDirect3DPixelShader9 **const ppOutShader
        );


    HRESULT CreateLockableTexture(
        __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
        __deref_out_ecount(1) CD3DLockableTexture ** const ppLockableTexture
        );

    HRESULT CreateSysMemUpdateSurface(
        UINT uWidth,
        UINT uHeight,
        D3DFORMAT fmtTexture,
        __in_xcount_opt(uWidth * uHeight * D3DFormatSize(fmtTexture)) void *pvPixels,
        __deref_out_ecount(1) IDirect3DSurface9 ** const ppD3DSysMemSurface
        );

    HRESULT CreateSysMemReferenceTexture(
        __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
        __in_xcount(
            pSurfDesc->Width * pSurfDesc->Height
            * D3DFormatSize(pSurfDesc->Format)
            ) void *pvPixels,
        __deref_out_ecount(1) IDirect3DTexture9 ** const ppD3DSysMemTexture
        );

    HRESULT UpdateSurface(
        __in_ecount(1) IDirect3DSurface9 *pD3DSysMemSrcSurface,
        __in_ecount_opt(1) const RECT *pSourceRect,
        __inout_ecount(1) IDirect3DSurface9 *pD3DPoolDefaultDestSurface,
        __in_ecount_opt(1) const POINT *pDestPoint
        );

    HRESULT UpdateTexture(
        __in_ecount(1) IDirect3DTexture9 *pD3DSysMemSrcTexture,
        __inout_ecount(1) IDirect3DTexture9 *pD3DPoolDefaultDestTexture
        );

    HRESULT StretchRect(
        __in_ecount(1) CD3DSurface *pSourceSurface,
        __in_ecount_opt(1) const RECT *pSourceRect,
        __inout_ecount(1) CD3DSurface *pDestSurface,
        __in_ecount_opt(1) const RECT *pDestRect,
        D3DTEXTUREFILTERTYPE Filter
        )
    {
        return StretchRect(
            pSourceSurface,
            pSourceRect,
            pDestSurface->ID3DSurface(),
            pDestRect,
            Filter
            );
    }

    HRESULT StretchRect(
        __in_ecount(1) CD3DSurface *pSourceSurface,
        __in_ecount_opt(1) const RECT *pSourceRect,
        __inout_ecount(1) IDirect3DSurface9 *pDestSurface,
        __in_ecount_opt(1) const RECT *pDestRect,
        D3DTEXTUREFILTERTYPE Filter
        );

    HRESULT CreateDepthBuffer(
        UINT uWidth,
        UINT uHeight,
        D3DMULTISAMPLE_TYPE MultisampleType,
        __deref_out_ecount(1) CD3DSurface ** const ppSurface
        );

    void CleanupFreedResources();

    HRESULT Present(
        __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const *pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        DWORD dwD3DPresentFlags
        );

    HRESULT SetTexture(
        DWORD dwTextureStage,
        __inout_ecount_opt(1) CD3DTexture *pD3DTexture
        );

    HRESULT
    SetD3DTexture(
        DWORD dwTextureStage,
        __in_ecount_opt(1) IDirect3DTexture9 *pD3DTexture
        );

    HRESULT DisableTextureTransform(DWORD dwTextureStage);

    HRESULT SetLinearPalette();

    HRESULT SetDepthStencilSurface(
        __in_ecount_opt(1) CD3DSurface *pD3DSurface
        );

    void ReleaseUseOfDepthStencilSurface(
        __in_ecount_opt(1) CD3DSurface *pSurface
        );

    HRESULT Clear(
        DWORD dwCount,
        __in_ecount_opt(dwCount) const D3DRECT* pRects,
        DWORD dwFlags,
        D3DCOLOR d3dColor,
        FLOAT rZValue,
        int iStencilValue
        );

    HRESULT ColorFill(
        __inout_ecount(1) IDirect3DSurface9 *pSurface,
        __in_ecount_opt(1) const RECT *pRect,
        D3DCOLOR color
        );

    CD3DResourceManager *GetResourceManager()
    {
        return &m_resourceManager;
    }

    HRESULT RenderTexture(
        __in_ecount(1) CD3DTexture *pD3DTexture,
        __in_ecount(1) const MilPointAndSizeL &rcDestination,
        TextureBlendMode blendMode
        );

    HRESULT CopyD3DTexture(
        __in_ecount(1) IDirect3DTexture9 *pD3DSourceTexture,
        __inout_ecount(1) IDirect3DTexture9 *pD3DDestinationTexture
        );

    HRESULT GetMinimalTextureDesc(
        __inout_ecount(1) D3DSURFACE_DESC *pd3dsd,
        bool fPalUsesAlpha,
        DWORD dwFlags
        ) const
    {
        return ::GetMinimalTextureDesc(m_pD3DDevice, m_d3ddm.Format, &m_caps, pd3dsd, fPalUsesAlpha, dwFlags);
    }

    HRESULT GetSupportedTextureFormat(
        MilPixelFormat::Enum fmtBitmapSource,          // Current format of bitmap
        MilPixelFormat::Enum fmtDestinationSurface,    // Format of surface onto
                                                 // which the bitmap will be
                                                 // drawn
        bool fForceAlpha,
        __out_ecount(1) MilPixelFormat::Enum &fmtTextureSource    // Format of the
                                                            // texture to hold
                                                            // the bitmap
        ) const;

    D3DMULTISAMPLE_TYPE GetSupportedMultisampleType(
        MilPixelFormat::Enum fmtDestinationSurface        // Format of target surface
        ) const;

    HRESULT SetClipRect(
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip
        );

    void GetClipRect(
        __out_ecount(1) MilPointAndSizeL * const prcClipRect
        ) const;

    // Our Adapter LUIDs come from D3D when an extended device is avaible
    // (WDDM). On XP we generate them ourselves, so this method is safe to use
    // for device identity regardless of driver model. However, in XPDM, these
    // auto-generated LUIDs will be different for every display set created.
    LUID GetD3DAdapterLUID() const { return m_luidD3DAdapter; }

    //
    // device capability accessors
    //

    TierType GetTier() const { return m_Tier; }
    bool IsLDDMDevice() const {return HwCaps::IsLDDMDevice(m_caps);}
    bool IsHALDevice() const {return m_caps.DeviceType == D3DDEVTYPE_HAL;}
    bool IsSWDevice() const {return HwCaps::IsSWDevice(m_caps);}
    BOOL CanAutoGenMipMap() const {return m_caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP;}

    bool CanStretchRectGenMipMap() const { return (m_caps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) != 0; }
    bool CanStretchRectFromTextures() const { return (m_caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) != 0; }
    bool CanMaskColorChannels() const { return HwCaps::CanMaskColorChannels(m_caps); }
    bool CanHandleBlendFactor() const { return HwCaps::CanHandleBlendFactor(m_caps); }
    bool SupportsD3DFMT_A8() const { return m_fSupportsD3DFMT_A8; }
    bool SupportsD3DFMT_P8() const { return m_fSupportsD3DFMT_P8; }
    bool SupportsD3DFMT_L8() const { return m_fSupportsD3DFMT_L8; }
    bool SupportsBorderColor() const { return (m_caps.TextureAddressCaps & D3DPTADDRESSCAPS_BORDER) != 0; }
    bool SupportsAnisotropicFiltering() const { return GetMaxDesiredAnisotropicFilterLevel() > 1; }
    bool SupportsNonPow2Conditionally() const { return (m_caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) && (m_caps.TextureCaps & D3DPTEXTURECAPS_POW2); }
    bool SupportsNonPow2Unconditionally() const { return !(m_caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) && !(m_caps.TextureCaps & D3DPTEXTURECAPS_POW2); }

    bool ShouldAttemptMultisample() const 
    {
        // If a previous attempt to render multisampled failed, do not continue to attempt multisampling
        return !m_fMultisampleFailed &&
            (GetTier() >= MIL_TIER(2,0));
    }

    void SetMultisampleFailed() { m_fMultisampleFailed = true; }

    bool SupportsTextureCap(DWORD dwTextureCap) const
    {
        Assert(((dwTextureCap-1) & dwTextureCap) == 0); // Check for single cap
        return (m_caps.TextureCaps & dwTextureCap) != 0;
    }
    DWORD GetMaxStreams() const {return m_caps.MaxStreams;}
    DWORD GetMaxTextureBlendStages() const {return m_caps.MaxTextureBlendStages;}
    DWORD GetMaxSimultaneousTextures() const {return m_caps.MaxSimultaneousTextures;}
    DWORD GetMaxTextureWidth() const {return m_caps.MaxTextureWidth;}
    DWORD GetMaxTextureHeight() const {return m_caps.MaxTextureHeight;}
    DWORD GetVertexShaderVersion() const {return m_caps.VertexShaderVersion;}
    DWORD GetPixelShaderVersion() const {return m_caps.PixelShaderVersion;}
    __range(1,UINT_MAX) DWORD GetMaxDesiredAnisotropicFilterLevel() const 
    {
        Assert(m_caps.MaxAnisotropy > 0);

        return (m_caps.MaxAnisotropy > 4) ? 4 : m_caps.MaxAnisotropy;
    }

    __out_ecount(1) CHwD3DVertexBuffer *Get3DVertexBuffer()
        { return m_pHwVertexBuffer; }

    __out_ecount(1) CHwD3DIndexBuffer *Get3DIndexBuffer()
        { return m_pHwIndexBuffer; }

    BOOL SupportsScissorRect() const {return m_caps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST;}

    BOOL SupportsLinearTosRGBPresentation() const {return m_caps.Caps3 & D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION;}

    __out_ecount(1) const FilterMode *GetSupportedAnistotropicFilterMode() const { return m_pCachedAnisoFilterMode; }

#if DBG==1
    bool IsPureDevice() const { return (m_dwD3DBehaviorFlags & D3DCREATE_PUREDEVICE) != 0; }
#endif

    bool IsExtendedDevice() const { return (m_pD3DDeviceEx != NULL); }

    D3DPOOL GetManagedPool() const {return m_ManagedPool;}

    CD3DGlyphBank* GetGlyphBank()
    {
        return &m_glyphBank;
    }

    HRESULT StartPrimitive(CD3DVertexBufferDUV2 **ppOutBuffer)
    {
        *ppOutBuffer = &m_vbBufferDUV2;

        m_vbBufferDUV2.Clear();

        return SetFVF(CD3DVertexXYZDUV2::Format);
    }

    HRESULT StartPrimitive(CD3DVertexBufferDUV6 **ppOutBuffer)
    {
        m_vbBufferDUV6.Clear();

        *ppOutBuffer = &m_vbBufferDUV6;
        return SetFVF(CD3DVertexXYZDUV6::Format);
    }

    HRESULT StartPrimitive(CD3DVertexBufferXYZNDSUV4 **ppOutBuffer)
    {
        m_vbBufferXYZNDSUV4.Clear();

        *ppOutBuffer = &m_vbBufferXYZNDSUV4;
        return SetFVF(CD3DVertexXYZNDSUV4::Format);
    }

    HRESULT EndPrimitiveFan(__in_ecount(1) CD3DVertexBuffer *pBuffer)
    {
        return FlushBufferFan(pBuffer);
    }

    CHwTVertexBuffer<CD3DVertexXYZDUV2> *GetVB_XYZDUV2()
    {
        return &m_vBufferXYZDUV2;
    }

    CHwTVertexBuffer<CD3DVertexXYZDUV8> *GetVB_XYZRHWDUV8()
    {
        return &m_vBufferXYZRHWDUV8;
    }

    HRESULT DrawBox(
        __in_ecount(1) const MilPointAndSize3F *pbox,
        const D3DFILLMODE d3dFillMode,
        const DWORD dwColor
        );

    HRESULT DrawIndexedTriangleList(
        UINT uBaseVertexIndex,
        UINT uMinIndex,
        UINT cVertices,
        UINT uStartIndex,
        UINT cPrimitives
        );

    HRESULT DrawTriangleList(
        UINT uStartVertex,
        UINT cPrimitives
        );

    HRESULT DrawTriangleStrip(
        UINT uStartVertex,
        UINT cPrimitives
        );

    HRESULT DrawIndexedTriangleListUP(
        __range(1, UINT_MAX) UINT uNumVertices,
        __range(1, UINT_MAX) UINT uPrimitiveCount,
        __in_xcount(sizeof(WORD) * uPrimitiveCount * 3) const WORD* pIndexData,
        __in const void* pVertexStreamZeroData,
        __range(1, UINT_MAX) UINT uVertexStreamZeroStride
        );

    HRESULT DrawPrimitiveUP(
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
        );

    HRESULT DrawVideoToSurface(
        __in_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __deref_opt_out_ecount(1) IWGXBitmapSource ** const ppBitmapSource
        );

    HRESULT CheckDeviceState(
        __in_opt HWND hwnd
        );

    BOOL SupportsGetScanLine() const {return m_caps.Caps & D3DCAPS_READ_SCANLINE;}

    HRESULT GetRasterStatus(
        UINT uSwapChain,
        D3DRASTER_STATUS *pRasterStatus
        )
    {
        Assert(SupportsGetScanLine());
        return m_pD3DDevice->GetRasterStatus(uSwapChain, pRasterStatus);
    }

    HRESULT WaitForVBlank(
        UINT uSwapChain
        );

    HRESULT GetNumQueuedPresents(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    // Advance the frame for the device.
    // IMPORTANT to call this with increasing frame numbers because
    // it handles releasing HW resources used in the last *TWO* frames.
    void AdvanceFrame(UINT uFrameNumber);

    void Use(__in_ecount(1) const CD3DResource &refResource)
    {
        m_resourceManager.Use(refResource);
    }

    UINT EnterUseContext()
    {
        return m_resourceManager.EnterUseContext();
    }

    void ExitUseContext(UINT uDepth)
    {
        m_resourceManager.ExitUseContext(uDepth);
    }

    bool IsInAUseContext() const
    {
        return m_resourceManager.IsInAUseContext();
    }

#if DBG==1
    static void DbgTraceDeviceCreationFailure(UINT uAdapter, __in PCSTR szMessage, HRESULT hrError);
    const IDirect3DDevice9* DbgGetID3DDevice9() const { return m_pD3DDevice; }
#endif

private:
    CD3DDeviceLevel1(
        __in_ecount(1) IMILPoolManager *pManager,
        DWORD dwBehaviorFlags
        );

    HRESULT Init(
        __inout_ecount(1) IDirect3DDevice9 *pID3DDevice,
        __in_ecount(1) CDisplay const *pDisplay
        );

    void GatherSupportedTextureFormats(
        __in_ecount(1) IDirect3D9 *pID3D
        );

    D3DMULTISAMPLE_TYPE GetMaxMultisampleTypeWithDepthSupport(
        __in_ecount(1) IDirect3D9 *pd3d9,
        D3DFORMAT d3dfmtTarget,
        D3DFORMAT d3dfmtDepth,
        D3DMULTISAMPLE_TYPE MaxMultisampleType
        ) const;

    void GatherSupportedMultisampleTypes(
        __in_ecount(1) IDirect3D9 *pd3d9
        );

    HRESULT GetRenderTargetFormatTestEntry(
        D3DFORMAT fmtRenderTarget,
        __deref_out_ecount(1) TargetFormatTestStatus * &pFormatTestEntry
        );

    HRESULT TestRenderTargetFormat(
        D3DFORMAT fmtRenderTarget,
        __inout_ecount(1) TargetFormatTestStatus *pFormatTestEntry
        );

    HRESULT TestLevel1Device(
        );

    void TestGetDC(
        __inout_ecount(1) CD3DSurface * const pD3DSurface,
        __inout_ecount(1) TargetFormatTestStatus * const pFormatTestEntry
        );

    HRESULT CheckBadDeviceDrivers(
        __in_ecount(1) const CDisplay *pDisplay
        );

    HRESULT BeginScene();
    HRESULT EndScene();

    HRESULT SetSurfaceToClippingMatrix(
        __in_ecount(1) const MilPointAndSizeL *prcViewport
        );

    HRESULT FlushBufferFan(
        __in_ecount(1) CD3DVertexBuffer *
        );

    HRESULT DrawLargePrimitiveUP(
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
        );

    void UpdateMetrics(
        UINT uNumVertices,
        UINT uNumPrimitives);

    // Destroys all outstanding markers.  Called on device error
    void ResetMarkers();

    // Insert a marker into the rendering stream so
    // we can determine when the GPU has processed up
    // to this point
    // The marker ID must be greater than any previous
    // id used.  QPC timer values are expected to be used
    HRESULT InsertGPUMarker(
        ULONGLONG ullMarkerId
        );

    // Tests whether all marker with lower ids have been consumed
    HRESULT IsConsumedGPUMarker(
        UINT uMarkerIndex,
        bool fFlushMarkers,
        __out_ecount(1) bool *pfMarkerConsumed
        );

    HRESULT FreeMarkerAndItsPredecessors(UINT uIndex);

    MIL_FORCEINLINE HRESULT HandleDIE(HRESULT hr)
    {
        if (hr == D3DERR_DRIVERINTERNALERROR)
        {
            // Return WGXERR_DISPLAYSTATEINVALID to upstream callers.
            // Present will pick up the D3DERR_DRIVERINTERNALERROR and
            // tear down the device.
            m_hrDisplayInvalid = D3DERR_DRIVERINTERNALERROR;
            hr = WGXERR_DISPLAYSTATEINVALID;
        }

        return hr;
    }

    HRESULT PresentWithGDI(
        __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const * pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        __out_ecount(1) bool *pfPresentProcessed
        );

    HRESULT PresentWithD3D(
        __inout_ecount(1) IDirect3DSwapChain9 *pD3DSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const *pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        DWORD dwD3DPresentFlags,
        __out_ecount(1) bool *pfPresentProcessed
        );

    HRESULT HandlePresentFailure(
         __in_ecount(1) CMILDeviceContext const *pMILDC,
        HRESULT hr);


    HRESULT ConsumePresentMarkers(
        bool fForceFlush
        );

    void SetGPUMarkersAsEnabled()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKERS_ENABLED;
    }

    void DisableGPUMarkers()
    {
        m_dwGPUMarkerFlags &= (~GPUMARKER_FLAGS_MARKERS_ENABLED);
        ResetMarkers();
    }

    bool AreGPUMarkersEnabled() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKERS_ENABLED) != 0;
    }

    void SetGPUMarkersAsTested()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKERS_TESTED;
    }

    bool AreGPUMarkersTested() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKERS_TESTED) != 0;
    }

    void SetGPUMarkersAsConsumed()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKER_CONSUMED;
    }

    bool HaveGPUMarkersBeenConsumed() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKER_CONSUMED) != 0;
    }

private:
    UINT m_cEntry;
    DWORD m_dwThreadId;

    // D3D objects
    IDirect3DDevice9 *m_pD3DDevice;
    IDirect3DDevice9Ex *m_pD3DDeviceEx;

    TargetFormatTestStatus m_RenderTargetTestStatusX8R8G8B8;
    TargetFormatTestStatus m_RenderTargetTestStatusA8R8G8B8;
    TargetFormatTestStatus m_RenderTargetTestStatusA2R10G10B10;

    // Active render target set on device
    CD3DSurface *m_pCurrentRenderTargetNoRef;
    // Hint for what depth buffer to release when releasing a RT from use.
    // It is meaningless unless m_pCurrentRenderTargetNoRef is not NULL.
    CD3DSurface *m_pDepthStencilBufferForCurrentRTNoRef;


    LUID m_luidD3DAdapter;
    D3DCAPS9 m_caps;
    TierType m_Tier;
    DWORD const m_dwD3DBehaviorFlags;

    const FilterMode *m_pCachedAnisoFilterMode; // What level of aniso this device supports.

    // This is false initially, and can be set on failure to prevent future attempts to multisample.
    bool m_fMultisampleFailed;

    // caps that are missed in m_caps:
    bool m_fSupportsD3DFMT_A8;
    bool m_fSupportsD3DFMT_P8;
    bool m_fSupportsD3DFMT_L8;

    MilPixelFormat::Enum m_fmtSupportFor128bppPRGBAFloat;
    MilPixelFormat::Enum m_fmtSupportFor128bppRGBFloat;
    MilPixelFormat::Enum m_fmtSupportFor32bppBGR101010;
    MilPixelFormat::Enum m_fmtSupportFor32bppPBGRA;
    MilPixelFormat::Enum m_fmtSupportFor32bppBGR;

    D3DMULTISAMPLE_TYPE m_multisampleTypeFor32bppBGR;
    D3DMULTISAMPLE_TYPE m_multisampleTypeFor32bppPBGRA;
    D3DMULTISAMPLE_TYPE m_multisampleTypeFor32bppBGR101010;

    bool m_fInScene;

    // Set to true once the device has been marked unusable and then given a
    // chance to destroy resources and notify its manager.
    bool m_fDeviceLostProcessed;

    // HRESULT indicating whether the display is invalid
    HRESULT m_hrDisplayInvalid;

    // Adaptor display mode
    D3DDISPLAYMODEEX m_d3ddm;   // ::Size's value indicates the valid fields

    // Current render target desc
    D3DSURFACE_DESC m_desc;

    // Vertex Buffers
    CD3DVertexBufferDUV2 m_vbBufferDUV2;
    CD3DVertexBufferDUV6 m_vbBufferDUV6;
    CD3DVertexBufferXYZNDSUV4 m_vbBufferXYZNDSUV4;

    // Matrix applied to surface coordinates to translate to homogeneous clip
    // coordinates.  It is always based on the viewport; so, if the viewport
    // changes this probably needs updated.
    CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC>
        m_matSurfaceToClip;


    CHwTVertexBuffer<CD3DVertexXYZDUV2>     m_vBufferXYZDUV2;
    CHwTVertexBuffer<CD3DVertexXYZDUV8>     m_vBufferXYZRHWDUV8;

    //CHwTVertexBuffer<CD3DVertexXYZDUV6>     m_vBufferXYZDUV6;
    //CHwTVertexBuffer<CD3DVertexXYZNDSUV4>   m_vBufferXYZNDSUV4;

    //
    // Custom VB/IB management
    //

    CHwD3DIndexBuffer *m_pHwIndexBuffer;
    CHwD3DVertexBuffer *m_pHwVertexBuffer;

    // Additional services for tracking resources and render state
    CD3DResourceManager m_resourceManager;

    //
    // Critical section in case we're using the RGBRast device.  We have to
    // protect it from being used by multiple threads simultaneously.
    //
    CCriticalSection m_csDeviceEntry;

    // Glyph rendering
    CD3DGlyphBank m_glyphBank;

    // Dummy back buffer for use when there is no current render target
    IDirect3DSurface9 *m_pD3DDummyBackBuffer;

    // Per frame metrics
    DWORD m_dwMetricsVerticesPerFrame;
    DWORD m_dwMetricsTrianglesPerFrame;

    D3DPOOL m_ManagedPool;


    // Last marker id specified
    ULONGLONG m_ullLastMarkerId;

    // last marker ID that was consumed
    ULONGLONG m_ullLastConsumedMarkerId;

    // Last frame number given to AdvanceFrame, when this changes
    // we free resources from last two frames as appropriate.
    UINT m_uFrameNumber;

    // Active and free marker lists
    DynArray<CGPUMarker *> m_rgpMarkerActive;
    DynArray<CGPUMarker *> m_rgpMarkerFree;

    UINT m_uNumSuccessfulPresentsSinceMarkerFlush;

    DWORD m_dwGPUMarkerFlags;

    //  Is there  HW VBlank support
    bool m_fHWVBlankTested;
    bool m_fHWVBlank;

    UINT m_presentFailureWindowMessage;

#if DBG_STEP_RENDERING
public:
    void DbgBeginStepRenderingPresent() {
        Assert(!m_fDbgInStepRenderingPresent);
        m_fDbgInStepRenderingPresent = true;
    }
    void DbgEndStepRenderingPresent() {
        Assert(m_fDbgInStepRenderingPresent);
        m_fDbgInStepRenderingPresent = false;
    }
    bool DbgInStepRenderingPresent() const {
        return m_fDbgInStepRenderingPresent;
    }

    HRESULT DbgSaveSurface(
        __in_ecount(1) CD3DSurface *pD3DSurface,
        __in_ecount(1) const MilPointAndSizeL &rcSave
        );

    HRESULT DbgRestoreSurface(
        __inout_ecount(1) CD3DSurface *pD3DSurface,
        __in_ecount(1) const MilPointAndSizeL &rcRestore
        );

    BOOL DbgCanShrinkRectLinear() const {
        return (m_caps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR);
    }

private:
    bool m_fDbgInStepRenderingPresent;
    CD3DSurface *m_pDbgSaveSurface;

#endif DBG_STEP_RENDERING

#if DBG
public:
    // per-frame debug info logging
    CD3DLog m_log;

    // Device thread protection checker
    mutable CAssertEntry    m_oDbgEntryCheck;

private:

    // Rendering stats
    CD3DStats m_d3dStats;
#endif DBG


public: 
    // Shader effect pipeline pereration.
    HRESULT PrepareShaderEffectPipeline(bool useVS30);
    HRESULT SetPassThroughPixelShader();
    HRESULT SetRenderTargetForEffectPipeline(__in_ecount(1) CD3DSurface *pD3DSurface);
    bool Is128BitFPTextureSupported() const;
private: 

    IDirect3DVertexShader9* m_pEffectPipelineVertexShader20; // Vertex shader 2.0 for shader effects pipeline.
    IDirect3DVertexShader9* m_pEffectPipelineVertexShader30; // Vertex shader 3.0 for shader effects pipeline.
    IDirect3DVertexBuffer9* m_pEffectPipelineVertexBuffer; // Scratch buffer for the shader effects pipeline. 
    IDirect3DPixelShader9* m_pEffectPipelinePassThroughPixelShader; // Pass-through pixel shader for shader effects pipelines.
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateMetrics()
//
//  Synopsis:
//      Update metrics if tracking them
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CD3DDeviceLevel1::UpdateMetrics(UINT uNumVertices, UINT uNumPrimitives)
{
    if (g_pMediaControl)
    {
        m_dwMetricsVerticesPerFrame += uNumVertices;
        m_dwMetricsTrianglesPerFrame += uNumPrimitives;
    }
}

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
extern int c_dbgPixelZoomModeScale;
extern bool DbgIsPixelZoomMode();
#endif




