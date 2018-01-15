#include "precomp.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>

class CShell32VistaModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CShell32VistaModule                         gModule;
CAtlWinModule                               gWinModule;

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        gModule.Init(ObjectMap, hInstance, NULL);
        DisableThreadLibraryCalls (hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return gModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return gModule.DllRegisterServer(FALSE);
}

STDAPI DllUnregisterServer()
{
    return gModule.DllUnregisterServer(FALSE);
}
