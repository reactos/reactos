#include "precomp.h"
#include <shlwapi.h> // for DllInstall prototype
#include "advpub.h"

#define GUID_STR_LEN    40

// global variables
HINSTANCE hInstance;

STDAPI DllUnregisterServer(void)
{
    // This DLL does not need to unregister anything now.
    // So, we simply return success here.
    return NOERROR;
}

STDAPI DllRegisterServer(void)
{

    // This DLL does not need to register anything now.
    // So, we simply return success here.
    return NOERROR;
}

BOOL DllInitialize(IN PVOID hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hmod;
        DisableThreadLibraryCalls(hInstance);
#ifdef DEBUG
        CcshellGetDebugFlags();
#endif
    }
#ifdef DEBUG
    else if (ulReason == DLL_PROCESS_DETACH) {
        DeskCheckForLeaks();
    }
#endif        

    return TRUE;
}

BOOL APIENTRY DllMain(IN HANDLE hDll, IN DWORD dwReason, IN LPVOID lpReserved)
{
    ASSERT(0);
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    // No Com object is registered by this DLL. So, let's fail!
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}
