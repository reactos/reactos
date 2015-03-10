/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window hooks
 * FILE:             win32ss/user/ntuser/hook.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   James Tabor (james.tabor@rectos.org)
 *                   Rafal Harabien (rafalh@reactos.org)
  * NOTE:            Most of this code was adapted from Wine,
 *                   Copyright (C) 2002 Alexandre Julliard
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserHook);

typedef struct _HOOKPACK
{
  PHOOK pHk;
  LPARAM lParam;
  PVOID pHookStructs;
} HOOKPACK, *PHOOKPACK;

UNICODE_STRING strUahModule;
UNICODE_STRING strUahInitFunc;
PPROCESSINFO ppiUahServer;

/* PRIVATE FUNCTIONS *********************************************************/

/* Calls ClientLoadLibrary in user32 in order to load or unload a module */
BOOL
IntLoadHookModule(int iHookID, HHOOK hHook, BOOL Unload)
{
   PPROCESSINFO ppi;
   BOOL bResult;

   ppi = PsGetCurrentProcessWin32Process();

   TRACE("IntLoadHookModule. Client PID: %p\n", PsGetProcessId(ppi->peProcess));

    /* Check if this is the api hook */
    if(iHookID == WH_APIHOOK)
    {
        if(!Unload && !(ppi->W32PF_flags & W32PF_APIHOOKLOADED))
        {
            /* A callback in user mode can trigger UserLoadApiHook to be called and
               as a result IntLoadHookModule will be called recursively.
               To solve this we set the flag that means that the appliaction has
               loaded the api hook before the callback and in case of error we remove it */
            ppi->W32PF_flags |= W32PF_APIHOOKLOADED;

            /* Call ClientLoadLibrary in user32 */
            bResult = co_IntClientLoadLibrary(&strUahModule, &strUahInitFunc, Unload, TRUE);
            TRACE("co_IntClientLoadLibrary returned %d\n", bResult );
            if (!bResult)
            {
                /* Remove the flag we set before */
                ppi->W32PF_flags &= ~W32PF_APIHOOKLOADED;
            }
            return bResult;
        }
        else if(Unload && (ppi->W32PF_flags & W32PF_APIHOOKLOADED))
        {
            /* Call ClientLoadLibrary in user32 */
            bResult = co_IntClientLoadLibrary(NULL, NULL, Unload, TRUE);
            if (bResult)
            {
                ppi->W32PF_flags &= ~W32PF_APIHOOKLOADED;
            }
            return bResult;
        }

        return TRUE;
    }

    STUB;

    return FALSE;
}

/*
IntHookModuleUnloaded:
Sends a internal message to all threads of the requested desktop
and notifies them that a global hook was destroyed
and an injected module must be unloaded.
As a result, IntLoadHookModule will be called for all the threads that
will receive the special purpose internal message.
*/
BOOL
IntHookModuleUnloaded(PDESKTOP pdesk, int iHookID, HHOOK hHook)
{
    PTHREADINFO ptiCurrent;
    PLIST_ENTRY ListEntry;
    PPROCESSINFO ppiCsr;

    TRACE("IntHookModuleUnloaded: iHookID=%d\n", iHookID);

    ppiCsr = PsGetProcessWin32Process(gpepCSRSS);

    ListEntry = pdesk->PtiList.Flink;
    while(ListEntry != &pdesk->PtiList)
    {
        ptiCurrent = CONTAINING_RECORD(ListEntry, THREADINFO, PtiLink);

        /* FIXME: Do some more security checks here */

        /* FIXME: The first check is a reactos specific hack for system threads */
        if(!PsIsSystemProcess(ptiCurrent->ppi->peProcess) &&
           ptiCurrent->ppi != ppiCsr)
        {
            if(ptiCurrent->ppi->W32PF_flags & W32PF_APIHOOKLOADED)
            {
                TRACE("IntHookModuleUnloaded: sending message to PID %p, ppi=%p\n", PsGetProcessId(ptiCurrent->ppi->peProcess), ptiCurrent->ppi);
                co_MsqSendMessageAsync( ptiCurrent,
                                        0,
                                        iHookID,
                                        TRUE,
                                        (LPARAM)hHook,
                                        NULL,
                                        0,
                                        FALSE,
                                        MSQ_INJECTMODULE);
            }
        }
        ListEntry = ListEntry->Flink;
    }

    return TRUE;
}

