#include <windows.h>
#include <psapi.h>
#include <epsapi.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ddk/ntddk.h>
#include <napi/teb.h>
#include <ntos/heap.h>
#include <ntdll/ldr.h>

#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))
