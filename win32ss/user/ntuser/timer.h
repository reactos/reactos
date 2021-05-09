#pragma once

typedef struct _TIMER
{
  HEAD           head;
  LIST_ENTRY     ptmrList;
  PTHREADINFO    pti;
  PWND           pWnd;         // hWnd
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

#define ID_EVENT_SYSTIMER_MOUSEHOVER     ID_TME_TIMER
#define ID_EVENT_SYSTIMER_FLASHWIN       (0xFFF8)
#define ID_EVENT_SYSTIMER_TRACKWIN       (0xFFF7)
#define ID_EVENT_SYSTIMER_ANIMATEDFADE   (0xFFF6)
#define ID_EVENT_SYSTIMER_INVALIDATEDCES (0xFFF5)

extern PKTIMER MasterTimer;

NTSTATUS NTAPI InitTimerImpl(VOID);
BOOL FASTCALL DestroyTimersForThread(PTHREADINFO pti);
BOOL FASTCALL DestroyTimersForWindow(PTHREADINFO pti, PWND Window);
BOOL FASTCALL IntKillTimer(PWND Window, UINT_PTR IDEvent, BOOL SystemTimer);
UINT_PTR FASTCALL IntSetTimer(PWND Window, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, INT Type);
PTIMER FASTCALL FindSystemTimer(PMSG);
BOOL FASTCALL ValidateTimerCallback(PTHREADINFO,LPARAM);
VOID CALLBACK SystemTimerProc(HWND,UINT,UINT_PTR,DWORD);
UINT_PTR FASTCALL SystemTimerSet(PWND,UINT_PTR,UINT,TIMERPROC);
BOOL FASTCALL PostTimerMessages(PWND);
VOID FASTCALL ProcessTimers(VOID);
VOID FASTCALL StartTheTimers(VOID);
