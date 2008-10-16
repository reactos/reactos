/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsystem/win32/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserGetThreadState(
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetThreadState\n");
   if (Routine != THREADSTATE_GETTHREADINFO)
   {
       UserEnterShared();
   }
   else
   {
       UserEnterExclusive();
   }

   switch (Routine)
   {
      case THREADSTATE_GETTHREADINFO:
         GetW32ThreadInfo();
         RETURN(0);

      case THREADSTATE_FOCUSWINDOW:
         RETURN( (DWORD)IntGetThreadFocusWindow());
      case THREADSTATE_CAPTUREWINDOW:
         /* FIXME should use UserEnterShared */
         RETURN( (DWORD)IntGetCapture());
      case THREADSTATE_PROGMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->Desktop->hProgmanWindow);
      case THREADSTATE_TASKMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->Desktop->hTaskManWindow);
      case THREADSTATE_ACTIVEWINDOW:
         RETURN ( (DWORD)UserGetActiveWindow());
   }
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserGetThreadState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


UINT
STDCALL
NtUserGetDoubleClickTime(VOID)
{
   UINT Result;
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject;
   PSYSTEM_CURSORINFO CurInfo;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetDoubleClickTime\n");
   UserEnterShared();

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObject);
   if (!NT_SUCCESS(Status))
      RETURN( (DWORD)FALSE);

   CurInfo = IntGetSysCursorInfo(WinStaObject);
   Result = CurInfo->DblClickSpeed;

   ObDereferenceObject(WinStaObject);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserGetDoubleClickTime, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
