/*
 * ReactOS USB UHCI miniport driver
 * Copyright (C) 2004 Mark Tempel
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
 */

/* INCLUDES *******************************************************************/

#include "usbuhci.h"
#include "../../usbport/usbport.h"
#include <debug.h>
/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	//return STATUS_SUCCESS;
	//DPRINT1("USBUHCI.SYS DriverEntry\n");
	PUSB_CONTROLLER_INTERFACE ControllerInterface;

	USBPORT_AllocateUsbControllerInterface(&ControllerInterface);

	/*
	 * Set up the list of callbacks here.
	 * TODO TODO TODO
	 */
	
	return USBPORT_RegisterUSBPortDriver(DriverObject, 0, ControllerInterface);
}
