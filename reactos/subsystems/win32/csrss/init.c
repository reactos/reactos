/* $Id$
 *
 * reactos/subsys/csrss/init.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <csrss.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE CsrHeap = (HANDLE) 0;

HANDLE CsrObjectDirectory = (HANDLE) 0;

UNICODE_STRING CsrDirectoryName;

extern HANDLE CsrssApiHeap;

static unsigned InitCompleteProcCount;
static CSRPLUGIN_INIT_COMPLETE_PROC *InitCompleteProcs = NULL;

static unsigned HardErrorProcCount;
static CSRPLUGIN_HARDERROR_PROC *HardErrorProcs = NULL;

HANDLE hSbApiPort = (HANDLE) 0;

HANDLE hBootstrapOk = (HANDLE) 0;

HANDLE hSmApiPort = (HANDLE) 0;

HANDLE hApiPort = (HANDLE) 0;

/**********************************************************************
 * CsrpAddInitCompleteProc/1
 */
static NTSTATUS FASTCALL
CsrpAddInitCompleteProc(CSRPLUGIN_INIT_COMPLETE_PROC Proc)
{
  CSRPLUGIN_INIT_COMPLETE_PROC *NewProcs;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  NewProcs = RtlAllocateHeap(CsrssApiHeap, 0,
                             (InitCompleteProcCount + 1)
                             * sizeof(CSRPLUGIN_INIT_COMPLETE_PROC));
  if (NULL == NewProcs)
    {
      return STATUS_NO_MEMORY;
    }
  if (0 != InitCompleteProcCount)
    {
      RtlCopyMemory(NewProcs, InitCompleteProcs,
                    InitCompleteProcCount * sizeof(CSRPLUGIN_INIT_COMPLETE_PROC));
      RtlFreeHeap(CsrssApiHeap, 0, InitCompleteProcs);
    }
  NewProcs[InitCompleteProcCount] = Proc;
  InitCompleteProcs = NewProcs;
  InitCompleteProcCount++;

  return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
CsrpAddHardErrorProc(CSRPLUGIN_HARDERROR_PROC Proc)
{
    CSRPLUGIN_HARDERROR_PROC *NewProcs;

    DPRINT("CSR: %s called\n", __FUNCTION__);

    NewProcs = RtlAllocateHeap(CsrssApiHeap, 0,
                               (HardErrorProcCount + 1)
                               * sizeof(CSRPLUGIN_HARDERROR_PROC));
    if (NULL == NewProcs)
    {
        return STATUS_NO_MEMORY;
    }
    if (0 != HardErrorProcCount)
    {
        RtlCopyMemory(NewProcs, HardErrorProcs,
            HardErrorProcCount * sizeof(CSRPLUGIN_HARDERROR_PROC));
        RtlFreeHeap(CsrssApiHeap, 0, HardErrorProcs);
    }

    NewProcs[HardErrorProcCount] = Proc;
    HardErrorProcs = NewProcs;
    HardErrorProcCount++;

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
  if (0 != InitCompleteProcCount)
    {
      for (i = 0; i < InitCompleteProcCount && Ok; i++)
        {
          Ok = (*(InitCompleteProcs[i]))();
        }
      RtlFreeHeap(CsrssApiHeap, 0, InitCompleteProcs);
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
    if (0 != HardErrorProcCount)
    {
        for (i = 0; i < HardErrorProcCount && Ok; i++)
        {
            Ok = (*(HardErrorProcs[i]))(ProcessData, HardErrorMessage);
        }
    }

    return Ok;
}

ULONG
InitializeVideoAddressSpace(VOID);

/**********************************************************************
 * CsrpCreateObjectDirectory/3
 */
static NTSTATUS
CsrpCreateObjectDirectory (int argc, char ** argv, char ** envp)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Attributes;

  DPRINT("CSR: %s called\n", __FUNCTION__);


	/* create object directory ('\Windows') */
	RtlCreateUnicodeString (&CsrDirectoryName,
	                        L"\\Windows");

	InitializeObjectAttributes (&Attributes,
	                            &CsrDirectoryName,
	                            OBJ_OPENIF,
	                            NULL,
	                            NULL);

	Status = NtOpenDirectoryObject(&CsrObjectDirectory,
	                               DIRECTORY_ALL_ACCESS,
	                               &Attributes);
	return Status;
}

/**********************************************************************
 * CsrpInitVideo/3
 *
 * TODO: we need a virtual device for sessions other than
 * TODO: the console one
 */
static NTSTATUS
CsrpInitVideo (int argc, char ** argv, char ** envp)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\??\\DISPLAY1");
  IO_STATUS_BLOCK Iosb;
  HANDLE VideoHandle = (HANDLE) 0;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  InitializeVideoAddressSpace();

  InitializeObjectAttributes(&ObjectAttributes,
			     &DeviceName,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenFile(&VideoHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &Iosb,
		      0,
		      0);
  if (NT_SUCCESS(Status))
    {
      NtClose(VideoHandle);
    }
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
CsrpInitWin32Csr (int argc, char ** argv, char ** envp)
{
  NTSTATUS Status;
  UNICODE_STRING DllName;
  HINSTANCE hInst;
  ANSI_STRING ProcName;
  CSRPLUGIN_INITIALIZE_PROC InitProc;
  CSRSS_EXPORTED_FUNCS Exports;
  PCSRSS_API_DEFINITION ApiDefinitions;
  PCSRSS_OBJECT_DEFINITION ObjectDefinitions;
  CSRPLUGIN_INIT_COMPLETE_PROC InitCompleteProc;
  CSRPLUGIN_HARDERROR_PROC HardErrorProc;

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
  Exports.CsrInsertObjectProc = CsrInsertObject;
  Exports.CsrGetObjectProc = CsrGetObject;
  Exports.CsrReleaseObjectProc = CsrReleaseObject;
  Exports.CsrEnumProcessesProc = CsrEnumProcesses;
  if (! (*InitProc)(&ApiDefinitions, &ObjectDefinitions, &InitCompleteProc,
                    &HardErrorProc, &Exports, CsrssApiHeap))
    {
      return STATUS_UNSUCCESSFUL;
    }

  Status = CsrApiRegisterDefinitions(ApiDefinitions);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Status = CsrRegisterObjectDefinitions(ObjectDefinitions);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  if (NULL != InitCompleteProc)
    {
      Status = CsrpAddInitCompleteProc(InitCompleteProc);
    }
  if (HardErrorProc) Status = CsrpAddHardErrorProc(HardErrorProc);

  return Status;
}

CSRSS_API_DEFINITION NativeDefinitions[] =
  {
    CSRSS_DEFINE_API(CREATE_PROCESS,               CsrCreateProcess),
    CSRSS_DEFINE_API(TERMINATE_PROCESS,            CsrTerminateProcess),
    CSRSS_DEFINE_API(CONNECT_PROCESS,              CsrConnectProcess),
    CSRSS_DEFINE_API(REGISTER_SERVICES_PROCESS,    CsrRegisterServicesProcess),
    CSRSS_DEFINE_API(GET_SHUTDOWN_PARAMETERS,      CsrGetShutdownParameters),
    CSRSS_DEFINE_API(SET_SHUTDOWN_PARAMETERS,      CsrSetShutdownParameters),
    CSRSS_DEFINE_API(GET_INPUT_HANDLE,             CsrGetInputHandle),
    CSRSS_DEFINE_API(GET_OUTPUT_HANDLE,            CsrGetOutputHandle),
    CSRSS_DEFINE_API(CLOSE_HANDLE,                 CsrCloseHandle),
    CSRSS_DEFINE_API(VERIFY_HANDLE,                CsrVerifyHandle),
    CSRSS_DEFINE_API(DUPLICATE_HANDLE,             CsrDuplicateHandle),
    CSRSS_DEFINE_API(GET_INPUT_WAIT_HANDLE,        CsrGetInputWaitHandle),
    { 0, 0, NULL }
  };

static NTSTATUS STDCALL
CsrpCreateListenPort (IN     LPWSTR  Name,
		      IN OUT PHANDLE Port,
		      IN     PTHREAD_START_ROUTINE ListenThread)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES  PortAttributes;
	UNICODE_STRING     PortName;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	RtlInitUnicodeString (& PortName, Name);
	InitializeObjectAttributes (& PortAttributes,
				    & PortName,
				    0,
				    NULL,
				    NULL);
	Status = NtCreatePort ( Port,
				& PortAttributes,
				LPC_MAX_DATA_LENGTH, /* TODO: make caller set it*/
				LPC_MAX_MESSAGE_LENGTH, /* TODO: make caller set it*/
				0); /* TODO: make caller set it*/
	if(!NT_SUCCESS(Status))
	{
		DPRINT1("CSR: %s: NtCreatePort failed (Status=%08lx)\n",
			__FUNCTION__, Status);
		return Status;
	}
	Status = RtlCreateUserThread(NtCurrentProcess(),
                               NULL,
                               FALSE,
                               0,
                               0,
                               0,
                               (PTHREAD_START_ROUTINE) ListenThread,
                               *Port,
                               NULL,
                               NULL);
	return Status;
}

/* === INIT ROUTINES === */

/**********************************************************************
 * CsrpCreateBNODirectory/3
 *
 * These used to be part of kernel32 startup, but that clearly wasn't a good
 * idea, as races were definately possible.  These are moved (as in the
 * previous fixmes).
 */
static NTSTATUS
CsrpCreateBNODirectory (int argc, char ** argv, char ** envp)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\BaseNamedObjects");
    UNICODE_STRING SymName = RTL_CONSTANT_STRING(L"Local");
    UNICODE_STRING SymName2 = RTL_CONSTANT_STRING(L"Global");
    HANDLE DirHandle, SymHandle;

    /* Seems like a good place to create these objects which are needed by
     * win32 processes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateDirectoryObject(&DirHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateDirectoryObject() failed %08x\n", Status);
    }

    /* Create the "local" Symbolic Link.
     * FIXME: CSR should do this -- Fixed */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SymName,
                               OBJ_CASE_INSENSITIVE,
                               DirHandle,
                               NULL);
    Status = NtCreateSymbolicLinkObject(&SymHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateDirectoryObject() failed %08x\n", Status);
    }

    /* Create the "global" Symbolic Link. */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SymName2,
                               OBJ_CASE_INSENSITIVE,
                               DirHandle,
                               NULL);
    Status = NtCreateSymbolicLinkObject(&SymHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateDirectoryObject() failed %08x\n", Status);
    }

    return Status;
}

