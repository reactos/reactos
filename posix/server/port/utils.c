/* $Id: utils.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 *
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/port/utils.c
 * DESCRIPTION: LPC port utilities.
 * DATE       : 2002-04-07
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
#include "utils.h"

/**********************************************************************
 *	PsxCheckConnectionRequest/2
 *
 * DESCRIPTION
 *	Check if we can accept the connection request sent to
 *      an LPC port. Protocol version and ConnectionType MUST match.
 */
NTSTATUS STDCALL
PsxCheckConnectionRequest (
    IN OUT PPSX_CONNECT_PORT_DATA  pConnectData,
    IN     PSX_CONNECTION_TYPE     ConnectionType
    )
{
    /* Check if the caller is ConnectionType */
    if (ConnectionType != pConnectData->ConnectionType)
    {
        debug_print(
	    L"PSXSS: "__FUNCTION__": ConnectionType=%d, expected %d",
	    pConnectData->ConnectionType,
	    ConnectionType
	    );
        return STATUS_UNSUCCESSFUL;
    }
    /* Check if the LPC protocol version matches */
    if (PSX_LPC_PROTOCOL_VERSION != pConnectData->Version)
    {
        debug_print(
	    L"PSXSS: "__FUNCTION__": Version=%d, expected %d",
            pConnectData->Version,
	    PSX_LPC_PROTOCOL_VERSION
	    );
        pConnectData->Version = PSX_LPC_PROTOCOL_VERSION;
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}
/* EOF */
