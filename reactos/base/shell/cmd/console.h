
#pragma once

/* Cache codepage */
extern UINT InputCodePage;
extern UINT OutputCodePage;

// /* Global variables */
// extern BOOL   bCtrlBreak;
// extern BOOL   bIgnoreEcho;
// extern BOOL   bExit;

VOID ConInDummy (VOID);
VOID ConInDisable (VOID);
VOID ConInEnable (VOID);
VOID ConInFlush (VOID);
VOID ConInKey (PINPUT_RECORD);
VOID ConInString (LPTSTR, DWORD);

VOID ConOutChar (TCHAR);
VOID ConOutPuts (LPTSTR);
VOID ConPrintfV(DWORD, LPTSTR, va_list);
INT ConPrintfVPaging(DWORD nStdHandle, BOOL, LPTSTR, va_list);
VOID ConOutPrintf (LPTSTR, ...);
INT ConOutPrintfPaging (BOOL NewPage, LPTSTR, ...);
VOID ConErrChar (TCHAR);
VOID ConErrPuts (LPTSTR);
VOID ConErrPrintf (LPTSTR, ...);
VOID ConOutFormatMessage (DWORD MessageId, ...);
VOID ConErrFormatMessage (DWORD MessageId, ...);

SHORT GetCursorX  (VOID);
SHORT GetCursorY  (VOID);
VOID  GetCursorXY (PSHORT, PSHORT);
VOID  SetCursorXY (SHORT, SHORT);

VOID GetScreenSize (PSHORT, PSHORT);
VOID SetCursorType (BOOL, BOOL);

VOID ConOutResPuts (UINT resID);
VOID ConErrResPuts (UINT resID);
VOID ConOutResPrintf (UINT resID, ...);
VOID ConErrResPrintf (UINT resID, ...);
VOID ConOutResPaging(BOOL NewPage, UINT resID);


BOOL ConSetTitle(IN LPCTSTR lpConsoleTitle);

#ifdef INCLUDE_CMD_BEEP
VOID ConRingBell(HANDLE hOutput);
#endif

#ifdef INCLUDE_CMD_CLS
VOID ConClearScreen(HANDLE hOutput);
#endif

#ifdef INCLUDE_CMD_COLOR
BOOL ConSetScreenColor(WORD wColor, BOOL bFill);
#endif

// TCHAR  cgetchar (VOID);
// BOOL   CheckCtrlBreak (INT);

// #define PROMPT_NO    0
// #define PROMPT_YES   1
// #define PROMPT_ALL   2
// #define PROMPT_BREAK 3

// INT PagePrompt (VOID);
// INT FilePromptYN (UINT);
// INT FilePromptYNA (UINT);
