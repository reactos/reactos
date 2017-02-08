/*
 * DESCRIPTION: Entry point for TDI.SYS
 * (c) Captain Obvious
 */

#include "precomp.h"

extern LONG CteTimeIncrement;

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    /* Initialize the time increment for CTE timers */
    CteTimeIncrement = KeQueryTimeIncrement();

    return STATUS_SUCCESS;
}

/* EOF */
