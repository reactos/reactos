/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/miarm.h
 * PURPOSE:         ARM Memory Manager Header
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING ((255*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING         ((19*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST          ((32*1024*1024) >> PAGE_SHIFT)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE         (128 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE              (128 * 1024 * 1024)
#define MI_MAX_FREE_PAGE_LISTS                 4

typedef enum _MMSYSTEM_PTE_POOL_TYPE
{
    SystemPteSpace,
    NonPagedPoolExpansion,
    MaximumPtePoolTypes
} MMSYSTEM_PTE_POOL_TYPE;

extern MMPTE HyperTemplatePte;

extern ULONG MmSizeOfNonPagedPoolInBytes;
extern ULONG MmMaximumNonPagedPoolInBytes;
extern PVOID MmNonPagedPoolStart;
extern PVOID MmNonPagedPoolExpansionStart;
extern PMMPTE MmFirstReservedMappingPte, MmLastReservedMappingPte;

VOID
NTAPI
MiInitializeArmPool(
    VOID
);

VOID
NTAPI
MiInitializeSystemPtes(
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE PoolType
);

/* EOF */
