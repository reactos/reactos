#ifndef __INCLUDE_NTOS_FSTYPES_H
#define __INCLUDE_NTOS_FSTYPES_H

#define FSRTL_TAG 	TAG('F','S','r','t')


typedef ULONG LBN;
typedef LBN *PLBN;

typedef ULONG VBN;
typedef VBN *PVBN;


typedef struct _LARGE_MCB
{
  PFAST_MUTEX FastMutex;
  ULONG MaximumPairCount;
  ULONG PairCount;
  POOL_TYPE PoolType;
  PVOID Mapping;
} LARGE_MCB, *PLARGE_MCB;

typedef struct _MCB {
    LARGE_MCB LargeMcb;
} MCB, *PMCB;

#endif  /* __INCLUDE_DDK_FSTYPES_H */
