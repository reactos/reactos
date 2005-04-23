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

#include <csrss/csrss.h>
#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <win32k/win32k.h>
#include <rosrtl/string.h>
#include <sm/helper.h>

#include "api.h"
#include "csrplugin.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE CsrHeap = (HANDLE) 0;

HANDLE CsrObjectDirectory = (HANDLE) 0;

UNICODE_STRING CsrDirectoryName;

extern HANDLE CsrssApiHeap;

static unsigned InitCompleteProcCount;
static CSRPLUGIN_INIT_COMPLETE_PROC *InitCompleteProcs = NULL;

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

ULONG
InitializeVideoAddressSpace(VOID);

/**********************************************************************
 * CsrpParseCommandLine/2
 */
static NTSTATUS
CsrpParseCommandLine (
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Attributes;

  DPRINT("CSR: %s called\n", __FUNCTION__);


   /*   DbgPrint ("Arguments: %ld\n", ArgumentCount);
   for (i = 0; i < ArgumentCount; i++)
     {
	DbgPrint ("Argument %ld: %S\n", i, ArgumentArray[i]);
	}*/


	/* create object directory ('\Windows') */
	RtlCreateUnicodeString (&CsrDirectoryName,
	                        L"\\Windows");

	InitializeObjectAttributes (&Attributes,
	                            &CsrDirectoryName,
	                            OBJ_OPENIF,
	                            NULL,
	                            NULL);

	Status = NtOpenDirectoryObject(&CsrObjectDirectory,
	                               0xF000F, /* ea:??? */
	                               &Attributes);
	return Status;
}

/**********************************************************************
 * CsrpInitVideo/0
 *
 * TODO: we need a virtual device for sessions other than
 * TODO: the console one
 */
static NTSTATUS
CsrpInitVideo (ULONG argc, PWSTR* argv)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  IO_STATUS_BLOCK Iosb;
  HANDLE VideoHandle = (HANDLE) 0;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("CSR: %s called\n", __FUNCTION__);

  InitializeVideoAddressSpace();

  RtlRosInitUnicodeStringFromLiteral(&DeviceName, L"\\??\\DISPLAY1");
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
 * CsrpInitWin32Csr/0
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
CsrpInitWin32Csr (ULONG argc, PWSTR* argv)
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
  if (! (*InitProc)(&ApiDefinitions, &ObjectDefinitions, &InitCompleteProc,
                    &Exports, CsrssApiHeap))
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

  return Status;
}

CSRSS_API_DEFINITION NativeDefinitions[] =
  {
    CSRSS_DEFINE_API(CSRSS_CREATE_PROCESS,               CsrCreateProcess),
    CSRSS_DEFINE_API(CSRSS_TERMINATE_PROCESS,            CsrTerminateProcess),
    CSRSS_DEFINE_API(CSRSS_CONNECT_PROCESS,              CsrConnectProcess),
    CSRSS_DEFINE_API(CSRSS_REGISTER_SERVICES_PROCESS,    CsrRegisterServicesProcess),
    CSRSS_DEFINE_API(CSRSS_GET_SHUTDOWN_PARAMETERS,      CsrGetShutdownParameters),
    CSRSS_DEFINE_API(CSRSS_SET_SHUTDOWN_PARAMETERS,      CsrSetShutdownParameters),
    CSRSS_DEFINE_API(CSRSS_GET_INPUT_HANDLE,             CsrGetInputHandle),
    CSRSS_DEFINE_API(CSRSS_GET_OUTPUT_HANDLE,            CsrGetOutputHandle),
    CSRSS_DEFINE_API(CSRSS_CLOSE_HANDLE,                 CsrCloseHandle),
    CSRSS_DEFINE_API(CSRSS_VERIFY_HANDLE,                CsrVerifyHandle),
    CSRSS_DEFINE_API(CSRSS_DUPLICATE_HANDLE,             CsrDuplicateHandle),
    CSRSS_DEFINE_API(CSRSS_GET_INPUT_WAIT_HANDLE,        CsrGetInputWaitHandle),
    { 0, 0, 0, NULL }
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
				260, /* TODO: make caller set it*/
				328, /* TODO: make caller set it*/
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
                               NULL,
                               NULL,
                               (PTHREAD_START_ROUTINE) ListenThread,
                               Port,
                               NULL,
                               NULL);
	return Status;
}

/* === INIT ROUTINES === */

/**********************************************************************
 * CsrpCreateCallbackPort/0
 */
static NTSTATUS
CsrpCreateHeap (ULONG argc, PWSTR* argv)
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
 * CsrpCreateCallbackPort/0
 */
static NTSTATUS
CsrpCreateCallbackPort (ULONG argc, PWSTR* argv)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	return CsrpCreateListenPort (L"\\Windows\\SbApiPort",
				     & hSbApiPort,
				     ServerSbApiPortThread);
}

/**********************************************************************
 * CsrpRegisterSubsystem/2
 */
static NTSTATUS
CsrpRegisterSubsystem (ULONG argc, PWSTR* argv)
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
 * CsrpCreateApiPort/0
 */
static NTSTATUS
CsrpCreateApiPort (ULONG argc, PWSTR* argv)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	return CsrpCreateListenPort (L"\\Windows\\ApiPort",
				     & hApiPort,
				     ServerApiPortThread);
}

/**********************************************************************
 * CsrpApiRegisterDef/0
 */
static NTSTATUS
CsrpApiRegisterDef (ULONG argc, PWSTR* argv)
{
	return CsrApiRegisterDefinitions(NativeDefinitions);
}

/**********************************************************************
 * CsrpCCTS/2
 */
static NTSTATUS
CsrpCCTS (ULONG argc, PWSTR* argv)
{
	return CsrClientConnectToServer();
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
CsrpRunWinlogon (ULONG argc, PWSTR* argv)
{
	NTSTATUS                      Status = STATUS_SUCCESS;
	UNICODE_STRING                ImagePath;
	UNICODE_STRING                CommandLine;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters = NULL;
	RTL_PROCESS_INFO              ProcessInfo;


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



typedef NTSTATUS (* CSR_INIT_ROUTINE)(ULONG, PWSTR*);

struct {
	BOOL Required;
	CSR_INIT_ROUTINE EntryPoint;
	PCHAR ErrorMessage;
} InitRoutine [] = {
	{TRUE, CsrpCreateCallbackPort, "create the callback port \\Windows\\SbApiPort"},
	{TRUE, CsrpRegisterSubsystem,  "register with SM"},
	{TRUE, CsrpCreateHeap,         "create the CSR heap"},
	{TRUE, CsrpCreateApiPort,      "create the api port \\Windows\\ApiPort"},
	{TRUE, CsrpParseCommandLine,   "parse the command line"},
	{TRUE, CsrpInitVideo,          "initialize video"},
	{TRUE, CsrpApiRegisterDef,     "initialize api definitions"},
	{TRUE, CsrpCCTS,               "connect client to server"},
	{TRUE, CsrpInitWin32Csr,       "load usermode dll"},
	{TRUE, CsrpRunWinlogon,        "run WinLogon"},
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
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
	INT       i = 0;
	NTSTATUS  Status = STATUS_SUCCESS;

	DPRINT("CSR: %s called\n", __FUNCTION__);

	for (i=0; i < (sizeof InitRoutine / sizeof InitRoutine[0]); i++)
	{
		Status = InitRoutine[i].EntryPoint(ArgumentCount,ArgumentArray);
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
