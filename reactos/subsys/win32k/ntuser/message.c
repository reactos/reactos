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
/* $Id: message.c,v 1.71 2004/07/03 17:40:25 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

typedef struct
{
  UINT uFlags;
  UINT uTimeout;
  ULONG_PTR Result;
} DOSENDMESSAGE, *PDOSENDMESSAGE;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
IntInitMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntCleanupMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}

#define MMS_SIZE_WPARAM      -1
#define MMS_SIZE_WPARAMWCHAR -2
#define MMS_SIZE_LPARAMSZ    -3
#define MMS_SIZE_SPECIAL     -4
#define MMS_FLAG_READ        0x01
#define MMS_FLAG_WRITE       0x02
#define MMS_FLAG_READWRITE   (MMS_FLAG_READ | MMS_FLAG_WRITE)
typedef struct tagMSGMEMORY
  {
    UINT Message;
    UINT Size;
    INT Flags;
  } MSGMEMORY, *PMSGMEMORY;

static MSGMEMORY MsgMemory[] =
  {
    { WM_CREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_DDE_ACK, sizeof(KMDDELPARAM), MMS_FLAG_READ },
    { WM_DDE_EXECUTE, MMS_SIZE_WPARAM, MMS_FLAG_READ },
    { WM_GETMINMAXINFO, sizeof(MINMAXINFO), MMS_FLAG_READWRITE },
    { WM_GETTEXT, MMS_SIZE_WPARAMWCHAR, MMS_FLAG_WRITE },
    { WM_NCCALCSIZE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_NCCREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_SETTEXT, MMS_SIZE_LPARAMSZ, MMS_FLAG_READ },
    { WM_STYLECHANGED, sizeof(STYLESTRUCT), MMS_FLAG_READ },
    { WM_STYLECHANGING, sizeof(STYLESTRUCT), MMS_FLAG_READWRITE },
    { WM_WINDOWPOSCHANGED, sizeof(WINDOWPOS), MMS_FLAG_READ },
    { WM_WINDOWPOSCHANGING, sizeof(WINDOWPOS), MMS_FLAG_READWRITE },
  };

static PMSGMEMORY FASTCALL
FindMsgMemory(UINT Msg)
  {
  PMSGMEMORY MsgMemoryEntry;

  /* See if this message type is present in the table */
  for (MsgMemoryEntry = MsgMemory;
       MsgMemoryEntry < MsgMemory + sizeof(MsgMemory) / sizeof(MSGMEMORY);
       MsgMemoryEntry++)
    {
      if (Msg == MsgMemoryEntry->Message)
        {
          return MsgMemoryEntry;
        }
    }

  return NULL;
}

static UINT FASTCALL
MsgMemorySize(PMSGMEMORY MsgMemoryEntry, WPARAM wParam, LPARAM lParam)
{
  CREATESTRUCTW *Cs;
  PUNICODE_STRING WindowName;
  PUNICODE_STRING ClassName;
  UINT Size;

  if (MMS_SIZE_WPARAM == MsgMemoryEntry->Size)
    {
      return (UINT) wParam;
    }
  else if (MMS_SIZE_WPARAMWCHAR == MsgMemoryEntry->Size)
    {
      return (UINT) (wParam * sizeof(WCHAR));
    }
  else if (MMS_SIZE_LPARAMSZ == MsgMemoryEntry->Size)
    {
      return (UINT) ((wcslen((PWSTR) lParam) + 1) * sizeof(WCHAR));
    }
  else if (MMS_SIZE_SPECIAL == MsgMemoryEntry->Size)
    {
      switch(MsgMemoryEntry->Message)
        {
        case WM_CREATE:
        case WM_NCCREATE:
          Cs = (CREATESTRUCTW *) lParam;
          WindowName = (PUNICODE_STRING) Cs->lpszName;
          ClassName = (PUNICODE_STRING) Cs->lpszClass;
          Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
          if (IS_ATOM(ClassName->Buffer))
            {
              Size += sizeof(WCHAR) + sizeof(ATOM);
            }
          else
            {
              Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
            }
          return Size;
          break;

        case WM_NCCALCSIZE:
          return wParam ? sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) : sizeof(RECT);
          break;

        default:
          assert(FALSE);
          return 0;
          break;
        }
    }
  else
    {
      return MsgMemoryEntry->Size;
    }
}