BOOL
FASTCALL
UserLoadApiHook(VOID)
{
    return IntLoadHookModule(WH_APIHOOK, 0, FALSE);
}

BOOL
FASTCALL
UserRegisterUserApiHook(
    PUNICODE_STRING pstrDllName,
    PUNICODE_STRING pstrFuncName)
{
    PTHREADINFO pti, ptiCurrent;
    HWND *List;
    PWND DesktopWindow, pwndCurrent;
    ULONG i;
    PPROCESSINFO ppiCsr;

    pti = PsGetCurrentThreadWin32Thread();
    ppiCsr = PsGetProcessWin32Process(gpepCSRSS);

    /* Fail if the api hook is already registered */
    if(gpsi->dwSRVIFlags & SRVINFO_APIHOOK)
    {
        return FALSE;
    }

    TRACE("UserRegisterUserApiHook. Server PID: %p\n", PsGetProcessId(pti->ppi->peProcess));

    /* Register the api hook */
    gpsi->dwSRVIFlags |= SRVINFO_APIHOOK;

    strUahModule = *pstrDllName;
    strUahInitFunc = *pstrFuncName;
    ppiUahServer = pti->ppi;

    /* Broadcast an internal message to every top level window */
    DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
    List = IntWinListChildren(DesktopWindow);

    if (List != NULL)
    {
        for (i = 0; List[i]; i++)
        {
            pwndCurrent = UserGetWindowObject(List[i]);
            if(pwndCurrent == NULL)
            {
                continue;
            }
            ptiCurrent = pwndCurrent->head.pti;

           /* FIXME: The first check is a reactos specific hack for system threads */
            if(PsIsSystemProcess(ptiCurrent->ppi->peProcess) ||
                ptiCurrent->ppi == ppiCsr)
            {
                continue;
            }

            co_MsqSendMessageAsync( ptiCurrent,
                                    0,
                                    WH_APIHOOK,
                                    FALSE,   /* Load the module */
                                    0,
                                    NULL,
                                    0,
                                    FALSE,
                                    MSQ_INJECTMODULE);
        }
        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
    }

    return TRUE;
}

BOOL
FASTCALL
UserUnregisterUserApiHook(VOID)
{
    PTHREADINFO pti;

    pti = PsGetCurrentThreadWin32Thread();

    /* Fail if the api hook is not registered */
    if(!(gpsi->dwSRVIFlags & SRVINFO_APIHOOK))
    {
        return FALSE;
    }

    /* Only the process that registered the api hook can uregister it */
    if(ppiUahServer != PsGetCurrentProcessWin32Process())
    {
        return FALSE;
    }

    TRACE("UserUnregisterUserApiHook. Server PID: %p\n", PsGetProcessId(pti->ppi->peProcess));

    /* Unregister the api hook */
    gpsi->dwSRVIFlags &= ~SRVINFO_APIHOOK;
    ppiUahServer = NULL;
    ReleaseCapturedUnicodeString(&strUahModule, UserMode);
    ReleaseCapturedUnicodeString(&strUahInitFunc, UserMode);

    /* Notify all applications that the api hook module must be unloaded */
    return IntHookModuleUnloaded(pti->rpdesk, WH_APIHOOK, 0);
}

