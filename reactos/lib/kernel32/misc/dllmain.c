/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/dllmain.c
 * PURPOSE:         Initialization 
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>
#include <wchar.h>
#include <kernel32/proc.h>
#include <internal/teb.h>

#include <kernel32/kernel32.h>

WINBOOL STDCALL DllMain (HANDLE hInst, 
			 ULONG ul_reason_for_call,
			 LPVOID lpReserved);



BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
   return(DllMain(hDll,dwReason,lpReserved));
}

VOID WINAPI __HeapInit(LPVOID base, ULONG minsize, ULONG maxsize);

WINBOOL STDCALL DllMain(HANDLE hInst, 
			ULONG ul_reason_for_call,
			LPVOID lpReserved)
{
   DPRINT("DllMain");
   switch (ul_reason_for_call) 
     {
      case DLL_PROCESS_ATTACH:
	  {     
	     DPRINT("DLL_PROCESS_ATTACH\n");
	  }
    	case DLL_THREAD_ATTACH:
	  {
	     //		Teb = HeapAlloc(GetProcessHeap(),0,sizeof(NT_TEB));
	     //		Teb->Peb = GetCurrentPeb();
	     //		Teb->HardErrorMode = SEM_NOGPFAULTERRORBOX;
	     //		Teb->dwTlsIndex=0;
	     break;
	  }
      case DLL_PROCESS_DETACH:
	  {
//	     HeapFree(GetProcessHeap(),0,Teb);
	     HeapDestroy(NtCurrentPeb()->ProcessHeap);
	     break;
	  }
      case DLL_THREAD_DETACH:
	  {
	     //		HeapFree(GetProcessHeap(),0,Teb);
	     break;
	  }
      default:
	break;
	
    }
   return TRUE;  
}