static FASTCALL NTSTATUS
PackParam(LPARAM *lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  NCCALCSIZE_PARAMS *UnpackedNcCalcsize;
  NCCALCSIZE_PARAMS *PackedNcCalcsize;
  CREATESTRUCTW *UnpackedCs;
  CREATESTRUCTW *PackedCs;
  PUNICODE_STRING WindowName;
  PUNICODE_STRING ClassName;
  UINT Size;
  PCHAR CsData;

  *lParamPacked = lParam;
  if (WM_NCCALCSIZE == Msg && wParam)
    {
      UnpackedNcCalcsize = (NCCALCSIZE_PARAMS *) lParam;
      if (UnpackedNcCalcsize->lppos != (PWINDOWPOS) (UnpackedNcCalcsize + 1))
        {
          PackedNcCalcsize = ExAllocatePoolWithTag(PagedPool,
                                                   sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS),
                                                   TAG_MSG);
          if (NULL == PackedNcCalcsize)
            {
              DPRINT1("Not enough memory to pack lParam\n");
              return STATUS_NO_MEMORY;
            }
          RtlCopyMemory(PackedNcCalcsize, UnpackedNcCalcsize, sizeof(NCCALCSIZE_PARAMS));
          PackedNcCalcsize->lppos = (PWINDOWPOS) (PackedNcCalcsize + 1);
          RtlCopyMemory(PackedNcCalcsize->lppos, UnpackedNcCalcsize->lppos, sizeof(WINDOWPOS));
          *lParamPacked = (LPARAM) PackedNcCalcsize;
        }
    }
  else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
    {
      UnpackedCs = (CREATESTRUCTW *) lParam;
      WindowName = (PUNICODE_STRING) UnpackedCs->lpszName;
      ClassName = (PUNICODE_STRING) UnpackedCs->lpszClass;
      Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
      if (IS_ATOM(ClassName->Buffer))
        {
          Size += sizeof(WCHAR) + sizeof(ATOM);
        }
      else
        {
          Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
        }
      PackedCs = ExAllocatePoolWithTag(PagedPool, Size, TAG_MSG);
      if (NULL == PackedCs)
        {
          DPRINT1("Not enough memory to pack lParam\n");
          return STATUS_NO_MEMORY;
        }
      RtlCopyMemory(PackedCs, UnpackedCs, sizeof(CREATESTRUCTW));
      CsData = (PCHAR) (PackedCs + 1);
      PackedCs->lpszName = (LPCWSTR) (CsData - (PCHAR) PackedCs);
      RtlCopyMemory(CsData, WindowName->Buffer, WindowName->Length);
      CsData += WindowName->Length;
      *((WCHAR *) CsData) = L'\0';
      CsData += sizeof(WCHAR);
      PackedCs->lpszClass = (LPCWSTR) (CsData - (PCHAR) PackedCs);
      if (IS_ATOM(ClassName->Buffer))
        {
          *((WCHAR *) CsData) = L'A';
          CsData += sizeof(WCHAR);
          *((ATOM *) CsData) = (ATOM)(DWORD_PTR) ClassName->Buffer;
          CsData += sizeof(ATOM);
        }
      else
        {
          *((WCHAR *) CsData) = L'S';
          CsData += sizeof(WCHAR);
          RtlCopyMemory(CsData, ClassName->Buffer, ClassName->Length);
          CsData += ClassName->Length;
          *((WCHAR *) CsData) = L'\0';
          CsData += sizeof(WCHAR);
        }
      ASSERT(CsData == (PCHAR) PackedCs + Size);
      *lParamPacked = (LPARAM) PackedCs;
    }

  return STATUS_SUCCESS;
}

static FASTCALL NTSTATUS
UnpackParam(LPARAM lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  NCCALCSIZE_PARAMS *UnpackedParams;
  NCCALCSIZE_PARAMS *PackedParams;
  PWINDOWPOS UnpackedWindowPos;

  if (lParamPacked == lParam)
    {
      return STATUS_SUCCESS;
    }

  if (WM_NCCALCSIZE == Msg && wParam)
    {
      PackedParams = (NCCALCSIZE_PARAMS *) lParamPacked;
      UnpackedParams = (NCCALCSIZE_PARAMS *) lParam;
      UnpackedWindowPos = UnpackedParams->lppos;
      RtlCopyMemory(UnpackedParams, PackedParams, sizeof(NCCALCSIZE_PARAMS));
      UnpackedParams->lppos = UnpackedWindowPos;
      RtlCopyMemory(UnpackedWindowPos, PackedParams + 1, sizeof(WINDOWPOS));
      ExFreePool((PVOID) lParamPacked);

      return STATUS_SUCCESS;
    }
  else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
    {
      ExFreePool((PVOID) lParamPacked);

      return STATUS_SUCCESS;
    }

  ASSERT(FALSE);

  return STATUS_INVALID_PARAMETER;
}


LRESULT STDCALL
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO UnsafeMsgInfo)
{
  NTSTATUS Status;
  NTUSERDISPATCHMESSAGEINFO MsgInfo;
  PWINDOW_OBJECT WindowObject;
  LRESULT Result = TRUE;

  Status = MmCopyFromCaller(&MsgInfo, UnsafeMsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }

  /* Process timer messages. */
  if (WM_TIMER == MsgInfo.Msg.message && 0 != MsgInfo.Msg.lParam)
    {
      LARGE_INTEGER LargeTickCount;
      /* FIXME: Call hooks. */

      /* FIXME: Check for continuing validity of timer. */

      MsgInfo.HandledByKernel = FALSE;
      KeQueryTickCount(&LargeTickCount);
      MsgInfo.Proc = (WNDPROC) MsgInfo.Msg.lParam;
      MsgInfo.Msg.lParam = (LPARAM)LargeTickCount.u.LowPart;
    }
  else if (NULL == MsgInfo.Msg.hwnd)
    {
      MsgInfo.HandledByKernel = TRUE;
      Result = 0;
    }
  else
    {
      /* Get the window object. */
      WindowObject = IntGetWindowObject(MsgInfo.Msg.hwnd);
      if (NULL == WindowObject)
        {
          SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
          MsgInfo.HandledByKernel = TRUE;
          Result = 0;
        }
      else
        {
          if (WindowObject->OwnerThread != PsGetCurrentThread())
            {
              IntReleaseWindowObject(WindowObject);
              DPRINT1("Window doesn't belong to the calling thread!\n");
              MsgInfo.HandledByKernel = TRUE;
              Result = 0;
            }
          else
            {
              /* FIXME: Call hook procedures. */

              MsgInfo.HandledByKernel = FALSE;
              Result = 0;
              if (0xFFFF0000 != ((DWORD) WindowObject->WndProcW & 0xFFFF0000))
                {
                  if (0xFFFF0000 != ((DWORD) WindowObject->WndProcA & 0xFFFF0000))
                    {
                      /* Both Unicode and Ansi winprocs are real, use whatever
                         usermode prefers */
                      MsgInfo.Proc = (MsgInfo.Ansi ? WindowObject->WndProcA
                                      : WindowObject->WndProcW);
                    }
                  else
                    {
                      /* Real Unicode winproc */
                      MsgInfo.Ansi = FALSE;
                      MsgInfo.Proc = WindowObject->WndProcW;
                    }
                }
              else
                {
                  /* Must have real Ansi winproc */
                  MsgInfo.Ansi = TRUE;
                  MsgInfo.Proc = WindowObject->WndProcA;
                }
            }
          IntReleaseWindowObject(WindowObject);
        }
    }
  Status = MmCopyToCaller(UnsafeMsgInfo, &MsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }
  
  return Result;
}


