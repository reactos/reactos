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

/* TODO: this file should be totally rewritten
 *
 * a) look if a special option is set for smss.exe in
 *    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options
 *
 * b) make smss register with itself for IMAGE_SUBSYSTEM_NATIVE
 *    (programmatically)
 *
 * c) make smss load win32k.sys as set in Kmode key
 *    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\SubSystems
 *
 * d) make smss initialize Debug (DBGSS) and Windows (CSRSS) as described
 *    in the registry key Required="Debug Windows"
 *    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\SubSystems
 *
 * e) make optional subsystems loadable (again: they must be described in the registry
 *    key Optional="Posix Os2" to be allowed to run)
 */
NTSTATUS
SmLoadSubsystems(VOID)
{
  SYSTEM_LOAD_AND_CALL_IMAGE ImageInfo;
  NTSTATUS Status;

  DPRINT("SM: loading subsystems\n");

  /* Load kernel mode subsystem (aka win32k.sys) */
  RtlRosInitUnicodeStringFromLiteral(&ImageInfo.ModuleName,
		       L"\\SystemRoot\\system32\\win32k.sys");

  Status = NtSetSystemInformation(SystemLoadAndCallImage,
				  &ImageInfo,
				  sizeof(SYSTEM_LOAD_AND_CALL_IMAGE));

  DPRINT("SMSS: Loaded win32k.sys (Status %lx)\n", Status);
#if 0
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
#endif

  /* FIXME: load more subsystems (csrss!) */

  return(Status);
}

NTSTATUS
SmRunCsrss(VOID)
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  HANDLE CsrssInitEvent;
  WCHAR UnicodeBuffer[MAX_PATH];

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
  wcscpy(UnicodeBuffer, L"\\??\\");
  wcscat(UnicodeBuffer, SharedUserData->NtSystemRoot);
  wcscat(UnicodeBuffer, L"\\system32\\csrss.exe");
  RtlInitUnicodeString(&UnicodeString,
		       UnicodeBuffer);

  RtlCreateProcessParameters(&ProcessParameters,
			     &UnicodeString,
			     NULL,
			     NULL,
			     NULL,
			     SmSystemEnvironment,
			     NULL,
			     NULL,
			     NULL,
			     NULL);

  Status = RtlCreateUserProcess(&UnicodeString,
				OBJ_CASE_INSENSITIVE,
				ProcessParameters,
				NULL,
				NULL,
				NULL,
				FALSE,
				NULL,
				NULL,
				&ProcessInfo);

  RtlDestroyProcessParameters (ProcessParameters);

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
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  WCHAR UnicodeBuffer[MAX_PATH];

  /*
   * Start the logon process (winlogon.exe)
   */

  DPRINT("SM: starting winlogon\n");

  /* initialize executable path */
  wcscpy(UnicodeBuffer, L"\\??\\");
  wcscat(UnicodeBuffer, SharedUserData->NtSystemRoot);
  wcscat(UnicodeBuffer, L"\\system32\\winlogon.exe");
  RtlInitUnicodeString(&UnicodeString,
		       UnicodeBuffer);

  RtlCreateProcessParameters(&ProcessParameters,
			     &UnicodeString,
			     NULL,
			     NULL,
			     NULL,
			     SmSystemEnvironment,
			     NULL,
			     NULL,
			     NULL,
			     NULL);

  Status = RtlCreateUserProcess(&UnicodeString,
				OBJ_CASE_INSENSITIVE,
				ProcessParameters,
				NULL,
				NULL,
				NULL,
				FALSE,
				NULL,
				NULL,
				&ProcessInfo);

  RtlDestroyProcessParameters(ProcessParameters);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("SM: %s: Loading winlogon.exe failed!\n", __FUNCTION__);
      NtTerminateProcess(Children[CHILD_CSRSS],
			 0);
      return(Status);
    }
  Children[CHILD_WINLOGON] = ProcessInfo.ProcessHandle;

  return Status;
}

/* EOF */
