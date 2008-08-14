#ifndef _WIN32K_TIMER_H
#define _WIN32K_TIMER_H

typedef struct _TIMER
{
  LIST_ENTRY     ptmrList;
  PW32THREADINFO pti;
  PWINDOW_OBJECT pWnd;         // hWnd
  UINT_PTR       nID;          // Specifies a nonzero timer identifier.
  INT            cmsCountdown; // uElapse
  INT            cmsRate;      // uElapse
  FLONG          flags;
  TIMERPROC      pfn;          // lpTimerFunc
} TIMER, *PTIMER;

NTSTATUS FASTCALL InitTimerImpl(VOID);
BOOL FASTCALL IntKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer);
UINT_PTR FASTCALL IntSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer);

#endif /* _WIN32K_TIMER_H */
