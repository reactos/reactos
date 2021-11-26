/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             win32ss/user/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);


/*
 * NOTE: _scwprintf() is NOT exported by ntoskrnl.exe,
 * only _vscwprintf() is, so we need to implement it here.
 * Code comes from sdk/lib/crt/printf/_scwprintf.c .
 * See also win32ss/user/winsrv/usersrv/harderror.c .
 */
int
__cdecl
_scwprintf(
    const wchar_t *format,
    ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = _vscwprintf(format, args);
    va_end(args);

    return len;
}


/*
 * Test the Thread to verify and validate it. Hard to the core tests are required.
 */
PTHREADINFO
FASTCALL
IntTID2PTI(HANDLE id)
{
   NTSTATUS Status;
   PETHREAD Thread;
   PTHREADINFO pti;
   Status = PsLookupThreadByThreadId(id, &Thread);
   if (!NT_SUCCESS(Status))
   {
      return NULL;
   }
   if (PsIsThreadTerminating(Thread))
   {
      ObDereferenceObject(Thread);
      return NULL;
   }
   pti = PsGetThreadWin32Thread(Thread);
   if (!pti)
   {
      ObDereferenceObject(Thread);
      return NULL;
   }
   // Validate and verify!
   _SEH2_TRY
   {
      if (pti->TIF_flags & TIF_INCLEANUP) pti = NULL;
      if (pti && !(pti->TIF_flags & TIF_GUITHREADINITIALIZED)) pti = NULL;
      if (PsGetThreadId(Thread) != id) pti = NULL;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      pti = NULL;
   }
   _SEH2_END
   ObDereferenceObject(Thread);
   return pti;
}

DWORD
FASTCALL
UserGetLanguageToggle(VOID)
{
    NTSTATUS Status;
    DWORD dwValue = 0;

    Status = RegReadUserSetting(L"Keyboard Layout\\Toggle", L"Layout Hotkey", REG_SZ, &dwValue, sizeof(dwValue));
    if (NT_SUCCESS(Status))
    {
        dwValue = atoi((char *)&dwValue);
        TRACE("Layout Hotkey %d\n",dwValue);
    }
    return dwValue;
}

USHORT
FASTCALL
UserGetLanguageID(VOID)
{
  HANDLE KeyHandle;
  OBJECT_ATTRIBUTES ObAttr;
//  http://support.microsoft.com/kb/324097
  ULONG Ret = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
  PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
  ULONG Size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
  UNICODE_STRING Language;

  RtlInitUnicodeString( &Language,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language");

  InitializeObjectAttributes( &ObAttr,
                              &Language,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
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
        if (!NT_SUCCESS(RtlUnicodeStringToInteger(&Language, 16, &Ret)))
        {
            Ret = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
        }
      }
      ExFreePoolWithTag(pKeyInfo, TAG_STRING);
    }
    ZwClose(KeyHandle);
  }
  TRACE("Language ID = %x\n",Ret);
  return (USHORT) Ret;
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
           TRACE("THREADSTATE_INSENDMESSAGE\n");

           ret = ISMEX_NOSEND;
           if (Message)
           {
             if (Message->ptiSender)
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
           pti = PsGetCurrentThreadWin32Thread();
           pti->pcti->timeLastRead = EngGetTickCount32();
           break;
         }

      case THREADSTATE_GETINPUTSTATE:
         ret = LOWORD(IntGetQueueStatus(QS_POSTMESSAGE|QS_TIMER|QS_PAINT|QS_SENDMESSAGE|QS_INPUT)) & (QS_KEY | QS_MOUSEBUTTON);
         break;

      case THREADSTATE_FOREGROUNDTHREAD:
         ret = (gpqForeground == GetW32ThreadInfo()->MessageQueue);
         break;
      case THREADSTATE_GETCURSOR:
         ret = (DWORD_PTR) (GetW32ThreadInfo()->MessageQueue->CursorObject ?
                            UserHMGetHandle(GetW32ThreadInfo()->MessageQueue->CursorObject) : 0);
         break;
      case THREADSTATE_GETMESSAGEEXTRAINFO:
         ret = (DWORD_PTR)MsqGetMessageExtraInfo();
        break;
      case THREADSTATE_UNKNOWN13:
         ret = FALSE; /* FIXME: See imm32 */
         break;
   }

   TRACE("Leave NtUserGetThreadState, ret=%lu\n", ret);
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

   TRACE("Leave NtUserGetDoubleClickTime, ret=%u\n", Result);
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
   PTHREADINFO W32Thread, pti;

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
      pti = PsGetCurrentThreadWin32Thread();

      // Validate Tread ID
      W32Thread = IntTID2PTI((HANDLE)(DWORD_PTR)idThread);

      if ( !W32Thread )
      {
          EngSetLastError(ERROR_ACCESS_DENIED);
          RETURN( FALSE);
      }

      Desktop = W32Thread->rpdesk;

      // Check Desktop and it must be the same as current.
      if ( !Desktop || Desktop != pti->rpdesk )
      {
          EngSetLastError(ERROR_ACCESS_DENIED);
          RETURN( FALSE);
      }

      if ( W32Thread->MessageQueue )
        MsgQueue = W32Thread->MessageQueue;
      else
      {
        MsgQueue = Desktop->ActiveMessageQueue;
      }
   }
   else
   {  /* Get the foreground thread */
      /* FIXME: Handle NULL queue properly? */
      MsgQueue = IntGetFocusMessageQueue();
      if(!MsgQueue)
      {
        EngSetLastError(ERROR_ACCESS_DENIED);
        RETURN( FALSE);
      }
   }

   CaretInfo = &MsgQueue->CaretInfo;

   SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);
