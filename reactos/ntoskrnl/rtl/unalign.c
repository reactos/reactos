/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/unalign.c
 * PURPOSE:         Unaligned stores and loads
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID RtlStoreUlong(PULONG Address,
		   ULONG Value)
{
   *Address=Value;
}

VOID RtlStoreUshort(PUSHORT Address,
		    USHORT Value)
{
   *Address=Value;
}

VOID RtlRetrieveUlong(PULONG DestinationAddress,
		      PULONG SourceAddress)
{
   *DestinationAddress = *SourceAddress;
}

VOID RtlRetrieveUshort(PUSHORT DestinationAddress,
		       PUSHORT SourceAddress)
{
   *DestinationAddress = *SourceAddress;
}
