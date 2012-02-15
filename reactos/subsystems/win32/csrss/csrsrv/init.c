/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsystems/win32/csrss/csrsrv/init.c
 * PURPOSE:         CSR Server DLL Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "srv.h"
#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

HANDLE CsrHeap = (HANDLE) 0;
HANDLE CsrObjectDirectory = (HANDLE) 0;
UNICODE_STRING CsrDirectoryName;
extern HANDLE CsrssApiHeap;
static unsigned ServerProcCount;
static CSRPLUGIN_SERVER_PROCS *ServerProcs = NULL;
HANDLE hSbApiPort = (HANDLE) 0;
HANDLE hBootstrapOk = (HANDLE) 0;
HANDLE hSmApiPort = (HANDLE) 0;
HANDLE hApiPort = (HANDLE) 0;
ULONG CsrDebug = 0xFFFFFFFF;
ULONG CsrMaxApiRequestThreads;
ULONG CsrTotalPerProcessDataLength;
ULONG SessionId;
HANDLE BNOLinksDirectory;
HANDLE SessionObjectDirectory;
HANDLE DosDevicesDirectory;

/* PRIVATE FUNCTIONS **********************************************************/

static NTSTATUS FASTCALL
CsrpAddServerProcs(CSRPLUGIN_SERVER_PROCS *Procs)
{
  CSRPLUGIN_SERVER_PROCS *NewProcs;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  NewProcs = RtlAllocateHeap(CsrssApiHeap, 0,
                             (ServerProcCount + 1)
                             * sizeof(CSRPLUGIN_SERVER_PROCS));
  if (NULL == NewProcs)
    {
      return STATUS_NO_MEMORY;
    }
  if (0 != ServerProcCount)
    {
      RtlCopyMemory(NewProcs, ServerProcs,
                    ServerProcCount * sizeof(CSRPLUGIN_SERVER_PROCS));
      RtlFreeHeap(CsrssApiHeap, 0, ServerProcs);
    }
  NewProcs[ServerProcCount] = *Procs;
  ServerProcs = NewProcs;
  ServerProcCount++;

  return STATUS_SUCCESS;
}

/**********************************************************************
 * CallInitComplete/0
 */
static BOOL FASTCALL
CallInitComplete(void)
{
  BOOL Ok;
  unsigned i;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  Ok = TRUE;
  for (i = 0; i < ServerProcCount && Ok; i++)
    {
      Ok = (*ServerProcs[i].InitCompleteProc)();
    }

  return Ok;
}

BOOL
CallHardError(IN PCSRSS_PROCESS_DATA ProcessData,
              IN PHARDERROR_MSG HardErrorMessage)
{
    BOOL Ok;
    unsigned i;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    Ok = TRUE;
    for (i = 0; i < ServerProcCount && Ok; i++)
    {
        Ok = (*ServerProcs[i].HardErrorProc)(ProcessData, HardErrorMessage);
    }

    return Ok;
}

NTSTATUS
CallProcessInherit(IN PCSRSS_PROCESS_DATA SourceProcessData,
                   IN PCSRSS_PROCESS_DATA TargetProcessData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    unsigned i;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    for (i = 0; i < ServerProcCount && NT_SUCCESS(Status); i++)
        Status = (*ServerProcs[i].ProcessInheritProc)(SourceProcessData, TargetProcessData);

    return Status;
}

NTSTATUS
CallProcessDeleted(IN PCSRSS_PROCESS_DATA ProcessData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    unsigned i;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    for (i = 0; i < ServerProcCount && NT_SUCCESS(Status); i++)
        Status = (*ServerProcs[i].ProcessDeletedProc)(ProcessData);

    return Status;
}

/**********************************************************************
 * CsrpInitWin32Csr/3
 *
 * TODO: this function should be turned more general to load an
 * TODO: hosted server DLL as received from the command line;
 * TODO: for instance: ServerDll=winsrv:ConServerDllInitialization,2
 * TODO:               ^method   ^dll   ^api                       ^sid
 * TODO:
 * TODO: CsrpHostServerDll (LPWSTR DllName,
 * TODO:                    LPWSTR ApiName,
 * TODO:                    DWORD  ServerId)
 */
