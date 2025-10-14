#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ntverp.h>

extern "C" {
#include <ntddk.h>
#include <ntstrsafe.h>
}

#include <wdf.h>

#define  FX_DYNAMICS_GENERATE_TABLE   1

//-----------------------------------------    ------------------------------------

extern "C" {

typedef VOID (NTAPI *WDFFUNC) (VOID);
#define  KMDF_DEFAULT_NAME   "Wdf01000"

void
__cxa_pure_virtual()
{
	__debugbreak();
}

}  // extern "C"
