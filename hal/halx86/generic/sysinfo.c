/*
 * PROJECT:         ReactOS HA:
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/sysinfo.c
 * PURPOSE:         HAL Information Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#ifdef __GNUC__
static PUCHAR realKdComPortInUse = 0;
PUCHAR *_KdComPortInUse = &realKdComPortInUse;
#else
PUCHAR _KdComPortInUse = 0;
#endif

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
HaliQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           IN OUT PVOID Buffer,
                           OUT PULONG ReturnedLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HaliSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN OUT PVOID Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