BOOL STDCALL
NtUserTranslateMessage(LPMSG lpMsg,
		       HKL dwhkl)
{
  NTSTATUS Status;
  MSG SafeMsg;

  Status = MmCopyFromCaller(&SafeMsg, lpMsg, sizeof(MSG));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  return IntTranslateKbdMessage(&SafeMsg, dwhkl);
}


VOID FASTCALL
IntSendHitTestMessages(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg)
{
  if(!Msg->hwnd || ThreadQueue->CaptureWindow)
  {
    return;
  }
  
  switch(Msg->message)
  {
    case WM_MOUSEMOVE:
    {
      IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
      break;
    }
    case WM_NCMOUSEMOVE:
    {
      IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
      break;
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
    {
      WPARAM wParam;
      PSYSTEM_CURSORINFO CurInfo;
      
      if(!IntGetWindowStationObject(InputWindowStation))
      {
        break;
      }
      CurInfo = IntGetSysCursorInfo(InputWindowStation);
      wParam = (WPARAM)(CurInfo->ButtonsDown);
      ObDereferenceObject(InputWindowStation);
      
      IntSendMessage(Msg->hwnd, WM_MOUSEMOVE, wParam, Msg->lParam);
      IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
      break;
    }
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCXBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCXBUTTONDBLCLK:
    {
      IntSendMessage(Msg->hwnd, WM_NCMOUSEMOVE, (WPARAM)Msg->wParam, Msg->lParam);
      IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
      break;
    }
  }
}

BOOL FASTCALL
IntActivateWindowMouse(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg, PWINDOW_OBJECT MsgWindow, 
                       USHORT *HitTest)
{
  ULONG Result;
  
  if(*HitTest == (USHORT)HTTRANSPARENT)
  {
    /* eat the message, search again! */
    return TRUE;
  }
  
  Result = IntSendMessage(MsgWindow->Self, WM_MOUSEACTIVATE, (WPARAM)NtUserGetParent(MsgWindow->Self), (LPARAM)MAKELONG(*HitTest, Msg->message));
  switch (Result)
  {
    case MA_NOACTIVATEANDEAT:
      return TRUE;
    case MA_NOACTIVATE:
      break;
    case MA_ACTIVATEANDEAT:
      IntMouseActivateWindow(MsgWindow);
      return TRUE;
    default:
      /* MA_ACTIVATE */
      IntMouseActivateWindow(MsgWindow);
      break;
  }
  
  return FALSE;
}

BOOL FASTCALL
IntTranslateMouseMessage(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg, USHORT *HitTest, BOOL Remove)
{
  PWINDOW_OBJECT Window;
  
  if(!(Window = IntGetWindowObject(Msg->hwnd)))
  {
    /* let's just eat the message?! */
    return TRUE;
  }
  
  if(ThreadQueue == Window->MessageQueue &&
     ThreadQueue->CaptureWindow != Window->Self)
  {
    /* only send WM_NCHITTEST messages if we're not capturing the window! */
    *HitTest = IntSendMessage(Window->Self, WM_NCHITTEST, 0, 
                              MAKELONG(Msg->pt.x, Msg->pt.y));
    
    if(*HitTest == (USHORT)HTTRANSPARENT)
    {
      PWINDOW_OBJECT DesktopWindow;
      HWND hDesktop = IntGetDesktopWindow();
      
      if((DesktopWindow = IntGetWindowObject(hDesktop)))
      {
        PWINDOW_OBJECT Wnd;
        
        WinPosWindowFromPoint(DesktopWindow, Window->MessageQueue, &Msg->pt, &Wnd);
        if(Wnd)
        {
          if(Wnd != Window)
          {
            /* post the message to the other window */
            Msg->hwnd = Wnd->Self;
            MsqPostMessage(Wnd->MessageQueue, Msg, FALSE);
            
            /* eat the message */
            IntReleaseWindowObject(Wnd);
            IntReleaseWindowObject(Window);
            IntReleaseWindowObject(DesktopWindow);
            return TRUE;
          }
	  IntReleaseWindowObject(Wnd); 
        }
        
        IntReleaseWindowObject(DesktopWindow);
      }
    }
  }
  else
  {
    *HitTest = HTCLIENT;
  }
  
  if(IS_BTN_MESSAGE(Msg->message, DOWN))
  {
    /* generate double click messages, if necessary */
    if ((((*HitTest) != HTCLIENT) || 
        (IntGetClassLong(Window, GCL_STYLE, FALSE) & CS_DBLCLKS)) &&
        MsqIsDblClk(Msg, Remove))
    {
      Msg->message += WM_LBUTTONDBLCLK - WM_LBUTTONDOWN;
    }
  }
  
  if(Msg->message != WM_MOUSEWHEEL)
  {
    
    if ((*HitTest) != HTCLIENT)
    {
      Msg->message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
      if((Msg->message == WM_NCRBUTTONUP) && 
         (((*HitTest) == HTCAPTION) || ((*HitTest) == HTSYSMENU)))
      {
        Msg->message = WM_CONTEXTMENU;
        Msg->wParam = (WPARAM)Window->Self;
      }
      else
      {
        Msg->wParam = *HitTest;
      }
      Msg->lParam = MAKELONG(Msg->pt.x, Msg->pt.y);
    }
    else if(ThreadQueue->MoveSize == NULL &&
            ThreadQueue->MenuOwner == NULL)
    {
      /* NOTE: Msg->pt should remain in screen coordinates. -- FiN */
      Msg->lParam = MAKELONG(
        Msg->pt.x - (WORD)Window->ClientRect.left,
        Msg->pt.y - (WORD)Window->ClientRect.top);
    }
  }
  
  IntReleaseWindowObject(Window);
  return FALSE;
}


/*
 * Internal version of PeekMessage() doing all the work
 */
BOOL FASTCALL
IntPeekMessage(PUSER_MESSAGE Msg,
                HWND Wnd,
                UINT MsgFilterMin,
                UINT MsgFilterMax,
                UINT RemoveMsg)
{
  LARGE_INTEGER LargeTickCount;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  PUSER_MESSAGE Message;
  BOOL Present, RemoveMessages;

  /* The queues and order in which they are checked are documented in the MSDN
     article on GetMessage() */

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  /* Inspect RemoveMsg flags */
  /* FIXME: The only flag we process is PM_REMOVE - processing of others must still be implemented */
  RemoveMessages = RemoveMsg & PM_REMOVE;
  
  CheckMessages:
  
  Present = FALSE;
  
  KeQueryTickCount(&LargeTickCount);
  ThreadQueue->LastMsgRead = LargeTickCount.u.LowPart;

  /* Dispatch sent messages here. */
  while (MsqDispatchOneSentMessage(ThreadQueue));
      
  /* Now look for a quit message. */
  
  if (ThreadQueue->QuitPosted)
  {
    /* According to the PSDK, WM_QUIT messages are always returned, regardless
       of the filter specified */
    Msg->Msg.hwnd = NULL;
    Msg->Msg.message = WM_QUIT;
    Msg->Msg.wParam = ThreadQueue->QuitExitCode;
    Msg->Msg.lParam = 0;
    Msg->FreeLParam = FALSE;
    if (RemoveMessages)
    {
      ThreadQueue->QuitPosted = FALSE;
    }
    return TRUE;
  }

  /* Now check for normal messages. */
  Present = MsqFindMessage(ThreadQueue,
                           FALSE,
                           RemoveMessages,
                           Wnd,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
  {
    RtlCopyMemory(Msg, Message, sizeof(USER_MESSAGE));
    if (RemoveMessages)
    {
      MsqDestroyMessage(Message);
    }
    goto MessageFound;
  }
  
  /* Check for hardware events. */
  Present = MsqFindMessage(ThreadQueue,
                           TRUE,
                           RemoveMessages,
                           Wnd,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
  {
    RtlCopyMemory(Msg, Message, sizeof(USER_MESSAGE));
    if (RemoveMessages)
    {
      MsqDestroyMessage(Message);
    }
    goto MessageFound;
  }
  
  /* Check for sent messages again. */
  while (MsqDispatchOneSentMessage(ThreadQueue));

  /* Check for paint messages. */
  if (IntGetPaintMessage(Wnd, MsgFilterMin, MsgFilterMax, PsGetWin32Thread(), &Msg->Msg, RemoveMessages))
  {
    Msg->FreeLParam = FALSE;
    return TRUE;
  }
  
  /* FIXME - get WM_(SYS)TIMER messages */
  
  if(Present)
  {
    MessageFound:
    
    if(RemoveMessages)
    {
      PWINDOW_OBJECT MsgWindow = NULL;;
      
      if(Msg->Msg.hwnd && (MsgWindow = IntGetWindowObject(Msg->Msg.hwnd)) &&
         Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST)
      {
        USHORT HitTest;
        
        if(IntTranslateMouseMessage(ThreadQueue, &Msg->Msg, &HitTest, TRUE))
          /* FIXME - check message filter again, if the message doesn't match anymore,
                     search again */
        {
          IntReleaseWindowObject(MsgWindow);
          /* eat the message, search again */
          goto CheckMessages;
        }
        if(ThreadQueue->CaptureWindow == NULL)
        {
          IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
          if((Msg->Msg.message != WM_MOUSEMOVE && Msg->Msg.message != WM_NCMOUSEMOVE) &&
             IS_BTN_MESSAGE(Msg->Msg.message, DOWN) &&
             IntActivateWindowMouse(ThreadQueue, &Msg->Msg, MsgWindow, &HitTest))
          {
            IntReleaseWindowObject(MsgWindow);
            /* eat the message, search again */
            goto CheckMessages;
          }
        }
      }
      else
      {
        IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
      }
      
      if(MsgWindow)
      {
        IntReleaseWindowObject(MsgWindow);
      }
      
      return TRUE;
    }
    
    USHORT HitTest;
    if((Msg->Msg.hwnd && Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST) &&
       IntTranslateMouseMessage(ThreadQueue, &Msg->Msg, &HitTest, FALSE))
      /* FIXME - check message filter again, if the message doesn't match anymore,
                 search again */
    {
      /* eat the message, search again */
      goto CheckMessages;
    }
    
    return TRUE;
  }

  return Present;
}

BOOL STDCALL
NtUserPeekMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND Wnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
  NTSTATUS Status;
  BOOL Present;
  NTUSERGETMESSAGEINFO Info;
  PWINDOW_OBJECT Window;
  PMSGMEMORY MsgMemoryEntry;
  PVOID UserMem;
  UINT Size;
  USER_MESSAGE Msg;

  /* Validate input */
  if (NULL != Wnd)
    {
      Window = IntGetWindowObject(Wnd);
      if (NULL == Window)
        {
          Wnd = NULL;
        }
      else
        {
          IntReleaseWindowObject(Window);
        }
    }

  if (MsgFilterMax < MsgFilterMin)
    {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
    }

  Present = IntPeekMessage(&Msg, Wnd, MsgFilterMin, MsgFilterMax, RemoveMsg);
  if (Present)
    {
      Info.Msg = Msg.Msg;
      /* See if this message type is present in the table */
      MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
      if (NULL == MsgMemoryEntry)
        {
          /* Not present, no copying needed */
          Info.LParamSize = 0;
        }
      else
        {
          /* Determine required size */
          Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam,
                               Info.Msg.lParam);
          /* Allocate required amount of user-mode memory */
          Info.LParamSize = Size;
          UserMem = NULL;
          Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                           &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return (BOOL) -1;
            }
          /* Transfer lParam data to user-mode mem */
          Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
          if (! NT_SUCCESS(Status))
            {
              ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                  &Info.LParamSize, MEM_DECOMMIT);
              SetLastNtError(Status);
              return (BOOL) -1;
            }
          Info.Msg.lParam = (LPARAM) UserMem;
        }
      if (Msg.FreeLParam && 0 != Msg.Msg.lParam)
        {
          ExFreePool((void *) Msg.Msg.lParam);
        }
      Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
      if (! NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return (BOOL) -1;
        }
    }

  return Present;
}

