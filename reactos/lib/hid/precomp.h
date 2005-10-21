#include <ddk/ntddk.h>
#include <windows.h>

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <ddk/hidusage.h>
#include <ddk/hidclass.h>
#include <ddk/hidpi.h>

extern HINSTANCE hDllInstance;
extern const GUID HidClassGuid;

#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED \
  DbgPrint("HID:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)
#endif

/* EOF */
