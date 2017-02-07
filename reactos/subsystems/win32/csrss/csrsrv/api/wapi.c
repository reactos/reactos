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

static unsigned ApiDefinitionsCount = 0;
static PCSRSS_API_DEFINITION ApiDefinitions = NULL;
UNICODE_STRING CsrApiPortName;
volatile LONG CsrpStaticThreadCount;
volatile LONG CsrpDynamicThreadTotal;
ULONG CsrMaxApiRequestThreads;

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

  New = RtlAllocateHeap(CsrHeap, 0,
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
      RtlFreeHeap(CsrHeap, 0, ApiDefinitions);
    }
  RtlCopyMemory(New + ApiDefinitionsCount, NewDefinitions,
                NewCount * sizeof(CSRSS_API_DEFINITION));
  ApiDefinitions = New;
  ApiDefinitionsCount += NewCount;

  return STATUS_SUCCESS;
}

VOID
FASTCALL
CsrApiCallHandler(PCSR_PROCESS ProcessData,
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
  Request->Status = STATUS_INVALID_SYSTEM_SERVICE;
}

VOID
CallHardError(IN PCSR_THREAD ThreadData,
              IN PHARDERROR_MSG HardErrorMessage);

static
VOID
NTAPI
CsrHandleHardError(IN PCSR_THREAD ThreadData,
                   IN OUT PHARDERROR_MSG Message)
{
    DPRINT1("CSR: received hard error %lx\n", Message->Status);

    /* Call the hard error handler in win32csr */
    CallHardError(ThreadData, Message);
}

/*++
 * @name CsrCallServerFromServer
 * @implemented NT4
 *
 * The CsrCallServerFromServer routine calls a CSR API from within a server.
 * It avoids using LPC messages since the request isn't coming from a client.
 *
 * @param ReceiveMsg
 *        Pointer to the CSR API Message to send to the server.
 *
 * @param ReplyMsg
 *        Pointer to the CSR API Message to receive from the server.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_ILLEGAL_FUNCTION
 *         if the opcode is invalid, or STATUS_ACCESS_VIOLATION if there
 *         was a problem executing the API.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCallServerFromServer(PCSR_API_MESSAGE ReceiveMsg,
                        PCSR_API_MESSAGE ReplyMsg)
{
#if 0 // real code
    ULONG ServerId;
    PCSR_SERVER_DLL ServerDll;
    ULONG ApiId;
    ULONG Reply;
    NTSTATUS Status;

    /* Get the Server ID */
    ServerId = CSR_SERVER_ID_FROM_OPCODE(ReceiveMsg->Opcode);

    /* Make sure that the ID is within limits, and the Server DLL loaded */
    if ((ServerId >= CSR_SERVER_DLL_MAX) ||
        (!(ServerDll = CsrLoadedServerDll[ServerId])))
    {
        /* We are beyond the Maximum Server ID */
        DPRINT1("CSRSS: %lx is invalid ServerDllIndex (%08x)\n", ServerId, ServerDll);
        ReplyMsg->Status = (ULONG)STATUS_ILLEGAL_FUNCTION;
        return STATUS_ILLEGAL_FUNCTION;
    }
    else
    {
        /* Get the API ID */
        ApiId = CSR_API_ID_FROM_OPCODE(ReceiveMsg->Opcode);

        /* Normalize it with our Base ID */
        ApiId -= ServerDll->ApiBase;

        /* Make sure that the ID is within limits, and the entry exists */
        if ((ApiId >= ServerDll->HighestApiSupported) ||
            ((ServerDll->ValidTable) && !(ServerDll->ValidTable[ApiId])))
        {
            /* We are beyond the Maximum API ID, or it doesn't exist */
            DPRINT1("CSRSS: %lx (%s) is invalid ApiTableIndex for %Z or is an "
                    "invalid API to call from the server.\n",
                    ServerDll->ValidTable[ApiId],
                    ((ServerDll->NameTable) && (ServerDll->NameTable[ApiId])) ?
                    ServerDll->NameTable[ApiId] : "*** UNKNOWN ***", &ServerDll->Name);
            DbgBreakPoint();
            ReplyMsg->Status = (ULONG)STATUS_ILLEGAL_FUNCTION;
            return STATUS_ILLEGAL_FUNCTION;
        }
    }

    if (CsrDebug & 2)
    {
        DPRINT1("CSRSS: %s Api Request received from server process\n",
                ServerDll->NameTable[ApiId]);
    }
        
    /* Validation complete, start SEH */
    _SEH2_TRY
    {
        /* Call the API and get the result */
        Status = (ServerDll->DispatchTable[ApiId])(ReceiveMsg, &Reply);

        /* Return the result, no matter what it is */
        ReplyMsg->Status = Status;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* If we got an exception, return access violation */
        ReplyMsg->Status = STATUS_ACCESS_VIOLATION;
    }
    _SEH2_END;

    /* Return success */
    return STATUS_SUCCESS;
