/***************************************************************************
 *  dll.cpp
 *
 *  Standard DLL entry-point functions 
 *
 ***************************************************************************/

#include "priv.h"

#define DECLARE_DEBUG
#define SZ_MODULE       "SRVWIZ"
#define SZ_DEBUGSECTION "SrvWiz"
#define SZ_DEBUGINI     "SrvWiz"
#include "debug.h"

HINSTANCE g_hinst = NULL;
LONG    g_cRefDll = 0;      /* Global reference count */
CRITICAL_SECTION g_csDll;   /* The shared critical section */


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
    switch(dwReason) 
    {
        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&g_csDll);
            DisableThreadLibraryCalls(hinst);
            g_hinst = hinst;
            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&g_csDll);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
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

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefDll);
}


STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefDll);
}

/*****************************************************************************
 *
 *    DllGetClassObject
 *
 *    OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *    The artificial refcount inside DllGetClassObject helps to
 *    avoid the race condition described in DllCanUnloadNow.  It's
 *    not perfect, but it makes the race window much smaller.
 *
 *****************************************************************************/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    DllAddRef();
    
    if (IsEqualIID(rclsid, CLSID_SrvWiz))
    {
        hres = CSrvWizFactory_Create(rclsid, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hres = CLASS_E_CLASSNOTAVAILABLE;
    }

    DllRelease();
    return hres;
}

/*****************************************************************************
 *
 *    DllCanUnloadNow
 *
 *    OLE entry point.  Fail iff there are outstanding refs.
 *
 *    There is an unavoidable race condition between DllCanUnloadNow
 *    and the creation of a new IClassFactory:  Between the time we
 *    return from DllCanUnloadNow() and the caller inspects the value,
 *    another thread in the same process may decide to call
 *    DllGetClassObject, thus suddenly creating an object in this DLL
 *    when there previously was none.
 *
 *    It is the caller's responsibility to prepare for this possibility;
 *    there is nothing we can do about it.
 *
 *****************************************************************************/

STDMETHODIMP DllCanUnloadNow(void)
{
    HRESULT hres;

    ENTERCRITICAL;

    hres = g_cRefDll ? S_FALSE : S_OK;

    LEAVECRITICAL;

    return hres;
}
