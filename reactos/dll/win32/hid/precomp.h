#define WIN32_NO_STATUS
#include <windows.h>
#include <debug.h>

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
typedef VOID
(WINAPI *PINTERFACE_REFERENCE)(
  PVOID  Context);
typedef VOID
(WINAPI *PINTERFACE_DEREFERENCE)(
  PVOID  Context);
#include <ntndk.h>
#include <hidusage.h>
#include <hidclass.h>
#include <hidpi.h>

extern HINSTANCE hDllInstance;
extern const GUID HidClassGuid;

/* EOF */
