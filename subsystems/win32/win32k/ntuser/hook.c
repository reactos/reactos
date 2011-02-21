/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window hooks
 * FILE:             subsystems/win32/win32k/ntuser/hook.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   James Tabor (james.tabor@rectos.org)
 *
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 * NOTE:             Most of this code was adapted from Wine,
 *                   Copyright (C) 2002 Alexandre Julliard
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

typedef struct _HOOKPACK
{
  PHOOK pHk; 
  LPARAM lParam;
  PVOID pHookStructs;
} HOOKPACK, *PHOOKPACK;

/* PRIVATE FUNCTIONS *********************************************************/

static
LRESULT
FASTCALL
IntCallLowLevelHook( PHOOK Hook,
                     INT Code,
                     WPARAM wParam,
                     LPARAM lParam)
{
    NTSTATUS Status;
    PTHREADINFO pti;
    PHOOKPACK pHP;
    INT Size;
    UINT uTimeout = 300;
    BOOL Block = FALSE;
    ULONG_PTR uResult = 0;

    if (Hook->Thread)
       pti = Hook->Thread->Tcb.Win32Thread;
    else
       pti = Hook->head.pti;

    pHP = ExAllocatePoolWithTag(NonPagedPool, sizeof(HOOKPACK), TAG_HOOK);
    if (!pHP) return 0;

    pHP->pHk = Hook;
    pHP->lParam = lParam;
    pHP->pHookStructs = NULL;
    Size = 0;

// This prevents stack corruption from the caller.
    switch(Hook->HookId)
    {
       case WH_JOURNALPLAYBACK:
       case WH_JOURNALRECORD:
          uTimeout = 0;
          Size = sizeof(EVENTMSG);
          break;
       case WH_KEYBOARD_LL:
          Size = sizeof(KBDLLHOOKSTRUCT);
          break;
       case WH_MOUSE_LL:
          Size = sizeof(MSLLHOOKSTRUCT);
          break;
       case WH_MOUSE:
          uTimeout = 200;
          Block = TRUE;
          Size = sizeof(MOUSEHOOKSTRUCT);
          break;
       case WH_KEYBOARD:
          uTimeout = 200;
          Block = TRUE;
          Size = sizeof(KBDLLHOOKSTRUCT);
          break;
    }

    if (Size)
    {
       pHP->pHookStructs = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_HOOK);
       if (pHP->pHookStructs) RtlCopyMemory(pHP->pHookStructs, (PVOID)lParam, Size);
    }

    /* FIXME should get timeout from
     * HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout */
    Status = co_MsqSendMessage( pti->MessageQueue,
                                IntToPtr(Code), // hWnd
                                Hook->HookId,   // Msg
                                wParam,
                               (LPARAM)pHP,
                                uTimeout,
                                Block,
                                MSQ_ISHOOK,
                               &uResult);
    if (!NT_SUCCESS(Status))
    {
       DPRINT1("Error Hook Call SendMsg. %d Status: 0x%x\n", Hook->HookId, Status);
       if (pHP->pHookStructs) ExFreePoolWithTag(pHP->pHookStructs, TAG_HOOK);
       ExFreePoolWithTag(pHP, TAG_HOOK);
    }
    return NT_SUCCESS(Status) ? uResult : 0;
}


//
// Dispatch MsgQueue Hook Call processor!
//
LRESULT
FASTCALL
co_CallHook( INT HookId,
             INT Code,
             WPARAM wParam,
             LPARAM lParam)
{
    LRESULT Result;
    PHOOK phk;
    PHOOKPACK pHP = (PHOOKPACK)lParam;

    phk = pHP->pHk;
    lParam = pHP->lParam;

    switch(HookId)
    {
       case WH_JOURNALPLAYBACK:
       case WH_JOURNALRECORD:
       case WH_KEYBOARD:
       case WH_KEYBOARD_LL:
       case WH_MOUSE_LL:
       case WH_MOUSE:
          lParam = (LPARAM)pHP->pHookStructs;
          break;
    }

    /* The odds are high for this to be a Global call. */
    Result = co_IntCallHookProc( HookId,
                                 Code,
                                 wParam,
                                 lParam,
                                 phk->Proc,
                                 phk->Ansi,
                                &phk->ModuleName);

    /* The odds so high, no one is waiting for the results. */
    if (pHP->pHookStructs) ExFreePoolWithTag(pHP->pHookStructs, TAG_HOOK);
    ExFreePoolWithTag(pHP, TAG_HOOK);
    return Result;
}

static
LRESULT
FASTCALL
co_HOOK_CallHookNext( PHOOK Hook,
                      INT Code,
                      WPARAM wParam,
                      LPARAM lParam)
{
    DPRINT("Calling Next HOOK %d\n", Hook->HookId);

    return co_IntCallHookProc( Hook->HookId,
                               Code,
                               wParam,
                               lParam,
                               Hook->Proc,
                               Hook->Ansi,
                              &Hook->ModuleName);
}

