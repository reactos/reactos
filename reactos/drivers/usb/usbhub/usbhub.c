/*
 * ReactOS USB hub driver
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
 */

/* INCLUDES *******************************************************************/
#include <stddef.h>
#include <windef.h>
#include <ddk/ntddk.h>
#include "../usbport/usbport.h"

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PVOID Context1, IN PVOID Context2)
{
	return STATUS_SUCCESS;
}

