/* $Id: callback.c,v 1.3 2002/06/18 21:51:11 dwelch Exp $
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

VOID STDCALL
W32kCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result)
{
}

LRESULT STDCALL
W32kSendNCCALCSIZEMessage(HWND Wnd, BOOL Validate, RECT Rect1,
			  RECT Rect2, RECT Rect3, PWINDOWPOS Pos)
{
}

LRESULT STDCALL
W32kSendCREATEMessage(HWND Wnd, CREATESTRUCT* CreateStruct)
{
  SENDCREATEMESSAGE_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  DWORD ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.CreateStruct = *CreateStruct;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDCREATE,
		     &Arguments,
		     sizeof(SENDCREATEMESSAGE_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result);
}

LRESULT STDCALL
W32kSendNCCREATEMessage(HWND Wnd, CREATESTRUCT* CreateStruct)
{
  SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  DWORD ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.CreateStruct = *CreateStruct;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDNCCREATE,
		     &Arguments,
		     sizeof(SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result);
}				

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