LRESULT
FASTCALL
IntCallDebugHook( PHOOK Hook,
                  int Code,
                  WPARAM wParam,
                  LPARAM lParam,
                  BOOL Ansi)
{
    LRESULT lResult = 0;
    ULONG Size;
    DEBUGHOOKINFO Debug;
    PVOID HooklParam = NULL;
    BOOL BadChk = FALSE;

    if (lParam)
    {
        _SEH2_TRY
        {
            ProbeForRead((PVOID)lParam,
                         sizeof(DEBUGHOOKINFO),
                         1);

            RtlCopyMemory(&Debug,
                          (PVOID)lParam,
                          sizeof(DEBUGHOOKINFO));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            BadChk = TRUE;
        }
        _SEH2_END;

        if (BadChk)
        {
            DPRINT1("HOOK WH_DEBUG read from lParam ERROR!\n");
            return lResult;
        }
    }
    else
        return lResult; /* Need lParam! */

    switch (wParam)
    {
        case WH_CBT:
        {
            switch (Debug.code)
            {
                case HCBT_CLICKSKIPPED:
                    Size = sizeof(MOUSEHOOKSTRUCTEX);
                    break;

                case HCBT_MOVESIZE:
                    Size = sizeof(RECT);
                    break;

                case HCBT_ACTIVATE:
                    Size = sizeof(CBTACTIVATESTRUCT); 
                    break;

                case HCBT_CREATEWND: /* Handle Ansi? */
                    Size = sizeof(CBT_CREATEWND);
                    /* What shall we do? Size += sizeof(HOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS); same as CREATESTRUCTEX */
                    break;

                default:
                    Size = sizeof(LPARAM);
            }
        }
        break;

        case WH_MOUSE_LL:
            Size = sizeof(MSLLHOOKSTRUCT);
            break;

        case WH_KEYBOARD_LL:
            Size = sizeof(KBDLLHOOKSTRUCT);
            break;

        case WH_MSGFILTER:
        case WH_SYSMSGFILTER:
        case WH_GETMESSAGE:
            Size = sizeof(MSG);
            break;

        case WH_JOURNALPLAYBACK:
        case WH_JOURNALRECORD:
            Size = sizeof(EVENTMSG);
            break;

        case WH_FOREGROUNDIDLE:
        case WH_KEYBOARD:
        case WH_SHELL:
        default:
            Size = sizeof(LPARAM);
    }

    if (Size > sizeof(LPARAM))
        HooklParam = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_HOOK);

    if (HooklParam)
    {
        _SEH2_TRY
        {
            ProbeForRead((PVOID)Debug.lParam,
                         Size,
                         1);

            RtlCopyMemory(HooklParam,
                          (PVOID)Debug.lParam,
                          Size);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            BadChk = TRUE;
        }
        _SEH2_END;

        if (BadChk)
        {
            DPRINT1("HOOK WH_DEBUG read from Debug.lParam ERROR!\n");
            ExFreePool(HooklParam);
            return lResult;
        }
    }

    if (HooklParam) Debug.lParam = (LPARAM)HooklParam;
    lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Debug);
    if (HooklParam) ExFreePoolWithTag(HooklParam, TAG_HOOK);

    return lResult;
}

