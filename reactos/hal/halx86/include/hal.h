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

/* IFS/DDK/NDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ndk/ntndk.h>

/* Internal Kernel Headers */
#include <internal/mm.h>
#include <internal/ke.h>

#define KPCR_BASE 0xFF000000 // HACK!

/* Internal HAL Headers */
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
