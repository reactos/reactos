/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown Protection Functions
 * PORTABILITY:     Checked
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ldr.h>
#include <internal/kd.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Call the general function with only one Reference add */
    return ExAcquireRundownProtectionEx(RunRef, 1);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    )
{
    /* Make sure a Rundown is not in progress */
    if (RunRef->Count & EX_RUNDOWN_ACTIVE) return FALSE;

    /* Increment Reference Count */
    RunRef->Count += Count * 2;

    /* Return Success */
    return TRUE;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Set the count to zero */
    RunRef->Count = 0;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Reset the count */
    RunRef->Count = 0;
}


/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    )
{
    PRUNDOWN_DESCRIPTOR RundownDescriptor;

    /* Check if Rundown is in progress */
    if (RunRef->Count & EX_RUNDOWN_ACTIVE) {

        /* Get Pointer */
        RundownDescriptor = RunRef->Pointer;

        /* Decrease Reference Count */
        RundownDescriptor->References -= Count;

        /* If anyone else is still referencing, don't signal the event */
        if (RundownDescriptor->References) return;

        /* Signal the event so anyone waiting on it can now kill it */
        KeSetEvent(RundownDescriptor->RundownEvent, 0, FALSE);

    } else {
        /* Decrease Count */
        RunRef->Count -= Count * 2;
    }
}

/*
* @implemented
*/
VOID
FASTCALL
ExReleaseRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Call the general function with only 1 reference removal */
     ExReleaseRundownProtectionEx(RunRef, 1);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExRundownCompleted (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Remove pending rundown */
    RunRef->Count++;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExWaitForRundownProtectionRelease (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    RUNDOWN_DESCRIPTOR RundownDescriptor;

    /* Check if anyone is referencing the structure */
    if (!RunRef->Count) {
        /* It's free, set it to Rundown Mode */
        RunRef->Count = EX_RUNDOWN_ACTIVE;
    } else {
        /* Check if it's already in rundown */
        if (RunRef->Count == EX_RUNDOWN_ACTIVE) return;
    }

    /* Save number of references */
    RundownDescriptor.References = RunRef->Count / 2;

    /* Pending references... wait on them to be closed with an event */
    KeInitializeEvent(RundownDescriptor.RundownEvent, NotificationEvent, FALSE);

    /* Save Rundown Descriptor. This is safe because this stack won't be modified */
    RunRef->Pointer = &RundownDescriptor;

    /* Set the Count to 1 so nobody else acquires and so release notifies us */
    RunRef->Count++;

    /* Wait for whoever needs to release to notify us */
    KeWaitForSingleObject(RundownDescriptor.RundownEvent, Executive, KernelMode, FALSE, NULL);
}
/* EOF */
