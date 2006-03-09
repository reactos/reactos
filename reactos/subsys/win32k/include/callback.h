#ifndef _WIN32K_CALLBACK_H
#define _WIN32K_CALLBACK_H

LRESULT STDCALL
co_IntCallWindowProc(WNDPROC Proc,
                  BOOLEAN IsAnsiProc,
                  HWND Wnd,
                  UINT Message,
                  WPARAM wParam,
                  LPARAM lParam,
                  INT lParamBufferSize);

VOID STDCALL
co_IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result);


HMENU STDCALL
co_IntLoadSysMenuTemplate();

BOOL STDCALL
co_IntLoadDefaultCursors(VOID);

LRESULT STDCALL
co_IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

VOID FASTCALL
IntCleanupThreadCallbacks(PW32THREAD W32Thread);

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size);

VOID FASTCALL
IntCbFreeMemory(PVOID Data);

#endif /* _WIN32K_CALLBACK_H */
