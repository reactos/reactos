/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bcd.c
 * PURPOSE:         Boot Library BCD Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* FUNCTIONS *****************************************************************/

/*++
 * @name BlGetBootOptionListSize
 *
 *     The BlGetBootOptionListSize routine
 *
 * @param  BcdOption
 *         UEFI Image Handle for the current loaded application.
 *
 * @return Size of the BCD option
 *
 *--*/
ULONG
BlGetBootOptionListSize (
    _In_ PBL_BCD_OPTION BcdOption
    )
{
    ULONG Size = 0, NextOffset = 0;
    PBL_BCD_OPTION NextOption;

    /* Loop all the options*/
    do
    {
        /* Move to the next one */
        NextOption = (PBL_BCD_OPTION)((ULONG_PTR)BcdOption + NextOffset);

        /* Compute the size of the next one */
        Size += BlGetBootOptionSize(NextOption);

        /* Update the offset */
        NextOffset = NextOption->NextEntryOffset;
    } while (NextOffset != 0);

    /* Return final computed size */
    return Size;
}

/*++
 * @name BlGetBootOptionSize
 *
 *     The BlGetBootOptionSize routine
 *
 * @param  BcdOption
 *         UEFI Image Handle for the current loaded application.
 *
 * @return Size of the BCD option
 *
 *--*/
ULONG
BlGetBootOptionSize (
    _In_ PBL_BCD_OPTION BcdOption
    )
{
    ULONG Size, Offset;

    /* Check if there's any data */
    if (BcdOption->DataOffset != 0)
    {
        /* Add the size of the data */
        Size = BcdOption->DataOffset + BcdOption->DataSize;
    }
    else
    {
        /* No data, just the structure itself */
        Size = sizeof(*BcdOption);
    }

    /* Any associated options? */
    Offset = BcdOption->ListOffset;
    if (Offset != 0)
    {
        /* Go get those too */
        Size += BlGetBootOptionListSize((PVOID)((ULONG_PTR)BcdOption + Offset));
    }

    /* Return the final size */
    return Size;
}
