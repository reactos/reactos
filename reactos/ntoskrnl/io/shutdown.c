/* $Id: shutdown.c,v 1.2 2000/03/26 19:38:26 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/shutdown.c
 * PURPOSE:         Implements shutdown notification
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL IoRegisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

VOID STDCALL IoUnregisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

/* EOF */