STDCALL
NtUserGetGUIThreadInfo(
   DWORD idThread, /* if NULL use foreground thread */
   LPGUITHREADINFO lpgui)
{
   NTSTATUS Status;
   PTHRDCARETINFO CaretInfo;
   GUITHREADINFO SafeGui;
   PDESKTOP_OBJECT Desktop;
   PUSER_MESSAGE_QUEUE MsgQueue;
   PETHREAD Thread = NULL;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserGetGUIThreadInfo\n");
   UserEnterShared();

   Status = MmCopyFromCaller(&SafeGui, lpgui, sizeof(DWORD));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   if(SafeGui.cbSize != sizeof(GUITHREADINFO))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   if(idThread)
   {
      Status = PsLookupThreadByThreadId((HANDLE)idThread, &Thread);
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         RETURN( FALSE);
      }
      Desktop = ((PTHREADINFO)Thread->Tcb.Win32Thread)->Desktop;
   }
   else
   {
      /* get the foreground thread */
      PTHREADINFO W32Thread = (PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread;
      Desktop = W32Thread->Desktop;
      if(Desktop)
      {
         MsgQueue = Desktop->ActiveMessageQueue;
         if(MsgQueue)
         {
            Thread = MsgQueue->Thread;
         }
      }
   }

   if(!Thread || !Desktop)
   {
      if(idThread && Thread)
         ObDereferenceObject(Thread);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   MsgQueue = (PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue;
   CaretInfo = MsgQueue->CaretInfo;

   SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);
   if(MsgQueue->MenuOwner)
      SafeGui.flags |= GUI_INMENUMODE | MsgQueue->MenuState;
   if(MsgQueue->MoveSize)
      SafeGui.flags |= GUI_INMOVESIZE;

   /* FIXME add flag GUI_16BITTASK */

   SafeGui.hwndActive = MsgQueue->ActiveWindow;
   SafeGui.hwndFocus = MsgQueue->FocusWindow;
   SafeGui.hwndCapture = MsgQueue->CaptureWindow;
   SafeGui.hwndMenuOwner = MsgQueue->MenuOwner;
   SafeGui.hwndMoveSize = MsgQueue->MoveSize;
   SafeGui.hwndCaret = CaretInfo->hWnd;

   SafeGui.rcCaret.left = CaretInfo->Pos.x;
   SafeGui.rcCaret.top = CaretInfo->Pos.y;
   SafeGui.rcCaret.right = SafeGui.rcCaret.left + CaretInfo->Size.cx;
   SafeGui.rcCaret.bottom = SafeGui.rcCaret.top + CaretInfo->Size.cy;

   if(idThread)
      ObDereferenceObject(Thread);

   Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetGUIThreadInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
STDCALL
NtUserGetGuiResources(
   HANDLE hProcess,
   DWORD uiFlags)
{
   PEPROCESS Process;
   PW32PROCESS W32Process;
   NTSTATUS Status;
   DWORD Ret = 0;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetGuiResources\n");
   UserEnterShared();

   Status = ObReferenceObjectByHandle(hProcess,
                                      PROCESS_QUERY_INFORMATION,
                                      PsProcessType,
                                      ExGetPreviousMode(),
                                      (PVOID*)&Process,
                                      NULL);

   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   W32Process = (PW32PROCESS)Process->Win32Process;
   if(!W32Process)
   {
      ObDereferenceObject(Process);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   switch(uiFlags)
   {
      case GR_GDIOBJECTS:
         {
            Ret = (DWORD)W32Process->GDIObjects;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserObjects;
            break;
         }
      default:
         {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;
         }
   }

   ObDereferenceObject(Process);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetGuiResources, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;
   Dest->MaximumLength = Dest->Length;

   if(Dest->Length > 0 && Src)
   {
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(Dest->Buffer);
         Dest->Buffer = NULL;
         return Status;
      }


      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;
   Dest->MaximumLength = 0;

   if(Dest->Length > 0 && Src)
   {
      Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(Dest->Buffer);
         Dest->Buffer = NULL;
         return Status;
      }

      /* make sure the string is null-terminated */
      Src = (PWSTR)((PBYTE)Dest->Buffer + Dest->Length);
      *Src = L'\0';

      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src)
{
   if (Src->Length + sizeof(WCHAR) <= Src->MaximumLength
         && L'\0' == Src->Buffer[Src->Length / sizeof(WCHAR)])
   {
      /* The unicode_string is already nul terminated. Just reuse it. */
      *Dest = Src->Buffer;
      return STATUS_SUCCESS;
   }

   *Dest = ExAllocatePoolWithTag(PagedPool, Src->Length + sizeof(WCHAR), TAG_STRING);
   if (NULL == *Dest)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(*Dest, Src->Buffer, Src->Length);
   (*Dest)[Src->Length / 2] = L'\0';

   return STATUS_SUCCESS;
}

void FASTCALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString)
{
   if (NullTerminated != UnicodeString->Buffer)
   {
      ExFreePool(NullTerminated);
   }
}

PW32PROCESSINFO
GetW32ProcessInfo(VOID)
{
    PW32PROCESSINFO pi;
    PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();

    if (W32Process == NULL)
    {
        /* FIXME - temporary hack for system threads... */
        return NULL;
    }

    if (W32Process->ProcessInfo == NULL)
    {
        pi = UserHeapAlloc(sizeof(W32PROCESSINFO));
        if (pi != NULL)
        {
            RtlZeroMemory(pi,
                          sizeof(W32PROCESSINFO));

            /* initialize it */
            pi->UserHandleTable = gHandleTable;
            pi->hUserHeap = W32Process->HeapMappings.KernelMapping;
            pi->UserHeapDelta = (ULONG_PTR)W32Process->HeapMappings.KernelMapping -
                                (ULONG_PTR)W32Process->HeapMappings.UserMapping;
            pi->psi = gpsi;

            if (InterlockedCompareExchangePointer(&W32Process->ProcessInfo,
                                                  pi,
                                                  NULL) != NULL)
            {
                UserHeapFree(pi);
            }
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return W32Process->ProcessInfo;
}

PW32THREADINFO
GetW32ThreadInfo(VOID)
{
    PTEB Teb;
    PW32THREADINFO ti;
    PCLIENTINFO ci;
    PTHREADINFO W32Thread = PsGetCurrentThreadWin32Thread();

    if (W32Thread == NULL)
    {
        /* FIXME - temporary hack for system threads... */
        return NULL;
    }

    /* allocate a THREADINFO structure if neccessary */
    if (W32Thread->ThreadInfo == NULL)
    {
        ti = UserHeapAlloc(sizeof(W32THREADINFO));
        if (ti != NULL)
        {
            RtlZeroMemory(ti,
                          sizeof(W32THREADINFO));

            /* initialize it */
            ti->kpi = GetW32ProcessInfo();
            ti->pi = UserHeapAddressToUser(ti->kpi);
            ti->Hooks = W32Thread->Hooks;
            if (W32Thread->Desktop != NULL)
            {
                ti->Desktop = W32Thread->Desktop->DesktopInfo;
                ti->DesktopHeapBase = W32Thread->Desktop->DesktopInfo->hKernelHeap;
                ti->DesktopHeapLimit = W32Thread->Desktop->DesktopInfo->HeapLimit;
                ti->DesktopHeapDelta = DesktopHeapGetUserDelta();
            }
            else
            {
                ti->Desktop = NULL;
                ti->DesktopHeapBase = NULL;
                ti->DesktopHeapLimit = 0;
                ti->DesktopHeapDelta = 0;
            }

            W32Thread->ThreadInfo = ti;
            /* update the TEB */
            Teb = NtCurrentTeb();
            ci = GetWin32ClientInfo();
            _SEH_TRY
            {
                ProbeForWrite(Teb,
                              sizeof(TEB),
                              sizeof(ULONG));

                Teb->Win32ThreadInfo = UserHeapAddressToUser(W32Thread->ThreadInfo);
                ci->pClientThreadInfo = &ti->ClientThreadInfo;
            }
            _SEH_HANDLE
            {
                SetLastNtError(_SEH_GetExceptionCode());
            }
            _SEH_END;
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return W32Thread->ThreadInfo;
}


/* EOF */
