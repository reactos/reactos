// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Maintains primary references to D3D module and Sw
//                   rasterizer (as needed).
//

#include "precomp.hpp"
#include "osversionhelper.h"

//#define SECONDARY_DOWNLEVEL_RGB9RAST_MODULE TEXT("RGB9Rast_1.dll")


//#define DdQueryDisplaySettingsUniqueness    GdiEntry13
//
extern "C" typedef ULONG
(APIENTRY
*PDdQueryDisplaySettingsUniqueness)(
    VOID
    );

PDdQueryDisplaySettingsUniqueness pDdQueryDisplaySettingsUniqueness =
				(PDdQueryDisplaySettingsUniqueness) GetProcAddress(GetModuleHandle(TEXT("Gdi32.dll")), "GdiEntry13");

//+------------------------------------------------------------------------
//
//  Function:  CD3DLoader::GetLoadRef
//
//  Synopsis:  Increase Direct3D load reference for this module.
//
//-------------------------------------------------------------------------
void
CD3DLoader::GetLoadRef(
    )
{
    g_DisplayManager.IncrementUseRef();
    return;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DLoader::ReleaseLoadRef
//
//  Synopsis:  Handle release of Direct3D load reference for this module.
//
//-------------------------------------------------------------------------
void
CD3DLoader::ReleaseLoadRef(
    )
{
    g_DisplayManager.DecrementUseRef();
}



class CD3DModuleLoaderInternal
{
public:

    CD3DModuleLoaderInternal();
    ~CD3DModuleLoaderInternal();

private:

    friend CD3DModuleLoader;

    HRESULT Init();

    HRESULT LoadD3DModule();

    void CleanupD3DReferences();

    HRESULT LoadSwRastModule();

    void CleanupSwRastReferences();

    HRESULT CreateD3DObjects(
        __deref_out_ecount(1) IDirect3D9 **ppID3D,
        __deref_out_ecount(1) IDirect3D9Ex **ppID3DEx
        );

    void ReleaseD3DLoadRef();

    HRESULT RegisterSoftwareDevice(
        __inout_ecount(1) IDirect3D9 *pID3D
        );

private:

    typedef IDirect3D9 * (WINAPI *DIRECT3DCREATE9FUNCTION)(UINT SDKVersion);
    typedef HRESULT (WINAPI *DIRECT3DCREATE9EXFUNCTION)(UINT SDKVersion, IDirect3D9Ex**);

    CCriticalSection m_csManagement;

    HRESULT m_hrD3DModuleLoad;
    HMODULE m_hD3D;
    DIRECT3DCREATE9FUNCTION m_pfnDirect3DCreate9;
    DIRECT3DCREATE9EXFUNCTION m_pfnDirect3DCreate9Ex;
    ULONG m_cD3DRefs;

    HRESULT m_hrSwRastModuleLoad;
    HMODULE m_hSwRast;
    FARPROC m_pfnSwRastD3D9GetSWInfo;
};

// This object's scope is limited to this file.  Any interaction
// with it should go through the CD3DModuleLoader class.
static CD3DModuleLoaderInternal g_D3DModuleLoader;


//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoader::Startup
//
//  Synopsis:  Initialize global D3D module loader
//
//-------------------------------------------------------------------------
HRESULT
CD3DModuleLoader::Startup()
{
    return g_D3DModuleLoader.Init();
}
    
//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoader::Shutdown
//
//  Synopsis:  Uninitialize global D3D module loader
//
//-------------------------------------------------------------------------
void
CD3DModuleLoader::Shutdown()
{
    g_D3DModuleLoader.CleanupD3DReferences();
}


//+------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoader::CreateD3DObjects
//
//  Synopsis:
//      Creates top level D3D object.
//      Increments the Direct3D module load count.
//
//-------------------------------------------------------------------------
HRESULT
CD3DModuleLoader::CreateD3DObjects(
    __deref_out_ecount(1) IDirect3D9 **ppID3D,
    __deref_out_ecount(1) IDirect3D9Ex **ppID3DEx
    )
{    
    return g_D3DModuleLoader.CreateD3DObjects(ppID3D, ppID3DEx);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoader::ReleaseD3DLoadRef
//
//  Synopsis:
//      Handle release of Direct3D load reference for this module.
//
//-------------------------------------------------------------------------
void
CD3DModuleLoader::ReleaseD3DLoadRef(
    )
{
    g_D3DModuleLoader.ReleaseD3DLoadRef();
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoader::RegisterSoftwareDevice
//
//  Synopsis:
//      Register a software rasterizer for given ID3D.
//
//  Note:
//      It is a caller responsibility not to call this method
//      many times against the same pID3D.
//
//-----------------------------------------------------------------------------

HRESULT
CD3DModuleLoader::RegisterSoftwareDevice(
    __inout_ecount(1) IDirect3D9 *pID3D
    )
{
    return g_D3DModuleLoader.RegisterSoftwareDevice(pID3D);
}

//+----------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoader::GetDisplayUniqueness
//
//  Synopsis:  Returns the current display uniqueness value
//
//-----------------------------------------------------------------------------

ULONG
CD3DModuleLoader::GetDisplayUniqueness(
    )
{
    return pDdQueryDisplaySettingsUniqueness();
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoaderInternal::CD3DModuleLoaderInternal
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DModuleLoaderInternal::CD3DModuleLoaderInternal()
{
    m_hrD3DModuleLoad = WGXERR_NOTINITIALIZED;
    m_hD3D = NULL;
    m_pfnDirect3DCreate9 = NULL;
    m_pfnDirect3DCreate9Ex = NULL;
    m_cD3DRefs = 0;
    m_hrSwRastModuleLoad = WGXERR_NOTINITIALIZED;
    m_hSwRast = NULL;
    m_pfnSwRastD3D9GetSWInfo = NULL;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoaderInternal::~CD3DModuleLoaderInternal
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DModuleLoaderInternal::~CD3DModuleLoaderInternal()
{
    CleanupD3DReferences();
    m_csManagement.DeInit();
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoaderInternal::Init
//
//  Synopsis:  Initialize basic members
//
//-------------------------------------------------------------------------
HRESULT 
CD3DModuleLoaderInternal::Init()
{
    HRESULT hr = S_OK;

    Assert(!m_csManagement.IsValid());
    Assert(m_hrD3DModuleLoad == WGXERR_NOTINITIALIZED);

    hr = m_csManagement.Init();

    Assert(m_csManagement.IsValid() || FAILED(hr));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoaderInternal::LoadD3DModule
//
//  Synopsis:  Initialize static D3D pointers and references
//
//-------------------------------------------------------------------------
#define USE_MESSAGEBOX_FOR_D3DINIT_ERRORS   0

#if USE_MESSAGEBOX_FOR_D3DINIT_ERRORS
#define D3DINIT_ERR(msg) \
    MessageBox(NULL, TEXT(msg), TEXT("Initialization Error"), MB_OK | MB_ICONERROR)
#else
#if DBG
#define D3DINIT_ERR(msg) \
    AssertMsg(0, msg)
#else
#define D3DINIT_ERR(msg) \
    OutputDebugString( TEXT("WARNING: MILCore: ") TEXT(msg) TEXT("\n") )
#endif DBG
#endif USE_MESSAGEBOX_FOR_D3DINIT_ERRORS

HRESULT
CD3DModuleLoaderInternal::LoadD3DModule(
    )
{
    HRESULT hr = S_OK;

    Assert(m_hrD3DModuleLoad == WGXERR_NOTINITIALIZED);

    Assert(!m_hD3D);
    Assert(!m_pfnDirect3DCreate9);
    Assert(!m_pfnDirect3DCreate9Ex);
    
    Assert(!SUCCEEDED(m_hrSwRastModuleLoad));
    Assert(!m_hSwRast);
    Assert(!m_pfnSwRastD3D9GetSWInfo);


    //
    // Load the D3D module
    //

    MIL_TW32_NOSLE(m_hD3D = LoadLibrary(TEXT("d3d9.dll")));
    if (FAILED(hr))
    {
        OutputDebugString(TEXT("WARNING: MILCore: Direct3D 9 is not installed or load failed.\n"));
    }
    else
    {
        Assert(m_hD3D);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get creation function address
        //

        m_pfnDirect3DCreate9Ex = (DIRECT3DCREATE9EXFUNCTION)GetProcAddress(m_hD3D, "Direct3DCreate9Ex");

        MIL_TW32_NOSLE(m_pfnDirect3DCreate9 =
            (DIRECT3DCREATE9FUNCTION)
                GetProcAddress(m_hD3D, "Direct3DCreate9")
                );

        if (FAILED(hr))
        {
            D3DINIT_ERR("Unable to locate Direct3DCreate9.\nPlease check for proper Direct3D 9 installation.");
        }
        else
        {
            Assert(m_pfnDirect3DCreate9);
        }
    }

    // Remember the result
    m_hrD3DModuleLoad = hr;

    if (FAILED(hr))
    {
        // Clean up for error
        if (m_hD3D)
        {
            IGNORE_W32(0, FreeLibrary(m_hD3D));
            m_hD3D = NULL;
        }
    }
    else
    {
        Assert(m_hD3D);
        Assert(m_pfnDirect3DCreate9);
    }

    Assert(m_hrD3DModuleLoad != WGXERR_NOTINITIALIZED);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoaderInternal::CreateD3DObjects
//
//  Synopsis:
//      Creates top level D3D object.
//      Increments the Direct3D module load count.
//
//-------------------------------------------------------------------------
HRESULT 
CD3DModuleLoaderInternal::CreateD3DObjects(
    __deref_out_ecount(1) IDirect3D9 **ppID3D,
    __deref_out_ecount(1) IDirect3D9Ex **ppID3DEx
    )
{
    HRESULT hr = S_OK;
    IDirect3D9 *pID3D = NULL;
    IDirect3D9Ex *pID3DEx = NULL;

    FPUStateSandbox oGuard;
    
    // Direct3DCreate9 causes a snowball of loading of various dlls,
    // depending on particular machine configuration.
    // From time to time we are facing with wrong code in some
    // unrelated executable that breaks FPU settings.
    // For instance, it can happen while executing the constructor
    // for static instance of some class (called by DllMain).
    // These issues should be investigated separately, but we
    // don't want our code to suffer. The cost of guard is
    // negligible, so we concluded that it would be better
    // to keep it forever.

    {
        CGuard<CCriticalSection> oGuard(m_csManagement);

        m_cD3DRefs++;

        if (m_hrD3DModuleLoad == WGXERR_NOTINITIALIZED)
        {
            IFC(LoadD3DModule());
        }
        else
        {
            IFC(m_hrD3DModuleLoad);
        }
    }

    if (m_pfnDirect3DCreate9Ex)
    {
        hr = m_pfnDirect3DCreate9Ex(D3D_SDK_VERSION, &pID3DEx);

        //
        // D3DERR_NOTAVAILABLE means either the SDK version was mismatched or
        // the driver is XPDM even though we are on Vista, in the latter case
        // we want to go on to make the down-level call and we will return 
        // WGXERR_UNSUPPORTEDVERSION if it fails anyway.
        // 
        if (D3DERR_NOTAVAILABLE == hr) 
        {
            hr = S_OK;
        }

        //
        // Any other failures should be considered hard failures. Bail out now.
        // 
        IFC(hr);

        if (pID3DEx != NULL)
        {
            IGNORE_HR(pID3DEx->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&pID3D)));
        }        
    }

    //
    // XPDM drivers on Vista can still return failure for the D3D9EX call, so,
    // try the downlevel call.
    //
    if (!pID3D) 
    {
        pID3D = TFAIL(NULL, m_pfnDirect3DCreate9(D3D_SDK_VERSION));
    }

    if (!pID3D)
    {
        //
        // If down-level create failed there are two possibilities:
        //  1) the version doesn't match or
        //  2) there isn't enough memory.
        // Since D3D doesn't provide any further information, we
        // guess that it is the former - definitely more common
        // during development.
        //
        TraceTag((tagMILWarning, "Direct3D 9 SDK version doesn't match."));

        IFC(WGXERR_UNSUPPORTEDVERSION);
    }

    *ppID3D = pID3D;
    pID3D = NULL;

    *ppID3DEx = pID3DEx;
    pID3DEx = NULL;

Cleanup:

    ReleaseInterfaceNoNULL(pID3DEx);
    ReleaseInterfaceNoNULL(pID3D);

    if (FAILED(hr))
    {
        CGuard<CCriticalSection> oGuard(m_csManagement);

        if (--m_cD3DRefs == 0)
        {
            CleanupD3DReferences();
        }
    }
    
    RRETURN(hr);
}
    
//+------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoaderInternal::ReleaseD3DLoadRef
//
//  Synopsis:
//      Handle release of Direct3D load reference for this module.
//
//-------------------------------------------------------------------------
void
CD3DModuleLoaderInternal::ReleaseD3DLoadRef(
    )
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    Assert(m_cD3DRefs > 0);

    if (--m_cD3DRefs == 0)
    {
        CleanupD3DReferences();
    }

    return;
}



//+------------------------------------------------------------------------
//
//  Function:  CD3DModuleLoaderInternal::CleanupD3DReferences
//
//  Synopsis:  Release all D3D references
//
//-------------------------------------------------------------------------
void 
CD3DModuleLoaderInternal::CleanupD3DReferences()
{
    Assert(m_cD3DRefs == 0);

    if (SUCCEEDED(m_hrD3DModuleLoad))
    {
        Assert(m_pfnDirect3DCreate9);
        m_pfnDirect3DCreate9 = NULL;
        m_pfnDirect3DCreate9Ex = NULL;

        Assert(m_hD3D);
        IGNORE_W32(0, FreeLibrary(m_hD3D));
        m_hD3D = NULL;

        m_hrD3DModuleLoad = WGXERR_NOTINITIALIZED;

        CleanupSwRastReferences();
    }
    else
    {
        Assert(!m_hD3D);
        Assert(!m_pfnDirect3DCreate9);

        Assert(!m_hSwRast);
        Assert(!m_pfnSwRastD3D9GetSWInfo);
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:  GetSwRastModuleName
//
//  Synopsis:  Return name of RGB rast module to load for this environment
//

__outro PCTSTR
GetSwRastModuleName(
    )
{
    // Future Consideration:   Key RGBRast module name off secure reg key
    //  because OSVersion may be shimmed for app compat.
    C_ASSERT( WIN32_VISTA_MINORVERSION == 0 );
    return (WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater()) ?
        TEXT("RGB9Rast.dll") :
        TEXT("RGB9Rast_2.dll");
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DModuleLoaderInternal::LoadSwRastModule
//
//  Synopsis:  Try to load RGB rasterizer DLL
//

HRESULT
CD3DModuleLoaderInternal::LoadSwRastModule(
    )
{
    HRESULT hr = S_OK;

    Assert(m_hrSwRastModuleLoad == WGXERR_NOTINITIALIZED);
    Assert(!m_hSwRast);
    Assert(!m_pfnSwRastD3D9GetSWInfo);

    //
    // Load the RGBRast module
    //

#ifndef SECONDARY_DOWNLEVEL_RGB9RAST_MODULE
    IFCW32_NOSLE(m_hSwRast = LoadLibrary(GetSwRastModuleName()));
#else
    MIL_TW32_NOSLE(m_hSwRast = LoadLibrary(GetSwRastModuleName()));

    if (FAILED(hr))
    {
        if (WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater())
        {
            goto Cleanup;
        }

        //
        // Try to load secondary RGB9Rast module.  If it fails use error
        // from attempt to load primary module.
        //

        TW32(0, m_hSwRast = LoadLibrary(SECONDARY_DOWNLEVEL_RGB9RAST_MODULE));

        if (!m_hSwRast)
        {
            goto Cleanup;
        }

        hr = S_OK;
    }

    Assert(m_hSwRast);
#endif

    //
    // Get register function address
    //

    IFCW32_NOSLE(m_pfnSwRastD3D9GetSWInfo = GetProcAddress(m_hSwRast, "D3D9GetSWInfo"));

Cleanup:

    if (FAILED(hr))
    {
        if (m_hSwRast)
        {
            IGNORE_W32(0, FreeLibrary(m_hSwRast));
            m_hSwRast = NULL;
        }
#ifdef PRERELEASE
        //
        // RGBRast is maintained by DX and does not ship with WPF.msi. A lot of people
        // are hitting this while self hosting and it is difficult for them to debug.
        // In shipping versions of WinFX, this is not an issue as the RGBRast is included.
        //
        // The DLL can be obtained from the external drop share:
        // \\vblwcpbuilds\release\EXTERNAL\downLevel\vbl_wcp_avalon
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND))
        {
            FreRIPW(L"Missing RGBRast.dll. Please obtain RGB9Rast.dll for Vista or RGB9Rast_2.dll for other OS versions, available at \\\\vblwcpbuilds\\release\\EXTERNAL\\downLevel\\vbl_wcp_avalon");
        }
#endif
    }

    //
    // Remember result because we don't expect it to ever change and continuous
    // load library attempts can be costly.
    //

    m_hrSwRastModuleLoad = hr;
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DModuleLoaderInternal::CleanupSwRastReferences
//
//  Synopsis:  Release all RGB rasterizer references
//

void
CD3DModuleLoaderInternal::CleanupSwRastReferences(
    )
{
    Assert(m_cD3DRefs == 0);

    if (SUCCEEDED(m_hrSwRastModuleLoad))
    {
        Assert(m_pfnSwRastD3D9GetSWInfo);
        m_pfnSwRastD3D9GetSWInfo = NULL;

        Assert(m_hSwRast);
        IGNORE_W32(0, FreeLibrary(m_hSwRast));
        m_hSwRast = NULL;

        m_hrSwRastModuleLoad = WGXERR_NOTINITIALIZED;
    }
    else
    {
        Assert(!m_hSwRast);
        Assert(!m_pfnSwRastD3D9GetSWInfo);
    }

    return;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DModuleLoaderInternal::RegisterSoftwareDevice
//
//  Synopsis:
//      Register a software rasterizer for given ID3D.
//
//  Note:
//      It is a caller responsibility not to call this method
//      many times against the same pID3D.
//

HRESULT
CD3DModuleLoaderInternal::RegisterSoftwareDevice(
    __inout_ecount(1) IDirect3D9 *pID3D
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(m_csManagement);

    //
    // Make sure Sw rasterizer module is loaded
    //

    if (m_hrSwRastModuleLoad == WGXERR_NOTINITIALIZED)
    {
        IFC(LoadSwRastModule());
    }
    else
    {
        IFC(m_hrSwRastModuleLoad);
    }

    //
    // Register
    //

    IFC(pID3D->RegisterSoftwareDevice(m_pfnSwRastD3D9GetSWInfo));

Cleanup:

    RRETURN(hr);
}





