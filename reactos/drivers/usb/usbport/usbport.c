/*
 * ReactOS USB Port driver
 * Copyright (C) 2004 Aleksey Bragin
 *           (C) 2005 Mark Tempel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * STATUS:
 *   19-Dec-2004 - just a stub for now, but with useful info by Filip
 */

/* INCLUDES *******************************************************************/
#include <stddef.h>
#include <windef.h>
#include <ddk/ntddk.h>
#include "usbport.h"
#include <debug.h>

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

/*
** Standard DriverEntry method.
** We do nothing here.  All real work is done in USBPRORT_RegisterUSBPortDriver.
*/
NTSTATUS STDCALL
DriverEntry(IN PVOID Context1, IN PVOID Context2)
{
	DPRINT1("USBPORT.SYS DriverEntry\n");
	return STATUS_SUCCESS;
}
/*
 * This method is used by miniports to connect set up 
 */
NTSTATUS STDCALL
USBPORT_RegisterUSBPortDriver(PDRIVER_OBJECT DriverObject, DWORD Unknown1,
    PUSB_CONTROLLER_INTERFACE Interface)
{
	//DPRINT1("USBPORT_RegisterUSBPortDriver\n");
	ASSERT(KeGetCurrentIRQL() < DISPATCH_LEVEL);

	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
USBPORT_GetHciMn(VOID)
{
	return 0x10000001;
}
/*
 * This method is to allow miniports to create 
 */
NTSTATUS STDCALL
USBPORT_AllocateUsbControllerInterface(OUT PUSB_CONTROLLER_INTERFACE *pControllerInterface)
{
	//DPRINT1("USBPORT_AllocateUsbControllerInterface\n");
	ASSERT(KeGetCurrentIRQL() < DISPATCH_LEVEL);
	ASSERT(0 != ControllerObject);

	*pControllerInterface = (PUSB_CONTROLLER_INTERFACE)ExAllocatePoolWithTag(PagedPool, sizeof(USB_CONTROLLER_INTERFACE),USB_CONTROLLER_INTERFACE_TAG);
	RtlZeroMemory(*pControllerInterface, sizeof(USB_CONTROLLER_INTERFACE));
	
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
USBPORT_FreeUsbControllerInterface(IN PUSB_CONTROLLER_INTERFACE ControllerInterface)
{
	//DPRINT1("USBPORT_FreeUsbControllerInterface\n");
	ASSERT(KeGetCurrentIRQL() < DISPATCH_LEVEL);	

	ExFreePool(ControllerInterface);
	
	return STATUS_SUCCESS;
}
