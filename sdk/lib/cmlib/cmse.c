/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmse.c
 * PURPOSE:         Configuration Manager Library - Security Subsystem Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES ******************************************************************/

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpRemoveSecurityCellList(IN PHHIVE Hive,
                          IN HCELL_INDEX SecurityCell)
{
    PCM_KEY_SECURITY SecurityData, FlinkCell, BlinkCell;

    PAGED_CODE();

    // ASSERT( (((PCMHIVE)Hive)->HiveSecurityLockOwner == KeGetCurrentThread()) || (CmpTestRegistryLockExclusive() == TRUE) );

    SecurityData = (PCM_KEY_SECURITY)HvGetCell(Hive, SecurityCell);
    if (!SecurityData) return;

    FlinkCell = (PCM_KEY_SECURITY)HvGetCell(Hive, SecurityData->Flink);
    if (!FlinkCell)
    {
        HvReleaseCell(Hive, SecurityCell);
        return;
    }

    BlinkCell = (PCM_KEY_SECURITY)HvGetCell(Hive, SecurityData->Blink);
    if (!BlinkCell)
    {
        HvReleaseCell(Hive, SecurityData->Flink);
        HvReleaseCell(Hive, SecurityCell);
        return;
    }

    /* Sanity checks */
    ASSERT(FlinkCell->Blink == SecurityCell);
    ASSERT(BlinkCell->Flink == SecurityCell);

    /* Unlink the security block and free it */
    FlinkCell->Blink = SecurityData->Blink;
    BlinkCell->Flink = SecurityData->Flink;
#ifdef USE_CM_CACHE
    CmpRemoveFromSecurityCache(Hive, SecurityCell);
#endif

    /* Release the cells */
    HvReleaseCell(Hive, SecurityData->Blink);
    HvReleaseCell(Hive, SecurityData->Flink);
    HvReleaseCell(Hive, SecurityCell);
}

VOID
NTAPI
CmpFreeSecurityDescriptor(IN PHHIVE Hive,
                          IN HCELL_INDEX Cell)
{
    PCM_KEY_NODE CellData;
    PCM_KEY_SECURITY SecurityData;

    PAGED_CODE();

    // ASSERT( (((PCMHIVE)Hive)->HiveSecurityLockOwner == KeGetCurrentThread()) || (CmpTestRegistryLockExclusive() == TRUE) );

    CellData = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!CellData) return;

    ASSERT(CellData->Signature == CM_KEY_NODE_SIGNATURE);

    // FIXME: ReactOS-specific: check whether this key has a security block.
    // On Windows there is no such check, all keys seem to have a valid
    // security block.
    // If we remove this check on ReactOS (and continue running) then we get
    // a BSOD at the end...
    if (CellData->Security == HCELL_NIL)
    {
        DPRINT("Cell 0x%08x (data 0x%p) has no security block!\n", Cell, CellData);
        HvReleaseCell(Hive, Cell);
        return;
    }

    SecurityData = (PCM_KEY_SECURITY)HvGetCell(Hive, CellData->Security);
    if (!SecurityData)
    {
        HvReleaseCell(Hive, Cell);
        return;
    }

    ASSERT(SecurityData->Signature == CM_KEY_SECURITY_SIGNATURE);

    if (SecurityData->ReferenceCount > 1)
    {
        SecurityData->ReferenceCount--;
    }
    else // if (SecurityData->ReferenceCount <= 1)
    {
        CmpRemoveSecurityCellList(Hive, CellData->Security);
        HvFreeCell(Hive, CellData->Security);
    }

    CellData->Security = HCELL_NIL;
    HvReleaseCell(Hive, CellData->Security);
    HvReleaseCell(Hive, Cell);
}
