 /* $Id: guicheck.c,v 1.7 2002/09/07 15:13:12 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GUI state check
 * FILE:             subsys/win32k/ntuser/guicheck.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:            The GuiCheck() function performs a few delayed operations:
 *                   1) A GUI process is assigned a window station
 *                   2) A message queue is created for a GUI thread before use
 *                   3) The system window classes are registered for a process
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <win32k/win32k.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>
#include <include/object.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
W32kGuiCheck(VOID)
{
  if (PsGetWin32Process() == NULL)
    {
      NTSTATUS Status;

      PsCreateWin32Process(PsGetCurrentProcess());

      InitializeListHead(&PsGetWin32Process()->ClassListHead);
      ExInitializeFastMutex(&PsGetWin32Process()->ClassListLock);      

      Status = 
	ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				    UserMode,
				    GENERIC_ALL,
				    &PsGetWin32Process()->WindowStation);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("W32K: Failed to reference a window station for "
		   "process.\n");
	}
    }

  if (PsGetWin32Thread() == NULL)
    {
      NTSTATUS Status;

      PsCreateWin32Thread(PsGetCurrentThread());
      PsGetWin32Thread()->MessageQueue = MsqCreateMessageQueue();
      InitializeListHead(&PsGetWin32Thread()->WindowListHead);
      ExInitializeFastMutex(&PsGetWin32Thread()->WindowListLock);

      /* By default threads get assigned their process's desktop. */
      PsGetWin32Thread()->Desktop = NULL;
      Status = ObReferenceObjectByHandle(PsGetCurrentProcess()->Win32Desktop,
					 GENERIC_ALL,
					 ExDesktopObjectType,
					 UserMode,
					 (PVOID*)&PsGetWin32Thread()->Desktop,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("W32K: Failed to reference a desktop for thread.\n");
	}
    }
}

/* EOF */
