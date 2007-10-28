/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmapi.c
 * PURPOSE:         Internal routines that implement Nt* API functionality
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "cm.h"

/* GLOBALS *******************************************************************/

BOOLEAN CmpNoWrite;
BOOLEAN CmpForceForceFlush;

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpFlushEntireRegistry(IN BOOLEAN ForceFlush)
{
    BOOLEAN Flushed = TRUE;

    /* Make sure that the registry isn't read-only now */
    if (CmpNoWrite) return TRUE;

    /* Otherwise, acquire the hive list lock and disable force flush */
    CmpForceForceFlush = FALSE;
    ExAcquirePushLockShared(&CmpHiveListHeadLock);

    /* Check if the hive list isn't empty */
    if (!IsListEmpty(&CmpHiveListHead))
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }

    /* Release the lock and return the flush state */
    ExReleasePushLock(&CmpHiveListHeadLock);
    return Flushed;
}

VOID
NTAPI
CmpReportNotify(IN PCM_KEY_CONTROL_BLOCK Kcb,
                IN PHHIVE Hive,
                IN HCELL_INDEX Cell,
                IN ULONG Filter)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
}
