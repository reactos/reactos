//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// dll.cpp 
//
//   Exported Dll functions.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "dll.h"
#include "clsfact.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "chanapi.h"
#include "persist.h"
#include "cdfview.h"
#include "iconhand.h"
#include "chanmgrp.h"
#include "chanmgri.h"
#include "chanmenu.h"
#include "proppgs.h"
#include <advpub.h>     // Self registration helper.
#include <olectl.h>     // SELFREG_E_CLASS definition.
#include <comcat.h>     // Catagory registration.

#define MLUI_INIT
#include <mluisupp.h>

BOOL g_bRunningOnNT = FALSE;

void DLL_ForcePreloadDlls(DWORD dwFlags)
{
    //
    // CoLoadLibrary is getting called here to add an extra reference count
    // to a COM dll so that it doesn't get unloaded by COM before we are through
    // with it.  This problem occurs since our object gets created on
    // one thread and then passed along to another where we instantiate an
    // COM object.  The secondary thread isn't guaranteed to have called
    // CoInitialize so we call it, then call CoCreateInstance then call 
    // CoUnitialize to clean up.  The side effect of all this is that dlls
    // are being unloaded while we still have references to them. 
    //
    if ((dwFlags & PRELOAD_MSXML) && !g_msxmlInst)
    {
        g_msxmlInst = CoLoadLibrary(L"msxml.dll", FALSE); // Not much we can if
                                                          // this fails
    }

#ifndef UNIX
    /* Unix does not use webcheck */
    if ((dwFlags & PRELOAD_WEBCHECK) && !g_webcheckInst)
    {
        g_webcheckInst = CoLoadLibrary(L"webcheck.dll", FALSE);
    }
#endif /* UNIX */
}

//
// Exported Functions.
//


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllMain ***
//
//    Dll entry point.
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
BOOL
WINAPI DllMain(
    HANDLE hInst,
    DWORD dwReason,
    LPVOID pReserved
)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        //
        // This is a hack to fix an oleaut32.dll bug.  If the oleaut32.dll is
        // loaded, then unloaded, then reloaded in the same process its heap
        // can become corrupt.  This Loadlibrary will ensure that oleaut32
        // stays loaded once cdfview is ever loaded.
        //

        LoadLibrary(TEXT("oleaut32.dll"));
        
        DisableThreadLibraryCalls((HINSTANCE)hInst);

        g_hinst = (HINSTANCE)hInst;
        GetModuleFileName(g_hinst, g_szModuleName, ARRAYSIZE(g_szModuleName));
        MLLoadResources(g_hinst, TEXT("cdfvwlc.dll"));

        Cache_Initialize();

        //
        // Read the debug flags defined in ShellExt.ini.  The filename, section
        // to read, and flag variables are defined in debug.cpp.
        //

        #ifdef DEBUG
        #ifndef UNIX
        CcshellGetDebugFlags();
        #endif /* UNIX */
        #endif

        g_bRunningOnNT = IsOS(OS_NT);

    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        MLFreeResources(g_hinst);

        //
        // REVIEW:  Clearing the cache on DLL unload.
        //

        Cache_Deinitialize(); 


        if (g_msxmlInst)
            CoFreeLibrary(g_msxmlInst);

        TraceMsg(TF_OBJECTS, "cdfview.dll unloaded!");
    }

    return TRUE;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllCanUnloadNow ***