static BOOL FASTCALL
IntWaitMessage(HWND Wnd,
                UINT MsgFilterMin,
                UINT MsgFilterMax)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  NTSTATUS Status;
  USER_MESSAGE Msg;

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  do
    {
      if (IntPeekMessage(&Msg, Wnd, MsgFilterMin, MsgFilterMax, PM_NOREMOVE))
	{
	  return TRUE;
	}

      /* Nothing found. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (STATUS_WAIT_0 <= STATUS_WAIT_0 && Status <= STATUS_WAIT_63);

  SetLastNtError(Status);

  return FALSE;
}

BOOL STDCALL
NtUserGetMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
		 HWND Wnd,
		 UINT MsgFilterMin,
		 UINT MsgFilterMax)
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      UnsafeMsg - Pointer to the structure which receives the returned message.
 *      Wnd - Window whose messages are to be retrieved.
 *      MsgFilterMin - Integer value of the lowest message value to be
 *                     retrieved.
 *      MsgFilterMax - Integer value of the highest message value to be
 *                     retrieved.
 */
{
  BOOL GotMessage;
  NTUSERGETMESSAGEINFO Info;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;
  PMSGMEMORY MsgMemoryEntry;
  PVOID UserMem;
  UINT Size;
  USER_MESSAGE Msg;

  /* Validate input */
  if (NULL != Wnd)
    {
      Window = IntGetWindowObject(Wnd);
      if(!Window)
        Wnd = NULL;
      else
        IntReleaseWindowObject(Window);
    }
  if (MsgFilterMax < MsgFilterMin)
    {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
    }

  do
    {
      GotMessage = IntPeekMessage(&Msg, Wnd, MsgFilterMin, MsgFilterMax, PM_REMOVE);
      if (GotMessage)
	{
          Info.Msg = Msg.Msg;
          /* See if this message type is present in the table */
          MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
          if (NULL == MsgMemoryEntry)
            {
              /* Not present, no copying needed */
              Info.LParamSize = 0;
            }
          else
            {
              /* Determine required size */
              Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam,
                                   Info.Msg.lParam);
              /* Allocate required amount of user-mode memory */
              Info.LParamSize = Size;
              UserMem = NULL;
              Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                               &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);

              if (! NT_SUCCESS(Status))
                {
                  SetLastNtError(Status);
                  return (BOOL) -1;
                }
              /* Transfer lParam data to user-mode mem */
              Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
              if (! NT_SUCCESS(Status))
                {
                  ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                      &Info.LParamSize, MEM_DECOMMIT);
                  SetLastNtError(Status);
                  return (BOOL) -1;
                }
              Info.Msg.lParam = (LPARAM) UserMem;
            }
          if (Msg.FreeLParam && 0 != Msg.Msg.lParam)
            {
              ExFreePool((void *) Msg.Msg.lParam);
            }
	  Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
	  if (! NT_SUCCESS(Status))
	    {
	      SetLastNtError(Status);
	      return (BOOL) -1;
	    }
	}
      else
	{
	  IntWaitMessage(Wnd, MsgFilterMin, MsgFilterMax);
	}
    }
  while (! GotMessage);

  return WM_QUIT != Info.Msg.message;
}

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

