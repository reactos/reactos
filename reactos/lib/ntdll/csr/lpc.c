/* $Id: lpc.c,v 1.1 2001/06/17 20:05:09 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/lpc.c
 * PURPOSE:         CSRSS Client/Server LPC API
 *
 * REVISIONS:
 * 	2001-06-16 (ea)
 * 		File api.c renamed lpc.c. Process/thread code moved
 * 		in thread.c. Check added on the LPC port.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <string.h>

#include <csrss/csrss.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

HANDLE WindowsApiPort = INVALID_HANDLE_VALUE;

/* FUNCTIONS *****************************************************************/

/* Possible CsrClientCallServer (the NT one):

NTSTATUS STDCALL
CsrClientCallServer(PCSRSS_XXX_REQUEST Request,
		    PCSRSS_XXX_REPLY Reply OPTIONAL,
		    ULONG CsrApiNumber,
		    ULONG MaxRequestReplyLength)

XXX_REQUEST and XXX_REPLY depend on the CsrApiNumber value and are not LPC
objects (the LPC_REQUEST is built here instead).
If Reply == NULL, use storage of Request to write the reply.

TO BE VERIFIED.

*/
NTSTATUS STDCALL
CsrClientCallServer(PCSRSS_API_REQUEST Request,
		    PCSRSS_API_REPLY Reply OPTIONAL,
		    ULONG Length,
		    ULONG ReplyLength)
{
   NTSTATUS Status;

   if (INVALID_HANDLE_VALUE == WindowsApiPort)
   {
	   DbgPrint ("NTDLL.%s: client not connected to CSRSS!\n", __FUNCTION__);
	   return (STATUS_UNSUCCESSFUL);
   }
   
//   DbgPrint("CsrClientCallServer(Request %x, Reply %x, Length %d, "
//	    "ReplyLength %d)\n", Request, Reply, Length, ReplyLength);
   
   Request->Header.DataSize = Length;
   Request->Header.MessageSize = sizeof(LPC_MESSAGE_HEADER) + Length;
  
   
   Status = NtRequestWaitReplyPort(WindowsApiPort,
				   &Request->Header,
				   (Reply?&Reply->Header:&Request->Header));
   
//   DbgPrint("Status %x\n", Status);
   
   return(Status);
}

NTSTATUS STDCALL
CsrClientConnectToServer(VOID)
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

/* EOF */
