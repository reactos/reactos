#include <ddk/winddi.h>

// HANDLE __cdecl LdrLoadModule (LPWSTR);

HANDLE
STDCALL
EngLoadImage (LPWSTR DriverName)
{
   return (HANDLE) LdrLoadModule(DriverName);
}