//
//    Determines whether this DLL is in use. If not, the caller can safely
//    unload the DLL from memory. 
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI DllCanUnloadNow(
    void
)
{
    HRESULT hr;

    if (0 == g_cDllRef)
    {
        Cache_FreeAll();
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    TraceMsg(TF_OBJECTS, "DllCanUnloadNow returned %s",
             hr == S_OK ? TEXT("TRUE") : TEXT("FALSE"));

    return g_cDllRef ? S_FALSE : S_OK;
}

//
// Create functions used by the class factory.
//

#define DEFINE_CREATEINSTANCE(cls, iface)                 \
HRESULT cls##_Create(IUnknown **ppIUnknown)               \
{                                                         \
    ASSERT(NULL != ppIUnknown);                           \
    *ppIUnknown = (iface *)new cls;                       \
    return (NULL != *ppIUnknown) ? S_OK : E_OUTOFMEMORY;  \
}

DEFINE_CREATEINSTANCE(CCdfView,       IShellFolder);
DEFINE_CREATEINSTANCE(CChannelMgr,    IChannelMgr);
DEFINE_CREATEINSTANCE(CIconHandler,   IExtractIcon);
DEFINE_CREATEINSTANCE(CChannelMenu,   IContextMenu);
DEFINE_CREATEINSTANCE(CPropertyPages, IShellPropSheetExt);


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllGetClassObject ***
//
//     Retrieves the class factory object for the cdf viewer.    
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppvObj
)
{
    //
    // Table used to pass the correct create function to the class factory.
    //

    static const struct _tagCLASSFACT {
        GUID const* pguid;
        CREATEPROC  pfn;
    } aClassFact[] = { {&CLSID_CDFVIEW,        CCdfView_Create},
                       {&CLSID_CDFINI,         CCdfView_Create},
                       {&CLSID_ChannelMgr,     CChannelMgr_Create},
                       {&CLSID_CDFICONHANDLER, CIconHandler_Create},
                       {&CLSID_CDFMENUHANDLER, CChannelMenu_Create},
                       {&CLSID_CDFPROPPAGES,   CPropertyPages_Create} };

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    ASSERT(ppvObj);

    *ppvObj = NULL;

    for (int i = 0; i < ARRAYSIZE(aClassFact); i++)
    {
        if (rclsid == *aClassFact[i].pguid)
        {
            CCdfClassFactory *pCdfClassFactory =
                                        new CCdfClassFactory(aClassFact[i].pfn);

            if (pCdfClassFactory)
            {
                hr = pCdfClassFactory->QueryInterface(riid, ppvObj);

                //
                // The 'new' created a class factory with a ref count of one.  The
                // above QueryInterface incremented the ref count by one or failed.
                // In either case the ClassFactory ref count should be decremented.
                //

                pCdfClassFactory->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
            break;
        }
    }

    ASSERT((SUCCEEDED(hr) && *ppvObj) || (FAILED(hr) && NULL == *ppvObj));

    return hr;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllRegisterServer ***
//
//     Self register the cdf viewer.    
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI DllRegisterServer(
    void
)
{
    HRESULT hr;

    //
    // Unregister previous versions of this control.
    //

    DllUnregisterServer();

    //
    // REVIEW this should be called at install time
    //
    DllInstall(TRUE, NULL);

    //
    // RegisterServerHelper uses advpack.dll to add registry entries using
    // entries found in the .rc.
    //

    //
    // REVIEW : Use #defines for "Reg" and "Unreg"
    //

    hr = RegisterServerHelper("Reg");
    
    if (SUCCEEDED(hr))
    {
        //
        // Register as a browseable shell extension.  This will allow a user
        // to type the path of a cdf in the address bar and browse to the cdf
        // in place. This call adds an entry to the HKCR\CLSID\CLSID_CDFVIEW
        // \Implemented Catagories key.
        //

        ICatRegister *pICatRegister;

        HRESULT hr2 = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_ICatRegister,
                                       (void**)&pICatRegister);

        if (SUCCEEDED(hr2))
        {
            ASSERT(pICatRegister);

            CATID acatid[1];
            acatid[0] = CATID_BrowsableShellExt;

            pICatRegister->RegisterClassImplCategories(CLSID_CDFVIEW, 1,
                                                       acatid);
            pICatRegister->Release();
        }
    }
       
    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllUnregisterServer ***
//
//     Self unregister the cdf viewer.    
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI DllUnregisterServer(
    void
)
{
    //
    // Note: The catagories registration in DllRegisterServer simply places a
    // value in CLSID\CLSID_CDFVIEW.  This unreg removes the whole key (see
    // "selfreg.inx").  So no special handeling of the removal of the catagory
    // is required.
    //

    return RegisterServerHelper("Unreg");
}

EXTERN_C
STDAPI DllInstall(BOOL fInstall, LPCWSTR pszCmdLine)
{
    if (fInstall)
    {
        //
        // IE5 no longer creates special channel folders.  Channels go into the
        // favorites folder.
        //

        /*
        Channel_CreateChannelFolder(DOC_CHANNEL);
        Channel_CreateChannelFolder(DOC_SOFTWAREUPDATE);
        */
    }
    else
    {
        //
        // REVIEW delete channels folder on uninstall?
        //
        ;
    }
    return S_OK;
}

//
// Internal Functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** RegisterServerHelper ***
//
//
// Description:
//     Helper function used to register and unregister the cdf viewer.
//
// Parameters:
//     [IN] szCmd - String passed to advpack.RegInstall.  Values use: "Reg" or
//                  "Unreg".
//
// Return:
//     SELFREG_E_CLASS if advpack.dll isn't accessed.  Otherwise the reuslt
//     from advpack.RegInstall is returned.
//
// Comments:
//     This helper is called by DllRegisterServer and DllUnregisterServer.
//
//     This function uses an exported function from advpack.dll to update the
//     registry.  Advpack uses the REGINST resource item to populate the
//     registry.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
RegisterServerHelper(
    LPSTR szCmd
)
{
    ASSERT(szCmd);

    HRESULT hr = SELFREG_E_CLASS;

    HINSTANCE hinstLib = LoadLibrary(TEXT("advpack.dll"));

    if (hinstLib)
    {
        REGINSTALL RegInstall = (REGINSTALL)GetProcAddress(hinstLib, 
                                                           achREGINSTALL);

        if (RegInstall)
            hr = RegInstall(g_hinst, szCmd, NULL);
        else
            TraceMsg(TF_ERROR, "DLLREG RegisterServerHelper() GetProcAddress Failed");

        FreeLibrary(hinstLib);
    }
    else
        TraceMsg(TF_ERROR, "DLLREG RegisterServerHelper() Failed to load Advpack.dll");

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllAddref ***
//
//    Increment the Dll ref counts.
//
////////////////////////////////////////////////////////////////////////////////
void
DllAddRef(
    void
)
{
    ASSERT (g_cDllRef < (ULONG)-1);

    InterlockedIncrement((PLONG)&g_cDllRef);

    TraceMsg(TF_OBJECTS, "%d Dll ref count", g_cDllRef);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** DllRelease ***
//
//    Decrements the Dll ref counts.
//
////////////////////////////////////////////////////////////////////////////////
void
DllRelease(
    void
)
{
    ASSERT(g_cDllRef != 0);

    InterlockedDecrement((PLONG)&g_cDllRef);

    TraceMsg(TF_OBJECTS, "%d Dll ref count", g_cDllRef);

    return;
}
