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
/* $Id: cancel.c,v 1.10 2003/07/10 15:47:00 royce Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/cancel.c
 * PURPOSE:         Cancel routine
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK CancelSpinLock;

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtCancelIoFile (IN	HANDLE			FileHandle,
		OUT	PIO_STATUS_BLOCK	IoStatusBlock)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
BOOLEAN STDCALL 
IoCancelIrp(PIRP Irp)
{
   KIRQL oldlvl;
   
   DPRINT("IoCancelIrp(Irp %x)\n",Irp);
   
   IoAcquireCancelSpinLock(&oldlvl);
   Irp->Cancel = TRUE;
   if (Irp->CancelRoutine == NULL)
   {
      IoReleaseCancelSpinLock(oldlvl);
      return(FALSE);
   }
   Irp->CancelIrql = oldlvl;
   Irp->CancelRoutine(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
   return(TRUE);
}

VOID 
IoInitCancelHandling(VOID)
{
   KeInitializeSpinLock(&CancelSpinLock);
}

/*
 * @implemented
 */
VOID STDCALL 
IoAcquireCancelSpinLock(PKIRQL Irql)
{
   KeAcquireSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
VOID STDCALL 
IoReleaseCancelSpinLock(KIRQL Irql)
{
   KeReleaseSpinLock(&CancelSpinLock,Irql);
}

/* EOF */
