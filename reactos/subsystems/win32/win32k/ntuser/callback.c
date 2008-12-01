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
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/wndproc.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 * NOTES:            Please use the Callback Memory Management functions for
 *                   callbacks to make sure, the memory is freed on thread
 *                   termination!
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* CALLBACK MEMORY MANAGEMENT ************************************************/

typedef struct _INT_CALLBACK_HEADER
{
   /* list entry in the THREADINFO structure */
   LIST_ENTRY ListEntry;
}
INT_CALLBACK_HEADER, *PINT_CALLBACK_HEADER;

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   if(!(Mem = ExAllocatePoolWithTag(PagedPool, Size + sizeof(INT_CALLBACK_HEADER),
                                    TAG_CALLBACK)))
   {
      return NULL;
   }

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   /* insert the callback memory into the thread's callback list */

   InsertTailList(&W32Thread->W32CallbackListHead, &Mem->ListEntry);

   return (Mem + 1);
}

VOID FASTCALL
IntCbFreeMemory(PVOID Data)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   ASSERT(Data);

   Mem = ((PINT_CALLBACK_HEADER)Data - 1);

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   /* remove the memory block from the thread's callback list */
   RemoveEntryList(&Mem->ListEntry);

   /* free memory */
   ExFreePoolWithTag(Mem, TAG_CALLBACK);
}

VOID FASTCALL
IntCleanupThreadCallbacks(PTHREADINFO W32Thread)
{
   PLIST_ENTRY CurrentEntry;
   PINT_CALLBACK_HEADER Mem;

   while (!IsListEmpty(&W32Thread->W32CallbackListHead))
   {
      CurrentEntry = RemoveHeadList(&W32Thread->W32CallbackListHead);
      Mem = CONTAINING_RECORD(CurrentEntry, INT_CALLBACK_HEADER,
                              ListEntry);

      /* free memory */
      ExFreePool(Mem);
   }
}


//
// Pass the Current Window handle and pointer to the Client Callback.
// This will help user space programs speed up read access with the window object.
//
static VOID
IntSetTebWndCallback (HWND * hWnd, PVOID * pWnd)
{
  HWND hWndS = *hWnd;
  PWINDOW_OBJECT Window = UserGetWindowObject(*hWnd);
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  *hWnd = ClientInfo->CallbackWnd.hWnd;
  *pWnd = ClientInfo->CallbackWnd.pvWnd;

  ClientInfo->CallbackWnd.hWnd  = hWndS;
  ClientInfo->CallbackWnd.pvWnd = DesktopHeapAddressToUser(Window->Wnd);
}

static VOID
IntRestoreTebWndCallback (HWND hWnd, PVOID pWnd)
{
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  ClientInfo->CallbackWnd.hWnd = hWnd;
  ClientInfo->CallbackWnd.pvWnd = pWnd;
}

/* FUNCTIONS *****************************************************************/

VOID APIENTRY
co_IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
                              HWND hWnd,
                              UINT Msg,
                              ULONG_PTR CompletionCallbackContext,
                              LRESULT Result)
{
   SENDASYNCPROC_CALLBACK_ARGUMENTS Arguments;
   PVOID ResultPointer, pWnd;
   ULONG ResultLength;
   NTSTATUS Status;

   Arguments.Callback = CompletionCallback;
   Arguments.Wnd = hWnd;
   Arguments.Msg = Msg;
   Arguments.Context = CompletionCallbackContext;
   Arguments.Result = Result;

   IntSetTebWndCallback (&hWnd, &pWnd);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_SENDASYNCPROC,
                               &Arguments,
                               sizeof(SENDASYNCPROC_CALLBACK_ARGUMENTS),
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   IntRestoreTebWndCallback (hWnd, pWnd);

   if (!NT_SUCCESS(Status))
   {
      return;
   }
   return;
}

