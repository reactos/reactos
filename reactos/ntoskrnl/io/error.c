/* $Id: error.c,v 1.4 2000/07/04 08:52:38 dwelch Exp $
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


VOID STDCALL IoSetHardErrorOrVerifyDevice(PIRP Irp, PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

VOID STDCALL IoRaiseHardError(PIRP Irp, PVPB Vpb, PDEVICE_OBJECT RealDeviceObject)
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