static
LRESULT
FASTCALL
co_IntCallLowLevelHook(PHOOK Hook,
                     INT Code,
                     WPARAM wParam,
                     LPARAM lParam)
{
    NTSTATUS Status;
    PTHREADINFO pti;
    PHOOKPACK pHP;
    INT Size = 0;
    UINT uTimeout = 300;
    BOOL Block = FALSE;
    ULONG_PTR uResult = 0;

    if (Hook->ptiHooked)
       pti = Hook->ptiHooked;
    else
       pti = Hook->head.pti;

    pHP = ExAllocatePoolWithTag(NonPagedPool, sizeof(HOOKPACK), TAG_HOOK);
    if (!pHP) return 0;

    pHP->pHk = Hook;
    pHP->lParam = lParam;
    pHP->pHookStructs = NULL;

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
          break;
    }

    if (Size)
    {
       pHP->pHookStructs = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_HOOK);
       if (pHP->pHookStructs) RtlCopyMemory(pHP->pHookStructs, (PVOID)lParam, Size);
    }

    /* FIXME: Should get timeout from
     * HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout */
    Status = co_MsqSendMessage( pti,
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
       ERR("Error Hook Call SendMsg. %d Status: 0x%x\n", Hook->HookId, Status);
       if (pHP->pHookStructs) ExFreePoolWithTag(pHP->pHookStructs, TAG_HOOK);
       ExFreePoolWithTag(pHP, TAG_HOOK);
    }
    return NT_SUCCESS(Status) ? uResult : 0;
}


//
// Dispatch MsgQueue Hook Call processor!
//
LRESULT
APIENTRY
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
       case WH_KEYBOARD_LL:
       case WH_MOUSE_LL:
       case WH_MOUSE:
          lParam = (LPARAM)pHP->pHookStructs;
       case WH_KEYBOARD:
          break;
    }
    /* The odds are high for this to be a Global call. */
    Result = co_IntCallHookProc( HookId,
                                 Code,
                                 wParam,
                                 lParam,
                                 phk->Proc,
                                 phk->ihmod,
                                 phk->offPfn,
                                 phk->Ansi,
                                &phk->ModuleName);

    /* The odds so high, no one is waiting for the results. */
    if (pHP->pHookStructs) ExFreePoolWithTag(pHP->pHookStructs, TAG_HOOK);
    ExFreePoolWithTag(pHP, TAG_HOOK);
    return Result;
}

static
LRESULT
APIENTRY
co_HOOK_CallHookNext( PHOOK Hook,
                      INT Code,
                      WPARAM wParam,
                      LPARAM lParam)
{
    TRACE("Calling Next HOOK %d\n", Hook->HookId);

    return co_IntCallHookProc( Hook->HookId,
                               Code,
                               wParam,
                               lParam,
                               Hook->Proc,
                               Hook->ihmod,
                               Hook->offPfn,
                               Hook->Ansi,
                              &Hook->ModuleName);
}

static
LRESULT
FASTCALL
co_IntCallDebugHook(PHOOK Hook,
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
            ERR("HOOK WH_DEBUG read from lParam ERROR!\n");
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

                case HCBT_CREATEWND: /* Handle ANSI? */
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
            ERR("HOOK WH_DEBUG read from Debug.lParam ERROR!\n");
            ExFreePoolWithTag(HooklParam, TAG_HOOK);
            return lResult;
        }
    }

    if (HooklParam) Debug.lParam = (LPARAM)HooklParam;
    lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Debug);
    if (HooklParam) ExFreePoolWithTag(HooklParam, TAG_HOOK);

    return lResult;
}

