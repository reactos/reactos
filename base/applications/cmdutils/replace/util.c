/*
 * PROJECT:     ReactOS Replace Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal helpers. See cmd/internal.c
 * COPYRIGHT:   Copyright Samuel Erdtman (samuel@erdtman.se)
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "replace.h"

BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */

/*
 * Helper function for getting the current path from drive
 * without changing the drive. Return code: 0 = ok, 1 = fail.
 * 'InPath' can have any size; if the two first letters are
 * not a drive with ':' it will get the current path on
 * the current drive exactly as GetCurrentDirectory() does.
 */
INT
GetRootPath(
    IN LPCTSTR InPath,
    OUT LPTSTR OutPath,
    IN INT size)
{
    if (InPath[0] && InPath[1] == _T(':'))
    {
        INT t = 0;

        if ((InPath[0] >= _T('0')) && (InPath[0] <= _T('9')))
        {
            t = (InPath[0] - _T('0')) + 28;
        }
        else if ((InPath[0] >= _T('a')) && (InPath[0] <= _T('z')))
        {
            t = (InPath[0] - _T('a')) + 1;
        }
        else if ((InPath[0] >= _T('A')) && (InPath[0] <= _T('Z')))
        {
            t = (InPath[0] - _T('A')) + 1;
        }

        return (_tgetdcwd(t, OutPath, size) == NULL);
    }

    /* Get current directory */
    return !GetCurrentDirectory(size, OutPath);
}

/*
 * Takes a path in and returns it with the correct case of the letters
 */
VOID GetPathCase( TCHAR * Path, TCHAR * OutPath)
{
    UINT i = 0;
    TCHAR TempPath[MAX_PATH];
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    _tcscpy(TempPath, _T(""));
    _tcscpy(OutPath, _T(""));

    for(i = 0; i < _tcslen(Path); i++)
    {
        if (Path[i] != _T('\\'))
        {
            _tcsncat(TempPath, &Path[i], 1);
            if (i != _tcslen(Path) - 1)
                continue;
        }
        /* Handle the base part of the path different.
           Because if you put it into findfirstfile, it will
           return your current folder */
        if (_tcslen(TempPath) == 2 && TempPath[1] == _T(':'))
        {
            _tcscat(OutPath, TempPath);
            _tcscat(OutPath, _T("\\"));
            _tcscat(TempPath, _T("\\"));
        }
        else
        {
            hFind = FindFirstFile(TempPath,&FindFileData);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                _tcscpy(OutPath, Path);
                return;
            }
            _tcscat(TempPath, _T("\\"));
            _tcscat(OutPath, FindFileData.cFileName);
            _tcscat(OutPath, _T("\\"));
            FindClose(hFind);
        }
    }
}

/*
 * Checks if a file exists (is accessible)
 */
BOOL IsExistingFile(IN LPCTSTR pszPath)
{
    DWORD attr = GetFileAttributes(pszPath);
    return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL IsExistingDirectory(IN LPCTSTR pszPath)
{
    DWORD attr = GetFileAttributes(pszPath);
    return ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

INT FilePromptYNA (UINT resID)
{
    TCHAR szMsg[RC_STRING_MAX_SIZE];
//  TCHAR cKey = 0;
//  LPTSTR szKeys = _T("yna");

    TCHAR szIn[10];
    LPTSTR p;

    if (resID != 0)
        ConOutResPrintf (resID);

    /* preliminary fix */
    ConInString(szIn, 10);

    _tcsupr (szIn);
    for (p = szIn; _istspace (*p); p++)
        ;

    LoadString(NULL, STRING_COPY_OPTION, szMsg, ARRAYSIZE(szMsg));

    if (_tcsncmp(p, &szMsg[0], 1) == 0)
        return PROMPT_YES;
    else if (_tcsncmp(p, &szMsg[1], 1) == 0)
        return PROMPT_NO;
    else if (_tcsncmp(p, &szMsg[2], 1) == 0)
        return PROMPT_ALL;
#if 0
    else if (*p == _T('\03'))
        return PROMPT_BREAK;
#endif

    return PROMPT_NO;

    /* unfinished solution */
#if 0
    RemoveBreakHandler();
    ConInDisable();

    do
    {
        ConInKey (&ir);
        cKey = _totlower (ir.Event.KeyEvent.uChar.AsciiChar);
        if (_tcschr (szKeys, cKey[0]) == NULL)
            cKey = 0;
    }
    while ((ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
           (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL));

    AddBreakHandler();
    ConInEnable();

    if ((ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
        ((ir.Event.KeyEvent.wVirtualKeyCode == _T('C')) &&
         (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
        return PROMPT_BREAK;

    return PROMPT_YES;
#endif
}

VOID ConInString(LPTSTR lpInput, DWORD dwLength)
{
    DWORD dwOldMode;
    DWORD dwRead = 0;
    HANDLE hFile;

    LPTSTR p;
    PCHAR pBuf;

#ifdef _UNICODE
    pBuf = (PCHAR)malloc(dwLength - 1);
#else
    pBuf = lpInput;
#endif
    ZeroMemory(lpInput, dwLength * sizeof(TCHAR));
    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    ReadFile(hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

#ifdef _UNICODE
    MultiByteToWideChar(GetConsoleCP(), 0, pBuf, dwRead, lpInput, dwLength - 1);
    free(pBuf);
#endif
    for (p = lpInput; *p; p++)
    {
        if (*p == _T('\r')) // Terminate at the carriage-return.
        {
            *p = _T('\0');
            break;
        }
    }

    SetConsoleMode(hFile, dwOldMode);
}

VOID msg_pause(VOID)
{
    ConOutResPuts(STRING_ERROR_D_PAUSEMSG);
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

VOID ConOutChar(TCHAR c)
{
    ConWrite(StdOut, &c, 1);
}

/*
 * get a character out-of-band and honor Ctrl-Break characters
 */
TCHAR
cgetchar (VOID)
{
    HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
    INPUT_RECORD irBuffer;
    DWORD  dwRead;

    do
    {
        ReadConsoleInput (hInput, &irBuffer, 1, &dwRead);
        if ((irBuffer.EventType == KEY_EVENT) &&
            (irBuffer.Event.KeyEvent.bKeyDown != FALSE))
        {
            if (irBuffer.Event.KeyEvent.dwControlKeyState &
                 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                if (irBuffer.Event.KeyEvent.wVirtualKeyCode == 'C')
                {
                    bCtrlBreak = TRUE;
                    break;
                }
            }
            else if ((irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
                     (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
                     (irBuffer.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL))
            {
                // Nothing to do
            }
            else
            {
                break;
            }
        }
    }
    while (TRUE);

#ifndef _UNICODE
    return irBuffer.Event.KeyEvent.uChar.AsciiChar;
#else
    return irBuffer.Event.KeyEvent.uChar.UnicodeChar;
#endif /* _UNICODE */
}
