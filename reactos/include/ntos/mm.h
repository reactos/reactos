#ifndef NTOS_MM_H
#define NTOS_MM_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

typedef struct _MADDRESS_SPACE
{
  LIST_ENTRY MAreaListHead;
  FAST_MUTEX Lock;
  ULONG LowestAddress;
  struct _EPROCESS* Process;
  PUSHORT PageTableRefCountTable;
  ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;

#endif /* NTOS_MM_H */