static NTSTATUS
CsrpInitWin32Csr (VOID)
{
  NTSTATUS Status;
  UNICODE_STRING DllName;
  HINSTANCE hInst;
  ANSI_STRING ProcName;
  CSRPLUGIN_INITIALIZE_PROC InitProc;
  CSRSS_EXPORTED_FUNCS Exports;
  PCSRSS_API_DEFINITION ApiDefinitions;
  CSRPLUGIN_SERVER_PROCS ServerProcs;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  RtlInitUnicodeString(&DllName, L"win32csr.dll");
  Status = LdrLoadDll(NULL, 0, &DllName, (PVOID *) &hInst);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  RtlInitAnsiString(&ProcName, "Win32CsrInitialization");
  Status = LdrGetProcedureAddress(hInst, &ProcName, 0, (PVOID *) &InitProc);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Exports.CsrEnumProcessesProc = CsrEnumProcesses;
  if (! (*InitProc)(&ApiDefinitions, &ServerProcs, &Exports, CsrssApiHeap))
    {
      return STATUS_UNSUCCESSFUL;
    }

  Status = CsrApiRegisterDefinitions(ApiDefinitions);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Status = CsrpAddServerProcs(&ServerProcs);
  return Status;
}

CSRSS_API_DEFINITION NativeDefinitions[] =
  {
    CSRSS_DEFINE_API(CREATE_PROCESS,               CsrCreateProcess),
    CSRSS_DEFINE_API(CREATE_THREAD,                CsrSrvCreateThread),
    CSRSS_DEFINE_API(TERMINATE_PROCESS,            CsrTerminateProcess),
    CSRSS_DEFINE_API(CONNECT_PROCESS,              CsrConnectProcess),
    CSRSS_DEFINE_API(REGISTER_SERVICES_PROCESS,    CsrRegisterServicesProcess),
    CSRSS_DEFINE_API(GET_SHUTDOWN_PARAMETERS,      CsrGetShutdownParameters),
    CSRSS_DEFINE_API(SET_SHUTDOWN_PARAMETERS,      CsrSetShutdownParameters),
    { 0, 0, NULL }
  };

static NTSTATUS WINAPI
CsrpCreateListenPort (IN     LPWSTR  Name,
		      IN OUT PHANDLE Port,
		      IN     PTHREAD_START_ROUTINE ListenThread)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES  PortAttributes;
	UNICODE_STRING     PortName;
    HANDLE ServerThread;
    CLIENT_ID ClientId;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	RtlInitUnicodeString (& PortName, Name);
	InitializeObjectAttributes (& PortAttributes,
				    & PortName,
				    0,
				    NULL,
				    NULL);
	Status = NtCreatePort ( Port,
				& PortAttributes,
				sizeof(SB_CONNECTION_INFO),
				sizeof(SB_API_MSG),
				32 * sizeof(SB_API_MSG));
	if(!NT_SUCCESS(Status))
	{
		DPRINT1("CSR: %s: NtCreatePort failed (Status=%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
	Status = RtlCreateUserThread(NtCurrentProcess(),
                               NULL,
                               TRUE,
                               0,
                               0,
                               0,
                               (PTHREAD_START_ROUTINE) ListenThread,
                               *Port,
                               &ServerThread,
                               &ClientId);
    
    if (ListenThread == (PVOID)ClientConnectionThread)
    {
        CsrAddStaticServerThread(ServerThread, &ClientId, 0);
    }
    
    NtResumeThread(ServerThread, NULL);
    NtClose(ServerThread);
	return Status;
}

/* === INIT ROUTINES === */

VOID
WINAPI
BasepFakeStaticServerData(VOID);

NTSTATUS
NTAPI
CsrSrvCreateSharedSection(IN PCHAR ParameterValue);

/**********************************************************************
 * CsrpCreateHeap/3
 */
static NTSTATUS
CsrpCreateHeap (VOID)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

    CsrHeap = RtlGetProcessHeap();
	CsrssApiHeap = RtlCreateHeap(HEAP_GROWABLE,
        	                       NULL,
                	               65536,
                        	       65536,
	                               NULL,
        	                       NULL);
	if (CsrssApiHeap == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
    
	return STATUS_SUCCESS;
}

/**********************************************************************
 * CsrpRegisterSubsystem/3
 */
BOOLEAN g_ModernSm;
static NTSTATUS
CsrpRegisterSubsystem (VOID)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES  BootstrapOkAttributes;
	UNICODE_STRING     Name;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	/*
	 * Create the event object the callback port
	 * thread will signal *if* the SM will
	 * authorize us to bootstrap.
	 */
	RtlInitUnicodeString (& Name, L"\\CsrssBooting");
	InitializeObjectAttributes(& BootstrapOkAttributes,
				   & Name,
				   0, NULL, NULL);
	Status = NtCreateEvent (& hBootstrapOk,
				EVENT_ALL_ACCESS,
				& BootstrapOkAttributes,
				SynchronizationEvent,
				FALSE);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("CSR: %s: NtCreateEvent failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
	/*
	 * Let's tell the SM a new environment
	 * subsystem server is in the system.
	 */
	RtlInitUnicodeString (& Name, L"\\Windows\\SbApiPort");
	DPRINT("CSR: %s: registering with SM for\n  IMAGE_SUBSYSTEM_WINDOWS_CUI == 3\n", __FUNCTION__);
	Status = SmConnectApiPort (& Name,
				   hSbApiPort,
				   IMAGE_SUBSYSTEM_WINDOWS_CUI,
				   & hSmApiPort);
    if (!NT_SUCCESS(Status))
    {
        Status = SmConnectToSm(&Name, hSbApiPort, IMAGE_SUBSYSTEM_WINDOWS_GUI, &hSmApiPort);
        g_ModernSm = TRUE;
    }
	if(!NT_SUCCESS(Status))
	{
		DPRINT("CSR: %s unable to connect to the SM (Status=0x%08lx)\n",
			__FUNCTION__, Status);
		NtClose (hBootstrapOk);
		return Status;
	}
	/*
	 *  Wait for SM to reply OK... If the SM
	 *  won't answer, we hang here forever!
	 */
	DPRINT("CSR: %s: waiting for SM to OK boot...\n", __FUNCTION__);
	Status = NtWaitForSingleObject (hBootstrapOk,
					FALSE,
					NULL);
	NtClose (hBootstrapOk);
	return Status;
}

/*++
 * @name CsrSetDirectorySecurity
 *
 * The CsrSetDirectorySecurity routine sets the security descriptor for the
 * specified Object Directory.
 *
 * @param ObjectDirectory
 *        Handle fo the Object Directory to protect.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSetDirectorySecurity(IN HANDLE ObjectDirectory)
{
    /* FIXME: Implement */
    return STATUS_SUCCESS;
}