static NTSTATUS FASTCALL
CopyMsgToKernelMem(MSG *KernelModeMsg, MSG *UserModeMsg, PMSGMEMORY MsgMemoryEntry)
{
  NTSTATUS Status;
  
  PVOID KernelMem;
  UINT Size;

  *KernelModeMsg = *UserModeMsg;

  /* See if this message type is present in the table */
  if (NULL == MsgMemoryEntry)
    {
      /* Not present, no copying needed */
      return STATUS_SUCCESS;
    }

  /* Determine required size */
  Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

  if (0 != Size)
    {
      /* Allocate kernel mem */
      KernelMem = ExAllocatePoolWithTag(PagedPool, Size, TAG_MSG);
      if (NULL == KernelMem)
        {
          DPRINT1("Not enough memory to copy message to kernel mem\n");
          return STATUS_NO_MEMORY;
        }
      KernelModeMsg->lParam = (LPARAM) KernelMem;

      /* Copy data if required */
      if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_READ))
        {
          Status = MmCopyFromCaller(KernelMem, (PVOID) UserModeMsg->lParam, Size);
          if (! NT_SUCCESS(Status))
            {
              DPRINT1("Failed to copy message to kernel: invalid usermode buffer\n");
              ExFreePool(KernelMem);
              return Status;
            }
        }
      else
        {
          /* Make sure we don't pass any secrets to usermode */
          RtlZeroMemory(KernelMem, Size);
        }
    }
  else
    {
      KernelModeMsg->lParam = 0;
    }

  return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
