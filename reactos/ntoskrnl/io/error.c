/* $Id: error.c,v 1.5 2000/12/10 19:15:45 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


VOID STDCALL IoRaiseHardError(PIRP Irp,
			      PVPB Vpb,
			      PDEVICE_OBJECT RealDeviceObject)
{
   UNIMPLEMENTED;
}

BOOLEAN IoIsTotalDeviceFailure(NTSTATUS Status)
{
   UNIMPLEMENTED;
}

BOOLEAN STDCALL IoRaiseInformationalHardError(NTSTATUS ErrorStatus,
					      PUNICODE_STRING String,
					      PKTHREAD Thread)
{
   UNIMPLEMENTED;
}


/* EOF */
