/*
 * Message queues definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_QUEUE_H
#define __WINE_QUEUE_H


#include <windows.h>
#include <user32/win.h>

/***** Window hooks *****/

#define WH_MIN		    (-1)
#define WH_MAX              12


#define WH_MINHOOK          WH_MIN
#define WH_MAXHOOK          WH_MAX
#define WH_NB_HOOKS         (WH_MAXHOOK-WH_MINHOOK+1)



  /* Message as stored in the queue (contains the extraInfo field) */
typedef struct tagQMSG
{
    DWORD   extraInfo;  /* Only in 3.1 */
    MSG   msg;
} QMSG;

typedef struct
{
  LRESULT   lResult;
  WINBOOL    bPending;
} QSMCTRL;

#pragma pack(1)

typedef struct tagMESSAGEQUEUE
{
  HQUEUE    next;                   /* 00 Next queue */
  HANDLE    hTask;                  /* 02 hTask owning the queue */
  WORD      msgSize;                /* 04 Size of messages in the queue */
  WORD      msgCount;               /* 06 Number of waiting messages */
  WORD      nextMessage;            /* 08 Next message to be retrieved */
  WORD      nextFreeMessage;        /* 0a Next available slot in the queue */
  WORD      queueSize;              /* 0c Size of the queue */
  DWORD     GetMessageTimeVal;      /* 0e Value for GetMessageTime */
  DWORD     GetMessagePosVal;       /* 12 Value for GetMessagePos */
  HQUEUE    self;                   /* 16 Handle to self (was: reserved) */
  DWORD     GetMessageExtraInfoVal; /* 18 Value for GetMessageExtraInfo */
  WORD      wParamHigh;             /* 1c High word of wParam (was: reserved)*/
  LPARAM    lParam;		     /* 1e Next 4 values set by SendMessage */
  WPARAM    wParam;                 /* 22 */
  UINT      msg;                    /* 24 */
  HWND      hWnd;                   /* 26 */
  DWORD     SendMessageReturn;      /* 28 Return value for SendMessage */
  WORD      wPostQMsg;              /* 2c PostQuitMessage flag */
  WORD      wExitCode;              /* 2e PostQuitMessage exit code */
  WORD      flags;                  /* 30 Queue flags */
  QSMCTRL*  smResultInit;           /* 32 1st LRESULT ptr - was: reserved */
  WORD      wWinVersion;            /* 36 Expected Windows version */
  HQUEUE    InSendMessageHandle;    /* 38 Queue of task that sent a message */
  HANDLE     hSendingTask;           /* 3a Handle of task that sent a message */
  HANDLE     hPrevSendingTask;       /* 3c Handle of previous sender */
  WORD      wPaintCount;            /* 3e Number of WM_PAINT needed */
  WORD      wTimerCount;            /* 40 Number of timers for this task */
  WORD      changeBits;             /* 42 Changed wake-up bits */
  WORD      wakeBits;               /* 44 Queue wake-up bits */
  WORD      wakeMask;               /* 46 Queue wake-up mask */
  QSMCTRL*  smResultCurrent;        /* 48 ptrs to SendMessage() LRESULT - point to */
  WORD      SendMsgReturnPtr[1];    /*    values on stack */
  HANDLE    hCurHook;               /* 4e Current hook */
  HANDLE    hooks[WH_NB_HOOKS];     /* 50 Task hooks list */
  QSMCTRL*  smResult;               /* 6c 3rd LRESULT ptr - was: reserved */
  QMSG      messages[1];            /* 70 Queue messages */
} MESSAGEQUEUE;

#pragma pack(4)

/* Extra (undocumented) queue wake bits - see "Undoc. Windows" */
#define QS_SMRESULT      0x8000  /* Queue has a SendMessage() result */
#define QS_SMPARAMSFREE  0x4000  /* SendMessage() parameters are available */

/* Queue flags */
#define QUEUE_SM_ASCII     0x0002  /* Currently sent message is Win32 */
#define QUEUE_SM_UNICODE   0x0004  /* Currently sent message is Unicode */

void QUEUE_DumpQueue( HQUEUE hQueue );
void QUEUE_WalkQueues(void);
WINBOOL QUEUE_IsExitingQueue( HQUEUE hQueue );
void QUEUE_SetExitingQueue( HQUEUE hQueue );
MESSAGEQUEUE *QUEUE_GetSysQueue(void);
void QUEUE_Signal( HANDLE hTask );
void QUEUE_SetWakeBit( MESSAGEQUEUE *queue, WORD bit );
void QUEUE_ClearWakeBit( MESSAGEQUEUE *queue, WORD bit );
void QUEUE_ReceiveMessage( MESSAGEQUEUE *queue );
void QUEUE_WaitBits( WORD bits );
void QUEUE_IncPaintCount( HQUEUE hQueue );
void QUEUE_DecPaintCount( HQUEUE hQueue );
void QUEUE_IncTimerCount( HQUEUE hQueue );
void QUEUE_DecTimerCount( HQUEUE hQueue );
WINBOOL QUEUE_CreateSysMsgQueue( int size );
WINBOOL QUEUE_DeleteMsgQueue( HQUEUE hQueue );
HANDLE QUEUE_GetQueueTask( HQUEUE hQueue );
WINBOOL QUEUE_AddMsg( HQUEUE hQueue, MSG * msg, DWORD extraInfo );
int QUEUE_FindMsg( MESSAGEQUEUE * msgQueue, HWND hwnd,
                          int first, int last );
void QUEUE_RemoveMsg( MESSAGEQUEUE * msgQueue, int pos );
void QUEUE_FlushMessages(HQUEUE);
void hardware_event( WORD message, WORD wParam, LONG lParam,
			    int xPos, int yPos, DWORD time, DWORD extraInfo );

HQUEUE  InitThreadInput( WORD unknown, WORD flags );

HQUEUE  SetThreadQueue( DWORD thread, HQUEUE hQueue );
HQUEUE  GetThreadQueue( DWORD thread );
VOID  SetFastQueue( DWORD thread, HANDLE hQueue );
HANDLE  GetFastQueue( void );

#endif  /* __WINE_QUEUE_H */