/**********************************************************************
 * CsrpCreateHeap/3
 */
static NTSTATUS
CsrpCreateHeap (int argc, char ** argv, char ** envp)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

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
 * CsrpCreateCallbackPort/3
 */
static NTSTATUS
CsrpCreateCallbackPort (int argc, char ** argv, char ** envp)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	return CsrpCreateListenPort (L"\\Windows\\SbApiPort",
				     & hSbApiPort,
				     ServerSbApiPortThread);
}

/**********************************************************************
 * CsrpRegisterSubsystem/3
 */
static NTSTATUS
CsrpRegisterSubsystem (int argc, char ** argv, char ** envp)
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

/**********************************************************************
 * 	EnvpToUnicodeString/2
 */
static ULONG FASTCALL
EnvpToUnicodeString (char ** envp, PUNICODE_STRING UnicodeEnv)
{
	ULONG        CharCount = 0;
	ULONG        Index = 0;
	ANSI_STRING  AnsiEnv;

	UnicodeEnv->Buffer = NULL;

	for (Index=0; NULL != envp[Index]; Index++)
	{
		CharCount += strlen (envp[Index]);
		++ CharCount;
	}
	++ CharCount;

	AnsiEnv.Buffer = RtlAllocateHeap (RtlGetProcessHeap(), 0, CharCount);
	if (NULL != AnsiEnv.Buffer)
	{

		PCHAR WritePos = AnsiEnv.Buffer;

		for (Index=0; NULL != envp[Index]; Index++)
		{
			strcpy (WritePos, envp[Index]);
			WritePos += strlen (envp[Index]) + 1;
		}

      /* FIXME: the last (double) nullterm should perhaps not be included in Length
       * but only in MaximumLength. -Gunnar */
		AnsiEnv.Buffer [CharCount-1] = '\0';
		AnsiEnv.Length             = CharCount;
		AnsiEnv.MaximumLength      = CharCount;

		RtlAnsiStringToUnicodeString (UnicodeEnv, & AnsiEnv, TRUE);
		RtlFreeHeap (RtlGetProcessHeap(), 0, AnsiEnv.Buffer);
	}
	return CharCount;
}
/**********************************************************************
 * 	CsrpLoadKernelModeDriver/3
 */