/*++
 * @name GetDosDevicesProtection
 *
 * The GetDosDevicesProtection creates a security descriptor for the DOS Devices
 * Object Directory.
 *
 * @param DosDevicesSd
 *        Pointer to the Security Descriptor to return.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks Depending on the DOS Devices Protection Mode (set in the registry),
 *          regular users may or may not have full access to the directory.
 *
 *--*/
NTSTATUS
NTAPI
GetDosDevicesProtection(OUT PSECURITY_DESCRIPTOR DosDevicesSd)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID WorldSid, CreatorSid, AdminSid, SystemSid;
    UCHAR KeyValueBuffer[0x40];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
    UNICODE_STRING KeyName;
    ULONG ProtectionMode = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PACL Dacl;
    PACCESS_ALLOWED_ACE Ace;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG ResultLength, SidLength, AclLength;

    /* Create the SD */
    Status = RtlCreateSecurityDescriptor(DosDevicesSd, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));

    /* Initialize the System SID */
    Status = RtlAllocateAndInitializeSid(&NtSidAuthority, 1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &SystemSid);
    ASSERT(NT_SUCCESS(Status));

    /* Initialize the World SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority, 1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    ASSERT(NT_SUCCESS(Status));

    /* Initialize the Admin SID */
    Status = RtlAllocateAndInitializeSid(&NtSidAuthority, 2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &AdminSid);
    ASSERT(NT_SUCCESS(Status));

    /* Initialize the Creator SID */
    Status = RtlAllocateAndInitializeSid(&CreatorAuthority, 1,
                                         SECURITY_CREATOR_OWNER_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &CreatorSid);
    ASSERT(NT_SUCCESS(Status));

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

    /* Check the Protection Mode */
    if (ProtectionMode & 3)
    {
        /* Calculate SID Lengths */
        SidLength = RtlLengthSid(CreatorSid) + RtlLengthSid(SystemSid) +
                    RtlLengthSid(AdminSid);
        AclLength = sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;

        /* Allocate memory for the DACL */
        Dacl = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, AclLength);
        ASSERT(Dacl != NULL);

        /* Build the ACL and add 3 ACEs */
        Status = RtlCreateAcl(Dacl, AclLength, ACL_REVISION2);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_ALL, SystemSid);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_ALL, AdminSid);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_ALL, CreatorSid);
        ASSERT(NT_SUCCESS(Status));

        /* Edit the ACEs to make them inheritable */
        Status = RtlGetAce(Dacl, 0, (PVOID*)&Ace);
        ASSERT(NT_SUCCESS(Status));
        Ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        Status = RtlGetAce(Dacl, 1, (PVOID*)&Ace);
        ASSERT(NT_SUCCESS(Status));
        Ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        Status = RtlGetAce(Dacl, 2, (PVOID*)&Ace);
        ASSERT(NT_SUCCESS(Status));
        Ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

        /* Set this DACL with the SD */
        Status = RtlSetDaclSecurityDescriptor(DosDevicesSd, TRUE, Dacl, FALSE);
        ASSERT(NT_SUCCESS(Status));
        goto Quickie;
    }
    else
    {
        /* Calculate SID Lengths */
        SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(SystemSid);
        AclLength = sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;

        /* Allocate memory for the DACL */
        Dacl = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, AclLength);
        ASSERT(Dacl != NULL);

        /* Build the ACL and add 3 ACEs */
        Status = RtlCreateAcl(Dacl, AclLength, ACL_REVISION2);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, WorldSid);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_ALL, SystemSid);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, GENERIC_ALL, WorldSid);
        ASSERT(NT_SUCCESS(Status));

        /* Edit the last ACE to make it inheritable */
        Status = RtlGetAce(Dacl, 2, (PVOID*)&Ace);
        ASSERT(NT_SUCCESS(Status));
        Ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

        /* Set this DACL with the SD */
        Status = RtlSetDaclSecurityDescriptor(DosDevicesSd, TRUE, Dacl, FALSE);
        ASSERT(NT_SUCCESS(Status));
        goto Quickie;
    }

