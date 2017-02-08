/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/main.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

#define MAX_BUFFER_SIZE 4096

typedef struct _COMMAND
{
    WCHAR *name;
    INT (*func)(INT, WCHAR**);
//    VOID (*help)(INT, WCHAR**);
} COMMAND, *PCOMMAND;

COMMAND cmds[] =
{
    {L"accounts",   cmdAccounts},
    {L"computer",   unimplemented},
    {L"config",     unimplemented},
    {L"continue",   cmdContinue},
    {L"file",       unimplemented},
    {L"group",      unimplemented},
    {L"help",       cmdHelp},
    {L"helpmsg",    cmdHelpMsg},
    {L"localgroup", cmdLocalGroup},
    {L"name",       unimplemented},
    {L"pause",      cmdPause},
    {L"print",      unimplemented},
    {L"send",       unimplemented},
    {L"session",    unimplemented},
    {L"share",      unimplemented},
    {L"start",      cmdStart},
    {L"statistics", unimplemented},
    {L"stop",       cmdStop},
    {L"time",       unimplemented},
    {L"use",        unimplemented},
    {L"user",       cmdUser},
    {L"view",       unimplemented},
    {NULL,          NULL}
};


VOID
PrintResourceString(
    INT resID,
    ...)
{
    WCHAR szMsgBuffer[MAX_BUFFER_SIZE];
    WCHAR szOutBuffer[MAX_BUFFER_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadStringW(GetModuleHandle(NULL), resID, szMsgBuffer, MAX_BUFFER_SIZE);
    _vsnwprintf(szOutBuffer, MAX_BUFFER_SIZE, szMsgBuffer, arg_ptr);
    va_end(arg_ptr);

    WriteToConsole(szOutBuffer);
}


VOID
PrintPaddedResourceString(
    INT resID,
    INT nPaddedLength)
{
    WCHAR szMsgBuffer[MAX_BUFFER_SIZE];
    INT nLength, i;

    nLength = LoadStringW(GetModuleHandle(NULL), resID, szMsgBuffer, MAX_BUFFER_SIZE);
    if (nLength < nPaddedLength)
    {
        for (i = nLength; i < nPaddedLength; i++)
            szMsgBuffer[i] = L' ';
        szMsgBuffer[nPaddedLength] = UNICODE_NULL;
    }

    WriteToConsole(szMsgBuffer);
}


VOID
PrintPadding(
    WCHAR chr,
    INT nPaddedLength)
{
    WCHAR szMsgBuffer[MAX_BUFFER_SIZE];
    INT i;

    for (i = 0; i < nPaddedLength; i++)
         szMsgBuffer[i] = chr;
    szMsgBuffer[nPaddedLength] = UNICODE_NULL;

    WriteToConsole(szMsgBuffer);
}


VOID
PrintToConsole(
    LPWSTR lpFormat,
    ...)
{
    WCHAR szBuffer[MAX_BUFFER_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, lpFormat);
    _vsnwprintf(szBuffer, MAX_BUFFER_SIZE, lpFormat, arg_ptr);
    va_end(arg_ptr);

    WriteToConsole(szBuffer);
}


VOID
WriteToConsole(
    LPWSTR lpString)
{
    CHAR szOemBuffer[MAX_BUFFER_SIZE * 2];
    HANDLE hOutput;
    DWORD dwLength;

    dwLength = wcslen(lpString);

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if ((GetFileType(hOutput) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR)
    {
        WriteConsoleW(hOutput,
                      lpString,
                      dwLength,
                      &dwLength,
                      NULL);
    }
    else
    {
        dwLength = WideCharToMultiByte(CP_OEMCP,
                                       0,
                                       lpString,
                                       dwLength,
                                       szOemBuffer,
                                       MAX_BUFFER_SIZE * 2,
                                       NULL,
                                       NULL);
        WriteFile(hOutput,
                  szOemBuffer,
                  dwLength,
                  &dwLength,
                  NULL);
    }
}


VOID
ReadFromConsole(
    LPWSTR lpInput,
    DWORD dwLength,
    BOOL bEcho)
{
    DWORD dwOldMode;
    DWORD dwRead = 0;
    HANDLE hFile;
    LPWSTR p;
    PCHAR pBuf;

    pBuf = HeapAlloc(GetProcessHeap(), 0, dwLength - 1);
    ZeroMemory(lpInput, dwLength * sizeof(WCHAR));
    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_LINE_INPUT | (bEcho ? ENABLE_ECHO_INPUT : 0));

    ReadFile(hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

    MultiByteToWideChar(CP_OEMCP, 0, pBuf, dwRead, lpInput, dwLength - 1);
    HeapFree(GetProcessHeap(), 0, pBuf);

    for (p = lpInput; *p; p++)
    {
        if (*p == L'\x0d')
        {
            *p = L'\0';
            break;
        }
    }

    SetConsoleMode(hFile, dwOldMode);
}


int wmain(int argc, WCHAR **argv)
{
    PCOMMAND cmdptr;

    if (argc < 2)
    {
        PrintResourceString(IDS_NET_SYNTAX);
        return 1;
    }

    /* Scan the command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (_wcsicmp(argv[1], cmdptr->name) == 0)
        {
            return cmdptr->func(argc, argv);
        }
    }

    PrintResourceString(IDS_NET_SYNTAX);

    return 1;
}

INT unimplemented(INT argc, WCHAR **argv)
{
    puts("This command is not implemented yet");
    return 1;
}