static NTSTATUS
CsrpLoadKernelModeDriver (int argc, char ** argv, char ** envp)
{
	NTSTATUS        Status = STATUS_SUCCESS;
	WCHAR           Data [MAX_PATH + 1];
	ULONG           DataLength = sizeof Data;
	ULONG           DataType = 0;
	UNICODE_STRING  Environment;


	DPRINT("SM: %s called\n", __FUNCTION__);


	EnvpToUnicodeString (envp, & Environment);
	Status = SmLookupSubsystem (L"Kmode",
				    Data,
				    & DataLength,
				    & DataType,
				    Environment.Buffer);
	RtlFreeUnicodeString (& Environment);
	if((STATUS_SUCCESS == Status) && (DataLength > sizeof Data[0]))
	{
		WCHAR                      ImagePath [MAX_PATH + 1] = {0};
		UNICODE_STRING             ModuleName;

		wcscpy (ImagePath, L"\\??\\");
		wcscat (ImagePath, Data);
		RtlInitUnicodeString (& ModuleName, ImagePath);
		Status = NtSetSystemInformation(/* FIXME: SystemLoadAndCallImage */
		                                SystemExtendServiceTableInformation,
						& ModuleName,
						sizeof ModuleName);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("WIN: %s: loading Kmode failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		}
	}
	return Status;
}

/**********************************************************************
 * CsrpCreateApiPort/2
 */
static NTSTATUS
CsrpCreateApiPort (int argc, char ** argv, char ** envp)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	CsrInitProcessData();

	return CsrpCreateListenPort(L"\\Windows\\ApiPort", &hApiPort,
		(PTHREAD_START_ROUTINE)ClientConnectionThread);
}

