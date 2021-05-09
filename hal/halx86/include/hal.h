/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * FILE:            hal/halx86/include/hal.h
 * PURPOSE:         HAL Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _HAL_PCH_
#define _HAL_PCH_

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* WDK HAL Compilation hack */
#include <excpt.h>
#include <ntdef.h>
#ifndef _MINIHAL_
#undef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)
#else
#undef NTSYSAPI
#define NTSYSAPI
#endif

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <arc/arc.h>

#include <ndk/asm.h>
#include <ndk/halfuncs.h>
#include <ndk/inbvfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/rtlfuncs.h>

/* For MSVC, this is required before using DATA_SEG (used in pcidata) */
#ifdef _MSC_VER
# pragma section("INIT", read,execute,discard)
#endif

/* Internal shared PCI and ACPI header */
#include <drivers/pci/pci.h>
#include <drivers/acpi/acpi.h>

/* Internal kernel headers */
#define KeGetCurrentThread _KeGetCurrentThread
#ifdef _M_AMD64
#include <internal/amd64/ke.h>
#include <internal/amd64/mm.h>
#include "internal/amd64/intrin_i.h"
#else
#include <internal/i386/ke.h>
#include <internal/i386/mm.h>
#include "internal/i386/intrin_i.h"
#endif

#define TAG_HAL    ' laH'
#define TAG_BUS_HANDLER 'BusH'

/* Internal HAL Headers */
#include "bus.h"
#include "halirq.h"
#include "haldma.h"
#if defined(SARCH_PC98)
#include <drivers/pc98/cpu.h>
#include <drivers/pc98/pic.h>
#include <drivers/pc98/pit.h>
#include <drivers/pc98/rtc.h>
#include <drivers/pc98/sysport.h>
#include <drivers/pc98/video.h>
#else
#include "halhw.h"
#endif
#include "halp.h"
#include "mps.h"
#include "halacpi.h"

#endif /* _HAL_PCH_ */
