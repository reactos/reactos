/* $Id: sbapi.c,v 1.3 2002/10/29 04:45:58 rex Exp $
 *
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/port/sbapi.c
 * DESCRIPTION: \POSIX+\SbApiPort LPC logic.
 * DATE       : 2001-03-23
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
 *	ProcessConnectionRequest/				PRIVATE
 */
PRIVATE NTSTATUS STDCALL
ProcessConnectionRequest (PPSX_MESSAGE pRequest)
{
    return STATUS_NOT_IMPLEMENTED;
}
/**********************************************************************
 *	ProcessRequest/				PRIVATE
 */
PRIVATE NTSTATUS STDCALL
ProcessRequest (PPSX_MESSAGE pRequest)
{
    return STATUS_NOT_IMPLEMENTED;
}
/**********************************************************************
 *	SbApiPortListener/1
 *
 * DESCRIPTION
 *	The \POSIX+\SbApiPort LPC port message dispatcher.
 *
 * NOTE
 *	what is this port for? Is "Sb" for "shared block"?
 */
VOID STDCALL
SbApiPortListener (PVOID pArg)
{
    NTSTATUS         Status;
    ULONG            PortIdentifier;
    PSX_MAX_MESSAGE  Request;
    PPSX_MAX_MESSAGE Reply = NULL;

    debug_print (L"PSXSS: ->"__FUNCTION__" pArg=%d", (ULONG) pArg);

    RtlZeroMemory (& Request, sizeof Request);
    /* TODO */
#ifdef __PSXSS_ON_W32__
    Sleep(30000);
    TerminateThread(GetCurrentThread(),Status);
#else
    NtTerminateThread(NtCurrentThread(),Status);
#endif
}
/* EOF */