CopyMsgToUserMem(MSG *UserModeMsg, MSG *KernelModeMsg)
{
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;
  UINT Size;

  /* See if this message type is present in the table */
  MsgMemoryEntry = FindMsgMemory(UserModeMsg->message);
  if (NULL == MsgMemoryEntry)
    {
      /* Not present, no copying needed */
      return STATUS_SUCCESS;
    }

  /* Determine required size */
  Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

  if (0 != Size)
    {
      /* Copy data if required */
      if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_WRITE))
        {
          Status = MmCopyToCaller((PVOID) UserModeMsg->lParam, (PVOID) KernelModeMsg->lParam, Size);
          if (! NT_SUCCESS(Status))
            {
              DPRINT1("Failed to copy message from kernel: invalid usermode buffer\n");
              ExFreePool((PVOID) KernelModeMsg->lParam);
              return Status;
            }
        }

      ExFreePool((PVOID) KernelModeMsg->lParam);
    }

  return STATUS_SUCCESS;
}

BOOL STDCALL
NtUserPostMessage(HWND Wnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  PWINDOW_OBJECT Window;
  MSG UserModeMsg, KernelModeMsg;
  LARGE_INTEGER LargeTickCount;
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;

  if (WM_QUIT == Msg)
    {
      MsqPostQuitMessage(PsGetWin32Thread()->MessageQueue, wParam);
    }
  else if (Wnd == HWND_BROADCAST)
    {
      HWND *List;
      PWINDOW_OBJECT DesktopWindow;
      ULONG i;

      DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);
      IntReleaseWindowObject(DesktopWindow);
      if (List != NULL)
        {
          for (i = 0; List[i]; i++)
            NtUserPostMessage(List[i], Msg, wParam, lParam);
          ExFreePool(List);
        }      
    }
  else
    {
      PSYSTEM_CURSORINFO CurInfo;
      Window = IntGetWindowObject(Wnd);
      if (NULL == Window)
        {
          SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
          return FALSE;
        }

      UserModeMsg.hwnd = Wnd;
      UserModeMsg.message = Msg;
      UserModeMsg.wParam = wParam;
      UserModeMsg.lParam = lParam;
      MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
      Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
      if (! NT_SUCCESS(Status))
        {
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return FALSE;
        }
      CurInfo = IntGetSysCursorInfo(PsGetWin32Process()->WindowStation);
      KernelModeMsg.pt.x = CurInfo->x;
      KernelModeMsg.pt.y = CurInfo->y;
      KeQueryTickCount(&LargeTickCount);
      KernelModeMsg.time = LargeTickCount.u.LowPart;
      MsqPostMessage(Window->MessageQueue, &KernelModeMsg,
                     NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam);
      IntReleaseWindowObject(Window);
    }

  return TRUE;
}

BOOL STDCALL
NtUserPostThreadMessage(DWORD idThread,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  MSG UserModeMsg, KernelModeMsg;
  PETHREAD peThread;
  PW32THREAD pThread;
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;

  Status = PsLookupThreadByThreadId((void *)idThread,&peThread);
  
  if( Status == STATUS_SUCCESS ) {
    pThread = peThread->Win32Thread;
    if( !pThread || !pThread->MessageQueue )
      {
	ObDereferenceObject( peThread );
	return FALSE;
      }

    UserModeMsg.hwnd = NULL;
    UserModeMsg.message = Msg;
    UserModeMsg.wParam = wParam;
    UserModeMsg.lParam = lParam;
    MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
    Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
    if (! NT_SUCCESS(Status))
      {
        ObDereferenceObject( peThread );
	SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
      }
    MsqPostMessage(pThread->MessageQueue, &KernelModeMsg,
                   NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam);
    ObDereferenceObject( peThread );
    return TRUE;
  } else {
    SetLastNtError( Status );
    return FALSE;
  }
}

DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
  UNIMPLEMENTED;

  return 0;
}

LRESULT FASTCALL
IntSendMessage(HWND hWnd,
               UINT Msg,
               WPARAM wParam,
               LPARAM lParam)
{
  ULONG_PTR Result = 0;
  if(IntSendMessageTimeout(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
  {
    return (LRESULT)Result;
  }
  return 0;
}

static LRESULT FASTCALL
IntSendMessageTimeoutSingle(HWND hWnd,
                            UINT Msg,
                            WPARAM wParam,
                            LPARAM lParam,
                            UINT uFlags,
                            UINT uTimeout,
                            ULONG_PTR *uResult)
{
  ULONG_PTR Result;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;
  PMSGMEMORY MsgMemoryEntry;
  INT lParamBufferSize;
  LPARAM lParamPacked;
  PW32THREAD Win32Thread;

  /* FIXME: Call hooks. */
  Window = IntGetWindowObject(hWnd);
  if (!Window)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
    }

  Win32Thread = PsGetWin32Thread();

  if (NULL != Win32Thread &&
      Window->MessageQueue == Win32Thread->MessageQueue)
    {
      if (Win32Thread->IsExiting)
        {
          /* Never send messages to exiting threads */
          IntReleaseWindowObject(Window);
          return FALSE;
        }

      /* See if this message type is present in the table */
      MsgMemoryEntry = FindMsgMemory(Msg);
      if (NULL == MsgMemoryEntry)
        {
          lParamBufferSize = -1;
        }
      else
        {
          lParamBufferSize = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
        }

      if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam)))
        {
          IntReleaseWindowObject(Window);
          DPRINT1("Failed to pack message parameters\n");
          return FALSE;
        }
      if (0xFFFF0000 != ((DWORD) Window->WndProcW & 0xFFFF0000))
        {
          Result = (ULONG_PTR)IntCallWindowProc(Window->WndProcW, FALSE, hWnd, Msg, wParam,
                                                lParamPacked,lParamBufferSize);
        }
      else
        {
          Result = (ULONG_PTR)IntCallWindowProc(Window->WndProcA, TRUE, hWnd, Msg, wParam,
                                                lParamPacked,lParamBufferSize);
        }
      
      if(uResult)
      {
        *uResult = Result;
      }
      
      if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam)))
        {
          IntReleaseWindowObject(Window);
          DPRINT1("Failed to unpack message parameters\n");
          return TRUE;
        }

      IntReleaseWindowObject(Window);
      return TRUE;
    }
  
  if(uFlags & SMTO_ABORTIFHUNG && MsqIsHung(Window->MessageQueue))
  {
    IntReleaseWindowObject(Window);
    /* FIXME - Set a LastError? */
    return FALSE;
  }
  
  Status = MsqSendMessage(Window->MessageQueue, hWnd, Msg, wParam, lParam, 
                          uTimeout, (uFlags & SMTO_BLOCK), uResult);
  if(Status == STATUS_TIMEOUT)
  {
    IntReleaseWindowObject(Window);
    SetLastWin32Error(ERROR_TIMEOUT);
    return FALSE;
  }

  IntReleaseWindowObject(Window);
  return TRUE;
}