#else // Hacky reactos code
    PCSR_PROCESS ProcessData;

    /* Get the Process Data */
    CsrLockProcessByClientId(&ReceiveMsg->Header.ClientId.UniqueProcess, &ProcessData);
    if (!ProcessData)
    {
        DPRINT1("Message: Unable to find data for process 0x%x\n",
                ReceiveMsg->Header.ClientId.UniqueProcess);
        return STATUS_NOT_SUPPORTED;
    }

    /* Validation complete, start SEH */
    _SEH2_TRY
    {
        /* Call the API and get the result */
        CsrApiCallHandler(ProcessData, ReplyMsg);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* If we got an exception, return access violation */
        ReplyMsg->Status = STATUS_ACCESS_VIOLATION;
    }
    _SEH2_END;

    /* Release the process reference */
    CsrUnlockProcess(ProcessData);

    /* Return success */
    return STATUS_SUCCESS;
#endif
}

/*++
 * @name CsrApiPortInitialize
 *
 * The CsrApiPortInitialize routine initializes the LPC Port used for
 * communications with the Client/Server Runtime (CSR) and initializes the
 * static thread that will handle connection requests and APIs.
 *
 * @param None
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrApiPortInitialize(VOID)
{
    ULONG Size;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE hRequestEvent, hThread;
    CLIENT_ID ClientId;
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_THREAD ServerThread;

    /* Calculate how much space we'll need for the Port Name */
    Size = CsrDirectoryName.Length + sizeof(CSR_PORT_NAME) + sizeof(WCHAR);

    /* Create the buffer for it */
    CsrApiPortName.Buffer = RtlAllocateHeap(CsrHeap, 0, Size);
    if (!CsrApiPortName.Buffer) return STATUS_NO_MEMORY;

    /* Setup the rest of the empty string */
    CsrApiPortName.Length = 0;
    CsrApiPortName.MaximumLength = (USHORT)Size;
    RtlAppendUnicodeStringToString(&CsrApiPortName, &CsrDirectoryName);
    RtlAppendUnicodeToString(&CsrApiPortName, UNICODE_PATH_SEP);
    RtlAppendUnicodeToString(&CsrApiPortName, CSR_PORT_NAME);
    if (CsrDebug & 1)
    {
        DPRINT1("CSRSS: Creating %wZ port and associated threads\n", &CsrApiPortName);
        DPRINT1("CSRSS: sizeof( CONNECTINFO ) == %ld  sizeof( API_MSG ) == %ld\n",
                sizeof(CSR_CONNECTION_INFO), sizeof(CSR_API_MESSAGE));
    }

    /* FIXME: Create a Security Descriptor */

    /* Initialize the Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CsrApiPortName,
                               0,
                               NULL,
                               NULL /* FIXME*/);

    /* Create the Port Object */
    Status = NtCreatePort(&CsrApiPort,
                          &ObjectAttributes,
                          LPC_MAX_DATA_LENGTH, // hack
                          LPC_MAX_MESSAGE_LENGTH, // hack
                          16 * PAGE_SIZE);
    if (NT_SUCCESS(Status))
    {
        /* Create the event the Port Thread will use */
        Status = NtCreateEvent(&hRequestEvent,
                               EVENT_ALL_ACCESS,
                               NULL,
                               SynchronizationEvent,
                               FALSE);
        if (NT_SUCCESS(Status))
        {
            /* Create the Request Thread */
            Status = RtlCreateUserThread(NtCurrentProcess(),
                                         NULL,
                                         TRUE,
                                         0,
                                         0,
                                         0,
                                         (PVOID)ClientConnectionThread,//CsrApiRequestThread,
                                         (PVOID)hRequestEvent,
                                         &hThread,
                                         &ClientId);
            if (NT_SUCCESS(Status))
            {
                /* Add this as a static thread to CSRSRV */
                CsrAddStaticServerThread(hThread, &ClientId, CsrThreadIsServerThread);

                /* Get the Thread List Pointers */
                ListHead = &CsrRootProcess->ThreadList;
                NextEntry = ListHead->Flink;

                /* Start looping the list */
                while (NextEntry != ListHead)
                {
                    /* Get the Thread */
                    ServerThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

                    /* Start it up */
                    Status = NtResumeThread(ServerThread->ThreadHandle, NULL);

                    /* Is this a Server Thread? */
                    if (ServerThread->Flags & CsrThreadIsServerThread)
                    {
                        /* If so, then wait for it to initialize */
                        Status = NtWaitForSingleObject(hRequestEvent, FALSE, NULL);
                        ASSERT(NT_SUCCESS(Status));
                    }

                    /* Next thread */
                    NextEntry = NextEntry->Flink;
                }

                /* We don't need this anymore */
                NtClose(hRequestEvent);
            }
        }
    }

    /* Return */
    return Status;
}

