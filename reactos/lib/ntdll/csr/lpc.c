/* $Id: lpc.c,v 1.7 2002/09/07 15:12:38 chorns Exp $
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

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>
#include <csrss/csrss.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE WindowsApiPort = INVALID_HANDLE_VALUE;
static PVOID CsrSectionMapBase = NULL;
static PVOID CsrSectionMapServerBase = NULL;
static HANDLE CsrCommHeap = NULL;

#define CSR_CONTROL_HEAP_SIZE (65536)

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
CsrCaptureParameterBuffer(PVOID ParameterBuffer,
			  ULONG ParameterBufferSize,
			  PVOID* ClientAddress,
			  PVOID* ServerAddress)
{
  PVOID Block;

  Block = RtlAllocateHeap(CsrCommHeap, 0, ParameterBufferSize);
  if (Block == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
  memcpy(Block, ParameterBuffer, ParameterBufferSize);
  *ClientAddress = Block;
  *ServerAddress = Block - CsrSectionMapBase + CsrSectionMapServerBase;
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
CsrReleaseParameterBuffer(PVOID ClientAddress)
{
  RtlFreeHeap(CsrCommHeap, 0, ClientAddress);
  return(STATUS_SUCCESS);
}

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
  
   Request->Header.DataSize = Length - sizeof(LPC_MESSAGE);
   Request->Header.MessageSize = Length;
   
   Status = NtRequestWaitReplyPort(WindowsApiPort,
				   &Request->Header,
				   (Reply?&Reply->Header:&Request->Header));
   
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
   LPC_SECTION_WRITE LpcWrite;
   HANDLE CsrSectionHandle;
   LARGE_INTEGER CsrSectionViewSize;

   CsrSectionViewSize.QuadPart = CSR_CSRSS_SECTION_SIZE;
   Status = NtCreateSection(&CsrSectionHandle,
			    SECTION_ALL_ACCESS,
			    NULL,
			    &CsrSectionViewSize,
			    PAGE_READWRITE,
			    SEC_COMMIT,
			    NULL);
   if (!NT_SUCCESS(Status))
     {
       return(Status);
     }
   RtlInitUnicodeStringFromLiteral(&PortName, L"\\Windows\\ApiPort");
   ConnectInfoLength = 0;
   LpcWrite.Length = sizeof(LPC_SECTION_WRITE);
   LpcWrite.SectionHandle = CsrSectionHandle;
   LpcWrite.SectionOffset = 0;
   LpcWrite.ViewSize = CsrSectionViewSize.u.LowPart;
   Status = NtConnectPort(&WindowsApiPort,
			  &PortName,
			  NULL,
			  &LpcWrite,
			  NULL,
			  NULL,
			  NULL,
			  &ConnectInfoLength);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   NtClose(CsrSectionHandle);
   CsrSectionMapBase = LpcWrite.ViewBase;
   CsrSectionMapServerBase = LpcWrite.TargetViewBase;

   /* Create the heap for communication for csrss. */
   CsrCommHeap = RtlCreateHeap(HEAP_NO_VALLOC,
			       CsrSectionMapBase,
			       CsrSectionViewSize.u.LowPart,
			       CsrSectionViewSize.u.LowPart,
			       0,
			       NULL);
   if (CsrCommHeap == NULL)
     {
       return(STATUS_NO_MEMORY);
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
