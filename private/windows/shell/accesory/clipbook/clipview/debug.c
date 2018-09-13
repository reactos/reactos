//
// Purpose: This module contains functions that are being debugged or
//   are of interest during debugging. Generally, a function will be
//   placed in this file temporarily during debugging to make it easier
//   to find when running NTSD, 'cause the linker doesn't seem to include
//   symbols for functions that are only called from within their
//   own .c file.
//

// #include <nt.h>
// #include <ntrtl.h>
// #include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <sedapi.h>
#include <commctrl.h>
#include <nddeapi.h>
#include <nddesec.h>

#include "clipbook.h"
#include "clipdsp.h"
#include "common.h"
#include "helpids.h"
#include "dialogs.h"
#include "debug.h"







// #define NOOLEITEMSPERMIT if netdde does not have
// support for allowing certain default OLE items in item-level
// conversations

#define NOOLEITEMSPERMIT

#ifdef NOOLEITEMSPERMIT
#define NOLEITEMS    5
static TCHAR *OleShareItems[NOLEITEMS] =
   {
   TEXT("StdDocumentName"),
   TEXT("EditEnvItems"),
   TEXT("StdHostNames"),
   TEXT("StdTargetDevice"),
   TEXT("StdDocDimensions")
   };
#endif // NOOLEITEMSPERMIT







// Typedef for dynamically loading the Edit Owner dialog.
typedef DWORD (WINAPI *LPFNOWNER)(
      HWND,
      HANDLE,
      LPWSTR,
      LPWSTR,
      LPWSTR,
      UINT,
      PSED_FUNC_APPLY_SEC_CALLBACK,
      ULONG,
      PSECURITY_DESCRIPTOR,
      BOOLEAN,
      BOOLEAN,
      LPDWORD,
      PSED_HELP_INFO,
      DWORD);







static  TCHAR   atchStatusBar[128];