static
LRESULT
APIENTRY
co_UserCallNextHookEx(PHOOK Hook,
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
                ERR("HOOK WH_MOUSE read from lParam ERROR!\n");
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
                    ERR("HOOK WH_MOUSE_LL read from lParam ERROR!\n");
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
                    ERR("HOOK WH_KEYBORD_LL read from lParam ERROR!\n");
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
                    ERR("HOOK WH_XMESSAGEX read from lParam ERROR!\n");
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
                        ERR("HOOK WH_GETMESSAGE write to lParam ERROR!\n");
                    }
                }
            }
            break;
        }

        case WH_CBT:
            TRACE("HOOK WH_CBT!\n");
            switch (Code)
            {
                case HCBT_CREATEWND:
                {
                    LPCBT_CREATEWNDW pcbtcww = (LPCBT_CREATEWNDW)lParam;

                    TRACE("HOOK HCBT_CREATEWND\n");
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
                                _Analysis_assume_(pcbtcww->lpcs->lpszClass != NULL);
                                ProbeForRead(pcbtcww->lpcs->lpszClass,
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
                                _Analysis_assume_(pcbtcww->lpcs->lpszClass != NULL);
                                ProbeForRead(pcbtcww->lpcs->lpszClass,
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
                        ERR("HOOK HCBT_CREATEWND write ERROR!\n");
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

                    TRACE("HOOK HCBT_MOVESIZE\n");

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
                            ERR("HOOK HCBT_MOVESIZE read from lParam ERROR!\n");
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

                    TRACE("HOOK HCBT_ACTIVATE\n");
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
                            ERR("HOOK HCBT_ACTIVATE read from lParam ERROR!\n");
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
                    TRACE("HOOK HCBT_ %d\n",Code);
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
                    ERR("HOOK WH_JOURNAL read from lParam ERROR!\n");
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
                        ERR("HOOK WH_JOURNAL write to lParam ERROR!\n");
                    }
                }
            }
            break;
        }

        case WH_DEBUG:
            lResult = co_IntCallDebugHook(Hook, Code, wParam, lParam, Ansi);
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
            ERR("Unsupported HOOK Id -> %d\n",Hook->HookId);
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

    Hook = (PHOOK)UserGetObject(gHandleTable, hHook, TYPE_HOOK);
    if (!Hook)
    {
       EngSetLastError(ERROR_INVALID_HOOK_HANDLE);
       return NULL;
    }

    UserReferenceObject(Hook);

    return Hook;
}

static
HHOOK*
FASTCALL
IntGetGlobalHookHandles(PDESKTOP pdo, int HookId)
{
    PLIST_ENTRY pLastHead, pElem;
    unsigned i = 0;
    unsigned cHooks = 0;
    HHOOK *pList;
    PHOOK pHook;

    pLastHead = &pdo->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)];
    for (pElem = pLastHead->Flink; pElem != pLastHead; pElem = pElem->Flink)
      ++cHooks;

    pList = ExAllocatePoolWithTag(PagedPool, (cHooks + 1) * sizeof(HHOOK), TAG_HOOK);
    if (!pList)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    for (pElem = pLastHead->Flink; pElem != pLastHead; pElem = pElem->Flink)
    {
        pHook = CONTAINING_RECORD(pElem, HOOK, Chain);
        NT_ASSERT(i < cHooks);
        pList[i++] = pHook->head.h;
    }
    pList[i] = NULL;

    return pList;
}

/* Find the next hook in the chain  */
PHOOK
FASTCALL
IntGetNextHook(PHOOK Hook)
{
    int HookId = Hook->HookId;
    PLIST_ENTRY pLastHead, pElem;
    PTHREADINFO pti;

    if (Hook->ptiHooked)
    {
       pti = Hook->ptiHooked;
       pLastHead = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];
    }
    else
    {
       pti = PsGetCurrentThreadWin32Thread();
       pLastHead = &pti->rpdesk->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)];
    }

    pElem = Hook->Chain.Flink;
    if (pElem != pLastHead)
       return CONTAINING_RECORD(pElem, HOOK, Chain);
    return NULL;
}

/* Free a hook, removing it from its chain */
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
    UserDeleteObject(UserHMGetHandle(Hook), TYPE_HOOK);
}

