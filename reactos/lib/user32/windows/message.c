/* $Id: message.c,v 1.27 2003/11/11 20:28:21 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/message.c
 * PURPOSE:         Messages
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <string.h>
#include <debug.h>

/*
 * @implemented
 */
LPARAM
STDCALL
GetMessageExtraInfo(VOID)
{
  UNIMPLEMENTED;
  return (LPARAM)0;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetMessagePos(VOID)
{
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();
  return(MAKELONG(ThreadData->LastMessage.pt.x, ThreadData->LastMessage.pt.y));
}


/*
 * @implemented
 */
LONG STDCALL
GetMessageTime(VOID)
{
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();
  return(ThreadData->LastMessage.time);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InSendMessage(VOID)
{
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
InSendMessageEx(
  LPVOID lpReserved)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return (LPARAM)0;
}


BOOL
MsgiAnsiToUnicodeMessage(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  *UnicodeMsg = *AnsiMsg;
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPWSTR Buffer = HeapAlloc(GetProcessHeap(), 0,
           AnsiMsg->wParam * sizeof(WCHAR));
        if (!Buffer)
          {
            return FALSE;
          }
        UnicodeMsg->lParam = (LPARAM)Buffer;
        break;
      }

    /* AnsiMsg->lParam is string (0-terminated) */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
      {
        UNICODE_STRING UnicodeString;
        RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)AnsiMsg->lParam);
        UnicodeMsg->lParam = (LPARAM)UnicodeString.Buffer;
        break;
      }

    case WM_NCCREATE:
    case WM_CREATE:
      {
        UNICODE_STRING UnicodeBuffer;
        struct s
        {
           CREATESTRUCTW cs;    /* new structure */
           LPCWSTR lpszName;    /* allocated Name */
           LPCWSTR lpszClass;   /* allocated Class */
        };
        struct s *xs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct s));
        if (!xs)
          {
            return FALSE;
          }
        xs->cs = *(CREATESTRUCTW *)AnsiMsg->lParam;
        if (HIWORD(xs->cs.lpszName))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)xs->cs.lpszName);
            xs->lpszName = xs->cs.lpszName = UnicodeBuffer.Buffer;
          }
        if (HIWORD(xs->cs.lpszClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)xs->cs.lpszClass);
            xs->lpszClass = xs->cs.lpszClass = UnicodeBuffer.Buffer;
          }
        UnicodeMsg->lParam = (LPARAM)xs;
        break;
      }

    case WM_MDICREATE:
      {
        UNICODE_STRING UnicodeBuffer;
        MDICREATESTRUCTW *cs =
            (MDICREATESTRUCTW *)HeapAlloc(GetProcessHeap(), 0, sizeof(*cs));

        if (!cs)
          {
            return FALSE;
          }

        *cs = *(MDICREATESTRUCTW *)AnsiMsg->lParam;

        if (HIWORD(cs->szClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)cs->szClass);
            cs->szClass = UnicodeBuffer.Buffer;
          }

        RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)cs->szTitle);
        cs->szTitle = UnicodeBuffer.Buffer;

        UnicodeMsg->lParam = (LPARAM)cs;
        break;
      }
    }

  return TRUE;
}


BOOL
MsgiAnsiToUnicodeReply(LPMSG UnicodeMsg, LPMSG AnsiMsg, LRESULT *Result)
{
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPWSTR Buffer = (LPWSTR)UnicodeMsg->lParam;
        LPSTR AnsiBuffer = (LPSTR)AnsiMsg->lParam;
        if (UnicodeMsg->wParam > 0 &&
            !WideCharToMultiByte(CP_ACP, 0, Buffer, -1,
            AnsiBuffer, UnicodeMsg->wParam, NULL, NULL))
          {
            AnsiBuffer[UnicodeMsg->wParam - 1] = 0;
          }
        HeapFree(GetProcessHeap(), 0, Buffer);
        break;
      }

    case WM_GETTEXTLENGTH:
    case CB_GETLBTEXTLEN:
    case LB_GETTEXTLEN:
      {
        /* FIXME: There may be one DBCS char for each Unicode char */
        *Result *= 2;
        break;
      }

    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
      {
        UNICODE_STRING UnicodeString;
        RtlInitUnicodeString(&UnicodeString, (PCWSTR)UnicodeMsg->lParam);
        RtlFreeUnicodeString(&UnicodeString);
        break;
      }

    case WM_NCCREATE:
    case WM_CREATE:
      {
	UNICODE_STRING UnicodeString;
        struct s
        {
           CREATESTRUCTW cs;	/* new structure */
           LPWSTR lpszName;	/* allocated Name */
           LPWSTR lpszClass;	/* allocated Class */
        };
        struct s *xs = (struct s *)UnicodeMsg->lParam;
        if (xs->lpszName)
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)xs->lpszName);
            RtlFreeUnicodeString(&UnicodeString);
          }
        if (xs->lpszClass)
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)xs->lpszClass);
            RtlFreeUnicodeString(&UnicodeString);
          }
        HeapFree(GetProcessHeap(), 0, xs);
      }
      break;

    case WM_MDICREATE:
      {
	UNICODE_STRING UnicodeString;
        MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)UnicodeMsg->lParam;
        if (HIWORD(cs->szTitle))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szTitle);
            RtlFreeUnicodeString(&UnicodeString);
          }
        if (HIWORD(cs->szClass))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szClass);
            RtlFreeUnicodeString(&UnicodeString);
          }
        HeapFree(GetProcessHeap(), 0, cs);
      }
      break;
    }
  return(TRUE);
}


