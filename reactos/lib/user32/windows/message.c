/* $Id: message.c,v 1.36 2004/03/11 14:47:43 weiden Exp $
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
  return (LPARAM)NtUserCallNoParam(NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO);
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
BOOL
STDCALL
InSendMessage(VOID)
{
  /* return(NtUserGetThreadState(THREADSTATE_INSENDMESSAGE) != ISMEX_NOSEND); */
  UNIMPLEMENTED;
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
  /* return NtUserGetThreadState(THREADSTATE_INSENDMESSAGE); */
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  return NtUserSetMessageExtraInfo(lParam);
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
MsgiAnsiToUnicodeCleanup(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        HeapFree(GetProcessHeap(), 0, (PVOID) UnicodeMsg->lParam);
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
    }

  MsgiAnsiToUnicodeCleanup(UnicodeMsg, AnsiMsg);

  return TRUE;
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

LRESULT FASTCALL
IntCallWindowProcW(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  LRESULT Result;

  if (IsAnsiProc)
    {
      User32ConvertToAsciiMessage(&Msg, &wParam, &lParam);
      Result = WndProc(hWnd, Msg, wParam, lParam);
      User32FreeAsciiConvertedMessage(Msg, wParam, lParam);
      return Result;
    }
  else
    {
      return WndProc(hWnd, Msg, wParam, lParam);
    }
}

