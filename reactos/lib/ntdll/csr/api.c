/* $Id: api.c,v 1.4 2000/03/22 18:35:50 dwelch Exp $
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

NTSTATUS STDCALL CsrClientCallServer(PCSRSS_API_REQUEST Request,
				     PCSRSS_API_REPLY Reply,
				     ULONG Length,
				     ULONG ReplyLength)
{
   LPCMESSAGE LpcRequest;
   LPCMESSAGE LpcReply;
   NTSTATUS Status;
   
//   DbgPrint("Length %d\n", Length);
   
   LpcRequest.ActualMessageLength = Length;
   LpcRequest.TotalMessageLength = sizeof(LPCMESSAGE) + Length;
   memcpy(LpcRequest.MessageData, Request, Length);
   
   Status = NtRequestWaitReplyPort(WindowsApiPort,
				   &LpcRequest,
				   &LpcReply);
   memcpy(Reply, LpcReply.MessageData, ReplyLength);
//   DbgPrint("Status %x Reply.Status %x\n", Status, Reply->Status);
   return(Status);
}

NTSTATUS STDCALL CsrClientConnectToServer(VOID)
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
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
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
