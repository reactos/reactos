/*
 * Message Logging functions
 */

#ifndef __WINE_SPY_H
#define __WINE_SPY_H



#define SPY_DISPATCHMESSAGE     0x0101
#define SPY_SENDMESSAGE         0x0103
#define SPY_DEFWNDPROC          0x0105


#define SPY_RESULT_OK           0x0001
#define SPY_RESULT_INVALIDHWND  0x0003
#define SPY_RESULT_DEFWND       0x0005


extern const char *SPY_GetMsgName( UINT msg );
extern void SPY_EnterMessage( INT iFlag, HWND hwnd, UINT msg,
                              WPARAM wParam, LPARAM lParam );
extern void SPY_ExitMessage( INT iFlag, HWND hwnd, UINT msg,
                             LRESULT lReturn );
extern int SPY_Init(void);

#endif /* __WINE_SPY_H */
