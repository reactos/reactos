/*
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    kernel/psmgr/psmgr.c
 * PURPOSE:                 Process managment
 * PROGRAMMER:              David Welch (welch@mcmail.com)
 */

/* INCLUDES **************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/psmgr.h>

/* FUNCTIONS ***************************************************************/

VOID PsMgrInit(VOID)
{
   ObjInitializeHandleTable(NULL);
   PsInitThreadManagment();
}