PBASE_STATIC_SERVER_DATA BaseStaticServerData;

NTSTATUS
NTAPI
CreateBaseAcls(OUT PACL* Dacl,
               OUT PACL* RestrictedDacl)
{
    PSID SystemSid, WorldSid, RestrictedSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    NTSTATUS Status;
    UCHAR KeyValueBuffer[0x40];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
    UNICODE_STRING KeyName;
    ULONG ProtectionMode = 0;
    ULONG AclLength, ResultLength;
    HANDLE hKey;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Open the Session Manager Key */
    RtlInitUnicodeString(&KeyName, SM_REG_KEY);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Read the key value */
        RtlInitUnicodeString(&KeyName, L"ProtectionMode");
        Status = NtQueryValueKey(hKey,
                                 &KeyName,
                                 KeyValuePartialInformation,
                                 KeyValueBuffer,
                                 sizeof(KeyValueBuffer),
                                 &ResultLength);

        /* Make sure it's what we expect it to be */
        KeyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
        if ((NT_SUCCESS(Status)) && (KeyValuePartialInfo->Type == REG_DWORD) &&
            (*(PULONG)KeyValuePartialInfo->Data))
        {
            /* Save the Protection Mode */
            ProtectionMode = *(PULONG)KeyValuePartialInfo->Data;
        }

        /* Close the handle */
        NtClose(hKey);
    }

    /* Allocate the System SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1, SECURITY_LOCAL_SYSTEM_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &SystemSid);
    ASSERT(NT_SUCCESS(Status));

    /* Allocate the World SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                         1, SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    ASSERT(NT_SUCCESS(Status));

    /* Allocate the restricted SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1, SECURITY_RESTRICTED_CODE_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    
    /* Allocate one ACL with 3 ACEs each for one SID */
    AclLength = sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                RtlLengthSid(SystemSid) +
                RtlLengthSid(RestrictedSid) +
                RtlLengthSid(WorldSid);
    *Dacl = RtlAllocateHeap(CsrHeap, 0, AclLength);
    ASSERT(*Dacl != NULL);

    /* Set the correct header fields */
    Status = RtlCreateAcl(*Dacl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));

    /* Give the appropriate rights to each SID */
    /* FIXME: Should check SessionId/ProtectionMode */
    Status = RtlAddAccessAllowedAce(*Dacl, ACL_REVISION2, DIRECTORY_QUERY | DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY | READ_CONTROL, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(*Dacl, ACL_REVISION2, DIRECTORY_ALL_ACCESS, SystemSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(*Dacl, ACL_REVISION2, DIRECTORY_TRAVERSE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));

    /* Now allocate the restricted DACL */
    *RestrictedDacl = RtlAllocateHeap(CsrHeap, 0, AclLength);
    ASSERT(*RestrictedDacl != NULL);

    /* Initialize it */
    Status = RtlCreateAcl(*RestrictedDacl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));

    /* And add the same ACEs as before */
    /* FIXME: Not really fully correct */
    Status = RtlAddAccessAllowedAce(*RestrictedDacl, ACL_REVISION2, DIRECTORY_QUERY | DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY | READ_CONTROL, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(*RestrictedDacl, ACL_REVISION2, DIRECTORY_ALL_ACCESS, SystemSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(*RestrictedDacl, ACL_REVISION2, DIRECTORY_TRAVERSE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    
    /* The SIDs are captured, can free them now */
    RtlFreeHeap(CsrHeap, 0, SystemSid);
    RtlFreeHeap(CsrHeap, 0, WorldSid);
    RtlFreeHeap(CsrHeap, 0, RestrictedSid);
    return Status;
}

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
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG SessionId;
    HANDLE BaseSrvNamedObjectDirectory;
    HANDLE BaseSrvRestrictedObjectDirectory;
    PACL BnoDacl, BnoRestrictedDacl;
    PSECURITY_DESCRIPTOR BnoSd;
    HANDLE SymHandle;
    UNICODE_STRING DirectoryName, SymlinkName;
    ULONG LuidEnabled;
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
    
    /* Get the session ID */
    SessionId = NtCurrentPeb()->SessionId;

    /* Get the Windows directory */
    RtlInitEmptyUnicodeString(&SystemRootString, Buffer, sizeof(Buffer));
    Status = RtlExpandEnvironmentStrings_U(NULL,
                                           &UnexpandedSystemRootString,
                                           &SystemRootString,
                                           NULL);
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

    /* Allocate a security descriptor and create it */
    BnoSd = RtlAllocateHeap(CsrHeap, 0, 1024);
    ASSERT(BnoSd);
    Status = RtlCreateSecurityDescriptor(BnoSd, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    
    /* Create the BNO and \Restricted DACLs */
    Status = CreateBaseAcls(&BnoDacl, &BnoRestrictedDacl);
    ASSERT(NT_SUCCESS(Status));

    /* Set the BNO DACL as active for now */
    Status = RtlSetDaclSecurityDescriptor(BnoSd, TRUE, BnoDacl, FALSE);
    ASSERT(NT_SUCCESS(Status));

    /* Create the BNO directory */
    RtlInitUnicodeString(&BnoString, L"\\BaseNamedObjects");
    InitializeObjectAttributes(&ObjectAttributes,
                               &BnoString,
                               OBJ_OPENIF | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               BnoSd);
    Status = NtCreateDirectoryObject(&BaseSrvNamedObjectDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    ASSERT(NT_SUCCESS(Status));

    /* Check if we are session 0 */
    if (!SessionId)
    {
        /* Mark this as a session 0 directory */
        Status = NtSetInformationObject(BaseSrvNamedObjectDirectory,
                                        ObjectSessionInformation,
                                        NULL,
                                        0);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Check if LUID device maps are enabled */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessLUIDDeviceMapsEnabled,
                                       &LuidEnabled,
                                       sizeof(LuidEnabled),
                                       NULL);
    ASSERT(NT_SUCCESS(Status));
    BaseStaticServerData->LUIDDeviceMapsEnabled = LuidEnabled;
    if (!BaseStaticServerData->LUIDDeviceMapsEnabled)
    {
        /* Make Global point back to BNO */
        RtlInitUnicodeString(&DirectoryName, L"Global");
        RtlInitUnicodeString(&SymlinkName, L"\\BaseNamedObjects");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryName,
                                   OBJ_OPENIF | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   BaseSrvNamedObjectDirectory,
                                   BnoSd);
        Status = NtCreateSymbolicLinkObject(&SymHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &SymlinkName);
        if ((NT_SUCCESS(Status)) && !(SessionId)) NtClose(SymHandle);

        /* Make local point back to \Sessions\x\BNO */
        RtlInitUnicodeString(&DirectoryName, L"Local");
        ASSERT(SessionId == 0);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryName,
                                   OBJ_OPENIF | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   BaseSrvNamedObjectDirectory,
                                   BnoSd);
        Status = NtCreateSymbolicLinkObject(&SymHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &SymlinkName);
        if ((NT_SUCCESS(Status)) && !(SessionId)) NtClose(SymHandle);

        /* Make Session point back to BNOLINKS */
        RtlInitUnicodeString(&DirectoryName, L"Session");
        RtlInitUnicodeString(&SymlinkName, L"\\Sessions\\BNOLINKS");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryName,
                                   OBJ_OPENIF | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   BaseSrvNamedObjectDirectory,
                                   BnoSd);
        Status = NtCreateSymbolicLinkObject(&SymHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &SymlinkName);
        if ((NT_SUCCESS(Status)) && !(SessionId)) NtClose(SymHandle);

        /* Create the BNO\Restricted directory and set the restricted DACL */
        RtlInitUnicodeString(&DirectoryName, L"Restricted");
        Status = RtlSetDaclSecurityDescriptor(BnoSd, TRUE, BnoRestrictedDacl, FALSE);
        ASSERT(NT_SUCCESS(Status));
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryName,
                                   OBJ_OPENIF | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   BaseSrvNamedObjectDirectory,
                                   BnoSd);
        Status = NtCreateDirectoryObject(&BaseSrvRestrictedObjectDirectory,
                                         DIRECTORY_ALL_ACCESS,
                                         &ObjectAttributes);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Finally, set the pointer */
    CsrSrvSharedStaticServerData[CSR_CONSOLE] = BaseStaticServerData;
}