LRESULT
FASTCALL
UserCallNextHookEx( PHOOK Hook,
                    int Code,
                    WPARAM wParam,
                    LPARAM lParam,
                    BOOL Ansi)
{
    LRESULT lResult = 0;
    BOOL BadChk = FALSE;

    /* Handle this one first. */
    if ((Hook->HookId == WH_MOUSE) ||
        (Hook->HookId == WH_CBT && Code == HCBT_CLICKSKIPPED))
    {
        MOUSEHOOKSTRUCTEX Mouse;
        if (lParam)
        {
            _SEH2_TRY
            {
                ProbeForRead((PVOID)lParam,
                             sizeof(MOUSEHOOKSTRUCTEX),
                             1);

                RtlCopyMemory(&Mouse,
                              (PVOID)lParam,
                              sizeof(MOUSEHOOKSTRUCTEX));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                BadChk = TRUE;
            }
            _SEH2_END;

            if (BadChk)
            {
                DPRINT1("HOOK WH_MOUSE read from lParam ERROR!\n");
            }
        }

        if (!BadChk)
        {
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Mouse);
        }

        return lResult;
    }

    switch(Hook->HookId)
    {
        case WH_MOUSE_LL:
        {
            MSLLHOOKSTRUCT Mouse;

            if (lParam)
            {
                _SEH2_TRY
                {
                    ProbeForRead((PVOID)lParam,
                                 sizeof(MSLLHOOKSTRUCT),
                                 1);

                    RtlCopyMemory(&Mouse,
                                  (PVOID)lParam,
                                  sizeof(MSLLHOOKSTRUCT));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    BadChk = TRUE;
                }
                _SEH2_END;

                if (BadChk)
                {
                    DPRINT1("HOOK WH_MOUSE_LL read from lParam ERROR!\n");
                }
            }

            if (!BadChk)
            {
                lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Mouse);
            }
            break;
        }

        case WH_KEYBOARD_LL:
        {
            KBDLLHOOKSTRUCT Keyboard;

            if (lParam)
            {
                _SEH2_TRY
                {
                    ProbeForRead((PVOID)lParam,
                                 sizeof(KBDLLHOOKSTRUCT),
                                 1);

                    RtlCopyMemory(&Keyboard,
                                  (PVOID)lParam,
                                  sizeof(KBDLLHOOKSTRUCT));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    BadChk = TRUE;
                }
                _SEH2_END;

                if (BadChk)
                {
                    DPRINT1("HOOK WH_KEYBORD_LL read from lParam ERROR!\n");
                }
            }

            if (!BadChk)
            {
                lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Keyboard);
            }
            break;
        }

        case WH_MSGFILTER:
        case WH_SYSMSGFILTER:
        case WH_GETMESSAGE:
        {
            MSG Msg;

            if (lParam)
            {
                _SEH2_TRY
                {
                    ProbeForRead((PVOID)lParam,
                                 sizeof(MSG),
                                 1);

                    RtlCopyMemory(&Msg,
                                  (PVOID)lParam,
                                  sizeof(MSG));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    BadChk = TRUE;
                }
                _SEH2_END;

                if (BadChk)
                {
                    DPRINT1("HOOK WH_XMESSAGEX read from lParam ERROR!\n");
                }
            }

            if (!BadChk)
            {
                lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Msg);

                if (lParam && (Hook->HookId == WH_GETMESSAGE))
                {
                    _SEH2_TRY
                    {
                        ProbeForWrite((PVOID)lParam,
                                      sizeof(MSG),
                                      1);

                        RtlCopyMemory((PVOID)lParam,
                                      &Msg,
                                      sizeof(MSG));
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        BadChk = TRUE;
                    }
                    _SEH2_END;

                    if (BadChk)
                    {
                        DPRINT1("HOOK WH_GETMESSAGE write to lParam ERROR!\n");
                    }
                }
            }
            break;
        }

        case WH_CBT:
            DPRINT("HOOK WH_CBT!\n");
            switch (Code)
            {
                case HCBT_CREATEWND:
                {
                    LPCBT_CREATEWNDW pcbtcww = (LPCBT_CREATEWNDW)lParam;

                    DPRINT("HOOK HCBT_CREATEWND\n");
                    _SEH2_TRY
                    {
                        if (Ansi)
                        {
                            ProbeForRead( pcbtcww,
                                          sizeof(CBT_CREATEWNDA),
                                          1);
                            ProbeForWrite(pcbtcww->lpcs,
                                          sizeof(CREATESTRUCTA),
                                          1);
                            ProbeForRead( pcbtcww->lpcs->lpszName,
                                          sizeof(CHAR),
                                          1);

                            if (!IS_ATOM(pcbtcww->lpcs->lpszClass))
                            {
                               ProbeForRead( pcbtcww->lpcs->lpszClass,
                                             sizeof(CHAR),
                                             1);
                            }
                        }
                        else
                        {
                            ProbeForRead( pcbtcww,
                                          sizeof(CBT_CREATEWNDW),
                                          1);
                            ProbeForWrite(pcbtcww->lpcs,
                                          sizeof(CREATESTRUCTW),
                                          1);
                            ProbeForRead( pcbtcww->lpcs->lpszName,
                                          sizeof(WCHAR),
                                          1);

                            if (!IS_ATOM(pcbtcww->lpcs->lpszClass))
                            {
                               ProbeForRead( pcbtcww->lpcs->lpszClass,
                                             sizeof(WCHAR),
                                             1);
                            }
                        }
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        BadChk = TRUE;
                    }
                    _SEH2_END;

                    if (BadChk)
                    {
                        DPRINT1("HOOK HCBT_CREATEWND write ERROR!\n");
                    }
                    /* The next call handles the structures. */
                    if (!BadChk && Hook->Proc)
                    {
                       lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);
                    }
                    break;
                }

                case HCBT_MOVESIZE:
                {
                    RECTL rt;

                    DPRINT("HOOK HCBT_MOVESIZE\n");

                    if (lParam)
                    {
                        _SEH2_TRY
                        {
                            ProbeForRead((PVOID)lParam,
                                         sizeof(RECT),
                                         1);

                            RtlCopyMemory(&rt,
                                          (PVOID)lParam,
                                          sizeof(RECT));
                        }
                        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                        {
                            BadChk = TRUE;
                        }
                        _SEH2_END;

                        if (BadChk)
                        {
                            DPRINT1("HOOK HCBT_MOVESIZE read from lParam ERROR!\n");
                        }
                    }

                    if (!BadChk)
                    {
                        lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&rt);
                    }
                    break;
                }

                case HCBT_ACTIVATE:
                {
                    CBTACTIVATESTRUCT CbAs;

                    DPRINT("HOOK HCBT_ACTIVATE\n");
                    if (lParam)
                    {
                        _SEH2_TRY
                        {
                            ProbeForRead((PVOID)lParam,
                                         sizeof(CBTACTIVATESTRUCT),
                                         1);

                            RtlCopyMemory(&CbAs,
                                          (PVOID)lParam,
                                          sizeof(CBTACTIVATESTRUCT));
                        }
                        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                        {
                            BadChk = TRUE;
                        }
                        _SEH2_END;

                        if (BadChk)
                        {
                            DPRINT1("HOOK HCBT_ACTIVATE read from lParam ERROR!\n");
                        }
                    }

                    if (!BadChk)
                    {
                        lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&CbAs);
                    }
                    break;
                }

                /* The rest just use default. */
                default:
                    DPRINT("HOOK HCBT_ %d\n",Code);
                    lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);
                    break;
            }
            break;
