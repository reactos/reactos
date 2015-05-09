/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            memory.h
 * PURPOSE:         Memory Management
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

/* DEFINITIONS ****************************************************************/

#define TOTAL_PAGES (MAX_ADDRESS / PAGE_SIZE)

typedef VOID
(WINAPI *PMEMORY_READ_HANDLER)
(
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef BOOLEAN
(WINAPI *PMEMORY_WRITE_HANDLER)
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
MemRead
(
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID
MemWrite
(
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

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

#endif // _MEMORY_H_

/* EOF */
