/*
 * Debug symbol provider for the shared virtio library.
 *
 * The static virtio library (sdk/lib/drivers/virtio) is linked into
 * viostor.sys and references the consumer-defined debug symbols declared
 * in kdebugprint.h. This file defines them for the miniport, mirroring
 * the provider pattern in netkvm's ParaNdis-Debug.c; output goes through
 * vDbgPrintEx so it is DbgPrint-compatible.
 */
#include <ntddk.h>
#include <stdarg.h>
#include <kdebugprint.h>

int virtioDebugLevel = 0;
int bDebugPrint = 0;

static void __cdecl VirtioDebugPrint(const char *Format, ...)
{
    va_list ArgList;

    va_start(ArgList, Format);
    vDbgPrintEx(DPFLTR_DEFAULT_ID, 9 | DPFLTR_MASK, Format, ArgList);
    va_end(ArgList);
}

tDebugPrintFunc VirtioDebugPrintProc = VirtioDebugPrint;
