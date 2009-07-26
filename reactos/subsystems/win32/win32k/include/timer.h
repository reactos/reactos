#ifndef _WIN32K_TIMER_H
#define _WIN32K_TIMER_H

typedef struct _TIMER
{
  LIST_ENTRY     ptmrList;
  PTHREADINFO    pti;
  PWINDOW_OBJECT pWnd;         // hWnd
  UINT_PTR       nID;          // Specifies a nonzero timer identifier.
  INT            cmsCountdown; // uElapse
  INT            cmsRate;      // uElapse
  FLONG          flags;
  TIMERPROC      pfn;          // lpTimerFunc
} TIMER, *PTIMER;

//
// Timer structure flags.
//
#define TMRF_READY   0x0001
#define TMRF_SYSTEM  0x0002
#define TMRF_RIT     0x0004
#define TMRF_INIT    0x0008
#define TMRF_ONESHOT 0x0010
#define TMRF_WAITING 0x0020
#define TMRF_TIFROMWND 0x0040

extern PKTIMER MasterTimer;

NTSTATUS FASTCALL InitTimerImpl(VOID);
BOOL FASTCALL IntKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer);
UINT_PTR FASTCALL IntSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer);
PTIMER FASTCALL FindSystemTimer(PMSG);
BOOL FASTCALL ValidateTimerCallback(PTHREADINFO,PWINDOW_OBJECT,WPARAM,LPARAM);
VOID CALLBACK SystemTimerProc(HWND,UINT,UINT_PTR,DWORD);
UINT_PTR FASTCALL SetSystemTimer(PWINDOW_OBJECT,UINT_PTR,UINT,TIMERPROC);
BOOL FASTCALL PostTimerMessages(PWINDOW_OBJECT);
VOID FASTCALL ProcessTimers(VOID);
VOID FASTCALL StartTheTimers(VOID);

#endif /* _WIN32K_TIMER_H */
