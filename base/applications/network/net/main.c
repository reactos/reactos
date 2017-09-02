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
    {L"use",        cmdUse},
    {L"user",       cmdUser},
    {L"view",       unimplemented},
    {NULL,          NULL}
};



VOID
PrintPaddedResourceString(
    UINT uID,
    INT nPaddedLength)
{
    INT nLength;

    nLength = ConResPuts(StdOut, uID);
    if (nLength < nPaddedLength)
        PrintPadding(L' ', nPaddedLength - nLength);
}


VOID
PrintPadding(
    WCHAR chr,
    INT nPaddedLength)
{
    INT i;
    WCHAR szMsgBuffer[MAX_BUFFER_SIZE];

    for (i = 0; i < nPaddedLength; i++)
         szMsgBuffer[i] = chr;
    szMsgBuffer[nPaddedLength] = UNICODE_NULL;

    ConPuts(StdOut, szMsgBuffer);
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

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc < 2)
    {
        ConResPuts(StdOut, IDS_NET_SYNTAX);
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

    ConResPuts(StdOut, IDS_NET_SYNTAX);

    return 1;
}

INT unimplemented(INT argc, WCHAR **argv)
{
    ConPuts(StdOut, L"This command is not implemented yet\n");
    return 1;
}
