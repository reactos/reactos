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
    {L"computer",   cmdComputer},
    {L"config",     cmdConfig},
    {L"continue",   cmdContinue},
    {L"file",       unimplemented},
    {L"group",      cmdGroup},
    {L"help",       cmdHelp},
    {L"helpmsg",    cmdHelpMsg},
    {L"localgroup", cmdLocalGroup},
    {L"pause",      cmdPause},
    {L"session",    unimplemented},
    {L"share",      cmdShare},
    {L"start",      cmdStart},
    {L"statistics", cmdStatistics},
    {L"stop",       cmdStop},
    {L"syntax",     cmdSyntax},
    {L"time",       unimplemented},
    {L"use",        cmdUse},
    {L"user",       cmdUser},
    {L"view",       unimplemented},
    {NULL,          NULL}
};

HMODULE hModuleNetMsg = NULL;


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


DWORD
TranslateAppMessage(
    DWORD dwMessage)
{
    switch (dwMessage)
    {
        case NERR_Success:
            return 3500; // APPERR_3500
        case ERROR_MORE_DATA:
            return 3513; // APPERR_3513
    }
    return dwMessage;
}

VOID
PrintMessageStringV(
    DWORD dwMessage,
    ...)
{
    PWSTR pBuffer;
    va_list args = NULL;

    va_start(args, dwMessage);

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                   hModuleNetMsg,
                   dwMessage,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&pBuffer,
                   0,
                   &args);
    va_end(args);

    if (pBuffer)
    {
        ConPuts(StdOut, pBuffer);
        LocalFree(pBuffer);
        pBuffer = NULL;
    }
}

VOID
PrintMessageString(
    DWORD dwMessage)
{
    PWSTR pBuffer;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   hModuleNetMsg,
                   dwMessage,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&pBuffer,
                   0,
                   NULL);
    if (pBuffer)
    {
        ConPuts(StdOut, pBuffer);
        LocalFree(pBuffer);
        pBuffer = NULL;
    }
}


VOID
PrintPaddedMessageString(
    DWORD dwMessage,
    INT nPaddedLength)
{
    PWSTR pBuffer;
    DWORD dwLength;

    dwLength = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                              FORMAT_MESSAGE_IGNORE_INSERTS,
                              hModuleNetMsg,
                              dwMessage,
                              LANG_USER_DEFAULT,
                              (LPWSTR)&pBuffer,
                              0,
                              NULL);
    if (pBuffer)
    {
        ConPuts(StdOut, pBuffer);
        LocalFree(pBuffer);
        pBuffer = NULL;
    }

    if (dwLength < (DWORD)nPaddedLength)
        PrintPadding(L' ', (DWORD)nPaddedLength - dwLength);
}


VOID
PrintErrorMessage(
    DWORD dwError)
{
    WCHAR szErrorBuffer[16];
    PWSTR pBuffer;
    PWSTR pErrorInserts[2] = {NULL, NULL};

    if (dwError >= MIN_LANMAN_MESSAGE_ID && dwError <= MAX_LANMAN_MESSAGE_ID)
    {
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       hModuleNetMsg,
                       dwError,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&pBuffer,
                       0,
                       NULL);
        if (pBuffer)
        {
            ConPrintf(StdErr, L"%s\n", pBuffer);
            LocalFree(pBuffer);
            pBuffer = NULL;
        }
    }
    else
    {
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dwError,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&pBuffer,
                       0,
                       NULL);
        if (pBuffer)
        {
            ConPrintf(StdErr, L"%s\n", pBuffer);
            LocalFree(pBuffer);
            pBuffer = NULL;
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
        /* Format insert for the 3514 message */
        swprintf(szErrorBuffer, L"%lu", dwError);
        pErrorInserts[0] = szErrorBuffer;

        /* Format and print the 3514 message */
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       hModuleNetMsg,
                       3514,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&pBuffer,
                       0,
                       (va_list *)pErrorInserts);
        if (pBuffer)
        {
            ConPrintf(StdErr, L"%s\n", pBuffer);
            LocalFree(pBuffer);
            pBuffer = NULL;
        }
    }
}


VOID
PrintNetMessage(
    DWORD dwMessage)
{
    PWSTR pBuffer;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_HMODULE |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   GetModuleHandleW(NULL),
                   dwMessage,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&pBuffer,
                   0,
                   NULL);
    if (pBuffer)
    {
        ConPuts(StdOut, pBuffer);
        ConPuts(StdOut, L"\n");
        LocalFree(pBuffer);
        pBuffer = NULL;
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
    WCHAR szDllBuffer[MAX_PATH];
    PCOMMAND cmdptr;
    int nResult = 0;
    BOOL bRun = FALSE;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Load netmsg.dll */
    GetSystemDirectoryW(szDllBuffer, ARRAYSIZE(szDllBuffer));
    wcscat(szDllBuffer, L"\\netmsg.dll");

    hModuleNetMsg = LoadLibrary(szDllBuffer);
    if (hModuleNetMsg == NULL)
    {
        ConPrintf(StdErr, L"Failed to load netmsg.dll\n");
        return 1;
    }

    if (argc < 2)
    {
        nResult = 1;
        goto done;
    }

    /* Scan the command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (_wcsicmp(argv[1], cmdptr->name) == 0)
        {
            nResult = cmdptr->func(argc, argv);
            bRun = TRUE;
            break;
        }
    }

done:
    if (bRun == FALSE)
    {
        PrintMessageString(4381);
        ConPuts(StdOut, L"\n");
        PrintNetMessage(MSG_NET_SYNTAX);
    }

    if (hModuleNetMsg != NULL)
        FreeLibrary(hModuleNetMsg);

    return nResult;
}

INT unimplemented(INT argc, WCHAR **argv)
{
    ConPuts(StdOut, L"This command is not implemented yet\n");
    return 1;
}
