/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/abios.c
 * PURPOSE:         Routines for ABIOS Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386FlatToGdtSelector(IN ULONG Base,
                        IN USHORT Length,
                        IN USHORT Selector)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386ReleaseGdtSelectors(OUT PULONG SelArray,
                          IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeI386AllocateGdtSelectors(OUT PULONG SelArray,
                           IN ULONG NumOfSelectors)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KeI386SetGdtSelector(IN ULONG Selector,
                     IN PKGDTENTRY GdtValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KeI386AbiosCall(IN USHORT CallId,
                IN PDRIVER_OBJECT DriverObject,
                IN PULONG RequestBlock,
                IN USHORT EntryPoint)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KeI386GetLid(IN USHORT DeviceId,
             IN USHORT RelativeLid,
             IN BOOLEAN SharedLid,
             IN PDRIVER_OBJECT DriverObject,
             OUT PUSHORT LogicalId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KeI386ReleaseLid(IN USHORT LogicalId,
                 IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

USHORT
NTAPI
KeI386Call16BitCStyleFunction(IN ULONG EntryOffset,
                              IN ULONG EntrySelector,
                              IN PULONG Parameters,
                              IN ULONG Size)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
KeI386Call16BitFunction(IN OUT PCONTEXT Context)
{
    UNIMPLEMENTED;
}

/* EOF */
