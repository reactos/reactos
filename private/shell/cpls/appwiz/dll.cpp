/***************************************************************************
 *  dll.c
 *
 *  Standard DLL entry-point functions 
 *
 ***************************************************************************/

// Note: Proxy/Stub Information from ATL
//
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for shappmgr.idl by adding the following 
//      files to the Outputs.
//          shappmgr_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f shappmgrps.mk in the project directory.


#include "priv.h"
#include "sccls.h"

#ifndef DOWNLEVEL
#include "adcctl.h"
#endif //DOWNLEVEL

#include <ntverp.h>
#include <advpub.h>         // For REGINSTALL

// Define GUIDs


extern "C" 
{
HINSTANCE g_hinst = NULL;

int g_cxIcon;
int g_cyIcon;
BOOL g_bMirroredOS;
LONG    g_cRefThisDll = 0;      // per-instance

#ifdef WX86
//
// from uninstal.c
//
extern BOOL bWx86Enabled;
BOOL IsWx86Enabled(VOID);
#endif

};

#ifndef DOWNLEVEL
CComModule _Module;         // ATL module object

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ADCCtl, CADCCtl)
END_OBJECT_MAP()
#endif //DOWNLEVEL

/*----------------------------------------------------------
Purpose: DllEntryPoint

*/
BOOL 
APIENTRY 
DllMain(
    IN HINSTANCE hinst, 
    IN DWORD dwReason, 
    IN LPVOID lpReserved)
{
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    switch(dwReason) 
    {
    case DLL_PROCESS_ATTACH:
#ifndef DOWNLEVEL
        _Module.Init(ObjectMap, hinst);
#endif //DOWNLEVEL
        DisableThreadLibraryCalls(hinst);
     
#ifdef DEBUG
        CcshellGetDebugFlags();
        
        if (g_dwBreakFlags & BF_ONDLLLOAD)
            DebugBreak();
#endif

        g_hinst = hinst;

        g_cxIcon = GetSystemMetrics(SM_CXICON);
        g_cyIcon = GetSystemMetrics(SM_CYICON);
        g_bMirroredOS = IS_MIRRORING_ENABLED();
#ifdef WX86
        bWx86Enabled = IsWx86Enabled();
#endif
        break;
        

    case DLL_PROCESS_DETACH:
#ifndef DOWNLEVEL
        _Module.Term();
#endif //DOWNLEVEL
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // We shouldn't get these because we called 
        // DisableThreadLibraryCalls(). 
        ASSERT_MSG(0, "DllMain received DLL_THREAD_ATTACH/DETACH!  We're not expecting this.");
        break;

    default:
        break;
    } 

    return TRUE;
} 


/*----------------------------------------------------------
Purpose: This function provides the DLL version info.  This 
         allows the caller to distinguish running NT SUR vs.
         Win95 shell vs. Nashville, etc.

         The caller must GetProcAddress this function.  

Returns: NO_ERROR
         ERROR_INVALID_PARAMETER if pinfo is invalid

*/

// All we have to do is declare this puppy and CCDllGetVersion does the rest
DLLVER_DUALBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);

/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.
*/
HRESULT _CallRegInstall(LPCSTR szSection, BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
#ifdef WINNT                
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(g_hinst, szSection, &stReg);
#else
            hr = pfnri(g_hinst, szSection, NULL);
#endif                
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;
    HRESULT hrExternal = S_OK;
    ASSERT(IS_VALID_STRING_PTRW(pszCmdLine, -1));

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    if (bInstall)
    {
        // Delete any old registration entries, then add the new ones.
        // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
        // (The inf engine doesn't guarantee DelReg/AddReg order, that's
        // why we explicitly unreg and reg here.)
        //
        hr = THR(_CallRegInstall("RegDll", FALSE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;
    }
    else
    {
        hr = THR(_CallRegInstall("UnregDll", TRUE));
        if (SUCCEEDED(hrExternal))
            hrExternal = hr;
    }

    return hrExternal;
}


/*----------------------------------------------------------
Purpose: Returns a class factory to create an object of
         the requested type.

*/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TraceMsg(TF_OBJLIFE, "DllGetClassObject called with riid=%x (%x)", riid, &riid);

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif

    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        // Try our native class factory
        HRESULT hres = GetClassObject(rclsid, riid, ppv);

#ifndef DOWNLEVEL
        if (FAILED(hres))
        {
            // Try the ATL class factory
            hres = _Module.GetClassObject(rclsid, riid, ppv);
        }
#endif //DOWNLEVEL

        return hres;
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif

#ifndef DOWNLEVEL
    // This component uses ATL and natively-implemented COM objects
    if (0 != g_cRefThisDll || 0 != _Module.GetLockCount())
        return S_FALSE;
#endif //DOWNLEVEL

    TraceMsg(DM_TRACE, "DllCanUnloadNow returning S_OK (bye, bye...)");
    return S_OK;
}


STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
    ASSERT(g_cRefThisDll < 1000);   // reasonable upper limit
}


STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);
    ASSERT(g_cRefThisDll >= 0);      // don't underflow
}


STDAPI DllRegisterServer(void)
{
    HRESULT hres = S_OK;
    
#ifdef _MERGE_PROXYSTUB
    hres = THR(PrxDllRegisterServer());
    if (FAILED(hres))
        return hres;
#endif

#ifndef DOWNLEVEL
    // registers object, typelib and all interfaces in typelib
    hres = THR(_Module.RegisterServer(TRUE));
#endif //DOWNLEVEL

    return hres;
}


STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif

#ifndef DOWNLEVEL
    _Module.UnregisterServer();
#endif //DOWNLEVEL
    return S_OK;
}
