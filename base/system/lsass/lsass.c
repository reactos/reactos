/*
 * reactos/subsys/system/lsass/lsass.c
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 *
 * 	19990704 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <lsass/lsasrv.h>
//#include <samsrv.h>
#include <lsass/lsass.h>

#define NDEBUG
#include <debug.h>

static VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv);

static SERVICE_TABLE_ENTRY ServiceTable[2] =
{
	{TEXT("NetLogon"), ServiceMain},
	{NULL, NULL}
};

static VOID CALLBACK
ServiceMain(
	IN DWORD argc,
	IN LPWSTR *argv)
{
	DPRINT("ServiceMain() called\n");
}

INT WINAPI
wWinMain(
	IN HINSTANCE hInstance,
	IN HINSTANCE hPrevInstance,
	IN LPWSTR lpCmdLine,
	IN INT nShowCmd)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("Local Security Authority Subsystem\n");
	DPRINT("  Initializing...\n");

	/* Initialize the LSA server DLL. */
	Status = LsapInitLsa();
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("LsapInitLsa() failed (Status 0x%08lX)\n", Status);
		goto ByeBye;
	}

#if 0
	/* Initialize the SAM server DLL. */
	Status = SamIInitialize();
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("SamIInitialize() failed (Status 0x%08lX)\n", Status);
		goto ByeBye;
	}
#endif

	/* FIXME: More initialization */

	StartServiceCtrlDispatcher(ServiceTable);

	DPRINT("  Done...\n");

ByeBye:
	NtTerminateThread(NtCurrentThread(), Status);

	return 0;
}

/* EOF */