/**********************************************************************
 * CsrpApiRegisterDef/0
 */
static NTSTATUS
CsrpApiRegisterDef (int argc, char ** argv, char ** envp)
{
	return CsrApiRegisterDefinitions(NativeDefinitions);
}

/**********************************************************************
 * CsrpCCTS/2
 */
static NTSTATUS
CsrpCCTS (int argc, char ** argv, char ** envp)
{
    ULONG Dummy;
    ULONG DummyLength = sizeof(Dummy);
	return CsrClientConnectToServer(L"\\Windows",
			0, &Dummy, &DummyLength, NULL);
}

/**********************************************************************
 * CsrpRunWinlogon/0
 *
 * Start the logon process (winlogon.exe).
 *
 * TODO: this should be moved in CsrpCreateSession/x (one per session)
 * TODO: in its own desktop (one logon desktop per winstation).
 */
static NTSTATUS
CsrpRunWinlogon (int argc, char ** argv, char ** envp)
{
	NTSTATUS                      Status = STATUS_SUCCESS;
	UNICODE_STRING                ImagePath;
	UNICODE_STRING                CommandLine;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters = NULL;
	RTL_USER_PROCESS_INFORMATION  ProcessInfo;


	DPRINT("CSR: %s called\n", __FUNCTION__);

	/* initialize the process parameters */
	RtlInitUnicodeString (& ImagePath, L"\\SystemRoot\\system32\\winlogon.exe");
	RtlInitUnicodeString (& CommandLine, L"");
	RtlCreateProcessParameters(& ProcessParameters,
				   & ImagePath,
				   NULL,
				   NULL,
				   & CommandLine,
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   NULL);
	/* Create the winlogon process */
	Status = RtlCreateUserProcess (& ImagePath,
				       OBJ_CASE_INSENSITIVE,
				       ProcessParameters,
				       NULL,
				       NULL,
				       NULL,
				       FALSE,
				       NULL,
				       NULL,
				       & ProcessInfo);
	/* Cleanup */
	RtlDestroyProcessParameters (ProcessParameters);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("SM: %s: loading winlogon.exe failed (Status=%08lx)\n",
				__FUNCTION__, Status);
	}
   ZwResumeThread(ProcessInfo.ThreadHandle, NULL);
	return Status;
}

