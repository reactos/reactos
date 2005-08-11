/* $Id$
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

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE WindowsApiPort = INVALID_HANDLE_VALUE;
static PVOID CsrSectionMapBase = NULL;
static PVOID CsrSectionMapServerBase = NULL;
static HANDLE CsrCommHeap = NULL;

#define CSR_CONTROL_HEAP_SIZE (65536)

/* FUNCTIONS *****************************************************************/

/* Possible CsrClientCallServer (the NT one): */

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
  if(ParameterBuffer != NULL)
  {
    memcpy(Block, ParameterBuffer, ParameterBufferSize);
  }
  *ClientAddress = Block;
  *ServerAddress = (PVOID)((ULONG_PTR)Block - (ULONG_PTR)CsrSectionMapBase + (ULONG_PTR)CsrSectionMapServerBase);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
CsrReleaseParameterBuffer(PVOID ClientAddress)
{
  RtlFreeHeap(CsrCommHeap, 0, ClientAddress);
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
CsrClientCallServer(PCSR_API_MESSAGE Request,
                    PVOID CapturedBuffer OPTIONAL,
                    CSR_API_NUMBER ApiNumber,
                    ULONG RequestLength)
{
    NTSTATUS Status;
    DPRINT("CSR: CsrClientCallServer!\n");
  
    /* Make sure it's valid */
    if (INVALID_HANDLE_VALUE == WindowsApiPort)
    {
        DPRINT1("NTDLL.%s: client not connected to CSRSS!\n", __FUNCTION__);
        return (STATUS_UNSUCCESSFUL);
    }

    /* Fill out the header */
    Request->Type = ApiNumber;
    Request->Header.u1.s1.DataLength = RequestLength - LPC_MESSAGE_BASE_SIZE;
    Request->Header.u1.s1.TotalLength = RequestLength;
    DPRINT("CSR: API: %x, u1.s1.DataLength: %x, u1.s1.TotalLength: %x\n", 
            ApiNumber,
            Request->Header.u1.s1.DataLength,
            Request->Header.u1.s1.TotalLength);
                
    /* Send the LPC Message */
    Status = NtRequestWaitReplyPort(WindowsApiPort,
                                    &Request->Header,
                                    &Request->Header);
        
    DPRINT("Got back: %x\n", Status);
    return(Status);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
CsrClientConnectToServer(PWSTR ObjectDirectory,
                         ULONG ServerId,
                         PVOID Unknown,
                         PVOID Context,
                         ULONG ContextLength,
                         PBOOLEAN ServerToServerCall)
{
   NTSTATUS Status;
   UNICODE_STRING PortName = RTL_CONSTANT_STRING(L"\\Windows\\ApiPort");
   ULONG ConnectInfoLength;
   CSR_API_MESSAGE Request;
   PORT_VIEW LpcWrite;
   HANDLE CsrSectionHandle;
   LARGE_INTEGER CsrSectionViewSize;

   if (WindowsApiPort != INVALID_HANDLE_VALUE)
     {
       return STATUS_SUCCESS;
     }

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
   ConnectInfoLength = 0;
   LpcWrite.Length = sizeof(PORT_VIEW);
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
     	WindowsApiPort = INVALID_HANDLE_VALUE;
	return(Status);
     }

   NtClose(CsrSectionHandle);
   CsrSectionMapBase = LpcWrite.ViewBase;
   CsrSectionMapServerBase = LpcWrite.ViewRemoteBase;

   /* Create the heap for communication for csrss. */
   CsrCommHeap = RtlCreateHeap(0,
			       CsrSectionMapBase,
			       CsrSectionViewSize.u.LowPart,
			       CsrSectionViewSize.u.LowPart,
			       0,
			       NULL);
   if (CsrCommHeap == NULL)
     {
       return(STATUS_NO_MEMORY);
     }

   Status = CsrClientCallServer(&Request,
				NULL,
				MAKE_CSR_API(CONNECT_PROCESS, CSR_NATIVE),
				sizeof(CSR_API_MESSAGE));
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (!NT_SUCCESS(Request.Status))
     {
	return(Request.Status);
     }
   return(STATUS_SUCCESS);
}

/* EOF */
