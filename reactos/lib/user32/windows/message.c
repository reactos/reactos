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


#if 0
LRESULT STDCALL SendMessageA( HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam )
{
	return  MSG_SendMessage( hwnd, msg, wParam, lParam,  FALSE );
}



LRESULT STDCALL SendMessageW(  HWND hwnd, UINT msg,   WPARAM wParam, 
  LPARAM lParam  ) 
{
	return MSG_SendMessage( hwnd, msg, wParam, lParam,  TRUE );
}
#endif


/***********************************************************************
 *           MSG_SendMessage
 *
 * Implementation of an inter-task SendMessage.
 */
LRESULT MSG_SendMessage( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,  WINBOOL bUnicode )
{
    WND * wndPtr;
    LRESULT ret;
    MSG msg;


//    SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wParam, lParam );
#if 0
    WND **list, **ppWnd;
    if (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST)
    {
        if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
            return TRUE;
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!WIN_IsWindow(wndPtr->hwndSelf)) 
		continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                MSG_SendMessage( wndPtr->hwndSelf, message, wParam, lParam, bUnicode );
        }
	WIN_DestroyList( list );
        return TRUE;
    }
#endif

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
	MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        return 0;
    }


    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = 0;
    msg.pt.x = 0;
    msg.pt.y = 0; 

 
  //  SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, ret );
    return ret;
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


/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
WINBOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd, HWND hwndOwner,
                               WPARAM code, WORD flags, WINBOOL sendIdle ) 
{
}