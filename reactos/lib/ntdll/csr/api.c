/* $Id: api.c,v 1.8 2000/06/29 23:35:27 dwelch Exp $
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
   NTSTATUS Status;
   
//   DbgPrint("CsrClientCallServer(Request %x, Reply %x, Length %d, "
//	    "ReplyLength %d)\n", Request, Reply, Length, ReplyLength);
   
   Request->Header.DataSize = Length;
   Request->Header.MessageSize = sizeof(LPC_MESSAGE_HEADER) + Length;
   
   Status = NtRequestWaitReplyPort(WindowsApiPort,
				   &Request->Header,
				   &Reply->Header);
   
//   DbgPrint("Status %x\n", Status);
   
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


NTSTATUS STDCALL CsrSetPriorityClass (HANDLE	hProcess,
				      DWORD	* PriorityClass)
{
	/* FIXME: call csrss to get hProcess' priority */
	*PriorityClass = CSR_PRIORITY_CLASS_NORMAL;

	return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
