#ifndef __WINE_CARET_H
#define __WINE_CARET_H


#define DCX_USESTYLE         0x00010000

typedef struct
{
    HWND     hwnd;
    UINT     hidden;
    WINBOOL     on;
    INT      x;
    INT      y;
    INT      width;
    INT      height;
    HBRUSH   hBrush;
    UINT     timeout;
    UINT     timerid;
} CARET;

typedef enum
{
    CARET_OFF = 0,
    CARET_ON,
    CARET_TOGGLE,
} DISPLAY_CARET;

HWND CARET_GetHwnd(void);
void CARET_GetRect(LPRECT lprc);  /* windows/caret.c */
void CARET_DisplayCaret( DISPLAY_CARET status );
//VOID CALLBACK CARET_Callback( HWND hwnd, UINT msg, UINT id, DWORD ctime);
void CARET_SetTimer(void);
void CARET_ResetTimer(void);
void CARET_KillTimer(void);


#endif