/* FIXME: failure cases! Fail: */
    /* Free the memory */
    RtlFreeHeap(CsrHeap, 0, Dacl);

/* FIXME: semi-failure cases! Quickie: */
Quickie:
    /* Free the SIDs */
    RtlFreeSid(SystemSid);
    RtlFreeSid(WorldSid);
    RtlFreeSid(AdminSid);
    RtlFreeSid(CreatorSid);

    /* Return */
    return Status;
}

/*++
 * @name FreeDosDevicesProtection
 *
 * The FreeDosDevicesProtection frees the security descriptor that was created
 * by GetDosDevicesProtection
 *
 * @param DosDevicesSd
 *        Pointer to the security descriptor to free.

 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
FreeDosDevicesProtection(IN PSECURITY_DESCRIPTOR DosDevicesSd)
{
    PACL Dacl;
    BOOLEAN Present, Default;
    NTSTATUS Status;

    /* Get the DACL corresponding to this SD */
    Status = RtlGetDaclSecurityDescriptor(DosDevicesSd, &Present, &Dacl, &Default);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(Present);
    ASSERT(Dacl != NULL);

    /* Free it */
    if ((NT_SUCCESS(Status)) && (Dacl)) RtlFreeHeap(CsrHeap, 0, Dacl);
}

/*++
 * @name CsrCreateSessionObjectDirectory
 *
 * The CsrCreateSessionObjectDirectory routine creates the BaseNamedObjects,
 * Session and Dos Devices directories for the specified session.
 *
 * @param Session
 *        Session ID for which to create the directories.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateSessionObjectDirectory(IN ULONG Session)
{
    WCHAR SessionBuffer[512], BnoBuffer[512];
    UNICODE_STRING SessionString, BnoString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BnoHandle;
    SECURITY_DESCRIPTOR DosDevicesSd;
    NTSTATUS Status;

    /* Generate the Session BNOLINKS Directory name */
    swprintf(SessionBuffer, L"%ws\\BNOLINKS", SESSION_ROOT);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Create it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateDirectoryObject(&BNOLinksDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: NtCreateDirectoryObject failed in "
                "CsrCreateSessionObjectDirectory - status = %lx\n", Status);
        return Status;
    }

    /* Now add the Session ID */
    swprintf(SessionBuffer, L"%ld", Session);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Check if this is the first Session */
    if (Session)
    {
        /* Not the first, so the name will be slighly more complex */
        swprintf(BnoBuffer, L"%ws\\%ld\\BaseNamedObjects", SESSION_ROOT, Session);
        RtlInitUnicodeString(&BnoString, BnoBuffer);
    }
    else
    {
        /* Use the direct name */
        RtlInitUnicodeString(&BnoString, L"\\BaseNamedObjects");
    }

    /* Create the symlink */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               BNOLinksDirectory,
                               NULL);
    Status = NtCreateSymbolicLinkObject(&BnoHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &BnoString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: NtCreateSymbolicLinkObject failed in "
                "CsrCreateSessionObjectDirectory - status = %lx\n", Status);
        return Status;
    }

    /* Create the \DosDevices Security Descriptor */
    Status = GetDosDevicesProtection(&DosDevicesSd);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now create a directory for this session */
    swprintf(SessionBuffer, L"%ws\\%ld", SESSION_ROOT, Session);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Create the directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               0,
                               &DosDevicesSd);
    Status = NtCreateDirectoryObject(&SessionObjectDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: NtCreateDirectoryObject failed in "
                "CsrCreateSessionObjectDirectory - status = %lx\n", Status);
        FreeDosDevicesProtection(&DosDevicesSd);
        return Status;
    }

    /* Next, create a directory for this session's DOS Devices */
    RtlInitUnicodeString(&SessionString, L"DosDevices");
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_CASE_INSENSITIVE,
                               SessionObjectDirectory,
                               &DosDevicesSd);
    Status = NtCreateDirectoryObject(&DosDevicesDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: NtCreateDirectoryObject failed in "
                "CsrCreateSessionObjectDirectory - status = %lx\n", Status);
    }

    /* Release the Security Descriptor */
    FreeDosDevicesProtection(&DosDevicesSd);

    /* Return */
    return Status;
}

