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
#include <kernel32/proc.h>

WINBOOL   
STDCALL   
DllMain (
	HANDLE hInst, 
	ULONG ul_reason_for_call,
	LPVOID lpReserved );


NT_PEB Peb;



NT_PEB *CurrentPeb;
NT_TEB *Teb;
PROCESSINFOW ProcessInfo;



WINBOOL   
STDCALL   
DllMain (
	HANDLE hInst, 
	ULONG ul_reason_for_call,
	LPVOID lpReserved )
{
    switch( ul_reason_for_call ) {
    	case DLL_PROCESS_ATTACH:
	{
		CurrentPeb = &Peb;
		CurrentPeb->ProcessHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS,8192,0);
		InitAtomTable(13);
	}
    	case DLL_THREAD_ATTACH:
	{
		Teb = HeapAlloc(GetProcessHeap(),0,sizeof(NT_TEB));
		Teb->Peb = GetCurrentPeb();
		Teb->HardErrorMode = SEM_NOGPFAULTERRORBOX;
		Teb->dwTlsIndex=0;
		break;
	}
    	case DLL_PROCESS_DETACH:
	{
		HeapFree(GetProcessHeap(),0,Teb);
		HeapDestroy(GetCurrentPeb()->ProcessHeap);
		break;
	}
	case DLL_THREAD_DETACH:
	{
		HeapFree(GetProcessHeap(),0,Teb);
		break;
	}
	default:
	break;
    
    }
    return TRUE;

}