STATIC LRESULT FASTCALL
IntCallWindowProcA(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  LRESULT Result;

  if (IsAnsiProc)
    {
      return WndProc(hWnd, Msg, wParam, lParam);
    }
  else
    {
      AnsiMsg.hwnd = hWnd;
      AnsiMsg.message = Msg;
      AnsiMsg.wParam = wParam;
      AnsiMsg.lParam = lParam;
      if (! MsgiAnsiToUnicodeMessage(&UnicodeMsg, &AnsiMsg))
        {
          return FALSE;
        }
      Result = WndProc(UnicodeMsg.hwnd, UnicodeMsg.message,
                       UnicodeMsg.wParam, UnicodeMsg.lParam);
      if (! MsgiAnsiToUnicodeReply(&UnicodeMsg, &AnsiMsg, &Result))
        {
          return FALSE;
        }
      return Result;
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
  BOOL IsHandle;
  WndProcHandle wphData;

  IsHandle = NtUserDereferenceWndProcHandle(lpPrevWndFunc,&wphData);
  if (! IsHandle)
    {
      return IntCallWindowProcA(TRUE, lpPrevWndFunc, hWnd, Msg, wParam, lParam);
    }
  else
    {
      return IntCallWindowProcA(! wphData.IsUnicode, wphData.WindowProc,
                                hWnd, Msg, wParam, lParam);
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
  if (! IsHandle)
    {
      return IntCallWindowProcW(FALSE, lpPrevWndFunc, hWnd, Msg, wParam, lParam);
    }
  else
    {
      return IntCallWindowProcW(! wphData.IsUnicode, wphData.WindowProc,
                                hWnd, Msg, wParam, lParam);
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
BOOL STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
SendMessageW(HWND Wnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  NTUSERSENDMESSAGEINFO Info;
  LRESULT Result;

  Info.Ansi = FALSE;
  Result = NtUserSendMessage(Wnd, Msg, wParam, lParam, &Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      Result = IntCallWindowProcW(Info.Ansi, Info.Proc, Wnd, Msg, wParam, lParam);
    }

  return Result;
}


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageA(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UcMsg;
  LRESULT Result;
  NTUSERSENDMESSAGEINFO Info;

  AnsiMsg.hwnd = Wnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  Info.Ansi = TRUE;
  Result = NtUserSendMessage(UcMsg.hwnd, UcMsg.message,
                             UcMsg.wParam, UcMsg.lParam, &Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      if (Info.Ansi)
        {
          /* Ansi message and Ansi window proc, that's easy. Clean up
             the Unicode message though */
          MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
          Result = IntCallWindowProcA(Info.Ansi, Info.Proc, Wnd, Msg, wParam, lParam);
        }
      else
        {
          /* Unicode winproc. Although we started out with an Ansi message we
             already converted it to Unicode for the kernel call. Reuse that
             message to avoid another conversion */
          Result = IntCallWindowProcW(Info.Ansi, Info.Proc, UcMsg.hwnd,
                                      UcMsg.message, UcMsg.wParam, UcMsg.lParam);
          if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
            {
              return FALSE;
            }
        }
    }
  else
    {
      /* Message sent by kernel. Convert back to Ansi */
      if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
        {
          return FALSE;
        }
    }

  return Result;
}


/*
 * @implemented
 */
BOOL
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
BOOL
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
  MSG AnsiMsg;
  MSG UcMsg;
  LRESULT Result;
  NTUSERSENDMESSAGEINFO Info;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  Info.Ansi = TRUE;
  Result = NtUserSendMessageTimeout(UcMsg.hwnd, UcMsg.message,
                                    UcMsg.wParam, UcMsg.lParam,
                                    fuFlags, uTimeout, (ULONG_PTR*)lpdwResult, &Info);
  if(!Result)
  {
      return FALSE;
  }
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      if (Info.Ansi)
        {
          /* Ansi message and Ansi window proc, that's easy. Clean up
             the Unicode message though */
          MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
          Result = IntCallWindowProcA(Info.Ansi, Info.Proc, hWnd, Msg, wParam, lParam);
        }
      else
        {
          /* Unicode winproc. Although we started out with an Ansi message we
             already converted it to Unicode for the kernel call. Reuse that
             message to avoid another conversion */
          Result = IntCallWindowProcW(Info.Ansi, Info.Proc, UcMsg.hwnd,
                                      UcMsg.message, UcMsg.wParam, UcMsg.lParam);
          if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
            {
              return FALSE;
            }
        }
      if(lpdwResult)
        *lpdwResult = Result;
      Result = TRUE;
    }
  else
    {
      /* Message sent by kernel. Convert back to Ansi */
      if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
        {
          return FALSE;
        }
    }

  return Result;
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
  NTUSERSENDMESSAGEINFO Info;
  LRESULT Result;

  Info.Ansi = FALSE;
  Result = NtUserSendMessageTimeout(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, 
                                    lpdwResult, &Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      Result = IntCallWindowProcW(Info.Ansi, Info.Proc, hWnd, Msg, wParam, lParam);
      if(lpdwResult)
        *lpdwResult = Result;
      return TRUE;
    }

  return Result;
}


/*
 * @unimplemented
 */
BOOL
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
BOOL
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
BOOL STDCALL
TranslateMessage(CONST MSG *lpMsg)
{
  return(TranslateMessageEx((LPMSG)lpMsg, 0));
}

/*
 * @implemented
 */
BOOL STDCALL
TranslateMessageEx(CONST MSG *lpMsg, DWORD unk)
{
  return(NtUserTranslateMessage((LPMSG)lpMsg, (HKL)unk));
}


/*
 * @implemented
 */
BOOL
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
BOOL STDCALL
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
RealGetQueueStatus(UINT flags)
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
BOOL STDCALL GetInputState(VOID)
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
BOOL STDCALL SetMessageQueue(int cMessagesMax)
{
  /* Function does nothing on 32 bit windows */
  return TRUE;
}
typedef DWORD (WINAPI * RealGetQueueStatusProc)(UINT flags);
typedef DWORD (WINAPI * RealMsgWaitForMultipleObjectsExProc)(DWORD nCount, LPHANDLE lpHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags);

typedef struct _USER_MESSAGE_PUMP_ADDRESSES {
	DWORD cbSize;
	//NtUserRealInternalGetMessageProc NtUserRealInternalGetMessage;
	//NtUserRealWaitMessageExProc NtUserRealWaitMessageEx;
	RealGetQueueStatusProc RealGetQueueStatus;
	RealMsgWaitForMultipleObjectsExProc RealMsgWaitForMultipleObjectsEx;
} USER_MESSAGE_PUMP_ADDRESSES, * PUSER_MESSAGE_PUMP_ADDRESSES;

DWORD
STDCALL
RealMsgWaitForMultipleObjectsEx(
  DWORD nCount,
  LPHANDLE pHandles,
  DWORD dwMilliseconds,
  DWORD dwWakeMask,
  DWORD dwFlags);

typedef BOOL (WINAPI * MESSAGEPUMPHOOKPROC)(BOOL Unregistering,PUSER_MESSAGE_PUMP_ADDRESSES MessagePumpAddresses);

RTL_CRITICAL_SECTION gcsMPH;
MESSAGEPUMPHOOKPROC gpfnInitMPH;
DWORD gcLoadMPH = 0;
USER_MESSAGE_PUMP_ADDRESSES gmph = {sizeof(USER_MESSAGE_PUMP_ADDRESSES),
	//NtUserRealInternalGetMessage,
	//NtUserRealInternalWaitMessageEx,
	RealGetQueueStatus,
	RealMsgWaitForMultipleObjectsEx
};

DWORD gfMessagePumpHook = 0;

BOOL WINAPI IsInsideMessagePumpHook()
{
	if(!gfMessagePumpHook)
		return FALSE;
	
	/* Since our TEB doesnt match that of real windows, testing this value is useless until we know what it does
	PUCHAR NtTeb = (PUCHAR)NtCurrentTeb();

	if(!*(PLONG*)&NtTeb[0x708])
		return FALSE;

	if(**(PLONG*)&NtTeb[0x708] <= 0)
		return FALSE;*/

	return TRUE;
}

void WINAPI ResetMessagePumpHook(PUSER_MESSAGE_PUMP_ADDRESSES Addresses)
{
	Addresses->cbSize = sizeof(USER_MESSAGE_PUMP_ADDRESSES);
	//Addresses->NtUserRealInternalGetMessage = (NtUserRealInternalGetMessageProc)NtUserRealInternalGetMessage;
	//Addresses->NtUserRealWaitMessageEx = (NtUserRealWaitMessageExProc)NtUserRealInternalWaitMessageEx;
	Addresses->RealGetQueueStatus = RealGetQueueStatus;
	Addresses->RealMsgWaitForMultipleObjectsEx = RealMsgWaitForMultipleObjectsEx;
}

BOOL WINAPI RegisterMessagePumpHook(MESSAGEPUMPHOOKPROC Hook)
{
	RtlEnterCriticalSection(&gcsMPH);
	if(!Hook) {
		SetLastError(ERROR_INVALID_PARAMETER);
		RtlLeaveCriticalSection(&gcsMPH);
		return FALSE;
	}
	if(!gcLoadMPH) {
		USER_MESSAGE_PUMP_ADDRESSES Addresses;
		gpfnInitMPH = Hook;
		ResetMessagePumpHook(&Addresses);
		if(!Hook(FALSE, &Addresses) || !Addresses.cbSize) {
			RtlLeaveCriticalSection(&gcsMPH);
			return FALSE;
		}
		memcpy(&gmph, &Addresses, Addresses.cbSize);
	} else {
		if(gpfnInitMPH != Hook) {
			RtlLeaveCriticalSection(&gcsMPH);
			return FALSE;
		}
	}
	if(NtUserCallNoParam(NOPARAM_ROUTINE_INIT_MESSAGE_PUMP)) {
		RtlLeaveCriticalSection(&gcsMPH);
		return FALSE;
	}
	if (!gcLoadMPH++) {
		InterlockedExchange(&gfMessagePumpHook, 1);
	}
	RtlLeaveCriticalSection(&gcsMPH);
	return TRUE;
}

BOOL WINAPI UnregisterMessagePumpHook(VOID)
{
	RtlEnterCriticalSection(&gcsMPH);
	if(gcLoadMPH > 0) {
		if(NtUserCallNoParam(NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP)) {
			gcLoadMPH--;
			if(!gcLoadMPH) {
				InterlockedExchange(&gfMessagePumpHook, 0);
				gpfnInitMPH(TRUE, NULL);
				ResetMessagePumpHook(&gmph);
				gpfnInitMPH = 0;
			}
			RtlLeaveCriticalSection(&gcsMPH);
			return TRUE;
		}
	}
	RtlLeaveCriticalSection(&gcsMPH);
	return FALSE;
}

DWORD WINAPI GetQueueStatus(UINT flags)
{
	return IsInsideMessagePumpHook() ? gmph.RealGetQueueStatus(flags) : RealGetQueueStatus(flags);
}

DWORD WINAPI MsgWaitForMultipleObjectsEx(DWORD nCount, LPHANDLE lpHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags)
{
	return IsInsideMessagePumpHook() ? gmph.RealMsgWaitForMultipleObjectsEx(nCount, lpHandles,dwMilliseconds, dwWakeMask, dwFlags) : RealMsgWaitForMultipleObjectsEx(nCount, lpHandles,dwMilliseconds, dwWakeMask, dwFlags);
}
