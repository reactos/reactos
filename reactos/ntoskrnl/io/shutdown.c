/*
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

NTSTATUS IoRegisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

VOID IoUnregisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}
