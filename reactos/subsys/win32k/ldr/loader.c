#include <ddk/winddi.h>

HANDLE EngLoadImage(LPWSTR DriverName)
{
   return LdrLoadModule(DriverName);
}
