/* $Id: terminal.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 * 
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/ob/terminal.c
 * DESCRIPTION: terminal
 * DATE       : 2002-04-04
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
#include <psxss.h>

/**********************************************************************
 *	WriteTerminal/4
 */
NTSTATUS STDCALL
WriteTerminal (
	IN     PPSX_TERMINAL Terminal,
	IN     PVOID Buffer,
	IN     ULONG Size,
	IN OUT PULONG WrittenSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
/**********************************************************************
 *	ReadTerminal/4
 */
NTSTATUS STDCALL
ReadTerminal (
	IN     PPSX_TERMINAL Terminal,
	IN OUT PVOID Buffer,
	IN     ULONG Size,
	IN OUT PULONG ReadSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
