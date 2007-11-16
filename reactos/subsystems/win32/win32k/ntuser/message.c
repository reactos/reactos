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
/* $Id$
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
}
DOSENDMESSAGE, *PDOSENDMESSAGE;

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
}
MSGMEMORY, *PMSGMEMORY;

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
      { WM_COPYDATA, MMS_SIZE_SPECIAL, MMS_FLAG_READ },
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
   UINT Size = 0;

   _SEH_TRY
   {
      if (MMS_SIZE_WPARAM == MsgMemoryEntry->Size)
      {
         Size = (UINT)wParam;
      }
      else if (MMS_SIZE_WPARAMWCHAR == MsgMemoryEntry->Size)
      {
         Size = (UINT) (wParam * sizeof(WCHAR));
      }
      else if (MMS_SIZE_LPARAMSZ == MsgMemoryEntry->Size)
      {
         Size = (UINT) ((wcslen((PWSTR) lParam) + 1) * sizeof(WCHAR));
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
               break;

            case WM_NCCALCSIZE:
               Size = wParam ? sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) : sizeof(RECT);
               break;

            case WM_COPYDATA:
               Size = sizeof(COPYDATASTRUCT) + ((PCOPYDATASTRUCT)lParam)->cbData;
               break;

            default:
               assert(FALSE);
               Size = 0;
               break;
         }
      }
      else
      {
         Size = MsgMemoryEntry->Size;
      }
   }
   _SEH_HANDLE
   {
      DPRINT1("Exception caught in MsgMemorySize()! Status: 0x%x\n", _SEH_GetExceptionCode());
      Size = 0;
   }
   _SEH_END;
   return Size;
}

static NTSTATUS
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

static NTSTATUS
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

BOOL
STDCALL
NtUserCallMsgFilter(
   LPMSG msg,
   INT code)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserCallMsgFilter\n");
   UserEnterExclusive();

   if (co_HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg))
      RETURN( TRUE);
   RETURN( co_HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg));

CLEANUP:
   DPRINT("Leave NtUserCallMsgFilter. ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}

LRESULT STDCALL
NtUserDispatchMessage(PNTUSERDISPATCHMESSAGEINFO UnsafeMsgInfo)
{
   NTSTATUS Status;
   NTUSERDISPATCHMESSAGEINFO MsgInfo;
   LRESULT Result = TRUE;
   DECLARE_RETURN(LRESULT);

   DPRINT("Enter NtUserDispatchMessage\n");
   UserEnterExclusive();

   Status = MmCopyFromCaller(&MsgInfo, UnsafeMsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
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
      PWINDOW_OBJECT Window;

      /* Get the window object. */
      Window = UserGetWindowObject(MsgInfo.Msg.hwnd);
      if (NULL == Window)
      {
         MsgInfo.HandledByKernel = TRUE;
         Result = 0;
      }
      else
      {
         if (Window->OwnerThread != PsGetCurrentThread())
         {
            DPRINT1("Window doesn't belong to the calling thread!\n");
            MsgInfo.HandledByKernel = TRUE;
            Result = 0;
         }
         else
         {
            /* FIXME: Call hook procedures. */

            MsgInfo.HandledByKernel = FALSE;
            Result = 0;

            if (Window->IsSystem)
            {
                MsgInfo.Proc = (!MsgInfo.Ansi ? Window->WndProc : Window->WndProcExtra);
            }
            else
            {
                MsgInfo.Ansi = !Window->Wnd->Unicode;
                MsgInfo.Proc = Window->WndProc;
            }
         }
      }
   }
   Status = MmCopyToCaller(UnsafeMsgInfo, &MsgInfo, sizeof(NTUSERDISPATCHMESSAGEINFO));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserDispatchMessage. ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL STDCALL
