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

#include <srv.h>

#define NDEBUG

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern HANDLE hApiPort;

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

  DPRINT("CSR: %s called\n", __FUNCTION__);

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
  unsigned DefIndex;
  ULONG Type;

  DPRINT("CSR: Calling handler for type: %x.\n", Request->Type);
  Type = Request->Type & 0xFFFF; /* FIXME: USE MACRO */
  DPRINT("CSR: API Number: %x ServerID: %x\n",Type, Request->Type >> 16);

  /* FIXME: Extract DefIndex instead of looping */
  for (DefIndex = 0; DefIndex < ApiDefinitionsCount; DefIndex++)
    {
      if (ApiDefinitions[DefIndex].Type == Type)
        {
          if (Request->Header.u1.s1.DataLength < ApiDefinitions[DefIndex].MinRequestSize)
            {
              DPRINT1("Request type %d min request size %d actual %d\n",
                      Type, ApiDefinitions[DefIndex].MinRequestSize,
                      Request->Header.u1.s1.DataLength);
              Request->Status = STATUS_INVALID_PARAMETER;
            }
          else
            {
              Request->Status = (ApiDefinitions[DefIndex].Handler)(ProcessData, Request);
            }
          return;
        }
    }
  DPRINT1("CSR: Unknown request type 0x%x\n", Request->Type);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Status = STATUS_INVALID_SYSTEM_SERVICE;
}

BOOL
CallHardError(IN PCSRSS_PROCESS_DATA ProcessData,
              IN PHARDERROR_MSG HardErrorMessage);

static
VOID
NTAPI
CsrHandleHardError(IN PCSRSS_PROCESS_DATA ProcessData,
                   IN OUT PHARDERROR_MSG Message)
{
    DPRINT1("CSR: received hard error %lx\n", Message->Status);

    /* Call the hard error handler in win32csr */
    (VOID)CallHardError(ProcessData, Message);
}

NTSTATUS WINAPI
CsrpHandleConnectionRequest (PPORT_MESSAGE Request,
                             IN HANDLE hApiListenPort)
{
    NTSTATUS Status;
    HANDLE ServerPort = NULL, ServerThread = NULL;
    PCSRSS_PROCESS_DATA ProcessData = NULL;
    REMOTE_PORT_VIEW LpcRead;
    CLIENT_ID ClientId;
    LpcRead.Length = sizeof(LpcRead);
    ServerPort = NULL;

    DPRINT("CSR: %s: Handling: %p\n", __FUNCTION__, Request);

    Status = NtAcceptConnectPort(&ServerPort,
                                 NULL,
                                 Request,
                                 TRUE,
                                 0,
                                 & LpcRead);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR: NtAcceptConnectPort() failed\n");
        return Status;
    }

    ProcessData = CsrGetProcessData(Request->ClientId.UniqueProcess);
    if (ProcessData == NULL)
    {
        ProcessData = CsrCreateProcessData(Request->ClientId.UniqueProcess);
        if (ProcessData == NULL)
        {
            DPRINT1("Unable to allocate or find data for process 0x%x\n",
                    Request->ClientId.UniqueProcess);
            Status = STATUS_UNSUCCESSFUL;
            return Status;
        }
    }

    ProcessData->CsrSectionViewBase = LpcRead.ViewBase;
    ProcessData->CsrSectionViewSize = LpcRead.ViewSize;
    ProcessData->ServerCommunicationPort = ServerPort;

    Status = NtCompleteConnectPort(ServerPort);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR: NtCompleteConnectPort() failed\n");
        return Status;
    }

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 (PTHREAD_START_ROUTINE)ClientConnectionThread,
                                 ServerPort,
                                 & ServerThread,
                                 &ClientId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR: Unable to create server thread\n");
        return Status;
    }
    
    CsrAddStaticServerThread(ServerThread, &ClientId, 0);
    
    NtResumeThread(ServerThread, NULL);

    NtClose(ServerThread);

    Status = STATUS_SUCCESS;
    DPRINT("CSR: %s done\n", __FUNCTION__);
    return Status;
}

PCSR_THREAD
NTAPI
CsrConnectToUser(VOID)
{
    PTEB Teb = NtCurrentTeb();
    PCSR_THREAD CsrThread;
#if 0
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING TempName;
    HANDLE hUser32;
    STRING StartupName;

    /* Check if we didn't already find it */
    if (!CsrClientThreadSetup)
    {
        /* Get the DLL Handle for user32.dll */
        RtlInitAnsiString(&DllName, "user32");
        RtlAnsiStringToUnicodeString(&TempName, &DllName, TRUE);
        Status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &TempName,
                                 &hUser32);
        RtlFreeUnicodeString(&TempName);

        /* If we got teh handle, get the Client Thread Startup Entrypoint */
        if (NT_SUCCESS(Status))
        {
            RtlInitAnsiString(&StartupName,"ClientThreadSetup");
            Status = LdrGetProcedureAddress(hUser32,
                                            &StartupName,
                                            0,
                                            (PVOID)&CsrClientThreadSetup);
        }
    }

    /* Connect to user32 */
    CsrClientThreadSetup();
#endif
    /* Save pointer to this thread in TEB */
    CsrThread = CsrLocateThreadInProcess(NULL, &Teb->ClientId);
    if (CsrThread) Teb->CsrClientThread = CsrThread;

    /* Return it */
    return CsrThread;
}

