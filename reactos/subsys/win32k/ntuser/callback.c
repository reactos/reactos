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
/* $Id: callback.c,v 1.25 2004/06/20 12:34:20 navaraf Exp $
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
  /* list entry in the W32THREAD structure */
  LIST_ENTRY ListEntry;
} INT_CALLBACK_HEADER, *PINT_CALLBACK_HEADER;

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size)
{
  PINT_CALLBACK_HEADER Mem;
  PW32THREAD W32Thread;
  
  if(!(Mem = ExAllocatePoolWithTag(PagedPool, Size + sizeof(INT_CALLBACK_HEADER),
                                   TAG_CALLBACK)))
  {
    return NULL;
  }
  
  W32Thread = PsGetWin32Thread();
  ASSERT(W32Thread);
  
  /* insert the callback memory into the thread's callback list */
  
  ExAcquireFastMutex(&W32Thread->W32CallbackListLock);
  InsertTailList(&W32Thread->W32CallbackListHead, &Mem->ListEntry);
  ExReleaseFastMutex(&W32Thread->W32CallbackListLock);
  
  return (Mem + 1);
}

VOID FASTCALL
IntCbFreeMemory(PVOID Data)
{
  PINT_CALLBACK_HEADER Mem;
  PW32THREAD W32Thread;
  
  ASSERT(Data);
  
  Mem = ((PINT_CALLBACK_HEADER)Data - 1);
  
  W32Thread = PsGetWin32Thread();
  ASSERT(W32Thread);
  
  /* remove the memory block from the thread's callback list */
  ExAcquireFastMutex(&W32Thread->W32CallbackListLock);
  RemoveEntryList(&Mem->ListEntry);
  ExReleaseFastMutex(&W32Thread->W32CallbackListLock);
  
  /* free memory */
  ExFreePool(Mem);
}

VOID FASTCALL
IntCleanupThreadCallbacks(PW32THREAD W32Thread)
{
  PLIST_ENTRY CurrentEntry;
  PINT_CALLBACK_HEADER Mem;
  
  ExAcquireFastMutex(&W32Thread->W32CallbackListLock);
  while (!IsListEmpty(&W32Thread->W32CallbackListHead))
  {
    CurrentEntry = RemoveHeadList(&W32Thread->W32CallbackListHead);
    Mem = CONTAINING_RECORD(CurrentEntry, INT_CALLBACK_HEADER, 
                            ListEntry);
    
    /* free memory */
    ExFreePool(Mem);
  }
  ExReleaseFastMutex(&W32Thread->W32CallbackListLock);
}

/* FUNCTIONS *****************************************************************/

VOID STDCALL
IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
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
IntCallWindowProc(WNDPROC Proc,
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
  PVOID ResultPointer;
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
  ResultPointer = Arguments;
  ResultLength = ArgumentLength;
  Status = NtW32Call(USER32_CALLBACK_WINDOWPROC,
		     Arguments,
		     ArgumentLength,
		     &ResultPointer,
		     &ResultLength);
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

HMENU STDCALL
IntLoadSysMenuTemplate()
{
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;

  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_LOADSYSMENUTEMPLATE,
		     NULL,
		     0,
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }
  return (HMENU)Result;
}

BOOL STDCALL
IntLoadDefaultCursors(VOID)
{
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;
  BOOL DefaultCursor = TRUE;

  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_LOADDEFAULTCURSORS,
		     &DefaultCursor,
		     sizeof(BOOL),
		     &ResultPointer,
		     &ResultLength);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
  return TRUE;
}

LRESULT STDCALL
IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName)
{
  ULONG ArgumentLength;
  PVOID Argument;
  LRESULT Result;
  NTSTATUS Status;
  PVOID ResultPointer;
  ULONG ResultLength;
  PHOOKPROC_CALLBACK_ARGUMENTS Common;
  CBT_CREATEWNDW *CbtCreateWnd;
  PCHAR Extra;
  PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra;
  PUNICODE_STRING WindowName;
  PUNICODE_STRING ClassName;

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
    }
  
  ResultPointer = &Result;
  ResultLength = sizeof(LRESULT);
  Status = NtW32Call(USER32_CALLBACK_HOOKPROC,
		     Argument,
		     ArgumentLength,
		     &ResultPointer,
		     &ResultLength);
  
  IntCbFreeMemory(Argument);
  
  if (!NT_SUCCESS(Status))
    {
      return 0;
    }

  return Result;
}

/* EOF */
