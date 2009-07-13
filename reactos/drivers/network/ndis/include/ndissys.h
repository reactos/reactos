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

#include <ntifs.h>
#include <ndis.h>
#include <xfilter.h>
#include <afilter.h>
#include <atm.h>
#include <ndistapi.h>
#include <ndisguid.h>
#include <debug.h>

#include "miniport.h"
#include "protocol.h"
#include "buffer.h"

/* Exported functions */
#ifndef EXPORT
#define EXPORT NTAPI
#endif

/* the version of NDIS we claim to be */
#define NDIS_VERSION 0x00050000

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
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

#endif /* __NDISSYS_H */

/* EOF */
