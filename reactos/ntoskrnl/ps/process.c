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

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

EPROCESS SystemProcess = {{0},};
HANDLE SystemProcessHandle = NULL;

/* FUNCTIONS *****************************************************************/

PKPROCESS KeGetCurrentProcess(VOID)
{
   return(NULL);
//   return(&(PsGetCurrentProcess()->Pcb));
}

struct _EPROCESS* PsGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   return(PsGetCurrentThread()->ThreadsProcess);
}