/*
   if (W32Thread->pMenuState->pGlobalPopupMenu)
   {
       SafeGui.flags |= GUI_INMENUMODE;

       if (W32Thread->pMenuState->pGlobalPopupMenu->spwndNotify)
          SafeGui.hwndMenuOwner = UserHMGetHandle(W32Thread->pMenuState->pGlobalPopupMenu->spwndNotify);

       if (W32Thread->pMenuState->pGlobalPopupMenu->fHasMenuBar)
       {
          if (W32Thread->pMenuState->pGlobalPopupMenu->fIsSysMenu)
          {
             SafeGui.flags |= GUI_SYSTEMMENUMODE;
          }
       }
       else
       {
          SafeGui.flags |= GUI_POPUPMENUMODE;
       }
   }
 */
   SafeGui.hwndMenuOwner = MsgQueue->MenuOwner;

   if (MsgQueue->MenuOwner)
      SafeGui.flags |= GUI_INMENUMODE | MsgQueue->MenuState;

   if (MsgQueue->MoveSize)
      SafeGui.flags |= GUI_INMOVESIZE;

   /* FIXME: Add flag GUI_16BITTASK */

   SafeGui.hwndActive = MsgQueue->spwndActive ? UserHMGetHandle(MsgQueue->spwndActive) : 0;
   SafeGui.hwndFocus = MsgQueue->spwndFocus ? UserHMGetHandle(MsgQueue->spwndFocus) : 0;
   SafeGui.hwndCapture = MsgQueue->spwndCapture ? UserHMGetHandle(MsgQueue->spwndCapture) : 0;
   SafeGui.hwndMoveSize = MsgQueue->MoveSize;
   SafeGui.hwndCaret = CaretInfo->hWnd;

   SafeGui.rcCaret.left = CaretInfo->Pos.x;
   SafeGui.rcCaret.top = CaretInfo->Pos.y;
   SafeGui.rcCaret.right = SafeGui.rcCaret.left + CaretInfo->Size.cx;
   SafeGui.rcCaret.bottom = SafeGui.rcCaret.top + CaretInfo->Size.cy;

   Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserGetGUIThreadInfo, ret=%u\n",_ret_);
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
                                      *PsProcessType,
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
   TRACE("Leave NtUserGetGuiResources, ret=%lu\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

VOID FASTCALL
IntSetWindowState(PWND pWnd, UINT Flag)
{
   UINT bit;
   if (gptiCurrent->ppi != pWnd->head.pti->ppi) return;
   bit = 1 << LOWORD(Flag);
   TRACE("SWS %x\n",bit);
   switch(HIWORD(Flag))
   {
      case 0:
          pWnd->state |= bit;
          break;
      case 1:
          pWnd->state2 |= bit;
          break;
      case 2:
          pWnd->ExStyle2 |= bit;
          break;
   }
}

VOID FASTCALL
IntClearWindowState(PWND pWnd, UINT Flag)
{
   UINT bit;
   if (gptiCurrent->ppi != pWnd->head.pti->ppi) return;
   bit = 1 << LOWORD(Flag);
   TRACE("CWS %x\n",bit);
   switch(HIWORD(Flag))
   {
      case 0:
          pWnd->state &= ~bit;
          break;
      case 1:
          pWnd->state2 &= ~bit;
          break;
      case 2:
          pWnd->ExStyle2 &= ~bit;
          break;
   }
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

void UserDbgAssertThreadInfo(BOOL showCaller)
{
    PTEB Teb;
    PPROCESSINFO ppi;
    PCLIENTINFO pci;
    PTHREADINFO pti;

    ppi = PsGetCurrentProcessWin32Process();
    pti = PsGetCurrentThreadWin32Thread();
    Teb = NtCurrentTeb();
    pci = GetWin32ClientInfo();

    ASSERT(Teb);
    ASSERT(pti);
    ASSERT(pti->ppi == ppi);
    ASSERT(pti->pClientInfo == pci);
    ASSERT(Teb->Win32ThreadInfo == pti);
    ASSERT(pci->ppi == ppi);
    ASSERT(pci->fsHooks == pti->fsHooks);
    ASSERT(pci->ulClientDelta == DesktopHeapGetUserDelta());
    if (pti->pcti && pci->pDeskInfo)
        ASSERT(pci->pClientThreadInfo == (PVOID)((ULONG_PTR)pti->pcti - pci->ulClientDelta));
    //if (pti->pcti && IsListEmpty(&pti->SentMessagesListHead))
    //    ASSERT((pti->pcti->fsChangeBits & QS_SENDMESSAGE) == 0);
    if (pti->KeyboardLayout)
        ASSERT(pci->hKL == pti->KeyboardLayout->hkl);
    if(pti->rpdesk != NULL)
        ASSERT(pti->pDeskInfo == pti->rpdesk->pDeskInfo);

    /*too bad we still get this assertion*/

    // Why? Not all flags are passed to the user and doing so could crash the system........

    /* ASSERT(pci->dwTIFlags == pti->TIF_flags); */
/*    if(pci->dwTIFlags != pti->TIF_flags)
    {
        ERR("pci->dwTIFlags(0x%x) doesn't match pti->TIF_flags(0x%x)\n", pci->dwTIFlags, pti->TIF_flags);
        if(showCaller)
        {
            DbgPrint("Caller:\n");
            KeRosDumpStackFrames(NULL, 10);
        }
        pci->dwTIFlags = pti->TIF_flags;
    }
*/
}

void
NTAPI
UserDbgPreServiceHook(ULONG ulSyscallId, PULONG_PTR pulArguments)
{
    UserDbgAssertThreadInfo(FALSE);
}

ULONG_PTR
NTAPI
UserDbgPostServiceHook(ULONG ulSyscallId, ULONG_PTR ulResult)
{
    /* Make sure that the first syscall is NtUserInitialize */
    /* too bad this fails */
    // ASSERT(gpepCSRSS);

    UserDbgAssertThreadInfo(TRUE);

    return ulResult;
}


PPROCESSINFO
GetW32ProcessInfo(VOID)
{
    return (PPROCESSINFO)PsGetCurrentProcessWin32Process();
}

PTHREADINFO
GetW32ThreadInfo(VOID)
{
    UserDbgAssertThreadInfo(TRUE);
    return (PTHREADINFO)PsGetCurrentThreadWin32Thread();
}


NTSTATUS
GetProcessLuid(
    IN PETHREAD Thread OPTIONAL,
    IN PEPROCESS Process OPTIONAL,
    OUT PLUID Luid)
{
    NTSTATUS Status;
    PACCESS_TOKEN Token = NULL;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    BOOLEAN CopyOnOpen, EffectiveOnly;

    if (Thread && Process)
        return STATUS_INVALID_PARAMETER;

    /* If nothing has been specified, use the current thread */
    if (!Thread && !Process)
        Thread = PsGetCurrentThread();

    if (Thread)
    {
        /* Use a thread token */
        ASSERT(!Process);
        Token = PsReferenceImpersonationToken(Thread,
                                              &CopyOnOpen,
                                              &EffectiveOnly,
                                              &ImpersonationLevel);

        /* If we don't have a thread token, use a process token */
        if (!Token)
            Process = PsGetThreadProcess(Thread);
    }
    if (!Token && Process)
    {
        /* Use a process token */
        Token = PsReferencePrimaryToken(Process);

        /* If we don't have a token, fail */
        if (!Token)
            return STATUS_NO_TOKEN;
    }
    ASSERT(Token);

    /* Query the LUID */
    Status = SeQueryAuthenticationIdToken(Token, Luid);

    /* Get rid of the token and return */
    ObDereferenceObject(Token);
    return Status;
}

/* EOF */
