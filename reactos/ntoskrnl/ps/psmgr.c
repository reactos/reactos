/*
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    ntoskrnl/ps/psmgr.c
 * PURPOSE:                 Process managment
 * PROGRAMMER:              David Welch (welch@mcmail.com)
 */

/* INCLUDES **************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID PiShutdownProcessManager(VOID)
{
   DPRINT("PiShutdownMemoryManager()\n");
   
   PiKillMostProcesses();
}

VOID PiInitProcessManager(VOID)
{
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitIdleThread();
   PiInitApcManagement();
}
