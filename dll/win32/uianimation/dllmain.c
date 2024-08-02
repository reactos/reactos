
#include "wine/config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HINSTANCE instance;

BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinstance;
        DisableThreadLibraryCalls(hinstance);
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(instance);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(instance);
}