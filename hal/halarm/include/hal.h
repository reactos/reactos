/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/include/hal.h
 * PURPOSE:         Hardware Abstraction Layer Header
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#define DbgPrint DbgPrintEarly
#include <stdio.h>

/* WDK HAL Compilation hack */
#include <excpt.h>
#include <ntdef.h>
#undef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)

#define CLOCK2_LEVEL CLOCK_LEVEL

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <ioaccess.h>
#include <bugcodes.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <iotypes.h>
#include <kefuncs.h>
#include <intrin.h>
#include <halfuncs.h>
#include <inbvfuncs.h>
#include <iofuncs.h>
#include <ldrtypes.h>
#include <obfuncs.h>

/* Internal shared PCI and ACPI header */
//#include <drivers/pci/pci.h>
//#include <drivers/acpi/acpi.h>

/* Internal HAL Headers */
#include "halp.h"
#include "gic.h"
#include "halacpi.h"

/* EOF */
