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
/* $Id: message.c,v 1.71.4.7 2004/09/23 19:42:30 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#define MMS_SIZE_WPARAM      -1
#define MMS_SIZE_WPARAMWCHAR -2
#define MMS_SIZE_LPARAMSZ    -3
#define MMS_SIZE_SPECIAL     -4
#define MMS_FLAG_READ        0x01
#define MMS_FLAG_WRITE       0x02
#define MMS_FLAG_READWRITE   (MMS_FLAG_READ | MMS_FLAG_WRITE)

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

PMSGMEMORY INTERNAL_CALL
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

UINT INTERNAL_CALL
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

static NTSTATUS INTERNAL_CALL
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

static NTSTATUS INTERNAL_CALL
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


BOOL INTERNAL_CALL
IntTranslateMouseMessage(PUSER_MESSAGE_QUEUE ThreadQueue, PKMSG Msg, USHORT *HitTest, BOOL Remove)
{
  PWINDOW_OBJECT Window;
  
  if(Msg->Window == NULL)
  {
    /* let's just eat the message?! */
    return TRUE;
  }
  
  if(ThreadQueue == Window->MessageQueue &&
     ThreadQueue->CaptureWindow != Window)
  {
    /* only send WM_NCHITTEST messages if we're not capturing the window! */
    *HitTest = IntSendMessage(Window, WM_NCHITTEST, 0, 
                              MAKELONG(Msg->pt.x, Msg->pt.y));
    
    /* FIXME - make sure the window still exists! */
    
    if(*HitTest == (USHORT)HTTRANSPARENT)
    {
      PWINDOW_OBJECT DesktopWindow;
      
      if((DesktopWindow = IntGetDesktopWindow()))
      {
        PWINDOW_OBJECT Wnd;
        
        WinPosWindowFromPoint(DesktopWindow, Window->MessageQueue, &Msg->pt, &Wnd);
        if(Wnd && (Wnd != Window))
        {
          /* post the message to the other window */
          Msg->Window = Wnd;
          MsqPostMessage(Wnd->MessageQueue, Msg, FALSE);
          
          /* eat the message */
          return TRUE;
        }
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
        Msg->wParam = (WPARAM)Window->Handle;
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
  
  return FALSE;
}


VOID INTERNAL_CALL
IntSendHitTestMessages(PUSER_MESSAGE_QUEUE ThreadQueue, PKMSG Msg)
{
  PWINDOW_OBJECT Window;
  
  if(!(Window = Msg->Window) || ThreadQueue->CaptureWindow)
  {
    return;
  }
  
  switch(Msg->message)
  {
    case WM_MOUSEMOVE:
    {
      IntSendMessage(Window, WM_SETCURSOR, (WPARAM)Window->Handle, MAKELPARAM(HTCLIENT, Msg->message));
      break;
    }
    case WM_NCMOUSEMOVE:
    {
      IntSendMessage(Window, WM_SETCURSOR, (WPARAM)Window->Handle, MAKELPARAM(Msg->wParam, Msg->message));
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
      
      IntSendMessage(Window, WM_MOUSEMOVE, wParam, Msg->lParam);
      IntSendMessage(Window, WM_SETCURSOR, (WPARAM)Window->Handle, MAKELPARAM(HTCLIENT, Msg->message));
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
      IntSendMessage(Window, WM_NCMOUSEMOVE, (WPARAM)Msg->wParam, Msg->lParam);
      IntSendMessage(Window, WM_SETCURSOR, (WPARAM)Window->Handle, MAKELPARAM(Msg->wParam, Msg->message));
      break;
    }
  }
}


BOOL INTERNAL_CALL
IntActivateWindowMouse(PUSER_MESSAGE_QUEUE ThreadQueue, PKMSG Msg, USHORT *HitTest)
{
  PWINDOW_OBJECT Parent;
  ULONG Result;
  
  if(*HitTest == (USHORT)HTTRANSPARENT)
  {
    /* eat the message, search again! */
    return TRUE;
  }
  
  Parent = IntGetParent(Msg->Window);
  
  Result = IntSendMessage(Msg->Window, WM_MOUSEACTIVATE, (WPARAM)(Parent ? Parent->Handle : 0), (LPARAM)MAKELONG(*HitTest, Msg->message));
  /* FIXME - make sure the window still exists */
  switch (Result)
  {
    case MA_NOACTIVATEANDEAT:
      return TRUE;
    case MA_NOACTIVATE:
      break;
    case MA_ACTIVATEANDEAT:
      IntMouseActivateWindow(IntGetDesktopWindow(), Msg->Window);
      return TRUE;
    default:
      /* MA_ACTIVATE */
      IntMouseActivateWindow(IntGetDesktopWindow(), Msg->Window);
      break;
  }
  
  return FALSE;
}

LRESULT INTERNAL_CALL
IntDispatchMessage(PNTUSERDISPATCHMESSAGEINFO MsgInfo, PWINDOW_OBJECT Window)
{
  if((MsgInfo->Msg.message == WM_TIMER || MsgInfo->Msg.message == WM_SYSTIMER) &&
     MsgInfo->Msg.lParam != 0)
  {
    LARGE_INTEGER LargeTickCount;
    
    /* FIXME - call hooks? */
    MsgInfo->HandledByKernel = FALSE;
    KeQueryTickCount(&LargeTickCount);
    MsgInfo->Proc = (WNDPROC) MsgInfo->Msg.lParam;
    MsgInfo->Msg.lParam = (LPARAM)LargeTickCount.u.LowPart;
    /* FIXME - We should propably handle WM_SYSTIMER messages in kmode first (for carets etc)
               and then pass to umode */
    return 0;
  }
  
  if(Window == NULL)
  {
    /* Nothing to do for messages to threads, no need to dispatch them */
    MsgInfo->HandledByKernel = TRUE;
    return 0;
  }
  
  /* FIXME - call hooks? */
  
  MsgInfo->HandledByKernel = FALSE;
  if(((DWORD) Window->WndProcW & 0xFFFF0000) != 0xFFFF0000)
  {
    if(((DWORD) Window->WndProcA & 0xFFFF0000) != 0xFFFF0000)
    {
      /* Both Unicode and Ansi winprocs are real, use whatever
         usermode prefers */
      MsgInfo->Proc = (MsgInfo->Ansi ? Window->WndProcA : Window->WndProcW);
    }
    else
    {
      /* Real Unicode winproc */
      MsgInfo->Ansi = FALSE;
      MsgInfo->Proc = Window->WndProcW;
    }
  }
  else
  {
    /* Must have real Ansi winproc */
    MsgInfo->Ansi = TRUE;
    MsgInfo->Proc = Window->WndProcA;
  }
  
  return 0;
}


/*
 * Internal version of PeekMessage() doing all the work
 */
BOOL INTERNAL_CALL
IntPeekMessage(PUSER_MESSAGE Msg,
               PWINDOW_OBJECT FilterWindow,
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
    Msg->Msg.Window = NULL;
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
                           FilterWindow,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
  {
    *Msg = *Message;
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
                           FilterWindow,
                           MsgFilterMin,
                           MsgFilterMax,
                           &Message);
  if (Present)
  {
    *Msg = *Message;
    if (RemoveMessages)
    {
      MsqDestroyMessage(Message);
    }
    goto MessageFound;
  }
  
  /* Check for sent messages again. */
  while (MsqDispatchOneSentMessage(ThreadQueue));

  /* Check for paint messages. */
  if (IntGetPaintMessage(FilterWindow, MsgFilterMin, MsgFilterMax, PsGetWin32Thread(), &Msg->Msg, RemoveMessages))
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
      if(Msg->Msg.Window &&
         Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST)
      {
        USHORT HitTest;
        
        if(IntTranslateMouseMessage(ThreadQueue, &Msg->Msg, &HitTest, TRUE))
          /* FIXME - check message filter again, if the message doesn't match anymore,
                     search again */
        {
          /* eat the message, search again */
          goto CheckMessages;
        }
        if(ThreadQueue->CaptureWindow == NULL)
        {
          IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
          if((Msg->Msg.message != WM_MOUSEMOVE && Msg->Msg.message != WM_NCMOUSEMOVE) &&
             IS_BTN_MESSAGE(Msg->Msg.message, DOWN) &&
             IntActivateWindowMouse(ThreadQueue, &Msg->Msg, &HitTest))
          {
            /* eat the message, search again */
            goto CheckMessages;
          }
        }
      }
      else
      {
        IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
      }
      
      return TRUE;
    }
    
    USHORT HitTest;
    if((Msg->Msg.Window &&
        Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST) &&
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


BOOL INTERNAL_CALL
IntWaitMessage(PWINDOW_OBJECT Window,
                UINT MsgFilterMin,
                UINT MsgFilterMax)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  NTSTATUS Status;
  USER_MESSAGE Msg;

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  do
    {
      if (IntPeekMessage(&Msg, Window, MsgFilterMin, MsgFilterMax, PM_NOREMOVE))
	{
	  return TRUE;
	}

      /* Nothing found. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (STATUS_WAIT_0 <= Status && Status <= STATUS_WAIT_63);

  SetLastNtError(Status);

  return FALSE;
}


BOOL INTERNAL_CALL
IntGetMessage(PUSER_MESSAGE Msg, PWINDOW_OBJECT FilterWindow,
              UINT MsgFilterMin, UINT MsgFilterMax, BOOL *GotMessage)
{
  ASSERT(Msg);
  
  /* Fix the filter */
  if(MsgFilterMax < MsgFilterMin)
  {
    MsgFilterMin = 0;
    MsgFilterMax = 0;
  }
  
  do
  {
    *GotMessage = IntPeekMessage(Msg, FilterWindow, MsgFilterMin, MsgFilterMax, PM_REMOVE);
    if(!(*GotMessage))
    {
      if(!IntWaitMessage(FilterWindow, MsgFilterMin, MsgFilterMax))
      {
        return (BOOL)-1;
      }
    }
  } while(!GotMessage);
  
  return WM_QUIT != Msg->Msg.message;
}


NTSTATUS INTERNAL_CALL
CopyMsgToKernelMem(PKMSG KernelModeMsg, MSG *UserModeMsg, PMSGMEMORY MsgMemoryEntry,
                   PWINDOW_OBJECT MsgWindow)
{
  NTSTATUS Status;
  
  PVOID KernelMem;
  UINT Size;

  MsgCopyMsgToKMsg(KernelModeMsg, UserModeMsg, MsgWindow);

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

NTSTATUS INTERNAL_CALL
CopyMsgToUserMem(MSG *UserModeMsg, PKMSG KernelModeMsg)
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

LRESULT INTERNAL_CALL
IntSendMessage(PWINDOW_OBJECT Window,
               UINT Msg,
               WPARAM wParam,
               LPARAM lParam)
{
  ULONG_PTR Result = 0;
  if(IntSendMessageTimeout(Window, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
  {
    return (LRESULT)Result;
  }
  return 0;
}

LRESULT INTERNAL_CALL
IntSendMessageTimeout(PWINDOW_OBJECT Window,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult)
{
  ULONG_PTR Result;
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;
  INT lParamBufferSize;
  LPARAM lParamPacked;
  PW32THREAD Win32Thread;

  ASSERT(Window);
  
  if(!IntIsWindow(Window) ||
     Window->MessageQueue->Thread->Win32Thread->IsExiting)
  {
    /* Never send messages to exiting threads */
    return FALSE;
  }

  Win32Thread = PsGetWin32Thread();
  
  ASSERT(Win32Thread != NULL);

  if (Window->MessageQueue == Win32Thread->MessageQueue)
    {
      /* This case will only happen if IntSendMessageTimeout() was called from
         inside kmode, NtUserSendMessageTimeout() will NOT call this function
         in this case, as it returns to umode where the window message callback
         function will be called! */

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
          DPRINT1("Failed to pack message parameters\n");
          return FALSE;
        }
      
      ObmReferenceObject(Window);
      if (0xFFFF0000 != ((DWORD) Window->WndProcW & 0xFFFF0000))
        {
          Result = (ULONG_PTR)IntCallWindowProc(Window->WndProcW, FALSE, Window, Msg, wParam,
                                                lParamPacked,lParamBufferSize);
        }
      else
        {
          Result = (ULONG_PTR)IntCallWindowProc(Window->WndProcA, TRUE, Window, Msg, wParam,
                                                lParamPacked,lParamBufferSize);
        }
      
      ObmDereferenceObject(Window);
      
      if(uResult)
      {
        *uResult = Result;
      }
      
      if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam)))
        {
          DPRINT1("Failed to unpack message parameters\n");
          return TRUE;
        }

      return TRUE;
    }
  
  if(uFlags & SMTO_ABORTIFHUNG && MsqIsHung(Window->MessageQueue))
  {
    /* FIXME - Set a LastError? */
    return FALSE;
  }
  
  Status = MsqSendMessage(Window->MessageQueue, Window, Msg, wParam, lParam, 
                          uTimeout, (uFlags & SMTO_BLOCK), uResult);
  if(Status == STATUS_TIMEOUT)
  {
    /* MSDN says GetLastError() should return 0 after timeout */
    SetLastWin32Error(0);
    return FALSE;
  }
  else if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return TRUE;
}

BOOL INTERNAL_CALL
IntPostThreadMessage(PW32THREAD W32Thread,
                     UINT Msg,
                     WPARAM wParam,
                     LPARAM lParam)
{
  MSG UserModeMsg;
  KMSG KernelModeMsg;
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;

  UserModeMsg.hwnd = NULL;
  UserModeMsg.message = Msg;
  UserModeMsg.wParam = wParam;
  UserModeMsg.lParam = lParam;

  MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
  /* FIXME - what if MsgMemoryEntry == NULL? */
  Status = CopyMsgToKernelMem(&KernelModeMsg,
                              &UserModeMsg,
                              MsgMemoryEntry,
                              NULL); /* pass NULL as the message window so it can be set in the KMSG struct */
  if(!NT_SUCCESS(Status))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  ASSERT(W32Thread->MessageQueue); /* FIXME - we should propably check this and fail gracefully */
  
  MsqPostMessage(W32Thread->MessageQueue, &KernelModeMsg,
                 NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam);

  return TRUE;
}

BOOL INTERNAL_CALL
IntPostMessage(PWINDOW_OBJECT Window,
               UINT Msg,
               WPARAM wParam,
               LPARAM lParam)
{
  MSG UserModeMsg;
  KMSG KernelModeMsg;
  NTSTATUS Status;
  PMSGMEMORY MsgMemoryEntry;
  
  if(Window == NULL)
  {
    /* Broadcast the message to all top level windows */

    PWINDOW_OBJECT Wnd, Desktop;
    
    if(!(Desktop = IntGetDesktopWindow()))
    {
      DPRINT1("Failed to broadcast message 0x%x! No desktop window found!\n", Msg);
      return FALSE;
    }
    
    for(Wnd = Desktop->FirstChild; Wnd != NULL; Wnd = Wnd->NextSibling)
    {
      IntPostMessage(Wnd,
                     Msg,
                     wParam,
                     lParam);
    }

    return TRUE; /* FIXME - return also TRUE if no top-level windows are present?! */
  }

  UserModeMsg.hwnd = (Window != NULL ? Window->Handle : NULL);
  UserModeMsg.message = Msg;
  UserModeMsg.wParam = wParam;
  UserModeMsg.lParam = lParam;

  MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
  /* FIXME - what if MsgMemoryEntry == NULL? */
  Status = CopyMsgToKernelMem(&KernelModeMsg,
                              &UserModeMsg,
                              MsgMemoryEntry,
                              Window); /* pass the message window so it can be set in the KMSG struct */
  if(!NT_SUCCESS(Status))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  MsqPostMessage(Window->MessageQueue, &KernelModeMsg,
                 NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam);

  return TRUE;
}

BOOL INTERNAL_CALL
IntInitMessagePumpHook()
{
	PsGetCurrentThread()->Win32Thread->MessagePumpHookValue++;
	return TRUE;
}

BOOL INTERNAL_CALL
IntUninitMessagePumpHook()
{
	if (PsGetCurrentThread()->Win32Thread->MessagePumpHookValue <= 0)
	{
		return FALSE;
	}
	PsGetCurrentThread()->Win32Thread->MessagePumpHookValue--;
	return TRUE;
}


