/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * FILE:            hal/halx86/include/hal.h
 * PURPOSE:         HAL Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* IFS/DDK/NDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ndk/ntndk.h>

/* Internal Kernel Headers */
//#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/i386/ps.h>

//Temporary hack below until ntoskrnl is on NDK
PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
					  IN PHYSICAL_ADDRESS LowestAcceptableAddress,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
			          IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
			          IN MEMORY_CACHING_TYPE CacheType OPTIONAL,
					  IN ULONG Alignment);
                      
/* Internal HAL Headers */
#include "apic.h"
#include "bus.h"
#include "halirq.h"
#include "halp.h"
#include "mps.h"
#include "ioapic.h"

/* Helper Header */
#include <reactos/helper.h>

/* EOF */