/*++
 * @name CsrParseServerCommandLine
 *
 * The CsrParseServerCommandLine routine parses the CSRSS command-line in the
 * registry and performs operations for each entry found.
 *
 * @param ArgumentCount
 *        Number of arguments on the command line.
 *
 * @param Arguments
 *        Array of arguments.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
FASTCALL
CsrParseServerCommandLine(IN ULONG ArgumentCount,
                          IN PCHAR Arguments[])
{
    NTSTATUS Status;
    PCHAR ParameterName = NULL, ParameterValue = NULL, EntryPoint, ServerString;
    ULONG i, DllIndex;
    ANSI_STRING AnsiString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Set the Defaults */
    CsrTotalPerProcessDataLength = 0;
    CsrObjectDirectory = NULL;
    CsrMaxApiRequestThreads = 16;

    /* Save our Session ID, and create a Directory for it */
    SessionId = NtCurrentPeb()->SessionId;
    Status = CsrCreateSessionObjectDirectory(SessionId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: CsrCreateSessionObjectDirectory failed (%lx)\n",
                Status);

        /* It's not fatal if the session ID isn't zero */
        if (SessionId) return Status;
        ASSERT(NT_SUCCESS(Status));
    }

    /* Loop through every argument */
    for (i = 1; i < ArgumentCount; i++)
    {
        /* Split Name and Value */
        ParameterName = Arguments[i];
        DPRINT1("Name: %s\n", ParameterName);
        ParameterValue = NULL;
        ParameterValue = strchr(ParameterName, '=');
        if (ParameterValue) *ParameterValue++ = ANSI_NULL;
        DPRINT1("Name=%s, Value=%s\n", ParameterName, ParameterValue);

        /* Check for Object Directory */
        if (!_stricmp(ParameterName, "ObjectDirectory"))
        {
            /* Check if a session ID is specified */
            if (SessionId)
            {
                DPRINT1("Sessions not yet implemented\n");
                ASSERT(SessionId);
            }

            /* Initialize the directory name */
            RtlInitAnsiString(&AnsiString, ParameterValue);
            Status = RtlAnsiStringToUnicodeString(&CsrDirectoryName,
                                                  &AnsiString,
                                                  TRUE);
            ASSERT(NT_SUCCESS(Status) || SessionId != 0);
            if (!NT_SUCCESS(Status)) return Status;

            /* Create it */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &CsrDirectoryName,
                                       OBJ_OPENIF | OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                       NULL,
                                       NULL);
            Status = NtCreateDirectoryObject(&CsrObjectDirectory,
                                             DIRECTORY_ALL_ACCESS,
                                             &ObjectAttributes);
            if (!NT_SUCCESS(Status)) return Status;

            /* Secure it */
            Status = CsrSetDirectorySecurity(CsrObjectDirectory);
            if (!NT_SUCCESS(Status)) return Status;
        }
        else if (!_stricmp(ParameterName, "SubSystemType"))
        {
            /* Ignored */
        }
        else if (!_stricmp(ParameterName, "MaxRequestThreads"))
        {
            Status = RtlCharToInteger(ParameterValue,
                                      0,
                                      &CsrMaxApiRequestThreads);
        }
        else if (!_stricmp(ParameterName, "RequestThreads"))
        {
            /* Ignored */
            Status = STATUS_SUCCESS;
        }
        else if (!_stricmp(ParameterName, "ProfileControl"))
        {
            /* Ignored */
        }
        else if (!_stricmp(ParameterName, "SharedSection"))
        {
            /* Create the Section */
            Status = CsrSrvCreateSharedSection(ParameterValue);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CSRSS: *** Invalid syntax for %s=%s (Status == %X)\n",
                        ParameterName, ParameterValue, Status);
                return Status;
            }

            /* Load us */
            BasepFakeStaticServerData();
            #if 0
            Status = CsrLoadServerDll("CSRSS", NULL, CSR_SRV_SERVER);
            #endif
        }
        else if (!_stricmp(ParameterName, "ServerDLL"))
        {
            /* Loop the command line */
            EntryPoint = NULL;
            Status = STATUS_INVALID_PARAMETER;
            ServerString = ParameterValue;
            while (*ServerString)
            {
                /* Check for the Entry Point */
                if ((*ServerString == ':') && (!EntryPoint))
                {
                    /* Found it. Add a nullchar and save it */
                    *ServerString++ = ANSI_NULL;
                    EntryPoint = ServerString;
                }

                /* Check for the Dll Index */
                if (*ServerString++ == ',') break;
            }

            /* Did we find something to load? */
            if (!*ServerString)
            {
                DPRINT1("CSRSS: *** Invalid syntax for ServerDll=%s (Status == %X)\n",
                        ParameterValue, Status);
                return Status;
            }

            /* Convert it to a ULONG */
            Status = RtlCharToInteger(ServerString, 10, &DllIndex);

            /* Add a null char if it was valid */
            if (NT_SUCCESS(Status)) ServerString[-1] = ANSI_NULL;

            /* Load it */
            if (CsrDebug & 1) DPRINT1("CSRSS: Should be loading ServerDll=%s:%s\n", ParameterValue, EntryPoint);
            Status = STATUS_SUCCESS;
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CSRSS: *** Failed loading ServerDll=%s (Status == 0x%x)\n",
                        ParameterValue, Status);
                return Status;
            }
        }
        else if (!_stricmp(ParameterName, "Windows"))
        {
            /* Ignored */
        }
        else
        {
            /* Invalid parameter on the command line */
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
CsrServerInitialization(ULONG ArgumentCount,
                        PCHAR Arguments[])
{
	NTSTATUS  Status = STATUS_SUCCESS;

	DPRINT("CSR: %s called\n", __FUNCTION__);

    Status = CsrpCreateHeap();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpCreateHeap", Status);
    }

    /* Parse the command line */
    Status = CsrParseServerCommandLine(ArgumentCount, Arguments);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: CsrParseServerCommandLine failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }
    
    CsrInitProcessData();

    Status = CsrpCreateListenPort(L"\\Windows\\ApiPort", &hApiPort, (PTHREAD_START_ROUTINE)ClientConnectionThread);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpCreateApiPort", Status);
    }

    Status = CsrApiRegisterDefinitions(NativeDefinitions);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrApiRegisterDefinitions", Status);
    }

    Status = CsrpInitWin32Csr();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpInitWin32Csr", Status);
    }

    Status = CsrpCreateListenPort(L"\\Windows\\SbApiPort", &hSbApiPort, ServerSbApiPortThread);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpCreateCallbackPort", Status);
    }

    Status = CsrpRegisterSubsystem();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpRegisterSubsystem", Status);
    }

    Status = NtSetDefaultHardErrorPort(hApiPort);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV failed in %s with status %lx\n", "CsrpCreateHardErrorPort", Status);
    }
    
	if (CallInitComplete())
	{
		return STATUS_SUCCESS;
	}
    
	return STATUS_UNSUCCESSFUL;
}

BOOL
NTAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    /* We don't do much */
    UNREFERENCED_PARAMETER(hDll);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);
    return TRUE;
}

/* EOF */
