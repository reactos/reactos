/* $Id: guicheck.c,v 1.2 2001/07/04 20:40:24 chorns Exp $
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
#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>


VOID
GuiCheck(VOID)
{
/*  if (NtCurrentTeb()->MessageQueue)
    return;

  NtCurrentTeb()->MessageQueue = MsqCreateMessageQueue();*/
}

/* EOF */