VOID
WINAPI
ClientConnectionThread(HANDLE ServerPort)
{
    NTSTATUS Status;
    BYTE RawRequest[LPC_MAX_DATA_LENGTH];
    PCSR_API_MESSAGE Request = (PCSR_API_MESSAGE)RawRequest;
    PCSR_API_MESSAGE Reply;
    PCSRSS_PROCESS_DATA ProcessData;
    PCSR_THREAD ServerThread;

    DPRINT("CSR: %s called\n", __FUNCTION__);
    
    /* Connect to user32 */
    while (!CsrConnectToUser())
    {
        /* Keep trying until we get a response */
        NtCurrentTeb()->Win32ClientInfo[0] = 0;
        //NtDelayExecution(FALSE, &TimeOut);
    }

    /* Reply must be NULL at the first call to NtReplyWaitReceivePort */
    ServerThread = NtCurrentTeb()->CsrClientThread;
    Reply = NULL;

    /* Loop and reply/wait for a new message */
    for (;;)
    {
        /* Send the reply and wait for a new request */
        Status = NtReplyWaitReceivePort(hApiPort,
                                        0,
                                        &Reply->Header,
                                        &Request->Header);
        /* Client died, continue */
        if (Status == STATUS_INVALID_CID)
        {
            Reply = NULL;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtReplyWaitReceivePort failed: %lx\n", Status);
            break;
        }

        /* If the connection was closed, handle that */
        if (Request->Header.u2.s2.Type == LPC_PORT_CLOSED)
        {
            DPRINT("Port died, oh well\n");
            CsrFreeProcessData( Request->Header.ClientId.UniqueProcess );
            break;
        }

        if (Request->Header.u2.s2.Type == LPC_CONNECTION_REQUEST)
        {
            CsrpHandleConnectionRequest((PPORT_MESSAGE)Request, ServerPort);
            Reply = NULL;
            continue;
        }

        if (Request->Header.u2.s2.Type == LPC_CLIENT_DIED)
        {
            DPRINT("Client died, oh well\n");
            Reply = NULL;
            continue;
        }

        if ((Request->Header.u2.s2.Type != LPC_ERROR_EVENT) &&
            (Request->Header.u2.s2.Type != LPC_REQUEST))
        {
            DPRINT1("CSR: received message %d\n", Request->Header.u2.s2.Type);
            Reply = NULL;
            continue;
        }

        DPRINT("CSR: Got CSR API: %x [Message Origin: %x]\n",
                Request->Type,
                Request->Header.ClientId.UniqueThread);

        /* Get the Process Data */
        ProcessData = CsrGetProcessData(Request->Header.ClientId.UniqueProcess);
        if (ProcessData == NULL)
        {
            DPRINT1("Message %d: Unable to find data for process 0x%x\n",
                    Request->Header.u2.s2.Type,
                    Request->Header.ClientId.UniqueProcess);
            break;
        }
        if (ProcessData->Terminated)
        {
            DPRINT1("Message %d: process %d already terminated\n",
                    Request->Type, Request->Header.ClientId.UniqueProcess);
            continue;
        }

        /* Check if we got a hard error */
        if (Request->Header.u2.s2.Type == LPC_ERROR_EVENT)
        {
            /* Call the Handler */
            CsrHandleHardError(ProcessData, (PHARDERROR_MSG)Request);
        }
        else
        {
            PCSR_THREAD Thread;
            PCSRSS_PROCESS_DATA Process = NULL;
            
            //DPRINT1("locate thread %lx/%lx\n", Request->Header.ClientId.UniqueProcess, Request->Header.ClientId.UniqueThread);
            Thread = CsrLocateThreadByClientId(&Process, &Request->Header.ClientId);
            //DPRINT1("Thread found: %p %p\n", Thread, Process);
                                          
            /* Call the Handler */
            if (Thread) NtCurrentTeb()->CsrClientThread = Thread;
            CsrApiCallHandler(ProcessData, Request);
            if (Thread) NtCurrentTeb()->CsrClientThread = ServerThread;
        }

        /* Send back the reply */
        Reply = Request;
    }

    /* Close the port and exit the thread */
    // NtClose(ServerPort);

    DPRINT("CSR: %s done\n", __FUNCTION__);
    RtlExitUserThread(STATUS_SUCCESS);
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
DWORD WINAPI
ServerSbApiPortThread (HANDLE hSbApiPortListen)
{
    HANDLE          hConnectedPort = (HANDLE) 0;
    PORT_MESSAGE    Request;
    PVOID           Context = NULL;
    NTSTATUS        Status = STATUS_SUCCESS;
    PPORT_MESSAGE Reply = NULL;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    RtlZeroMemory(&Request, sizeof(PORT_MESSAGE));
    Status = NtListenPort (hSbApiPortListen, & Request);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR: %s: NtListenPort(SB) failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
    } else {
        DPRINT("-- 1\n");
        Status = NtAcceptConnectPort(&hConnectedPort,
                                     NULL,
                                     &Request,
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
                                                    &Request);
                    if(!NT_SUCCESS(Status))
                    {
                        DPRINT1("CSR: %s: NtReplyWaitReceivePort failed (Status=0x%08lx)\n",
                                __FUNCTION__, Status);
                        break;
                    }

                    switch (Request.u2.s2.Type) //fix .h PORT_MESSAGE_TYPE(Request))
                    {
                        /* TODO */
                        default:
                        DPRINT1("CSR: %s received message (type=%d)\n",
                                __FUNCTION__, Request.u2.s2.Type);
                    }
                    DPRINT("-- 5\n");
                }
            }
        }
    }

    DPRINT("CSR: %s: terminating!\n", __FUNCTION__);
    if(hConnectedPort) NtClose (hConnectedPort);
    NtClose (hSbApiPortListen);
    NtTerminateThread (NtCurrentThread(), Status);
    return 0;
}

/* EOF */
