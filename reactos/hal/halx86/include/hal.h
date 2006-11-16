/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * FILE:            hal/halx86/include/hal.h
 * PURPOSE:         HAL Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* WDK HAL Complation hack */
#ifdef _MSC_VER
#include <excpt.h>
#include <ntdef.h>
#undef _NTHAL_
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <iotypes.h>
#include <kefuncs.h>
#include <halfuncs.h>
#include <iofuncs.h>
#include <ldrtypes.h>
#include <obfuncs.h>

#define KPCR_BASE 0xFF000000 // HACK!

/* Internal HAL Headers */
#include "internal/pci.h"
#include "apic.h"
#include "bus.h"
#include "halirq.h"
#include "haldma.h"
#include "halp.h"
#include "mps.h"
#include "ioapic.h"

/* Helper Header */
#include <reactos/helper.h>

/* EOF */
