#pragma once

LRESULT APIENTRY
co_IntCallWindowProc(WNDPROC Proc,
                  BOOLEAN IsAnsiProc,
                  HWND Wnd,
                  UINT Message,
                  WPARAM wParam,
                  LPARAM lParam,
                  INT lParamBufferSize);

VOID APIENTRY
co_IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result);


HMENU APIENTRY
co_IntLoadSysMenuTemplate(VOID);

BOOL APIENTRY
co_IntLoadDefaultCursors(VOID);

LRESULT APIENTRY
co_IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                INT Mod,
                ULONG_PTR offPfn,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

LRESULT APIENTRY
co_IntCallEventProc(HWINEVENTHOOK hook,
                           DWORD event,
                             HWND hwnd,
                         LONG idObject,
                          LONG idChild,
                   DWORD dwEventThread,
                   DWORD dwmsEventTime,
                     WINEVENTPROC Proc);

VOID FASTCALL
IntCleanupThreadCallbacks(PTHREADINFO W32Thread);

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size);

VOID FASTCALL
IntCbFreeMemory(PVOID Data);

HMENU APIENTRY co_IntCallLoadMenu(HINSTANCE,PUNICODE_STRING);

NTSTATUS APIENTRY co_IntClientThreadSetup(VOID);

BOOL
NTAPI
co_IntClientLoadLibrary(PUNICODE_STRING strLibName,
                        PUNICODE_STRING strInitFunc,
                        BOOL Unload,
                        BOOL ApiHook);

BOOL
APIENTRY
co_IntGetCharsetInfo(LCID Locale, PCHARSETINFO pCs);

HANDLE FASTCALL co_IntCopyImage(HANDLE,UINT,INT,INT,UINT);

BOOL FASTCALL co_IntSetWndIcons(VOID);
VOID FASTCALL co_IntDeliverUserAPC(VOID);
