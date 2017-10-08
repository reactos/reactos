
#pragma once

/* Cache codepage for text streams */
extern UINT InputCodePage;
extern UINT OutputCodePage;

/* Global console Screen and Pager */
extern CON_SCREEN StdOutScreen;
extern CON_PAGER  StdOutPager;

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


VOID ConOutChar(TCHAR);
VOID ConErrChar(TCHAR);

VOID __cdecl ConFormatMessage(PCON_STREAM Stream, DWORD MessageId, ...);

#define ConOutPuts(szStr) \
    ConPuts(StdOut, (szStr))

#define ConErrPuts(szStr) \
    ConPuts(StdErr, (szStr))

#define ConOutResPuts(uID) \
    ConResPuts(StdOut, (uID))

#define ConErrResPuts(uID) \
    ConResPuts(StdErr, (uID))

#define ConOutPrintf(szStr, ...) \
    ConPrintf(StdOut, (szStr), ##__VA_ARGS__)

#define ConErrPrintf(szStr, ...) \
    ConPrintf(StdErr, (szStr), ##__VA_ARGS__)

#define ConOutResPrintf(uID, ...) \
    ConResPrintf(StdOut, (uID), ##__VA_ARGS__)

#define ConErrResPrintf(uID, ...) \
    ConResPrintf(StdErr, (uID), ##__VA_ARGS__)

#define ConOutFormatMessage(MessageId, ...) \
    ConFormatMessage(StdOut, (MessageId), ##__VA_ARGS__)

#define ConErrFormatMessage(MessageId, ...) \
    ConFormatMessage(StdErr, (MessageId), ##__VA_ARGS__)


BOOL ConPrintfVPaging(PCON_PAGER Pager, BOOL StartPaging, LPTSTR szFormat, va_list arg_ptr);
BOOL __cdecl ConOutPrintfPaging(BOOL StartPaging, LPTSTR szFormat, ...);
VOID ConOutResPaging(BOOL StartPaging, UINT resID);

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

#ifdef INCLUDE_CMD_COLOR
BOOL ConSetScreenColor(HANDLE hOutput, WORD wColor, BOOL bFill);
#endif

// TCHAR  cgetchar (VOID);
// BOOL   CheckCtrlBreak (INT);

// #define PROMPT_NO    0
// #define PROMPT_YES   1
// #define PROMPT_ALL   2
// #define PROMPT_BREAK 3

// INT FilePromptYN (UINT);
// INT FilePromptYNA (UINT);
