#include <ddk/winddi.h>

// HANDLE __cdecl LdrLoadModule (LPWSTR);

HANDLE
STDCALL
EngLoadImage (LPWSTR DriverName)
{
   return (HANDLE) LdrLoadModule(DriverName);
}

HANDLE
STDCALL
EngLoadModule(LPWSTR ModuleName)
{
   // FIXME: should load as readonly
   return (HANDLE) LdrLoadModule(ModuleName);
}