NTSTATUS WINAPI
CsrpHandleConnectionRequest (PPORT_MESSAGE Request)
{
    NTSTATUS Status;
    HANDLE ServerPort = NULL;//, ServerThread = NULL;
    PCSR_PROCESS ProcessData = NULL;
    REMOTE_PORT_VIEW RemotePortView;
//    CLIENT_ID ClientId;
    BOOLEAN AllowConnection = FALSE;
    PCSR_CONNECTION_INFO ConnectInfo;
    ServerPort = NULL;

    DPRINT("CSR: %s: Handling: %p\n", __FUNCTION__, Request);

    ConnectInfo = (PCSR_CONNECTION_INFO)(Request + 1);

    /* Save the process ID */
    RtlZeroMemory(ConnectInfo, sizeof(CSR_CONNECTION_INFO));

    CsrLockProcessByClientId(Request->ClientId.UniqueProcess, &ProcessData);
    if (!ProcessData)
    {
        DPRINT1("CSRSRV: Unknown process: %lx. Will be rejecting connection\n",
                Request->ClientId.UniqueProcess);
    }
    
    if ((ProcessData) && (ProcessData != CsrRootProcess))
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
    else if (ProcessData == CsrRootProcess)
    {
        AllowConnection = TRUE;
    }

    /* Release the process */
    if (ProcessData) CsrUnlockProcess(ProcessData);

    /* Setup the Port View Structure */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);
    RemotePortView.ViewSize = 0;
    RemotePortView.ViewBase = NULL;

    /* Save the Process ID */
    ConnectInfo->ProcessId = NtCurrentTeb()->ClientId.UniqueProcess;

    Status = NtAcceptConnectPort(&ServerPort,
                                 AllowConnection ? UlongToPtr(ProcessData->SequenceNumber) : 0,
                                 Request,
                                 AllowConnection,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
         DPRINT1("CSRSS: NtAcceptConnectPort - failed.  Status == %X\n", Status);
    }
    else if (AllowConnection)
    {
        if (CsrDebug & 2)
        {
            DPRINT1("CSRSS: ClientId: %lx.%lx has ClientView: Base=%p, Size=%lx\n",
                    Request->ClientId.UniqueProcess,
                    Request->ClientId.UniqueThread,
                    RemotePortView.ViewBase,
                    RemotePortView.ViewSize);
        }

        /* Set some Port Data in the Process */
        ProcessData->ClientPort = ServerPort;
        ProcessData->ClientViewBase = (ULONG_PTR)RemotePortView.ViewBase;
        ProcessData->ClientViewBounds = (ULONG_PTR)((ULONG_PTR)RemotePortView.ViewBase +
                                                    (ULONG_PTR)RemotePortView.ViewSize);

        /* Complete the connection */
        Status = NtCompleteConnectPort(ServerPort);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CSRSS: NtCompleteConnectPort - failed.  Status == %X\n", Status);
        }
    }
    else
    {
        DPRINT1("CSRSS: Rejecting Connection Request from ClientId: %lx.%lx\n",
                Request->ClientId.UniqueProcess,
                Request->ClientId.UniqueThread);
    }

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

