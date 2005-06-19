/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * FILE:            hal/halx86/include/hal.h
 * PURPOSE:         HAL Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */
#include <ddk/ntddk.h>
#include <stdio.h>

/* FIXME: NDK Headers */
#include <roskrnl.h>

/* Internal Kernel Headers */
//#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/i386/ps.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

//Temporary hack below.
PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
					  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
			          IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
			          IN MEMORY_CACHING_TYPE CacheType OPTIONAL,
					  IN ULONG Alignment);
                      
/* FIXME: NDK */
VOID STDCALL KeEnterKernelDebugger (VOID);
VOID FASTCALL KiAcquireSpinLock(PKSPIN_LOCK SpinLock);
VOID FASTCALL KiReleaseSpinLock(PKSPIN_LOCK SpinLock);
VOID STDCALL KiDispatchInterrupt(VOID);
NTSTATUS
STDCALL
ObCreateObject (
    IN KPROCESSOR_MODE      ObjectAttributesAccessMode OPTIONAL,
    IN POBJECT_TYPE         ObjectType,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE      AccessMode,
    IN OUT PVOID            ParseContext OPTIONAL,
    IN ULONG                ObjectSize,
    IN ULONG                PagedPoolCharge OPTIONAL,
    IN ULONG                NonPagedPoolCharge OPTIONAL,
    OUT PVOID               *Object
);

/* Debug Header */
#include <debug.h>

/* Internal HAL Headers */
#include "apic.h"
#include "bus.h"
#include "halirq.h"
#include "halp.h"
#include "mps.h"
#include "ioapic.h"

/* Helper Macros FIXME: NDK */
#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)(l))
#endif

/* EOF */
