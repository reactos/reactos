#ifndef _WIN32K_CALLBACK_H
#define _WIN32K_CALLBACK_H

LRESULT INTERNAL_CALL
IntCallWindowProc(WNDPROC Proc,
                  BOOLEAN IsAnsiProc,
                  PWINDOW_OBJECT Window,
                  UINT Message,
                  WPARAM wParam,
                  LPARAM lParam,
                  INT lParamBufferSize);

VOID INTERNAL_CALL
IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			   PWINDOW_OBJECT Window,
			   UINT Msg,
			   ULONG_PTR CompletionCallbackContext,
			   LRESULT Result);


HMENU INTERNAL_CALL
IntLoadSysMenuTemplate();

BOOL INTERNAL_CALL
IntLoadDefaultCursors(VOID);

LRESULT INTERNAL_CALL
IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

VOID INTERNAL_CALL
IntCleanupThreadCallbacks(PW32THREAD W32Thread);

PVOID INTERNAL_CALL
IntCbAllocateMemory(ULONG Size);

VOID INTERNAL_CALL
IntCbFreeMemory(PVOID Data);

#endif /* _WIN32K_CALLBACK_H */
