
/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/ntifs.h>
#include <ddk/tvout.h>
#include <ndk/ntndk.h>

#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#define INTERNAL_CALL NTAPI

#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>

#include <reactos/win32k/ntgdityp.h>
#include <reactos/win32k/ntgdihdl.h>
#include <reactos/win32k/win32kdc.h>
#include <reactos/drivers/directx/dxg.h>
#include <reactos/drivers/directx/dxeng.h>


/* exported functions */
NTSTATUS DriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS GsDriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS DxDdCleanupDxGraphics();


/* Driver list export functions */

/* Internel functions */
BOOL DdHmgDestroy();

