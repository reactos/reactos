/* $Id: session.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 *
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/port/session.c
 * DESCRIPTION: \POSIX+\SessionPort LPC port logic.
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
#include <psx/lpcproto.h>
#include "utils.h"

/**********************************************************************
 *	ProcessConnectionRequest/				PRIVATE
 */
PRIVATE NTSTATUS STDCALL
ProcessConnectionRequest (PLPC_MAX_MESSAGE pRequest)
{
    PPSX_CONNECT_PORT_DATA pConnectData = (PPSX_CONNECT_PORT_DATA) & pRequest->Data;
    NTSTATUS               Status;
    HANDLE                 hConnectedPort;
    ULONG                  ulPortIdentifier;

    debug_print (L"PSXSS: ->"__FUNCTION__);

    /* Check if the caller is a terminal */
    Status = PsxCheckConnectionRequest (
                pConnectData,
                PSX_CONNECTION_TYPE_TERMINAL
		);
    if (!NT_SUCCESS(Status))
    {
        Status = NtAcceptConnectPort (
                    & hConnectedPort,
		    NULL,
		    & pRequest->Header,
		    FALSE, /* reject connection request */
		    NULL,
		    NULL
		    );
        if (!NT_SUCCESS(Status))
	{
            debug_print(
	        L"PSXSS: "__FUNCTION__": NtAcceptConnectPort failed with status=%08x",
		Status
		);
	}
        return STATUS_UNSUCCESSFUL;
    }
    /* OK, accept the connection */
    Status = NtAcceptConnectPort (
                & hConnectedPort,
		& ulPortIdentifier,
	        & pRequest->Header,
		TRUE, /* accept connection request */
		NULL,
		NULL
		);
    if (!NT_SUCCESS(Status))
    {
        debug_print(L"PSXSS: "__FUNCTION__": NtAcceptConnectPort failed with status=%08x", Status);
        return Status;
    }
    /* OK, now create a new PSX_SESSION object */
    Status = PsxCreateSession (
                pRequest,
		hConnectedPort,
		ulPortIdentifier
		);
    if (!NT_SUCCESS(Status))
    {
        debug_print(L"PSXSS: "__FUNCTION__": PsxCreateSession failed with status=%08x", Status);
        return Status;
    }
    Status = NtCompleteConnectPort (hConnectedPort);
    if (!NT_SUCCESS(Status))
    {
        debug_print(L"PSXSS: "__FUNCTION__": NtCompleteConnectPort failed with status=%08x", Status);
        return Status;
    }
    debug_print (L"PSXSS: <-"__FUNCTION__);
    return STATUS_SUCCESS;
}
/**********************************************************************
 *	ProcessRequest/				PRIVATE
 */
PRIVATE NTSTATUS STDCALL
ProcessRequest (PPSX_MAX_MESSAGE pRequest)
{
    debug_print (L"PSXSS: ->"__FUNCTION__);
    /* TODO: Read data from the section */
    /* TODO: send data to the process */
    debug_print (L"PSXSS: <-"__FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}
/**********************************************************************
 *	SessionPortListener/1
 *
 * DESCRIPTION
 *	Listen on port \POSIX+\SessionPort and create new sessions
 *	when a new terminal emulator calls.
 *
 * ARGUMENTS
 *	\POSIX+\SessionPort handle.
 *
 * RETURN VALUE
 *	None.
 */
VOID STDCALL
SessionPortListener (PVOID pArg)
{
    ULONG            ulIndex = (ULONG) pArg;
    NTSTATUS         Status;
    LPC_TYPE         RequestType;
    ULONG            PortIdentifier;
    PSX_MAX_MESSAGE  Request;
    PPSX_MAX_MESSAGE Reply = NULL;
    BOOL             NullReply = FALSE;

    debug_print (L"PSXSS: ->"__FUNCTION__" pArg=%d", ulIndex);

    while (TRUE)
    {
        Reply = NULL;
        NullReply = FALSE;
        while (!NullReply)
        {
            Status = NtReplyWaitReceivePort (
                        Server.Port[ulIndex].hObject,
                        0,
                        (PLPC_MESSAGE) Reply,
                        (PLPC_MESSAGE) & Request
                        );
            if (!NT_SUCCESS(Status))
            {
                break;
            }
            RequestType = PORT_MESSAGE_TYPE(Request);
            switch (RequestType)
            {
            case LPC_CONNECTION_REQUEST:
                ProcessConnectionRequest ((PLPC_MAX_MESSAGE) & Request);
                NullReply = TRUE;
                continue;
            case LPC_CLIENT_DIED:
            case LPC_PORT_CLOSED:
            case LPC_DEBUG_EVENT:
            case LPC_ERROR_EVENT:
            case LPC_EXCEPTION:
                NullReply = TRUE;
                continue;
            default:
                if (RequestType != LPC_REQUEST)
                {
                    NullReply = TRUE;
                    continue;
                }
            }
            Reply = & Request;
            Reply->PsxHeader.Status = ProcessRequest (& Request);
            NullReply = FALSE;
        }
        if ((STATUS_INVALID_HANDLE == Status) ||
            (STATUS_OBJECT_TYPE_MISMATCH == Status))
        {
            break;
        }
    }
#ifdef __PSXSS_ON_W32__
    TerminateThread(GetCurrentThread(),Status);
#else
    NtTerminateThread(NtCurrentThread(),Status);
#endif
}
/* EOF */
