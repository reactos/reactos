
#ifndef __WINE_MSG_H
#define __WINE_MSG_H



#define WM_SYSTIMER	    0x0118

#define WH_HARDWARE	    8


#include <user32/win.h>
#include <user32/queue.h>
#include <user32/sysmetr.h>
#include <user32/debug.h>

typedef struct
{
    HWND    hWnd;
    UINT    wMessage;
    WPARAM  wParam;
    LPARAM    lParam;
} HARDWAREHOOKSTRUCT, *LPHARDWAREHOOKSTRUCT;



#define MAX_QUEUE_SIZE 256

extern WINBOOL MouseButtonsStates[3];
extern WINBOOL AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
extern BYTE QueueKeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];

#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK

typedef enum { SYSQ_MSG_ABANDON, SYSQ_MSG_SKIP, 
               SYSQ_MSG_ACCEPT, SYSQ_MSG_CONTINUE } SYSQ_STATUS;

extern MESSAGEQUEUE *pCursorQueue;			 /* queue.c */
extern MESSAGEQUEUE *pActiveQueue;


/* message.c */
WINBOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd,
                                      HWND hwndOwner, WPARAM code,
                                      WORD flags, WINBOOL sendIdle );

WINBOOL MSG_PeekMessage( LPMSG msg, HWND hwnd, WORD first, WORD last,
                               WORD flags, WINBOOL peek );

void  MSG_CallWndProcHook( LPMSG pmsg, WINBOOL bUnicode );

LRESULT MSG_SendMessage(WND *Wnd, UINT msg,
                                WPARAM wParam, LPARAM lParam);


LRESULT MSG_SendMessageInterTask(  HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam, WINBOOL bUnicode);

WINBOOL MSG_DoTranslateMessage( UINT message, HWND hwnd,
                                      WPARAM wParam, LPARAM lParam );

void joySendMessages(void);




#endif