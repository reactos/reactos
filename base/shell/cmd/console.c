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


UINT InputCodePage;
UINT OutputCodePage;


BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

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


VOID ConInFlush (VOID)
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
        if ((lpBuffer->EventType == KEY_EVENT) &&
            (lpBuffer->Event.KeyEvent.bKeyDown == TRUE))
            break;
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

static VOID ConWrite(TCHAR *str, DWORD len, DWORD nStdHandle)
{
    DWORD dwWritten;
    HANDLE hOutput = GetStdHandle(nStdHandle);
    PVOID p;

    /* Check whether we are writing to a console and if so, write to it */
    if (IsConsoleHandle(hOutput))
    {
        if (WriteConsole(hOutput, str, len, &dwWritten, NULL))
            return;
    }

    /* We're writing to a file or pipe instead of the console. Convert the
     * string from TCHARs to the desired output format, if the two differ */
    if (bUnicodeOutput)
    {
#ifndef _UNICODE
        WCHAR *buffer = cmd_alloc(len * sizeof(WCHAR));
        if (!buffer)
        {
            error_out_of_memory();
            return;
        }
        len = (DWORD)MultiByteToWideChar(OutputCodePage, 0, str, (INT)len, buffer, (INT)len);
        str = (PVOID)buffer;
#endif
        /*
         * Find any newline character in the buffer,
         * send the part BEFORE the newline, then send
         * a carriage-return + newline, and then send
         * the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (str && *(PWCHAR)str && len > 0)
        {
            p = wcspbrk((PWCHAR)str, L"\r\n");
            if (p)
            {
                len -= ((PWCHAR)p - (PWCHAR)str) + 1;
                WriteFile(hOutput, str, ((PWCHAR)p - (PWCHAR)str) * sizeof(WCHAR), &dwWritten, NULL);
                WriteFile(hOutput, L"\r\n", 2 * sizeof(WCHAR), &dwWritten, NULL);
                str = (PVOID)((PWCHAR)p + 1);
            }
            else
            {
                WriteFile(hOutput, str, len * sizeof(WCHAR), &dwWritten, NULL);
                break;
            }
        }

        // WriteFile(hOutput, str, len * sizeof(WCHAR), &dwWritten, NULL);
#ifndef _UNICODE
        cmd_free(buffer);
#endif
    }
    else
    {
#ifdef _UNICODE
        CHAR *buffer = cmd_alloc(len * MB_LEN_MAX * sizeof(CHAR));
        if (!buffer)
        {
            error_out_of_memory();
            return;
        }
        len = WideCharToMultiByte(OutputCodePage, 0, str, len, buffer, len * MB_LEN_MAX, NULL, NULL);
        str = (PVOID)buffer;
#endif
        /*
         * Find any newline character in the buffer,
         * send the part BEFORE the newline, then send
         * a carriage-return + newline, and then send
         * the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (str && *(PCHAR)str && len > 0)
        {
            p = strpbrk((PCHAR)str, "\r\n");
            if (p)
            {
                len -= ((PCHAR)p - (PCHAR)str) + 1;
                WriteFile(hOutput, str, ((PCHAR)p - (PCHAR)str), &dwWritten, NULL);
                WriteFile(hOutput, "\r\n", 2, &dwWritten, NULL);
                str = (PVOID)((PCHAR)p + 1);
            }
            else
            {
                WriteFile(hOutput, str, len, &dwWritten, NULL);
                break;
            }
        }

        // WriteFile(hOutput, str, len, &dwWritten, NULL);
#ifdef _UNICODE
        cmd_free(buffer);
#endif
    }
}

VOID ConOutChar(TCHAR c)
{
    ConWrite(&c, 1, STD_OUTPUT_HANDLE);
}

VOID ConPuts(LPTSTR szText, DWORD nStdHandle)
{
    ConWrite(szText, (DWORD)_tcslen(szText), nStdHandle);
}

VOID ConOutResPaging(BOOL NewPage, UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    LoadString(CMD_ModuleHandle, resID, szMsg, ARRAYSIZE(szMsg));
    ConOutPrintfPaging(NewPage, szMsg);
}

VOID ConOutResPuts(UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    LoadString(CMD_ModuleHandle, resID, szMsg, ARRAYSIZE(szMsg));
    ConPuts(szMsg, STD_OUTPUT_HANDLE);
}

VOID ConOutPuts(LPTSTR szText)
{
    ConPuts(szText, STD_OUTPUT_HANDLE);
}


VOID ConPrintf(LPTSTR szFormat, va_list arg_ptr, DWORD nStdHandle)
{
    TCHAR szOut[OUTPUT_BUFFER_SIZE];
    DWORD len;

    len = (DWORD)_vstprintf(szOut, szFormat, arg_ptr);
    ConWrite(szOut, len, nStdHandle);
}

INT ConPrintfPaging(BOOL NewPage, LPTSTR szFormat, va_list arg_ptr, DWORD nStdHandle)
{
    INT len;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    TCHAR szOut[OUTPUT_BUFFER_SIZE];
    DWORD dwWritten;
    HANDLE hOutput = GetStdHandle(nStdHandle);

    /* used to count number of lines since last pause */
    static int LineCount = 0;

    /* used to see how big the screen is */
    int ScreenLines = 0;

    /* chars since start of line */
    int CharSL;

    int from = 0, i = 0;

    if (NewPage == TRUE)
        LineCount = 0;

    /* rest LineCount and return if no string have been given */
    if (szFormat == NULL)
        return 0;

    /* Get the size of the visual screen that can be printed too */
    if (!IsConsoleHandle(hOutput) || !GetConsoleScreenBufferInfo(hOutput, &csbi))
    {
        /* We assume it's a file handle */
        ConPrintf(szFormat, arg_ptr, nStdHandle);
        return 0;
    }
    /* Subtract 2 to account for "press any key..." and for the blank line at the end of PagePrompt() */
    ScreenLines = (csbi.srWindow.Bottom  - csbi.srWindow.Top) - 4;
    CharSL = csbi.dwCursorPosition.X;

    /* Make sure they didn't make the screen to small */
    if (ScreenLines < 4)
    {
        ConPrintf(szFormat, arg_ptr, nStdHandle);
        return 0;
    }

    len = _vstprintf(szOut, szFormat, arg_ptr);

    while (i < len)
    {
        /* Search until the end of a line is reached */
        if (szOut[i++] != _T('\n') && ++CharSL < csbi.dwSize.X)
            continue;

        LineCount++;
        CharSL=0;

        if (LineCount >= ScreenLines)
        {
            WriteConsole(hOutput, &szOut[from], i-from, &dwWritten, NULL);
            from = i;

            if (PagePrompt() != PROMPT_YES)
            {
                return 1;
            }
            /* Reset the number of lines being printed */
            LineCount = 0;
        }
    }

    WriteConsole(hOutput, &szOut[from], i-from, &dwWritten, NULL);

    return 0;
}

VOID ConErrFormatMessage(DWORD MessageId, ...)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    DWORD ret;
    LPTSTR text;
    va_list arg_ptr;

    va_start(arg_ptr, MessageId);
    ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        MessageId,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &text,
                        0,
                        &arg_ptr);

    va_end(arg_ptr);
    if (ret > 0)
    {
        ConErrPuts(text);
        LocalFree(text);
    }
    else
    {
        LoadString(CMD_ModuleHandle, STRING_CONSOLE_ERROR, szMsg, ARRAYSIZE(szMsg));
        ConErrPrintf(szMsg);
    }
}

