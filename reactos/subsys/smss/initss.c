/* $Id: init.c 13449 2005-02-06 21:55:07Z ea $
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


/* TODO: this file should be totally rewritten
 *
 * a) look if a special option is set for smss.exe in
 *    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options
 *
 * b) make smss register with itself for IMAGE_SUBSYSTEM_NATIVE
 *    (programmatically)
 *
 * d) make smss initialize Debug (DBGSS) and Windows (CSRSS) as described
 *    in the registry key Required="Debug Windows"
 *    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\SubSystems
 *
 * e) make optional subsystems loadable (again: they must be described in the registry
 *    key Optional="Posix Os2" to be allowed to run)
 */

/**********************************************************************
 */
NTSTATUS
SmLoadSubsystems(VOID)
{
	SYSTEM_LOAD_AND_CALL_IMAGE ImageInfo;
	NTSTATUS                   Status = STATUS_SUCCESS;
	WCHAR                      Data [MAX_PATH + 1];
	ULONG                      DataLength = sizeof Data;
	ULONG                      DataType = 0;


	DPRINT("SM: loading subsystems\n");
 
	/* Load Kmode subsystem (aka win32k.sys) */
	Status = SmLookupSubsystem (L"Kmode",
				    Data,
				    & DataLength,
				    & DataType,
				    TRUE);
	if((STATUS_SUCCESS == Status) && (DataLength > sizeof Data[0]))
	{
		WCHAR ImagePath [MAX_PATH + 1] = {0};

		wcscpy (ImagePath, L"\\??\\");
		wcscat (ImagePath, Data);
		RtlZeroMemory (& ImageInfo, sizeof ImageInfo);
		RtlInitUnicodeString (& ImageInfo.ModuleName, ImagePath);
		Status = NtSetSystemInformation(SystemLoadAndCallImage,
					  & ImageInfo,
					  sizeof ImageInfo);
		if(!NT_SUCCESS(Status))
		{
			DPRINT("SM: loading Kmode failed (Status=0x%08lx)\n",
				Status);
			return Status;
		}
	}
	/* TODO: load Required subsystems (Debug Windows) */
	return Status;
}

NTSTATUS
SmRunCsrss(VOID)
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  OBJECT_ATTRIBUTES ObjectAttributes;
  RTL_PROCESS_INFO ProcessInfo;
  HANDLE CsrssInitEvent;
  WCHAR ImagePath [MAX_PATH];

  DPRINT("SM: initializing csrss\n");

  /* Run csrss.exe */
  RtlRosInitUnicodeStringFromLiteral(&UnicodeString,
				  L"\\CsrssInitDone");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     EVENT_ALL_ACCESS,
			     0,
			     NULL);
  Status = NtCreateEvent(&CsrssInitEvent,
			 EVENT_ALL_ACCESS,
			 &ObjectAttributes,
			 NotificationEvent,
			 FALSE);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to create csrss notification event\n");
    }

  /*
   * Start the Win32 subsystem (csrss.exe)
   */

  /* initialize executable path */
  wcscpy(ImagePath, L"\\??\\");
  wcscat(ImagePath, SharedUserData->NtSystemRoot);
  wcscat(ImagePath, L"\\system32\\csrss.exe");

  Status = SmCreateUserProcess(ImagePath,
		  		L"",
				FALSE, /* wait */
				NULL,
				FALSE, /* terminate */
				& ProcessInfo);
  
  if (!NT_SUCCESS(Status))
    {
      DPRINT("SM: %s: Loading csrss.exe failed!\n", __FUNCTION__);
      return(Status);
    }

  Status = NtWaitForSingleObject(CsrssInitEvent,
			FALSE,
			NULL);

  Children[CHILD_CSRSS] = ProcessInfo.ProcessHandle;

  return Status;
}

NTSTATUS
SmRunWinlogon(VOID)
{
  NTSTATUS         Status = STATUS_SUCCESS;
  RTL_PROCESS_INFO ProcessInfo;
  WCHAR            ImagePath [MAX_PATH];

  /*
   * Start the logon process (winlogon.exe)
   */

  DPRINT("SM: starting winlogon\n");

  /* initialize executable path */
  wcscpy(ImagePath, L"\\??\\");
  wcscat(ImagePath, SharedUserData->NtSystemRoot);
  wcscat(ImagePath, L"\\system32\\winlogon.exe");

  Status = SmCreateUserProcess(ImagePath,
		  		L"",
				FALSE, /* wait */
				NULL,
				FALSE, /* terminate */
		  		& ProcessInfo);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("SM: %s: Loading winlogon.exe failed!\n", __FUNCTION__);
      NtTerminateProcess(Children[CHILD_CSRSS], 0);
      return(Status);
    }

  Children[CHILD_WINLOGON] = ProcessInfo.ProcessHandle;

  return Status;
}

/* EOF */
