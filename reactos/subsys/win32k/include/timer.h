#ifndef _WIN32K_TIMER_H
#define _WIN32K_TIMER_H

typedef struct _MSG_TIMER_ENTRY{
   LIST_ENTRY     ListEntry;
   LARGE_INTEGER  Timeout;
   struct _USER_MESSAGE_QUEUE* MessageQueue;
   UINT           Period;
   MSG            Msg;
} MSG_TIMER_ENTRY, *PMSG_TIMER_ENTRY;

NTSTATUS FASTCALL InitTimerImpl(VOID);
VOID FASTCALL RemoveTimersThread(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL RemoveTimersWindow(HWND hWnd);
PMSG_TIMER_ENTRY FASTCALL IntRemoveTimer(HWND hWnd, UINT_PTR IDEvent, BOOL SysTimer);
UINT_PTR FASTCALL IntSetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc, BOOL SystemTimer);

#endif /* _WIN32K_TIMER_H */