/* Remove a hook, freeing it from the chain */
BOOLEAN
IntRemoveHook(PVOID Object)
{
    INT HookId;
    PTHREADINFO pti;
    PDESKTOP pdo;
    PHOOK Hook = Object;

    HookId = Hook->HookId;

    if (Hook->ptiHooked) // Local
    {
       pti = Hook->ptiHooked;

       IntFreeHook( Hook);

       if ( IsListEmpty(&pti->aphkStart[HOOKID_TO_INDEX(HookId)]) )
       {
          pti->fsHooks &= ~HOOKID_TO_FLAG(HookId);
          _SEH2_TRY
          {
             pti->pClientInfo->fsHooks = pti->fsHooks;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
              /* Do nothing */
              (void)0;
          }
          _SEH2_END;
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
       }
    }

    return TRUE;
}

/*
  Win32k Kernel Space Hook Caller.
 */
LRESULT
APIENTRY
co_HOOK_CallHooks( INT HookId,
                   INT Code,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PHOOK Hook, SaveHook;
    PTHREADINFO pti;
    PCLIENTINFO ClientInfo;
    PLIST_ENTRY pLastHead;
    PDESKTOP pdo;
    BOOL Local = FALSE, Global = FALSE;
    LRESULT Result = 0;
    USER_REFERENCE_ENTRY Ref;

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
          TRACE("No PDO %d\n", HookId);
          goto Exit;
       }
    }
    else
    {
       pdo = pti->rpdesk;
    }

    if ( pti->TIF_flags & (TIF_INCLEANUP|TIF_DISABLEHOOKS))
    {
       TRACE("Hook Thread dead %d\n", HookId);
       goto Exit;
    }

    if ( ISITHOOKED(HookId) )
    {
       TRACE("Local Hooker %d\n", HookId);
       Local = TRUE;
    }

    if ( pdo->pDeskInfo->fsHooks & HOOKID_TO_FLAG(HookId) )
    {
       TRACE("Global Hooker %d\n", HookId);
       Global = TRUE;
    }

    if ( !Local && !Global ) goto Exit; // No work!

    Hook = NULL;

    /* SetWindowHookEx sorts out the Thread issue by placing the Hook to
       the correct Thread if not NULL.
     */
    if ( Local )
    {
       pLastHead = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];
       if (IsListEmpty(pLastHead))
       {
          ERR("No Local Hook Found!\n");
          goto Exit;
       }

       Hook = CONTAINING_RECORD(pLastHead->Flink, HOOK, Chain);
       ObReferenceObject(pti->pEThread);
       IntReferenceThreadInfo(pti);
       UserRefObjectCo(Hook, &Ref);

       ClientInfo = pti->pClientInfo;
       SaveHook = pti->sphkCurrent;
       /* Note: Setting pti->sphkCurrent will also lock the next hook to this
        *       hook ID. So, the CallNextHookEx will only call to that hook ID
        *       chain anyway. For Thread Hooks....
        */

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
                                    Hook->ihmod,
                                    Hook->offPfn,
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
              /* Do nothing */
              (void)0;
          }
          _SEH2_END;
       }
       pti->sphkCurrent = SaveHook;
       Hook->phkNext = NULL;
       UserDerefObjectCo(Hook);
       IntDereferenceThreadInfo(pti);
       ObDereferenceObject(pti->pEThread);
    }

    if ( Global )
    {
       PTHREADINFO ptiHook;
       HHOOK *pHookHandles;
       unsigned i;

       /* Keep hooks in array because hooks can be destroyed in user world */
       pHookHandles = IntGetGlobalHookHandles(pdo, HookId);
       if(!pHookHandles)
          goto Exit;

      /* Performance goes down the drain. If more hooks are associated to this
       * hook ID, this will have to post to each of the thread message queues
       * or make a direct call.
       */
       for(i = 0; pHookHandles[i]; ++i)
       {
          Hook = (PHOOK)UserGetObject(gHandleTable, pHookHandles[i], TYPE_HOOK);
          if(!Hook)
          {
              ERR("Invalid hook!\n");
              continue;
          }

         /* Hook->Thread is null, we hax around this with Hook->head.pti. */
          ptiHook = Hook->head.pti;

          if ( (pti->TIF_flags & TIF_DISABLEHOOKS) || (ptiHook->TIF_flags & TIF_INCLEANUP))
          {
             TRACE("Next Hook %p, %p\n", ptiHook->rpdesk, pdo);
             continue;
          }
          UserRefObjectCo(Hook, &Ref);

          if (ptiHook != pti )
          {
                                                  // Block | TimeOut
             if ( HookId == WH_JOURNALPLAYBACK || //   1   |    0
                  HookId == WH_JOURNALRECORD   || //   1   |    0
                  HookId == WH_KEYBOARD        || //   1   |   200
                  HookId == WH_MOUSE           || //   1   |   200
                  HookId == WH_KEYBOARD_LL     || //   0   |   300
                  HookId == WH_MOUSE_LL )         //   0   |   300
             {
                TRACE("\nGlobal Hook posting to another Thread! %d\n",HookId );
                Result = co_IntCallLowLevelHook(Hook, Code, wParam, lParam);
             }
             else if (ptiHook->ppi == pti->ppi)
             {
                TRACE("\nGlobal Hook calling to another Thread! %d\n",HookId );
                ObReferenceObject(ptiHook->pEThread);
                IntReferenceThreadInfo(ptiHook);
                Result = co_IntCallHookProc( HookId,
                                             Code,
                                             wParam,
                                             lParam,
                                             Hook->Proc,
                                             Hook->ihmod,
                                             Hook->offPfn,
                                             Hook->Ansi,
                                            &Hook->ModuleName);
                IntDereferenceThreadInfo(ptiHook);
                ObDereferenceObject(ptiHook->pEThread);
             }
          }
          else
          { /* Make the direct call. */
             TRACE("Global going Local Hook calling to Thread! %d\n",HookId );
             ObReferenceObject(pti->pEThread);
             IntReferenceThreadInfo(pti);
             Result = co_IntCallHookProc( HookId,
                                          Code,
                                          wParam,
                                          lParam,
                                          Hook->Proc,
                                          Hook->ihmod,
                                          Hook->offPfn,
                                          Hook->Ansi,
                                         &Hook->ModuleName);
             IntDereferenceThreadInfo(pti);
             ObDereferenceObject(pti->pEThread);
          }
          UserDerefObjectCo(Hook);
       }
       ExFreePoolWithTag(pHookHandles, TAG_HOOK);
       TRACE("Ret: Global HookId %d Result 0x%x\n", HookId,Result);
    }
