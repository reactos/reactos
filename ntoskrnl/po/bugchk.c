/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager graceful bug check support for PoShutdownBugCheck
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Crashes down the system and performs a power action.
 * This is typically used for debugging purposes on forced
 * shutdowns to test the power rundown states.
 *
 * @param[in] LogError
 * If set to TRUE, the function will poke the I/O manager
 * to write a specific log dump describing the reason of the crash.
 *
 * @param[in] BugCheckCode
 * The main bugcheck value that indicates the reason of the crash.
 *
 * @param[in] BugCheckParameter1
 * The additional parameter for the bugcheck indicating the reason
 * of the crash.
 *
 * @param[in] BugCheckParameter2
 * The additional 2nd parameter for the bugcheck indicating the
 * reason of the crash.
 *
 * @param[in] BugCheckParameter3
 * The additional 3rd parameter for the bugcheck indicating the
 * reason of the crash.
 *
 * @param[in] BugCheckParameter4
 * The additional 4th parameter for the bugcheck indicating the
 * reason of the crash.
 */
VOID
NTAPI
PoShutdownBugCheck(
    _In_ BOOLEAN LogError,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
