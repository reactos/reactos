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

/* FUNCTIONS ***************************************************************/

VOID PsInit(VOID)
{
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitIdleThread();
}