LRESULT FASTCALL
IntSendMessageTimeout(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult)
{
  PWINDOW_OBJECT DesktopWindow;
  HWND *Children;
  HWND *Child;

  if (HWND_BROADCAST != hWnd)
    {
      return IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

  DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
  if (NULL == DesktopWindow)
    {
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return 0;
    }
  Children = IntWinListChildren(DesktopWindow);
  IntReleaseWindowObject(DesktopWindow);
  if (NULL == Children)
    {
      return 0;
    }

  for (Child = Children; NULL != *Child; Child++)
    {
      IntSendMessageTimeoutSingle(*Child, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

  ExFreePool(Children);

  return (LRESULT) TRUE;
}


/* This function posts a message if the destination's message queue belongs to
   another thread, otherwise it sends the message. It does not support broadcast
   messages! */
LRESULT FASTCALL
IntPostOrSendMessage(HWND hWnd,
                     UINT Msg,
                     WPARAM wParam,
                     LPARAM lParam)
{
  ULONG_PTR Result;
  PWINDOW_OBJECT Window;
  
  if(hWnd == HWND_BROADCAST)
  {
    return 0;
  }
  
  Window = IntGetWindowObject(hWnd);
  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  if(Window->MessageQueue != PsGetWin32Thread()->MessageQueue)
  {
    Result = NtUserPostMessage(hWnd, Msg, wParam, lParam);
  }
  else
  {
    if(!IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
    {
      Result = 0;
    }
  }
  
  IntReleaseWindowObject(Window);
  
  return (LRESULT)Result;
}

LRESULT FASTCALL
IntDoSendMessage(HWND Wnd,
		 UINT Msg,
		 WPARAM wParam,
		 LPARAM lParam,
		 PDOSENDMESSAGE dsm,
	         PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
  LRESULT Result = TRUE;
  NTSTATUS Status;
  PWINDOW_OBJECT Window;
  NTUSERSENDMESSAGEINFO Info;
  MSG UserModeMsg;
  MSG KernelModeMsg;
  PMSGMEMORY MsgMemoryEntry;

  RtlZeroMemory(&Info, sizeof(NTUSERSENDMESSAGEINFO));

  /* FIXME: Call hooks. */
  if (HWND_BROADCAST != Wnd)
    {
      Window = IntGetWindowObject(Wnd);
      if (NULL == Window)
        {
          /* Tell usermode to not touch this one */
          Info.HandledByKernel = TRUE;
          MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
          SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
          return 0;
        }
    }

  /* FIXME: Check for an exiting window. */

  /* See if the current thread can handle the message */
  if (HWND_BROADCAST != Wnd && NULL != PsGetWin32Thread() &&
      Window->MessageQueue == PsGetWin32Thread()->MessageQueue)
    {
      /* Gather the information usermode needs to call the window proc directly */
      Info.HandledByKernel = FALSE;
      if (0xFFFF0000 != ((DWORD) Window->WndProcW & 0xFFFF0000))
        {
          if (0xFFFF0000 != ((DWORD) Window->WndProcA & 0xFFFF0000))
            {
              /* Both Unicode and Ansi winprocs are real, see what usermode prefers */
              Status = MmCopyFromCaller(&(Info.Ansi), &(UnsafeInfo->Ansi),
                                        sizeof(BOOL));
              if (! NT_SUCCESS(Status))
                {
                  Info.Ansi = ! Window->Unicode;
                }
              Info.Proc = (Info.Ansi ? Window->WndProcA : Window->WndProcW);
            }
          else
            {
              /* Real Unicode winproc */
              Info.Ansi = FALSE;
              Info.Proc = Window->WndProcW;
            }
        }
      else
        {
          /* Must have real Ansi winproc */
          Info.Ansi = TRUE;
          Info.Proc = Window->WndProcA;
        }
      IntReleaseWindowObject(Window);
    }
  else
    {
      /* Must be handled by other thread */
      if (HWND_BROADCAST != Wnd)
        {
          IntReleaseWindowObject(Window);
        }
      Info.HandledByKernel = TRUE;
      UserModeMsg.hwnd = Wnd;
      UserModeMsg.message = Msg;
      UserModeMsg.wParam = wParam;
      UserModeMsg.lParam = lParam;
      MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
      Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
      if (! NT_SUCCESS(Status))
        {
          MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return (dsm ? 0 : -1);
        }
      if(!dsm)
      {
        Result = IntSendMessage(KernelModeMsg.hwnd, KernelModeMsg.message,
                                KernelModeMsg.wParam, KernelModeMsg.lParam);
      }
      else
      {
        Result = IntSendMessageTimeout(KernelModeMsg.hwnd, KernelModeMsg.message,
                                       KernelModeMsg.wParam, KernelModeMsg.lParam,
                                       dsm->uFlags, dsm->uTimeout, &dsm->Result);
      }
      Status = CopyMsgToUserMem(&UserModeMsg, &KernelModeMsg);
      if (! NT_SUCCESS(Status))
        {
          MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return(dsm ? 0 : -1);
        }
    }

  Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
  if (! NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
    }

  return (LRESULT)Result;
}

LRESULT STDCALL
NtUserSendMessageTimeout(HWND hWnd,
			 UINT Msg,
			 WPARAM wParam,
			 LPARAM lParam,
			 UINT uFlags,
			 UINT uTimeout,
			 ULONG_PTR *uResult,
	                 PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
  DOSENDMESSAGE dsm;
  LRESULT Result;

  dsm.uFlags = uFlags;
  dsm.uTimeout = uTimeout;
  Result = IntDoSendMessage(hWnd, Msg, wParam, lParam, &dsm, UnsafeInfo);
  if(uResult != NULL && Result != 0)
  {
    NTSTATUS Status;
    
    Status = MmCopyToCaller(uResult, &dsm.Result, sizeof(ULONG_PTR));
    if(!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  }
  return Result;
}

LRESULT STDCALL
NtUserSendMessage(HWND Wnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam,
                  PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
  return IntDoSendMessage(Wnd, Msg, wParam, lParam, NULL, UnsafeInfo);
}

BOOL STDCALL
NtUserSendMessageCallback(HWND hWnd,
			  UINT Msg,
			  WPARAM wParam,
			  LPARAM lParam,
			  SENDASYNCPROC lpCallBack,
			  ULONG_PTR dwData)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserWaitMessage(VOID)
{

  return IntWaitMessage(NULL, 0, 0);
}

DWORD STDCALL
NtUserGetQueueStatus(BOOL ClearChanges)
{
   PUSER_MESSAGE_QUEUE Queue;
   DWORD Result;

   Queue = PsGetWin32Thread()->MessageQueue;

   IntLockMessageQueue(Queue);

   Result = MAKELONG(Queue->ChangedBits, Queue->WakeBits);
   if (ClearChanges)
   {
      Queue->ChangedBits = 0;
   }

   IntUnLockMessageQueue(Queue);

   return Result;
}

BOOL STDCALL
IntInitMessagePumpHook()
{
	PsGetCurrentThread()->Win32Thread->MessagePumpHookValue++;
	return TRUE;
}

BOOL STDCALL
IntUninitMessagePumpHook()
{
	if (PsGetCurrentThread()->Win32Thread->MessagePumpHookValue <= 0)
	{
		return FALSE;
	}
	PsGetCurrentThread()->Win32Thread->MessagePumpHookValue--;
	return TRUE;
}

/* EOF */
