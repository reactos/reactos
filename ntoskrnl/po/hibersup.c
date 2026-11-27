/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power system hibernation infrastructure support
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Marks the specific area of RAM as the snapshot for hibernation.
 * The Power Manager will take that range into consideration when
 * the system undergoes hibernation as it'll clone the whole specific
 * area of RAM into the hibernation file.
 *
 * @param[in] HiberContext
 * A pointer to arbitrary data. This usually points to the hibernation
 * context of the system which keeps track of all hibernation shenanigans.
 *
 * @param[in] Flags
 * A flag bit passed by the caller of which influences how this function
 * behaves. The following flags are:
 *
 * PO_MEM_PRESERVE -- The following memory range needs to be preserved.
 *                    The Power Manager is held responsible to preserve
 *                    these pages for the whole life time.
 *
 * PO_MEM_CLONE -- Keep a copy of the memory pages specified by the start
 *                 of the range.
 *
 * PO_MEM_CL_OR_NCHK -- Similar to the flag above, except that if cloning
 *                      is not possible for whatever reason, do not perform
 *                      checksum integrity checks against the target pages.
 *
 * PO_MEM_DISCARD -- Tells the Power Manager the memory pages in RAM starting
 *                   with the specific range are not to be considered for
 *                   conservation in the hibernation file. As such, all that
 *                   memory will be discarded during a low-power transition.
 *
 * PO_MEM_PAGE_ADDRESS -- Tells the Power Manager that the starting range
 *                        are physical pages. Physical memory pages are handled
 *                        differently from the virtual ones.
 *
 * PO_MEM_BOOT_PHASE -- Reserved by the system. This indicates that the memory
 *                      pages are to be handled differently during the boot phase.
 *
 * @param[in] StartPage
 * A pointer to arbitrary data. This usually points to the starting range of RAM
 * of which this function marks the hibernation range.
 *
 * @param[in] Length
 * The length of the memory range in RAM to be marked for hibernation,
 * provided by the caller.
 *
 * @param[in] PageTag
 * The tag that identifies the page range, provided by the caller.
 */
VOID
NTAPI
PoSetHiberRange(
    _In_ PVOID HiberContext,
    _In_ ULONG Flags,
    _In_ PVOID StartPage,
    _In_ ULONG Length,
    _In_ ULONG PageTag)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
