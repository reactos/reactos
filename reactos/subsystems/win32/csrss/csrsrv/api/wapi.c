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

static unsigned ApiDefinitionsCount = 0;
static PCSRSS_API_DEFINITION ApiDefinitions = NULL;

PCHAR CsrServerSbApiName[5] =
{
    "SbCreateSession",
    "SbTerminateSession",
    "SbForeignSessionComplete",
    "SbCreateProcess",
    "Unknown Csr Sb Api Number"
};

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
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Status = STATUS_INVALID_SYSTEM_SERVICE;
}

BOOL
CallHardError(IN PCSR_PROCESS ProcessData,
              IN PHARDERROR_MSG HardErrorMessage);

static
VOID
NTAPI
CsrHandleHardError(IN PCSR_PROCESS ProcessData,
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
CsrSrvAttachSharedSection(IN PCSR_PROCESS CsrProcess OPTIONAL,
                          OUT PCSR_CONNECTION_INFO ConnectInfo)
{
    NTSTATUS Status;
    ULONG ViewSize = 0;

    /* Check if we have a process */
    if (CsrProcess)
    {
        /* Map the section into this process */
        DPRINT("CSR Process Handle: %p. CSR Process: %p\n", CsrProcess->ProcessHandle, CsrProcess);
        Status = NtMapViewOfSection(CsrSrvSharedSection,
                                    CsrProcess->ProcessHandle,
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
    BOOLEAN LuidEnabled;
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
    NtQueryInformationProcess(NtCurrentProcess(),
                              ProcessLUIDDeviceMapsEnabled,
                              &LuidEnabled,
                              sizeof(LuidEnabled),
                              NULL);
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
CsrpHandleConnectionRequest (PPORT_MESSAGE Request,
                             IN HANDLE hApiListenPort)
{
    NTSTATUS Status;
    HANDLE ServerPort = NULL, ServerThread = NULL;
    PCSR_PROCESS ProcessData = NULL;
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

    if (ProcessData->ProcessHandle == NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;

        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
        DPRINT1("WARNING: CSR PROCESS WITH NO CSR PROCESS HANDLE???\n");
        ClientId.UniqueThread = 0;
        Status = NtOpenProcess(&ProcessData->ProcessHandle,
                               PROCESS_ALL_ACCESS,
                               &ObjectAttributes,
                               &Request->ClientId);
        DPRINT1("Status: %lx. Handle: %lx\n", Status, ProcessData->ProcessHandle);
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

    ProcessData->ClientViewBase = (ULONG_PTR)LpcRead.ViewBase;
    ProcessData->ClientViewBounds = LpcRead.ViewSize;
    ProcessData->ClientPort = ServerPort;

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
    PCSR_PROCESS ProcessData;
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
        if (ProcessData->Flags & CsrProcessTerminated)
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
            PCSR_PROCESS Process = NULL;

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

/* SESSION MANAGER FUNCTIONS**************************************************/

/*++
 * @name CsrSbCreateSession
 *
 * The CsrSbCreateSession API is called by the Session Manager whenever a new
 * session is created.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbCreateSession routine will initialize a new CSR NT
 *          Session and allocate a new CSR Process for the subsystem process.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbCreateSession(IN PSB_API_MSG ApiMessage)
{
    PSB_CREATE_SESSION_MSG CreateSession = &ApiMessage->CreateSession;
    HANDLE hProcess, hThread;
//    PCSR_PROCESS CsrProcess;
    NTSTATUS Status;
    KERNEL_USER_TIMES KernelTimes;
    //PCSR_THREAD CsrThread;
    //PVOID ProcessData;
    //ULONG i;

    /* Save the Process and Thread Handles */
    hProcess = CreateSession->ProcessInfo.ProcessHandle;
    hThread = CreateSession->ProcessInfo.ThreadHandle;

#if 0
    /* Lock the Processes */
    CsrAcquireProcessLock();

    /* Allocate a new process */
    CsrProcess = CsrAllocateProcess();
    if (!CsrProcess)
    {
        /* Fail */
        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        CsrReleaseProcessLock();
        return TRUE;
    }
#endif

    /* Set the exception port */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessExceptionPort,
                                     &hApiPort,//&CsrApiPort,
                                     sizeof(HANDLE));

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* Fail the request */
#if 0
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();
#endif
        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)STATUS_NO_MEMORY;
    }

    /* Get the Create Time */
    Status = NtQueryInformationThread(hThread,
                                      ThreadTimes,
                                      &KernelTimes,
                                      sizeof(KERNEL_USER_TIMES),
                                      NULL);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* Fail the request */
#if 0
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();
#endif

        /* Strange as it seems, NTSTATUSes are actually returned */
        return (BOOLEAN)Status;
    }

    /* Allocate a new Thread */
#if 0
    CsrThread = CsrAllocateThread(CsrProcess);
    if (!CsrThread)
    {
        /* Fail the request */
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();

        ApiMessage->ReturnValue = STATUS_NO_MEMORY;
        return TRUE;
    }

    /* Setup the Thread Object */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrThread->ThreadHandle = hThread;
    ProtectHandle(hThread);
    CsrThread->Flags = 0;

    /* Insert it into the Process List */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Setup Process Data */
    CsrProcess->ClientId = CreateSession->ProcessInfo.ClientId;
    CsrProcess->ProcessHandle = hProcess;
    CsrProcess->NtSession = CsrAllocateNtSession(CreateSession->SessionId);

    /* Set the Process Priority */
    CsrSetBackgroundPriority(CsrProcess);

    /* Get the first data location */
    ProcessData = &CsrProcess->ServerData[CSR_SERVER_DLL_MAX];

    /* Loop every DLL */
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Check if the DLL is loaded and has Process Data */
        if (CsrLoadedServerDll[i] && CsrLoadedServerDll[i]->SizeOfProcessData)
        {
            /* Write the pointer to the data */
            CsrProcess->ServerData[i] = ProcessData;

            /* Move to the next data location */
            ProcessData = (PVOID)((ULONG_PTR)ProcessData +
                                  CsrLoadedServerDll[i]->SizeOfProcessData);
        }
        else
        {
            /* Nothing for this Process */
            CsrProcess->ServerData[i] = NULL;
        }
    }

    /* Insert the Process */
    CsrInsertProcess(NULL, NULL, CsrProcess);
#endif
    /* Activate the Thread */
    ApiMessage->ReturnValue = NtResumeThread(hThread, NULL);

    /* Release lock and return */
//    CsrReleaseProcessLock();
    return TRUE;
}

