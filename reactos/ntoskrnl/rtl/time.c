/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/time.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID RtlTimeToTimeFields(PLARGE_INTEGER Time,
			 PTIME_FIELDS TimeFields)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlTimeFieldsToTime(PTIME_FIELDS TimeFields,
			    PLARGE_INTEGER Time)
{
   UNIMPLEMENTED;
}
