/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             subsystems/win32/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

SHORT
FASTCALL
IntGdiGetLanguageID(VOID)
{
  HANDLE KeyHandle;
  OBJECT_ATTRIBUTES ObAttr;
//  http://support.microsoft.com/kb/324097
  ULONG Ret = 0x409; // English
  PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
  ULONG Size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
  UNICODE_STRING Language;

  RtlInitUnicodeString( &Language,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language");

  InitializeObjectAttributes( &ObAttr,
                            &Language,
                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

  if ( NT_SUCCESS(ZwOpenKey(&KeyHandle, KEY_READ, &ObAttr)))
  {
     pKeyInfo = ExAllocatePoolWithTag(PagedPool, Size, TAG_STRING);
     if ( pKeyInfo )
     {
        RtlInitUnicodeString(&Language, L"Default");

        if ( NT_SUCCESS(ZwQueryValueKey( KeyHandle,
                                         &Language,
                        KeyValuePartialInformation,
                                          pKeyInfo,
                                              Size,
                                             &Size)) )
      {
        RtlInitUnicodeString(&Language, (PWSTR)pKeyInfo->Data);
        RtlUnicodeStringToInteger(&Language, 16, &Ret);
      }
      ExFreePoolWithTag(pKeyInfo, TAG_STRING);
    }
    ZwClose(KeyHandle);
  }
  TRACE("Language ID = %x\n",Ret);
  return (SHORT) Ret;
}

HBRUSH
FASTCALL
GetControlColor(
   PWND pwndParent,
   PWND pwnd,
   HDC hdc,
   UINT CtlMsg)
{
    HBRUSH hBrush;

    if (!pwndParent) pwndParent = pwnd;

    if ( pwndParent->head.pti->ppi != PsGetCurrentProcessWin32Process())
    {
       return (HBRUSH)IntDefWindowProc( pwndParent, CtlMsg, (WPARAM)hdc, (LPARAM)UserHMGetHandle(pwnd), FALSE);
    }

    hBrush = (HBRUSH)co_IntSendMessage( UserHMGetHandle(pwndParent), CtlMsg, (WPARAM)hdc, (LPARAM)UserHMGetHandle(pwnd));

    if (!hBrush || !GreIsHandleValid(hBrush))
    {
       hBrush = (HBRUSH)IntDefWindowProc( pwndParent, CtlMsg, (WPARAM)hdc, (LPARAM)UserHMGetHandle(pwnd), FALSE);
    }
    return hBrush;
}

HBRUSH
FASTCALL
GetControlBrush(
   PWND pwnd,
   HDC  hdc,
   UINT ctlType)
{
    PWND pwndParent = IntGetParent(pwnd);
    return GetControlColor( pwndParent, pwnd, hdc, ctlType);
}

HBRUSH
APIENTRY
NtUserGetControlBrush(
   HWND hwnd,
   HDC  hdc,
   UINT ctlType)
{
   PWND pwnd;
   HBRUSH hBrush = NULL;

   UserEnterExclusive();
   if ( (pwnd = UserGetWindowObject(hwnd)) &&
       ((ctlType - WM_CTLCOLORMSGBOX) < CTLCOLOR_MAX) &&
        hdc )
   {
      hBrush = GetControlBrush(pwnd, hdc, ctlType);
   }
   UserLeave();
   return hBrush;
}

/*
 * Called from PaintRect, works almost like wine PaintRect16 but returns hBrush.
 */
HBRUSH
APIENTRY
NtUserGetControlColor(
   HWND hwndParent,
   HWND hwnd,
   HDC hdc,
   UINT CtlMsg) // Wine PaintRect: WM_CTLCOLORMSGBOX + hbrush
{
   PWND pwnd, pwndParent = NULL;
   HBRUSH hBrush = NULL;

   UserEnterExclusive();
   if ( (pwnd = UserGetWindowObject(hwnd)) &&
       ((CtlMsg - WM_CTLCOLORMSGBOX) < CTLCOLOR_MAX) &&
        hdc )
   {
      if (hwndParent) pwndParent = UserGetWindowObject(hwndParent);
      hBrush = GetControlColor( pwndParent, pwnd, hdc, CtlMsg);
   }
   UserLeave();
   return hBrush;
}

/*
 * @unimplemented
 */
DWORD_PTR APIENTRY
NtUserGetThreadState(
   DWORD Routine)
{
   DWORD_PTR ret = 0;

   TRACE("Enter NtUserGetThreadState\n");
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
         break;
      case THREADSTATE_FOCUSWINDOW:
         ret = (DWORD_PTR)IntGetThreadFocusWindow();
         break;
      case THREADSTATE_CAPTUREWINDOW:
         /* FIXME: Should use UserEnterShared */
         ret = (DWORD_PTR)IntGetCapture();
         break;
      case THREADSTATE_PROGMANWINDOW:
         ret = (DWORD_PTR)GetW32ThreadInfo()->pDeskInfo->hProgmanWindow;
         break;
      case THREADSTATE_TASKMANWINDOW:
         ret = (DWORD_PTR)GetW32ThreadInfo()->pDeskInfo->hTaskManWindow;
         break;
      case THREADSTATE_ACTIVEWINDOW:
         ret = (DWORD_PTR)UserGetActiveWindow();
         break;
      case THREADSTATE_INSENDMESSAGE:
         {
           PUSER_SENT_MESSAGE Message = 
                ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->pusmCurrent;
           ERR("THREADSTATE_INSENDMESSAGE\n");

           ret = ISMEX_NOSEND;
           if (Message)
           {
             if (Message->SenderQueue)
                ret = ISMEX_SEND;
             else
             {
                if (Message->CompletionCallback)
                   ret = ISMEX_CALLBACK;
                else
                   ret = ISMEX_NOTIFY;
             }
             /* If ReplyMessage */
             if (Message->QS_Flags & QS_SMRESULT) ret |= ISMEX_REPLIED;
           }

           break;         
         }
      case THREADSTATE_GETMESSAGETIME:
         ret = ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->timeLast;
         break;

      case THREADSTATE_UPTIMELASTREAD:
         {
           PTHREADINFO pti;
           LARGE_INTEGER LargeTickCount;
           pti = PsGetCurrentThreadWin32Thread();
           KeQueryTickCount(&LargeTickCount);
           pti->MessageQueue->LastMsgRead = LargeTickCount.u.LowPart;
           pti->pcti->tickLastMsgChecked = LargeTickCount.u.LowPart;
         }
         break;

      case THREADSTATE_GETINPUTSTATE:
         ret = LOWORD(IntGetQueueStatus(QS_POSTMESSAGE|QS_TIMER|QS_PAINT|QS_SENDMESSAGE|QS_INPUT)) & (QS_KEY | QS_MOUSEBUTTON);
         break;
   }

   TRACE("Leave NtUserGetThreadState, ret=%i\n", ret);
   UserLeave();

   return ret;
}

DWORD
APIENTRY
NtUserSetThreadState(
   DWORD Set,
   DWORD Flags)
{
   PTHREADINFO pti;
   DWORD Ret = 0;
   // Test the only flags user can change.
   if (Set & ~(QF_FF10STATUS|QF_DIALOGACTIVE|QF_TABSWITCHING|QF_FMENUSTATUS|QF_FMENUSTATUSBREAK)) return 0;
   if (Flags & ~(QF_FF10STATUS|QF_DIALOGACTIVE|QF_TABSWITCHING|QF_FMENUSTATUS|QF_FMENUSTATUSBREAK)) return 0;   
   UserEnterExclusive();
   pti = PsGetCurrentThreadWin32Thread();
   if (pti->MessageQueue)
   {
      Ret = pti->MessageQueue->QF_flags;    // Get the queue flags.
      if (Set)
         pti->MessageQueue->QF_flags |= (Set&Flags); // Set the queue flags.
      else
      {
         if (Flags) pti->MessageQueue->QF_flags &= ~Flags; // Clr the queue flags.
      }
   }
   UserLeave();
   return Ret;
}

UINT
APIENTRY
NtUserGetDoubleClickTime(VOID)
{
   UINT Result;

   TRACE("Enter NtUserGetDoubleClickTime\n");
   UserEnterShared();

   // FIXME: Check if this works on non-interactive winsta
   Result = gspv.iDblClickTime;

   TRACE("Leave NtUserGetDoubleClickTime, ret=%i\n", Result);
   UserLeave();
   return Result;
}

BOOL
APIENTRY
NtUserGetGUIThreadInfo(
   DWORD idThread, /* If NULL use foreground thread */
   LPGUITHREADINFO lpgui)
{
   NTSTATUS Status;
   PTHRDCARETINFO CaretInfo;
   GUITHREADINFO SafeGui;
   PDESKTOP Desktop;
   PUSER_MESSAGE_QUEUE MsgQueue;
   PTHREADINFO W32Thread;
   PETHREAD Thread = NULL;

   DECLARE_RETURN(BOOLEAN);

   TRACE("Enter NtUserGetGUIThreadInfo\n");
   UserEnterShared();

   Status = MmCopyFromCaller(&SafeGui, lpgui, sizeof(DWORD));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   if(SafeGui.cbSize != sizeof(GUITHREADINFO))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   if (idThread)
   {
      Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)idThread, &Thread);
      if(!NT_SUCCESS(Status))
      {
         EngSetLastError(ERROR_ACCESS_DENIED);
         RETURN( FALSE);
      }
      W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread;
      Desktop = W32Thread->rpdesk;
   }
   else
   {  /* Get the foreground thread */
      Thread = PsGetCurrentThread();
      W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread;
      Desktop = W32Thread->rpdesk;
   }

   if (!Thread || !Desktop )
   {
      if(idThread && Thread)
         ObDereferenceObject(Thread);
      EngSetLastError(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   if ( W32Thread->MessageQueue )
      MsgQueue = W32Thread->MessageQueue;
   else
   {
      if ( Desktop ) MsgQueue = Desktop->ActiveMessageQueue;
   }

   CaretInfo = MsgQueue->CaretInfo;

   SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);

   if (MsgQueue->MenuOwner)
      SafeGui.flags |= GUI_INMENUMODE | MsgQueue->MenuState;

   if (MsgQueue->MoveSize)
      SafeGui.flags |= GUI_INMOVESIZE;

   /* FIXME: Add flag GUI_16BITTASK */

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

   if (idThread)
      ObDereferenceObject(Thread);

   Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserGetGUIThreadInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
APIENTRY
NtUserGetGuiResources(
   HANDLE hProcess,
   DWORD uiFlags)
{
   PEPROCESS Process;
   PPROCESSINFO W32Process;
   NTSTATUS Status;
   DWORD Ret = 0;
   DECLARE_RETURN(DWORD);

   TRACE("Enter NtUserGetGuiResources\n");
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

   W32Process = (PPROCESSINFO)Process->Win32Process;
   if(!W32Process)
   {
      ObDereferenceObject(Process);
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   switch(uiFlags)
   {
      case GR_GDIOBJECTS:
         {
            Ret = (DWORD)W32Process->GDIHandleCount;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserHandleCount;
            break;
         }
      default:
         {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            break;
         }
   }

   ObDereferenceObject(Process);

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserGetGuiResources, ret=%i\n",_ret_);
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
         ExFreePoolWithTag(Dest->Buffer, TAG_STRING);
         Dest->Buffer = NULL;
         return Status;
      }


      return STATUS_SUCCESS;
   }

   /* String is empty */
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
         ExFreePoolWithTag(Dest->Buffer, TAG_STRING);
         Dest->Buffer = NULL;
         return Status;
      }

      /* Make sure the string is null-terminated */
      Src = (PWSTR)((PBYTE)Dest->Buffer + Dest->Length);
      *Src = L'\0';

      return STATUS_SUCCESS;
   }

   /* String is empty */
   return STATUS_SUCCESS;
}