/*++
 * @name CsrSbForeignSessionComplete
 *
 * The CsrSbForeignSessionComplete API is called by the Session Manager
 * whenever a foreign session is completed (ie: terminated).
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbForeignSessionComplete API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbForeignSessionComplete(IN PSB_API_MSG ApiMessage)
{
    /* Deprecated/Unimplemented in NT */
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

/*++
 * @name CsrSbTerminateSession
 *
 * The CsrSbTerminateSession API is called by the Session Manager
 * whenever a foreign session should be destroyed.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbTerminateSession API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbTerminateSession(IN PSB_API_MSG ApiMessage)
{
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

/*++
 * @name CsrSbCreateProcess
 *
 * The CsrSbCreateProcess API is called by the Session Manager
 * whenever a foreign session is created and a new process should be started.
 *
 * @param ApiMessage
 *        Pointer to the Session Manager API Message.
 *
 * @return TRUE in case of success, FALSE othwerwise.
 *
 * @remarks The CsrSbCreateProcess API is not yet implemented.
 *
 *--*/
BOOLEAN
NTAPI
CsrSbCreateProcess(IN PSB_API_MSG ApiMessage)
{
    ApiMessage->ReturnValue = STATUS_NOT_IMPLEMENTED;
    return TRUE;
}

PSB_API_ROUTINE CsrServerSbApiDispatch[5] =
{
    CsrSbCreateSession,
    CsrSbTerminateSession,
    CsrSbForeignSessionComplete,
    CsrSbCreateProcess,
    NULL
};

