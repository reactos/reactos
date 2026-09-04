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
//      Implements CD3DDeviceManager which maintains a list of existing D3D
//      devices via the CD3DDeviceLevel1 wrappers and creates new
//      CD3DDeviceLevel1's as they are needed.
//
//      Also keeps a shared NullRef Device for creation of our Device
//      Independent objects.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#define ENABLE_NON_LDDM_DWM 1

ExternTag(tagD3DStats);
DeclareTag(tagDisablePureDevice, "MIL-HW", "Disable pure device");
DeclareTag(tagDisableHWGroupAdapterSupport, "MIL-HW", "Disable HW group adapter support");

MtExtern(D3DDevice);

CD3DDeviceManager g_D3DDeviceManager;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Create
//
//  Synopsis:
//      Initialize global D3D device manager
//
//------------------------------------------------------------------------------
__checkReturn HRESULT 
CD3DDeviceManager::Create()
{
    HRESULT hr = S_OK;

    Assert(!g_D3DDeviceManager.m_csManagement.IsValid());
    Assert(!g_D3DDeviceManager.m_fD3DLoaded);

    //
    // Call init
    //

    IFC(g_D3DDeviceManager.Init());

Cleanup:
    RRETURN(hr);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Delete
//
//  Synopsis:
//      Uninitialize global D3D device manager
//
//------------------------------------------------------------------------------
void 
CD3DDeviceManager::Delete()
{
    if (g_D3DDeviceManager.m_fD3DLoaded)
    {
        ReleaseInterface(g_D3DDeviceManager.m_pID3D);
        ReleaseInterface(g_D3DDeviceManager.m_pDisplaySet);
        g_D3DDeviceManager.m_fD3DLoaded = false;
    }
    else
    {
        Assert(!g_D3DDeviceManager.m_pID3D);
        Assert(!g_D3DDeviceManager.m_pDisplaySet);
    }

    ReleaseInterface(g_D3DDeviceManager.m_pNextDisplaySet);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Get
//
//  Synopsis:
//      Returns the global D3D device manager and increments possible callers to
//      Get and non-static methods
//
//------------------------------------------------------------------------------
CD3DDeviceManager *CD3DDeviceManager::Get()
{
    g_D3DDeviceManager.IncCallers();
    return &g_D3DDeviceManager;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Release
//
//  Synopsis:
//      Decrements possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::Release()
{
    g_D3DDeviceManager.DecCallers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::NotifyDisplayChange
//
//  Synopsis:
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::NotifyDisplayChange(
    __in_ecount(1) CDisplaySet const * pOldDisplaySet,
    __in_ecount(1) CDisplaySet const * pNewDisplaySet
    )
{
    g_D3DDeviceManager.HandleDisplayChange(pOldDisplaySet, pNewDisplaySet);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::CD3DDeviceManager
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DDeviceManager::CD3DDeviceManager()
{
    m_cCallers = 0;
    m_pID3D = NULL;
    m_fD3DLoaded = false;
    m_pDisplaySet = NULL;
    m_pNextDisplaySet = NULL;
    m_iFirstUnusable = 0;
    m_pNullRefDevice = NULL;
    m_pSWDevice = NULL;
#if DBG
    m_fDbgCreatingNewDevice = false;
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::~CD3DDeviceMaanger
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DDeviceManager::~CD3DDeviceManager()
{
#if DBG
    Assert(!m_fDbgCreatingNewDevice);

    INT iCount = m_rgDeviceList.GetCount();

    Assert(iCount == 0);
    Assert(m_iFirstUnusable == 0);

    for (INT i=0; i < iCount; i++)
    {
        CD3DDeviceLevel1 *pDeviceLevel1 = m_rgDeviceList[i].pDeviceLevel1;

        Assert(pDeviceLevel1);
        Assert(pDeviceLevel1->GetRefCount() == 0);

        delete pDeviceLevel1;
    }
#endif

    if (m_fD3DLoaded)
    {
        ReleaseInterfaceNoNULL(m_pNullRefDevice);
        ReleaseInterfaceNoNULL(m_pSWDevice);
        ReleaseInterfaceNoNULL(m_pID3D);
    }

    ReleaseInterfaceNoNULL(m_pDisplaySet);
    ReleaseInterfaceNoNULL(m_pNextDisplaySet);

    m_csManagement.DeInit();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Init
//
//  Synopsis:
//      Initialize static D3D pointers and references
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceManager::Init(
    )
{
    HRESULT hr = S_OK;

    if (m_fD3DLoaded)
    {
        IFC(E_UNEXPECTED);
    }

    if (m_csManagement.IsValid())
    {
        IFC(E_UNEXPECTED);
    }

    IFC(m_csManagement.Init());

Cleanup:
    Assert(m_csManagement.IsValid() || FAILED(hr));

#pragma prefast(suppress: 15, "m_csManagement is deleted in destructor")
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetSWDevice
//
//  Synopsis:
//      Creates and holds onto a single D3DDEVTYPE_SW device.
//
//  Return Value:
//      S_OK if it was successfully able to use an existing or create a new
//           software device.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceManager::GetSWDevice(
    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDevice
    )
{
    HRESULT hr = S_OK;

    *ppDevice = NULL;
    
    // Start a critical section to make sure we only create one device.

    CGuard<CCriticalSection> oGuard(m_csManagement);

    IDirect3DDevice9 *pIDirect3DDevice = NULL;
    CD3DDeviceLevel1 *pDeviceLevel1 = NULL;
    
    if (m_pID3D == NULL)
    {
        //
        // Initialize d3d references
        //

        IFC(InitializeD3DReferences(NULL));
    }

    Assert(m_pID3D);
    Assert(m_pDisplaySet);
    Assert(m_pDisplaySet->D3DObject() == m_pID3D);
    IFC(m_pDisplaySet->EnsureSwRastIsRegistered());

    //
    // Return our existing NULL ref device or create one
    //

    if (m_pSWDevice == NULL)
    {
        D3DPRESENT_PARAMETERS presentParams;

        ZeroMemory(&presentParams, sizeof(D3DPRESENT_PARAMETERS));
        presentParams.BackBufferWidth = 1;
        presentParams.BackBufferHeight = 1;
        presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
        presentParams.BackBufferCount = 1;
        presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
        presentParams.hDeviceWindow = NULL;
        presentParams.Windowed = TRUE;
        presentParams.EnableAutoDepthStencil = FALSE;
        presentParams.AutoDepthStencilFormat = D3DFMT_UNKNOWN;

        DWORD dwBehaviorFlags =
            D3DCREATE_SOFTWARE_VERTEXPROCESSING |
            D3DCREATE_MULTITHREADED |
            D3DCREATE_FPU_PRESERVE |
            D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;

        //
        // D3D9.0c requires a valid window for CreateDevice.
        // For windowed targets we are happy to pass any random window
        // to get things to work.  Since creating our own dummy window
        // in a library has significant issues behind it including
        // performance and app compat, we just grab the desktop window.
        //
        
        IFC(m_pID3D->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_SW,
            GetDesktopWindow(),
            dwBehaviorFlags,
            &presentParams,
            &pIDirect3DDevice
            ));

        IFC(CD3DDeviceLevel1::Create(
            pIDirect3DDevice,
            m_pDisplaySet->Display(D3DADAPTER_DEFAULT),
            this,
            dwBehaviorFlags,
            &pDeviceLevel1
            ));

        // All tracked devices are potential non-static method callers
        m_cCallers++;

        // We hold a pointer to the m_pSWDevice without an AddRef (like
        // the other managed devices.)
        m_pSWDevice = pDeviceLevel1;
    }

    m_pSWDevice->AddRef();
    *ppDevice = m_pSWDevice;

Cleanup:
    if (hr == D3DERR_DEVICELOST)
    {
        hr = WGXERR_DISPLAYSTATEINVALID;
    }
    
    ReleaseInterfaceNoNULL(pIDirect3DDevice);
    ReleaseInterfaceNoNULL(pDeviceLevel1);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::IncCallers
//
//  Synopsis:
//      Increments possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::IncCallers()
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    m_cCallers++;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::DecCallers
//
//  Synopsis:
//      Decrements possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::DecCallers()
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    Assert(m_cCallers > 0);

    --m_cCallers;

    if (m_cCallers == 0)
    {
        // Managed devices (including the SW device) are callers so
        // there should be none now.
        Assert(m_rgDeviceList.GetCount() == 0);
        Assert(!m_pSWDevice);

        if (m_fD3DLoaded)
        {
            if (m_pID3D)
            {
                ReleaseInterface(m_pNullRefDevice);
                CD3DRegistryDatabase::Cleanup();
                m_pID3D->Release();
                m_pID3D = NULL;
            }
            ReleaseInterface(m_pDisplaySet);
            m_fD3DLoaded = false;
        }

        ReleaseInterface(m_pNextDisplaySet);
        
        Assert(!m_pID3D);
        Assert(!m_pDisplaySet);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::InitializeD3DReferences
//
//  Synopsis:
//      Initialize static D3D pointers and references
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::InitializeD3DReferences(
    __in_ecount_opt(1) CDisplaySet const *pGivenDisplaySet
    )
{
    HRESULT hr = S_OK;

    CDisplaySet const *pDisplaySet = NULL;
    IDirect3D9 *pID3DNoRef = NULL;

    IFC(g_DisplayManager.DangerousGetLatestDisplaySet(&pDisplaySet));

    Assert(pDisplaySet);

    if (pGivenDisplaySet && pGivenDisplaySet != pDisplaySet)
    {
        //
        // Caller has supplied pGivenDisplaySet that's obsolete.
        // We should not use it because we need not unworkable device,
        // and we should not use newest display set because it'll
        // cause mismatches with the caller that relies upon pGivenDisplaySet
        // and expects us to use it.
        // Just fail.
        // 

        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    //
    // Make sure ID3D is available
    //

    IFC(pDisplaySet->GetD3DObjectNoRef(&pID3DNoRef));

    Assert(pID3DNoRef);

    Assert(m_cCallers > 0);

    Assert(m_fD3DLoaded || (m_rgDeviceList.GetCount() == 0));

    //
    // Check if there is a new D3D that we should be using
    //

    if (m_pID3D != pID3DNoRef)
    {
        //
        // If there was a prior D3D (and the new one is different) then there
        // must have been a mode change or other event to invalidate the old
        // D3D.  All old devices would now be unusable.
        // Every mode changeis accompanied with HandleDisplayChange() call
        // that should release m_pID3D and m_pDisplaySet.
        //

        Assert(!m_pID3D);
        Assert(!m_pDisplaySet);

        //
        // Initialize registry
        //

        IFC(CD3DRegistryDatabase::InitializeFromRegistry(pID3DNoRef));

        //
        // Save D3D reference
        //

        m_pID3D = pID3DNoRef;
        m_pID3D->AddRef();
        m_fD3DLoaded = true;

        //
        // Save DisplaySet reference
        //

        m_pDisplaySet = pDisplaySet; // Transfer reference
        pDisplaySet = NULL;
    }
    else
    {
        Assert(m_fD3DLoaded);
        Assert(m_pDisplaySet == pDisplaySet);
    }

    //
    // Now that we've settle onto current display set we can
    // release m_pNextDisplaySet that was only used to protect
    // against d3d module unloading.
    //

    ReleaseInterface(m_pNextDisplaySet);

Cleanup:
    ReleaseInterfaceNoNULL(pDisplaySet);
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::HandleDisplayChange
//
//  Synopsis:
//      Mark all D3D devices as unusable and release static D3D pointers and
//      references.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::HandleDisplayChange(
    __in_ecount(1) CDisplaySet const * pOldDisplaySet,
    __in_ecount(1) CDisplaySet const * pNewDisplaySet
    )
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    //
    // Check whether we hold on display set that became obsolete;
    // if not so then we don't care.
    //
    if (pOldDisplaySet == m_pDisplaySet)
    {
        // Mark all usable D3D devices as unusable
        while (m_iFirstUnusable > 0)
        {
#if DBG
            UINT dbgOldFirstUnusable = m_iFirstUnusable;
#endif
            CD3DDeviceLevel1 *pDevice = m_rgDeviceList[m_iFirstUnusable - 1].pDeviceLevel1;
            ENTER_DEVICE_FOR_SCOPE(*pDevice);
            pDevice->MarkUnusable(false /* This call is entry protected by above Enter */);
#if DBG
            // We're depending upon MarkUnusable decrementing this number to avoid 
            // an infinite loop.
            Assert(m_iFirstUnusable == dbgOldFirstUnusable - 1);
#endif
        }

        if (m_pSWDevice)
        {
            m_pSWDevice->MarkUnusable(TRUE);
        }

        if (m_fD3DLoaded)
        {
            Assert(m_cCallers > 0);

            if (m_pID3D)
            {
                CD3DRegistryDatabase::Cleanup();
                m_pID3D->Release();
                m_pID3D = NULL;
            }

            //
            // Do not release the load reference on D3D at this point
            // as there may still be devices using D3D.  Note: if m_cCallers
            // is 1 and the device count is 0, there is probably a factory
            // holding it normal reference.
            //
        }
        else
        {
            Assert(!m_pID3D);
        }

        ReleaseInterface(m_pDisplaySet);

        //
        // Hold on pNewDisplaySet to avoid releasing d3d module.
        //

        ReplaceInterface(m_pNextDisplaySet, pNewDisplaySet);
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ChooseTargetFormat
//
//  Synopsis:
//      Selects render target format based on RT Init Flags
//
//------------------------------------------------------------------------------
void
ChooseTargetFormat(
    MilRTInitialization::Flags dwFlags,  // Presentation options
    __out_ecount(1) D3DFORMAT *pFmt      // The format of the target
    )
{
    if (dwFlags & MilRTInitialization::NeedDestinationAlpha)
    {
        *pFmt = D3DFMT_A8R8G8B8;
    }
    else
    {
        *pFmt = D3DFMT_X8R8G8B8;
    }
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      CheckDisplayFormat
//
//  Synopsis:
//      Determine target format and confirm device support with given mode.
//

HRESULT
CheckDisplayFormat(
    __in_ecount(1) IDirect3D9 *pID3D,
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT DisplayFormat,
    MilRTInitialization::Flags RTInitFlags
    )
{
    HRESULT hr = S_OK;

    D3DFORMAT TargetFormat;

    ChooseTargetFormat(RTInitFlags, OUT &TargetFormat);

    IFC(pID3D->CheckDeviceType(
        Adapter,
        DeviceType,
        DisplayFormat,
        TargetFormat,
        true // windowed
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetDisplayMode
//
//  Synopsis:
//      Get display mode for adapter
//
//------------------------------------------------------------------------------

HRESULT
CD3DDeviceManager::GetDisplayMode(
    __inout_ecount(1) D3DDeviceCreationParameters *pCreateParams,
    __out_xcount_full(pCreateParams->NumberOfAdaptersInGroup) D3DDISPLAYMODEEX *rgDisplayModes
    ) const
{
    HRESULT hr = S_OK;

    // Since we do not support fullscreen, we cannot create adapter groups.
    Assert(!(pCreateParams->BehaviorFlags & D3DCREATE_ADAPTERGROUP_DEVICE));
    Assert(pCreateParams->NumberOfAdaptersInGroup == 1);
    IFC(m_pDisplaySet->Display(pCreateParams->AdapterOrdinal)->GetMode(rgDisplayModes, NULL));

    IFC(CheckDisplayFormat(
        m_pID3D,
        pCreateParams->AdapterOrdinal,
        pCreateParams->DeviceType,
        rgDisplayModes[0].Format,
        pCreateParams->RTInitFlags
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::DoesWindowedHwDeviceExist
//
//  Synopsis:
//      Returns 'true' if a non-fullscreen, hardware device exists on the given 
//      context and adapter id.
//
//------------------------------------------------------------------------------

bool
CD3DDeviceManager::DoesWindowedHwDeviceExist(
    UINT uAdapter
    )
{
    D3DDeviceCreationParameters oCreateParams;
    bool succeeded = false;
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(m_csManagement);

    IFC(InitializeD3DReferences(NULL));

    IFC(ComposeCreateParameters(
        GetDesktopWindow(),           // hwnd doesn't matter for non-fullscreen
        MilRTInitialization::Default,
        uAdapter,
        D3DDEVTYPE_HAL,
        &oCreateParams
        ));
    
    IFC(FindDeviceMatch(
        &oCreateParams,
        0,                  // uStartIndex
        m_iFirstUnusable,   // uLimitPlusOne
        NULL                // ppDevice
        ));

    succeeded = true;

Cleanup:
    return succeeded;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::FindDeviceMatch
//
//  Synopsis:
//      Finds an existing CD3DDeviceLevel1 object that can satisfy the settings
//      given, with the list range given.
//
//  Return Value:
//      S_OK if an acceptable state manager was located
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::FindDeviceMatch(
    __inout_ecount(1) D3DDeviceCreationParameters *pCreateParams,
    UINT uStartIndex,
    UINT uLimitPlusOne,
    __deref_opt_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
    ) const
{
    HRESULT hr = E_FAIL;

    for (UINT i = uStartIndex; i < uLimitPlusOne; i++)
    {
        D3DDeviceInformation const &oDevInfo = m_rgDeviceList[i];

        Assert(oDevInfo.pDeviceLevel1);

        //   Should hFocusWindow be compared?
        //  hFocusWindow does not need to be the same hwnd for a new
        //  swapchain as it was for the original hwnd when the device
        //  was created, but all impacts from using a different hwnd
        //  for new swapchains needs to be investigated.  There may
        //  be a perf impact for which window is in focus.

        if (   (pCreateParams->DeviceType ==
                oDevInfo.CreateParams.DeviceType)
            && (   // nothing else matters for SW type since we share a device
                   (pCreateParams->DeviceType == D3DDEVTYPE_SW)
                || (   (pCreateParams->AdapterOrdinal ==
                        oDevInfo.CreateParams.AdapterOrdinal)
                       // behavior flag are the same excluding D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX
                    && ( ( (pCreateParams->BehaviorFlags ^ oDevInfo.CreateParams.BehaviorFlags)
                           & ~D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX) == 0)
                   )
               )
           )
        {
            if (ppDeviceLevel1)
            {
                CD3DDeviceLevel1 *pDeviceLevel1 =
                    oDevInfo.pDeviceLevel1;

                Assert(pDeviceLevel1);

                pDeviceLevel1->AddRef();
                *ppDeviceLevel1 = pDeviceLevel1;

                // Update behavior flags
                pCreateParams->BehaviorFlags = oDevInfo.CreateParams.BehaviorFlags;
            }

            hr = S_OK;

            break;
        }
    }

    // No RRETURN because we don't want spew or capture here.
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetAvaliableDevice
//
//  Synopsis:
//      Finds an available CD3DDeviceLevel1 object that satisfies the settings
//      given.
//
//  Notes:
//      pCreateParams->BehaviorFlags may have state of
//      D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX bit changed upon success.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::GetAvailableDevice(
    __inout_ecount(1) D3DDeviceCreationParameters *pCreateParams,
    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
    ) const
{
    HRESULT hr = S_OK;

    hr = FindDeviceMatch(
        pCreateParams,
        0,
        m_iFirstUnusable,
        ppDeviceLevel1
        );

#if DBG
    if (FAILED(hr))
    {
        if (SUCCEEDED(FindDeviceMatch(
            pCreateParams,
            m_iFirstUnusable,
            m_rgDeviceList.GetCount(),
            ppDeviceLevel1
            )))
        {
            ReleaseInterface(*ppDeviceLevel1);  // Get rid of useless reference
            TraceTag((tagMILWarning,
                      "A new D3D device will be created before its"
                      " matching predecessor will be completely freed."
                      ));
        }
    }
#endif DBG

    // No RRETURN because we don't want spew or capture here.
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetD3DDeviceAndPresentParams
//
//  Synopsis:
//      Finds or creates a CD3DDeviceLevel1 object that can satisfy the settings
//      it was given.
//
//  Return Value:
//      S_OK if an acceptable state manager was located
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::GetD3DDeviceAndPresentParams(
    __in_opt HWND hwnd,                                         // destination window

    MilRTInitialization::Flags dwFlags,                         // Presentation/
                                                                // fallback options

    __in_ecount_opt(1) CDisplay const *pDisplay,                // Targeted display -
                                                                // can be NULL for SW

    D3DDEVTYPE type,                                            // D3D device type

    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1,    // recieves the located
                                                                // D3D device

    __out_ecount_opt(1) D3DPRESENT_PARAMETERS *pPresentParams,  // receives
                                                                // presentation
                                                                // parameters that
                                                                // should be used to
                                                                // create a swap chain

    __out_ecount_opt(1) UINT *pAdapterOrdinalInGroup
    )
{
    HRESULT hr = S_OK;

    Assert(m_cCallers > 0);

    *ppDeviceLevel1 = NULL;

    CGuard<CCriticalSection> oGuard(m_csManagement);

    D3DDeviceCreationParameters CreateParams;

    //
    // Ensure we have an adapter index to work with
    //

    UINT uAdapter = 0;

    if (pDisplay)
    {
        uAdapter = pDisplay->GetDisplayIndex();
    }
    else
    {
        if (type != D3DDEVTYPE_SW)
        {
            IFC(WGXERR_INVALIDPARAMETER);
        }

        uAdapter = 0;
    }

    IFC(InitializeD3DReferences(pDisplay ? pDisplay->DisplaySet() : NULL));

    // Check our registry database to see if we are allowed to create the a 
    // d3d device on this adapter

    if (type == D3DDEVTYPE_HAL)
    {
        if (uAdapter >= m_pID3D->GetAdapterCount())
        {
            IFC(WGXERR_NO_HARDWARE_DEVICE);
        }

        //
        // Check for adapter/driver disables.
        //

        bool fEnabled;

        IFC(CD3DRegistryDatabase::IsAdapterEnabled(uAdapter, &fEnabled));

        if (!fEnabled)
        {
            TRACE_DEVICECREATE_FAILURE(
                    uAdapter, 
                    "Registry settings disabled hw acceleration " 
                    "(see HKEY_CURRENT_USER\\Software\\Microsoft\\Avalon.Graphics)", hr
                    );
            IFC(WGXERR_NO_HARDWARE_DEVICE);
        }
    }

    if (type == D3DDEVTYPE_SW)
    {
        //
        // Make sure Sw rasterizer is registered with current ID3D
        //

        Assert(m_pID3D);
        Assert(m_pDisplaySet);
        Assert(m_pDisplaySet->D3DObject() == m_pID3D);
        IFC(m_pDisplaySet->EnsureSwRastIsRegistered());
    }


    IFC(ComposeCreateParameters(
        hwnd,
        dwFlags,
        uAdapter,
        type,
        &CreateParams
        ));

    {
        //
        // Get display mode(s) and finalize group support
        //

        DynArrayIA<D3DDISPLAYMODEEX, 4> drgDisplayModes;
        D3DDISPLAYMODEEX *rgDisplayModes;

        IFC(drgDisplayModes.AddMultiple(
            CreateParams.NumberOfAdaptersInGroup,
            &rgDisplayModes
            ));

        IFC(GetDisplayMode(
            &CreateParams,
            rgDisplayModes
            ));

        //
        // Try to find an existing device
        //

        ASSIGN_HR(hr, GetAvailableDevice(
            &CreateParams,
            ppDeviceLevel1
            ));

        //
        // Write present parameters, for specified adapter
        //
        // Note: For fullscreen case when an existing device is found,
        //       technically the present parameters should be read from
        //       appropriate swap chain.  For code simplicity we just assume
        //       caller is passing the same RTInitFlags for each adapter and
        //       that ComposePresentParameters can be relied upon to generate
        //       consistent values.  If caller uses different RTInitFlags then
        //       the attempt to create a second fullscreen device should fail.
        //

        D3DPRESENT_PARAMETERS oPresentParameters;

        if (!pPresentParams)
        {
            pPresentParams = &oPresentParameters;
        }

        ComposePresentParameters(
            rgDisplayModes[CreateParams.AdapterOrdinalInGroup],
            CreateParams,
            pPresentParams
            );

        if (FAILED(hr))
        {
            //
            // Create new device
            //

            IFC(CreateNewDevice(
                &CreateParams,
                pPresentParams,
                rgDisplayModes,
                ppDeviceLevel1
                ));
        }
    }

    if (pAdapterOrdinalInGroup)
    {
        *pAdapterOrdinalInGroup = CreateParams.AdapterOrdinalInGroup;
    }

Cleanup:

    //
    // If the mode has changed at this point, independent of success or failure
    // so far, release any device and return failure.
    //

    if (m_pDisplaySet && m_pDisplaySet->DangerousHasDisplayStateChanged())
    {
        ReleaseInterface(*ppDeviceLevel1);
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::UnusedNotification
//
//  Synopsis:
//      Receives notification that one of its managed objects is no longer in
//      use.  The object may be tracked and saved for later use or
//      UnusedNotification should delete the object.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::UnusedNotification(
    __inout_ecount(1) CMILPoolResource *pUnused // pointer to no longer used CD3DDeviceLevel1
    )
{
    if (m_fD3DLoaded)
    {
        CGuard<CCriticalSection> oGuard(m_csManagement);

        CD3DDeviceLevel1 *pDeviceLevel1 = DYNCAST(CD3DDeviceLevel1, pUnused);

        // Is pUnused the SW device?
        if (m_pSWDevice == pDeviceLevel1 && pDeviceLevel1->GetRefCount() == 0)
        {
            delete pDeviceLevel1;
            m_pSWDevice = NULL;

            DecCallers();
        }
        else
        {
            // If not look for it in the list of HW devices.
            UINT uCount = m_rgDeviceList.GetCount();

            for (UINT i = 0; i < uCount; i++)
            {
                if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
                {
                    //
                    // Make sure this object hasn't since been handed
                    //  back out since its last Release.
                    //
                    // Note that if it was handed back out, but was
                    //  again Released so that the current Ref count is
                    //  zero, then we will remove the entry now and the
                    //  pending call to this function will not be able
                    //  to locate the entry.  This is the primary reason
                    //  we never return success - the object may have
                    //  been maanged by this manager, but we won't be
                    //  able to find it in the list.  In fact that object
                    //  should be deleted at that point in time.
                    //

                    if (pDeviceLevel1->GetRefCount() != 0)
                    {
                        break;
                    }

                    //
                    // Object destruction must be done before DecCallers so
                    // that the device may clean up all of its D3D resources
                    // before D3D is potentially unloaded.
                    //

                    NotifyDeviceLost(m_rgDeviceList[i]);
                    delete pDeviceLevel1;

                    //
                    // A match was found so remove it by replacing it
                    // with the last element in the list (if it isn't
                    // already) and then shrinking the list size by 1.
                    //

                    UINT iLast = uCount - 1;

                    if (i < m_iFirstUnusable)
                    {
                        // Move up the last usable
                        m_rgDeviceList[i] = m_rgDeviceList[--m_iFirstUnusable];
                        // Move up the last unusable into the first unusable
                        m_rgDeviceList[m_iFirstUnusable] = m_rgDeviceList[iLast];
                    }
                    else
                    {
                        // Just move up the last unusable
                        m_rgDeviceList[i] = m_rgDeviceList[iLast];
                    }

                    m_rgDeviceList.SetCount(iLast);

                    //
                    // When all of the entries have been freed, shrink
                    // to free all dynamic memory, since the debug
                    // memory trackers don't appreciate being called
                    // at process detatch.
                    //

                    if (iLast == 0)
                    {
                        m_rgDeviceList.ShrinkToSize();
                    }

                    // Remove this previously tracked device as a caller
                    DecCallers();

                    break;
                }
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::UnusableNotification
//
//  Synopsis:
//      Receives notification that one of its managed objects is no longer
//      usable.  The object may not be handed out anymore, but needs to still be
//      tracked until it is know to have deleted all of its D3D resources.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::UnusableNotification(
    __inout_ecount(1) CMILPoolResource *pUnusable // pointer to lost CD3DDeviceLevel1
    )
{
    Assert(m_fD3DLoaded);

    CGuard<CCriticalSection> oGuard(m_csManagement);

    const CD3DDeviceLevel1 *pDeviceLevel1 = DYNCAST(CD3DDeviceLevel1, pUnusable);

    //
    // Find entry and move to the unusable section of the list
    //

#if DBG
    bool fFound = false;
#endif

    for (UINT i = 0; i < m_iFirstUnusable; i++)
    {
        if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
        {
            NotifyDeviceLost(m_rgDeviceList[i]);
            
            m_iFirstUnusable--;
            if (i != m_iFirstUnusable)
            {
                D3DDeviceInformation temp = m_rgDeviceList[i];
                // Move last usable into newly unusable's place
                m_rgDeviceList[i] = m_rgDeviceList[m_iFirstUnusable];
                // Move newly unusable into first unusable position
                m_rgDeviceList[m_iFirstUnusable] = temp;
            }

#if DBG
            fFound = true;
#endif
            break;
        }
    }

#if DBG
    //
    // Normally the device should be found in the usable list,
    // but if a mode change has happened then it might have been
    // moved to the unusable section; so, check there too.
    //
    // But only check if we are not currently creating a new device.  During
    // creation the device won't be in the list until after some tests.
    //
    // The SW device is not in the device list so ignore it.
    //

    if (!fFound && !m_fDbgCreatingNewDevice && pUnusable != m_pSWDevice)
    {
        UINT uTotal = m_rgDeviceList.GetCount();

        for (UINT i = m_iFirstUnusable; i < uTotal; i++)
        {
            if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
            {
                TraceTag((tagMILWarning,
                          "Device was lost upon Present after mode change."));
                fFound = true;
                break;
            }
        }

        Assert(fFound);
    }
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::ComposeCreateParameters
//
//  Synopsis:
//      Takes MILRender's Initialize/Presentation parameters and translates them
//      into D3D CreateDevice parameters.
//
//  Return Value:
//      S_OK - successful translation to D3D create parameters
//
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::ComposeCreateParameters(
    __in_opt HWND hwnd,                             // destination window
    MilRTInitialization::Flags dwFlags,             // RT/device options
    UINT uAdapter,                                  // D3D adapter ordinal
    D3DDEVTYPE type,                                // D3D device type
    __out_ecount(1) D3DDeviceCreationParameters *pD3DCreateParams
    ) const
{
    HRESULT hr = S_OK;

    Assert(m_fD3DLoaded);
    Assert(m_pID3D);

    // Note: D3D9.0c requires a valid window for CreateDevice.

    // Make sure the window is valid if this is not full screen
    // or the hwnd is not NULL
    if (hwnd && !IsWindow(hwnd))
    {
        IFC(__HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
    }

    D3DCAPS9 caps;

    //
    // Get adapter capabilites
    //

    IFC(m_pID3D->GetDeviceCaps(
        uAdapter, 
        type, 
        &caps
        ));

    // Set Creation Parameters
    pD3DCreateParams->AdapterOrdinal = uAdapter;
    pD3DCreateParams->DeviceType = type;
    pD3DCreateParams->hFocusWindow = hwnd;

    //  We need to create with Disable Driver Management EX
    // flag because if we don't when there's not enough video memory to move a managed
    // texture to video memory, d3d will disable the texture stage and move on without
    // reporting any error to us.  With this flag they will return an out of video
    // memory error.

    pD3DCreateParams->BehaviorFlags = D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;

    // 
    // Check if we are an LDDM device.
    //
    // The flag D3DCAPS2_CANSHARERESOURCE in caps.Caps2 stands not only for
    // resources sharing, but also for DX9.L features. 
    //
    if ((caps.Caps2 & D3DCAPS2_CANSHARERESOURCE))
    {
        // (WinOSBug 1415987)
        // DWM (fullscreen and no HWND) must avoid D3D's automatic disabling of
        // screensavers after N calls to Present.  Use of ScreenSaver create flag
        // accomplishes this.  It is safe to use all the time - windowed or not.
        // Let fullscreen windowed applications also avoid disabling screensaver's
        // automatically since we don't really know the nature of the application.
    
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_SCREENSAVER;

        //
        //
        // *** TEMPORARY *** work around AV/corruption issues from us deleting
        // system memory before we delete the system memory shared surface
        //
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_DISABLE_PSGP_THREADING; 
    }

    // Ensure that DX maintains our existing FPU state instead of clobbering it.
    // Note that this will cause DX to run in our FPU state, i.e., they will not set
    // to something else and restore.

    pD3DCreateParams->BehaviorFlags |= D3DCREATE_FPU_PRESERVE;

    // We are multithreaded unless MilRTInitialization::SingleThreadedUsage is specified
    if ((dwFlags & MilRTInitialization::SingleThreadedUsage) == 0)
    {
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_MULTITHREADED;
    }
    
#if DBG
    // We need to disable driver management to get the perf stats from dx

    if (IsTagEnabled(tagD3DStats))
    {
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT;
    }

#endif

    // Note: 3/04/2003 chrisra - Adding the check for 8 lights here.
    //  This is the minimum that we currently need, and the requirement
    //  shouldn't limit the cards we support.
    // Update: 3/15/2006 jordanpa - we no longer use HW lighting so
    //  we don't need the 8 restriction
    if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0 &&
        type != D3DDEVTYPE_SW // software device needs software processing
        )
    {
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

        // Use the pure device if it is available.  Using the pure device is a 
        // substantial working set improvement and theoretical execution speed
        // improvement.

        if (((caps.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0)
            && !IsTagEnabled(tagDisablePureDevice))
        {
            pD3DCreateParams->BehaviorFlags |= D3DCREATE_PUREDEVICE;
        }
    }
    else
    {
        pD3DCreateParams->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    pD3DCreateParams->MasterAdapterOrdinal = uAdapter;
    pD3DCreateParams->AdapterOrdinalInGroup = 0;
    pD3DCreateParams->NumberOfAdaptersInGroup = 1;

    pD3DCreateParams->RTInitFlags = dwFlags;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::ComposePresentParameters
//
//  Synopsis:
//      Takes MILCore's Initialize/Presentation parameters and translates them
//      into D3D present parameters.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::ComposePresentParameters(
    __in_ecount(1) D3DDISPLAYMODEEX const &displayMode,
    __in_ecount(1) D3DDeviceCreationParameters const &CreateParams,
    __out_ecount(1) D3DPRESENT_PARAMETERS *pD3DPresentParams
    )
{
    // Set Presentation Parameters
    pD3DPresentParams->BackBufferWidth = 1;
    pD3DPresentParams->BackBufferHeight = 1;
    
    ChooseTargetFormat(
        CreateParams.RTInitFlags,
        OUT &(pD3DPresentParams->BackBufferFormat)
        );

    pD3DPresentParams->Windowed = TRUE;
    pD3DPresentParams->FullScreen_RefreshRateInHz = 0;
        
    pD3DPresentParams->MultiSampleType = D3DMULTISAMPLE_NONE;
    pD3DPresentParams->MultiSampleQuality = 0;

    if ((CreateParams.RTInitFlags & MilRTInitialization::PresentRetainContents) ||
        IsTagEnabled(tagMILStepRendering))
    {
        pD3DPresentParams->SwapEffect = D3DSWAPEFFECT_COPY;
    }
    else
    {
        pD3DPresentParams->SwapEffect = D3DSWAPEFFECT_DISCARD;
    }

    pD3DPresentParams->BackBufferCount = 1;
    pD3DPresentParams->hDeviceWindow = CreateParams.hFocusWindow;
    pD3DPresentParams->EnableAutoDepthStencil = FALSE;
    pD3DPresentParams->AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    // 
    // Ensure that D3D never presents from one display adapter to the other.
    //
    pD3DPresentParams->Flags = 0;
    
    if ((CreateParams.RTInitFlags & MilRTInitialization::DisableDisplayClipping) == 0)
    {
        pD3DPresentParams->Flags |= D3DPRESENTFLAG_DEVICECLIP;
    }

    if (CreateParams.RTInitFlags & MilRTInitialization::PresentImmediately)
    {
        pD3DPresentParams->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
    else
    {
        pD3DPresentParams->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
    
    if ((CreateParams.RTInitFlags & MilRTInitialization::PresentUsingMask)  != MilRTInitialization::PresentUsingHal)
    {
        //
        // If we're presenting with GDI we need to have a lockable
        // backbuffer.
        //
        pD3DPresentParams->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    }

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::CreateNewDevice
//
//  Synopsis:
//      CreateNewDevice creates a new D3D device and a new CD3DDeviceLevel1 to
//      use the D3D device, then adds the device wrapper to the tracking list.
//
//  Return Value:
//      S_OK - New D3D device and wrapper were successfully created
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::CreateNewDevice(
    __inout_ecount(1)
    D3DDeviceCreationParameters *pD3DCreateParams,  // D3D Device creation
                                                    // parameters
    __inout_ecount(1)
    D3DPRESENT_PARAMETERS *pBasePresentParams,      // Base D3D Presentation
                                                    // parameters
    __in_xcount(pD3DCreateParams->NumberOfAdaptersInGroup)
    D3DDISPLAYMODEEX *rgDisplayModes,               // Array of display modes

    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
    )
{
    HRESULT hr = S_OK;

    Assert(m_fD3DLoaded);
    Assert(m_pID3D);
    Assert(m_pDisplaySet);

    *ppDeviceLevel1 = NULL;

    D3DPRESENT_PARAMETERS *rgPresentParams;

    IDirect3DDevice9 *pID3DDevice = NULL;
    IDirect3DDevice9Ex *pID3DDeviceEx = NULL;

    IDirect3D9Ex* pID3DEx = NULL;
    IGNORE_HR(m_pID3D->QueryInterface(IID_IDirect3D9Ex, (void**)&pID3DEx));

    DynArrayIA<D3DPRESENT_PARAMETERS, 4> drgPresentParams;

    rgPresentParams = pBasePresentParams;

    Assert(pBasePresentParams->Windowed);
    Assert(pD3DCreateParams->NumberOfAdaptersInGroup == 1);
    // When creating the implicit swap chain we
    //  always use a dummy of 1x1.
    Assert(rgPresentParams[0].BackBufferWidth == 1);
    Assert(rgPresentParams[0].BackBufferHeight == 1);

    //
    // Before trying to create a device (especially a fullscreen one which can
    // itself set the display modes) make sure it hasn't changed since we
    // acquired mode information.
    //

    if (m_pDisplaySet->DangerousHasDisplayStateChanged())
    {
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    {
        MtSetDefault(Mt(D3DDevice));

        {
            // In a sleep/resume transition, DX may leak D3D resources 
            // when calling CreateDeviceEx. The amount of memory leaked is between 2MB and 
            // 7MB per resume/sleep cycle. To work-around this issue we call GetAdapterDisplayMode 
            // just before trying to create the device. GetAdapterDisplayMode will also fail 
            // in that particular situation, but will not leak large amounts of memory.  Obviously, 
            // there is still a potential for this failure to not be discovered by 
            // GetAdapaterDisplayMode if the internal power state change happens between the
            // GetAdapterDisplayMode and CreateDeviceEx below. The only fix that will address 
            // that particular issue is in DX.
            
            D3DDISPLAYMODE displayMode;
            ZeroMemory(&displayMode, sizeof(displayMode));
            MIL_THR(m_pID3D->GetAdapterDisplayMode(
                pD3DCreateParams->AdapterOrdinal,
                &displayMode
                ));

            if (FAILED(hr))
            {
                if (!IsOOM(hr))
                {
                    // We should really check for specific HRESULTs here, but we cannot trust the DX documentation
                    // and therefore consider all failures besides E_OUTOFMEMORY as invalid display state.                
                    TraceTag((tagError, "D3D not in a good state before trying to create device. hr = %x. Returning WGXERR_DISPLAYSTATEINVALID", hr));
                    IFC(WGXERR_DISPLAYSTATEINVALID);
                }
                else
                {
                    IFC(hr);
                }
            }
        }
        

        if (pID3DEx)
        {            
            MIL_THR(pID3DEx->CreateDeviceEx(
                pD3DCreateParams->AdapterOrdinal,
                pD3DCreateParams->DeviceType,
                pD3DCreateParams->hFocusWindow,
                pD3DCreateParams->BehaviorFlags,
                rgPresentParams,
                NULL, // fullscreen display mode
                &pID3DDeviceEx
                ));
            if (SUCCEEDED(hr))
            {
                hr = pID3DDeviceEx->QueryInterface(IID_IDirect3DDevice9, (void**)&pID3DDevice);
                ReleaseInterface(pID3DDeviceEx);
            }
        }
        else
        {
            MIL_THR(m_pID3D->CreateDevice(
                pD3DCreateParams->AdapterOrdinal,
                pD3DCreateParams->DeviceType,
                pD3DCreateParams->hFocusWindow,
                pD3DCreateParams->BehaviorFlags,
                rgPresentParams,
                &pID3DDevice
                ));
        }

        //
        // We now use D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX which isn't available 
        // on most peoples builds, so we fail to create the dx device.  So, for now,
        // we fallback on older machines.
        //
        //

        if (hr == D3DERR_INVALIDCALL
            && (pD3DCreateParams->BehaviorFlags & D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX))
        {
            pD3DCreateParams->BehaviorFlags &= ~D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;

            if (pID3DEx)
            {
                MIL_THR(pID3DEx->CreateDeviceEx(
                    pD3DCreateParams->AdapterOrdinal,
                    pD3DCreateParams->DeviceType,
                    pD3DCreateParams->hFocusWindow,
                    pD3DCreateParams->BehaviorFlags,
                    rgPresentParams,
                    NULL, // fullscreen display mode
                    &pID3DDeviceEx
                    ));
                if (SUCCEEDED(hr))
                {
                    hr = pID3DDeviceEx->QueryInterface(IID_IDirect3DDevice9, (void**)&pID3DDevice);
                    ReleaseInterface(pID3DDeviceEx);
                }
            }
            else
            {
                MIL_THR(m_pID3D->CreateDevice(
                    pD3DCreateParams->AdapterOrdinal,
                    pD3DCreateParams->DeviceType,
                    pD3DCreateParams->hFocusWindow,
                    pD3DCreateParams->BehaviorFlags,
                    rgPresentParams,
                    &pID3DDevice
                    ));
            }
        }

        if (FAILED(hr))
        {
            TRACE_DEVICECREATE_FAILURE(
                pD3DCreateParams->AdapterOrdinal,
                "Failed to create d3d device",  
                hr
                );

            if (hr == D3DERR_DEVICELOST)
            {
                MIL_THR(WGXERR_DISPLAYSTATEINVALID);
            }
        }
        else
        {
            for (size_t i = 0; i < m_rgAdapterStatusListeners.GetCount(); i++)
            {
                m_rgAdapterStatusListeners[i]->NotifyAdapterStatus(
                    pD3DCreateParams->AdapterOrdinal, 
                    true        // fIsValid
                    );
            }
        }
    }

Cleanup:

    ReleaseInterface(pID3DEx);

#if DBG
    // Note that device creation is in process incase the device becomes
    // unusable during initialization; so that UnusableNotification won't
    // assert that this new device is not yet in its tracking list. 
    m_fDbgCreatingNewDevice = true;
#endif

    CD3DDeviceLevel1 *pDeviceLevel1 = NULL;

    if (SUCCEEDED(hr))
    {
        MIL_THR(CD3DDeviceLevel1::Create(
            pID3DDevice,
            m_pDisplaySet->Display(pD3DCreateParams->AdapterOrdinal),
            this,
            pD3DCreateParams->BehaviorFlags,
            &pDeviceLevel1
            ));
    }

    if (SUCCEEDED(hr))
    {
        for (UINT i = 0; SUCCEEDED(hr) && i < pD3DCreateParams->NumberOfAdaptersInGroup; i++)
        {
            MIL_THR(pDeviceLevel1->CheckRenderTargetFormat(rgPresentParams[i].BackBufferFormat));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Make sure there is space for the new entry
        MIL_THR(m_rgDeviceList.ReserveSpace(1));
    }

    if (SUCCEEDED(hr))
    {
        UINT iNewLast = m_rgDeviceList.GetCount();

        // Increase the length
        m_rgDeviceList.SetCount(iNewLast+1);

        // Move the first unusable entry to the end if needed
        if (m_iFirstUnusable < iNewLast)
        {
            m_rgDeviceList[iNewLast] = m_rgDeviceList[m_iFirstUnusable];
        }
        else
        {
            Assert(m_iFirstUnusable == iNewLast);
        }

        // Place the new entry in at the end of the usable list
        m_rgDeviceList[m_iFirstUnusable].pDeviceLevel1 = pDeviceLevel1;
        m_rgDeviceList[m_iFirstUnusable].CreateParams  = *pD3DCreateParams;
        m_rgDeviceList[m_iFirstUnusable].fIsDeviceLost = false;
        #if DBG
            // Hang on to the master (or only) present parameter for dbg info.
            // It may only be used for asserts.
            m_rgDeviceList[m_iFirstUnusable].DbgPresentParams = rgPresentParams[0];
        #endif

        m_iFirstUnusable++;

        // All tracked devices are potential non-static method callers
        m_cCallers++;

        //
        // Return the value and assign the reference from
        // creation to the returned pointer.
        //

        *ppDeviceLevel1 = pDeviceLevel1;
    }
    else
    {
        if (pDeviceLevel1)
        {
            //
            // This device was created but we were unable to track it; so,
            // Release to get the count back to zero and then delete to
            // actually free it.  During the Release this manager will
            // be called, but won't be able to find this resource in its
            // list and therefore won't delete it.
            //

            Verify(pDeviceLevel1->Release() == 0);
            delete pDeviceLevel1;
        }
    }

#if DBG
    // Device creation and tracking is complete
    m_fDbgCreatingNewDevice = false;
#endif

    ReleaseInterfaceNoNULL(pID3DDevice);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::AddAdapterStatusListener
//
//  Synopsis:
//      Adds listener to the list. 
//
//------------------------------------------------------------------------------

HRESULT 
CD3DDeviceManager::AddAdapterStatusListener(__in IAdapterStatusListener *pListener)
{
    CGuard<CCriticalSection> oGuard(m_csManagement);
    
    RRETURN(m_rgAdapterStatusListeners.Add(pListener));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::RemoveAdapterStatusListener
//
//  Synopsis:
//      Removes listener to the list. 
//
//------------------------------------------------------------------------------

void 
CD3DDeviceManager::RemoveAdapterStatusListener(__in IAdapterStatusListener *pListener)
{
    CGuard<CCriticalSection> oGuard(m_csManagement);
        
    m_rgAdapterStatusListeners.Remove(pListener);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::NotifyDeviceLost
//
//  Synopsis:
//      Notifies all listeners that the device in "info" has been lost. This should be called
//      whenever a device is being destroyed. fIsDeviceLost protects us from incorrectly
//      overnotifying in the multi-window case. For example, if one window isn't presenting
//      when device lost happens then it won't know that its device is lost until it presents and
//      in the mean time the same device may have already been lost and recreated
//      by another presenting window. We don't want to send another lost message since 
//      rendering for that adapter has been restored.
//
//------------------------------------------------------------------------------

void
CD3DDeviceManager::NotifyDeviceLost(D3DDeviceInformation &info)
{
    if (!info.fIsDeviceLost)
    {
        info.fIsDeviceLost = true;

        for (size_t i = 0; i < m_rgAdapterStatusListeners.GetCount(); ++i)
        {
            m_rgAdapterStatusListeners[i]->NotifyAdapterStatus(
                info.CreateParams.AdapterOrdinal, 
                false       // fIsValid
                );
        }
    }
}







