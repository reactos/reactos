/* $Id: wapi.c,v 1.37 2004/07/03 17:15:02 hbirr Exp $
 * 
 * reactos/subsys/csrss/api/wapi.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <csrss/csrss.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <debug.h>

#include "api.h"

/* GLOBALS *******************************************************************/

HANDLE CsrssApiHeap;

static unsigned ApiDefinitionsCount = 0;
static PCSRSS_API_DEFINITION ApiDefinitions = NULL;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
CsrApiRegisterDefinitions(PCSRSS_API_DEFINITION NewDefinitions)
{
  unsigned NewCount;
  PCSRSS_API_DEFINITION Scan;
  PCSRSS_API_DEFINITION New;

  NewCount = 0;
  for (Scan = NewDefinitions; 0 != Scan->Handler; Scan++)
    {
      NewCount++;
    }

  New = RtlAllocateHeap(CsrssApiHeap, 0,
                        (ApiDefinitionsCount + NewCount)
                        * sizeof(CSRSS_API_DEFINITION));
  if (NULL == New)
    {
      DPRINT1("Unable to allocate memory\n");
      return STATUS_NO_MEMORY;
    }
  if (0 != ApiDefinitionsCount)
    {
      RtlCopyMemory(New, ApiDefinitions,
                    ApiDefinitionsCount * sizeof(CSRSS_API_DEFINITION));
      RtlFreeHeap(CsrssApiHeap, 0, ApiDefinitions);
    }
  RtlCopyMemory(New + ApiDefinitionsCount, NewDefinitions,
                NewCount * sizeof(CSRSS_API_DEFINITION));
  ApiDefinitions = New;
  ApiDefinitionsCount += NewCount;

  return STATUS_SUCCESS;
}

VOID FASTCALL
CsrApiCallHandler(PCSRSS_PROCESS_DATA ProcessData,
                  PCSRSS_API_REQUEST Request,
                  PCSRSS_API_REPLY Reply)
{
  BOOL Found;
  unsigned DefIndex;

  Found = FALSE;
  for (DefIndex = 0; ! Found && DefIndex < ApiDefinitionsCount; DefIndex++)
    {
      if (ApiDefinitions[DefIndex].Type == Request->Type)
        {
          if (Request->Header.DataSize < ApiDefinitions[DefIndex].MinRequestSize)
            {
              DPRINT1("Request type %d min request size %d actual %d\n",
                      Request->Type, ApiDefinitions[DefIndex].MinRequestSize,
                      Request->Header.DataSize);
              Reply->Status = STATUS_INVALID_PARAMETER;
            }
          else
            {
              (ApiDefinitions[DefIndex].Handler)(ProcessData, Request, Reply);
              Found = TRUE;
            }
        }
    }
  if (! Found)
    {
      DPRINT1("CSR: Unknown request type 0x%x\n", Request->Type);
      Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
      Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
      Reply->Status = STATUS_INVALID_SYSTEM_SERVICE;
    }
}

static void STDCALL
ClientConnectionThread(HANDLE ServerPort)
{
  NTSTATUS Status;
  LPC_MAX_MESSAGE LpcReply;
  LPC_MAX_MESSAGE LpcRequest;
  PCSRSS_API_REQUEST Request;
  PCSRSS_PROCESS_DATA ProcessData;
  PCSRSS_API_REPLY Reply;
   
  Reply = NULL;
   
  for (;;)
    {
      Status = NtReplyWaitReceivePort(ServerPort,
                                      0,
                                      &Reply->Header,
                                      &LpcRequest.Header);
      if (! NT_SUCCESS(Status))
        {
          DPRINT1("CSR: NtReplyWaitReceivePort failed\n");
          break;
        }
	
      if (LpcRequest.Header.MessageType == LPC_PORT_CLOSED)
        {
          CsrFreeProcessData( (ULONG)LpcRequest.Header.ClientId.UniqueProcess );
          break;
        }

      Request = (PCSRSS_API_REQUEST)&LpcRequest;
      Reply = (PCSRSS_API_REPLY)&LpcReply;
	
      ProcessData = CsrGetProcessData((ULONG)LpcRequest.Header.ClientId.UniqueProcess);
      if (ProcessData == NULL)
        {
          DPRINT1("CSR: Message %d: Unable to find data for process %d\n",
	          LpcRequest.Header.MessageType, (ULONG)LpcRequest.Header.ClientId.UniqueProcess);
	  break;
        }	          


      CsrApiCallHandler(ProcessData, Request, Reply);
    }
  NtClose(ServerPort);
  RtlRosExitUserThread(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME
 *	Thread_Api
 *
 * DESCRIPTION
 * 	Handle connection requests from clients to the port
 * 	"\Windows\ApiPort".
 */
void STDCALL
ServerApiPortThead(PVOID PortHandle)
{
   NTSTATUS Status;
   LPC_MAX_MESSAGE Request;
   HANDLE ServerPort;
   HANDLE ServerThread;
   PCSRSS_PROCESS_DATA ProcessData;
   
   CsrInitProcessData();
   
   for (;;)
     {
        LPC_SECTION_READ LpcRead;
        ServerPort = NULL;

	Status = NtListenPort(PortHandle, &Request.Header);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: NtListenPort() failed\n");
	     break;
	  }
	Status = NtAcceptConnectPort(&ServerPort,
				     PortHandle,
				     NULL,
				     1,
				     0,
				     &LpcRead);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: NtAcceptConnectPort() failed\n");
	     break;
	  }

	ProcessData = CsrCreateProcessData((ULONG)Request.Header.ClientId.UniqueProcess);
	if (ProcessData == NULL)
	  {
	     DPRINT1("Unable to allocate or find data for process %d\n",
	             (ULONG)Request.Header.ClientId.UniqueProcess);
	     Status = STATUS_UNSUCCESSFUL;
	     break;
	  }
	    

	ProcessData->CsrSectionViewBase = LpcRead.ViewBase;
	ProcessData->CsrSectionViewSize = LpcRead.ViewSize;
	
	Status = NtCompleteConnectPort(ServerPort);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: NtCompleteConnectPort() failed\n");
	     break;
	  }
	
	Status = RtlCreateUserThread(NtCurrentProcess(),
				     NULL,
				     FALSE,
				     0,
				     NULL,
				     NULL,
				     (PTHREAD_START_ROUTINE)ClientConnectionThread,
				     ServerPort,
				     &ServerThread,
				     NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: Unable to create server thread\n");
	     break;
	  }
	NtClose(ServerThread);
     }
   if (ServerPort)
     {
       NtClose(ServerPort);
     }
   NtClose(PortHandle);
   NtTerminateThread(NtCurrentThread(), Status);
}

/* EOF */
