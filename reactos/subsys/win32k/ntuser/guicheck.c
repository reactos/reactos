/* $Id: guicheck.c,v 1.4 2002/01/27 01:11:24 dwelch Exp $
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

#include <ddk/ntddk.h>
#include <napi/teb.h>
#include <win32k/win32k.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>
#include <include/object.h>
#include <napi/win32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
W32kGuiCheck(VOID)
{
  if (PsGetWin32Process() == NULL)
    {
      PsCreateWin32Process(PsGetCurrentProcess());

      InitializeListHead(&PsGetWin32Process()->ClassListHead);
      ExInitializeFastMutex(&PsGetWin32Process()->ClassListLock);
      InitializeListHead(&PsGetWin32Process()->WindowListHead);
      ExInitializeFastMutex(&PsGetWin32Process()->WindowListLock);
      PsGetWin32Process()->HandleTable = ObmCreateHandleTable();
    }

  if (PsGetWin32Thread() == NULL)
    {
      PsCreateWin32Thread(PsGetCurrentThread());
      PsGetWin32Thread()->MessageQueue = MsqCreateMessageQueue();
    }
}

/* EOF */