PPROCESSINFO
GetW32ProcessInfo(VOID)
{
    return (PPROCESSINFO)PsGetCurrentProcessWin32Process();
}

PTHREADINFO
GetW32ThreadInfo(VOID)
{
    PTEB Teb;
    PPROCESSINFO ppi;
    PCLIENTINFO pci;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti == NULL)
    {
        /* FIXME: Temporary hack for system threads... */
        return NULL;
    }
    /* Initialize it */
    pti->ppi = ppi = GetW32ProcessInfo();

    if (pti->rpdesk != NULL)
    {
       pti->pDeskInfo = pti->rpdesk->pDeskInfo;
    }
    else
    {
       pti->pDeskInfo = NULL;
    }
    /* Update the TEB */
    Teb = NtCurrentTeb();
    pci = GetWin32ClientInfo();
    pti->pClientInfo = pci;
    _SEH2_TRY
    {
        if(Teb)
        {    
            ProbeForWrite( Teb,
                           sizeof(TEB),
                           sizeof(ULONG));

            Teb->Win32ThreadInfo = (PW32THREAD) pti;
        }

        pci->ppi = ppi;
        pci->fsHooks = pti->fsHooks;
        if (pti->KeyboardLayout) pci->hKL = pti->KeyboardLayout->hkl;
        pci->dwTIFlags = pti->TIF_flags;
        /* CI may not have been initialized. */
        if (!pci->pDeskInfo && pti->pDeskInfo)
        {
           if (!pci->ulClientDelta) pci->ulClientDelta = DesktopHeapGetUserDelta();

           pci->pDeskInfo = (PVOID)((ULONG_PTR)pti->pDeskInfo - pci->ulClientDelta);
        }
        if (pti->pcti && pci->pDeskInfo)
           pci->pClientThreadInfo = (PVOID)((ULONG_PTR)pti->pcti - pci->ulClientDelta);
        else
           pci->pClientThreadInfo = NULL;

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return pti;
}

/* EOF */
