/* $Id: callback.c,v 1.1 2002/01/27 14:47:44 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/wndproc.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>
#include <user32/callback.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

LRESULT STDCALL
W32kCallWindowProc(WNDPROC Proc,
		   HWND Wnd,
		   UINT Message,
		   WPARAM wParam,
		   LPARAM lParam)
{
  WINDOWPROC_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;

  Arguments.Proc = Proc;
  Arguments.Wnd = Wnd;
  Arguments.Msg = Message;
  Arguments.wParam = wParam;
  Arguments.lParam = lParam;
  ResultPointer = &Result;
  Status = NtW32Call(USER32_CALLBACK_WINDOWPROC,
		     &Arguments,
		     sizeof(WINDOWPROC_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     sizeof(LRESULT));
  if (!NT_SUCCESS(Status))
    {
      return(0xFFFFFFFF);
    }
  return(Result);
}

/* EOF */
