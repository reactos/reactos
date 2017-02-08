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

#include <ndis.h>

#include "debug.h"
#include "miniport.h"
#include "protocol.h"
#include "efilter.h"
#include "buffer.h"

/* Exported functions */
#ifndef EXPORT
#define EXPORT NTAPI
#endif

/* the version of NDIS we claim to be */
#define NDIS_VERSION 0x00050001

#define NDIS_TAG  0x4e4d4953

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#define ExInterlockedRemoveEntryList(_List,_Lock) \
 { KIRQL OldIrql; \
   KeAcquireSpinLock(_Lock, &OldIrql); \
   RemoveEntryList(_List); \
   KeReleaseSpinLock(_Lock, OldIrql); \
 }

/* missing protypes */
VOID
NTAPI
ExGetCurrentProcessorCounts(
   PULONG ThreadKernelTime,
   PULONG TotalCpuTime,
   PULONG ProcessorNumber);

VOID
NTAPI
ExGetCurrentProcessorCpuUsage(
    PULONG CpuUsage);

/* portability fixes */
#ifdef _M_AMD64
#define KfReleaseSpinLock KeReleaseSpinLock
#define KefAcquireSpinLockAtDpcLevel KeAcquireSpinLockAtDpcLevel
#define KefReleaseSpinLockFromDpcLevel KeReleaseSpinLockFromDpcLevel
#endif

#endif /* __NDISSYS_H */
