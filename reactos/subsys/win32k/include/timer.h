#ifndef _SUBSYS_WIN32K_INCLUDE_TIMER_H
#define _SUBSYS_WIN32K_INCLUDE_TIMER_H

typedef struct _MSG_TIMER_ENTRY{
   LIST_ENTRY     ListEntry;
   LARGE_INTEGER  Timeout;
   HANDLE          ThreadID;
   UINT           Period;
   MSG            Msg;
} MSG_TIMER_ENTRY, *PMSG_TIMER_ENTRY;

NTSTATUS FASTCALL InitTimerImpl(VOID);
VOID FASTCALL RemoveTimersThread(HANDLE ThreadID);
PMSG_TIMER_ENTRY FASTCALL IntRemoveTimer(HWND hWnd, UINT_PTR IDEvent, HANDLE ThreadID, BOOL SysTimer);
UINT_PTR FASTCALL IntSetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc, BOOL SystemTimer);

#endif /* ndef _SUBSYS_WIN32K_INCLUDE_TIMER_H */
