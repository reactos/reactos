/* $Id$
 *
 * init.c - Session Manager initialization
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


/* FUNCTIONS ****************************************************************/

static NTSTATUS
SmpSignalInitEvent(VOID)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  HANDLE ReactOSInitEvent;

  RtlRosInitUnicodeStringFromLiteral(&UnicodeString, L"\\ReactOSInitDone");
  InitializeObjectAttributes(&ObjectAttributes,
    &UnicodeString,
    EVENT_ALL_ACCESS,
    0,
    NULL);
  Status = NtOpenEvent(&ReactOSInitEvent,
    EVENT_ALL_ACCESS,
    &ObjectAttributes);
  if (NT_SUCCESS(Status))
    {
      LARGE_INTEGER Timeout;
      /* This will cause the boot screen image to go away (if displayed) */
      NtPulseEvent(ReactOSInitEvent, NULL);

      /* Wait for the display mode to be changed (if in graphics mode) */
      Timeout.QuadPart = -50000000LL;  /* 5 second timeout */
      NtWaitForSingleObject(ReactOSInitEvent, FALSE, &Timeout);

      NtClose(ReactOSInitEvent);
    }
  else
    {
      /* We don't really care if this fails */
      DPRINT1("SM: Failed to open ReactOS init notification event\n");
    }
  return Status;
}

typedef NTSTATUS (* SM_INIT_ROUTINE)(VOID);

struct {
	BOOL Required;
	SM_INIT_ROUTINE EntryPoint;
	PCHAR ErrorMessage;
} InitRoutine [] = {
	{TRUE,  SmCreateHeap,                 "create private heap, aborting"},
	{TRUE,  SmCreateObjectDirectories,    "create object directories"},
	{TRUE,  SmCreateApiPort,              "create \\SmApiPort"},
	{TRUE,  SmCreateEnvironment,          "create the system environment"},
	{TRUE,  SmSetEnvironmentVariables,    "set system environment variables"},
	{TRUE,  SmInitDosDevices,             "create dos device links"},
	{TRUE,  SmRunBootApplications,        "run boot applications"},
	{TRUE,  SmProcessFileRenameList,      "process the file rename list"},
	{FALSE, SmLoadKnownDlls,              "preload system DLLs"},
	{TRUE,  SmCreatePagingFiles,          "create paging files"},
	{TRUE,  SmInitializeRegistry,         "initialize the registry"},
	{FALSE, SmUpdateEnvironment,          "update environment variables"},
	{TRUE,  SmInitializeClientManagement, "initialize client management"},
	{TRUE,  SmLoadSubsystems,             "load subsystems"},
	{FALSE, SmpSignalInitEvent,           "open ReactOS init notification event"},
	{TRUE,  SmRunCsrss,                   "run csrss"},
	{TRUE,  SmRunWinlogon,                "run winlogon"},
	{TRUE,  SmInitializeDbgSs,            "initialize DbgSs"}
};

NTSTATUS
InitSessionManager(VOID)
{
  int i;
  NTSTATUS Status;

  for (i=0; i < (sizeof InitRoutine / sizeof InitRoutine[0]); i++)
  {
    Status = InitRoutine[i].EntryPoint();
    if(!NT_SUCCESS(Status))
    {
      DPRINT1("SM: %s: failed to %s (Status=%lx)\n", 
	__FUNCTION__,
	InitRoutine[i].ErrorMessage,
	Status);
      if (InitRoutine[i].Required)
      {
        return(Status);
      }
    }
  }


  return(STATUS_SUCCESS);
}

/* EOF */
