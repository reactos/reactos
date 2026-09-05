/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Thermal zones manager infrastructure
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KSPIN_LOCK PopThermalZoneLock;
LIST_ENTRY PopThermalZones;
ULONG PopCoolingSystemMode = 0;

/* PRIVATE FUNCTIONS **********************************************************/

/* EOF */
