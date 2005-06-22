/* $Id$
 *
 * reactos/subsys/csrss/api/wapi.c
 *
 * CSRSS port message processing
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <rosrtl/thread.h>

#include "api.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE CsrssApiHeap = (HANDLE) 0;

static unsigned ApiDefinitionsCount = 0;
static PCSRSS_API_DEFINITION ApiDefinitions = NULL;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
CsrApiRegisterDefinitions(PCSRSS_API_DEFINITION NewDefinitions)
{
  unsigned NewCount;
  PCSRSS_API_DEFINITION Scan;
  PCSRSS_API_DEFINITION New;

	DPRINT("CSR: %s called", __FUNCTION__);

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

VOID 
FASTCALL
CsrApiCallHandler(PCSRSS_PROCESS_DATA ProcessData,
                  PCSR_API_MESSAGE Request)
{
  BOOL Found = FALSE;
  unsigned DefIndex;
  ULONG Type;
  
  DPRINT("CSR: Calling handler for type: %x.\n", Request->Type);
  Type = Request->Type & 0xFFFF; /* FIXME: USE MACRO */
  DPRINT("CSR: API Number: %x ServerID: %x\n",Type, Request->Type >> 16);

  /* FIXME: Extract DefIndex instead of looping */
  for (DefIndex = 0; ! Found && DefIndex < ApiDefinitionsCount; DefIndex++)
    {
      if (ApiDefinitions[DefIndex].Type == Type)
        {
          if (Request->Header.DataSize < ApiDefinitions[DefIndex].MinRequestSize)
            {
              DPRINT1("Request type %d min request size %d actual %d\n",
                      Type, ApiDefinitions[DefIndex].MinRequestSize,
                      Request->Header.DataSize);
              Request->Status = STATUS_INVALID_PARAMETER;
            }
          else
            {
              (ApiDefinitions[DefIndex].Handler)(ProcessData, Request);
              Found = TRUE;
            }
        }
    }
  if (! Found)
    {
      DPRINT1("CSR: Unknown request type 0x%x\n", Request->Type);
      Request->Header.MessageSize = sizeof(CSR_API_MESSAGE);
      Request->Header.DataSize = sizeof(CSR_API_MESSAGE) - LPC_MESSAGE_BASE_SIZE;
      Request->Status = STATUS_INVALID_SYSTEM_SERVICE;
    }
}

