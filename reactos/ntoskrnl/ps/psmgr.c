/*
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    ntoskrnl/ps/psmgr.c
 * PURPOSE:                 Process managment
 * PROGRAMMER:              David Welch (welch@mcmail.com)
 */

/* INCLUDES **************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ps.h>

/* FUNCTIONS ***************************************************************/

VOID PsInit(VOID)
{
   ObjInitializeHandleTable(NULL);
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitIdleThread();
}
