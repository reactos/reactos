/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Ext2 File System Management
 * FILE:            dll/win32/uext2/uext2.c
 * PURPOSE:         uext2 DLL initialisation
 *
 * PROGRAMMERS:     Pierre Schweitzer
 */

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

INT WINAPI
DllMain(
	IN HINSTANCE hinstDLL,
	IN DWORD     dwReason,
	IN LPVOID    lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(dwReason);
	UNREFERENCED_PARAMETER(lpvReserved);

	return TRUE;
}