Exit:
    return Result;
}

BOOL
FASTCALL
IntUnhookWindowsHook(int HookId, HOOKPROC pfnFilterProc)
{
    PHOOK Hook;
    PLIST_ENTRY pLastHead, pElement;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (HookId < WH_MINHOOK || WH_MAXHOOK < HookId )
    {
       EngSetLastError(ERROR_INVALID_HOOK_FILTER);
       return FALSE;
    }

    if (pti->fsHooks)
    {
       pLastHead = &pti->aphkStart[HOOKID_TO_INDEX(HookId)];

       pElement = pLastHead->Flink;
       while (pElement != pLastHead)
       {
          Hook = CONTAINING_RECORD(pElement, HOOK, Chain);

          /* Get the next element now, we might free the hook in what follows */
          pElement = Hook->Chain.Flink;

          if (Hook->Proc == pfnFilterProc)
          {
             if (Hook->head.pti == pti)
             {
                IntRemoveHook(Hook);
                return TRUE;
             }
             else
             {
                EngSetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
             }
          }
       }
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

    TRACE("Enter NtUserCallNextHookEx\n");
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
       lResult = co_UserCallNextHookEx( NextObj, Code, wParam, lParam, NextObj->Ansi);
    }
    RETURN( lResult);

CLEANUP:
    TRACE("Leave NtUserCallNextHookEx, ret=%i\n",_ret_);
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
    PHOOK Hook = NULL;
    UNICODE_STRING ModuleName;
    NTSTATUS Status;
    HHOOK Handle;
    PTHREADINFO pti, ptiHook = NULL;
    DECLARE_RETURN(HHOOK);

    TRACE("Enter NtUserSetWindowsHookEx\n");
    UserEnterExclusive();

    pti = PsGetCurrentThreadWin32Thread();

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
           TRACE("Local hook installing Global HookId: %d\n",HookId);
           /* these can only be global */
           EngSetLastError(ERROR_GLOBAL_ONLY_HOOK);
           RETURN( NULL);
       }

       if ( !(ptiHook = IntTID2PTI( (HANDLE)ThreadId )))
       {
          ERR("Invalid thread id 0x%x\n", ThreadId);
          EngSetLastError(ERROR_INVALID_PARAMETER);
          RETURN( NULL);
       }

       if ( ptiHook->rpdesk != pti->rpdesk) // gptiCurrent->rpdesk)
       {
          ERR("Local hook wrong desktop HookId: %d\n",HookId);
          EngSetLastError(ERROR_ACCESS_DENIED);
          RETURN( NULL);
       }

       if (ptiHook->ppi != pti->ppi)
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
             ERR("Local hook needs hMod HookId: %d\n",HookId);
             EngSetLastError(ERROR_HOOK_NEEDS_HMOD);
             RETURN( NULL);
          }

          if ( (ptiHook->TIF_flags & (TIF_CSRSSTHREAD|TIF_SYSTEMTHREAD)) &&
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
    else  /* System-global hook */
    {
       ptiHook = pti; // gptiCurrent;
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
          ERR("Global hook needs hMod HookId: %d\n",HookId);
          EngSetLastError(ERROR_HOOK_NEEDS_HMOD);
          RETURN( NULL);
       }
    }

    Status = IntValidateWindowStationHandle( PsGetCurrentProcess()->Win32WindowStation,
                                             KernelMode,
                                             0,
                                            &WinStaObj,
                                             0);

    if (!NT_SUCCESS(Status))
    {
       SetLastNtError(Status);
       RETURN( NULL);
    }
    ObDereferenceObject(WinStaObj);

    Hook = UserCreateObject(gHandleTable, NULL, ptiHook, (PHANDLE)&Handle, TYPE_HOOK, sizeof(HOOK));

    if (!Hook)
    {
       RETURN( NULL);
    }

    Hook->ihmod   = (INT)Mod; // Module Index from atom table, Do this for now.
    Hook->HookId  = HookId;
    Hook->rpdesk  = ptiHook->rpdesk;
    Hook->phkNext = NULL; /* Dont use as a chain! Use link lists for chaining. */
    Hook->Proc    = HookProc;
    Hook->Ansi    = Ansi;

    TRACE("Set Hook Desk %p DeskInfo %p Handle Desk %p\n", pti->rpdesk, pti->pDeskInfo, Hook->head.rpdesk);

    if (ThreadId)  /* Thread-local hook */
    {
       InsertHeadList(&ptiHook->aphkStart[HOOKID_TO_INDEX(HookId)], &Hook->Chain);
       ptiHook->sphkCurrent = NULL;
       Hook->ptiHooked = ptiHook;
       ptiHook->fsHooks |= HOOKID_TO_FLAG(HookId);

       if (ptiHook->pClientInfo)
       {
          if ( ptiHook->ppi == pti->ppi) /* gptiCurrent->ppi) */
          {
             _SEH2_TRY
             {
                ptiHook->pClientInfo->fsHooks = ptiHook->fsHooks;
                ptiHook->pClientInfo->phkCurrent = NULL;
             }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
             {
                ERR("Problem writing to Local ClientInfo!\n");
             }
             _SEH2_END;
          }
          else
          {
             KeAttachProcess(&ptiHook->ppi->peProcess->Pcb);
             _SEH2_TRY
             {
                ptiHook->pClientInfo->fsHooks = ptiHook->fsHooks;
                ptiHook->pClientInfo->phkCurrent = NULL;
             }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
             {
                ERR("Problem writing to Remote ClientInfo!\n");
             }
             _SEH2_END;
             KeDetachProcess();
          }
       }
    }
    else
    {
       InsertHeadList(&ptiHook->rpdesk->pDeskInfo->aphkStart[HOOKID_TO_INDEX(HookId)], &Hook->Chain);
       Hook->ptiHooked = NULL;
       //gptiCurrent->pDeskInfo->fsHooks |= HOOKID_TO_FLAG(HookId);
       ptiHook->rpdesk->pDeskInfo->fsHooks |= HOOKID_TO_FLAG(HookId);
       ptiHook->sphkCurrent = NULL;
       ptiHook->pClientInfo->phkCurrent = NULL;
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
       //// FIXME: Need to load from user32 to verify hMod before calling hook with hMod set!!!!
       //// Mod + offPfn == new HookProc Justin Case module is from another process.
       FIXME("NtUserSetWindowsHookEx Setting process hMod instance addressing.\n");
       /* Make proc relative to the module base */
       Hook->offPfn = (ULONG_PTR)((char *)HookProc - (char *)Mod);
    }
    else
       Hook->offPfn = 0;

    TRACE("Installing: HookId %d Global %s\n", HookId, !ThreadId ? "TRUE" : "FALSE");
    RETURN( Handle);