VOID ConOutFormatMessage(DWORD MessageId, ...)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    DWORD ret;
    LPTSTR text;
    va_list arg_ptr;

    va_start(arg_ptr, MessageId);
    ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        MessageId,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &text,
                        0,
                        &arg_ptr);

    va_end(arg_ptr);
    if (ret > 0)
    {
        ConErrPuts(text);
        LocalFree(text);
    }
    else
    {
        LoadString(CMD_ModuleHandle, STRING_CONSOLE_ERROR, szMsg, ARRAYSIZE(szMsg));
        ConErrPrintf(szMsg);
    }
}

VOID ConOutResPrintf(UINT resID, ...)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadString(CMD_ModuleHandle, resID, szMsg, ARRAYSIZE(szMsg));
    ConPrintf(szMsg, arg_ptr, STD_OUTPUT_HANDLE);
    va_end(arg_ptr);
}

VOID ConOutPrintf(LPTSTR szFormat, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    ConPrintf(szFormat, arg_ptr, STD_OUTPUT_HANDLE);
    va_end(arg_ptr);
}

INT ConOutPrintfPaging(BOOL NewPage, LPTSTR szFormat, ...)
{
    INT iReturn;
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    iReturn = ConPrintfPaging(NewPage, szFormat, arg_ptr, STD_OUTPUT_HANDLE);
    va_end(arg_ptr);
    return iReturn;
}

VOID ConErrChar(TCHAR c)
{
    ConWrite(&c, 1, STD_ERROR_HANDLE);
}


VOID ConErrResPuts(UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    LoadString(CMD_ModuleHandle, resID, szMsg, ARRAYSIZE(szMsg));
    ConPuts(szMsg, STD_ERROR_HANDLE);
}

VOID ConErrPuts(LPTSTR szText)
{
    ConPuts(szText, STD_ERROR_HANDLE);
}


VOID ConErrResPrintf(UINT resID, ...)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadString(CMD_ModuleHandle, resID, szMsg, ARRAYSIZE(szMsg));
    ConPrintf(szMsg, arg_ptr, STD_ERROR_HANDLE);
    va_end(arg_ptr);
}

VOID ConErrPrintf(LPTSTR szFormat, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    ConPrintf(szFormat, arg_ptr, STD_ERROR_HANDLE);
    va_end(arg_ptr);
}


VOID SetCursorXY(SHORT x, SHORT y)
{
    COORD coPos;

    coPos.X = x;
    coPos.Y = y;
    SetConsoleCursorPosition(GetStdHandle (STD_OUTPUT_HANDLE), coPos);
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

    SetConsoleCursorInfo(GetStdHandle (STD_OUTPUT_HANDLE), &cci);
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

/* EOF */
