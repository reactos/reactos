// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore factory. Contains factory methods accessible to product code
//
//

#include "precomp.hpp"
#include "osversionhelper.h"

MtDefine(CMILFactory, MILApi, "CMILFactory");
MtDefine(BitmapDataClient, Mem, "BitmapMemClient");
DeclareTag(tagUseRemoting, "CompEng", "Use remoting")

//+----------------------------------------------------------------------------
//
//  Member:    MILCreateFactory
//
//  Synopsis:  Create factory object
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
MILCreateFactory(
    __deref_out_ecount(1) IMILCoreFactory **ppIFactory,
    UINT SDKVersion
    )
{
    HRESULT hr = S_OK;

    CMILFactory *pMILFactory = NULL;

    // Since this is responsible for the initialization of MIL, we cannot
    // have an API_ENTRY macro here due to the float point state-saver code.

    if (!ppIFactory)
    {
        IFC(E_INVALIDARG);
    }

    // Check SDKVersion for compatibility
    if (SDKVersion != MIL_SDK_VERSION)
    {
        APIError("Incorrect version number.");

        IFC(WGXERR_UNSUPPORTEDVERSION);
    }

    IFC(CMILFactory::Create(&pMILFactory));

    *ppIFactory = pMILFactory;
    pMILFactory = NULL;

Cleanup:

    ReleaseInterfaceNoNULL(pMILFactory);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::Create
//
//  Synopsis:  Public static method to enforce correct creation of a CMILFactory.
//
//-----------------------------------------------------------------------------
/*static*/
HRESULT
CMILFactory::
Create(
    __deref_out_ecount(1) CMILFactory **ppFactory
    )
{
    HRESULT hr = S_OK;

    CMILFactory *pFactory = new CMILFactory;
    IFCOOM(pFactory);
    pFactory->AddRef();

    IFC(pFactory->Init());

    *ppFactory = pFactory;
    pFactory = NULL;

Cleanup:

    ReleaseInterfaceNoNULL(pFactory);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::CMILFactory
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CMILFactory::CMILFactory(
    ) : m_pDisplaySet(NULL),
        m_hrLastDisplaySetUpdate(S_OK)
{
    // Let D3D device manager know it may be called and would need ID3D
    CD3DDeviceManager::Get();
    CAssertDllInUse::Enter();
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::~CMILFactory
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CMILFactory::~CMILFactory()
{
    ReleaseInterfaceNoNULL(m_pDisplaySet);

    CAssertDllInUse::Leave();
    // Let D3D device manager know there is one less caller
    CD3DDeviceManager::Release();
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::Init
//
//  Synopsis:  Performs any initialization on a CMILFactory which can fail.
//
//-----------------------------------------------------------------------------
HRESULT
CMILFactory::Init(
    void
    )
{
    HRESULT hr = S_OK;

    IFC(m_lock.Init());

    //
    // We don't instantiate the display set up front, we wait until the first render
    // pass or a display set request to create it. Once created, the only time a
    // display set update can come through is at render time.
    //
    Assert(m_pDisplaySet == NULL);

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::HrFindInterface
//
//  Synopsis:  QI Support method
//
//-----------------------------------------------------------------------------
HRESULT
CMILFactory::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILCoreFactory)
        {
            *ppvObject = static_cast<IMILCoreFactory *>(this);

            hr = S_OK;
        }
        else
        {
            // No other interfaces are supported from this object!
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::QueryCurrentGraphicsAccelerationCaps
//
//  Synopsis:  Get current display settings and query Tier of primary device or
//             common minimum of all display devices. This can fail if we never
//             successfully created a display set.
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void)
CMILFactory::QueryCurrentGraphicsAccelerationCaps(
    bool fReturnCommonMinimum,
    __out_ecount(1) ULONG *pulDisplayUniqueness,
    __out_ecount(1) MilGraphicsAccelerationCaps *pCaps
    )
{
    HRESULT hr = S_OK;

    const CDisplaySet *pDisplaySet = NULL;

    MIL_THR(GetCurrentDisplaySet(&pDisplaySet));

    //
    // We don't swap in a new display set until we can create a new one,
    // however, we do sent a SW tier notification and will return that
    // we are in SW for any tier queries.
    //
    if (SUCCEEDED(hr) && SUCCEEDED(m_hrLastDisplaySetUpdate))
    {
        //
        // This cannot fail.
        //
        pDisplaySet->GetGraphicsAccelerationCaps(
            fReturnCommonMinimum,
            pulDisplayUniqueness,
            pCaps
            );
    }
    else
    {
        //
        // Set data to no acceleration on failure
        //
        *pulDisplayUniqueness = 0;
        *pCaps = CDisplaySet::GetNoHardwareAccelerationCaps();
    }

    ReleaseInterface(pDisplaySet);
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::CreateBitmapRenderTarget
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMILFactory::CreateBitmapRenderTarget(
    UINT width,
    UINT height,
    MilPixelFormat::Enum format,
    FLOAT dpiX,
    FLOAT dpiY,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
    )
{
    API_ENTRY(CMILFactory::CreateBitmapRenderTarget);

    HRESULT hr = S_OK;

    if ((width == 0) || (height == 0) ||
        (dpiX <= 0.0f) || (dpiY <= 0.0f) ||
        (ppIRenderTargetBitmap == NULL))
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        if ((dwFlags & ~(MilRTInitialization::SoftwareOnly | MilRTInitialization::HardwareOnly)) ||
            (dwFlags == (MilRTInitialization::SoftwareOnly | MilRTInitialization::HardwareOnly)))
        {
            MIL_THR(E_INVALIDARG);
        }
    }
    if (SUCCEEDED(hr))
    {
        if ((format != MilPixelFormat::PBGRA32bpp) && (format != MilPixelFormat::PRGBA128bppFloat))
        {
            MIL_THR(WGXERR_UNSUPPORTEDPIXELFORMAT);
        }
    }

    if (SUCCEEDED(hr))
    {
        if ( !(dwFlags & MilRTInitialization::HardwareOnly) )
        {
            MIL_THR(CSwRenderTargetBitmap::Create(
                width,
                height,
                format,
                dpiX,
                dpiY,
                DisplayId::None,
                ppIRenderTargetBitmap
                DBG_STEP_RENDERING_COMMA_PARAM(NULL) // pDisplayRTParent
                ));
        }
        else
        {
            MIL_THR(WGXERR_NOTIMPLEMENTED);
        }
    }

    API_CHECK(hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::CreateMediaPlayer
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMILFactory::CreateMediaPlayer(
    __inout_ecount(1) IUnknown *pEventProxy,
    bool canOpenAnyMedia,
    __deref_out_ecount(1) IMILMedia **ppMedia
    )
{
    API_ENTRY(CMILFactory::CreateMediaPlayer);
    HRESULT hr = S_OK;

    CEventProxy *pProxy = NULL;

    IFC(pEventProxy->QueryInterface(IID_IMILEventProxy, reinterpret_cast<void **>(&pProxy)));

    IFC(CMILAV::CreateMedia(pProxy, canOpenAnyMedia, ppMedia));

Cleanup:
    ReleaseInterface(pProxy);
    API_CHECK(hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::CreateSWRenderTargetForBitmap
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CMILFactory::CreateSWRenderTargetForBitmap(
    __inout_ecount(1) IWICBitmap *pIBitmap,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
    )
{
    API_ENTRY(CMILFactory::CreateSWRenderTargetForBitmap);

    HRESULT hr = S_OK;
    IWGXBitmap *pWGXBitmap = NULL;

    if (pIBitmap == NULL || ppIRenderTargetBitmap == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(CWICWrapperBitmap::Create(
            pIBitmap,
            &pWGXBitmap
            ));
        
        if (SUCCEEDED(hr))
        {
            MIL_THR(CSwRenderTargetBitmap::Create(
                pWGXBitmap,
                DisplayId::None,
                ppIRenderTargetBitmap
                DBG_STEP_RENDERING_COMMA_PARAM(NULL) // pDisplayRTParent
                ));
        }
    }

    ReleaseInterface(pWGXBitmap);
    
    API_CHECK(hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::UpdateDisplayState
//
//  Synopsis:  Checks to see if the current display set is invalid, and, if it
//             is, attempts to create a new one. If this fails, it just keeps the
//             current display set indefinitely.
//
//  Returns:   True if the display state did in fact change.
//
//-----------------------------------------------------------------------------
__success(true) HRESULT
CMILFactory::UpdateDisplayState(
    __out_ecount(1) bool  *pDisplayStateChanged, 
    __out_ecount(1) int  *pDisplayCount
    )
{
    HRESULT hr = S_OK;

    const CDisplaySet *pNewDisplaySet = NULL;
    const CDisplaySet *pOldDisplaySet = NULL;
    bool displaySetChanged = false;

    bool firstInitialization = !IsDisplaySetInitialized();

    IFC(GetCurrentDisplaySet(&pOldDisplaySet));

    //
    // Has the display set changed now?
    //
    if (pOldDisplaySet->DangerousHasDisplayStateChanged())
    {
        //
        // Try to get a new display set from the display manager.
        // If it fails, we just hold onto the old display set, but
        // we do want to log it.
        //
        MIL_THR(g_DisplayManager.DangerousGetLatestDisplaySet(&pNewDisplaySet));

        if (SUCCEEDED(hr))
        {
            displaySetChanged = true;
            *pDisplayCount = pNewDisplaySet->GetDisplayCount();

            m_hrLastDisplaySetUpdate = hr;

            //
            // Great! Put in the new display set.
            //
            CGuard<CCriticalSection> guard(m_lock);

            //
            // If the display set has already changed, don't change it twice.
            //
            ReplaceInterface(m_pDisplaySet, pNewDisplaySet);
        }
    }

    if ((pNewDisplaySet == nullptr) ||
        (firstInitialization && (*pDisplayCount == 0)))
    {
        *pDisplayCount = pOldDisplaySet->GetDisplayCount();
    }

    if (*pDisplayCount == 0)
    {
        hr = WGXERR_DISPLAYSTATEINVALID;
    }

    //
    // If we failed to create the display set, or to swap in a new
    //
    if (FAILED(hr))
    {
        *pDisplayCount = 0;
        //
        // For every failure we want to return precisely one tier change
        // notification.
        //
        if (SUCCEEDED(m_hrLastDisplaySetUpdate) || 
            (pOldDisplaySet->GetDisplayCount() != 0))
        {
            displaySetChanged = true;
        }

        //
        // Record the failure, ensure that another failure doesn't cause us
        // to send another tier change notification.
        //
        m_hrLastDisplaySetUpdate = hr;
    }


Cleanup:

    *pDisplayStateChanged = displaySetChanged;

    //
    // Break on any unanticipated errors, we anticipate display state changes and
    // we anticipate OOM and OOVM. This is to aid debugging this case which has
    // historically proved difficult.
    //
    if (   FAILED(hr)
        && hr != WGXERR_DISPLAYSTATEINVALID
        && hr != E_OUTOFMEMORY
        && hr != D3DERR_OUTOFVIDEOMEMORY)
    {
        MilUnexpectedError(hr, TEXT("Could not create display set."));
    }

    ReleaseInterfaceNoNULL(pNewDisplaySet);
    ReleaseInterfaceNoNULL(pOldDisplaySet);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CMILFactory::ComputeRenderTargetTypeAndPresentTechnique
//
//  Synopsis:
//      Examine given render target usage context (target window properties)
//      and basic render target initialization flags to compute a completely
//      filled out set of render target initialization flags.
//

HRESULT
CMILFactory::ComputeRenderTargetTypeAndPresentTechnique(
    __in_opt HWND hwnd,
    MilWindowProperties::Flags flWindowProperties,
    MilWindowLayerType::Enum eWindowLayerType,
    MilRTInitialization::Flags InFlags,
    __out_ecount(1) MilRTInitialization::Flags * const pOutFlags
    )
{
    HRESULT hr = S_OK;

    // By default we should present using the normal device abstraction:
    //  D3D for hardware,
    //  GDI for software.
    // Note: This ignores any specification from caller.
    MilRTInitialization::Flags presentUsing = MilRTInitialization::PresentUsingHal;

    if (hwnd)
    {
        //
        // These checks are done is a specific order and should not be
        // rearranged without care.  The order of checks in sorted by
        // priority/dominance.  Composited checks are dominant, followed by
        // UpdateLayeredWindow, and RTL has least priority.
        //

        //
        // Check for composited windows
        //
        // Future Consideration:   Check MilWindowProperties::Compostited
        //  and use MilRTInitialization::PresentUsingBitBlt or, if need destination alpha,
        //  MilRTInitialization::PresentUsingAlphaBlend.  Compositied windows should always
        //  be redirected.

        //
        // Check for layered windows
        //
        if (   (presentUsing == MilRTInitialization::PresentUsingHal)
            && (eWindowLayerType != MilWindowLayerType::NotLayered))
        {
            if (eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer)
            {
                // We are rendering to a window that uses UpdateLayeredWindow.
                presentUsing = MilRTInitialization::PresentUsingUpdateLayeredWindow;
            }
            else
            {
                // We are rendering to a window that uses
                // SetLayeredWindowAttributes.  For the User32 redirection to
                // work we must present through the DC.
                presentUsing = MilRTInitialization::PresentUsingBitBlt;
            }
        }

        //
        // If still using HAL check for other conditions that need GDI based
        // presentation.
        //
        if (presentUsing == MilRTInitialization::PresentUsingHal)
        {
            if (
                   //
                   // We're running in a scenario where due to interaction
                   // between channel clients and the application, we must
                   // present through GDI.
                   //
                   (flWindowProperties & MilWindowProperties::PresentUsingGdi)
                   //
                   // Check for FlowDirection=RightToLeft windows
                   //
                   // Support for RTL windows through DX is available starting
                   // with Vista.  Otherwise we hit a DX bug (Windows OS Bug
                   // 1185634).  To avoid the bug we use GDI to Present.
                   //
                   // When we're running on XP/Server2003 with a right to left
                   // layout window.  We must use GDI to present to avoid the
                   // DX bug.
                   //
                || (   (flWindowProperties & MilWindowProperties::RtlLayout)
                    && (!WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater()))
                   //
                   // Check for non-client area presentation
                   //
                   // D3D has a present flag D3DPRESENTFLAG_NONCLIENT, but
                   // support varies.  The easiest way to cover all cases is
                   // simply to is GDI BitBlt when non-client rendering is
                   // requested.
                   //
                || (InFlags & MilRTInitialization::RenderNonClient))
            {
                presentUsing = MilRTInitialization::PresentUsingBitBlt;
            }
        }
    }

    // Output result updating the flags with the MIL_PRESENT_USING_XXX option.
    *pOutFlags = (InFlags & ~MilRTInitialization::PresentUsingMask) | presentUsing;

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CMILFactory::CreateDesktopRenderTarget
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CMILFactory::CreateDesktopRenderTarget(
    __in_opt HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetHWND **ppIRenderTarget
    )
{
    API_ENTRY(CMILTestFactory::CreateDesktopRenderTarget);

    HRESULT hr = S_OK;
    CDisplaySet const *pDisplaySet = NULL;

    if (NULL == ppIRenderTarget)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Check for registry keys that override the render target initialization flags
    //

    {
        // Open HKEY_CURRENT_USER\\Software\\Microsoft\\Avalon.Graphics
        CDisplayRegKey keyGraphics(HKEY_CURRENT_USER, _T(""));

        if (keyGraphics.IsValid())
        {
            DWORD dwValue;

            //
            // Check for reference rasterizer request, this RT
            // isn't already specified as software or NULL.
            //
            // If requested but not present we will fall over to SW,
            // assuming HW Only isn't also specified.
            //

            dwValue = 0;
            if (   !(dwFlags & MilRTInitialization::SoftwareOnly)
                && keyGraphics.ReadDWORD(_T("UseReferenceRasterizer"), &dwValue)
                && dwValue)
            {
                dwFlags |= MilRTInitialization::UseRefRast | MilRTInitialization::HardwareOnly;
            }
        }
    }

    IFC(GetCurrentDisplaySet(&pDisplaySet));

    Assert(pDisplaySet);

    IFC(HrValidateInitializeCall(
        hwnd,
        eWindowLayerType,
        dwFlags
        ));

    IFC(CDesktopRenderTarget::Create(
        hwnd,
        pDisplaySet,
        eWindowLayerType,
        dwFlags,
        ppIRenderTarget
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pDisplaySet);

    API_CHECK(hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:   CMILFactory::GetCurrentDisplaySet
//
//  Synopsis: Safely gets the current display set with a reference count on it.
//
//  Returns:  Success or failure.
//
//-----------------------------------------------------------------------------
HRESULT
CMILFactory::GetCurrentDisplaySet(
    const CDisplaySet **ppCurrentDisplaySet
        // The returned display set.
    )
{
    HRESULT hr = S_OK;

    const CDisplaySet *pCurrentDisplaySet = NULL;

    {
        CGuard<CCriticalSection> guard(m_lock);

        //
        // If we don't have a display set yet, try and create one.
        //
        if (m_pDisplaySet == NULL)
        {
            {
                CUnGuard<CCriticalSection> unguard(m_lock);

                // This call is okay here since if m_pDisplaySet is NULL, it means this
                // is the first time we've tried to get the DisplaySet for the MILFactory.
                // We aren't in the middle of a render pass yet where changing the
                // DisplaySet is dangerous.
                IFC(g_DisplayManager.DangerousGetLatestDisplaySet(&pCurrentDisplaySet));
            }

            //
            // If we don't have a display set (still) set it now.
            //
            if (m_pDisplaySet == NULL)
            {
                SetInterface(m_pDisplaySet, pCurrentDisplaySet);
            }
            else
            {
                ReplaceInterface(pCurrentDisplaySet, m_pDisplaySet);
            }
        }
        else
        {
            SetInterface(pCurrentDisplaySet, m_pDisplaySet);
        }
    }

    *ppCurrentDisplaySet = pCurrentDisplaySet;
    pCurrentDisplaySet = NULL;

Cleanup:

    ReleaseInterface(pCurrentDisplaySet);

    RRETURN(hr);
}




