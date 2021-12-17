/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          NtUserCallXxx call stubs
 * FILE:             win32ss/user/ntuser/simplecall.c
 * PROGRAMERS:       Ge van Geldorp (ge@gse.nl)
 *                   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserMisc);

/* Registered logon process ID */
HANDLE gpidLogon = 0;

BOOL FASTCALL
co_IntRegisterLogonProcess(HANDLE ProcessId, BOOL Register)
{
    NTSTATUS Status;
    PEPROCESS Process;

    Status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    ProcessId = Process->UniqueProcessId;

    ObDereferenceObject(Process);

    if (Register)
    {
        /* Register the logon process */
        if (gpidLogon != 0)
            return FALSE;

        gpidLogon = ProcessId;
    }
    else
    {
        /* Deregister the logon process */
        if (gpidLogon != ProcessId)
            return FALSE;

        gpidLogon = 0;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
DWORD_PTR
APIENTRY
NtUserCallNoParam(DWORD Routine)
{
    DWORD_PTR Result = 0;

    TRACE("Enter NtUserCallNoParam\n");
    UserEnterExclusive();

    switch (Routine)
    {
        case NOPARAM_ROUTINE_CREATEMENU:
            Result = (DWORD_PTR)UserCreateMenu(GetW32ThreadInfo()->rpdesk, FALSE);
            break;

        case NOPARAM_ROUTINE_CREATEMENUPOPUP:
            Result = (DWORD_PTR)UserCreateMenu(GetW32ThreadInfo()->rpdesk, TRUE);
            break;

        case NOPARAM_ROUTINE_DESTROY_CARET:
            Result = (DWORD_PTR)co_IntDestroyCaret(PsGetCurrentThread()->Tcb.Win32Thread);
            break;

        case NOPARAM_ROUTINE_INIT_MESSAGE_PUMP:
            Result = (DWORD_PTR)IntInitMessagePumpHook();
            break;

        case NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP:
            Result = (DWORD_PTR)IntUninitMessagePumpHook();
            break;

        case NOPARAM_ROUTINE_MSQCLEARWAKEMASK:
            Result = (DWORD_PTR)IntMsqClearWakeMask();
            break;

        case NOPARAM_ROUTINE_GETMSESSAGEPOS:
        {
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            Result = (DWORD_PTR)MAKELONG(pti->ptLast.x, pti->ptLast.y);
            break;
        }

        case NOPARAM_ROUTINE_RELEASECAPTURE:
            Result = (DWORD_PTR)IntReleaseCapture();
            break;

        case NOPARAM_ROUTINE_LOADUSERAPIHOOK:
            Result = UserLoadApiHook();
            break;

        case NOPARAM_ROUTINE_ZAPACTIVEANDFOUS:
        {
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            TRACE("Zapping the Active and Focus window out of the Queue!\n");
            pti->MessageQueue->spwndFocus = NULL;
            pti->MessageQueue->spwndActive = NULL;
            Result = 0;
            break;
        }

        /* this is a ReactOS only case and is needed for gui-on-demand */
        case NOPARAM_ROUTINE_ISCONSOLEMODE:
            Result = (ScreenDeviceContext == NULL);
            break;

        case NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING:
            gpsi->dwSRVIFlags |= SRVINFO_IMM32; // Always set.
            Result = TRUE; // Always return TRUE.
            break;

        default:
            ERR("Calling invalid routine number 0x%x in NtUserCallNoParam\n", Routine);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            break;
    }

    TRACE("Leave NtUserCallNoParam, ret=%p\n",(PVOID)Result);
    UserLeave();

    return Result;
}


/*
 * @implemented
 */
DWORD_PTR
APIENTRY
NtUserCallOneParam(
    DWORD_PTR Param,
    DWORD Routine)
{
    DWORD_PTR Result;

    TRACE("Enter NtUserCallOneParam\n");

    UserEnterExclusive();

    switch (Routine)
    {
        case ONEPARAM_ROUTINE_POSTQUITMESSAGE:
        {
            PTHREADINFO pti;
            pti = PsGetCurrentThreadWin32Thread();
            MsqPostQuitMessage(pti, Param);
            Result = TRUE;
            break;
        }

        case ONEPARAM_ROUTINE_BEGINDEFERWNDPOS:
        {
            PSMWP psmwp;
            HDWP hDwp = NULL;
            INT count = (INT)Param;

            if (count < 0)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                Result = 0;
                break;
            }

            /* Windows allows zero count, in which case it allocates context for 8 moves */
            if (count == 0) count = 8;

            psmwp = (PSMWP)UserCreateObject(gHandleTable,
                                            NULL,
                                            NULL,
                                            (PHANDLE)&hDwp,
                                            TYPE_SETWINDOWPOS,
                                            sizeof(SMWP));
            if (!psmwp)
            {
                Result = 0;
                break;
            }

            psmwp->acvr = ExAllocatePoolWithTag(PagedPool, count * sizeof(CVR), USERTAG_SWP);
            if (!psmwp->acvr)
            {
                UserDeleteObject(hDwp, TYPE_SETWINDOWPOS);
                Result = 0;
                break;
            }

            RtlZeroMemory(psmwp->acvr, count * sizeof(CVR));
            psmwp->bHandle = TRUE;
            psmwp->ccvr = 0;          // actualCount
            psmwp->ccvrAlloc = count; // suggestedCount
            Result = (DWORD_PTR)hDwp;
            break;
        }

        case ONEPARAM_ROUTINE_SHOWCURSOR:
            Result = (DWORD_PTR)UserShowCursor((BOOL)Param);
            break;

        case ONEPARAM_ROUTINE_GETDESKTOPMAPPING:
        {
            PTHREADINFO ti;
            ti = GetW32ThreadInfo();
            if (ti != NULL)
            {
                /* Try convert the pointer to a user mode pointer if the desktop is
                   mapped into the process */
                Result = (DWORD_PTR)DesktopHeapAddressToUser((PVOID)Param);
            }
            else
            {
                Result = 0;
            }
            break;
        }

        case ONEPARAM_ROUTINE_WINDOWFROMDC:
            Result = (DWORD_PTR)IntWindowFromDC((HDC)Param);
            break;

        case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
        {
            Result = gspv.bMouseBtnSwap;
            gspv.bMouseBtnSwap = Param ? TRUE : FALSE;
            gpsi->aiSysMet[SM_SWAPBUTTON] = gspv.bMouseBtnSwap;
            break;
        }

        case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
            Result = (DWORD_PTR)IntSetCaretBlinkTime((UINT)Param);
            break;

        case ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO:
            Result = (DWORD_PTR)MsqSetMessageExtraInfo((LPARAM)Param);
            break;

        case ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT:
        {
            if (!(Result = (DWORD_PTR)IntCreateCurIconHandle((DWORD)Param)))
            {
                EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                Result = 0;
            }
            break;
        }

        case ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING:
        {
            BOOL Enable;
            PPROCESSINFO Process = PsGetCurrentProcessWin32Process();

            if (Process != NULL)
            {
                Enable = (BOOL)(Param != 0);

                if (Enable)
                {
                    Process->W32PF_flags &= ~W32PF_NOWINDOWGHOSTING;
                }
                else
                {
                    Process->W32PF_flags |= W32PF_NOWINDOWGHOSTING;
                }

                Result = TRUE;
                break;
            }

            Result = FALSE;
            break;
        }

        case ONEPARAM_ROUTINE_GETINPUTEVENT:
            Result = (DWORD_PTR)IntMsqSetWakeMask(Param);
            break;

        case ONEPARAM_ROUTINE_GETKEYBOARDTYPE:
            Result = UserGetKeyboardType(Param);
            break;

        case ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT:
            Result = (DWORD_PTR)UserGetKeyboardLayout(Param);
            break;

        case ONEPARAM_ROUTINE_RELEASEDC:
            Result = UserReleaseDC(NULL, (HDC) Param, FALSE);
            break;

        case ONEPARAM_ROUTINE_REALIZEPALETTE:
            Result = UserRealizePalette((HDC) Param);
            break;

        case ONEPARAM_ROUTINE_GETQUEUESTATUS:
        {
            Result = IntGetQueueStatus((DWORD)Param);
            break;
        }

        case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
            /* FIXME: Should use UserEnterShared */
            Result = UserEnumClipboardFormats(Param);
            break;

        case ONEPARAM_ROUTINE_GETCURSORPOS:
        {
            PPOINTL pptl;
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            Result = TRUE;
            if (pti->rpdesk != IntGetActiveDesktop())
            {
                Result = FALSE;
                break;
            }
            _SEH2_TRY
            {
                ProbeForWrite((POINT*)Param,sizeof(POINT),1);
               pptl = (PPOINTL)Param;
               *pptl = gpsi->ptCursor;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Result = FALSE;
            }
            _SEH2_END;
            break;
        }

        case ONEPARAM_ROUTINE_SETPROCDEFLAYOUT:
        {
            PPROCESSINFO ppi;
            if (Param & LAYOUT_ORIENTATIONMASK || Param == LAYOUT_LTR)
            {
                ppi = PsGetCurrentProcessWin32Process();
                ppi->dwLayout = Param;
                Result = TRUE;
                break;
            }
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Result = FALSE;
            break;
        }

        case ONEPARAM_ROUTINE_GETPROCDEFLAYOUT:
        {
            PPROCESSINFO ppi;
            PDWORD pdwLayout;
            Result = TRUE;

            if (PsGetCurrentProcess() == gpepCSRSS)
            {
                EngSetLastError(ERROR_INVALID_ACCESS);
                Result = FALSE;
                break;
            }

            ppi = PsGetCurrentProcessWin32Process();
            _SEH2_TRY
            {
               pdwLayout = (PDWORD)Param;
               *pdwLayout = ppi->dwLayout;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastNtError(_SEH2_GetExceptionCode());
                Result = FALSE;
            }
            _SEH2_END;
            break;
        }

        case ONEPARAM_ROUTINE_REPLYMESSAGE:
            Result = co_MsqReplyMessage((LRESULT)Param);
            break;

        case ONEPARAM_ROUTINE_MESSAGEBEEP:
            /* TODO: Implement sound sentry */
            Result = UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, Param);
           break;

        case ONEPARAM_ROUTINE_CREATESYSTEMTHREADS:
            Result = UserSystemThreadProc(Param);
            break;

        case ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW:
            Result = (DWORD_PTR)IntLockSetForegroundWindow(Param);
            break;

        case ONEPARAM_ROUTINE_ALLOWSETFOREGND:
            Result = (DWORD_PTR)IntAllowSetForegroundWindow(Param);
            break;

        default:
            ERR("Calling invalid routine number 0x%x in NtUserCallOneParam(), Param=0x%x\n",
                Routine, Param);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Result = 0;
            break;
    }

   TRACE("Leave NtUserCallOneParam, ret=%p\n", (PVOID)Result);
   UserLeave();

   return Result;
}


