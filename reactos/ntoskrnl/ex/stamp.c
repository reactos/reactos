/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/stamp.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static ULONG TimeStamp = 0;

/* FUNCTIONS *****************************************************************/

ULONG ExGetTimeStamp(VOID)
{
   return(InterlockedIncrement(&TimeStamp));
}
