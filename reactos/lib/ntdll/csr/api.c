/* $Id: api.c,v 1.3 2000/02/27 02:05:06 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/api.c
 * PURPOSE:         CSRSS API
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <string.h>

#include <csrss/csrss.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

static HANDLE WindowsApiPort;

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
CsrClientCallServer(PCSRSS_API_REQUEST Request,
		    PCSRSS_API_REPLY Reply,
		    ULONG Unknown3,
		    ULONG Unknown4)
{
   LPCMESSAGE LpcRequest;
   LPCMESSAGE LpcReply;
   NTSTATUS Status;
   
   LpcRequest.ActualMessageLength = MAX_MESSAGE_DATA;
   LpcRequest.TotalMessageLength = sizeof(LPCMESSAGE);
   memcpy(LpcRequest.MessageData, Request, sizeof(CSRSS_API_REQUEST));
   
   Status = NtRequestWaitReplyPort(WindowsApiPort,
				   &LpcRequest,
				   &LpcReply);
   return(Status);
}

NTSTATUS
STDCALL
CsrClientConnectToServer(ULONG Unknown1,
			 ULONG Unknown2,
			 ULONG Unknown3,
			 ULONG Unknown4,
			 ULONG Unknown5,
			 ULONG Unknown6)
{
   NTSTATUS Status;
   UNICODE_STRING PortName;
   ULONG ConnectInfoLength;
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   
   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   ConnectInfoLength = 0;
   Status = NtConnectPort(&WindowsApiPort,
			  &PortName,
			  NULL,
			  NULL,
			  NULL,
			  NULL,
			  NULL,
			  &ConnectInfoLength);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Request.Type = CSRSS_CONNECT_PROCESS;
   Status = CsrClientCallServer(&Request,
				&Reply,
				0, 0);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (!NT_SUCCESS(Reply.Status))
     {
	return(Reply.Status);
     }
   return(STATUS_SUCCESS);
}
