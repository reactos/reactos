/* $Id: lpc.c,v 1.2 2001/08/31 20:06:17 ea Exp $
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

#define CSR_CCS_NATIVE	0x0000
#define CSR_CCS_CSR	0x0001
#define CSR_CCS_GUI	0x0002

typedef union _CSR_CCS_API
{
	WORD	Index;		// CSRSS API number
	WORD	Subsystem;	// 0=NTDLL;1=KERNEL32;2=KERNEL32

} CSR_CCS_API, * PCSR_CCS_API;

NTSTATUS STDCALL
CsrClientCallServer(PVOID Request,
		    PVOID Unknown OPTIONAL,
		    CSR_CCS_API CsrApi,
		    ULONG SizeOfData);
		    
Request is the family of PCSRSS_XXX_REQUEST objects.
XXX_REQUEST depend on the CsrApiNumber.Index.

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
