#pragma once

#include "hardware.h"

typedef struct _MEM_ENTRY
{
    UCHAR InUse:1;
    UCHAR Blocks:7;
} MEM_ENTRY, *PMEM_ENTRY;

typedef struct _MEM_HEADER
{
    UCHAR IsFull;
    MEM_ENTRY Entry[124];
    UCHAR Reserved[3];
} MEM_HEADER, *PMEM_HEADER;

VOID
DumpPages(VOID);

ULONG
AllocateMemory(PEHCI_HOST_CONTROLLER hcd, ULONG Size, ULONG *PhysicalAddress);

VOID
ReleaseMemory(PEHCI_HOST_CONTROLLER hcd, ULONG Address);
