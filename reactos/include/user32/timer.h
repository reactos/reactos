
#include <user32/win.h>
#include <user32/queue.h>

typedef struct tagTIMER
{
    HWND           hwnd;
    HQUEUE         hq;
    UINT           msg;  /* WM_TIMER or WM_SYSTIMER */
    UINT           id;
    UINT           timeout;
    struct tagTIMER *next;
    DWORD            expires;  /* Next expiration, or 0 if already expired */
    TIMERPROC      proc;
} TIMER;

#define NB_TIMERS            34
#define NB_RESERVED_TIMERS    2  /* for SetSystemTimer */

#define WM_SYSTIMER	    0x0118



void TIMER_InsertTimer( TIMER * pTimer );
void TIMER_RemoveTimer( TIMER * pTimer );
void TIMER_ClearTimer( TIMER * pTimer );
void TIMER_SwitchQueue( HQUEUE old, HQUEUE newQ );
void TIMER_RemoveWindowTimers( HWND hwnd );
void TIMER_RemoveQueueTimers( HQUEUE hqueue );
void TIMER_RestartTimer( TIMER * pTimer, DWORD curTime );
LONG TIMER_GetNextExpiration(void);
void TIMER_ExpireTimers(void);
WINBOOL TIMER_GetTimerMsg( MSG *msg, HWND hwnd,
                          HQUEUE hQueue,WINBOOL remove );

UINT TIMER_SetTimer( HWND hwnd, UINT id, UINT timeout,
                              TIMERPROC proc, WINBOOL sys );
WINBOOL TIMER_KillTimer( HWND hwnd, UINT id,WINBOOL sys );
