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

#include <ntifs.h>
#include <ndis.h>
#include <xfilter.h>
#include <afilter.h>
#include <atm.h>

#if _MSC_VER
/* FIXME: These were removed and are no longer used! */
#define NdisWorkItemSendLoopback NdisWorkItemReserved
#else /* _MSC_VER */
/* FIXME: We miss the ATM headers. */
typedef struct _ATM_ADDRESS *PATM_ADDRESS;
#endif /* _MSC_VER */

struct _ADAPTER_BINDING;

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

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#endif /* __NDISSYS_H */

/* EOF */
