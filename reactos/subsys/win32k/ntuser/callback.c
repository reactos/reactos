/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: callback.c,v 1.10 2003/06/05 03:55:36 mdill Exp $
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
#include <include/window.h>
#include <include/msgqueue.h>
#include <user32/callback.h>
#include <include/callback.h>

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
  SENDASYNCPROC_CALLBACK_ARGUMENTS Arguments;
  NTSTATUS Status;

  Arguments.Callback = CompletionCallback;
  Arguments.Wnd = hWnd;
  Arguments.Msg = Msg;
  Arguments.Context = CompletionCallbackContext;
  Arguments.Result = Result;
  Status = NtW32Call(USER32_CALLBACK_SENDASYNCPROC,
		     &Arguments,
		     sizeof(SENDASYNCPROC_CALLBACK_ARGUMENTS),
		     NULL,
		     NULL);
  if (!NT_SUCCESS(Status))
    {
      return;
    }
  return;  
}

LRESULT STDCALL
W32kSendNCCALCSIZEMessage(HWND Wnd, BOOL Validate, PRECT Rect,
			  NCCALCSIZE_PARAMS* Params)
{
  SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS Arguments;
  SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.Validate = Validate;
  if (!Validate)
    {
      Arguments.Rect = *Rect;
    }
  else
    {
      Arguments.Params = *Params;
    }
  ResultPointer = &Result;
  ResultLength = sizeof(SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDNCCALCSIZE,
		     &Arguments,
		     sizeof(SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  if (!Validate)
    {
      *Rect = Result.Rect;
    }
  else
    {
      *Params = Result.Params;
    }
  return(Result.Result);
}

LRESULT STDCALL
W32kSendCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct)
{
  SENDCREATEMESSAGE_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

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
W32kSendNCCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct)
{
  SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

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
  ULONG ResultLength;

  if (W32kIsDesktopWindow(Wnd))
    {
      return(W32kDesktopWindowProc(Wnd, Message, wParam, lParam));
    }

  Arguments.Proc = Proc;
  Arguments.Wnd = Wnd;
  Arguments.Msg = Message;
  Arguments.wParam = wParam;
  Arguments.lParam = lParam;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_WINDOWPROC,
		     &Arguments,
		     sizeof(WINDOWPROC_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0xFFFFFFFF);
    }
  return(Result);
}

LRESULT STDCALL
W32kSendGETMINMAXINFOMessage(HWND Wnd, MINMAXINFO* MinMaxInfo)
{
  SENDGETMINMAXINFO_CALLBACK_ARGUMENTS Arguments;
  SENDGETMINMAXINFO_CALLBACK_RESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.MinMaxInfo = *MinMaxInfo;
  ResultPointer = &Result;
  ResultLength = sizeof(Result);
  Status = NtW32Call(USER32_CALLBACK_SENDGETMINMAXINFO,
		     &Arguments,
		     sizeof(SENDGETMINMAXINFO_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result.Result);  
}

LRESULT STDCALL
W32kSendWINDOWPOSCHANGINGMessage(HWND Wnd, WINDOWPOS* WindowPos)
{
  SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.WindowPos = *WindowPos;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDWINDOWPOSCHANGING,
		     &Arguments,
		     sizeof(SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result);
}

LRESULT STDCALL
W32kSendWINDOWPOSCHANGEDMessage(HWND Wnd, WINDOWPOS* WindowPos)
{
  SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.WindowPos = *WindowPos;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDWINDOWPOSCHANGED,
		     &Arguments,
		     sizeof(SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result);
}

LRESULT STDCALL
W32kSendSTYLECHANGINGMessage(HWND Wnd, DWORD WhichStyle, STYLESTRUCT* Style)
{
  SENDSTYLECHANGING_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.Style = *Style;
  Arguments.WhichStyle = WhichStyle;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDSTYLECHANGING,
		     &Arguments,
		     sizeof(SENDSTYLECHANGING_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  *Style = Arguments.Style;  
  return(Result);
}

LRESULT STDCALL
W32kSendSTYLECHANGEDMessage(HWND Wnd, DWORD WhichStyle, STYLESTRUCT* Style)
{
  SENDSTYLECHANGED_CALLBACK_ARGUMENTS Arguments;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  Arguments.Wnd = Wnd;
  Arguments.Style = *Style;
  Arguments.WhichStyle = WhichStyle;
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_SENDSTYLECHANGED,
		     &Arguments,
		     sizeof(SENDSTYLECHANGED_CALLBACK_ARGUMENTS),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return(Result);
}

LRESULT STDCALL
W32kCallTrampolineWindowProc(WNDPROC Proc,
			     HWND Wnd,
			     UINT Message,
			     WPARAM wParam,
			     LPARAM lParam)
{
  switch (Message)
    {
    case WM_NCCREATE:
      return W32kSendNCCREATEMessage(Wnd, (CREATESTRUCTW*)lParam);
     
    case WM_CREATE:
      return W32kSendCREATEMessage(Wnd, (CREATESTRUCTW*)lParam);

    case WM_GETMINMAXINFO:
      return W32kSendGETMINMAXINFOMessage(Wnd, (MINMAXINFO*)lParam);

    case WM_NCCALCSIZE:
      {
	if (wParam)
	  {
	    return W32kSendNCCALCSIZEMessage(Wnd, TRUE, NULL, 
					     (NCCALCSIZE_PARAMS*)lParam);
	  }
	else
	  {
	    return W32kSendNCCALCSIZEMessage(Wnd, FALSE, (RECT*)lParam, NULL);
	  }
      }

    case WM_WINDOWPOSCHANGING:
      return W32kSendWINDOWPOSCHANGINGMessage(Wnd, (WINDOWPOS*) lParam);

    case WM_WINDOWPOSCHANGED:
      return W32kSendWINDOWPOSCHANGEDMessage(Wnd, (WINDOWPOS*) lParam);

    case WM_STYLECHANGING:
      return W32kSendSTYLECHANGINGMessage(Wnd, wParam, (STYLESTRUCT*) lParam);

    case WM_STYLECHANGED:
	  return W32kSendSTYLECHANGEDMessage(Wnd, wParam, (STYLESTRUCT*) lParam);

    default:
      return(W32kCallWindowProc(Proc, Wnd, Message, wParam, lParam));
    }
}

/* EOF */
