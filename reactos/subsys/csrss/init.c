/* $Id: init.c,v 1.28 2004/07/03 17:15:02 hbirr Exp $
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

#include "api.h"
#include "csrplugin.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE CsrInitEvent = INVALID_HANDLE_VALUE;
HANDLE CsrHeap = INVALID_HANDLE_VALUE;

HANDLE CsrObjectDirectory = INVALID_HANDLE_VALUE;

UNICODE_STRING CsrDirectoryName;

extern HANDLE CsrssApiHeap;

static unsigned InitCompleteProcCount;
static CSRPLUGIN_INIT_COMPLETE_PROC *InitCompleteProcs = NULL;

static NTSTATUS FASTCALL
AddInitCompleteProc(CSRPLUGIN_INIT_COMPLETE_PROC Proc)
{
  CSRPLUGIN_INIT_COMPLETE_PROC *NewProcs;

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

static BOOL FASTCALL
CallInitComplete(void)
{
  BOOL Ok;
  unsigned i;

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

static NTSTATUS
CsrParseCommandLine (
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Attributes;


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
	                            0,
	                            NULL,
	                            NULL);

	Status = NtCreateDirectoryObject(&CsrObjectDirectory,
	                                 0xF000F,
	                                 &Attributes);

	return Status;
}


static VOID
CsrInitVideo(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  IO_STATUS_BLOCK Iosb;
  HANDLE VideoHandle;
  NTSTATUS Status;

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
}

static NTSTATUS FASTCALL
InitWin32Csr()
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
      Status = AddInitCompleteProc(InitCompleteProc);
    }

  return Status;
}

CSRSS_API_DEFINITION NativeDefinitions[] =
  {
    CSRSS_DEFINE_API(CSRSS_CREATE_PROCESS,               CsrCreateProcess),
    CSRSS_DEFINE_API(CSRSS_TERMINATE_PROCESS,            CsrTerminateProcess),
    CSRSS_DEFINE_API(CSRSS_CONNECT_PROCESS,              CsrConnectProcess),
    CSRSS_DEFINE_API(CSRSS_REGISTER_SERVICES_PROCESS,    CsrRegisterServicesProcess),
    CSRSS_DEFINE_API(CSRSS_EXIT_REACTOS,                 CsrExitReactos),
    CSRSS_DEFINE_API(CSRSS_GET_SHUTDOWN_PARAMETERS,      CsrGetShutdownParameters),
    CSRSS_DEFINE_API(CSRSS_SET_SHUTDOWN_PARAMETERS,      CsrSetShutdownParameters),
    CSRSS_DEFINE_API(CSRSS_GET_INPUT_HANDLE,             CsrGetInputHandle),
    CSRSS_DEFINE_API(CSRSS_GET_OUTPUT_HANDLE,            CsrGetOutputHandle),
    CSRSS_DEFINE_API(CSRSS_CLOSE_HANDLE,                 CsrCloseHandle),
    CSRSS_DEFINE_API(CSRSS_VERIFY_HANDLE,                CsrVerifyHandle),
    CSRSS_DEFINE_API(CSRSS_DUPLICATE_HANDLE,             CsrDuplicateHandle),
    { 0, 0, 0, NULL }
  };


/**********************************************************************
 * NAME
 * 	CsrServerInitialization
 *
 * DESCRIPTION
 * 	Create a directory object (\windows) and a named LPC port
 * 	(\windows\ApiPort)
 *
 * RETURN VALUE
 * 	TRUE: Initialization OK; otherwise FALSE.
 */
BOOL
STDCALL
CsrServerInitialization (
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObAttributes;
  UNICODE_STRING PortName;
  HANDLE ApiPortHandle;

  Status = CsrParseCommandLine (ArgumentCount, ArgumentArray);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("CSR: Unable to parse the command line (Status: %x)\n", Status);
      return FALSE;
    }

  CsrInitVideo();

  CsrssApiHeap = RtlCreateHeap(HEAP_GROWABLE,
                               NULL,
                               65536,
                               65536,
                               NULL,
                               NULL);
  if (CsrssApiHeap == NULL)
    {
      DPRINT1("CSR: Failed to create private heap, aborting\n");
      return FALSE;
    }

  Status = CsrApiRegisterDefinitions(NativeDefinitions);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  /* NEW NAMED PORT: \ApiPort */
  RtlRosInitUnicodeStringFromLiteral(&PortName, L"\\Windows\\ApiPort");
  InitializeObjectAttributes(&ObAttributes,
                             &PortName,
                             0,
                             NULL,
                             NULL);
  Status = NtCreatePort(&ApiPortHandle,
                        &ObAttributes,
                        260,
                        328,
                        0);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("CSR: Unable to create \\ApiPort (Status %x)\n", Status);
      return FALSE;
    }
  Status = RtlCreateUserThread(NtCurrentProcess(),
                               NULL,
                               FALSE,
                               0,
                               NULL,
                               NULL,
                               (PTHREAD_START_ROUTINE)ServerApiPortThead,
                               ApiPortHandle,
                               NULL,
                               NULL);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("CSR: Unable to create server thread\n");
      NtClose(ApiPortHandle);
      return FALSE;
    }
  Status = CsrClientConnectToServer();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CsrClientConnectToServer() failed (Status %x)\n", Status);
      return FALSE;
    }
  Status = InitWin32Csr();
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("CSR: Unable to load usermode dll (Status %x)\n", Status);
      return FALSE;
    }

  return CallInitComplete();
}

/* EOF */
