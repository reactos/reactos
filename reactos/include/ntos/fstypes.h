#ifndef __INCLUDE_NTOS_FSTYPES_H
#define __INCLUDE_NTOS_FSTYPES_H

#define FSRTL_TAG 	TAG('F','S','r','t')


typedef ULONG LBN;
typedef LBN *PLBN;

typedef ULONG VBN;
typedef VBN *PVBN;

#ifndef __USE_W32API
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
#endif

typedef struct _MAILSLOT_CREATE_PARAMETERS 
{
    ULONG           MailslotQuota;
    ULONG           MaximumMessageSize;
    LARGE_INTEGER   ReadTimeout;
    BOOLEAN         TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS 
{
    ULONG           NamedPipeType;
    ULONG           ReadMode;
    ULONG           CompletionMode;
    ULONG           MaximumInstances;
    ULONG           InboundQuota;
    ULONG           OutboundQuota;
    LARGE_INTEGER   DefaultTimeout;
    BOOLEAN         TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

#endif  /* __INCLUDE_DDK_FSTYPES_H */