NtUserTranslateMessage(LPMSG lpMsg,
                       HKL dwhkl)
{
   NTSTATUS Status;
   MSG SafeMsg;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserTranslateMessage\n");
   UserEnterExclusive();

   Status = MmCopyFromCaller(&SafeMsg, lpMsg, sizeof(MSG));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( IntTranslateKbdMessage(&SafeMsg, dwhkl));

CLEANUP:
   DPRINT("Leave NtUserTranslateMessage: ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


VOID FASTCALL
co_IntSendHitTestMessages(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg)
{
   if(!Msg->hwnd || ThreadQueue->CaptureWindow)
   {
      return;
   }

   switch(Msg->message)
   {
      case WM_MOUSEMOVE:
         {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
            break;
         }
      case WM_NCMOUSEMOVE:
         {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
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

            co_IntSendMessage(Msg->hwnd, WM_MOUSEMOVE, wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
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
            co_IntSendMessage(Msg->hwnd, WM_NCMOUSEMOVE, (WPARAM)Msg->wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
            break;
         }
   }
}

BOOL FASTCALL
co_IntActivateWindowMouse(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg, PWINDOW_OBJECT MsgWindow,
                          USHORT *HitTest)
{
   ULONG Result;
   PWINDOW_OBJECT Parent;

   ASSERT_REFS_CO(MsgWindow);

   if(*HitTest == (USHORT)HTTRANSPARENT)
   {
      /* eat the message, search again! */
      return TRUE;
   }

   Parent = IntGetParent(MsgWindow);//fixme: deref retval?
   /* fixme: abort if no parent ? */
   Result = co_IntSendMessage(MsgWindow->hSelf,
                              WM_MOUSEACTIVATE,
                              (WPARAM) (Parent ? Parent->hSelf : NULL),
                              (LPARAM)MAKELONG(*HitTest, Msg->message)
                             );

   switch (Result)
   {
      case MA_NOACTIVATEANDEAT:
         return TRUE;
      case MA_NOACTIVATE:
         break;
      case MA_ACTIVATEANDEAT:
         co_IntMouseActivateWindow(MsgWindow);
         return TRUE;
      default:
         /* MA_ACTIVATE */
         co_IntMouseActivateWindow(MsgWindow);
         break;
   }

   return FALSE;
}

BOOL FASTCALL
co_IntTranslateMouseMessage(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg, USHORT *HitTest, BOOL Remove)
{
   PWINDOW_OBJECT Window;
   USER_REFERENCE_ENTRY Ref, DesktopRef;

   if(!(Window = UserGetWindowObject(Msg->hwnd)))
   {
      /* let's just eat the message?! */
      return TRUE;
   }

   UserRefObjectCo(Window, &Ref);

   if(ThreadQueue == Window->MessageQueue &&
         ThreadQueue->CaptureWindow != Window->hSelf)
   {
      /* only send WM_NCHITTEST messages if we're not capturing the window! */
      *HitTest = co_IntSendMessage(Window->hSelf, WM_NCHITTEST, 0,
                                   MAKELONG(Msg->pt.x, Msg->pt.y));

      if(*HitTest == (USHORT)HTTRANSPARENT)
      {
         PWINDOW_OBJECT DesktopWindow;
         HWND hDesktop = IntGetDesktopWindow();

         if((DesktopWindow = UserGetWindowObject(hDesktop)))
         {
            PWINDOW_OBJECT Wnd;

            UserRefObjectCo(DesktopWindow, &DesktopRef);

            co_WinPosWindowFromPoint(DesktopWindow, Window->MessageQueue, &Msg->pt, &Wnd);
            if(Wnd)
            {
               if(Wnd != Window)
               {
                  /* post the message to the other window */
                  Msg->hwnd = Wnd->hSelf;
                  if(!(Wnd->Status & WINDOWSTATUS_DESTROYING))
                  {
                     MsqPostMessage(Wnd->MessageQueue, Msg, FALSE,
                                    Msg->message == WM_MOUSEMOVE ? QS_MOUSEMOVE :
                                    QS_MOUSEBUTTON);
                  }

                  /* eat the message */
                  UserDerefObject(Wnd);
                  UserDerefObjectCo(DesktopWindow);
                  UserDerefObjectCo(Window);
                  return TRUE;
               }
               UserDerefObject(Wnd);
            }

            UserDerefObjectCo(DesktopWindow);
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
            (Window->Wnd->Class->Style & CS_DBLCLKS)) &&
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
            Msg->wParam = (WPARAM)Window->hSelf;
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
                          Msg->pt.x - (WORD)Window->Wnd->ClientRect.left,
                          Msg->pt.y - (WORD)Window->Wnd->ClientRect.top);
      }
   }

   UserDerefObjectCo(Window);
   return FALSE;
}


/*
 * Internal version of PeekMessage() doing all the work
 */
BOOL FASTCALL
co_IntPeekMessage(PUSER_MESSAGE Msg,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
   LARGE_INTEGER LargeTickCount;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PUSER_MESSAGE Message;
   BOOL Present, RemoveMessages;
   USER_REFERENCE_ENTRY Ref;
   USHORT HitTest;

   /* The queues and order in which they are checked are documented in the MSDN
      article on GetMessage() */

   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetCurrentThreadWin32Thread()->MessageQueue;

   /* Inspect RemoveMsg flags */
   /* FIXME: The only flag we process is PM_REMOVE - processing of others must still be implemented */
   RemoveMessages = RemoveMsg & PM_REMOVE;

CheckMessages:

   Present = FALSE;

   KeQueryTickCount(&LargeTickCount);
   ThreadQueue->LastMsgRead = LargeTickCount.u.LowPart;

   /* Dispatch sent messages here. */
   while (co_MsqDispatchOneSentMessage(ThreadQueue))
      ;

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
   Present = co_MsqFindMessage(ThreadQueue,
                               FALSE,
                               RemoveMessages,
                               hWnd,
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
   Present = co_MsqFindMessage(ThreadQueue,
                               TRUE,
                               RemoveMessages,
                               hWnd,
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
   while (co_MsqDispatchOneSentMessage(ThreadQueue))
      ;

   /* Check for paint messages. */
   if (IntGetPaintMessage(hWnd, MsgFilterMin, MsgFilterMax, PsGetCurrentThreadWin32Thread(), &Msg->Msg, RemoveMessages))
   {
      Msg->FreeLParam = FALSE;
      return TRUE;
   }

   /* Check for WM_(SYS)TIMER messages */
   Present = MsqGetTimerMessage(ThreadQueue, hWnd, MsgFilterMin, MsgFilterMax,
                                &Msg->Msg, RemoveMessages);
   if (Present)
   {
      Msg->FreeLParam = FALSE;
      goto MessageFound;
   }

   if(Present)
   {
MessageFound:

      if(RemoveMessages)
      {
         PWINDOW_OBJECT MsgWindow = NULL;

         if(Msg->Msg.hwnd && (MsgWindow = UserGetWindowObject(Msg->Msg.hwnd)) &&
               Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST)
         {
            USHORT HitTest;

            UserRefObjectCo(MsgWindow, &Ref);

            if(co_IntTranslateMouseMessage(ThreadQueue, &Msg->Msg, &HitTest, TRUE))
               /* FIXME - check message filter again, if the message doesn't match anymore,
                          search again */
            {
               UserDerefObjectCo(MsgWindow);
               /* eat the message, search again */
               goto CheckMessages;
            }

            if(ThreadQueue->CaptureWindow == NULL)
            {
               co_IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
               if((Msg->Msg.message != WM_MOUSEMOVE && Msg->Msg.message != WM_NCMOUSEMOVE) &&
                     IS_BTN_MESSAGE(Msg->Msg.message, DOWN) &&
                     co_IntActivateWindowMouse(ThreadQueue, &Msg->Msg, MsgWindow, &HitTest))
               {
                  UserDerefObjectCo(MsgWindow);
                  /* eat the message, search again */
                  goto CheckMessages;
               }
            }

            UserDerefObjectCo(MsgWindow);
         }
         else
         {
            co_IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
         }

//         if(MsgWindow)
//         {
//            UserDerefObject(MsgWindow);
//         }

         return TRUE;
      }

      if((Msg->Msg.hwnd && Msg->Msg.message >= WM_MOUSEFIRST && Msg->Msg.message <= WM_MOUSELAST) &&
            co_IntTranslateMouseMessage(ThreadQueue, &Msg->Msg, &HitTest, FALSE))
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
                  HWND hWnd,
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
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPeekMessage\n");
   UserEnterExclusive();

   /* Validate input */
   if (hWnd && hWnd != INVALID_HANDLE_VALUE)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN(-1);
      }
   }

   if (MsgFilterMax < MsgFilterMin)
   {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
   }

   Present = co_IntPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, RemoveMsg);
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
            RETURN( (BOOL) -1);
         }
         /* Transfer lParam data to user-mode mem */
         Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
         if (! NT_SUCCESS(Status))
         {
            ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                &Info.LParamSize, MEM_DECOMMIT);
            SetLastNtError(Status);
            RETURN( (BOOL) -1);
         }
         Info.Msg.lParam = (LPARAM) UserMem;
      }
      if (RemoveMsg && Msg.FreeLParam && 0 != Msg.Msg.lParam)
      {
         ExFreePool((void *) Msg.Msg.lParam);
      }
      Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( (BOOL) -1);
      }
   }

   RETURN( Present);