static NTSTATUS
CsrpCreateHardErrorPort (int argc, char ** argv, char ** envp)
{
    return NtSetDefaultHardErrorPort(hApiPort);
}

typedef NTSTATUS (* CSR_INIT_ROUTINE)(int,char**,char**);

struct {
	BOOL Required;
	CSR_INIT_ROUTINE EntryPoint;
	PCHAR ErrorMessage;
} InitRoutine [] = {
        {TRUE, CsrpCreateBNODirectory,   "create base named objects directory"},
	{TRUE, CsrpCreateCallbackPort,   "create the callback port \\Windows\\SbApiPort"},
	{TRUE, CsrpRegisterSubsystem,    "register with SM"},
	{TRUE, CsrpCreateHeap,           "create the CSR heap"},
	{TRUE, CsrpCreateApiPort,        "create the api port \\Windows\\ApiPort"},
    {TRUE, CsrpCreateHardErrorPort,  "create the hard error port"},
	{TRUE, CsrpCreateObjectDirectory,"create the object directory \\Windows"},
	{TRUE, CsrpLoadKernelModeDriver, "load Kmode driver"},
	{TRUE, CsrpInitVideo,            "initialize video"},
	{TRUE, CsrpApiRegisterDef,       "initialize api definitions"},
	{TRUE, CsrpCCTS,                 "connect client to server"},
	{TRUE, CsrpInitWin32Csr,         "load usermode dll"},
	{TRUE, CsrpRunWinlogon,          "run WinLogon"},
};

/**********************************************************************
 * NAME
 * 	CsrServerInitialization
 *
 * DESCRIPTION
 * 	Initialize the Win32 environment subsystem server.
 *
 * RETURN VALUE
 * 	TRUE: Initialization OK; otherwise FALSE.
 */
BOOL STDCALL
CsrServerInitialization (
	int argc,
	char ** argv,
	char ** envp
	)
{
	UINT       i = 0;
	NTSTATUS  Status = STATUS_SUCCESS;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	for (i=0; i < (sizeof InitRoutine / sizeof InitRoutine[0]); i++)
	{
		Status = InitRoutine[i].EntryPoint(argc,argv,envp);
		if(!NT_SUCCESS(Status))
		{
			DPRINT1("CSR: %s: failed to %s (Status=%08lx)\n",
				__FUNCTION__,
				InitRoutine[i].ErrorMessage,
				Status);
			if (InitRoutine[i].Required)
			{
				return FALSE;
			}
		}
	}
	if (CallInitComplete())
	{
		Status = SmCompleteSession (hSmApiPort,hSbApiPort,hApiPort);
		return TRUE;
	}
	return FALSE;
}

/* EOF */