/*++
 * @name CsrpCheckRequestThreads
 *
 * The CsrpCheckRequestThreads routine checks if there are no more threads
 * to handle CSR API Requests, and creates a new thread if possible, to
 * avoid starvation.
 *
 * @param None.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         if a new thread couldn't be created.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrpCheckRequestThreads(VOID)
{
    HANDLE hThread;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    /* Decrease the count, and see if we're out */
    if (!(_InterlockedDecrement(&CsrpStaticThreadCount)))
    {
        /* Check if we've still got space for a Dynamic Thread */
        if (CsrpDynamicThreadTotal < CsrMaxApiRequestThreads)
        {
            /* Create a new dynamic thread */
            Status = RtlCreateUserThread(NtCurrentProcess(),
                                         NULL,
                                         TRUE,
                                         0,
                                         0,
                                         0,
                                         (PVOID)ClientConnectionThread,//CsrApiRequestThread,
                                         NULL,
                                         &hThread,
                                         &ClientId);
            /* Check success */
            if (NT_SUCCESS(Status))
            {
                /* Increase the thread counts */
                _InterlockedIncrement(&CsrpStaticThreadCount);
                _InterlockedIncrement(&CsrpDynamicThreadTotal);

                /* Add a new server thread */
                if (CsrAddStaticServerThread(hThread,
                                             &ClientId,
                                             CsrThreadIsServerThread))
                {
                    /* Activate it */
                    NtResumeThread(hThread, NULL);
                }
                else
                {
                    /* Failed to create a new static thread */
                    _InterlockedDecrement(&CsrpStaticThreadCount);
                    _InterlockedDecrement(&CsrpDynamicThreadTotal);

                    /* Terminate it */
                    DPRINT1("Failing\n");
                    NtTerminateThread(hThread, 0);
                    NtClose(hThread);

                    /* Return */
                    return STATUS_UNSUCCESSFUL;
                }
            }
        }
    }

    /* Success */
    return STATUS_SUCCESS;
}

