/* $Id$
 *
 * server.c - ReactOS/Win32 base enviroment subsystem server
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 */
#include "basesrv.h"

#define NDEBUG
#include <debug.h>

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL BaseSetProcessCreateNotify (BASE_PROCESS_CREATE_NOTIFY_ROUTINE ProcessCreateNotifyProc)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	DPRINT("BASESRV: %s(%08lx) called\n", __FUNCTION__, ProcessCreateNotifyProc);
	return Status;
}

/* EOF */
