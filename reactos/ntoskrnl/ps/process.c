/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              ntoskrnl/ps/process.c
 * PURPOSE:           Process managment
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              21/07/98: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

EPROCESS SystemProcess = {{0},};
HANDLE SystemProcessHandle = NULL;

/* FUNCTIONS *****************************************************************/

VOID PsInitProcessManagment(VOID)
{
   InitializeListHead(&(SystemProcess.Pcb.MemoryAreaList));
}

PKPROCESS KeGetCurrentProcess(VOID)
{
   return(&(PsGetCurrentProcess()->Pcb));
}

struct _EPROCESS* PsGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   DPRINT("PsGetCurrentProcess() = %x\n",PsGetCurrentThread()->ThreadsProcess);
   return(PsGetCurrentThread()->ThreadsProcess);
}

