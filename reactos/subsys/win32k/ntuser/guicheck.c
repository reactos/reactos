/* $Id: guicheck.c,v 1.3 2002/01/13 22:52:08 dwelch Exp $
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

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
W32kGuiCheck(VOID)
{
  if (PsGetWin32Thread()->MessageQueue != NULL)
    return;
  
  PsGetWin32Thread()->MessageQueue = MsqCreateMessageQueue();
}

/* EOF */
