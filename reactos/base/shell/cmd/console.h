
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
VOID ConErrChar (TCHAR);
VOID ConPrintfV(DWORD, LPTSTR, va_list);

VOID ConPuts(DWORD nStdHandle, LPTSTR szText);
VOID ConPrintf(DWORD nStdHandle, LPTSTR szFormat, ...);
VOID ConResPuts(DWORD nStdHandle, UINT resID);
VOID ConResPrintf(DWORD nStdHandle, UINT resID, ...);
VOID ConFormatMessage(DWORD nStdHandle, DWORD MessageId, ...);

#define ConOutPuts(szStr) \
    ConPuts(STD_OUTPUT_HANDLE, (szStr))

#define ConErrPuts(szStr) \
    ConPuts(STD_ERROR_HANDLE, (szStr))

#define ConOutResPuts(uID) \
    ConResPuts(STD_OUTPUT_HANDLE, (uID))

#define ConErrResPuts(uID) \
    ConResPuts(STD_ERROR_HANDLE, (uID))

#define ConOutPrintf(szStr, ...) \
    ConPrintf(STD_OUTPUT_HANDLE, (szStr), ##__VA_ARGS__)

#define ConErrPrintf(szStr, ...) \
    ConPrintf(STD_ERROR_HANDLE, (szStr), ##__VA_ARGS__)

#define ConOutResPrintf(uID, ...) \
    ConResPrintf(STD_OUTPUT_HANDLE, (uID), ##__VA_ARGS__)

#define ConErrResPrintf(uID, ...) \
    ConResPrintf(STD_ERROR_HANDLE, (uID), ##__VA_ARGS__)

#define ConOutFormatMessage(MessageId, ...) \
    ConFormatMessage(STD_OUTPUT_HANDLE, (MessageId), ##__VA_ARGS__)

#define ConErrFormatMessage(MessageId, ...) \
    ConFormatMessage(STD_ERROR_HANDLE, (MessageId), ##__VA_ARGS__)


BOOL ConPrintfVPaging(DWORD nStdHandle, BOOL, LPTSTR, va_list);
BOOL ConOutPrintfPaging (BOOL NewPage, LPTSTR, ...);
VOID ConOutResPaging(BOOL NewPage, UINT resID);

SHORT GetCursorX  (VOID);
SHORT GetCursorY  (VOID);
VOID  GetCursorXY (PSHORT, PSHORT);
VOID  SetCursorXY (SHORT, SHORT);

VOID GetScreenSize (PSHORT, PSHORT);
VOID SetCursorType (BOOL, BOOL);


#ifdef INCLUDE_CMD_COLOR
BOOL ConGetDefaultAttributes(PWORD pwDefAttr);
#endif


BOOL ConSetTitle(IN LPCTSTR lpConsoleTitle);

#ifdef INCLUDE_CMD_BEEP
VOID ConRingBell(HANDLE hOutput);
#endif

#ifdef INCLUDE_CMD_CLS
VOID ConClearScreen(HANDLE hOutput);
#endif

#ifdef INCLUDE_CMD_COLOR
BOOL ConSetScreenColor(HANDLE hOutput, WORD wColor, BOOL bFill);
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