CLEANUP:
   DPRINT("Leave NtUserPeekMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static BOOL FASTCALL
co_IntWaitMessage(HWND Wnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   NTSTATUS Status;
   USER_MESSAGE Msg;

   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetCurrentThreadWin32Thread()->MessageQueue;

   do
   {
      if (co_IntPeekMessage(&Msg, Wnd, MsgFilterMin, MsgFilterMax, PM_NOREMOVE))
      {
         return TRUE;
      }

      /* Nothing found. Wait for new messages. */
      Status = co_MsqWaitForNewMessages(ThreadQueue, Wnd, MsgFilterMin, MsgFilterMax);
   }
   while ((STATUS_WAIT_0 <= Status && Status <= STATUS_WAIT_63) || STATUS_TIMEOUT == Status);

   SetLastNtError(Status);

   return FALSE;
}

BOOL STDCALL
NtUserGetMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
                 HWND hWnd,
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
   PWINDOW_OBJECT Window = NULL;
   PMSGMEMORY MsgMemoryEntry;
   PVOID UserMem;
   UINT Size;
   USER_MESSAGE Msg;
   DECLARE_RETURN(BOOL);
//   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserGetMessage\n");
   UserEnterExclusive();

   /* Validate input */
   if (hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(-1);
   }

//   if (Window) UserRefObjectCo(Window, &Ref);

   if (MsgFilterMax < MsgFilterMin)
   {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
   }

   do
   {
      GotMessage = co_IntPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, PM_REMOVE);
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
               RETURN( (BOOL) -1);
            }
            /* Transfer lParam data to user-mode mem */
            Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
            if (! NT_SUCCESS(Status))
            {
               ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                   &Info.LParamSize, MEM_DECOMMIT);
               SetLastNtError(Status);
               RETURN( (BOOL) -1);
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
            RETURN( (BOOL) -1);
         }
      }
      else if (! co_IntWaitMessage(hWnd, MsgFilterMin, MsgFilterMax))
      {
         RETURN( (BOOL) -1);
      }
   }
   while (! GotMessage);

   RETURN( WM_QUIT != Info.Msg.message);