CLEANUP:
    if (Hook)
        UserDereferenceObject(Hook);
    TRACE("Leave NtUserSetWindowsHookEx, ret=%p\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL
APIENTRY
NtUserUnhookWindowsHookEx(HHOOK Hook)
{
    PHOOK HookObj;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserUnhookWindowsHookEx\n");
    UserEnterExclusive();

    if (!(HookObj = IntGetHookObject(Hook)))
    {
        ERR("Invalid handle passed to NtUserUnhookWindowsHookEx\n");
        /* SetLastNtError(Status); */
        RETURN( FALSE);
    }

    ASSERT(Hook == UserHMGetHandle(HookObj));

    IntRemoveHook(HookObj);

    UserDereferenceObject(HookObj);

    RETURN( TRUE);

CLEANUP:
    TRACE("Leave NtUserUnhookWindowsHookEx, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL
APIENTRY
NtUserRegisterUserApiHook(
    PUNICODE_STRING m_dllname1,
    PUNICODE_STRING m_funname1,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    BOOL ret;
    UNICODE_STRING strDllNameSafe;
    UNICODE_STRING strFuncNameSafe;
    NTSTATUS Status;

    /* Probe and capture parameters */
    Status = ProbeAndCaptureUnicodeString(&strDllNameSafe, UserMode, m_dllname1);
    if(!NT_SUCCESS(Status))
    {
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = ProbeAndCaptureUnicodeString(&strFuncNameSafe, UserMode, m_funname1);
    if(!NT_SUCCESS(Status))
    {
        ReleaseCapturedUnicodeString(&strDllNameSafe, UserMode);
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    UserEnterExclusive();

    /* Call internal function */
    ret = UserRegisterUserApiHook(&strDllNameSafe, &strFuncNameSafe);

    UserLeave();

    /* Cleanup only in case of failure */
    if(ret == FALSE)
    {
        ReleaseCapturedUnicodeString(&strDllNameSafe, UserMode);
        ReleaseCapturedUnicodeString(&strFuncNameSafe, UserMode);
    }

    return ret;
}

BOOL
APIENTRY
NtUserUnregisterUserApiHook(VOID)
{
    BOOL ret;

    UserEnterExclusive();
    ret = UserUnregisterUserApiHook();
    UserLeave();

    return ret;
}

/* EOF */
