/* $Id: dllmain.c,v 1.24 2002/11/14 18:21:05 chorns Exp $
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

#define NTOS_MODE_USER
#include <ntos.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS *******************************************************************/

extern UNICODE_STRING SystemDirectory;
extern UNICODE_STRING WindowsDirectory;

HANDLE hProcessHeap = NULL;
HANDLE hBaseDir = NULL;

static WINBOOL DllInitialized = FALSE;

WINBOOL STDCALL DllMain (HANDLE hInst,
			 ULONG ul_reason_for_call,
			 LPVOID lpReserved);

/* Critical section for various kernel32 data structures */
CRITICAL_SECTION DllLock;

/* FUNCTIONS *****************************************************************/

static NTSTATUS 
OpenBaseDirectory(PHANDLE DirHandle)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING Name = UNICODE_STRING_INITIALIZER(L"\\BaseNamedObjects");
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes,
			      &Name,
			      OBJ_PERMANENT,
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


BOOL WINAPI
DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
   return(DllMain(hDll,dwReason,lpReserved));
}

WINBOOL STDCALL
DllMain(HANDLE hInst,
	ULONG ul_reason_for_call,
	LPVOID lpReserved)
{
   DPRINT("DllMain(hInst %x, ul_reason_for_call %d)\n",
	  hInst, ul_reason_for_call);

   switch (ul_reason_for_call)
     {
      case DLL_PROCESS_ATTACH:
	  {
	     NTSTATUS Status;

	     DPRINT("DLL_PROCESS_ATTACH\n");

	     LdrDisableThreadCalloutsForDll ((PVOID)hInst);

	     /*
	      * Connect to the csrss server
	      */
	     Status = CsrClientConnectToServer();
	     if (!NT_SUCCESS(Status))
	       {
		  DbgPrint("Failed to connect to csrss.exe: expect trouble "
			   "Status was %X\n", Status);
		  ZwTerminateProcess(NtCurrentProcess(), Status);
	       }

	     hProcessHeap = RtlGetProcessHeap();

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
		  DbgPrint("Failed to open object base directory: expect trouble\n");
	       }

	     /* Initialize the DLL critical section */
	     RtlInitializeCriticalSection(&DllLock);
	     
	     /* Insert more dll attach stuff here! */

	     DllInitialized = TRUE;

	     break;
	  }
      case DLL_PROCESS_DETACH:
	  {
	     DPRINT("DLL_PROCESS_DETACH\n");
	     if (DllInitialized == TRUE)
	       {
		 /* Insert more dll detach stuff here! */
		 
		 /* Delete DLL critical section */
		 RtlDeleteCriticalSection (&DllLock);

		 /* Close object base directory */
		 NtClose(hBaseDir);
		 
		 RtlFreeUnicodeString (&SystemDirectory);
		 RtlFreeUnicodeString (&WindowsDirectory);
	       }
	     break;
	  }
      default:
	break;
    }
   return TRUE;
}

/* EOF */
