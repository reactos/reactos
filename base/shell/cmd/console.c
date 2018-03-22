/*
 *  CONSOLE.C - console input/output functions.
 *
 *
 *  History:
 *
 *    20-Jan-1999 (Eric Kohl)
 *        started
 *
 *    03-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 *
 *    01-Jul-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        Added ConPrintfPaging and ConOutPrintfPaging
 *
 *    02-Feb-2007 (Paolo Devoti <devotip at gmail.com>)
 *        Fixed ConPrintfPaging
 */

#include "precomp.h"

#define OUTPUT_BUFFER_SIZE  4096

/* Cache codepage for text streams */
UINT InputCodePage;
UINT OutputCodePage;

/* Global console Screen and Pager */
CON_SCREEN StdOutScreen = INIT_CON_SCREEN(StdOut);
CON_PAGER  StdOutPager  = INIT_CON_PAGER(&StdOutScreen);



/********************* Console STREAM IN utility functions ********************/

VOID ConInDisable(VOID)
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;

    GetConsoleMode(hInput, &dwMode);
    dwMode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);
}

VOID ConInEnable(VOID)
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;

    GetConsoleMode(hInput, &dwMode);
    dwMode |= ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);
}

VOID ConInFlush(VOID)
{
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

VOID ConInKey(PINPUT_RECORD lpBuffer)
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD  dwRead;

    if (hInput == INVALID_HANDLE_VALUE)
        WARN ("Invalid input handle!!!\n");

    do
    {
        ReadConsoleInput(hInput, lpBuffer, 1, &dwRead);
        if (lpBuffer->EventType == KEY_EVENT &&
            lpBuffer->Event.KeyEvent.bKeyDown)
        {
            break;
        }
    }
    while (TRUE);
}

VOID ConInString(LPTSTR lpInput, DWORD dwLength)
{
    DWORD dwOldMode;
    DWORD dwRead = 0;
    HANDLE hFile;

    LPTSTR p;
    PCHAR pBuf;

#ifdef _UNICODE
    pBuf = (PCHAR)cmd_alloc(dwLength - 1);
#else
    pBuf = lpInput;
#endif
    ZeroMemory(lpInput, dwLength * sizeof(TCHAR));
    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    ReadFile(hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

#ifdef _UNICODE
    MultiByteToWideChar(InputCodePage, 0, pBuf, dwRead, lpInput, dwLength - 1);
    cmd_free(pBuf);
#endif
    for (p = lpInput; *p; p++)
    {
        if (*p == _T('\x0d'))
        {
            *p = _T('\0');
            break;
        }
    }

    SetConsoleMode(hFile, dwOldMode);
}



/******************** Console STREAM OUT utility functions ********************/

VOID ConOutChar(TCHAR c)
{
    ConWrite(StdOut, &c, 1);
}

VOID ConErrChar(TCHAR c)
{
    ConWrite(StdErr, &c, 1);
}

VOID __cdecl ConFormatMessage(PCON_STREAM Stream, DWORD MessageId, ...)
{
    INT Len;
    va_list arg_ptr;

    va_start(arg_ptr, MessageId);
    Len = ConMsgPrintfV(Stream,
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        MessageId,
                        LANG_USER_DEFAULT,
                        &arg_ptr);
    va_end(arg_ptr);

    if (Len <= 0)
        ConResPrintf(Stream, STRING_CONSOLE_ERROR, MessageId);
}



/************************** Console PAGER functions ***************************/

BOOL ConPrintfVPaging(PCON_PAGER Pager, BOOL StartPaging, LPTSTR szFormat, va_list arg_ptr)
{
    // INT len;
    TCHAR szOut[OUTPUT_BUFFER_SIZE];

    /* Return if no string has been given */
    if (szFormat == NULL)
        return TRUE;

    /*len =*/ _vstprintf(szOut, szFormat, arg_ptr);

    // return ConPutsPaging(Pager, PagePrompt, StartPaging, szOut);
    return ConWritePaging(Pager, PagePrompt, StartPaging,
                          szOut, wcslen(szOut));
}

BOOL __cdecl ConOutPrintfPaging(BOOL StartPaging, LPTSTR szFormat, ...)
{
    BOOL bRet;
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    bRet = ConPrintfVPaging(&StdOutPager, StartPaging, szFormat, arg_ptr);
    va_end(arg_ptr);
    return bRet;
}

VOID ConOutResPaging(BOOL StartPaging, UINT resID)
{
    ConResPaging(&StdOutPager, PagePrompt, StartPaging, resID);
}



/************************** Console SCREEN functions **************************/

VOID SetCursorXY(SHORT x, SHORT y)
{
    COORD coPos;

    coPos.X = x;
    coPos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coPos);
}

VOID GetCursorXY(PSHORT x, PSHORT y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    *x = csbi.dwCursorPosition.X;
    *y = csbi.dwCursorPosition.Y;
}

SHORT GetCursorX(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.dwCursorPosition.X;
}

SHORT GetCursorY(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.dwCursorPosition.Y;
}

VOID SetCursorType(BOOL bInsert, BOOL bVisible)
{
    CONSOLE_CURSOR_INFO cci;

    cci.dwSize = bInsert ? 10 : 99;
    cci.bVisible = bVisible;

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
}

VOID GetScreenSize(PSHORT maxx, PSHORT maxy)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        csbi.dwSize.X = 80;
        csbi.dwSize.Y = 25;
    }

    if (maxx) *maxx = csbi.dwSize.X;
    if (maxy) *maxy = csbi.dwSize.Y;
}




#ifdef INCLUDE_CMD_COLOR

BOOL ConGetDefaultAttributes(PWORD pwDefAttr)
{
    BOOL Success;
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    /* Do not modify *pwDefAttr if we fail, in which case use default attributes */

    hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                          OPEN_EXISTING, 0, NULL);
    if (hConsole == INVALID_HANDLE_VALUE)
        return FALSE; // No default console

    Success = GetConsoleScreenBufferInfo(hConsole, &csbi);
    if (Success)
        *pwDefAttr = csbi.wAttributes;

    CloseHandle(hConsole);
    return Success;
}

#endif


BOOL ConSetTitle(IN LPCTSTR lpConsoleTitle)
{
    /* Now really set the console title */
    return SetConsoleTitle(lpConsoleTitle);
}

#ifdef INCLUDE_CMD_BEEP
VOID ConRingBell(HANDLE hOutput)
{
#if 0
    /* Emit an error beep sound */
    if (IsConsoleHandle(hOutput))
        Beep(800, 200);
    else if (IsTTYHandle(hOutput))
        ConOutPuts(_T("\a")); // BEL character 0x07
    else
#endif
        MessageBeep(-1);
}
#endif

#ifdef INCLUDE_CMD_COLOR
BOOL ConSetScreenColor(HANDLE hOutput, WORD wColor, BOOL bFill)
{
    DWORD dwWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coPos;

    /* Foreground and Background colors can't be the same */
    if ((wColor & 0x0F) == (wColor & 0xF0) >> 4)
        return FALSE;

    /* Fill the whole background if needed */
    if (bFill)
    {
        GetConsoleScreenBufferInfo(hOutput, &csbi);

        coPos.X = 0;
        coPos.Y = 0;
        FillConsoleOutputAttribute(hOutput,
                                   wColor & 0x00FF,
                                   csbi.dwSize.X * csbi.dwSize.Y,
                                   coPos,
                                   &dwWritten);
    }

    /* Set the text attribute */
    SetConsoleTextAttribute(hOutput, wColor & 0x00FF);
    return TRUE;
}
#endif

/* EOF */
