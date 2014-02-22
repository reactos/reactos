#ifndef _HID_PCH_
#define _HID_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#include <ndk/umtypes.h>

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
typedef VOID
(WINAPI *PINTERFACE_REFERENCE)(
  PVOID  Context);
typedef VOID
(WINAPI *PINTERFACE_DEREFERENCE)(
  PVOID  Context);

#include <hidclass.h>

extern HINSTANCE hDllInstance;
extern const GUID HidClassGuid;

#endif /* _HID_PCH_ */
