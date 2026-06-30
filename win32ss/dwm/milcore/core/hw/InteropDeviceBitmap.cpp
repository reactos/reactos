// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//+-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CInteropDeviceBitmap, MILRender, "CInteropDeviceBitmap");

//+-----------------------------------------------------------------------------
//
//  WPFGFX Exported Function:
//      InteropDeviceBitmap_Create
//
//  Synopsis:
//      Validates params and creates a bitmap
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
HRESULT WINAPI
InteropDeviceBitmap_Create(
    __in IUnknown *pIUserD3DResource,
    __in_range(0, DBL_MAX) double dpiX,
    __in_range(0, DBL_MAX) double dpiY,
    UINT uVersion,
    __in CInteropDeviceBitmap::FrontBufferAvailableCallbackPtr pfnAvailable,
    BOOL isSoftwareFallbackEnabled,
    __deref_out CInteropDeviceBitmap **ppInteropDeviceBitmap,
    __out UINT *puWidth,
    __out UINT *puHeight
    )
{
    HRESULT hr = S_OK;

    CInteropDeviceBitmap *pInteropDeviceBitmap = NULL;

    CHECKPTR(pIUserD3DResource);
    CHECKPTR(pfnAvailable);
    CHECKPTR(ppInteropDeviceBitmap);
    CHECKPTR(puWidth);
    CHECKPTR(puHeight);

    if (dpiX < 0 || dpiY < 0)
    {
        IFC(E_INVALIDARG);
    }

    IFC(CInteropDeviceBitmap::Create(
        pIUserD3DResource,
        dpiX,
        dpiY,
        uVersion,
        pfnAvailable,
        !!isSoftwareFallbackEnabled,
        &pInteropDeviceBitmap
        ));

    IFC(pInteropDeviceBitmap->GetSize(puWidth, puHeight));

    *ppInteropDeviceBitmap = pInteropDeviceBitmap;  // steal ref
    pInteropDeviceBitmap = NULL;
        
Cleanup:
    ReleaseInterface(pInteropDeviceBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  WPFGFX Exported Function:
//      InteropDeviceBitmap_Detach
//
//  Synopsis:
//      Calls Detach() on the bitmap
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
void WINAPI
InteropDeviceBitmap_Detach(
    __in CInteropDeviceBitmap *pInteropDeviceBitmap
    )
{
    if (pInteropDeviceBitmap)
    {
        pInteropDeviceBitmap->Detach();
    }
}

//+-----------------------------------------------------------------------------
//
//  WPFGFX Exported Function:
//      InteropDeviceBitmap_AddDirtyRect
//
//  Synopsis:
//      Validates dimensions and calls AddUserDirtyRect() on the bitmap
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
HRESULT WINAPI
InteropDeviceBitmap_AddDirtyRect(
    int iX, 
    int iY, 
    int iW, 
    int iH,
    __in CInteropDeviceBitmap *pInteropDeviceBitmap
    )
{
    HRESULT hr = S_OK;

    CHECKPTR(pInteropDeviceBitmap);

    if (iX < 0 || iY < 0 || iW < 0 || iH < 0)
    {
        IFC(E_INVALIDARG);
    }
    else if (iW > 0 || iH > 0)
    {
        // CMilRectU will add these ints together, but addition is safe because 
        // INT_MAX + INT_MAX = UINT_MAX - 1
        CMilRectU rc(
            static_cast<UINT>(iX),
            static_cast<UINT>(iY),
            static_cast<UINT>(iW),
            static_cast<UINT>(iH),
            XYWH_Parameters
            );
        IFC(pInteropDeviceBitmap->AddUserDirtyRect(rc));
    }
    // else silently succeed for an empty rect
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  WPFGFX Exported Function:
//      InteropDeviceBitmap_GetAsSoftwareBitmap
//
//  Synopsis:
//      Forwards to GetAsSoftwareBitmap on the given bitmap
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------

HRESULT WINAPI
InteropDeviceBitmap_GetAsSoftwareBitmap(
    __in CInteropDeviceBitmap *pInteropDeviceBitmap,
    __deref_out IWICBitmapSource **ppIWICBitmapSource
    )
{
    HRESULT hr = S_OK;

    CHECKPTR(ppIWICBitmapSource);
    CHECKPTR(pInteropDeviceBitmap);

    IFC(pInteropDeviceBitmap->GetAsSoftwareBitmap(ppIWICBitmapSource));
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::GetUpdateMethod
//
//  Synopsis:
//      Helper method that determines the optimal update method for the front
//      buffer.
//
//      Requirements for surface sharing
//        1) Created on IDirect3DDevice9Ex
//        2) D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES as that's how we'll copy 
//           across the two devices
//        3) D3DCAPS2_CANSHARERESOURCE. This should always be true for 9Ex but
//           we'll double check to be sure.
//
//      Requirements for bit blitting
//        1) GetDC must work (which means lockable w/ pixel format support)
//        2) D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES as that's how we'll copy 
//           to a texture once we're on the destination device
//
//      BitBlt is much slower than software copy on Vista WDDM so we'll only
//      allow it on pre-Vista OSes.
//
//------------------------------------------------------------------------------

/* static */ CInteropDeviceBitmap::FrontBufferUpdateMethod 
CInteropDeviceBitmap::GetUpdateMethod(
    __in IDirect3DDevice9 *pID3DDevice,
    __in_opt const IDirect3DDevice9Ex *pID3DDeviceEx,
    __in IDirect3DSurface9 *pID3DSurface
    )
{   
    HRESULT hr = S_OK;
    FrontBufferUpdateMethod method = Software;

    D3DCAPS9 caps;
    IFC(pID3DDevice->GetDeviceCaps(&caps));

    if ((caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) == D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES)
    {
        if (pID3DDeviceEx && (caps.Caps2 & D3DCAPS2_CANSHARERESOURCE) == D3DCAPS2_CANSHARERESOURCE)
        {
            method = SharedSurface;
        }
        else if (!WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater())
        {
            HDC hdc;
            IFC(pID3DSurface->GetDC(&hdc));
            IFC(pID3DSurface->ReleaseDC(hdc));

            // Failure of GetDC or ReleaseDC will skip this assignment
            method = BitBlt;
        }
    }

Cleanup:
    return method;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::Create
//
//  Synopsis:
//      Validates user surface and creates a CInteropDeviceBitmap
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
HRESULT 
CInteropDeviceBitmap::Create(
    __in IUnknown *pIUserSurface,
    __in_range(0, DBL_MAX) double dpiX,
    __in_range(0, DBL_MAX) double dpiY,
    UINT uVersion,
    __in FrontBufferAvailableCallbackPtr pfnAvailable,
    bool isSoftwareFallbackEnabled,
    __deref_out CInteropDeviceBitmap **ppInteropDeviceBitmap
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pID3DUserSurface = NULL;
    IDirect3DDevice9 *pID3DUserDevice = NULL;
    IDirect3DDevice9Ex *pID3DUserDeviceEx = NULL;
    CD3DDeviceManager *pDeviceManager = CD3DDeviceManager::Get();
    D3DSURFACE_DESC desc;
    CInteropDeviceBitmap *pInteropDeviceBitmap = NULL;
        
    IFC(pIUserSurface->QueryInterface(
        __uuidof(IDirect3DSurface9), 
        reinterpret_cast<void **>(&pID3DUserSurface)
        ));
    IFC(pID3DUserSurface->GetDesc(&desc));

    // Ensuring that the surface isn't bigger than SURFACE_RECT_MAX will allow us to cast unsigned 
    // bounds rects to signed bound rects safely 
    C_ASSERT(SURFACE_RECT_MAX <= INT_MAX);
    if (desc.Width > SURFACE_RECT_MAX || desc.Height > SURFACE_RECT_MAX)
    {
        IFC(WGXERR_D3DI_INVALIDSURFACESIZE);
    }

    if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8)
    {
        IFC(WGXERR_UNSUPPORTEDPIXELFORMAT);
    }
    
    if (   (desc.Usage & D3DUSAGE_DEPTHSTENCIL) == D3DUSAGE_DEPTHSTENCIL
        || (desc.Usage & D3DUSAGE_RENDERTARGET) != D3DUSAGE_RENDERTARGET)
    {
        IFC(WGXERR_D3DI_INVALIDSURFACEUSAGE);
    }
    
    if (desc.Pool != D3DPOOL_DEFAULT)
    {
        IFC(WGXERR_D3DI_INVALIDSURFACEPOOL);
    }

    // Since we've QI'd to surface, this should always be true
    Assert(desc.Type == D3DRTYPE_SURFACE);

    IFC(pID3DUserSurface->GetDevice(&pID3DUserDevice));

    // Check to see if the user's device is dead. On 9Ex, TestCooperativeLevel always returns
    // S_OK so we must call CheckDeviceState instead
    if (SUCCEEDED(pID3DUserDevice->QueryInterface(
        __uuidof(IDirect3DDevice9Ex),
        reinterpret_cast<void **>(&pID3DUserDeviceEx)
        )))
    {    
        if (FAILED(pID3DUserDeviceEx->CheckDeviceState(/* hWindow = */ NULL)))
        {
            IFC(WGXERR_D3DI_INVALIDSURFACEDEVICE);
        }
    }
    else
    {
        if (FAILED(pID3DUserDevice->TestCooperativeLevel()))
        {
            IFC(WGXERR_D3DI_INVALIDSURFACEDEVICE);
        }
    }

    FrontBufferUpdateMethod method = 
        CInteropDeviceBitmap::GetUpdateMethod(pID3DUserDevice, pID3DUserDeviceEx, pID3DUserSurface);

    //
    // MSAA is only allowed in shared surface mode because it's the only way it will be fast. GetDC
    // will not work with MSAA. GetRenderTargetData does not work on MSAA surfaces but our front 
    // buffer will not be multisampled as it is a RT texture. So we only need to worry about this
    // when reading the back buffer to software.
    //
    if (   (desc.MultiSampleType != D3DMULTISAMPLE_NONE || desc.MultiSampleQuality != 0) 
        && method != SharedSurface)
    {
        IFC(WGXERR_D3DI_INVALIDANTIALIASINGSETTINGS);
    }

    //
    //     Adapter numbers aren't always equivalent across device objects
    //      We want to create our front buffer on the same video card as the user's back buffer to
    //      guarantee that sharing and bitblt work. The only way to do this consistently on XDDM
    //      and WDDM is to use the adapter id number. Unfortunately, if the user created his device
    //      a long time ago and the adapter order has since changed then his "adapter #x" might 
    //      not map to our "adapter #x." 
    //
    //      However, this is unlikely and sharing and bitblt will continue to work as long as it's 
    //      the same video card. I think this could only really be a problem for the multi video 
    //      card situation but then we'll copy through software anyway.
    //
    D3DDEVICE_CREATION_PARAMETERS oCreationParams;
    IFC(pID3DUserDevice->GetCreationParameters(&oCreationParams));

    pInteropDeviceBitmap = new CInteropDeviceBitmap(
        uVersion,
        pfnAvailable,
        isSoftwareFallbackEnabled,
        desc.Width,
        desc.Height,
        D3DFormatToPixelFormat(desc.Format, /* fPremultiplied = */ TRUE),
        method,
        oCreationParams.AdapterOrdinal,
        pID3DUserSurface
        );
    IFCOOM(pInteropDeviceBitmap);
    pInteropDeviceBitmap->AddRef();

    IFC(pInteropDeviceBitmap->SetResolution(dpiX, dpiY));
    IFC(pInteropDeviceBitmap->m_cs.Init());
    IFC(pDeviceManager->AddAdapterStatusListener(pInteropDeviceBitmap));

    *ppInteropDeviceBitmap = pInteropDeviceBitmap;  // steal ref
    pInteropDeviceBitmap = NULL;

Cleanup:
    ReleaseInterface(pInteropDeviceBitmap);
    ReleaseInterface(pID3DUserDevice);
    ReleaseInterface(pID3DUserDeviceEx);
    // CInteropDeviceBitmap's ctor took a ref and we QI'd, so Release once
    ReleaseInterface(pID3DUserSurface);

    CD3DDeviceManager::Release();
 
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::CInteropDeviceBitmap
//
//  Synopsis:
//      Ctor
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
CInteropDeviceBitmap::CInteropDeviceBitmap(
    UINT uVersion,
    __in FrontBufferAvailableCallbackPtr pfnAvailable,
    bool isSoftwareFallbackEnabled,
    __in_range(0, SURFACE_RECT_MAX) UINT uWidth,
    __in_range(0, SURFACE_RECT_MAX) UINT uHeight,
    MilPixelFormat::Enum fmtPixel,
    FrontBufferUpdateMethod oUpdateMethod,
    UINT uAdapter,
    __in IDirect3DSurface9 *pUserSurface
    )
    : 
    CDeviceBitmap(uWidth, uHeight, fmtPixel),
    m_pIUserSurface(pUserSurface), m_cUserDirtyRects(0), m_uVersion(uVersion),
    m_pfnAvailable(pfnAvailable), m_fIsSoftwareFallbackEnabled(isSoftwareFallbackEnabled), 
    m_oUpdateMethod(oUpdateMethod), m_fIsHwRenderingDisabled(false),
    m_uAdapter(uAdapter), m_pISoftwareBitmap(NULL)
{
    m_pIUserSurface->AddRef();

    ZeroMemory(&m_luidDevice, sizeof(m_luidDevice));
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::~CInteropDeviceBitmap
//
//  Synopsis:
//      Dtor
//
//  Thread Affinity:
//      UI or Render thread
//
//------------------------------------------------------------------------------
CInteropDeviceBitmap::~CInteropDeviceBitmap()
{
    CD3DDeviceManager *pDeviceManager = CD3DDeviceManager::Get();
    pDeviceManager->RemoveAdapterStatusListener(this);
    CD3DDeviceManager::Release();

    ReleaseInterface(m_pISoftwareBitmap);
    ReleaseInterface(m_pIUserSurface); 
    m_cs.DeInit();
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::Detach
//
//  Synopsis:
//      Disassociates this bitmap from its managed D3DImage
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
void
CInteropDeviceBitmap::Detach()
{
    CGuard<CCriticalSection> oGuard(m_cs);

    //
    // Release now so the user can immediately reclaim his surface. Unlike 
    // NotifyAdapterStatus, we don't delete our realizations because we still
    // need to display the front buffer
    //
    ReleaseInterface(m_pIUserSurface);

    //
    // We won't remove ourselves from listening to the device manager until 
    // destruction, but nulling out the callback will prevent managed code
    // from being notified
    //
    m_pfnAvailable = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::NotifyAdapterStatus
//
//  Synopsis:
//      Takes lock because it's public and forwards to internal implementation
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
void 
CInteropDeviceBitmap::NotifyAdapterStatus(UINT uAdapter, bool fIsValid)
{
    CGuard<CCriticalSection> oGuard(m_cs);

    NotifyAdapterStatusInternal(uAdapter, fIsValid);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::NotifyAdapterStatusInternal
//
//  Synopsis:
//      Calls back to the associated D3DImage on the UI thread to update it on
//      the status of the adapter
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
void 
CInteropDeviceBitmap::NotifyAdapterStatusInternal(UINT uAdapter, bool fIsValid)
{
    if (uAdapter == m_uAdapter)
    {
        if (!fIsValid)
        {
            // We must release the user's surface so he can recover his device. If software fallback is enabled, we'll
            // release the surface when the user explicitly calls SetBackBuffer on D3DImage again with a null value. 
            // It's the user's responsibility to check for device loss.
            if (m_fIsSoftwareFallbackEnabled)
            {
                // Fall back to software and keep the user's surface around.
                IGNORE_HR(CopyToSoftwareBitmap(&m_pISoftwareBitmap));
            }
            else
            {
                ReleaseInterface(m_pIUserSurface);
            }

            //
            //  Release ALL hw realizations
            //
            //  We may have hw realizations on other devices dependent upon our primary color
            //  source (front buffer) via shared handle or BitBlt. Since we lost the front buffer,
            //  release every color source.
            //
            ReleaseResources();

            // Never gets set to false again because next SetBackBuffer will create new bitmap,
            // unless software fallback is enabled.
            m_fIsHwRenderingDisabled = true;
        }
        else if (m_fIsSoftwareFallbackEnabled)
        {
            m_fIsHwRenderingDisabled = false;
        }

        if (m_pfnAvailable)
        {
            (*m_pfnAvailable)(fIsValid, m_uVersion);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::AddUserDirtyRect
//
//  Synopsis:
//      Adds "rc" to the list of rects to be copied from the user's buffer to 
//      our buffer during present.
//
//      IMPORTANT: More than 5 rects and we'll union them together for performance reasons.
//                 This means the area between dirty rects MUST be valid.
//
//      NOTE: This is called AddUserDirtyRect to prevent shadowing of 
//            IWGXBitmap::AddDirtyRect
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------
HRESULT
CInteropDeviceBitmap::AddUserDirtyRect(__in const CMilRectU &rc)
{
    CGuard<CCriticalSection> oGuard(m_cs);

    HRESULT hr = S_OK;
    const CMilRectU rcBounds(0, 0, m_nWidth, m_nHeight, XYWH_Parameters);
    
    if (!rcBounds.DoesContain(rc))
    {
        IFC(E_INVALIDARG);
    }

    for (UINT i = 0; i < m_cUserDirtyRects; ++i)
    {
        if (m_rgUserDirtyRects[i].DoesContain(rc))
        {
            // Dirty rect already in list, we're done
            goto Cleanup;
        }
    }    
    
    if (m_cUserDirtyRects >= c_maxBitmapDirtyListSize)
    {
        // Collapse dirty list to a single large rect (including new rect)
        while (m_cUserDirtyRects > 1)
        {
            m_rgUserDirtyRects[0].Union(m_rgUserDirtyRects[--m_cUserDirtyRects]);
        }
        m_rgUserDirtyRects[0].Union(rc);

        Assert(m_cUserDirtyRects == 1);
    }
    else
    { 
        m_rgUserDirtyRects[m_cUserDirtyRects++] = rc;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::Present
//
//  Synopsis:
//      Copies the user dirty rects from the user's back buffer to our front
//      buffer. Also lazily creates our front buffer
//
//      IMPORTANT: This should only be called by the D3DImage resource because 
//                 the resource synchronizes with the UI thread
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
HRESULT
CInteropDeviceBitmap::Present()
{
    CGuard<CCriticalSection> oGuard(m_cs);

    HRESULT hr = S_OK;

    Assert(m_cUserDirtyRects > 0);

    CleanupInvalidSource();

    //
    // The user may have ignored our rendering disabled notification and continued 
    // to render so let's silently fail until we get a valid surface again.
    //
    if (!m_fIsHwRenderingDisabled && m_pIUserSurface)
    {
        if (!m_poDeviceBitmapInfo)
        {
            IFC(CreateFrontBuffer());
        }
        
        IFC(UpdateFrontBuffer());

        if (!m_fIsHwRenderingDisabled)
        {
            // Hardware copy succeeded. Make sure we release the last software bitmap if one exists.
            // (The value of m_fIsHwRenderingDisabled may change during the CreateFrontBuffer call.)
            ReleaseInterface(m_pISoftwareBitmap);
        }
    }

Cleanup:
    //
    //    Present failures
    //
    // Present could fail for a lot of reasons, but we don't want to bring down the app if 
    // device lost happens during this process. Since we don't trust D3D to return the right
    // HRESULT at device lost, we'll ignore all D3D failures. This may hide bugs, but we can enable 
    // breaking with a regkey if we have a repro.
    //
    // OOVM is not something that'll happen from device lost, but we don't have a good way to
    // message that back to the user on the UI thread. If we return OOVM now it'll crash the app.
    // So we'll ignore it and hopefully OOVM will happen again during render and then composition
    // can fallback to SW.
    //
    if (IsD3DFailure(hr))
    {
        MilUnexpectedError(hr, TEXT("CInteropDeviceBitmap::Present D3D failure"));

        hr = S_OK;
    }

    if (m_pIUserSurface && m_fIsHwRenderingDisabled && m_fIsSoftwareFallbackEnabled)
    {
        // If software fallback is enabled and the front buffer is unavailable, we try to fall back to
        // software no matter what.
        IGNORE_HR(CopyToSoftwareBitmap(&m_pISoftwareBitmap));
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::CreateFrontBuffer
//
//  Synopsis:
//      Helper method to create our front buffer on the same adapter as the 
//      user's surface
//
//  Returns:      
//      S_OK if the front buffer is created or we haven't created our device
//      for the given adapter yet
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
HRESULT
CInteropDeviceBitmap::CreateFrontBuffer()
{
    HRESULT hr = S_OK;

    const CDisplay *pDisplay = NULL;
    CD3DDeviceLevel1 *pD3DDevice = NULL;
    CD3DDeviceManager *pDeviceManager = CD3DDeviceManager::Get();
    CHwBitmapCache *pCache = NULL;
    HANDLE hSharedHandle = NULL;
    CHwDeviceBitmapColorSource *pDBCS = NULL;
    const CMilRectU rcBounds(0, 0, m_nWidth, m_nHeight, XYWH_Parameters);
    bool fSucceeded = false;
    
    Assert(!m_poDeviceBitmapInfo);

    //
    // We don't want GetD3DDeviceAndPresentParams() to create a device. A device won't exist 
    // if composition hasn't created render targets or if it's lost. In that case, notify the
    // D3DImage and silently fail. Later, when we end up creating our device, we'll get
    // the Notify, send true to the D3DImage, and the user will set a new back buffer
    //
    if (pDeviceManager->DoesWindowedHwDeviceExist(m_uAdapter))
    {
        IFC(GetDisplayFromUserDevice(&pDisplay));

        IFC(pDeviceManager->GetD3DDeviceAndPresentParams(
            GetDesktopWindow(),   // hwnd doesn't matter for non-fullscreen
            MilRTInitialization::Default,
            pDisplay,
            D3DDEVTYPE_HAL,
            &pD3DDevice,
            NULL,
            NULL
            ));

        {
            ENTER_DEVICE_FOR_SCOPE(*pD3DDevice);

            IFC(CHwBitmapCache::GetCache(
                pD3DDevice,
                this,
                /* pICacheAlternate = */ NULL,
                /* fSetResourceRequired = */ true,
                &pCache
                ));

            switch (m_oUpdateMethod)
            {
                case SharedSurface:
                    IFC(pCache->CreateSharedColorSource(
                        m_PixelFormat,
                        rcBounds,
                        pDBCS,
                        &hSharedHandle
                        ));
                    break;

                case BitBlt:
                    IFC(pCache->CreateBitBltColorSource(
                        m_PixelFormat,
                        rcBounds,
                        false,          // fIsDependent
                        pDBCS
                        ));
                    break;

                // For software copy, we could go with either color source, but we'll go with
                // a standard DBCS because it's more efficient (not lockable)
                case Software:
                    IFC(pCache->CreateSharedColorSource(
                        m_PixelFormat,
                        rcBounds,
                        pDBCS,
                        NULL            // pSharedHandle
                        ));
                    break;

                default:
                    RIP("unhandled enum value");
                    break;
            }

            IFC(SetDeviceBitmapColorSource(
                hSharedHandle,
                pDBCS
                ));

            m_luidDevice = pD3DDevice->GetD3DAdapterLUID();
        }

        fSucceeded = true;
    }

Cleanup:
    ReleaseInterface(pD3DDevice);
    ReleaseInterfaceNoNULL(pDisplay);  
    ReleaseInterfaceNoNULL(pCache);
    ReleaseInterfaceNoNULL(pDBCS);
    CD3DDeviceManager::Release();

    NotifyAdapterStatusInternal(m_uAdapter, fSucceeded);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::GetDisplayFromUserDevice
//
//  Synopsis:
//      Helper function thet extracts the matching display for the user's device
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
HRESULT
CInteropDeviceBitmap::GetDisplayFromUserDevice(
    __deref_out const CDisplay **ppDisplay
    )
{ 
    Assert(m_pIUserSurface);

    HRESULT hr = S_OK;
    IDirect3DDevice9 *pID3DUserDevice = NULL;
    IDirect3D9 *pID3DUserObject = NULL;
    const CDisplaySet *pDisplaySet = NULL; 

    IFC(m_pIUserSurface->GetDevice(&pID3DUserDevice));
    IFC(pID3DUserDevice->GetDirect3D(&pID3DUserObject));

    HMONITOR hMon = pID3DUserObject->GetAdapterMonitor(m_uAdapter);

    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);
    UINT uDisplayIndex;
    IFC(pDisplaySet->GetDisplayIndexFromMonitor(hMon, uDisplayIndex));

    IFC(pDisplaySet->GetDisplay(uDisplayIndex, ppDisplay));

Cleanup:    
    ReleaseInterface(pID3DUserDevice);
    ReleaseInterface(pID3DUserObject);
    ReleaseInterface(pDisplaySet);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::UpdateFrontBuffer
//
//  Synopsis:
//      Helper function thet extracts the matching display for the user's device
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------

HRESULT 
CInteropDeviceBitmap::UpdateFrontBuffer()
{
    Assert(m_cUserDirtyRects > 0);
    
    HRESULT hr = S_OK;

    if (m_poDeviceBitmapInfo)
    {
        Assert(m_poDeviceBitmapInfo->m_pbcs);
        Assert(m_poDeviceBitmapInfo->m_pbcs->IsValid());
        
        IFC(m_poDeviceBitmapInfo->m_pbcs->UpdateSurface(
            m_cUserDirtyRects,
            m_rgUserDirtyRects,
            m_pIUserSurface
            ));

        // Update bitmap validity and dirty it for cross adapter purposes
        for (UINT i = 0; i < m_cUserDirtyRects; ++i)
        {
            AddUpdateRect(m_rgUserDirtyRects[i]);
        }

        // Dirty rect processing complete
        m_cUserDirtyRects = 0;
    }

Cleanup:    
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::TryCreateDependentDeviceColorSource
//
//  Synopsis:
//      Potentially creates a color source on the new device that will be 
//      dependent upon our front buffer.
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------
bool 
CInteropDeviceBitmap::TryCreateDependentDeviceColorSource(
    __in const LUID &luidNewDevice,
    __in CHwBitmapCache *pNewCache
    )
{
    CGuard<CCriticalSection> oGuard(m_cs);
    
    HRESULT hr = S_OK;
    CHwDeviceBitmapColorSource *pDBCS = NULL;
    const CMilRectU rcBounds(0, 0, m_nWidth, m_nHeight, XYWH_Parameters);
    bool fCreated = false;

    CleanupInvalidSource();

    if (m_poDeviceBitmapInfo && !m_fIsHwRenderingDisabled)
    {
        switch (m_oUpdateMethod)
        {
            case SharedSurface:
                if (luidNewDevice == m_luidDevice)
                {
                    Assert(m_poDeviceBitmapInfo->m_hShared);

                    IFC(pNewCache->CreateSharedColorSource(
                        m_PixelFormat,
                        rcBounds,
                        pDBCS,
                        &m_poDeviceBitmapInfo->m_hShared
                        ));

                    // Since we're sharing a handle with the up-to-date front buffer, 
                    // everything is valid and updates happen automatically
                    pDBCS->UpdateValidBounds(rcBounds);

                    fCreated = true;
                }
                break;

            case BitBlt:
                // BitBlt works across different video cards so no cross-device check needed
                IFC(pNewCache->CreateBitBltColorSource(
                    m_PixelFormat,
                    rcBounds,
                    true,       // fIsDependent
                    pDBCS
                    ));

                fCreated = true;

                // The new color source will be updated in 
                // CHwBitBltDeviceBitmapColorSource::Realize()
                break;

            case Software:
                //
                // Future Consideration:   WDDM could share surfaces here
                //
                // Just because software copy happens from back to front doesn't mean software
                // copy has to happen cross monitor. We could share the front buffer's handle onto
                // the other monitor, assuming same LUID.
                //
                // See below comment about why doing nothing here is okay.
                //
                break;

            default:
                RIP("unhandled case");
                break;
        }
    }

    //
    //  We will have not created a dependent color source if...
    //      1) WDDM and different video cards
    //      2) Software copying is our only option
    //      3) Something went wrong
    //
    //  Failure to create a dependent color source will result in a normal CHwBitmapColorSource
    //  being created and it will pull from the front buffer through software
    //

Cleanup:
    ReleaseInterfaceNoNULL(pDBCS);

    return fCreated;
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::GetAsSoftwareBitmap
//
//  Synopsis:
//      Creates a software bitmap from the back buffer's contents. If we can't
//      read for any reason, return failure and the UI thread will deal with
//      it as it wishes.
//
//  Thread Affinity:
//      UI thread
//
//------------------------------------------------------------------------------

HRESULT
CInteropDeviceBitmap::GetAsSoftwareBitmap(
    __deref_out IWICBitmapSource **ppIBitmapSource
    )
{
    CGuard<CCriticalSection> oGuard(m_cs);
    
    HRESULT hr = S_OK;
    IWGXBitmap *pBitmap = NULL;

    IFC(CopyToSoftwareBitmap(&pBitmap));

    // If reading the back buffer failed, we could read from the front buffer but this
    // is called from the UI thread and there are serious concerns about thread safety

    IFC(WrapInClosestBitmapInterface(pBitmap, ppIBitmapSource));

Cleanup:
    ReleaseInterface(pBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::CopyToSoftwareBitmap
//
//  Synopsis:
//      Copies to a software bitmap from the back buffer's contents. Creates a 
//      new bitmap if *ppBitmap is NULL, otherwise copies to the existing bitmap.
//
//  Thread Affinity:
//      UI or render thread
//
//------------------------------------------------------------------------------

HRESULT
CInteropDeviceBitmap::CopyToSoftwareBitmap(__deref_inout IWGXBitmap **ppBitmap)
{
    CGuard<CCriticalSection> oGuard(m_cs);

    HRESULT hr = S_OK;
    CSystemMemoryBitmap *pSystemMemoryBitmap = NULL;
    IWGXBitmap *pBitmap = NULL; // Does not get released. Not an actual ref.
    IWGXBitmapLock *pLock = NULL;
    const CMilRectU rcFull(0, 0, m_nWidth, m_nHeight, XYWH_Parameters);

    if (!m_pIUserSurface)
    {
        IFC(E_FAIL);
    }

    pBitmap = *ppBitmap;
    if (pBitmap == NULL)
    {
        // Allocate a new bitmap.
        IFC(CSystemMemoryBitmap::Create(
            m_nWidth,
            m_nHeight,
            m_PixelFormat,
            /* fClear = */ FALSE,  // No clear needed as we're about to fill the entire bitmap
            /* fDynamic = */ FALSE,
            &pSystemMemoryBitmap
            ));

        pBitmap = pSystemMemoryBitmap;
    }

    IFC(pBitmap->Lock(NULL, MilBitmapLock::Write, &pLock));

    BYTE *pData;
    UINT cbBitmap;
    UINT cbStride;
    IFC(pLock->GetStride(&cbStride));
    IFC(pLock->GetDataPointer(&cbBitmap, &pData));

    Assert(cbBitmap == cbStride * m_nHeight);

    IFC(ReadRenderTargetIntoSysMemBuffer(
        m_pIUserSurface,
        rcFull,
        m_PixelFormat,
        cbStride,
        DBG_ANALYSIS_PARAM_COMMA(cbBitmap)
        pData
        ));

    if (pSystemMemoryBitmap != NULL)
    {
        *ppBitmap = pSystemMemoryBitmap;
        pSystemMemoryBitmap = NULL;
    }

Cleanup:
    ReleaseInterface(pLock);
    ReleaseInterface(pSystemMemoryBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CInteropDeviceBitmap::GetSoftwareBitmapSource
//
//  Synopsis:
//      Gets the last software copy of the user's surface if it exists.
//
//  Thread Affinity:
//      Render thread
//
//------------------------------------------------------------------------------

HRESULT
CInteropDeviceBitmap::GetSoftwareBitmapSource(__deref_out IWGXBitmapSource **ppBitmapSource)
{
    CGuard<CCriticalSection> oGuard(m_cs);

    HRESULT hr = S_OK;

    *ppBitmapSource = NULL;

    if (m_pISoftwareBitmap != NULL)
    {
        IFC(m_pISoftwareBitmap->QueryInterface(IID_IWGXBitmapSource, reinterpret_cast<void **>(ppBitmapSource)));
    }

Cleanup:
    RRETURN(hr);
}