VOID STATIC
User32ConvertToAsciiMessage(UINT* Msg, WPARAM* wParam, LPARAM* lParam)
{
  switch((*Msg))
    {
    case WM_CREATE:
    case WM_NCCREATE:
      {
	CREATESTRUCTA* CsA;
	CREATESTRUCTW* CsW;
	UNICODE_STRING UString;
	ANSI_STRING AString;

	CsW = (CREATESTRUCTW*)(*lParam);
	CsA = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(CREATESTRUCTA));
	memcpy(CsA, CsW, sizeof(CREATESTRUCTW));

	RtlInitUnicodeString(&UString, CsW->lpszName);
	RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
	CsA->lpszName = AString.Buffer;
	if (HIWORD((ULONG)CsW->lpszClass) != 0)
	  {
	    RtlInitUnicodeString(&UString, CsW->lpszClass);
	    RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
	    CsA->lpszClass = AString.Buffer;
	  }
	(*lParam) = (LPARAM)CsA;
	break;
      }
    case WM_SETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	RtlInitUnicodeString(&UnicodeString, (PWSTR) *lParam);
	if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
	                                            &UnicodeString,
	                                            TRUE)))
	  {
	    *lParam = (LPARAM) AnsiString.Buffer;
	  }
	break;
      }
    }
}


VOID STATIC
User32FreeAsciiConvertedMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch(Msg)
    {
    case WM_GETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	LPSTR TempString;
	LPSTR InString;
	InString = (LPSTR)lParam;
	TempString = RtlAllocateHeap(RtlGetProcessHeap(), 0, strlen(InString) + 1);
	strcpy(TempString, InString);
	RtlInitAnsiString(&AnsiString, TempString);
	UnicodeString.Length = wParam * sizeof(WCHAR);
	UnicodeString.MaximumLength = wParam * sizeof(WCHAR);
	UnicodeString.Buffer = (PWSTR)lParam;
	if (! NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
	                                              &AnsiString,
	                                              FALSE)))
	  {
	    if (1 <= wParam)
	      {
		UnicodeString.Buffer[0] = L'\0';
	      }
	  }
	RtlFreeHeap(RtlGetProcessHeap(), 0, TempString);
	break;
      }
    case WM_SETTEXT:
      {
	ANSI_STRING AnsiString;
	RtlInitAnsiString(&AnsiString, (PSTR) lParam);
	RtlFreeAnsiString(&AnsiString);
	break;
      }
    case WM_CREATE:
    case WM_NCCREATE:
      {
	CREATESTRUCTA* Cs;

	Cs = (CREATESTRUCTA*)lParam;
	RtlFreeHeap(RtlGetProcessHeap(), 0, (LPSTR)Cs->lpszName);
	if (HIWORD((ULONG)Cs->lpszClass) != 0)
	  {
            RtlFreeHeap(RtlGetProcessHeap(), 0, (LPSTR)Cs->lpszClass);
	  }
	RtlFreeHeap(RtlGetProcessHeap(), 0, Cs);
	break;
      }
    }
}


/*
 * @implemented
 */
LRESULT STDCALL
CallWindowProcA(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  LRESULT Result;
  BOOL IsHandle;
  WndProcHandle wphData;

  IsHandle = NtUserDereferenceWndProcHandle(lpPrevWndFunc,&wphData);
  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (!IsHandle)
  {
      return(lpPrevWndFunc(hWnd, Msg, wParam, lParam));
  } else {
    if (wphData.IsUnicode)
    {
      if (!MsgiAnsiToUnicodeMessage(&UnicodeMsg, &AnsiMsg))
        {
          return(FALSE);
        }
      Result = wphData.WindowProc(UnicodeMsg.hwnd, UnicodeMsg.message,
                                  UnicodeMsg.wParam, UnicodeMsg.lParam);
      if (!MsgiAnsiToUnicodeReply(&UnicodeMsg, &AnsiMsg, &Result))
        {
          return(FALSE);
        }
      return(Result);
    }
  else
    {
		return(wphData.WindowProc(hWnd, Msg, wParam, lParam));
	}
    }
}


/*
 * @implemented
 */
