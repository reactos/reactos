/*
 * Message definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_MESSAGE_H
#define __WINE_MESSAGE_H

#include <user32/win.h>
#include <user32/queue.h>

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

LRESULT MSG_SendMessage( HQUEUE hDestQueue, HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam, WORD flags );


void joySendMessages(void);

#endif  /* __WINE_MESSAGE_H */
