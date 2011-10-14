/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Mouse functions
 * FILE:             subsystems/win32/win32k/ntuser/input.c
 * PROGRAMERS:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Rafal Harabien (rafalh@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserInput);

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    IntMouseInput(&mi,FALSE); \
  ClearMouseInput(mi);

VOID NTAPI
UserProcessMouseInput(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
    PMOUSE_INPUT_DATA mid;
    MOUSEINPUT mi;
    ULONG i;

    ClearMouseInput(mi);
    mi.time = 0;
    mi.dwExtraInfo = 0;
    for(i = 0; i < InputCount; i++)
    {
        mid = (Data + i);
        mi.dx += mid->LastX;
        mi.dy += mid->LastY;

        /* Check if the mouse move is absolute */
        if (mid->Flags == MOUSE_MOVE_ABSOLUTE)
        {
            /* Set flag to convert to screen location */
            mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
        }

        if(mid->ButtonFlags)
        {
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_WHEEL)
            {
                mi.mouseData = mid->ButtonData;
                mi.dwFlags |= MOUSEEVENTF_WHEEL;
                SendMouseEvent(mi);
            }
        }
    }

    SendMouseEvent(mi);
}

BOOL FASTCALL
IntMouseInput(MOUSEINPUT *mi, BOOL Injected)
{
    const UINT SwapBtnMsg[2][2] =
    {
        {WM_LBUTTONDOWN, WM_RBUTTONDOWN},
        {WM_LBUTTONUP, WM_RBUTTONUP}
    };
    const WPARAM SwapBtn[2] =
    {
        MK_LBUTTON, MK_RBUTTON
    };
    POINT MousePos;
    PSYSTEM_CURSORINFO CurInfo;
    BOOL SwapButtons;
    MSG Msg;

    ASSERT(mi);

    CurInfo = IntGetSysCursorInfo();

    if(!mi->time)
    {
        LARGE_INTEGER LargeTickCount;
        KeQueryTickCount(&LargeTickCount);
        mi->time = MsqCalculateMessageTime(&LargeTickCount);
    }

    SwapButtons = gspv.bMouseBtnSwap;

    MousePos = gpsi->ptCursor;

    if(mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        if(mi->dwFlags & MOUSEEVENTF_ABSOLUTE)
        {
            MousePos.x = mi->dx * UserGetSystemMetrics(SM_CXVIRTUALSCREEN) >> 16;
            MousePos.y = mi->dy * UserGetSystemMetrics(SM_CYVIRTUALSCREEN) >> 16;
        }
        else
        {
            MousePos.x += mi->dx;
            MousePos.y += mi->dy;
        }
    }

    /*
     * Insert the messages into the system queue
     */
    Msg.wParam = 0;
    Msg.lParam = MAKELPARAM(MousePos.x, MousePos.y);
    Msg.pt = MousePos;

    if (IS_KEY_DOWN(gafAsyncKeyState, VK_SHIFT))
    {
        Msg.wParam |= MK_SHIFT;
    }

    if (IS_KEY_DOWN(gafAsyncKeyState, VK_CONTROL))
    {
        Msg.wParam |= MK_CONTROL;
    }

    if(mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        UserSetCursorPos(MousePos.x, MousePos.y, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_LEFTDOWN)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_LBUTTON, TRUE);
        Msg.message = SwapBtnMsg[0][SwapButtons];
        CurInfo->ButtonsDown |= SwapBtn[SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_LEFTUP)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_LBUTTON, FALSE);
        Msg.message = SwapBtnMsg[1][SwapButtons];
        CurInfo->ButtonsDown &= ~SwapBtn[SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_MIDDLEDOWN)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_MBUTTON, TRUE);
        Msg.message = WM_MBUTTONDOWN;
        CurInfo->ButtonsDown |= MK_MBUTTON;
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_MIDDLEUP)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_MBUTTON, FALSE);
        Msg.message = WM_MBUTTONUP;
        CurInfo->ButtonsDown &= ~MK_MBUTTON;
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_RIGHTDOWN)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_RBUTTON, TRUE);
        Msg.message = SwapBtnMsg[0][!SwapButtons];
        CurInfo->ButtonsDown |= SwapBtn[!SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_RIGHTUP)
    {
        SET_KEY_DOWN(gafAsyncKeyState, VK_RBUTTON, FALSE);
        Msg.message = SwapBtnMsg[1][!SwapButtons];
        CurInfo->ButtonsDown &= ~SwapBtn[!SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }

    if((mi->dwFlags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP)) &&
            (mi->dwFlags & MOUSEEVENTF_WHEEL))
    {
        /* fail because both types of events use the mouseData field */
        return FALSE;
    }

    if(mi->dwFlags & MOUSEEVENTF_XDOWN)
    {
        Msg.message = WM_XBUTTONDOWN;
        if(mi->mouseData & XBUTTON1)
        {
            SET_KEY_DOWN(gafAsyncKeyState, VK_XBUTTON1, TRUE);
            CurInfo->ButtonsDown |= MK_XBUTTON1;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
        if(mi->mouseData & XBUTTON2)
        {
            SET_KEY_DOWN(gafAsyncKeyState, VK_XBUTTON2, TRUE);
            CurInfo->ButtonsDown |= MK_XBUTTON2;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
    }
    else if(mi->dwFlags & MOUSEEVENTF_XUP)
    {
        Msg.message = WM_XBUTTONUP;
        if(mi->mouseData & XBUTTON1)
        {
            SET_KEY_DOWN(gafAsyncKeyState, VK_XBUTTON1, FALSE);
            CurInfo->ButtonsDown &= ~MK_XBUTTON1;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
        if(mi->mouseData & XBUTTON2)
        {
            SET_KEY_DOWN(gafAsyncKeyState, VK_XBUTTON2, FALSE);
            CurInfo->ButtonsDown &= ~MK_XBUTTON2;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
    }
    if(mi->dwFlags & MOUSEEVENTF_WHEEL)
    {
        Msg.message = WM_MOUSEWHEEL;
        Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, mi->mouseData);
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }

    return TRUE;
}

BOOL
FASTCALL
IntQueryTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    PDESKTOP pDesk;
    PTHREADINFO pti;

    pti = PsGetCurrentThreadWin32Thread();
    pDesk = pti->rpdesk;

    /* Always cleared with size set and return true. */
    RtlZeroMemory(lpEventTrack , sizeof(TRACKMOUSEEVENT));
    lpEventTrack->cbSize = sizeof(TRACKMOUSEEVENT);

    if ( pDesk->dwDTFlags & (DF_TME_LEAVE | DF_TME_HOVER) &&
            pDesk->spwndTrack &&
            pti->MessageQueue == pDesk->spwndTrack->head.pti->MessageQueue )
    {
        if ( pDesk->htEx != HTCLIENT )
            lpEventTrack->dwFlags |= TME_NONCLIENT;

        if ( pDesk->dwDTFlags & DF_TME_LEAVE )
            lpEventTrack->dwFlags |= TME_LEAVE;

        if ( pDesk->dwDTFlags & DF_TME_HOVER )
        {
            lpEventTrack->dwFlags |= TME_HOVER;
            lpEventTrack->dwHoverTime = pDesk->dwMouseHoverTime;
        }
        lpEventTrack->hwndTrack = UserHMGetHandle(pDesk->spwndTrack);
    }
    return TRUE;
}

BOOL
FASTCALL
IntTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    PDESKTOP pDesk;
    PTHREADINFO pti;
    PWND pWnd;
    POINT point;

    pti = PsGetCurrentThreadWin32Thread();
    pDesk = pti->rpdesk;

    if (!(pWnd = UserGetWindowObject(lpEventTrack->hwndTrack)))
        return FALSE;

    /* Tracking spwndTrack same as pWnd */
    if ( lpEventTrack->dwFlags & TME_CANCEL ) // Canceled mode.
    {
        if ( lpEventTrack->dwFlags & TME_LEAVE )
            pDesk->dwDTFlags &= ~DF_TME_LEAVE;

        if ( lpEventTrack->dwFlags & TME_HOVER )
        {
            if ( pDesk->dwDTFlags & DF_TME_HOVER )
            {   // Kill hover timer.
                IntKillTimer(pWnd, ID_EVENT_SYSTIMER_MOUSEHOVER, TRUE);
                pDesk->dwDTFlags &= ~DF_TME_HOVER;
            }
        }
    }
    else // Not Canceled.
    {
       pDesk->spwndTrack = pWnd;
        if ( lpEventTrack->dwFlags & TME_LEAVE )
            pDesk->dwDTFlags |= DF_TME_LEAVE;

        if ( lpEventTrack->dwFlags & TME_HOVER )
        {
            pDesk->dwDTFlags |= DF_TME_HOVER;

            if ( !lpEventTrack->dwHoverTime || lpEventTrack->dwHoverTime == HOVER_DEFAULT )
                pDesk->dwMouseHoverTime = gspv.iMouseHoverTime; // use the system default hover time-out.
            else
                pDesk->dwMouseHoverTime = lpEventTrack->dwHoverTime;
            // Start timer for the hover period.
            IntSetTimer( pWnd, ID_EVENT_SYSTIMER_MOUSEHOVER, pDesk->dwMouseHoverTime, SystemTimerProc, TMRF_SYSTEM);
            // Get windows thread message points.
            point = pWnd->head.pti->ptLast;
            // Set desktop mouse hover from the system default hover rectangle.
            RECTL_vSetRect(&pDesk->rcMouseHover,
                           point.x - gspv.iMouseHoverWidth  / 2,
                           point.y - gspv.iMouseHoverHeight / 2,
                           point.x + gspv.iMouseHoverWidth  / 2,
                           point.y + gspv.iMouseHoverHeight / 2);
        }
    }
    return TRUE;
}

