/* $Id: dllmain.c,v 1.16 2001/01/20 12:19:57 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/dllmain.c
 * PURPOSE:         Initialization 
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <ntdll/ldr.h>
#include <napi/shared_data.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>


extern UNICODE_STRING SystemDirectory;
extern UNICODE_STRING WindowsDirectory;

HANDLE hProcessHeap = NULL;

static WINBOOL DllInitialized = FALSE;

WINBOOL STDCALL DllMain (HANDLE hInst,
			 ULONG ul_reason_for_call,
			 LPVOID lpReserved);


BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
   return(DllMain(hDll,dwReason,lpReserved));
}


WINBOOL STDCALL DllMain(HANDLE hInst,
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
	     PKUSER_SHARED_DATA SharedUserData = 
		(PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE;

	     DPRINT("DLL_PROCESS_ATTACH\n");

	     LdrDisableThreadCalloutsForDll ((PVOID)hInst);

	     /*
	      * Connect to the csrss server
	      */
	     Status = CsrClientConnectToServer();
	     if (!NT_SUCCESS(Status))
	       {
		  DbgPrint("Failed to connect to csrss.exe: expect trouble\n");
		  //	ZwTerminateProcess(NtCurrentProcess(), Status);
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

	     /* Insert more dll attach stuff here! */

	     DllInitialized = TRUE;

	     break;
	  }
      case DLL_PROCESS_DETACH:
	  {
	     DPRINT("DLL_PROCESS_DETACH\n");
	     if (DllInitialized == TRUE)
	       {
		  RtlFreeUnicodeString (&SystemDirectory);
		  RtlFreeUnicodeString (&WindowsDirectory);

		  /* Insert more dll detach stuff here! */

	       }
	     break;
	  }
      default:
	break;
    }
   return TRUE;
}

/* EOF */
