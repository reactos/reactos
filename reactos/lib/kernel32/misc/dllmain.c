/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* GLOBALS *******************************************************************/

extern UNICODE_STRING SystemDirectory;
extern UNICODE_STRING WindowsDirectory;

HANDLE hProcessHeap = NULL;
HMODULE hCurrentModule = NULL;
HANDLE hBaseDir = NULL;

static BOOL DllInitialized = FALSE;

BOOL STDCALL
DllMain(HANDLE hInst,
	DWORD dwReason,
	LPVOID lpReserved);

/* Critical section for various kernel32 data structures */
RTL_CRITICAL_SECTION DllLock;
RTL_CRITICAL_SECTION ConsoleLock;

extern BOOL WINAPI DefaultConsoleCtrlHandler(DWORD Event);
extern __declspec(noreturn) VOID CALLBACK ConsoleControlDispatcher(DWORD CodeAndFlag);

extern BOOL FASTCALL NlsInit();
extern VOID FASTCALL NlsUninit();

HANDLE
STDCALL
DuplicateConsoleHandle(HANDLE hConsole,
                       DWORD dwDesiredAccess,
                       BOOL	bInheritHandle,
                       DWORD dwOptions);

/* FUNCTIONS *****************************************************************/

static NTSTATUS
OpenBaseDirectory(PHANDLE DirHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\BaseNamedObjects");
  NTSTATUS Status;

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE|OBJ_PERMANENT,
			     NULL,
			     NULL);

  Status = NtOpenDirectoryObject(DirHandle,
				 DIRECTORY_ALL_ACCESS,
				 &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      Status = NtCreateDirectoryObject(DirHandle,
				       DIRECTORY_ALL_ACCESS,
				       &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("NtCreateDirectoryObject() failed\n");
	}

      return Status;
    }

  return STATUS_SUCCESS;
}

BOOL
STDCALL
BasepInitConsole(VOID)
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;

    WCHAR lpTest[MAX_PATH];
    GetModuleFileNameW(NULL, lpTest, MAX_PATH);
    DPRINT("BasepInitConsole for : %S\n", lpTest);
    DPRINT("Our current console handles are: %lx, %lx, %lx %lx\n", 
           Parameters->ConsoleHandle, Parameters->StandardInput, 
           Parameters->StandardOutput, Parameters->StandardError);

    /* We have nothing to do if this isn't a console app... */
    if (RtlImageNtHeader(GetModuleHandle(NULL))->OptionalHeader.Subsystem !=
        IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        DPRINT("Image is not a console application\n");
        Parameters->ConsoleHandle = NULL;
        return TRUE;
    }

    /* Assume one is needed */
    Request.Data.AllocConsoleRequest.ConsoleNeeded = TRUE;

    /* Handle the special flags given to us by BasepInitializeEnvironment */
    if (Parameters->ConsoleHandle == HANDLE_DETACHED_PROCESS)
    {
        /* No console to create */
        DPRINT("No console to create\n");
        Parameters->ConsoleHandle = NULL;
        Request.Data.AllocConsoleRequest.ConsoleNeeded = FALSE;
    }
    else if (Parameters->ConsoleHandle == HANDLE_CREATE_NEW_CONSOLE)
    {
        /* We'll get the real one soon */
        DPRINT("Creating new console\n");
        Parameters->ConsoleHandle = NULL;
    }
    else if (Parameters->ConsoleHandle == HANDLE_CREATE_NO_WINDOW)
    {
        /* We'll get the real one soon */
        DPRINT1("NOT SUPPORTED: HANDLE_CREATE_NO_WINDOW\n");
        Parameters->ConsoleHandle = NULL;
    }
    else
    {
        if (Parameters->ConsoleHandle == INVALID_HANDLE_VALUE)
        {
            Parameters->ConsoleHandle = 0;
        }
        DPRINT("Using existing console: %x\n", Parameters->ConsoleHandle);
    }

    /* Initialize Console Ctrl Handler */
    RtlInitializeCriticalSection(&ConsoleLock);
    SetConsoleCtrlHandler(DefaultConsoleCtrlHandler, TRUE);

    /* Now use the proper console handle */
    Request.Data.AllocConsoleRequest.Console = Parameters->ConsoleHandle;

    /*
     * Normally, we should be connecting to the Console CSR Server...
     * but we don't have one yet, so we will instead simply send a create
     * console message to the Base Server. When we finally have a Console
     * Server, this code should be changed to send connection data instead.
     *
     * Also note that this connection should be made for any console app, even
     * in the case above where -we- return.
     */
    CsrRequest = MAKE_CSR_API(ALLOC_CONSOLE, CSR_CONSOLE);
    Request.Data.AllocConsoleRequest.CtrlDispatcher = ConsoleControlDispatcher;
    Status = CsrClientCallServer(&Request, 
                                 NULL,
                                 CsrRequest,
                                 sizeof(CSR_API_MESSAGE));
    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        DPRINT1("CSR Failed to give us a console\n");
        /* We're lying here, so at least the process can load... */
        return TRUE;
    }

    /* We got the handles, let's set them */
    if ((Parameters->ConsoleHandle = Request.Data.AllocConsoleRequest.Console))
    {
        /* If we already had some, don't use the new ones */
        if (!Parameters->StandardInput)
        {
            Parameters->StandardInput = Request.Data.AllocConsoleRequest.InputHandle;
        }
        if (!Parameters->StandardOutput)
        {
            Parameters->StandardOutput = Request.Data.AllocConsoleRequest.OutputHandle;
        }
        if (!Parameters->StandardError)
        {
            Parameters->StandardError = Request.Data.AllocConsoleRequest.OutputHandle;
        }
    }

    DPRINT("Console setup: %lx, %lx, %lx, %lx\n", 
            Parameters->ConsoleHandle,
            Parameters->StandardInput,
            Parameters->StandardOutput,
            Parameters->StandardError);
    return TRUE;
}


BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  NTSTATUS Status;

  (void)lpReserved;

  DPRINT("DllMain(hInst %lx, dwReason %lu)\n",
	 hDll, dwReason);

  switch (dwReason)
    {
      case DLL_PROCESS_ATTACH:
	DPRINT("DLL_PROCESS_ATTACH\n");

	LdrDisableThreadCalloutsForDll ((PVOID)hDll);

	/*
	 * Connect to the csrss server
	 */
	Status = CsrClientConnectToServer(L"\\Windows\\ApiPort",
                                      0,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL);
	if (!NT_SUCCESS(Status))
	  {
	    DbgPrint("Failed to connect to csrss.exe (Status %lx)\n",
		     Status);
	    ZwTerminateProcess(NtCurrentProcess(), Status);
	    return FALSE;
	  }

	hProcessHeap = RtlGetProcessHeap();
   hCurrentModule = hDll;

	/*
	 * Initialize WindowsDirectory and SystemDirectory
	 */
	DPRINT("NtSystemRoot: %S\n",
	       SharedUserData->NtSystemRoot);
	RtlCreateUnicodeString (&WindowsDirectory,
				SharedUserData->NtSystemRoot);
	SystemDirectory.MaximumLength = WindowsDirectory.MaximumLength + 18;
	SystemDirectory.Length = WindowsDirectory.Length + 18;
	SystemDirectory.Buffer = RtlAllocateHeap (hProcessHeap,
						  0,
						  SystemDirectory.MaximumLength);
	wcscpy (SystemDirectory.Buffer, WindowsDirectory.Buffer);
	wcscat (SystemDirectory.Buffer, L"\\System32");

	/* Open object base directory */
	Status = OpenBaseDirectory(&hBaseDir);
	if (!NT_SUCCESS(Status))
	  {
	    DbgPrint("Failed to open object base directory (Status %lx)\n",
		     Status);
	    return FALSE;
	  }

	/* Initialize the DLL critical section */
	RtlInitializeCriticalSection(&DllLock);

	/* Initialize the National Language Support routines */
        if (! NlsInit())
          {
            return FALSE;
          }

    /* Initialize Console Support */
    if (!BasepInitConsole())
    {
        DPRINT1("Failure to set up console\n");
        return FALSE;
    }

   /* Insert more dll attach stuff here! */

	DllInitialized = TRUE;
	break;

      case DLL_PROCESS_DETACH:
	DPRINT("DLL_PROCESS_DETACH\n");
	if (DllInitialized == TRUE)
	  {
	    /* Insert more dll detach stuff here! */

            NlsUninit();

	    /* Delete DLL critical section */
	    RtlDeleteCriticalSection (&ConsoleLock);
	    RtlDeleteCriticalSection (&DllLock);

	    /* Close object base directory */
	    NtClose(hBaseDir);

	    RtlFreeUnicodeString (&SystemDirectory);
	    RtlFreeUnicodeString (&WindowsDirectory);
	  }
	break;

      default:
	break;
    }

   return TRUE;
}

/* EOF */