CLEANUP:
//   if (Window) UserDerefObjectCo(Window);

   DPRINT("Leave NtUserGetMessage\n");
   UserLeave();
   END_CLEANUP;
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

BOOL FASTCALL
UserPostMessage(HWND Wnd,
                UINT Msg,
                WPARAM wParam,
                LPARAM lParam)
{
   MSG UserModeMsg, KernelModeMsg;
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;
   PMSGMEMORY MsgMemoryEntry;

   if (WM_QUIT == Msg)
   {
      MsqPostQuitMessage(PsGetCurrentThreadWin32Thread()->MessageQueue, wParam);
   }
   else if (Wnd == HWND_BROADCAST)
   {
      HWND *List;
      PWINDOW_OBJECT DesktopWindow;
      ULONG i;

      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);

      if (List != NULL)
      {
         for (i = 0; List[i]; i++)
            UserPostMessage(List[i], Msg, wParam, lParam);
         ExFreePool(List);
      }
   }
   else
   {
      PWINDOW_OBJECT Window;

      Window = UserGetWindowObject(Wnd);
      if (NULL == Window)
      {
         return FALSE;
      }
      if(Window->Status & WINDOWSTATUS_DESTROYING)
      {
         DPRINT1("Attempted to post message to window 0x%x that is being destroyed!\n", Wnd);
         /* FIXME - last error code? */
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
      IntGetCursorLocation(PsGetCurrentThreadWin32Thread()->Desktop->WindowStation,
                           &KernelModeMsg.pt);
      KeQueryTickCount(&LargeTickCount);
      KernelModeMsg.time = MsqCalculateMessageTime(&LargeTickCount);
      MsqPostMessage(Window->MessageQueue, &KernelModeMsg,
                     NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam,
                     QS_POSTMESSAGE);
   }

   return TRUE;
}


