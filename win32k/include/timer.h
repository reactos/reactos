#ifndef _WIN32K_TIMER_H
#define _WIN32K_TIMER_H

#include <include/window.h>

typedef struct _TIMER_ENTRY{
   LIST_ENTRY     ListEntry;
   LARGE_INTEGER  ExpiryTime;
   PWINDOW_OBJECT  Wnd;
   PUSER_MESSAGE_QUEUE Queue;
   UINT_PTR       IDEvent;
   UINT           Period;
   TIMERPROC      TimerFunc;
   UINT           Message;
} TIMER_ENTRY, *PTIMER_ENTRY;






NTSTATUS FASTCALL InitTimerImpl(VOID);

BOOL FASTCALL 
UserKillTimer(PWINDOW_OBJECT Wnd, UINT_PTR IDEvent, BOOL SystemTimer);

UINT_PTR FASTCALL
UserSetTimer(
   PWINDOW_OBJECT Wnd,
   UINT_PTR IDEvent,
   UINT Elapse,
   TIMERPROC TimerFunc,
   BOOL SystemTimer
   );

#endif /* _WIN32K_TIMER_H */