STATIC
VOID
STDCALL
ClientConnectionThread(HANDLE ServerPort)
{
    NTSTATUS Status;
    LPC_MAX_MESSAGE LpcReply;
    LPC_MAX_MESSAGE LpcRequest;
    PCSR_API_MESSAGE Request;
    PCSR_API_MESSAGE Reply;
    PCSRSS_PROCESS_DATA ProcessData;
  
    DPRINT("CSR: %s called", __FUNCTION__);

    /* Loop and reply/wait for a new message */
    for (;;)
    {
        /* Send the reply and wait for a new request */
        Status = NtReplyWaitReceivePort(ServerPort,
                                        0,
                                        &Reply->Header,
                                        &LpcRequest.Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CSR: NtReplyWaitReceivePort failed\n");
            break;
        }
        
        /* If the connection was closed, handle that */
        if (LpcRequest.Header.MessageType == LPC_PORT_CLOSED)
        {
            CsrFreeProcessData( LpcRequest.Header.ClientId.UniqueProcess );
            break;
        }
        
        /* Get the CSR Message */
        Request = (PCSR_API_MESSAGE)&LpcRequest;
        Reply = (PCSR_API_MESSAGE)&LpcReply;
      
        DPRINT("CSR: Got CSR API: %x [Message Origin: %x]\n", 
                Request->Type, 
                Request->Header.ClientId.UniqueProcess);

        /* Get the Process Data */
        ProcessData = CsrGetProcessData(LpcRequest.Header.ClientId.UniqueProcess);
        if (ProcessData == NULL)
        {
            DPRINT1("CSR: Message %d: Unable to find data for process 0x%x\n",
                    LpcRequest.Header.MessageType,
                    LpcRequest.Header.ClientId.UniqueProcess);
            break;
        }

        /* Call the Handler */
        CsrApiCallHandler(ProcessData, Request);
        
        /* Send back the reply */
        Reply = Request;
    }
    
    /* Close the port and exit the thread */
    NtClose(ServerPort);
    RtlRosExitUserThread(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME
 *	ServerApiPortThread/1
 *
 * DESCRIPTION
 * 	Handle connection requests from clients to the port
 * 	"\Windows\ApiPort".
 */
DWORD STDCALL
ServerApiPortThread (PVOID PortHandle)
{
   NTSTATUS Status = STATUS_SUCCESS;
   LPC_MAX_MESSAGE Request;
   HANDLE hApiListenPort = * (PHANDLE) PortHandle;
   HANDLE ServerPort = (HANDLE) 0;
   HANDLE ServerThread = (HANDLE) 0;
   PCSRSS_PROCESS_DATA ProcessData = NULL;

   CsrInitProcessData();

	DPRINT("CSR: %s called", __FUNCTION__);

   for (;;)
     {
        LPC_SECTION_READ LpcRead;
        ServerPort = NULL;

	Status = NtListenPort (hApiListenPort, & Request.Header);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: NtListenPort() failed\n");
	     break;
	  }
	Status = NtAcceptConnectPort(& ServerPort,
				     hApiListenPort,
				     NULL,
				     TRUE,
				     0,
				     & LpcRead);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("CSR: NtAcceptConnectPort() failed\n");
	     break;
	  }

	ProcessData = CsrCreateProcessData(Request.Header.ClientId.UniqueProcess);
	if (ProcessData == NULL)
	  {
	     DPRINT1("Unable to allocate or find data for process 0x%x\n",
	             Request.Header.ClientId.UniqueProcess);
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
				     & ServerThread,
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
   return 0;
}

/**********************************************************************
 * NAME
 *	ServerSbApiPortThread/1
 *
 * DESCRIPTION
 * 	Handle connection requests from SM to the port
 * 	"\Windows\SbApiPort". We will accept only one
 * 	connection request (from the SM).
 */
DWORD STDCALL
ServerSbApiPortThread (PVOID PortHandle)
{
	HANDLE          hSbApiPortListen = * (PHANDLE) PortHandle;
	HANDLE          hConnectedPort = (HANDLE) 0;
	LPC_MAX_MESSAGE Request = {{0}};
	PVOID           Context = NULL;
	NTSTATUS        Status = STATUS_SUCCESS;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	Status = NtListenPort (hSbApiPortListen, & Request.Header);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CSR: %s: NtListenPort(SB) failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
	} else {
DPRINT("-- 1\n");
		Status = NtAcceptConnectPort (& hConnectedPort,
						hSbApiPortListen,
   						NULL,
   						TRUE,
   						NULL,
	   					NULL);
		if(!NT_SUCCESS(Status))
		{
			DPRINT1("CSR: %s: NtAcceptConnectPort() failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		} else {
DPRINT("-- 2\n");
			Status = NtCompleteConnectPort (hConnectedPort);
			if(!NT_SUCCESS(Status))
			{
				DPRINT1("CSR: %s: NtCompleteConnectPort() failed (Status=0x%08lx)\n",
					__FUNCTION__, Status);
			} else {
DPRINT("-- 3\n");
				PLPC_MESSAGE Reply = NULL;
				/*
				 * Tell the init thread the SM gave the
				 * green light for boostrapping.
				 */
				Status = NtSetEvent (hBootstrapOk, NULL);
				if(!NT_SUCCESS(Status))
				{
					DPRINT1("CSR: %s: NtSetEvent failed (Status=0x%08lx)\n",
						__FUNCTION__, Status);
				}
				/* Wait for messages from the SM */
DPRINT("-- 4\n");
				while (TRUE)
				{
					Status = NtReplyWaitReceivePort(hConnectedPort,
                                      					Context,
									Reply,
									& Request.Header);
					if(!NT_SUCCESS(Status))
					{
						DPRINT1("CSR: %s: NtReplyWaitReceivePort failed (Status=0x%08lx)\n",
							__FUNCTION__, Status);
						break;
					}
					switch (Request.Header.MessageType)//fix .h PORT_MESSAGE_TYPE(Request))
					{
						/* TODO */
					default:
						DPRINT1("CSR: %s received message (type=%d)\n",
							__FUNCTION__, Request.Header.MessageType);
					}
DPRINT("-- 5\n");
				}
			}
		}
	}
	DPRINT1("CSR: %s: terminating!\n", __FUNCTION__);
	if(hConnectedPort) NtClose (hConnectedPort);
	NtClose (hSbApiPortListen);
	NtTerminateThread (NtCurrentThread(), Status);
	return 0;
}

/* EOF */
