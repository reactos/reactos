#ifndef __SUBSYS_WIN32K_INCLUDE_CALLBACK_H
#define __SUBSYS_WIN32K_INCLUDE_CALLBACK_H

LRESULT STDCALL
W32kCallWindowProc(WNDPROC Proc,
		   HWND Wnd,
		   UINT Message,
		   WPARAM wParam,
		   LPARAM lParam);
LRESULT STDCALL
W32kCallTrampolineWindowProc(WNDPROC Proc,
			     HWND Wnd,
			     UINT Message,
			     WPARAM wParam,
			     LPARAM lParam);
LRESULT STDCALL
W32kSendNCCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct);
LRESULT STDCALL
W32kSendCREATEMessage(HWND Wnd, CREATESTRUCTW* CreateStruct);
VOID STDCALL
W32kCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result);
LRESULT STDCALL
W32kSendNCCALCSIZEMessage(HWND Wnd, BOOL Validate, PRECT Rect,
			  NCCALCSIZE_PARAMS* Params);
LRESULT STDCALL
W32kSendGETMINMAXINFOMessage(HWND Wnd, MINMAXINFO* MinMaxInfo);

#endif /* __SUBSYS_WIN32K_INCLUDE_CALLBACK_H */
