/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/error.c
 * PURPOSE:         Handle media errors
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/*
 * @unimplemented
 */
VOID STDCALL 
IoRaiseHardError(PIRP Irp,
		 PVPB Vpb,
		 PDEVICE_OBJECT RealDeviceObject)
{
   UNIMPLEMENTED;
}

BOOLEAN 
IoIsTotalDeviceFailure(NTSTATUS Status)
{
   UNIMPLEMENTED;
   return(FALSE);
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL 
IoRaiseInformationalHardError(NTSTATUS ErrorStatus,
			      PUNICODE_STRING String,
			      PKTHREAD Thread)
{
   UNIMPLEMENTED;
   return(FALSE);
}


/* EOF */
