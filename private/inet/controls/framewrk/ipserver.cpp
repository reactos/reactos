//=--------------------------------------------------------------------------=
// IPServer.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implements all exported DLL functions for the program, as well as a few
// others that will be used by same
//
#include "IPServer.H"
#include "LocalSrv.H"

#include "AutoObj.H"
#include "ClassF.H"
#include "CtrlObj.H"
#include "Globals.H"
#include "Unknown.H"
#include "Util.H"

//=--------------------------------------------------------------------------=
// Private module level data
//
// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// These are used for reflection in OLE Controls.  Not that big of a hit that
// we mind defining them for all servers, including automation or generic
// COM.
//
char g_szReflectClassName [] = "CtlFrameWork_ReflectWindow";
BYTE g_fRegisteredReflect = FALSE;


//=--------------------------------------------------------------------------=
// allow controls to register for DLL_THREAD_ATTACH and DLL_THREAD_DETACH 
//
THRDNFYPROC g_pfnThreadProc = NULL;

extern "C" void SetLibraryThreadProc(THRDNFYPROC pfnThreadNotify)
{
    if ((g_pfnThreadProc = pfnThreadNotify) == NULL)
        DisableThreadLibraryCalls((HMODULE)g_hInstance);
}


// ref count for LockServer
//
LONG  g_cLocks;


// private routines for this file.
//
int       IndexOfOleObject(REFCLSID);
HRESULT   RegisterAllObjects(void);
HRESULT   UnregisterAllObjects(void);

//=--------------------------------------------------------------------------=
// DllMain
//=--------------------------------------------------------------------------=
// yon standard LibMain.
//
// Parameters and Output:
//    - see SDK Docs on DllMain
//
// Notes:
//
BOOL WINAPI DllMain
(
    HANDLE hInstance,
    DWORD  dwReason,
    void  *pvReserved
)
{
    int i;

    switch (dwReason) {
      // set up some global variables, and get some OS/Version information
      // set up.
      //
      case DLL_PROCESS_ATTACH:
        {
        DWORD dwVer = GetVersion();
        DWORD dwWinVer;

        //  swap the two lowest bytes of dwVer so that the major and minor version
        //  numbers are in a usable order.
        //  for dwWinVer: high byte = major version, low byte = minor version
        //     OS               Sys_WinVersion  (as of 5/2/95)
        //     =-------------=  =-------------=
        //     Win95            0x035F   (3.95)
        //     WinNT ProgMan    0x0333   (3.51)
        //     WinNT Win95 UI   0x0400   (4.00)
        //
        dwWinVer = (UINT)(((dwVer & 0xFF) << 8) | ((dwVer >> 8) & 0xFF));
        g_fSysWinNT = FALSE;
        g_fSysWin95 = FALSE;
        g_fSysWin95Shell = FALSE;

        if (dwVer < 0x80000000) {
            g_fSysWinNT = TRUE;
            g_fSysWin95Shell = (dwWinVer >= 0x0334);
        } else  {
            g_fSysWin95 = TRUE;
            g_fSysWin95Shell = TRUE;
        }

        // initialize a critical seciton for our apartment threading support
        //
        InitializeCriticalSection(&g_CriticalSection);

        // create an initial heap for everybody to use.
        // currently, we're going to let the system make things thread-safe,
        // which will make them a little slower, but hopefully not enough
        // to notice
        //
        g_hHeap = GetProcessHeap();
        if (!g_hHeap) {
            FAIL("Couldn't get Process Heap.  Not good!");
            return FALSE;
        }

        g_hInstance = (HINSTANCE)hInstance;

        // give the user a chance to initialize whatever
        //
        InitializeLibrary();

        // if they didn't ask for thread notifications then optimize by turning
        // them off for our DLL.
        //
        if (!g_pfnThreadProc)
            DisableThreadLibraryCalls((HMODULE)hInstance);
        }
        break;

      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
        if (g_pfnThreadProc)
            g_pfnThreadProc(hInstance, dwReason, pvReserved);
        break;

      // do  a little cleaning up!
      //
      case DLL_PROCESS_DETACH:

        // clean up our critical seciton
        //
        DeleteCriticalSection(&g_CriticalSection);

        // unregister all the registered window classes.
        //
        i = 0;

        while (!ISEMPTYOBJECT(i)) {
            if (g_ObjectInfo[i].usType == OI_CONTROL) {
                if (CTLWNDCLASSREGISTERED(i))
                    UnregisterClass(WNDCLASSNAMEOFCONTROL(i), g_hInstance);
            }
            i++;
        }

        // clean up our parking window.
        //
        if (g_hwndParking) {
            DestroyWindow(g_hwndParking);
            UnregisterClass("CtlFrameWork_Parking", g_hInstance);
            --g_cLocks;
        }

        // clean up after reflection, if appropriate.
        //
        if (g_fRegisteredReflect)
            UnregisterClass(g_szReflectClassName, g_hInstance);

        // give the user a chance to do some cleaning up
        //
        UninitializeLibrary();
        break;
    }

    return TRUE;
}