LRESULT STDCALL
CallWindowProcW(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  BOOL IsHandle;
  WndProcHandle wphData;

  IsHandle = NtUserDereferenceWndProcHandle(lpPrevWndFunc,&wphData);
  if (!IsHandle)
  {
      return(lpPrevWndFunc(hWnd, Msg, wParam, lParam));
  } else {
   if (!wphData.IsUnicode)
    {
      LRESULT Result;
      User32ConvertToAsciiMessage(&Msg, &wParam, &lParam);
      Result = wphData.WindowProc(hWnd, Msg, wParam, lParam);
      User32FreeAsciiConvertedMessage(Msg, wParam, lParam);
      return(Result);
    }
  else
    {
		return(wphData.WindowProc(hWnd, Msg, wParam, lParam));
	}
    }
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageA(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage(lpmsg));
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageW(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage((LPMSG)lpmsg));
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetMessageA(LPMSG lpMsg,
	    HWND hWnd,
	    UINT wMsgFilterMin,
	    UINT wMsgFilterMax)
{
  BOOL Res;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  Res = NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = *lpMsg;
    }
  return(Res);
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetMessageW(LPMSG lpMsg,
	    HWND hWnd,
	    UINT wMsgFilterMin,
	    UINT wMsgFilterMax)
{
  BOOL Res;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  Res = NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = *lpMsg;
    }
  return(Res);
}


/*
 * @implemented
 */
WINBOOL STDCALL
PeekMessageA(LPMSG lpMsg,
	     HWND hWnd,
	     UINT wMsgFilterMin,
	     UINT wMsgFilterMax,
	     UINT wRemoveMsg)
{
  BOOL Res;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  Res =  NtUserPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = *lpMsg;
    }
  return(Res);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
PeekMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  BOOL Res;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();
  
  Res = NtUserPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = *lpMsg;
    }
  return(Res);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
PostMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostMessage(hWnd, Msg, wParam, lParam);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
PostMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostMessage(hWnd, Msg, wParam, lParam);
}


/*
 * @implemented
 */
VOID
STDCALL
PostQuitMessage(
  int nExitCode)
{
  (void) NtUserPostMessage(NULL, WM_QUIT, nExitCode, 0);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
PostThreadMessageA(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
PostThreadMessageW(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageW(HWND hWnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  return(NtUserSendMessage(hWnd, Msg, wParam, lParam));
}


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UcMsg;
  LRESULT Result;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;

  if (!MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return(FALSE);
    }
  Result = SendMessageW(UcMsg.hwnd, UcMsg.message, UcMsg.wParam, UcMsg.lParam);
  if (!MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
    {
      return(FALSE);
    }
  return(Result);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SendMessageCallbackA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SendMessageCallbackW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendMessageTimeoutA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendMessageTimeoutW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
TranslateMessage(CONST MSG *lpMsg)
{
  return(TranslateMessageEx((LPMSG)lpMsg, 0));
}

/*
 * @implemented
 */
WINBOOL STDCALL
TranslateMessageEx(CONST MSG *lpMsg, DWORD unk)
{
  return(NtUserTranslateMessage((LPMSG)lpMsg, (HKL)unk));
}


/*
 * @implemented
 */
WINBOOL
STDCALL
WaitMessage(VOID)
{
  return NtUserWaitMessage();
}


/*
 * @implemented
 */
UINT STDCALL
RegisterWindowMessageA(LPCSTR lpString)
{
  UNICODE_STRING String;
  BOOLEAN Result;
  UINT Atom;

  Result = RtlCreateUnicodeStringFromAsciiz(&String, (PCSZ)lpString);
  if (!Result)
    {
      return(0);
    }
  Atom = NtUserRegisterWindowMessage(&String);
  RtlFreeUnicodeString(&String);
  return(Atom);
}


/*
 * @implemented
 */
UINT STDCALL
RegisterWindowMessageW(LPCWSTR lpString)
{
  UNICODE_STRING String;

  RtlInitUnicodeString(&String, lpString);
  return(NtUserRegisterWindowMessage(&String));
}

/*
 * @implemented
 */
HWND STDCALL
SetCapture(HWND hWnd)
{
  return(NtUserSetCapture(hWnd));
}

/*
 * @implemented
 */
HWND STDCALL
GetCapture(VOID)
{
  return(NtUserGetCapture());
}

/*
 * @implemented
 */
WINBOOL STDCALL
ReleaseCapture(VOID)
{
  NtUserSetCapture(NULL);
  return(TRUE);
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetQueueStatus(UINT flags)
{
   DWORD ret;
   WORD changed_bits, wake_bits; 

#if 0 /* wine stuff. don't know what it does... */

   /* check for pending X events */
   if (USER_Driver.pMsgWaitForMultipleObjectsEx)
      USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );
#endif

   ret = NtUserGetQueueStatus(TRUE /*ClearChanges*/);

   changed_bits = LOWORD(ret);
   wake_bits = HIWORD(ret);

   return MAKELONG(changed_bits & flags, wake_bits & flags);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL GetInputState(VOID)
{
   DWORD ret;
   WORD  wake_bits;

#if 0 /* wine stuff. don't know what it does... */ 

   /* check for pending X events */
   if (USER_Driver.pMsgWaitForMultipleObjectsEx)
     USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );
#endif

   ret = NtUserGetQueueStatus(FALSE /*ClearChanges*/);
   
   wake_bits = HIWORD(ret);

   return wake_bits & (QS_KEY | QS_MOUSEBUTTON);
}

/*
 * @implemented
 */
WINBOOL STDCALL SetMessageQueue(int cMessagesMax)
{
  /* Function does nothing on 32 bit windows */
  return TRUE;
}



/* EOF */