VOID
WINAPI
ClientConnectionThread(IN PVOID Parameter)
{
    PTEB Teb = NtCurrentTeb();
    LARGE_INTEGER TimeOut;
    NTSTATUS Status;
    BYTE RawRequest[LPC_MAX_DATA_LENGTH];
    PCSR_API_MESSAGE Request = (PCSR_API_MESSAGE)RawRequest;
    PCSR_API_MESSAGE Reply;
    PCSR_PROCESS CsrProcess;
    PCSR_THREAD ServerThread, CsrThread;
    ULONG MessageType;
    HANDLE ReplyPort;
    PDBGKM_MSG DebugMessage;
    PHARDERROR_MSG HardErrorMsg;
    PCLIENT_DIED_MSG ClientDiedMsg;
    DPRINT("CSR: %s called\n", __FUNCTION__);

    /* Setup LPC loop port and message */
    Reply = NULL;
    ReplyPort = CsrApiPort;

    /* Connect to user32 */
    while (!CsrConnectToUser())
    {
        /* Set up the timeout for the connect (30 seconds) */
        TimeOut.QuadPart = -30 * 1000 * 1000 * 10;

        /* Keep trying until we get a response */
        Teb->Win32ClientInfo[0] = 0;
        NtDelayExecution(FALSE, &TimeOut);
    }

    /* Get our thread */
    ServerThread = Teb->CsrClientThread;

    /* If we got an event... */
    if (Parameter)
    {
        /* Set it, to let stuff waiting on us load */
        Status = NtSetEvent((HANDLE)Parameter, NULL);
        ASSERT(NT_SUCCESS(Status));

        /* Increase the Thread Counts */
        _InterlockedIncrement(&CsrpStaticThreadCount);
        _InterlockedIncrement(&CsrpDynamicThreadTotal);
    }

    /* Now start the loop */
    while (TRUE)
    {
        /* Make sure the real CID is set */
        Teb->RealClientId = Teb->ClientId;

        /* Debug check */
        if (Teb->CountOfOwnedCriticalSections)
        {
            DPRINT1("CSRSRV: FATAL ERROR. CsrThread is Idle while holding %lu critical sections\n",
                    Teb->CountOfOwnedCriticalSections);
            DPRINT1("CSRSRV: Last Receive Message %lx ReplyMessage %lx\n",
                    Request, Reply);
            DbgBreakPoint();
        }

        /* Send the reply and wait for a new request */
        DPRINT("Replying to: %lx (%lx)\n", ReplyPort, CsrApiPort);
        Status = NtReplyWaitReceivePort(ReplyPort,
                                        0,
                                        &Reply->Header,
                                        &Request->Header);
        /* Check if we didn't get success */
        if (Status != STATUS_SUCCESS)
        {
            /* Was it a failure or another success code? */
            if (!NT_SUCCESS(Status))
            {
                /* Check for specific status cases */
                if ((Status != STATUS_INVALID_CID) &&
                    (Status != STATUS_UNSUCCESSFUL) &&
                    ((Status == STATUS_INVALID_HANDLE) || (ReplyPort == CsrApiPort)))
                {
                    /* Notify the debugger */
                    DPRINT1("CSRSS: ReceivePort failed - Status == %X\n", Status);
                    DPRINT1("CSRSS: ReplyPortHandle %lx CsrApiPort %lx\n", ReplyPort, CsrApiPort);
                }

                /* We failed big time, so start out fresh */
                Reply = NULL;
                ReplyPort = CsrApiPort;
                DPRINT1("failed: %lx\n", Status);
                continue;
            }
            else
            {
                /* A bizare "success" code, just try again */
                DPRINT1("NtReplyWaitReceivePort returned \"success\" status 0x%x\n", Status);
                continue;
            }
        }
        
        /* Use whatever Client ID we got */
        Teb->RealClientId = Request->Header.ClientId;

        /* Get the Message Type */
        MessageType = Request->Header.u2.s2.Type;
        
        /* Handle connection requests */
        if (MessageType == LPC_CONNECTION_REQUEST)
        {
            /* Handle the Connection Request */
            DPRINT("Accepting new connection\n");
            CsrpHandleConnectionRequest((PPORT_MESSAGE)Request);
            Reply = NULL;
            ReplyPort = CsrApiPort;
            continue;
        }
        
        /* It's some other kind of request. Get the lock for the lookup */
        CsrAcquireProcessLock();

        /* Now do the lookup to get the CSR_THREAD */
        CsrThread = CsrLocateThreadByClientId(&CsrProcess,
                                              &Request->Header.ClientId);

        /* Did we find a thread? */
        if (!CsrThread)
        {
            /* This wasn't a CSR Thread, release lock */
            CsrReleaseProcessLock();
            
            /* If this was an exception, handle it */
            if (MessageType == LPC_EXCEPTION)
            {
                DPRINT1("Exception from unknown thread, just continue\n");
                Reply = Request;
                ReplyPort = CsrApiPort;
                Reply->Status = DBG_CONTINUE;
            }
            else if (MessageType == LPC_PORT_CLOSED ||
                     MessageType == LPC_CLIENT_DIED)
            {
                /* The Client or Port are gone, loop again */
                DPRINT1("Death from unknown thread, just continue\n");
                Reply = NULL;
                ReplyPort = CsrApiPort;
            }
            else if (MessageType == LPC_ERROR_EVENT)
            {
                /* If it's a hard error, handle this too */
                DPRINT1("Hard error from unknown thread, call handlers\n");
HandleHardError:
                HardErrorMsg = (PHARDERROR_MSG)Request;

                /* Default it to unhandled */
                HardErrorMsg->Response = ResponseNotHandled;

                /* Check if there are free api threads */
                CsrpCheckRequestThreads();
                if (CsrpStaticThreadCount)
                {
                    CsrHandleHardError(CsrThread, (PHARDERROR_MSG)Request);
                }
                
                /* If the response was 0xFFFFFFFF, we'll ignore it */
                if (HardErrorMsg->Response == 0xFFFFFFFF)
                {
                    Reply = NULL;
                    ReplyPort = CsrApiPort;
                }
                else
                {
                    if (CsrThread) CsrDereferenceThread(CsrThread);
                    Reply = Request;
                    ReplyPort = CsrApiPort;
                }
            }
            else if (MessageType == LPC_REQUEST)
            {
                /* This is an API Message coming from a non-CSR Thread */
                DPRINT1("No thread found for request %lx and clientID %lx.%lx\n",
                        Request->Type & 0xFFFF,
                        Request->Header.ClientId.UniqueProcess,
                        Request->Header.ClientId.UniqueThread);
                Reply = Request;
                ReplyPort = CsrApiPort;
                Reply->Status = STATUS_ILLEGAL_FUNCTION;
            }
            else if (MessageType == LPC_DATAGRAM)
            {
                DPRINT1("Kernel datagram: not yet supported\n");
                Reply = NULL;
                ReplyPort = CsrApiPort;
            }
            else
            {
                /* Some other ignored message type */
                Reply = NULL;
                ReplyPort = CsrApiPort;
            }

            /* Keep going */
            continue;
        }
        
        /* We have a valid thread, was this an LPC Request? */
        if (MessageType != LPC_REQUEST)
        {
            /* It's not an API, check if the client died */
            if (MessageType == LPC_CLIENT_DIED)
            {
                /* Get the information and check if it matches our thread */
                ClientDiedMsg = (PCLIENT_DIED_MSG)Request;
                if (ClientDiedMsg->CreateTime.QuadPart == CsrThread->CreateTime.QuadPart)
                {
                    /* Reference the thread */
                    CsrLockedReferenceThread(CsrThread);

                    /* Destroy the thread in the API Message */
                    CsrDestroyThread(&Request->Header.ClientId);

                    /* Check if the thread was actually ourselves */
                    if (CsrProcess->ThreadCount == 1)
                    {
                        /* Kill the process manually here */
                        DPRINT1("Last thread\n");
                        CsrDestroyProcess(&CsrThread->ClientId, 0);
                    }

                    /* Remove our extra reference */
                    CsrLockedDereferenceThread(CsrThread);
                }

                /* Release the lock and keep looping */
                CsrReleaseProcessLock();
                Reply = NULL;
                ReplyPort = CsrApiPort;
                continue;
            }
            
            /* Reference the thread and release the lock */
            CsrLockedReferenceThread(CsrThread);
            CsrReleaseProcessLock();

            /* If this was an exception, handle it */
            if (MessageType == LPC_EXCEPTION)
            {
                /* Kill the process */
                DPRINT1("Exception in %lx.%lx. Killing...\n",
                        Request->Header.ClientId.UniqueProcess,
                        Request->Header.ClientId.UniqueThread);
                NtTerminateProcess(CsrProcess->ProcessHandle, STATUS_ABANDONED);

                /* Destroy it from CSR */
                CsrDestroyProcess(&Request->Header.ClientId, STATUS_ABANDONED);

                /* Return a Debug Message */
                DebugMessage = (PDBGKM_MSG)Request;
                DebugMessage->ReturnedStatus = DBG_CONTINUE;
                Reply = Request;
                ReplyPort = CsrApiPort;

                /* Remove our extra reference */
                CsrDereferenceThread(CsrThread);
            }
            else if (MessageType == LPC_ERROR_EVENT)
            {
                DPRINT1("Hard error from known CSR thread... handling\n");
                goto HandleHardError;
            }
            else
            {
                /* Something else */
                DPRINT1("Unhandled message type: %lx\n", MessageType);
                CsrDereferenceThread(CsrThread);
                Reply = NULL;
            }

            /* Keep looping */
            continue;
        }

        /* We got an API Request */
        CsrLockedReferenceThread(CsrThread);
        CsrReleaseProcessLock();
        
        /* Assume success */
        Reply = Request;
        Request->Status = STATUS_SUCCESS;

        /* Now we reply to a particular client */
        ReplyPort = CsrThread->Process->ClientPort;
        
        DPRINT("CSR: Got CSR API: %x [Message Origin: %x]\n",
                Request->Type,
                Request->Header.ClientId.UniqueThread);

        /* Validation complete, start SEH */
        _SEH2_TRY
        {
            /* Make sure we have enough threads */
            CsrpCheckRequestThreads();
            
            /* Set the client thread pointer */
            NtCurrentTeb()->CsrClientThread = CsrThread;

            /* Call the Handler */
            CsrApiCallHandler(CsrThread->Process, Request);
            
            /* Increase the static thread count */
            _InterlockedIncrement(&CsrpStaticThreadCount);

            /* Restore the server thread */
            NtCurrentTeb()->CsrClientThread = ServerThread;
            
            /* Check if this is a dead client now */
            if (Request->Type == 0xBABE)
            {
                /* Reply to the death message */
                NtReplyPort(ReplyPort, &Reply->Header);

                /* Reply back to the API port now */
                ReplyPort = CsrApiPort;
                Reply = NULL;

                /* Drop the reference */
                CsrDereferenceThread(CsrThread);
            }
            else
            {
                /* Drop the reference */
                CsrDereferenceThread(CsrThread);
            }
        }
        _SEH2_EXCEPT(CsrUnhandledExceptionFilter(_SEH2_GetExceptionInformation()))
        {
            Reply = NULL;
            ReplyPort = CsrApiPort;
        }
        _SEH2_END;
    }

    /* Close the port and exit the thread */
    // NtClose(ServerPort);

    DPRINT1("CSR: %s done\n", __FUNCTION__);
    /* We're out of the loop for some reason, terminate! */
    NtTerminateThread(NtCurrentThread(), Status);
    //return Status;
}

