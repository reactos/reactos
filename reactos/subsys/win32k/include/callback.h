#ifndef _WIN32K_CALLBACK_H
#define _WIN32K_CALLBACK_H

LRESULT STDCALL
IntCallWindowProc(WNDPROC Proc,
		   HWND Wnd,
		   UINT Message,
		   WPARAM wParam,
		   LPARAM lParam);
LRESULT STDCALL
IntCallTrampolineWindowProc(WNDPROC Proc,
			     HWND Wnd,
			     UINT Message,
			     WPARAM wParam,
			     LPARAM lParam);
LRESULT STDCALL
IntSendNCCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct);
LRESULT STDCALL
IntSendCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct);
VOID STDCALL
IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result);

LRESULT STDCALL
IntSendNCCALCSIZEMessage(HWND Wnd, BOOL Validate, PRECT Rect,
			  NCCALCSIZE_PARAMS* Params);

LRESULT STDCALL
IntSendGETMINMAXINFOMessage(HWND Wnd, MINMAXINFO* MinMaxInfo);

LRESULT STDCALL
IntSendWINDOWPOSCHANGINGMessage(HWND Wnd, WINDOWPOS* WindowPos);

LRESULT STDCALL
IntSendWINDOWPOSCHANGEDMessage(HWND Wnd, WINDOWPOS* WindowPos);

LRESULT STDCALL
IntSendSTYLECHANGINGMessage(HWND Wnd, DWORD WhichStyle, STYLESTRUCT* Style);

LRESULT STDCALL
IntSendSTYLECHANGEDMessage(HWND Wnd, DWORD WhichStyle, STYLESTRUCT* Style);

HMENU STDCALL
IntLoadSysMenuTemplate();

BOOL STDCALL
IntLoadDefaultCursors(BOOL SetDefault);

LRESULT STDCALL
IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

#endif /* _WIN32K_CALLBACK_H */
