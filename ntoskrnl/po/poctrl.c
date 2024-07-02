/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager control switches (lid, power buttons, etc) mechanisms
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PopControlSwitches;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopSetButtonPowerAction(
    _Inout_ PPOWER_ACTION_POLICY Button,
    _In_ POWER_ACTION Action)
{
    PAGED_CODE();

    /* Punish the caller for bogus power actions */
    ASSERT((Action >= PowerActionNone) && (Action <= PowerActionDisplayOff));

    /* Setup the actions for this button */
    Button->Action = Action;
    Button->Flags = 0;
    Button->EventCode = 0;
}

/* EOF */
