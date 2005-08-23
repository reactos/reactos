/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndissys.h
 * PURPOSE:     NDIS library definitions
 * NOTES:       Spin lock acquire order:
 *                - Miniport list lock
 *                - Adapter list lock
 */
#ifndef __NDISSYS_H
#define __NDISSYS_H

typedef unsigned long NDIS_STATS;
#include <ndis.h>
#include <xfilter.h>
#include <afilter.h>

#if _MSC_VER
/* FIXME: These were removed and are no longer used! */
#define NdisWorkItemHalt NdisMaxWorkItems
#define NdisWorkItemSendLoopback (NdisMaxWorkItems + 1)
#else /* _MSC_VER */
/* FIXME: We miss the ATM headers. */
typedef struct _ATM_ADDRESS *PATM_ADDRESS;
#endif /* _MSC_VER */

/* FIXME: This should go away once NDK will be compatible with MS DDK headers. */
#if _MSC_VER
NTSTATUS NTAPI ZwDuplicateObject(IN HANDLE, IN HANDLE, IN HANDLE, OUT PHANDLE, IN ACCESS_MASK, IN ULONG, IN ULONG);
#else
#include <ndk/ntndk.h>
#endif

#define NDIS_MINIPORT_WORK_QUEUE_SIZE 10

struct _ADAPTER_BINDING;

typedef struct _INTERNAL_NDIS_MINIPORT_WORK_ITEM {
    SINGLE_LIST_ENTRY Link;
    struct _ADAPTER_BINDING *AdapterBinding;
    NDIS_MINIPORT_WORK_ITEM RealWorkItem;
} INTERNAL_NDIS_MINIPORT_WORK_ITEM, *PINTERNAL_NDIS_MINIPORT_WORK_ITEM;

typedef struct _NDISI_PACKET_POOL {
  NDIS_SPIN_LOCK  SpinLock;
  struct _NDIS_PACKET *FreeList;
  UINT  PacketLength;
  UCHAR  Buffer[1];
} NDISI_PACKET_POOL, * PNDISI_PACKET_POOL;

#include "miniport.h"
#include "protocol.h"

#include <debug.h>

/* Exported functions */
#ifndef EXPORT
#define EXPORT NTAPI
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define NDIS_TAG  0x4e4d4953

#ifdef DBG

#define DEBUG_REFCHECK(Object) {            \
    if ((Object)->RefCount <= 0) {          \
        NDIS_DbgPrint(MIN_TRACE, ("Object at (0x%X) has invalid reference count (%d).\n", \
            (Object), (Object)->RefCount)); \
        }                                   \
}

#else

#define DEBUG_REFCHECK(Object)

#endif


/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)                         \
{                                                       \
    DEBUG_REFCHECK(Object);                             \
    NDIS_DbgPrint(DEBUG_REFCOUNT, ("Referencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));                 \
                                                        \
    InterlockedIncrement((PLONG)&((Object)->RefCount)); \
}

/*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)                                \
{                                                                \
    DEBUG_REFCHECK(Object);                                      \
    NDIS_DbgPrint(DEBUG_REFCOUNT, ("Dereferencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));                          \
                                                                 \
    if (InterlockedDecrement((PLONG)&((Object)->RefCount)) == 0) \
        PoolFreeBuffer(Object);                                  \
}


#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#endif /* __NDISSYS_H */

/* EOF */
