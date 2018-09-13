/*++
   
    Contains DLLMain and standard OLE COM object creation stuff.

--*/


#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <olectl.h> // Dll[Un]RegisterServer

#include "classfac.h"
#include "psexsup.h"

//   GUID stuff

// this is only done once
// TODO, see if this is appropriate

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "guid.h"
#pragma data_seg()


HINSTANCE   g_hInst;
LONG        g_DllRefCount = 0;
BOOL        g_bShowIETB;
BOOL        g_bShowISTB;
int         g_nColumn1;
int         g_nColumn2;


extern "C" BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            g_hInst = hInstance;

            //
            // initialize Protected Storage Support routines
            //
            
            if(!InitializePStoreSupport())
                return FALSE;

		    //
			// init common controls
			//

		    INITCOMMONCONTROLSEX iccex;
		    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		    iccex.dwICC = ICC_LISTVIEW_CLASSES;
		    InitCommonControlsEx(&iccex);

            break;

        case DLL_PROCESS_DETACH:
            ShutdownPStoreSupport();
            break;
    }

    return TRUE;
}                                 


STDAPI
DllCanUnloadNow(
    void
    )
{
    return (g_DllRefCount ? S_FALSE : S_OK);
}


STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppReturn
    )
{

    //
    // if we don't support this classid, return the proper error code
    //

    if(!IsEqualCLSID(rclsid, CLSID_PStoreNameSpace))
       return CLASS_E_CLASSNOTAVAILABLE;

    //
    // create a CClassFactory object and check it for validity
    //

    CClassFactory *pClassFactory = new CClassFactory();
    if(NULL == pClassFactory)
       return E_OUTOFMEMORY;

    //
    // get the QueryInterface return for our return value
    //

    HRESULT hResult = pClassFactory->QueryInterface(riid, ppReturn);

    //
    // call Release to decrement the ref count - creating the object set it to
    // one and QueryInterface incremented it - since its being used externally
    // (not by us), we only want the ref count to be 1
    //

    pClassFactory->Release();

    return hResult;
}



//
// overload new and delete so we don't need to bring in full CRT
//


void * __cdecl operator new(unsigned int cb)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

void __cdecl operator delete(void * pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}