/*
 Note WH_JOURNALPLAYBACK,
    "To have the system wait before processing the message, the return value
     must be the amount of time, in clock ticks, that the system should wait."
 */
        case WH_JOURNALPLAYBACK:
        case WH_JOURNALRECORD:
        {
            EVENTMSG EventMsg;

            if (lParam)
            {
                _SEH2_TRY
                {
                    ProbeForRead((PVOID)lParam,
                                 sizeof(EVENTMSG),
                                 1);

                    RtlCopyMemory(&EventMsg,
                                  (PVOID)lParam,
                                  sizeof(EVENTMSG));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    BadChk = TRUE;
                }
                _SEH2_END;

                if (BadChk)
                {
                    DPRINT1("HOOK WH_JOURNAL read from lParam ERROR!\n");
                }
            }

            if (!BadChk) 
            {               
                lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)(lParam ? &EventMsg : NULL));

                if (lParam)
                {
                    _SEH2_TRY
                    {
                        ProbeForWrite((PVOID)lParam,
                                      sizeof(EVENTMSG),
                                      1);

                        RtlCopyMemory((PVOID)lParam,
                                      &EventMsg,
                                      sizeof(EVENTMSG));
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        BadChk = TRUE;
                    }
                    _SEH2_END;

                    if (BadChk)
                    {
                        DPRINT1("HOOK WH_JOURNAL write to lParam ERROR!\n");
                    }
                }
            }
            break;
        }

        case WH_DEBUG:
            lResult = IntCallDebugHook(Hook, Code, wParam, lParam, Ansi);
            break;

        /*
         * Default the rest like, WH_FOREGROUNDIDLE, WH_KEYBOARD and WH_SHELL.
         */
        case WH_FOREGROUNDIDLE:
        case WH_KEYBOARD:
        case WH_SHELL:
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);      
            break;

        default:
            DPRINT1("Unsupported HOOK Id -> %d\n",Hook->HookId);
            break;
    }
    return lResult; 
}

PHOOK
FASTCALL
IntGetHookObject(HHOOK hHook)
{
    PHOOK Hook;
    
    if (!hHook)
    {
       EngSetLastError(ERROR_INVALID_HOOK_HANDLE);
       return NULL;
    }

    Hook = (PHOOK)UserGetObject(gHandleTable, hHook, otHook);
    if (!Hook)
    {
       EngSetLastError(ERROR_INVALID_HOOK_HANDLE);
       return NULL;
    }

    UserReferenceObject(Hook);

    return Hook;
}

/* get the first hook in the chain */
static
PHOOK
FASTCALL
IntGetFirstHook(PLIST_ENTRY Table)
{
    PLIST_ENTRY Elem = Table->Flink;

    if (IsListEmpty(Table)) return NULL;

    return Elem == Table ? NULL : CONTAINING_RECORD(Elem, HOOK, Chain);
}

static
PHOOK
FASTCALL
IntGetNextGlobalHook(PHOOK Hook, PDESKTOP pdo)
{
    int HookId = Hook->HookId;
    PLIST_ENTRY Elem;

    Elem = Hook->Chain.Flink;
    if (Elem != &pdo->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)])
       return CONTAINING_RECORD(Elem, HOOK, Chain);
    return NULL;
}

/* find the next hook in the chain  */
PHOOK
FASTCALL
IntGetNextHook(PHOOK Hook)
{
    int HookId = Hook->HookId;
    PLIST_ENTRY Elem;
    PTHREADINFO pti;

    if (Hook->Thread)
    {
       pti = ((PTHREADINFO)Hook->Thread->Tcb.Win32Thread);

       Elem = Hook->Chain.Flink;
       if (Elem != &pti->aphkStart[HOOKID_TO_INDEX(HookId)])
          return CONTAINING_RECORD(Elem, HOOK, Chain);
    }
    else
    {
       pti = PsGetCurrentThreadWin32Thread();
       return IntGetNextGlobalHook(Hook, pti->rpdesk);
    }
    return NULL;
}

/* free a hook, removing it from its chain */
static
VOID
FASTCALL
IntFreeHook(PHOOK Hook)
{
    RemoveEntryList(&Hook->Chain);
    if (Hook->ModuleName.Buffer)
    {
       ExFreePoolWithTag(Hook->ModuleName.Buffer, TAG_HOOK);
       Hook->ModuleName.Buffer = NULL;
    }
    /* Close handle */
    UserDeleteObject(UserHMGetHandle(Hook), otHook);
}

/* remove a hook, freeing it from the chain */
static
BOOL
FASTCALL
IntRemoveHook(PHOOK Hook)
{
    INT HookId;
    PTHREADINFO pti;
    PDESKTOP pdo;

    HookId = Hook->HookId;

    if (Hook->Thread) // Local
    {
       pti = ((PTHREADINFO)Hook->Thread->Tcb.Win32Thread);

       IntFreeHook( Hook);

       if ( IsListEmpty(&pti->aphkStart[HOOKID_TO_INDEX(HookId)]) )
       {
          pti->fsHooks &= ~HOOKID_TO_FLAG(HookId);
          _SEH2_TRY
          {
             GetWin32ClientInfo()->fsHooks = pti->fsHooks;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
          }
          _SEH2_END;
          return TRUE;
       }
    }
    else // Global
    {
       IntFreeHook( Hook);

       pdo = IntGetActiveDesktop();

       if ( pdo &&
            pdo->pDeskInfo &&
            IsListEmpty(&pdo->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)]) )
       {
          pdo->pDeskInfo->fsHooks &= ~HOOKID_TO_FLAG(HookId);
          return TRUE;
       }
    }
    return FALSE;
}

