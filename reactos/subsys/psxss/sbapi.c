/* $Id: sbapi.c,v 1.2 1999/09/07 17:12:39 ea Exp $
 *
 * sbapi.c - Displatcher for the \SbApiPort
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
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#define PROTO_LPC
#include <ddk/ntddk.h>

/* The \SbApi dispatcher: what is this port for? */

LPC_RETURN_CODE
PortRequestDispatcher_PsxSbApi (
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	return LPC_ERROR_INVALID_FUNCTION;
}


/* EOF */