LRESULT APIENTRY
co_IntCallWindowProc(WNDPROC Proc,
                     BOOLEAN IsAnsiProc,
                     HWND Wnd,
                     UINT Message,
                     WPARAM wParam,
                     LPARAM lParam,
                     INT lParamBufferSize)
{
   WINDOWPROC_CALLBACK_ARGUMENTS StackArguments;
   PWINDOWPROC_CALLBACK_ARGUMENTS Arguments;
   NTSTATUS Status;
   PVOID ResultPointer, pWnd;
   ULONG ResultLength;
   ULONG ArgumentLength;
   LRESULT Result;

   if (0 < lParamBufferSize)
   {
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS) + lParamBufferSize;
      Arguments = IntCbAllocateMemory(ArgumentLength);
      if (NULL == Arguments)
      {
         DPRINT1("Unable to allocate buffer for window proc callback\n");
         return -1;
      }
      RtlMoveMemory((PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                    (PVOID) lParam, lParamBufferSize);
   }
   else
   {
      Arguments = &StackArguments;
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS);
   }
   Arguments->Proc = Proc;
   Arguments->IsAnsiProc = IsAnsiProc;
   Arguments->Wnd = Wnd;
   Arguments->Msg = Message;
   Arguments->wParam = wParam;
   Arguments->lParam = lParam;
   Arguments->lParamBufferSize = lParamBufferSize;
   ResultPointer = NULL;
   ResultLength = ArgumentLength;

   IntSetTebWndCallback (&Wnd, &pWnd);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_WINDOWPROC,
                               Arguments,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   _SEH2_TRY
   {
      /* Simulate old behaviour: copy into our local buffer */
      RtlMoveMemory(Arguments, ResultPointer, ArgumentLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   UserEnterCo();

   IntRestoreTebWndCallback (Wnd, pWnd);

   if (!NT_SUCCESS(Status))
   {
      if (0 < lParamBufferSize)
      {
         IntCbFreeMemory(Arguments);
      }
      return -1;
   }
   Result = Arguments->Result;

   if (0 < lParamBufferSize)
   {
      RtlMoveMemory((PVOID) lParam,
                    (PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                    lParamBufferSize);
      IntCbFreeMemory(Arguments);
   }

   return Result;
}

HMENU APIENTRY
co_IntLoadSysMenuTemplate()
{
   LRESULT Result;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADSYSMENUTEMPLATE,
                               NULL,
                               0,
                               &ResultPointer,
                               &ResultLength);

   /* Simulate old behaviour: copy into our local buffer */
   Result = *(LRESULT*)ResultPointer;

   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      return(0);
   }
   return (HMENU)Result;
}

BOOL APIENTRY
co_IntLoadDefaultCursors(VOID)
{
   LRESULT Result;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   BOOL DefaultCursor = TRUE;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADDEFAULTCURSORS,
                               &DefaultCursor,
                               sizeof(BOOL),
                               &ResultPointer,
                               &ResultLength);

   /* Simulate old behaviour: copy into our local buffer */
   Result = *(LRESULT*)ResultPointer;

   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }
   return TRUE;
}

