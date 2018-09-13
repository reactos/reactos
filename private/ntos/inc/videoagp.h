/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    videoagp.h

Abstract:
    Video miniport AGP support.

Notes:

Revision History:

--*/

#ifndef __VIDEOAGP_H__
#define __VIDEOAGP_H__

typedef
PHYSICAL_ADDRESS
(*PAGP_RESERVE_PHYSICAL)(
    IN PVOID HwDeviceExtension,
    IN ULONG Pages,
    IN ULONG Caching,
    OUT PVOID *PhysicalReserveContext
    );

typedef
VOID
(*PAGP_RELEASE_PHYSICAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID PhysicalReserveContext
    );

typedef
BOOLEAN
(*PAGP_COMMIT_PHYSICAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID PhysicalReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

typedef
VOID
(*PAGP_FREE_PHYSICAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID PhysicalReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

typedef
PVOID
(*PAGP_RESERVE_VIRTUAL)(
    IN PVOID HwDeviceExtension,
    IN HANDLE ProcessHandle,
    IN PVOID PhysicalReserveContext,
    OUT PVOID *VirtualReserveContext
    );

typedef
VOID
(*PAGP_RELEASE_VIRTUAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID VirtualReserveContext
    );

typedef
PVOID
(*PAGP_COMMIT_VIRTUAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

typedef
VOID
(*PAGP_FREE_VIRTUAL)(
    IN PVOID HwDeviceExtension,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

typedef struct _VIDEO_PORT_AGP_SERVICES
{
    PAGP_RESERVE_PHYSICAL AgpReservePhysical;
    PAGP_RELEASE_PHYSICAL AgpReleasePhysical;
    PAGP_COMMIT_PHYSICAL  AgpCommitPhysical;
    PAGP_FREE_PHYSICAL    AgpFreePhysical;

    PAGP_RESERVE_VIRTUAL  AgpReserveVirtual;
    PAGP_RELEASE_VIRTUAL  AgpReleaseVirtual;
    PAGP_COMMIT_VIRTUAL   AgpCommitVirtual;
    PAGP_FREE_VIRTUAL     AgpFreeVirtual;
    ULONGLONG AllocationLimit;

} VIDEO_PORT_AGP_SERVICES, *PVIDEO_PORT_AGP_SERVICES;

BOOLEAN
VideoPortGetAgpServices(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_PORT_AGP_SERVICES AgpServices
    );

#endif // ifndef __VIDEOAGP_H__
