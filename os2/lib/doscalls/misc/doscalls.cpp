/* $Id: doscalls.cpp,v 1.1 2002/07/26 00:23:13 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/doscalls.c
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *    13-03-2002  Created
 */


// here's only the NTDLL needet
#include <ddk/ntddk.h>

 


BOOL STDCALL DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case 1://DLL_PROCESS_ATTACH:
	case 2://DLL_THREAD_ATTACH:
	case 3://DLL_THREAD_DETACH:
	case 0://DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

