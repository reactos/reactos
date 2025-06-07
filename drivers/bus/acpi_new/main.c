#include "precomp.h"
//#define NDEBUG
#include <debug.h>

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    ACPIInitUACPI();
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