VOID
FASTCALL
HOOK_DestroyThreadHooks(PETHREAD Thread)
{
   PTHREADINFO pti;
   PDESKTOP pdo;
   int HookId;
   PHOOK HookObj;
   PLIST_ENTRY pElem;

   pti = Thread->Tcb.Win32Thread;
   pdo = IntGetActiveDesktop();

   if (!pti || !pdo)
   {
      DPRINT1("Kill Thread Hooks pti 0x%x pdo 0x%x\n",pti,pdo);
      return;
   }
   ObReferenceObject(Thread);

// Local Thread cleanup.
   if (pti->fsHooks)
   {
      for (HookId = WH_MINHOOK; HookId <= WH_MAXHOOK; HookId++)
      {
         PLIST_ENTRY pLLE = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];

         if (IsListEmpty(pLLE)) continue;

         pElem = pLLE->Flink;
         HookObj = CONTAINING_RECORD(pElem, HOOK, Chain);
         do
         {
            if (!HookObj) break;
            if (IntRemoveHook(HookObj)) break;
            pElem = HookObj->Chain.Flink;
            HookObj = CONTAINING_RECORD(pElem, HOOK, Chain);
         }
         while (pElem != pLLE);
      }
      pti->fsHooks = 0;
   }
// Global search based on Thread and cleanup.
   if (pdo->pDeskInfo->fsHooks)
   {
      for (HookId = WH_MINHOOK; HookId <= WH_MAXHOOK; HookId++)
      {
         PLIST_ENTRY pGLE = &pdo->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)];

         if (IsListEmpty(pGLE)) continue;

         pElem = pGLE->Flink;
         HookObj = CONTAINING_RECORD(pElem, HOOK, Chain);
         do
         {
            if (!HookObj) break;
            if (HookObj->head.pti == pti)
            {
               if (IntRemoveHook(HookObj)) break;
            }
            pElem = HookObj->Chain.Flink;
            HookObj = CONTAINING_RECORD(pElem, HOOK, Chain);
         }
         while (pElem != pGLE);
      }
   }
   ObDereferenceObject(Thread);
   return;
}

/*
  Win32k Kernel Space Hook Caller.
 */
LRESULT
FASTCALL
co_HOOK_CallHooks( INT HookId,
                   INT Code,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PHOOK Hook, SaveHook;
    PTHREADINFO pti;
    PCLIENTINFO ClientInfo;
    PLIST_ENTRY pLLE, pGLE;
    PDESKTOP pdo;
    BOOL Local = FALSE, Global = FALSE;
    LRESULT Result = 0;

    ASSERT(WH_MINHOOK <= HookId && HookId <= WH_MAXHOOK);

    pti = PsGetCurrentThreadWin32Thread();
    if (!pti || !pti->rpdesk || !pti->rpdesk->pDeskInfo)
    {
       pdo = IntGetActiveDesktop();
    /* If KeyboardThread|MouseThread|(RawInputThread or RIT) aka system threads,
       pti->fsHooks most likely, is zero. So process KbT & MsT to "send" the message.
     */
       if ( !pti || !pdo || (!(HookId == WH_KEYBOARD_LL) && !(HookId == WH_MOUSE_LL)) )
       {
          DPRINT("No PDO %d\n", HookId);
          goto Exit;
       }
    }
    else
    {
       pdo = pti->rpdesk;
    }

    if ( pti->TIF_flags & (TIF_INCLEANUP|TIF_DISABLEHOOKS))
    {
       DPRINT("Hook Thread dead %d\n", HookId);
       goto Exit;
    }

    if ( ISITHOOKED(HookId) )
    {
       DPRINT("Local Hooker %d\n", HookId);
       Local = TRUE;
    }

    if ( pdo->pDeskInfo->fsHooks & HOOKID_TO_FLAG(HookId) )
    {
       DPRINT("Global Hooker %d\n", HookId);
       Global = TRUE;
    }

    if ( !Local && !Global ) goto Exit; // No work!

    Hook = NULL;

    /* SetWindowHookEx sorts out the Thread issue by placing the Hook to
       the correct Thread if not NULL.
     */
    if ( Local )
    {
       pLLE = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];
       Hook = IntGetFirstHook(pLLE);
       if (!Hook)
       {
          DPRINT1("No Local Hook Found!\n");
          goto Exit;
       }
       ObReferenceObject(Hook->Thread);

       ClientInfo = pti->pClientInfo;
       SaveHook = pti->sphkCurrent;

       /* Load it for the next call. */
       pti->sphkCurrent = Hook;
       Hook->phkNext = IntGetNextHook(Hook);
       if (ClientInfo)
       {
          _SEH2_TRY
          {
             ClientInfo->phkCurrent = Hook;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
             ClientInfo = NULL; // Don't bother next run.
          }
          _SEH2_END;
       }
       Result = co_IntCallHookProc( HookId,
                                    Code,
                                    wParam,
                                    lParam,
                                    Hook->Proc,
                                    Hook->Ansi,
                                   &Hook->ModuleName);
       if (ClientInfo)
       {
          _SEH2_TRY
          {
             ClientInfo->phkCurrent = SaveHook;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
          }
          _SEH2_END;
       }
       pti->sphkCurrent = SaveHook;
       Hook->phkNext = NULL;
       ObDereferenceObject(Hook->Thread);
    }

    if ( Global )
    {
       PTHREADINFO ptiHook;

       pGLE = &pdo->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)];
       Hook = IntGetFirstHook(pGLE);
       if (!Hook)
       {
          DPRINT1("No Global Hook Found!\n");
          goto Exit;
       }
      /* Performance goes down the drain. If more hooks are associated to this
       * hook ID, this will have to post to each of the thread message queues
       * or make a direct call.
       */
       do
       {
         /* Hook->Thread is null, we hax around this with Hook->head.pti. */
          ptiHook = Hook->head.pti;

         /* "Global hook monitors messages for all threads in the same desktop
          *  as the calling thread."
          */
          if ( ptiHook->TIF_flags & (TIF_INCLEANUP|TIF_DISABLEHOOKS) ||
               ptiHook->rpdesk != pdo)
          {
             DPRINT("Next Hook 0x%x, 0x%x\n",ptiHook->rpdesk,pdo);
             Hook = IntGetNextGlobalHook(Hook, pdo);
             if (!Hook) break;
             continue;
          }
          // Lockup the thread while this links through user world.
          ObReferenceObject(ptiHook->pEThread);
          if (ptiHook != pti )
          {                                       // Block | TimeOut
             if ( HookId == WH_JOURNALPLAYBACK || //   1   |    0
                  HookId == WH_JOURNALRECORD   || //   1   |    0
                  HookId == WH_KEYBOARD        || //   1   |   200
                  HookId == WH_MOUSE           || //   1   |   200
                  HookId == WH_KEYBOARD_LL     || //   0   |   300
                  HookId == WH_MOUSE_LL )         //   0   |   300
             {
                DPRINT("\nGlobal Hook posting to another Thread! %d\n",HookId );
                Result = IntCallLowLevelHook(Hook, Code, wParam, lParam);
             }
          }
          else
          { /* Make the direct call. */
             DPRINT("\nLocal Hook calling to Thread! %d\n",HookId );
             Result = co_IntCallHookProc( HookId,
                                          Code,
                                          wParam,
                                          lParam,
                                          Hook->Proc,
                                          Hook->Ansi,
                                         &Hook->ModuleName);
          }
          ObDereferenceObject(ptiHook->pEThread);
          Hook = IntGetNextGlobalHook(Hook, pdo);
       }
       while ( Hook );
       DPRINT("Ret: Global HookId %d Result 0x%x\n", HookId,Result);
    }
