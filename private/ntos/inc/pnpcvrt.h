/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpcvrt.h

Abstract:

    This module contains the declarations for the internal APIs used to
    convert PNP resource descriptors to NT descriptors.

Author:

    Robert Nelson (robertn) 10/13/97


Revision History:


--*/

#ifndef _PNPCVRT_
#define _PNPCVRT_

VOID
PpBiosResourcesSetToDisabled (
    IN OUT PUCHAR BiosData,
    OUT    PULONG Length
    );

#define PPCONVERTFLAG_SET_RESTART_LCPRI               0x00000001
#define PPCONVERTFLAG_FORCE_FIXED_IO_16BIT_DECODE     0x00000002

NTSTATUS
PpBiosResourcesToNtResources (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PUCHAR *BiosData,
    IN ULONG ConvertFlags,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *ReturnedList,
    OUT PULONG ReturnedLength
    );

NTSTATUS
PpCmResourcesToBiosResources (
    IN PCM_RESOURCE_LIST CmResources,
    IN PUCHAR BiosRequirements,
    IN PUCHAR *BiosResources,
    IN PULONG Length
    );

#endif // _PNPCVRT_
