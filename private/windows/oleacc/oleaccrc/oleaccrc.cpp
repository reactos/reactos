// Copyright (c) 1996-1999 Microsoft Corporation

/*
 *
 *  Minimal DllMain - to keep linker happy.
 *  DLL otherwise contains just resources.
 *
 *
 *  Note: set compiler/linker options to ignore
 *  default libraries, and set entry point symbol
 *  to DllMain.
 *
 */
#include <windows.h>

BOOL WINAPI DllMain( HANDLE hInst, 
                     ULONG ul_reason_for_call,
                     LPVOID lpReserved )
{
	return TRUE;
}