/*
 * @implemented
 */
DWORD_PTR
APIENTRY
NtUserCallTwoParam(
    DWORD_PTR Param1,
    DWORD_PTR Param2,
    DWORD Routine)
{
    PWND Window;
    DWORD_PTR Ret;
    TRACE("Enter NtUserCallTwoParam\n");
    UserEnterExclusive();

    switch (Routine)
    {
        case TWOPARAM_ROUTINE_REDRAWTITLE:
        {
            Window = UserGetWindowObject((HWND)Param1);
            Ret = (DWORD_PTR)UserPaintCaption(Window, (INT)Param2);
            break;
        }

        case TWOPARAM_ROUTINE_SETMENUBARHEIGHT:
        {
            PMENU MenuObject = IntGetMenuObject((HMENU)Param1);
            if (!MenuObject)
            {
                Ret = 0;
                break;
            }

            if (Param2 > 0)
            {
                Ret = (MenuObject->cyMenu == (int)Param2);
                MenuObject->cyMenu = (int)Param2;
            }
            else
                Ret = (DWORD_PTR)MenuObject->cyMenu;
            IntReleaseMenuObject(MenuObject);
            break;
        }

        case TWOPARAM_ROUTINE_SETGUITHRDHANDLE:
        {
            PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
            ASSERT(pti->MessageQueue);
            Ret = (DWORD_PTR)MsqSetStateWindow(pti, (ULONG)Param1, (HWND)Param2);
            break;
        }

        case TWOPARAM_ROUTINE_ENABLEWINDOW:
            Ret = IntEnableWindow((HWND)Param1, (BOOL)Param2);
            break;

        case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
        {
            Window = UserGetWindowObject((HWND)Param1);
            if (!Window)
            {
                Ret = 0;
                break;
            }

            Ret = (DWORD_PTR)IntShowOwnedPopups(Window, (BOOL)Param2);
            break;
        }

        case TWOPARAM_ROUTINE_ROS_UPDATEUISTATE:
        {
            WPARAM wParam;
            Window = UserGetWindowObject((HWND)Param1);
            if (!Window)
            {
                Ret = 0;
                break;
            }

            /* Unpack wParam */
            wParam = MAKEWPARAM((Param2 >> 3) & 0x3,
                                Param2 & (UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE));

            Ret = UserUpdateUiState(Window, wParam);
            break;
        }

        case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
        {
            HWND hwnd = (HWND)Param1;
            BOOL fAltTab = (BOOL)Param2;
            Ret = 0;
            Window = UserGetWindowObject(hwnd);
            if (!Window)
                break;

            if (gpqForeground && !fAltTab)
            {
                PWND pwndActive = gpqForeground->spwndActive;
                if (pwndActive && !(pwndActive->ExStyle & WS_EX_TOPMOST))
                {
                    co_WinPosSetWindowPos(pwndActive, HWND_BOTTOM, 0, 0, 0, 0,
                                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                                          SWP_ASYNCWINDOWPOS);
                }

                UserSetActiveWindow(Window);
                break;
            }

            co_IntSetForegroundWindowMouse(Window);

            if (fAltTab && (Window->style & WS_MINIMIZE))
            {
                MSG msg = { Window->head.h, WM_SYSCOMMAND, SC_RESTORE, 0 };
                MsqPostMessage(Window->head.pti, &msg, FALSE, QS_POSTMESSAGE, 0, 0);
            }
            break;
        }

        case TWOPARAM_ROUTINE_SETCARETPOS:
            Ret = (DWORD_PTR)co_IntSetCaretPos((int)Param1, (int)Param2);
            break;

        case TWOPARAM_ROUTINE_REGISTERLOGONPROCESS:
            Ret = (DWORD_PTR)co_IntRegisterLogonProcess((HANDLE)Param1, (BOOL)Param2);
            break;

        case TWOPARAM_ROUTINE_SETCURSORPOS:
            Ret = (DWORD_PTR)UserSetCursorPos((int)Param1, (int)Param2, 0, 0, FALSE);
            break;

        case TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK:
            Ret = IntUnhookWindowsHook((int)Param1, (HOOKPROC)Param2);
            break;

        default:
            ERR("Calling invalid routine number 0x%x in NtUserCallTwoParam(), Param1=0x%x Parm2=0x%x\n",
                Routine, Param1, Param2);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = 0;
    }


    TRACE("Leave NtUserCallTwoParam, ret=%p\n", (PVOID)Ret);
    UserLeave();

    return Ret;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserCallHwndLock(
    HWND hWnd,
    DWORD Routine)
{
    BOOL Ret = FALSE;
    PWND Window;
    USER_REFERENCE_ENTRY Ref;

    TRACE("Enter NtUserCallHwndLock\n");
    UserEnterExclusive();

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        Ret = FALSE;
        goto Exit;
    }

    UserRefObjectCo(Window, &Ref);

    /* FIXME: Routine can be 0x53 - 0x5E */
    switch (Routine)
    {
        case HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS:
            co_WinPosArrangeIconicWindows(Window);
            break;

        case HWNDLOCK_ROUTINE_DRAWMENUBAR:
        {
            TRACE("HWNDLOCK_ROUTINE_DRAWMENUBAR\n");
            Ret = TRUE;
            if ((Window->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
                co_WinPosSetWindowPos(Window,
                                      HWND_DESKTOP,
                                      0, 0, 0, 0,
                                      SWP_NOSIZE |
                                      SWP_NOMOVE |
                                      SWP_NOZORDER |
                                      SWP_NOACTIVATE |
                                      SWP_FRAMECHANGED);
            break;
        }

        case HWNDLOCK_ROUTINE_REDRAWFRAME:
            co_WinPosSetWindowPos(Window,
                                  HWND_DESKTOP,
                                  0, 0, 0, 0,
                                  SWP_NOSIZE |
                                  SWP_NOMOVE |
                                  SWP_NOZORDER |
                                  SWP_NOACTIVATE |
                                  SWP_FRAMECHANGED);
            Ret = TRUE;
            break;

        case HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK:
            co_WinPosSetWindowPos(Window,
                                  HWND_DESKTOP,
                                  0, 0, 0, 0,
                                  SWP_NOSIZE |
                                  SWP_NOMOVE |
                                  SWP_NOZORDER |
                                  SWP_NOACTIVATE |
                                  SWP_FRAMECHANGED);
            if (!Window->spwndOwner && !IntGetParent(Window))
            {
                co_IntShellHookNotify(HSHELL_REDRAW, (WPARAM)hWnd, FALSE); // FIXME Flashing?
            }
            Ret = TRUE;
            break;

        case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW:
            TRACE("co_IntSetForegroundWindow 1 0x%p\n", hWnd);
            Ret = co_IntSetForegroundWindow(Window);
            TRACE("co_IntSetForegroundWindow 2 0x%p\n", hWnd);
            break;

        case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOWMOUSE:
            TRACE("co_IntSetForegroundWindow M 1 0x%p\n", hWnd);
            Ret = co_IntSetForegroundWindowMouse(Window);
            TRACE("co_IntSetForegroundWindow M 2 0x%p\n", hWnd);
            break;

        case HWNDLOCK_ROUTINE_UPDATEWINDOW:
            co_IntUpdateWindows(Window, RDW_ALLCHILDREN, FALSE);
            Ret = TRUE;
            break;
    }

    UserDerefObjectCo(Window);

Exit:
    TRACE("Leave NtUserCallHwndLock, ret=%u\n", Ret);
    UserLeave();

    return Ret;
}

/*
 * @unimplemented
 */
HWND
APIENTRY
NtUserCallHwndOpt(
    HWND hWnd,
    DWORD Routine)
{
    switch (Routine)
    {
        case HWNDOPT_ROUTINE_SETPROGMANWINDOW:
            GetW32ThreadInfo()->pDeskInfo->hProgmanWindow = hWnd;
            break;

        case HWNDOPT_ROUTINE_SETTASKMANWINDOW:
            GetW32ThreadInfo()->pDeskInfo->hTaskManWindow = hWnd;
            break;
    }

    return hWnd;
}

DWORD
APIENTRY
NtUserCallHwnd(
    HWND hWnd,
    DWORD Routine)
{
    switch (Routine)
    {
        case HWND_ROUTINE_GETWNDCONTEXTHLPID:
        {
            PWND Window;
            DWORD HelpId;

            UserEnterShared();

            if (!(Window = UserGetWindowObject(hWnd)))
            {
                UserLeave();
                return 0;
            }

            HelpId = (DWORD)(DWORD_PTR)UserGetProp(Window, gpsi->atomContextHelpIdProp, TRUE);

            UserLeave();
            return HelpId;
        }

        case HWND_ROUTINE_REGISTERSHELLHOOKWINDOW:
            if (IntIsWindow(hWnd))
                return IntRegisterShellHookWindow(hWnd);
            return FALSE;
            break;

        case HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW:
            if (IntIsWindow(hWnd))
                return IntDeRegisterShellHookWindow(hWnd);
            return FALSE;

        case HWND_ROUTINE_SETMSGBOX:
        {
            PWND Window;
            UserEnterExclusive();
            if ((Window = UserGetWindowObject(hWnd)))
            {
                Window->state |= WNDS_MSGBOX;
            }
            UserLeave();
            return FALSE;
        }
    }

    STUB;

    return 0;
}

DWORD
APIENTRY
NtUserCallHwndParam(
    HWND hWnd,
    DWORD_PTR Param,
    DWORD Routine)
{

    switch (Routine)
    {
        case HWNDPARAM_ROUTINE_KILLSYSTEMTIMER:
        {
            DWORD ret;

            UserEnterExclusive();
            ret = IntKillTimer(UserGetWindowObject(hWnd), (UINT_PTR)Param, TRUE);
            UserLeave();
            return ret;
        }

        case HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID:
        {
            PWND Window;

            UserEnterExclusive();
            if (!(Window = UserGetWindowObject(hWnd)))
            {
                UserLeave();
                return FALSE;
            }

            if (Param)
                UserSetProp(Window, gpsi->atomContextHelpIdProp, (HANDLE)Param, TRUE);
            else
                UserRemoveProp(Window, gpsi->atomContextHelpIdProp, TRUE);

            UserLeave();
            return TRUE;
        }

        case HWNDPARAM_ROUTINE_SETDIALOGPOINTER:
        {
            PWND pWnd;
            USER_REFERENCE_ENTRY Ref;

            UserEnterExclusive();

            if (!(pWnd = UserGetWindowObject(hWnd)))
            {
                UserLeave();
                return 0;
            }
            UserRefObjectCo(pWnd, &Ref);

            if (pWnd->head.pti->ppi == PsGetCurrentProcessWin32Process() &&
                pWnd->cbwndExtra >= DLGWINDOWEXTRA &&
                !(pWnd->state & WNDS_SERVERSIDEWINDOWPROC))
            {
                pWnd->DialogPointer = (PVOID)Param;
                if (Param)
                {
                    if (!pWnd->fnid) pWnd->fnid = FNID_DIALOG;
                    pWnd->state |= WNDS_DIALOGWINDOW;
                }
                else
                {
                    pWnd->fnid |= FNID_DESTROY;
                    pWnd->state &= ~WNDS_DIALOGWINDOW;
                }
            }

            UserDerefObjectCo(pWnd);
            UserLeave();
            return 0;
        }

        case HWNDPARAM_ROUTINE_ROS_NOTIFYWINEVENT:
        {
            PWND pWnd;
            PNOTIFYEVENT pne;
            UserEnterExclusive();
            pne = (PNOTIFYEVENT)Param;
            if (hWnd)
                pWnd = UserGetWindowObject(hWnd);
            else
                pWnd = NULL;
            IntNotifyWinEvent(pne->event, pWnd, pne->idObject, pne->idChild, pne->flags);
            UserLeave();
            return 0;
        }

        case HWNDPARAM_ROUTINE_CLEARWINDOWSTATE:
        {
            PWND pWnd;
            UserEnterExclusive();
            pWnd = UserGetWindowObject(hWnd);
            if (pWnd) IntClearWindowState(pWnd, (UINT)Param);
            UserLeave();
            return 0;
        }

        case HWNDPARAM_ROUTINE_SETWINDOWSTATE:
        {
            PWND pWnd;
            UserEnterExclusive();
            pWnd = UserGetWindowObject(hWnd);
            if (pWnd) IntSetWindowState(pWnd, (UINT)Param);
            UserLeave();
            return 0;
        }
    }

    STUB;

    return 0;
}

DWORD
APIENTRY
NtUserCallHwndParamLock(
    HWND hWnd,
    DWORD_PTR Param,
    DWORD Routine)
{
    DWORD Ret = FALSE;
    PWND Window;
    USER_REFERENCE_ENTRY Ref;

    TRACE("Enter NtUserCallHwndParamLock\n");
    UserEnterExclusive();

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        Ret = FALSE;
        goto Exit;
    }

    UserRefObjectCo(Window, &Ref);

    switch (Routine)
    {
        case TWOPARAM_ROUTINE_VALIDATERGN:
        {
            PREGION Rgn = REGION_LockRgn((HRGN)Param);
            Ret = (DWORD)co_UserRedrawWindow(Window, NULL, Rgn, RDW_VALIDATE);
            if (Rgn) REGION_UnlockRgn(Rgn);
            break;
        }
    }

    UserDerefObjectCo(Window);

Exit:

    TRACE("Leave NtUserCallHwndParamLock, ret=%lu\n", Ret);
    UserLeave();

    return Ret;
}

/* EOF */
