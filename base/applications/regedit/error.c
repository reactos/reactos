/*
 * Regedit errors, warnings, informations displaying
 *
 * Copyright (C) 2010 Adam Kachwalla <geekdundee@gmail.com>
 * Copyright (C) 2012 Hermès Bélusca - Maïto <hermes.belusca@sfr.fr>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

int ErrorMessageBox(HWND hWnd, LPCWSTR lpTitle, DWORD dwErrorCode, ...)
{
    int iRet = 0;
    LPWSTR lpMsgBuf = NULL;
    DWORD Status = 0;
    va_list args;

    va_start(args, dwErrorCode);

    Status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErrorCode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPWSTR)&lpMsgBuf,
                            0,
                            &args);

    va_end(args);

    iRet = MessageBoxW(hWnd, (Status && lpMsgBuf ? lpMsgBuf : L"Error displaying error message."), lpTitle, MB_OK | MB_ICONERROR);

    if (lpMsgBuf) LocalFree(lpMsgBuf);

    /* Return the MessageBoxW information */
    return iRet;
}

int InfoMessageBox(HWND hWnd, UINT uType, LPCWSTR lpTitle, LPCWSTR lpMessage, ...)
{
    int iRet = 0;
    LPWSTR lpMsgBuf = NULL;
    va_list args;

    va_start(args, lpMessage);

    if (lpMessage)
    {
        SIZE_T strLen = _vscwprintf(lpMessage, args);

        /* Create a buffer on the heap and zero it out (LPTR) */
        lpMsgBuf = (LPWSTR)LocalAlloc(LPTR, (strLen + 1) * sizeof(WCHAR));
        if (lpMsgBuf)
        {
            _vsnwprintf(lpMsgBuf, strLen, lpMessage, args);
        }
    }

    va_end(args);

    iRet = MessageBoxW(hWnd, (lpMessage && lpMsgBuf ? lpMsgBuf : L"Error displaying info message."), lpTitle, uType);

    if (lpMsgBuf) LocalFree(lpMsgBuf);

    /* Return the MessageBoxW information */
    return iRet;
}