/*++
 * @name CsrReleaseCapturedArguments
 * @implemented NT5.1
 *
 * The CsrReleaseCapturedArguments routine releases a Capture Buffer
 * that was previously captured with CsrCaptureArguments.
 *
 * @param ApiMessage
 *        Pointer to the CSR API Message containing the Capture Buffer
 *        that needs to be released.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage)
{
    PCSR_CAPTURE_BUFFER RemoteCaptureBuffer, LocalCaptureBuffer;
    SIZE_T BufferDistance;
    ULONG PointerCount;
    ULONG_PTR **PointerOffsets, *CurrentPointer;

    /* Get the capture buffers */
    RemoteCaptureBuffer = ApiMessage->CsrCaptureData;
    LocalCaptureBuffer = RemoteCaptureBuffer->PreviousCaptureBuffer;

    /* Free the previous one */
    RemoteCaptureBuffer->PreviousCaptureBuffer = NULL;

    /* Find out the difference between the two buffers */
    BufferDistance = (ULONG_PTR)LocalCaptureBuffer - (ULONG_PTR)RemoteCaptureBuffer;

    /* Save the pointer count and offset pointer */
    PointerCount = RemoteCaptureBuffer->PointerCount;
    PointerOffsets = (ULONG_PTR**)(RemoteCaptureBuffer + 1);

    /* Start the loop */
    while (PointerCount)
    {
        /* Get the current pointer */
        CurrentPointer = *PointerOffsets++;
        if (CurrentPointer)
        {
            /* Add it to the CSR Message structure */
            CurrentPointer += (ULONG_PTR)ApiMessage;

            /* Modify the pointer to take into account its new position */
            *CurrentPointer += BufferDistance;
        }

        /* Move to the next Pointer */
        PointerCount--;
    }

    /* Copy the data back */
    RtlMoveMemory(LocalCaptureBuffer, RemoteCaptureBuffer, RemoteCaptureBuffer->Size);

    /* Free our allocated buffer */
    RtlFreeHeap(CsrHeap, 0, RemoteCaptureBuffer);
}

/* EOF */