Exit:
    return Result;
}

BOOL
FASTCALL
IntUnhookWindowsHook(int HookId, HOOKPROC pfnFilterProc)
{
    PHOOK Hook;
    PLIST_ENTRY pLLE, pLE;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (HookId < WH_MINHOOK || WH_MAXHOOK < HookId )
    {
       EngSetLastError(ERROR_INVALID_HOOK_FILTER);
       return FALSE;
    }

    if (pti->fsHooks)
    {
       pLLE = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];

       if (IsListEmpty(pLLE)) return FALSE;

       pLE = pLLE->Flink;
       Hook = CONTAINING_RECORD(pLE, HOOK, Chain);
       do
       {
          if (!Hook) break;
          if (Hook->Proc == pfnFilterProc)
          {
             if (Hook->head.pti == pti)
             {
                IntRemoveHook(Hook);
                UserDereferenceObject(Hook);
                return TRUE;
             }
             else
             {
                EngSetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
             }
          }
          pLE = Hook->Chain.Flink;
          Hook = CONTAINING_RECORD(pLE, HOOK, Chain);
       }
       while (pLE != pLLE);
    }
    return FALSE;
}

/*
 *  Support for compatibility only? Global hooks are processed in kernel space.
 *  This is very thread specific! Never seeing applications with more than one
 *  hook per thread installed. Most of the applications are Global hookers and
 *  associated with just one hook Id. Maybe it's for diagnostic testing or a
 *  throw back to 3.11?
 */
