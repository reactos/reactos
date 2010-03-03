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

/* WDK HAL Compilation hack */
#include <excpt.h>
#include <ntdef.h>
#undef _NTHAL_
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#ifndef _MINIHAL_
#undef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)
#else
#undef _NTSYSTEM_
#endif

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <bugcodes.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <ntndk.h>

/* Internal kernel headers */
#include "internal/pci.h"
#define KeGetCurrentThread _KeGetCurrentThread
#include <internal/i386/ke.h>
#include <internal/i386/mm.h>
#ifdef _M_AMD64
#include "internal/amd64/intrin_i.h"
#else
#include "internal/i386/intrin_i.h"
#endif

/* Internal HAL Headers */
#include "apic.h"
#include "bus.h"
#include "halirq.h"
#include "haldma.h"
#include "halp.h"
#include "mps.h"
#include "ioapic.h"

/* EOF */
