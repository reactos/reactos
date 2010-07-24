/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/largepag.c
 * PURPOSE:         ARM Memory Manager Large Page Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::LARGEPAGE"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

LIST_ENTRY MmProcessList;
PMMPTE MiLargePageHyperPte;
ULONG MiLargePageRangeIndex;
MI_LARGE_PAGE_RANGES MiLargePageRanges[64];
WCHAR MmLargePageDriverBuffer[512] = {0};
ULONG MmLargePageDriverBufferLength = -1;
LIST_ENTRY MiLargePageDriverList;
BOOLEAN MiLargePageAllDrivers;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
MiInitializeLargePageSupport(VOID)
{
#if _MI_PAGING_LEVELS > 2
#error "PAE/x64 Not Implemented"
#else
    /* Initialize the large-page hyperspace PTE used for initial mapping */
    MiLargePageHyperPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(MiLargePageHyperPte);
    MiLargePageHyperPte->u.Long = 0;

    /* Initialize the process tracking list, and insert the system process */
    InitializeListHead(&MmProcessList);
    InsertTailList(&MmProcessList, &PsGetCurrentProcess()->MmProcessLinks);
#endif    
}

VOID
NTAPI
MiSyncCachedRanges(VOID)
{
    ULONG i;

    /* Scan every range */
    for (i = 0; i < MiLargePageRangeIndex; i++)
    {
        DPRINT1("No support for large pages\n");
        while (TRUE);
    }
}

VOID
NTAPI
MiInitializeDriverLargePageList(VOID)
{
    PWCHAR p, pp;

    /* Initialize the list */
    InitializeListHead(&MiLargePageDriverList);

    /* Bail out if there's nothing */
    if (MmLargePageDriverBufferLength == 0xFFFFFFFF) return;

    /* Loop from start to finish */
    p = MmLargePageDriverBuffer;
    pp = MmLargePageDriverBuffer + (MmLargePageDriverBufferLength / sizeof(WCHAR));
    while (p < pp)
    {
        /* Skip whitespaces */
        if ((*p == L' ') || (*p == L'\n') || (*p == L'\r') || (*p == L'\t'))
        {
            /* Skip the character */
            p++;
            continue;
        }
        
        /* A star means everything */
        if (*p == L'*')
        {
            /* No need to keep going */
            MiLargePageAllDrivers = TRUE;
            break;
        }
        
        DPRINT1("Large page drivers not supported\n");
        ASSERT(FALSE);
    }
}

/* EOF */
