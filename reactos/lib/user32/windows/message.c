#include <windows.h>
#include <user32/message.h>
#include <user32/winproc.h>
#include <user32/win.h>
#include <user32/spy.h>
#include <user32/hook.h>


/***********************************************************************
 *           SendMessage  Send Window Message
 *
 *  Sends a message to the window procedure of the specified window.
 *  SendMessage() will not return until the called window procedure
 *  either returns or calls ReplyMessage().
 *
 *  Use PostMessage() to send message and return immediately. A window
 *  procedure may use InSendMessage() to detect
 *  SendMessage()-originated messages.
 *
 *  Applications which communicate via HWND_BROADCAST may use
 *  RegisterWindowMessage() to obtain a unique message to avoid conflicts
 *  with other applications.
 *
 * CONFORMANCE
 * 
 *  ECMA-234, Win 
 */

LRESULT STDCALL SendMessageA( HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam )
{
    WND * wndPtr;
    WND **list, **ppWnd;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST)
    {
        if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
            return TRUE;
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessageA( wndPtr->hwndSelf, msg, wParam, lParam );
        }
	HeapFree( GetProcessHeap(), 0, list );
        return TRUE;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
	MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        //WARN(msg, "invalid hwnd %08x\n", hwnd );
        return 0;
    }
#if 0
    if (QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))
        return 0;  /* Don't send anything if the task is dying */
#endif
    SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wParam, lParam );

    if (wndPtr->hmemTaskQ != GetFastQueue())
        ret = MSG_SendMessage( wndPtr->hmemTaskQ, hwnd, msg, wParam, lParam,
                               QUEUE_SM_ASCII );
    else
        ret = CallWindowProcA( (WNDPROC)wndPtr->winproc,
                                 hwnd, msg, wParam, lParam );

    SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, ret );
    return ret;
}



LRESULT STDCALL SendMessageW( 
  HWND hwnd,    /* Window to send message to. If HWND_BROADCAST, 
                 the message will be sent to all top-level windows. */

  UINT msg,      /* message */
  WPARAM wParam, /* message parameter */
  LPARAM lParam    /* additional message parameter */
) {
    WND * wndPtr;
    WND **list, **ppWnd;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST)
    {
        if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
            return TRUE;
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessageW( wndPtr->hwndSelf, msg, wParam, lParam );
        }
	HeapFree( GetProcessHeap(), 0, list );
        return TRUE;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
        MSG_CallWndProcHook( (LPMSG)&hwnd, TRUE);

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        //WARN(msg, "invalid hwnd %08x\n", hwnd );
        return 0;
    }
#if 0
    if (QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))
        return 0;  /* Don't send anything if the task is dying */
#endif
    SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wParam, lParam );

    if (wndPtr->hmemTaskQ != GetFastQueue())
        ret = MSG_SendMessage( wndPtr->hmemTaskQ, hwnd, msg, wParam, lParam,
                                QUEUE_SM_ASCII | QUEUE_SM_UNICODE );
    else
        ret = CallWindowProcW( (WNDPROC)wndPtr->winproc,
                                 hwnd, msg, wParam, lParam );

    SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           MSG_SendMessage
 *
 * Implementation of an inter-task SendMessage.
 */
LRESULT MSG_SendMessage( HQUEUE hDestQueue, HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam, WORD flags )
{
	return 0;
}
/************************************************************************
 *	     MSG_CallWndProcHook
 */
void  MSG_CallWndProcHook( LPMSG pmsg, WINBOOL bUnicode )
{
   CWPSTRUCT cwp;

   cwp.lParam = pmsg->lParam;
   cwp.wParam = pmsg->wParam;
   cwp.message = pmsg->message;
   cwp.hwnd = pmsg->hwnd;

   if (bUnicode) 
	HOOK_CallHooksW(WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp);
   else 
	HOOK_CallHooksA( WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp );

   pmsg->lParam = cwp.lParam;
   pmsg->wParam = cwp.wParam;
   pmsg->message = cwp.message;
   pmsg->hwnd = cwp.hwnd;
}
