#define WIN32_NO_STATUS
#include <windows.h>

#define DDKAPI __stdcall
#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
typedef VOID
(DDKAPI *PINTERFACE_REFERENCE)(
  PVOID  Context);
typedef VOID
(DDKAPI *PINTERFACE_DEREFERENCE)(
  PVOID  Context);
#include <ntndk.h>
#include <hidusage.h>
#include <hidclass.h>
#include <hidpi.h>

extern HINSTANCE hDllInstance;
extern const GUID HidClassGuid;

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED \
  DbgPrint("HID:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)
#endif

/* EOF */