BOOL
APIENTRY
NtUserTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    TRACKMOUSEEVENT saveTME;
    BOOL Ret = FALSE;

    TRACE("Enter NtUserTrackMouseEvent\n");
    UserEnterExclusive();

    _SEH2_TRY
    {
        ProbeForRead(lpEventTrack, sizeof(TRACKMOUSEEVENT), 1);
        RtlCopyMemory(&saveTME, lpEventTrack, sizeof(TRACKMOUSEEVENT));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(goto Exit;)
    }
    _SEH2_END;

    if ( saveTME.cbSize != sizeof(TRACKMOUSEEVENT) )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (saveTME.dwFlags & ~(TME_CANCEL | TME_QUERY | TME_NONCLIENT | TME_LEAVE | TME_HOVER) )
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        goto Exit;
    }

    if ( saveTME.dwFlags & TME_QUERY )
    {
        Ret = IntQueryTrackMouseEvent(&saveTME);
        _SEH2_TRY
        {
            ProbeForWrite(lpEventTrack, sizeof(TRACKMOUSEEVENT), 1);
            RtlCopyMemory(lpEventTrack, &saveTME, sizeof(TRACKMOUSEEVENT));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
    }
    else
    {
        Ret = IntTrackMouseEvent(&saveTME);
    }

Exit:
    TRACE("Leave NtUserTrackMouseEvent, ret=%i\n", Ret);
    UserLeave();
    return Ret;
}

