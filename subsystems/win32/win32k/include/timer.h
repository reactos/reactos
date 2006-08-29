#ifndef _WIN32K_TIMER_H
#define _WIN32K_TIMER_H

NTSTATUS FASTCALL InitTimerImpl(VOID);
BOOL FASTCALL IntKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer);
UINT_PTR FASTCALL IntSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer);

#endif /* _WIN32K_TIMER_H */
