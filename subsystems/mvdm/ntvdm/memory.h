/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/memory.h
 * PURPOSE:         Memory Management
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

/* DEFINITIONS ****************************************************************/

#define TOTAL_PAGES (MAX_ADDRESS / PAGE_SIZE)

typedef VOID
(FASTCALL *PMEMORY_READ_HANDLER)
(
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef BOOLEAN
(FASTCALL *PMEMORY_WRITE_HANDLER)
(
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

/* FUNCTIONS ******************************************************************/

BOOLEAN MemInitialize(VOID);
VOID MemCleanup(VOID);
VOID MemExceptionHandler(ULONG FaultAddress, BOOLEAN Writing);

VOID
FASTCALL
EmulatorReadMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID
FASTCALL
EmulatorWriteMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID
FASTCALL
EmulatorCopyMemory
(
    PFAST486_STATE State,
    ULONG DestAddress,
    ULONG SrcAddress,
    ULONG Size
);

VOID EmulatorSetA20(BOOLEAN Enabled);
BOOLEAN EmulatorGetA20(VOID);

BOOL
MemInstallFastMemoryHook
(
    PVOID Address,
    ULONG Size,
    PMEMORY_READ_HANDLER ReadHandler,
    PMEMORY_WRITE_HANDLER WriteHandler
);

BOOL
MemRemoveFastMemoryHook
(
    PVOID Address,
    ULONG Size
);

BOOLEAN
MemQueryMemoryZone
(
    ULONG StartAddress,
    PULONG Length,
    PBOOLEAN Hooked
);

#endif /* _MEMORY_H_ */
