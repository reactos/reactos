/* $Id: null.c,v 1.3 2002/10/29 04:45:54 rex Exp $
 * 
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/call/null.c
 * DESCRIPTION: Void system call.
 * DATE       : 2002-04-05
 * AUTHOR     : Emanuele Aliberti <eal@users.sf.net>
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
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <ntdll/rtl.h>
#include <napi/lpc.h>
#include <psxss.h>

NTSTATUS STDCALL syscall_null (PPSX_MAX_MESSAGE Msg)
{
	Msg->PsxHeader.Status = STATUS_SUCCESS;
	return STATUS_SUCCESS;
}
/* EOF */
