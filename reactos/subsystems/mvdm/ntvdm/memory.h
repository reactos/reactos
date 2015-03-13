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
(WINAPI *PVDD_MEMORY_HANDLER)
(
    DWORD FaultingAddress,
    BOOLEAN Writing
);

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

typedef struct _MEM_HOOK
{
    LIST_ENTRY Entry;
    HANDLE hVdd;
    ULONG Count;

    union
    {
        PVDD_MEMORY_HANDLER VddHandler;

        struct
        {
            PMEMORY_READ_HANDLER FastReadHandler;
            PMEMORY_WRITE_HANDLER FastWriteHandler;
        };
    };
} MEM_HOOK, *PMEM_HOOK;

/* FUNCTIONS ******************************************************************/

BOOLEAN MemInitialize(VOID);
VOID MemCleanup(VOID);
VOID MemExceptionHandler(DWORD Address, BOOLEAN Writing);

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

BOOL
WINAPI
VDDInstallMemoryHook
(
    HANDLE hVdd,
    PVOID pStart,
    DWORD dwCount,
    PVDD_MEMORY_HANDLER pHandler
);

BOOL
WINAPI
VDDDeInstallMemoryHook
(
    HANDLE hVdd,
    PVOID pStart,
    DWORD dwCount
);

#endif // _MEMORY_H_

/* EOF */
