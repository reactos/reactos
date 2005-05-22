/* $Id$
 *
 * initss.c - Load the subsystems
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
 */


#include "smss.h"

#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

/* SM handle for its own \SmApiPort */
HANDLE hSmApiPort = (HANDLE) 0;


/* TODO:
 *
 * a) look if a special option is set for smss.exe in
 *    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options
 */

/**********************************************************************
 *	SmpRegisterSmss/0
 *
 * DESCRIPTION
 *	Make smss register with itself for IMAGE_SUBSYSTEM_NATIVE
 *	(programmatically). This also opens hSmApiPort to be used
 *	in loading required subsystems.
 */

static NTSTATUS
SmpRegisterSmss(VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;
	RTL_PROCESS_INFO ProcessInfo;

	
	DPRINT("SM: %s called\n",__FUNCTION__);
	
	RtlZeroMemory (& ProcessInfo, sizeof ProcessInfo);
	ProcessInfo.Size = sizeof ProcessInfo;
	ProcessInfo.ProcessHandle = (HANDLE) SmSsProcessId;
	ProcessInfo.ClientId.UniqueProcess = (HANDLE) SmSsProcessId;
	DPRINT("SM: %s: ProcessInfo.ProcessHandle=%lx\n",
		__FUNCTION__,ProcessInfo.ProcessHandle);
	Status = SmCreateClient (& ProcessInfo, L"Session Manager");
	if (NT_SUCCESS(Status))
	{
		UNICODE_STRING SbApiPortName = {0,0,NULL};
		
		RtlInitUnicodeString (& SbApiPortName, L"");	
		Status = SmConnectApiPort(& SbApiPortName,
					  (HANDLE) -1, /* SM has no SB port */
					  IMAGE_SUBSYSTEM_NATIVE,
					  & hSmApiPort);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("SM: %s: SMLIB!SmConnectApiPort failed (Status=0x%08lx)\n",
				__FUNCTION__,Status);
			return Status;
		}
		DPRINT("SM self registered\n");
	}
	else
	{
		DPRINT1("SM: %s: SmCreateClient failed (Status=0x%08lx)\n",
			__FUNCTION__, Status);
	}
	/*
	 * Note that you don't need to call complete session
	 * because connection handling code autocompletes
	 * the client structure for IMAGE_SUBSYSTEM_NATIVE.
	 */
	return Status;
}


/**********************************************************************
 * 	SmpLoadKernelModeSubsystem/0
 */
static NTSTATUS
SmpLoadKernelModeSubsystem (VOID)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	WCHAR     Data [MAX_PATH + 1];
	ULONG     DataLength = sizeof Data;
	ULONG     DataType = 0;


	DPRINT("SM: %s called\n", __FUNCTION__);

	Status = SmLookupSubsystem (L"Kmode",
				    Data,
				    & DataLength,
				    & DataType,
				    TRUE);
	if((STATUS_SUCCESS == Status) && (DataLength > sizeof Data[0]))
	{
		WCHAR                      ImagePath [MAX_PATH + 1] = {0};
		SYSTEM_LOAD_AND_CALL_IMAGE ImageInfo;

		wcscpy (ImagePath, L"\\??\\");
		wcscat (ImagePath, Data);
		RtlZeroMemory (& ImageInfo, sizeof ImageInfo);
		RtlInitUnicodeString (& ImageInfo.ModuleName, ImagePath);
		Status = NtSetSystemInformation(SystemLoadAndCallImage,
						& ImageInfo,
						sizeof ImageInfo);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("SM: %s: loading Kmode failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
		}
	}
	return Status;
}

/**********************************************************************
 * 	SmpLoadRequiredSubsystems/0
 */
static NTSTATUS
SmpLoadRequiredSubsystems (VOID)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	WCHAR     Data [MAX_PATH + 1];
	ULONG     DataLength = sizeof Data;
	ULONG     DataType = 0;

	
	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlZeroMemory (Data, DataLength);
	Status = SmLookupSubsystem (L"Required",
				    Data,
				    & DataLength,
				    & DataType,
				    FALSE);
	if((STATUS_SUCCESS == Status) && (DataLength > sizeof Data[0]))
	{
		PWCHAR Name = NULL;
		ULONG Offset = 0;
		
		for (Name = Data; (Offset < DataLength); )
		{
			if(L'\0' != *Name)
			{
				UNICODE_STRING Program;

				/* Run the current program */
				RtlInitUnicodeString (& Program, Name);
				Status = SmExecuteProgram (hSmApiPort, & Program);
				if(!NT_SUCCESS(Status))
				{
					DPRINT1("SM: %s failed to run '%S' program (Status=0x%08lx)\n",
						__FUNCTION__, Name, Status);
				}
				/* Look for the next program */
				while ((L'\0' != *Name) && (Offset < DataLength))
				{
					++ Name;
					++ Offset;
				}
			}
			++ Name;
			++ Offset;
		}
	}

	return Status;
}

/**********************************************************************
 * 	SmLoadSubsystems/0
 */
NTSTATUS
SmLoadSubsystems(VOID)
{
	NTSTATUS  Status = STATUS_SUCCESS;

	
	DPRINT("SM: loading subsystems\n");

	/* SM self registers */
	Status = SmpRegisterSmss();
	if(!NT_SUCCESS(Status)) return Status;
	/* Load Kmode subsystem (aka win32k.sys) */
	Status = SmpLoadKernelModeSubsystem();
	if(!NT_SUCCESS(Status)) return Status;
	/* Load Required subsystems (Debug Windows) */
	Status = SmpLoadRequiredSubsystems();
	if(!NT_SUCCESS(Status)) return Status;
	/* done */
	return Status;
}

/* EOF */
