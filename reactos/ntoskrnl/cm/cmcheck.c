/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmcheck.c
 * PURPOSE:         Configuration Manager - Hive and Key Validation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

ULONG
NTAPI
CmCheckRegistry(IN PCMHIVE RegistryHive,
                IN ULONG Flags)
{
    /* FIXME: HACK! */
    return 0;
}
