/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmquota.c
 * PURPOSE:         Configuration Manager - Wrappers for Hive Operations
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

BOOLEAN CmpGlobalQuota;
BOOLEAN CmpGlobalQuotaAllowed;
 
/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpSetGlobalQuotaAllowed(VOID)
{
    /* Set flag ON if quota enabled */
    CmpGlobalQuotaAllowed = CmpGlobalQuota;
}
