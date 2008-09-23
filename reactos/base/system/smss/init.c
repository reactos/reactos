/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/init.c
 * PURPOSE:         Initialization.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

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
	{TRUE,  SmLoadSubsystems,             "load subsystems"}
};

NTSTATUS
InitSessionManager(VOID)
{
  UINT i = 0;
  NTSTATUS Status = STATUS_SUCCESS;

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
