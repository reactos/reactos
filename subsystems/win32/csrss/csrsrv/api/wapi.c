/* $Id$
 *
 * subsystems/win32/csrss/csrsrv/api/wapi.c
 *
 * CSRSS port message processing
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

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

PVOID CsrSrvSharedSectionHeap;
PVOID CsrSrvSharedSectionBase;
PVOID *CsrSrvSharedStaticServerData;
ULONG CsrSrvSharedSectionSize;
HANDLE CsrSrvSharedSection;

/*++
 * @name CsrSrvCreateSharedSection
 *
 * The CsrSrvCreateSharedSection creates the Shared Section that all CSR Server
 * DLLs and Clients can use to share data.
 *
 * @param ParameterValue
 *        Specially formatted string from our registry command-line which
 *        specifies various arguments for the shared section.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSrvCreateSharedSection(IN PCHAR ParameterValue)
{
    PCHAR SizeValue = ParameterValue;
    ULONG Size;
    NTSTATUS Status;
    LARGE_INTEGER SectionSize;
    ULONG ViewSize = 0;
    SYSTEM_BASIC_INFORMATION CsrNtSysInfo;
    PPEB Peb = NtCurrentPeb();
    
    /* ReactOS Hackssss */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &CsrNtSysInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);
    ASSERT(NT_SUCCESS(Status));
    
    /* Find the first comma, and null terminate */
    while (*SizeValue)
    {
        if (*SizeValue == ',')
        {
            *SizeValue++ = '\0';
            break;
        }
        else
        {
            SizeValue++;
        }
    }
    
    /* Make sure it's valid */
    if (!*SizeValue) return STATUS_INVALID_PARAMETER;
    
    /* Convert it to an integer */
    Status = RtlCharToInteger(SizeValue, 0, &Size);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Multiply by 1024 entries and round to page size */
    CsrSrvSharedSectionSize = ROUND_UP(Size * 1024, CsrNtSysInfo.PageSize);
    DPRINT1("Size: %lx\n", CsrSrvSharedSectionSize);
    
    /* Create the Secion */
    SectionSize.LowPart = CsrSrvSharedSectionSize;
    SectionSize.HighPart = 0;
    Status = NtCreateSection(&CsrSrvSharedSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_BASED | SEC_RESERVE,
                             NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Map the section */
    Status = NtMapViewOfSection(CsrSrvSharedSection,
                                NtCurrentProcess(),
                                &CsrSrvSharedSectionBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                MEM_TOP_DOWN,
                                PAGE_EXECUTE_READWRITE);
    if(!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(CsrSrvSharedSection);
        return(Status);
    }
    
    /* FIXME: Write the value to registry */
    
    /* The Heap is the same place as the Base */
    CsrSrvSharedSectionHeap = CsrSrvSharedSectionBase;
    
    /* Create the heap */
    if (!(RtlCreateHeap(HEAP_ZERO_MEMORY,
                        CsrSrvSharedSectionHeap,
                        CsrSrvSharedSectionSize,
                        PAGE_SIZE,
                        0,
                        0)))
    {
        /* Failure, unmap section and return */
        NtUnmapViewOfSection(NtCurrentProcess(),
                             CsrSrvSharedSectionBase);
        NtClose(CsrSrvSharedSection);
        return STATUS_NO_MEMORY;
    }
    
    /* Now allocate space from the heap for the Shared Data */
    CsrSrvSharedStaticServerData = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                                   0,
                                                   4 * // HAX CSR_SERVER_DLL_MAX *
                                                   sizeof(PVOID));
    
    /* Write the values to the PEB */
    Peb->ReadOnlySharedMemoryBase = CsrSrvSharedSectionBase;
    Peb->ReadOnlySharedMemoryHeap = CsrSrvSharedSectionHeap;
    Peb->ReadOnlyStaticServerData = CsrSrvSharedStaticServerData;
    
    /* Return */
    return STATUS_SUCCESS;
}

/*++
 * @name CsrSrvAttachSharedSection
 *
 * The CsrSrvAttachSharedSection maps the CSR Shared Section into a new
 * CSR Process' address space, and returns the pointers to the section
 * through the Connection Info structure.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process that is attempting a connection.
 *
 * @param ConnectInfo
 *        Pointer to the CSR Connection Info structure for the incoming
 *        connection.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSrvAttachSharedSection(IN PCSRSS_PROCESS_DATA CsrProcess OPTIONAL,
                          OUT PCSR_CONNECTION_INFO ConnectInfo)
{
    NTSTATUS Status;
    ULONG ViewSize = 0;
    
    /* Check if we have a process */
    if (CsrProcess)
    {
        /* Map the section into this process */
        DPRINT("CSR Process Handle: %p. CSR Process: %p\n", CsrProcess->Process, CsrProcess);
        Status = NtMapViewOfSection(CsrSrvSharedSection,
                                    CsrProcess->Process,
                                    &CsrSrvSharedSectionBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewUnmap,
                                    SEC_NO_CHANGE,
                                    PAGE_EXECUTE_READ);
        if (Status == STATUS_CONFLICTING_ADDRESSES)
        {
            /* I Think our csrss tries to connect to itself... */
            DPRINT1("Multiple mapping hack\n");
            Status = STATUS_SUCCESS;
        }
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    /* Write the values in the Connection Info structure */
    ConnectInfo->SharedSectionBase = CsrSrvSharedSectionBase;
    ConnectInfo->SharedSectionHeap = CsrSrvSharedSectionHeap;
    ConnectInfo->SharedSectionData = CsrSrvSharedStaticServerData;
    
    /* Return success */
    return STATUS_SUCCESS;
}