/*++
 * @name CsrSbApiHandleConnectionRequest
 *
 * The CsrSbApiHandleConnectionRequest routine handles and accepts a new
 * connection request to the SM API LPC Port.
 *
 * @param ApiMessage
 *        Pointer to the incoming CSR API Message which contains the
 *        connection request.
 *
 * @return STATUS_SUCCESS in case of success, or status code which caused
 *         the routine to error.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSbApiHandleConnectionRequest(IN PSB_API_MSG Message)
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW RemotePortView;
    HANDLE hPort;

    /* Set the Port View Structure Length */
    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);

    /* Accept the connection */
    Status = NtAcceptConnectPort(&hPort,
                                 NULL,
                                 (PPORT_MESSAGE)Message,
                                 TRUE,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Sb Accept Connection failed %lx\n", Status);
        return Status;
    }

    /* Complete the Connection */
    Status = NtCompleteConnectPort(hPort);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: Sb Complete Connection failed %lx\n",Status);
    }

    /* Return status */
    return Status;
}

/*++
 * @name CsrSbApiRequestThread
 *
 * The CsrSbApiRequestThread routine handles incoming messages or connection
 * requests on the SM API LPC Port.
 *
 * @param Parameter
 *        System-default user-defined parameter. Unused.
 *
 * @return The thread exit code, if the thread is terminated.
 *
 * @remarks Before listening on the port, the routine will first attempt
 *          to connect to the user subsystem.
 *
 *--*/
VOID
NTAPI
CsrSbApiRequestThread(IN PVOID Parameter)
{
    NTSTATUS Status;
    SB_API_MSG ReceiveMsg;
    PSB_API_MSG ReplyMsg = NULL;
    PVOID PortContext;
    ULONG MessageType;

    /* Start the loop */
    while (TRUE)
    {
        /* Wait for a message to come in */
        Status = NtReplyWaitReceivePort(CsrSbApiPort,
                                        &PortContext,
                                        &ReplyMsg->h,
                                        &ReceiveMsg.h);

        /* Check if we didn't get success */
        if (Status != STATUS_SUCCESS)
        {
            /* If we only got a warning, keep going */
            if (NT_SUCCESS(Status)) continue;

            /* We failed big time, so start out fresh */
            ReplyMsg = NULL;
            DPRINT1("CSRSS: ReceivePort failed - Status == %X\n", Status);
            continue;
        }

        /* Save the message type */
        MessageType = ReceiveMsg.h.u2.s2.Type;

        /* Check if this is a connection request */
        if (MessageType == LPC_CONNECTION_REQUEST)
        {
            /* Handle connection request */
            CsrSbApiHandleConnectionRequest(&ReceiveMsg);

            /* Start over */
            ReplyMsg = NULL;
            continue;
        }

        /* Check if the port died */
        if (MessageType == LPC_PORT_CLOSED)
        {
            /* Close the handle if we have one */
            if (PortContext) NtClose((HANDLE)PortContext);

            /* Client died, start over */
            ReplyMsg = NULL;
            continue;
        }
        else if (MessageType == LPC_CLIENT_DIED)
        {
            /* Client died, start over */
            ReplyMsg = NULL;
            continue;
        }

        /*
         * It's an API Message, check if it's within limits. If it's not, the
         * NT Behaviour is to set this to the Maximum API.
         */
        if (ReceiveMsg.ApiNumber > SbpMaxApiNumber)
        {
            ReceiveMsg.ApiNumber = SbpMaxApiNumber;
            DPRINT1("CSRSS: %lx is invalid Sb ApiNumber\n", ReceiveMsg.ApiNumber);
         }

        /* Reuse the message */
        ReplyMsg = &ReceiveMsg;

        /* Make sure that the message is supported */
        if (ReceiveMsg.ApiNumber < SbpMaxApiNumber)
        {
            /* Call the API */
            if (!CsrServerSbApiDispatch[ReceiveMsg.ApiNumber](&ReceiveMsg))
            {
                /* It failed, so return nothing */
                ReplyMsg = NULL;
            }
        }
        else
        {
            /* We don't support this API Number */
            ReplyMsg->ReturnValue = STATUS_NOT_IMPLEMENTED;
        }
    }
}

/* EOF */