extern MOUSEMOVEPOINT MouseHistoryOfMoves[];
extern INT gcur_count;

DWORD
APIENTRY
NtUserGetMouseMovePointsEx(
    UINT cbSize,
    LPMOUSEMOVEPOINT lpptIn,
    LPMOUSEMOVEPOINT lpptOut,
    int nBufPoints,
    DWORD resolution)
{
    MOUSEMOVEPOINT Safeppt;
    //BOOL Hit;
    INT Count = -1;
    DECLARE_RETURN(DWORD);

    TRACE("Enter NtUserGetMouseMovePointsEx\n");
    UserEnterExclusive();

    if ((cbSize != sizeof(MOUSEMOVEPOINT)) || (nBufPoints < 0) || (nBufPoints > 64))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        RETURN( -1);
    }

    if (!lpptIn || (!lpptOut && nBufPoints))
    {
        EngSetLastError(ERROR_NOACCESS);
        RETURN( -1);
    }

    _SEH2_TRY
    {
        ProbeForRead( lpptIn, cbSize, 1);
        RtlCopyMemory(&Safeppt, lpptIn, cbSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(RETURN( -1))
    }
    _SEH2_END;

    // http://msdn.microsoft.com/en-us/library/ms646259(v=vs.85).aspx
    // This explains the math issues in transforming points.
    Count = gcur_count; // FIFO is forward so retrieve backward.
    //Hit = FALSE;
    do
    {
        if (Safeppt.x == 0 && Safeppt.y == 0)
            break; // No test.
        // Finds the point, it returns the last nBufPoints prior to and including the supplied point.
        if (MouseHistoryOfMoves[Count].x == Safeppt.x && MouseHistoryOfMoves[Count].y == Safeppt.y)
        {
            if ( Safeppt.time ) // Now test time and it seems to be absolute.
            {
                if (Safeppt.time == MouseHistoryOfMoves[Count].time)
                {
                    //Hit = TRUE;
                    break;
                }
                else
                {
                    if (--Count < 0) Count = 63;
                    continue;
                }
            }
            //Hit = TRUE;
            break;
        }
        if (--Count < 0) Count = 63;
    }
    while ( Count != gcur_count);

    switch(resolution)
    {
        case GMMP_USE_DISPLAY_POINTS:
            if (nBufPoints)
            {
                _SEH2_TRY
                {
                    ProbeForWrite(lpptOut, cbSize, 1);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    SetLastNtError(_SEH2_GetExceptionCode());
                    _SEH2_YIELD(RETURN( -1))
                }
                _SEH2_END;
            }
            Count = nBufPoints;
            break;
        case GMMP_USE_HIGH_RESOLUTION_POINTS:
            break;
        default:
            EngSetLastError(ERROR_POINT_NOT_FOUND);
            RETURN( -1);
    }

    RETURN( Count);

CLEANUP:
    TRACE("Leave NtUserGetMouseMovePointsEx, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}