BOOL STDCALL
NtUserPostMessage(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPostMessage\n");
   UserEnterExclusive();

   RETURN(UserPostMessage(hWnd, Msg, wParam, lParam));

CLEANUP:
   DPRINT("Leave NtUserPostMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPostThreadMessage\n");
   UserEnterExclusive();

   Status = PsLookupThreadByThreadId((HANDLE)idThread,&peThread);

   if( Status == STATUS_SUCCESS )
   {
      pThread = (PW32THREAD)peThread->Tcb.Win32Thread;
      if( !pThread || !pThread->MessageQueue )
      {
         ObDereferenceObject( peThread );
         RETURN( FALSE);
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
         RETURN( FALSE);
      }
      MsqPostMessage(pThread->MessageQueue, &KernelModeMsg,
                     NULL != MsgMemoryEntry && 0 != KernelModeMsg.lParam,
                     QS_POSTMESSAGE);
      ObDereferenceObject( peThread );
      RETURN( TRUE);
   }
   else
   {
      SetLastNtError( Status );
      RETURN( FALSE);
   }

CLEANUP:
   DPRINT("Leave NtUserPostThreadMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
   UNIMPLEMENTED;

   return 0;
}

LRESULT FASTCALL
co_IntSendMessage(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam)
{
   ULONG_PTR Result = 0;
   if(co_IntSendMessageTimeout(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
   {
      return (LRESULT)Result;
   }
   return 0;
}

static
LRESULT FASTCALL
co_IntSendMessageTimeoutSingle(HWND hWnd,
                               UINT Msg,
                               WPARAM wParam,
                               LPARAM lParam,
                               UINT uFlags,
                               UINT uTimeout,
                               ULONG_PTR *uResult)
{
   ULONG_PTR Result;
   NTSTATUS Status;
   PWINDOW_OBJECT Window = NULL;
   PMSGMEMORY MsgMemoryEntry;
   INT lParamBufferSize;
   LPARAM lParamPacked;
   PW32THREAD Win32Thread;
   DECLARE_RETURN(LRESULT);
   USER_REFERENCE_ENTRY Ref;

   /* FIXME: Call hooks. */
   if (!(Window = UserGetWindowObject(hWnd)))
   {
       RETURN( FALSE);
   }

   UserRefObjectCo(Window, &Ref);

   Win32Thread = PsGetCurrentThreadWin32Thread();

   if (NULL != Win32Thread &&
         Window->MessageQueue == Win32Thread->MessageQueue)
   {
      if (Win32Thread->IsExiting)
      {
         /* Never send messages to exiting threads */
          RETURN( FALSE);
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
         DPRINT1("Failed to pack message parameters\n");
          RETURN( FALSE);
      }

      Result = (ULONG_PTR)co_IntCallWindowProc(Window->WndProc, !Window->Wnd->Unicode, hWnd, Msg, wParam,
               lParamPacked,lParamBufferSize);

      if(uResult)
      {
         *uResult = Result;
      }

      if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam)))
      {
         DPRINT1("Failed to unpack message parameters\n");
          RETURN( TRUE);
      }

       RETURN( TRUE);
   }

   if(uFlags & SMTO_ABORTIFHUNG && MsqIsHung(Window->MessageQueue))
   {
      /* FIXME - Set a LastError? */
       RETURN( FALSE);
   }

   if(Window->Status & WINDOWSTATUS_DESTROYING)
   {
      /* FIXME - last error? */
      DPRINT1("Attempted to send message to window 0x%x that is being destroyed!\n", hWnd);
       RETURN( FALSE);
   }

   Status = co_MsqSendMessage(Window->MessageQueue, hWnd, Msg, wParam, lParam,
                              uTimeout, (uFlags & SMTO_BLOCK), FALSE, uResult);


   if (STATUS_TIMEOUT == Status)
   {
      /* MSDN says GetLastError() should return 0 after timeout */
      SetLastWin32Error(0);
       RETURN( FALSE);
   }
   else if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
       RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   END_CLEANUP;
}

