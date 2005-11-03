


// TODO: HEader: To be pasted




// INCL_DOSMEMMGR || !INCL_NOCOMMON 
#define  INCL_DOSMEMMGR 
#undefine INCL_NOCOMMON

#include <os2.h>


// TODO: Expain that this is for memory alloc/free funcs of Os2 api



// TODO: Implement functions and give them bodies
ULONG DosAllocMem (PPVOID pBaseAddress, ULONG ulObjectSize,
    ULONG ulAllocationFlags);
ULONG DosAllocSharedMem (PPVOID pBaseAddress, PCSZ pszName,
    ULONG ulObjectSize, ULONG ulAllocationFlags);
ULONG DosFreeMem (PVOID pBaseAddress);
ULONG DosGetNamedSharedMem (PPVOID pBaseAddress, PCSZ pszSharedMemName,
    ULONG ulAttributeFlags);
ULONG DosGetSharedMem (CPVOID pBaseAddress, ULONG ulAttributeFlags);
ULONG DosGiveSharedMem (CPVOID pBaseAddress, PID idProcessId,
    ULONG ulAttributeFlags);
ULONG DosQueryMem (CPVOID pBaseAddress, PULONG pulRegionSize,
    PULONG pulAllocationFlags);
ULONG DosSetMem (CPVOID pBaseAddress, ULONG ulRegionSize,
    ULONG ulAttributeFlags);
ULONG DosSubAllocMem (PVOID pOffset, PPVOID pBlockOffset, ULONG ulSize);
ULONG DosSubFreeMem (PVOID pOffset, PVOID pBlockOffset, ULONG ulSize);
ULONG DosSubSetMem (PVOID pOffset, ULONG ulFlags, ULONG ulSize);
ULONG DosSubUnsetMem (PVOID pOffset);

// TODO: Add file to makefile