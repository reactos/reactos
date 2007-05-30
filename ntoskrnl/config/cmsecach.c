/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmsecach.c
 * PURPOSE:         Configuration Manager - Security Cache
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpInitSecurityCache(IN PCMHIVE Hive)
{
    ULONG i;

    /* Reset data */
    Hive->SecurityCount = 0;
    Hive->SecurityCacheSize = 0;
    Hive->SecurityHitHint = -1;
    Hive->SecurityCache = NULL;

    /* Loop every security hash */
    for (i = 0; i < 64; i++)
    {
        /* Initialize it */
        InitializeListHead(&Hive->SecurityHash[i]);
    }
}
