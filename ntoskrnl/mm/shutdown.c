/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/shutdown.c
 * PURPOSE:         Memory Manager Shutdown
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* PRIVATE FUNCTIONS *********************************************************/

VOID
MiShutdownSystem(VOID)
{
    ULONG i;
    PFN_NUMBER Page;
    BOOLEAN Dirty;

    /* Loop through all the paging files */
    for (i = 0; i < MmNumberOfPagingFiles; i++)
    {
        /* Free page file name */
        ASSERT(MmPagingFile[i]->PageFileName.Buffer != NULL);
        ExFreePoolWithTag(MmPagingFile[i]->PageFileName.Buffer, TAG_MM);
        MmPagingFile[i]->PageFileName.Buffer = NULL;

        /* And close them */
        ZwClose(MmPagingFile[i]->FileHandle);
    }

    /* Loop through all the pages owned by the legacy Mm and page them out, if needed. */
    /* We do it as long as there are dirty pages, since flushing can cause the FS to dirtify new ones. */
    do
    {
        Dirty = FALSE;

        Page = MmGetLRUFirstUserPage();
        while (Page)
        {
            LARGE_INTEGER SegmentOffset;
            PMM_SECTION_SEGMENT Segment = MmGetSectionAssociation(Page, &SegmentOffset);

            if (Segment)
            {
                if ((*Segment->Flags) & MM_DATAFILE_SEGMENT)
                {
                    MmLockSectionSegment(Segment);

                    ULONG_PTR Entry = MmGetPageEntrySectionSegment(Segment, &SegmentOffset);

                    if (!IS_SWAP_FROM_SSE(Entry) && IS_DIRTY_SSE(Entry))
                    {
                        Dirty = TRUE;
                        MmCheckDirtySegment(Segment, &SegmentOffset, FALSE, TRUE);
                    }

                    MmUnlockSectionSegment(Segment);
                }

                MmDereferenceSegment(Segment);
            }

            Page = MmGetLRUNextUserPage(Page, FALSE);
        }
    } while (Dirty);
}

VOID
MmShutdownSystem(IN ULONG Phase)
{
    if (Phase == 0)
    {
        MiShutdownSystem();
    }
    else if (Phase == 1)
    {
        ULONG i;

        /* Loop through all the paging files */
        for (i = 0; i < MmNumberOfPagingFiles; i++)
        {
            /* And dereference them */
            ObDereferenceObject(MmPagingFile[i]->FileObject);
        }
    }
    else
    {
        ASSERT(Phase == 2);

        UNIMPLEMENTED;
    }
}