LRESULT APIENTRY
co_IntCallHookProc(INT HookId,
                   INT Code,
                   WPARAM wParam,
                   LPARAM lParam,
                   HOOKPROC Proc,
                   BOOLEAN Ansi,
                   PUNICODE_STRING ModuleName)
{
   ULONG ArgumentLength;
   PVOID Argument;
   LRESULT Result = 0;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   PHOOKPROC_CALLBACK_ARGUMENTS Common;
   CBT_CREATEWNDW *CbtCreateWnd =NULL;
   PCHAR Extra;
   PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra ;
   PUNICODE_STRING WindowName = NULL;
   PUNICODE_STRING ClassName = NULL;

   ArgumentLength = sizeof(HOOKPROC_CALLBACK_ARGUMENTS) - sizeof(WCHAR)
                    + ModuleName->Length;
   switch(HookId)
   {
      case WH_CBT:
         switch(Code)
         {
            case HCBT_CREATEWND:
               CbtCreateWnd = (CBT_CREATEWNDW *) lParam;
               ArgumentLength += sizeof(HOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS);
               WindowName = (PUNICODE_STRING) (CbtCreateWnd->lpcs->lpszName);
               ArgumentLength += WindowName->Length + sizeof(WCHAR);
               ClassName = (PUNICODE_STRING) (CbtCreateWnd->lpcs->lpszClass);
               if (! IS_ATOM(ClassName->Buffer))
               {
                  ArgumentLength += ClassName->Length + sizeof(WCHAR);
               }
               break;
            default:
               DPRINT1("Trying to call unsupported CBT hook %d\n", Code);
               return 0;
         }
         break;
      case WH_KEYBOARD_LL:
         ArgumentLength += sizeof(KBDLLHOOKSTRUCT);
         break;
      case WH_MOUSE_LL:
         ArgumentLength += sizeof(MSLLHOOKSTRUCT);
         break;
      case WH_MOUSE:
         ArgumentLength += sizeof(MOUSEHOOKSTRUCT);
         break;
     case WH_CALLWNDPROC:
         ArgumentLength += sizeof(CWPSTRUCT);
         break;
      case WH_CALLWNDPROCRET:
         ArgumentLength += sizeof(CWPRETSTRUCT);
         break;
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         ArgumentLength += sizeof(MSG);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;
      default:
         DPRINT1("Trying to call unsupported window hook %d\n", HookId);
         return 0;
   }

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      DPRINT1("HookProc callback failed: out of memory\n");
      return 0;
   }
   Common = (PHOOKPROC_CALLBACK_ARGUMENTS) Argument;
   Common->HookId = HookId;
   Common->Code = Code;
   Common->wParam = wParam;
   Common->lParam = lParam;
   Common->Proc = Proc;
   Common->Ansi = Ansi;
   Common->ModuleNameLength = ModuleName->Length;
   memcpy(Common->ModuleName, ModuleName->Buffer, ModuleName->Length);
   Extra = (PCHAR) Common->ModuleName + Common->ModuleNameLength;

   switch(HookId)
   {
      case WH_CBT:
         switch(Code)
         {
            case HCBT_CREATEWND:
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               CbtCreatewndExtra = (PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS) Extra;
               CbtCreatewndExtra->Cs = *(CbtCreateWnd->lpcs);
               CbtCreatewndExtra->WndInsertAfter = CbtCreateWnd->hwndInsertAfter;
               Extra = (PCHAR) (CbtCreatewndExtra + 1);
               RtlCopyMemory(Extra, WindowName->Buffer, WindowName->Length);
               CbtCreatewndExtra->Cs.lpszName = (LPCWSTR) (Extra - (PCHAR) CbtCreatewndExtra);
               CbtCreatewndExtra->Cs.lpszClass = ClassName->Buffer;
               Extra += WindowName->Length;
               *((WCHAR *) Extra) = L'\0';
               Extra += sizeof(WCHAR);
               if (! IS_ATOM(ClassName->Buffer))
               {
                  RtlCopyMemory(Extra, ClassName->Buffer, ClassName->Length);
                  CbtCreatewndExtra->Cs.lpszClass =
                     (LPCWSTR) MAKELONG(Extra - (PCHAR) CbtCreatewndExtra, 1);
                  Extra += ClassName->Length;
                  *((WCHAR *) Extra) = L'\0';
               }
               break;
         }
         break;
      case WH_KEYBOARD_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(KBDLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MSLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MOUSEHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_CALLWNDPROC:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CWPSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_CALLWNDPROCRET:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CWPRETSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;         
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MSG));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
//         DPRINT1("KHOOK Memory: %x\n",Common);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;         
   }

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_HOOKPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   _SEH2_TRY
   {
      ProbeForRead((PVOID)*(LRESULT*)ResultPointer,
                                   sizeof(LRESULT),
                                                 1);
      /* Simulate old behaviour: copy into our local buffer */
      Result = *(LRESULT*)ResultPointer;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Result = 0;
   }
   _SEH2_END;

   IntCbFreeMemory(Argument);

   if (!NT_SUCCESS(Status))
   {
      return 0;
   }

   return Result;
}

LRESULT
APIENTRY
co_IntCallEventProc(HWINEVENTHOOK hook,
                           DWORD event,
                             HWND hWnd,
                         LONG idObject,
                          LONG idChild,
                   DWORD dwEventThread,
                   DWORD dwmsEventTime,
                     WINEVENTPROC Proc)
{
   LRESULT Result;
   NTSTATUS Status;
   PEVENTPROC_CALLBACK_ARGUMENTS Common;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer, pWnd;

   ArgumentLength = sizeof(EVENTPROC_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      DPRINT1("EventProc callback failed: out of memory\n");
      return 0;
   }
   Common = (PEVENTPROC_CALLBACK_ARGUMENTS) Argument;
   Common->hook = hook;
   Common->event = event;
   Common->hwnd = hWnd;
   Common->idObject = idObject;
   Common->idChild = idChild;
   Common->dwEventThread = dwEventThread;
   Common->dwmsEventTime = dwmsEventTime;
   Common->Proc = Proc;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   IntSetTebWndCallback (&hWnd, &pWnd);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_EVENTPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   /* Simulate old behaviour: copy into our local buffer */
   Result = *(LRESULT*)ResultPointer;

   UserEnterCo();

   IntRestoreTebWndCallback (hWnd, pWnd);

   IntCbFreeMemory(Argument);
  
   if (!NT_SUCCESS(Status))
   {
      return 0;
   }

   return Result;
}

/* EOF */