LRESULT
APIENTRY
NtUserCallNextHookEx( int Code,
                      WPARAM wParam,
                      LPARAM lParam,
                      BOOL Ansi)
{
    PTHREADINFO pti;
    PHOOK HookObj, NextObj;
    PCLIENTINFO ClientInfo;
    LRESULT lResult = 0;
    DECLARE_RETURN(LRESULT);

    DPRINT("Enter NtUserCallNextHookEx\n");
    UserEnterExclusive();

    pti = GetW32ThreadInfo();

    HookObj = pti->sphkCurrent;

    if (!HookObj) RETURN( 0);

    NextObj = HookObj->phkNext;

    pti->sphkCurrent = NextObj;
    ClientInfo = pti->pClientInfo;
    _SEH2_TRY
    {
       ClientInfo->phkCurrent = NextObj;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
       ClientInfo = NULL;
    }
    _SEH2_END;

    /* Now in List run down. */
    if (ClientInfo && NextObj)
    {
       NextObj->phkNext = IntGetNextHook(NextObj);
       lResult = UserCallNextHookEx( NextObj, Code, wParam, lParam, NextObj->Ansi);
    }
    RETURN( lResult);

CLEANUP:
    DPRINT("Leave NtUserCallNextHookEx, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

HHOOK
APIENTRY
NtUserSetWindowsHookAW( int idHook, 
                        HOOKPROC lpfn,
                        BOOL Ansi)
{
    DWORD ThreadId;
    UNICODE_STRING USModuleName;

    RtlInitUnicodeString(&USModuleName, NULL);
    ThreadId = PtrToUint(NtCurrentTeb()->ClientId.UniqueThread);

    return NtUserSetWindowsHookEx( NULL,
                                  &USModuleName,
                                   ThreadId,
                                   idHook,
                                   lpfn,
                                   Ansi);
}

HHOOK
APIENTRY
NtUserSetWindowsHookEx( HINSTANCE Mod,
                        PUNICODE_STRING UnsafeModuleName,
                        DWORD ThreadId,
                        int HookId,
                        HOOKPROC HookProc,
                        BOOL Ansi)
{
    PWINSTATION_OBJECT WinStaObj;
    PHOOK Hook;
    UNICODE_STRING ModuleName;
    NTSTATUS Status;
    HHOOK Handle;
    PETHREAD Thread = NULL;
    PTHREADINFO ptiCurrent, pti = NULL;
    BOOL Hit = FALSE;
    DECLARE_RETURN(HHOOK);

    DPRINT("Enter NtUserSetWindowsHookEx\n");
    UserEnterExclusive();

    ptiCurrent = PsGetCurrentThreadWin32Thread();

    if (HookId < WH_MINHOOK || WH_MAXHOOK < HookId )
    {
        EngSetLastError(ERROR_INVALID_HOOK_FILTER);
        RETURN( NULL);
    }

    if (!HookProc)
    {
        EngSetLastError(ERROR_INVALID_FILTER_PROC);
        RETURN( NULL);
    }

    if (ThreadId)  /* thread-local hook */
    {
       if ( HookId == WH_JOURNALRECORD ||
            HookId == WH_JOURNALPLAYBACK ||
            HookId == WH_KEYBOARD_LL ||
            HookId == WH_MOUSE_LL ||
            HookId == WH_SYSMSGFILTER)
       {
           DPRINT1("Local hook installing Global HookId: %d\n",HookId);
           /* these can only be global */
           EngSetLastError(ERROR_GLOBAL_ONLY_HOOK);
           RETURN( NULL);
       }

       if (!NT_SUCCESS(PsLookupThreadByThreadId((HANDLE)(DWORD_PTR) ThreadId, &Thread)))
       {
          DPRINT1("Invalid thread id 0x%x\n", ThreadId);
          EngSetLastError(ERROR_INVALID_PARAMETER);
          RETURN( NULL);
       }

       pti = Thread->Tcb.Win32Thread;

       ObDereferenceObject(Thread);

       if ( pti->rpdesk != ptiCurrent->rpdesk) // gptiCurrent->rpdesk)
       {
          DPRINT1("Local hook wrong desktop HookId: %d\n",HookId);
          EngSetLastError(ERROR_ACCESS_DENIED);
          RETURN( NULL);
       }

       if (Thread->ThreadsProcess != PsGetCurrentProcess())
       {
          if ( !Mod &&
              (HookId == WH_GETMESSAGE ||
               HookId == WH_CALLWNDPROC ||
               HookId == WH_CBT ||
               HookId == WH_HARDWARE ||
               HookId == WH_DEBUG ||
               HookId == WH_SHELL ||
               HookId == WH_FOREGROUNDIDLE ||
               HookId == WH_CALLWNDPROCRET) )
          {
             DPRINT1("Local hook needs hMod HookId: %d\n",HookId);
             EngSetLastError(ERROR_HOOK_NEEDS_HMOD);
             RETURN( NULL);
          }

          if ( (pti->TIF_flags & (TIF_CSRSSTHREAD|TIF_SYSTEMTHREAD)) && 
               (HookId == WH_GETMESSAGE ||
                HookId == WH_CALLWNDPROC ||
                HookId == WH_CBT ||
                HookId == WH_HARDWARE ||
                HookId == WH_DEBUG ||
                HookId == WH_SHELL ||
                HookId == WH_FOREGROUNDIDLE ||
                HookId == WH_CALLWNDPROCRET) )
          {
             EngSetLastError(ERROR_HOOK_TYPE_NOT_ALLOWED);
             RETURN( NULL);
          }
       }
    }
    else  /* system-global hook */
    {                                                                                
       pti = ptiCurrent; // gptiCurrent;
       if ( !Mod &&
            (HookId == WH_GETMESSAGE ||
             HookId == WH_CALLWNDPROC ||
             HookId == WH_CBT ||
             HookId == WH_SYSMSGFILTER ||
             HookId == WH_HARDWARE ||
             HookId == WH_DEBUG ||
             HookId == WH_SHELL ||
             HookId == WH_FOREGROUNDIDLE ||
             HookId == WH_CALLWNDPROCRET) )
       {
          DPRINT1("Global hook needs hMod HookId: %d\n",HookId);
          EngSetLastError(ERROR_HOOK_NEEDS_HMOD);
          RETURN( NULL);
       }
    }

    Status = IntValidateWindowStationHandle( PsGetCurrentProcess()->Win32WindowStation,
                                             KernelMode,
                                             0,
                                            &WinStaObj);

    if (!NT_SUCCESS(Status))
    {
       SetLastNtError(Status);
       RETURN( NULL);
    }
    ObDereferenceObject(WinStaObj);

    Hook = UserCreateObject(gHandleTable, NULL, &Handle, otHook, sizeof(HOOK));

    if (!Hook)
    {
       RETURN( NULL);
    }

    Hook->ihmod   = (INT)Mod; // Module Index from atom table, Do this for now.
    Hook->Thread  = Thread; /* Set Thread, Null is Global. */
    Hook->HookId  = HookId;
    Hook->rpdesk  = pti->rpdesk;
    Hook->phkNext = NULL; /* Dont use as a chain! Use link lists for chaining. */
    Hook->Proc    = HookProc;
    Hook->Ansi    = Ansi;

    DPRINT("Set Hook Desk 0x%x DeskInfo 0x%x Handle Desk 0x%x\n",pti->rpdesk, pti->pDeskInfo,Hook->head.rpdesk);

    if (ThreadId)  /* thread-local hook */
    {
       InsertHeadList(&pti->aphkStart[HOOKID_TO_INDEX(HookId)], &Hook->Chain);
       pti->sphkCurrent = NULL;
       Hook->ptiHooked = pti;
       pti->fsHooks |= HOOKID_TO_FLAG(HookId);

       if (pti->pClientInfo)
       {
          if ( pti->ppi == ptiCurrent->ppi) /* gptiCurrent->ppi) */
          {
             _SEH2_TRY
             {
                pti->pClientInfo->fsHooks = pti->fsHooks;
                pti->pClientInfo->phkCurrent = NULL;
             }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
             {
                Hit = TRUE;
             }
             _SEH2_END;
             if (Hit)
             {
                DPRINT1("Problem writing to Local ClientInfo!\n");
             }
          }
          else
          {
             KeAttachProcess(&pti->ppi->peProcess->Pcb);
             _SEH2_TRY
             {
                pti->pClientInfo->fsHooks = pti->fsHooks;
                pti->pClientInfo->phkCurrent = NULL;
             }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
             {
                Hit = TRUE;
             }
             _SEH2_END;
             KeDetachProcess();
             if (Hit)
             {
                DPRINT1("Problem writing to Remote ClientInfo!\n");
             }
          }
       }
    }
    else
    {
       InsertHeadList(&pti->rpdesk->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)], &Hook->Chain);
       Hook->ptiHooked = NULL;
       //gptiCurrent->pDeskInfo->fsHooks |= HOOKID_TO_FLAG(HookId);
       pti->rpdesk->pDeskInfo->fsHooks |= HOOKID_TO_FLAG(HookId);
       pti->sphkCurrent = NULL;
       pti->pClientInfo->phkCurrent = NULL;
    }

    RtlInitUnicodeString(&Hook->ModuleName, NULL);

    if (Mod)
    {
       Status = MmCopyFromCaller(&ModuleName,
                                  UnsafeModuleName,
                                  sizeof(UNICODE_STRING));
       if (!NT_SUCCESS(Status))
       {
          IntRemoveHook(Hook);
          SetLastNtError(Status);
          RETURN( NULL);
       }

       Hook->ModuleName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                        ModuleName.MaximumLength,
                                                        TAG_HOOK);
       if (NULL == Hook->ModuleName.Buffer)
       {
          IntRemoveHook(Hook);
          EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
          RETURN( NULL);
       }

       Hook->ModuleName.MaximumLength = ModuleName.MaximumLength;
       Status = MmCopyFromCaller( Hook->ModuleName.Buffer,
                                  ModuleName.Buffer,
                                  ModuleName.MaximumLength);
       if (!NT_SUCCESS(Status))
       {
          ExFreePoolWithTag(Hook->ModuleName.Buffer, TAG_HOOK);
          Hook->ModuleName.Buffer = NULL;
          IntRemoveHook(Hook);
          SetLastNtError(Status);
          RETURN( NULL);
       }

       Hook->ModuleName.Length = ModuleName.Length;
       /* make proc relative to the module base */
       Hook->offPfn = (ULONG_PTR)((char *)HookProc - (char *)Mod);
    }
    else
       Hook->offPfn = 0;

    DPRINT("Installing: HookId %d Global %s\n", HookId, !ThreadId ? "TRUE" : "FALSE");
    RETURN( Handle);

CLEANUP:
    DPRINT("Leave NtUserSetWindowsHookEx, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL
APIENTRY
NtUserUnhookWindowsHookEx(HHOOK Hook)
{
    PHOOK HookObj;
    DECLARE_RETURN(BOOL);

    DPRINT("Enter NtUserUnhookWindowsHookEx\n");
    UserEnterExclusive();

    if (!(HookObj = IntGetHookObject(Hook)))
    {
        DPRINT1("Invalid handle passed to NtUserUnhookWindowsHookEx\n");
        /* SetLastNtError(Status); */
        RETURN( FALSE);
    }

    ASSERT(Hook == UserHMGetHandle(HookObj));

    IntRemoveHook(HookObj);

    UserDereferenceObject(HookObj);

    RETURN( TRUE);

CLEANUP:
    DPRINT("Leave NtUserUnhookWindowsHookEx, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

/* EOF */
