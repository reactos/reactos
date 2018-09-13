//--------------------------------------------------------------------------------
//
//      File:   EFFECTS.CPP
//
//      Implements the interfaces to the DLL.
//
//--------------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//
// Count number of objects and number of locks.
//
ULONG       g_cObj = 0;
ULONG       g_cLock = 0;
HINSTANCE   g_hInst = NULL;
BOOL        g_RunningOnNT = FALSE;

// OLE-Registry magic number
static const GUID CLSID_PlusPackCplExt = { 0x41e300e0, 0x78b6, 0x11ce,
                                           { 0x84, 0x9b, 0x44, 0x45,
                                             0x53, 0x54, 0x00, 0x00
                                           }
                                         };


//---------------------------------------------------------------------------
// DllMain()
//---------------------------------------------------------------------------
int APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    if( dwReason == DLL_PROCESS_ATTACH )        // Initializing
    {
        OSVERSIONINFO os;

        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ))
        {
            if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
            {
                g_RunningOnNT = TRUE;
            }
        }

        g_hInst = hInstance;



        DisableThreadLibraryCalls(hInstance);
    }

    return 1;
}
//---------------------------------------------------------------------------
//      DllGetClassObject()
//
//      If someone calls with our CLSID, create an IClassFactory and pass it to
//      them, so they can create and use one of our CPropSheetExt objects.
//
//---------------------------------------------------------------------------
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppvOut )
{
    *ppvOut = NULL; // Assume Failure
    if( IsEqualCLSID( rclsid, CLSID_PlusPackCplExt ) )
    {
        //
        //Check that we can provide the interface
        //
        if( IsEqualIID( riid, IID_IUnknown) ||
            IsEqualIID( riid, IID_IClassFactory )
           )
        {
            //Return our IClassFactory for CPropSheetExt objects
            *ppvOut = (LPVOID* )new CClassFactory();
            if( NULL != *ppvOut )
            {
                //AddRef the object through any interface we return
                ((CClassFactory*)*ppvOut)->AddRef();
                return NOERROR;
            }
            return E_OUTOFMEMORY;
        }
        return E_NOINTERFACE;
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}

//---------------------------------------------------------------------------
//      DllCanUnloadNow()
//
//      If we are not locked, and no objects are active, then we can exit.
//
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
    SCODE   sc;

    //
    //Our answer is whether there are any object or locks
    //
    sc = (0L == g_cObj && 0 == g_cLock) ? S_OK : S_FALSE;

    return ResultFromScode(sc);
}