//=--------------------------------------------------------------------------=
// DllRegisterServer
//=--------------------------------------------------------------------------=
// registers the Automation server
//
// Output:
//    HRESULT
//
// Notes:
//
STDAPI DllRegisterServer
(
    void
)
{
    HRESULT hr;

    hr = RegisterAllObjects();
    RETURN_ON_FAILURE(hr);

    // call user registration function.
    //
    return (RegisterData())? S_OK : E_FAIL;
}



//=--------------------------------------------------------------------------=
// DllUnregisterServer
//=--------------------------------------------------------------------------=
// unregister's the Automation server
//
// Output:
//    HRESULT
//
// Notes:
//
STDAPI DllUnregisterServer
(
    void
)
{
    HRESULT hr;

    hr = UnregisterAllObjects();
    RETURN_ON_FAILURE(hr);

    // call user unregistration function
    //
    return (UnregisterData()) ? S_OK : E_FAIL;
}


//=--------------------------------------------------------------------------=
// DllCanUnloadNow
//=--------------------------------------------------------------------------=
// we are being asked whether or not it's okay to unload the DLL.  just check
// the lock counts on remaining objects ...
//
// Output:
//    HRESULT        - S_OK, can unload now, S_FALSE, can't.
//
// Notes:
//
STDAPI DllCanUnloadNow
(
    void
)
{
    // if there are any objects lying around, then we can't unload.  The
    // controlling CUnknownObject class that people should be inheriting from
    // takes care of this
    //
    return (g_cLocks) ? S_FALSE : S_OK;
}


//=--------------------------------------------------------------------------=
// DllGetClassObject
//=--------------------------------------------------------------------------=
// creates a ClassFactory object, and returns it.
//
// Parameters:
//    REFCLSID        - CLSID for the class object
//    REFIID          - interface we want class object to be.
//    void **         - pointer to where we should ptr to new object.
//
// Output:
//    HRESULT         - S_OK, CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY,
//                      E_INVALIDARG, E_UNEXPECTED
//
// Notes:
//
STDAPI DllGetClassObject
(
    REFCLSID rclsid,
    REFIID   riid,
    void   **ppvObjOut
)
{
    HRESULT hr;
    void   *pv;
    int     iIndex;

    // arg checking
    //
    if (!ppvObjOut)
        return E_INVALIDARG;

    // first of all, make sure they're asking for something we work with.
    //
    iIndex = IndexOfOleObject(rclsid);
    if (iIndex == -1)
        return CLASS_E_CLASSNOTAVAILABLE;

    // create the blank object.
    //
    pv = (void *)new CClassFactory(iIndex);
    if (!pv)
        return E_OUTOFMEMORY;

    // QI for whatever the user has asked for.
    //
    hr = ((IUnknown *)pv)->QueryInterface(riid, ppvObjOut);
    ((IUnknown *)pv)->Release();

    return hr;
}
//=--------------------------------------------------------------------------=
// IndexOfOleObject
//=--------------------------------------------------------------------------=
// returns the index in our global table of objects of the given CLSID.  if
// it's not a supported object, then we return -1
//
// Parameters:
//    REFCLSID     - [in] duh.
//
// Output:
//    int          - >= 0 is index into global table, -1 means not supported
//
// Notes:
//
int IndexOfOleObject
(
    REFCLSID rclsid
)
{
    int x = 0;

    // an object is creatable if it's CLSID is in the table of all allowable object
    // types.
    //
    while (!ISEMPTYOBJECT(x)) {
        if (OBJECTISCREATABLE(x)) {
            if (rclsid == CLSIDOFOBJECT(x))
                return x;
        }
        x++;
    }

    return -1;
}