PBASE_STATIC_SERVER_DATA BaseStaticServerData;

VOID
WINAPI
BasepFakeStaticServerData(VOID)
{
    NTSTATUS Status;
    WCHAR Buffer[MAX_PATH];
    PWCHAR HeapBuffer;
    UNICODE_STRING SystemRootString;
    UNICODE_STRING UnexpandedSystemRootString = RTL_CONSTANT_STRING(L"%SystemRoot%");
    UNICODE_STRING BaseSrvCSDString;
    UNICODE_STRING BaseSrvWindowsDirectory;
    UNICODE_STRING BaseSrvWindowsSystemDirectory;
    UNICODE_STRING BnoString;
    RTL_QUERY_REGISTRY_TABLE BaseServerRegistryConfigurationTable[2] =
    {
        {
            NULL,
            RTL_QUERY_REGISTRY_DIRECT,
            L"CSDVersion",
            &BaseSrvCSDString
        },
        {0}
    };
    
    /* Get the Windows directory */
    RtlInitEmptyUnicodeString(&SystemRootString, Buffer, sizeof(Buffer));
    Status = RtlExpandEnvironmentStrings_U(NULL,
                                           &UnexpandedSystemRootString,
                                           &SystemRootString,
                                           NULL);
    DPRINT1("Status: %lx. Root: %wZ\n", Status, &SystemRootString);
    ASSERT(NT_SUCCESS(Status));
    
    /* Create the base directory */
    Buffer[SystemRootString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    Status = RtlCreateUnicodeString(&BaseSrvWindowsDirectory,
                                    SystemRootString.Buffer);
    ASSERT(NT_SUCCESS(Status));
    
    /* Create the system directory */
    wcscat(SystemRootString.Buffer, L"\\system32");
    Status = RtlCreateUnicodeString(&BaseSrvWindowsSystemDirectory,
                                    SystemRootString.Buffer);
    ASSERT(NT_SUCCESS(Status));
    
    /* FIXME: Check Session ID */
    wcscpy(Buffer, L"\\BaseNamedObjects");
    RtlInitUnicodeString(&BnoString, Buffer);
    
    /* Allocate the server data */
    BaseStaticServerData = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                           HEAP_ZERO_MEMORY,
                                           sizeof(BASE_STATIC_SERVER_DATA));
    ASSERT(BaseStaticServerData != NULL);
    
    /* Process timezone information */
    BaseStaticServerData->TermsrvClientTimeZoneId = TIME_ZONE_ID_INVALID;
    BaseStaticServerData->TermsrvClientTimeZoneChangeNum = 0;
    Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                                      &BaseStaticServerData->TimeOfDay,
                                      sizeof(BaseStaticServerData->TimeOfDay),
                                      NULL);
    ASSERT(NT_SUCCESS(Status));
    
    /* Make a shared heap copy of the Windows directory */
    BaseStaticServerData->WindowsDirectory = BaseSrvWindowsDirectory;
    HeapBuffer = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                 0,
                                 BaseSrvWindowsDirectory.MaximumLength);
    ASSERT(HeapBuffer);
    RtlCopyMemory(HeapBuffer,
                  BaseStaticServerData->WindowsDirectory.Buffer,
                  BaseSrvWindowsDirectory.MaximumLength);
    BaseStaticServerData->WindowsDirectory.Buffer = HeapBuffer;
    
    /* Make a shared heap copy of the System directory */
    BaseStaticServerData->WindowsSystemDirectory = BaseSrvWindowsSystemDirectory;
    HeapBuffer = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                 0,
                                 BaseSrvWindowsSystemDirectory.MaximumLength);
    ASSERT(HeapBuffer);
    RtlCopyMemory(HeapBuffer,
                  BaseStaticServerData->WindowsSystemDirectory.Buffer,
                  BaseSrvWindowsSystemDirectory.MaximumLength);
    BaseStaticServerData->WindowsSystemDirectory.Buffer = HeapBuffer;
    
    /* This string is not used */
    RtlInitEmptyUnicodeString(&BaseStaticServerData->WindowsSys32x86Directory,
                              NULL,
                              0);
    
    /* Make a shared heap copy of the BNO directory */
    BaseStaticServerData->NamedObjectDirectory = BnoString;
    BaseStaticServerData->NamedObjectDirectory.MaximumLength = BnoString.Length +
                                                               sizeof(UNICODE_NULL);
    HeapBuffer = RtlAllocateHeap(CsrSrvSharedSectionHeap,
                                 0,
                                 BaseStaticServerData->NamedObjectDirectory.MaximumLength);
    ASSERT(HeapBuffer);
    RtlCopyMemory(HeapBuffer,
                  BaseStaticServerData->NamedObjectDirectory.Buffer,
                  BaseStaticServerData->NamedObjectDirectory.MaximumLength);
    BaseStaticServerData->NamedObjectDirectory.Buffer = HeapBuffer;
    
    /*
     * Confirmed that in Windows, CSDNumber and RCNumber are actually Length
     * and MaximumLength of the CSD String, since the same UNICODE_STRING is
     * being queried twice, the first time as a ULONG!
     *
     * Somehow, in Windows this doesn't cause a buffer overflow, but it might
     * in ReactOS, so this code is disabled until someone figures out WTF.
     */
    BaseStaticServerData->CSDNumber = 0;
    BaseStaticServerData->RCNumber = 0;
    
    /* Initialize the CSD string and query its value from the registry */
    RtlInitEmptyUnicodeString(&BaseSrvCSDString, Buffer, sizeof(Buffer));
    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"",
                                    BaseServerRegistryConfigurationTable,
                                    NULL,
                                    NULL);
    if (NT_SUCCESS(Status))
    {
        /* Copy into the shared buffer */
        wcsncpy(BaseStaticServerData->CSDVersion,
                BaseSrvCSDString.Buffer,
                BaseSrvCSDString.Length / sizeof(WCHAR));
    }
    else
    {
        /* NULL-terminate to indicate nothing is there */
        BaseStaticServerData->CSDVersion[0] = UNICODE_NULL;
    }
    
    /* Cache the system information */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BaseStaticServerData->SysInfo,
                                      sizeof(BaseStaticServerData->SysInfo),
                                      NULL);
    ASSERT(NT_SUCCESS(Status));
    
    /* FIXME: Should query the registry for these */
    BaseStaticServerData->DefaultSeparateVDM = FALSE;
    BaseStaticServerData->IsWowTaskReady = FALSE;
    BaseStaticServerData->LUIDDeviceMapsEnabled = FALSE;

    /* FIXME: Symlinks */
    
    /* Finally, set the pointer */
    CsrSrvSharedStaticServerData[CSR_CONSOLE] = BaseStaticServerData;
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
    BOOLEAN AllowConnection = FALSE;
    PCSR_CONNECTION_INFO ConnectInfo;
    LpcRead.Length = sizeof(LpcRead);
    ServerPort = NULL;

    DPRINT("CSR: %s: Handling: %p\n", __FUNCTION__, Request);

    ConnectInfo = (PCSR_CONNECTION_INFO)(Request + 1);
    
    /* Save the process ID */
    RtlZeroMemory(ConnectInfo, sizeof(CSR_CONNECTION_INFO));
    ConnectInfo->ProcessId = NtCurrentTeb()->ClientId.UniqueProcess;
    
    ProcessData = CsrGetProcessData(Request->ClientId.UniqueProcess);
    if (ProcessData == NULL)
    {
        ProcessData = CsrCreateProcessData(Request->ClientId.UniqueProcess);
        if (ProcessData == NULL)
        {
            DPRINT1("Unable to allocate or find data for process 0x%x\n",
                    Request->ClientId.UniqueProcess);
        }
    }
    
    if (ProcessData->Process == NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
        DPRINT1("WARNING: CSR PROCESS WITH NO CSR PROCESS HANDLE???\n");
        ClientId.UniqueThread = 0;
        Status = NtOpenProcess(&ProcessData->Process,
                               PROCESS_ALL_ACCESS,
                               &ObjectAttributes,
                               &Request->ClientId);
        DPRINT1("Status: %lx. Handle: %lx\n", Status, ProcessData->Process);
    }
    
    if (ProcessData)
    {
        /* Attach the Shared Section */
        Status = CsrSrvAttachSharedSection(ProcessData, ConnectInfo);
        if (NT_SUCCESS(Status))
        {
            DPRINT("Connection ok\n");
            AllowConnection = TRUE;
        }
        else
        {
            DPRINT1("Shared section map failed: %lx\n", Status);
        }
    }
    
    Status = NtAcceptConnectPort(&ServerPort,
                                 NULL,
                                 Request,
                                 AllowConnection,
                                 0,
                                 & LpcRead);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR: NtAcceptConnectPort() failed\n");
        return Status;
    }

    ProcessData->CsrSectionViewBase = LpcRead.ViewBase;
    ProcessData->CsrSectionViewSize = LpcRead.ViewSize;
    ProcessData->ServerCommunicationPort = ServerPort;

    if (AllowConnection) Status = NtCompleteConnectPort(ServerPort);
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