LRESULT FASTCALL
co_IntSendMessageTimeout(HWND hWnd,
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
      return co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, uFlags, uTimeout, uResult);
   }

   DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
   if (NULL == DesktopWindow)
   {
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return 0;
   }

   Children = IntWinListChildren(DesktopWindow);
   if (NULL == Children)
   {
      return 0;
   }

   for (Child = Children; NULL != *Child; Child++)
   {
      co_IntSendMessageTimeoutSingle(*Child, Msg, wParam, lParam, uFlags, uTimeout, uResult);
   }

   ExFreePool(Children);

   return (LRESULT) TRUE;
}


/* This function posts a message if the destination's message queue belongs to
   another thread, otherwise it sends the message. It does not support broadcast
   messages! */
LRESULT FASTCALL
co_IntPostOrSendMessage(HWND hWnd,
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

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      return 0;
   }

   if(Window->MessageQueue != PsGetCurrentThreadWin32Thread()->MessageQueue)
   {
      Result = UserPostMessage(hWnd, Msg, wParam, lParam);
   }
   else
   {
      if(!co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
      {
         Result = 0;
      }
   }

   return (LRESULT)Result;
}

LRESULT FASTCALL
co_IntDoSendMessage(HWND hWnd,
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
   if (HWND_BROADCAST != hWnd)
   {
      Window = UserGetWindowObject(hWnd);
      if (NULL == Window)
      {
         /* Tell usermode to not touch this one */
         Info.HandledByKernel = TRUE;
         MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
         return 0;
      }
   }

   /* FIXME: Check for an exiting window. */

   /* See if the current thread can handle the message */
   if (HWND_BROADCAST != hWnd && NULL != PsGetCurrentThreadWin32Thread() &&
         Window->MessageQueue == PsGetCurrentThreadWin32Thread()->MessageQueue)
   {
      /* Gather the information usermode needs to call the window proc directly */
      Info.HandledByKernel = FALSE;

      Status = MmCopyFromCaller(&(Info.Ansi), &(UnsafeInfo->Ansi),
                                sizeof(BOOL));
      if (! NT_SUCCESS(Status))
      {
         Info.Ansi = ! Window->Wnd->Unicode;
      }

      if (Window->IsSystem)
      {
          Info.Proc = (!Info.Ansi ? Window->WndProc : Window->WndProcExtra);
      }
      else
      {
          Info.Ansi = !Window->Wnd->Unicode;
          Info.Proc = Window->WndProc;
      }
   }
   else
   {
      /* Must be handled by other thread */
//      if (HWND_BROADCAST != hWnd)
//      {
//         UserDerefObject(Window);
//      }
      Info.HandledByKernel = TRUE;
      UserModeMsg.hwnd = hWnd;
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
         Result = co_IntSendMessage(KernelModeMsg.hwnd, KernelModeMsg.message,
                                    KernelModeMsg.wParam, KernelModeMsg.lParam);
      }
      else
      {
         Result = co_IntSendMessageTimeout(KernelModeMsg.hwnd, KernelModeMsg.message,
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
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSendMessageTimeout\n");
   UserEnterExclusive();

   dsm.uFlags = uFlags;
   dsm.uTimeout = uTimeout;
   Result = co_IntDoSendMessage(hWnd, Msg, wParam, lParam, &dsm, UnsafeInfo);
   if(uResult != NULL && Result != 0)
   {
      NTSTATUS Status;

      Status = MmCopyToCaller(uResult, &dsm.Result, sizeof(ULONG_PTR));
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
   }
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserSendMessageTimeout, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

LRESULT STDCALL
NtUserSendMessage(HWND Wnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam,
                  PNTUSERSENDMESSAGEINFO UnsafeInfo)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSendMessage\n");
   UserEnterExclusive();

   RETURN(co_IntDoSendMessage(Wnd, Msg, wParam, lParam, NULL, UnsafeInfo));

CLEANUP:
   DPRINT("Leave NtUserSendMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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


BOOL FASTCALL
UserSendNotifyMessage(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam)
{
   BOOL Result = TRUE;
   // Basicly the same as IntPostOrSendMessage
   if (hWnd == HWND_BROADCAST) //Handle Broadcast
   {
      HWND *List;
      PWINDOW_OBJECT DesktopWindow;
      ULONG i;

      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);

      if (List != NULL)
      {
         for (i = 0; List[i]; i++)
         {
            UserSendNotifyMessage(List[i], Msg, wParam, lParam);
         }
         ExFreePool(List);
      }
   }
   else
   {
     ULONG_PTR PResult;
     PWINDOW_OBJECT Window;
     NTSTATUS Status;
     MSG UserModeMsg;
     MSG KernelModeMsg;
     PMSGMEMORY MsgMemoryEntry;

      if(!(Window = UserGetWindowObject(hWnd))) return FALSE;

      if(Window->MessageQueue != PsGetCurrentThreadWin32Thread()->MessageQueue)
      { // Send message w/o waiting for it.
         Result = UserPostMessage(hWnd, Msg, wParam, lParam);
      }
      else
      { // Handle message and callback.
         UserModeMsg.hwnd = hWnd;
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
         Result = co_IntSendMessageTimeoutSingle(
                                   KernelModeMsg.hwnd, KernelModeMsg.message,
                                   KernelModeMsg.wParam, KernelModeMsg.lParam,
                                   SMTO_NORMAL, 0, &PResult);

         Status = CopyMsgToUserMem(&UserModeMsg, &KernelModeMsg);
         if (! NT_SUCCESS(Status))
         {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return FALSE;
         }
      }
   }
   return Result;
}


BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
   DECLARE_RETURN(BOOL);

   DPRINT("EnterNtUserSendNotifyMessage\n");
   UserEnterExclusive();

   RETURN(UserSendNotifyMessage(hWnd, Msg, wParam, lParam));

CLEANUP:
   DPRINT("Leave NtUserSendNotifyMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}


BOOL STDCALL
NtUserWaitMessage(VOID)
{
   DECLARE_RETURN(BOOL);

   DPRINT("EnterNtUserWaitMessage\n");
   UserEnterExclusive();

   RETURN(co_IntWaitMessage(NULL, 0, 0));

CLEANUP:
   DPRINT("Leave NtUserWaitMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD STDCALL
NtUserGetQueueStatus(BOOL ClearChanges)
{
   PUSER_MESSAGE_QUEUE Queue;
   DWORD Result;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetQueueStatus\n");
   UserEnterExclusive();

   Queue = PsGetCurrentThreadWin32Thread()->MessageQueue;

   Result = MAKELONG(Queue->QueueBits, Queue->ChangedBits);
   if (ClearChanges)
   {
      Queue->ChangedBits = 0;
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserGetQueueStatus, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL STDCALL
IntInitMessagePumpHook()
{
   ((PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread)->MessagePumpHookValue++;
   return TRUE;
}

BOOL STDCALL
IntUninitMessagePumpHook()
{
   if (((PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread)->MessagePumpHookValue <= 0)
   {
      return FALSE;
   }
   ((PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread)->MessagePumpHookValue--;
   return TRUE;
}

/* EOF */