//=--------------------------------------------------------------------------=
// RegisterAllObjects
//=--------------------------------------------------------------------------=
// registers all the objects for the given automation server.
//
// Parameters:
//    none
//
// Output:
//    HERSULT        - S_OK, E_FAIL
//
// Notes:
//
HRESULT RegisterAllObjects
(
    void
)
{
    ITypeLib *pTypeLib;
    HRESULT hr;
    DWORD   dwPathLen;
    char    szTmp[MAX_PATH];
    int     x = 0;

    // loop through all of our creatable objects [those that have a clsid in
    // our global table] and register them.
    //
    while (!ISEMPTYOBJECT(x)) {
        if (!OBJECTISCREATABLE(x)) {
            x++;
            continue;
        }

        // depending on the object type, register different pieces of information
        //
        switch (g_ObjectInfo[x].usType) {

          // for both simple co-creatable objects and proeprty pages, do the same
          // thing
          //
          case OI_UNKNOWN:
          case OI_PROPERTYPAGE:
            RegisterUnknownObject(NAMEOFOBJECT(x), CLSIDOFOBJECT(x));
            break;

          case OI_AUTOMATION:
            RegisterAutomationObject(g_szLibName, NAMEOFOBJECT(x), VERSIONOFOBJECT(x), 
                                     *g_pLibid, CLSIDOFOBJECT(x));
            break;

          case OI_CONTROL:
            RegisterControlObject(g_szLibName, NAMEOFOBJECT(x), VERSIONOFOBJECT(x),
                                  *g_pLibid, CLSIDOFOBJECT(x), OLEMISCFLAGSOFCONTROL(x),
                                  BITMAPIDOFCONTROL(x));
            break;

        }
        x++;
    }

    // Load and register our type library.
    //
    if (g_fServerHasTypeLibrary) {
        dwPathLen = GetModuleFileName(g_hInstance, szTmp, MAX_PATH);
        MAKE_WIDEPTR_FROMANSI(pwsz, szTmp);
        hr = LoadTypeLib(pwsz, &pTypeLib);
        RETURN_ON_FAILURE(hr);
        hr = RegisterTypeLib(pTypeLib, pwsz, NULL);
        pTypeLib->Release();
        RETURN_ON_FAILURE(hr);
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// UnregisterAllObjects
//=--------------------------------------------------------------------------=
// un-registers all the objects for the given automation server.
//
// Parameters:
//    none
//
// Output:
//    HRESULT        - S_OK
//
// Notes:
//
HRESULT UnregisterAllObjects
(
    void
)
{
    int x = 0;

    // loop through all of our creatable objects [those that have a clsid in
    // our global table] and register them.
    //
    while (!ISEMPTYOBJECT(x)) {
        if (!OBJECTISCREATABLE(x)) {
            x++;
            continue;
        }

        switch (g_ObjectInfo[x].usType) {

          case OI_UNKNOWN:
          case OI_PROPERTYPAGE:
            UnregisterUnknownObject(CLSIDOFOBJECT(x));
            break;

          case OI_CONTROL:
            UnregisterControlObject(g_szLibName, NAMEOFOBJECT(x), VERSIONOFOBJECT(x), 
                                    CLSIDOFOBJECT(x));
    
          case OI_AUTOMATION:
            UnregisterAutomationObject(g_szLibName, NAMEOFOBJECT(x), VERSIONOFOBJECT(x), 
                                       CLSIDOFOBJECT(x));
            break;

        }
        x++;
    }

    // if we've got one, unregister our type library [this isn't an API function
    // -- we've implemented this ourselves]
    //
    if (g_pLibid)
        UnregisterTypeLibrary(*g_pLibid);

    return S_OK;
}
