/*
 * PROJECT:     ReactOS Tasklist Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays a list of currently running processes on the computer.
 * COPYRIGHT:   Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "tasklist.h"

// the strings in opList are the command-line options.
// should always correspond with the defines below, in sequence (except OP_PARAM_INVALID)
static PCWSTR opList[] = { L"?", L"nh" };

#define OP_PARAM_INVALID    -1
#define OP_PARAM_HELP       0
#define OP_PARAM_NOHEADER    1

// the max string length PrintResString can handle
#define RES_STR_MAXLEN 64

// Print split line
VOID PrintSplitLine(UINT Length)
{
    for (; Length; Length--)
    {
        ConPuts(StdOut, L"=");
    }
}

// Print spaces
VOID PrintSpace(UINT Length)
{
    ConPrintf(StdOut, L"%*ls", (INT)Length, L"");
}

// Print a string.
// if bAlignLeft == TRUE then aligned to left, otherwise aligned to right
// MaxWidth is the width for printing.
INT PrintString(LPCWSTR String, UINT MaxWidth, BOOL bAlignLeft)
{
    return ConPrintf(StdOut, bAlignLeft ? L"%-*.*ls" : L"%*.*ls", MaxWidth, MaxWidth, String);
}

// Print a string from resource
// if bAlignLeft == TRUE then aligned to left, otherwise aligned to right
// MaxWidth is the width for printing.
// The string WILL be truncated if it's longer than RES_STR_MAXLEN
INT PrintResString(HINSTANCE hInstance, UINT uID, UINT MaxWidth, BOOL bAlignLeft)
{
    if (!hInstance)
        return 0;
    
    WCHAR StringBuffer[RES_STR_MAXLEN];
    LoadStringW(hInstance, uID, StringBuffer, _countof(StringBuffer));
    return PrintString(StringBuffer, MaxWidth, bAlignLeft);
}

// Print a number, aligned to right.
// MaxWidth is the width for printing.
// the number WILL NOT be truncated if it's longer than MaxWidth
VOID PrintNum(LONGLONG Number, UINT MaxWidth)
{
    ConPrintf(StdOut, L"%*lld", MaxWidth, Number);
}

// Print memory size using KB as unit, with comma-separated number, aligned to right.
// MaxWidth is the width for printing.
// the number WILL be truncated if it's longer than MaxWidth
BOOL PrintMemory(SIZE_T MemorySizeByte, UINT MaxWidth, HINSTANCE hInstance)
{
    if (!hInstance)
        return FALSE;

    SIZE_T MemorySize = MemorySizeByte >> 10;

    WCHAR NumberString[27] = { 0 }; // length 26 is enough to display ULLONG_MAX in decimal with comma, one more for zero-terminated.
    C_ASSERT(sizeof(SIZE_T) <= 8);

    PWCHAR pNumberStr = NumberString;

    // calculate the length
    UINT PrintLength = 0;
    SIZE_T Tmp = MemorySize;
    do
    {
        Tmp /= 10;
        PrintLength++;
    } while (Tmp);

    INT Mod = 1;
    for (INT i = 0; i < PrintLength - 1; i++)
    {
        Mod *= 10;
    }

    for (INT i = PrintLength - 1; i >= 0; i--)
    {
        *pNumberStr = L'0' + (MemorySize / Mod);
        MemorySize %= Mod;
        pNumberStr++;
        
        if (i && i % 3 == 0)
        {
            *pNumberStr = L',';
            pNumberStr++;
        }
        Mod /= 10;
    }

    WCHAR FormatStr[RES_STR_MAXLEN];
    LoadStringW(hInstance, IDS_MEMORY_STR, FormatStr, _countof(FormatStr));

    WCHAR String[RES_STR_MAXLEN + _countof(NumberString)];
    
    swprintf(String, FormatStr, NumberString);
    PrintString(String, MaxWidth, FALSE);

    return TRUE;
}

VOID PrintHeader(HINSTANCE hInstance)
{
    if (!hInstance)
        return;

    PrintResString(hInstance, IDS_HEADER_IMAGENAME, COLUMNWIDTH_IMAGENAME, TRUE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_PID,       COLUMNWIDTH_PID,       FALSE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_SESSION,   COLUMNWIDTH_SESSION,   FALSE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_MEMUSAGE,  COLUMNWIDTH_MEMUSAGE,  FALSE);

    ConPuts(StdOut, L"\n");

    PrintSplitLine(COLUMNWIDTH_IMAGENAME);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_PID);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_SESSION);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_MEMUSAGE);

    ConPuts(StdOut, L"\n");
}

BOOL EnumProcessAndPrint(BOOL bNoHeader)
{
    // Call NtQuerySystemInformation for the process information
    ULONG ProcessInfoBufferLength = 0;
    ULONG ResultLength = 0;
    PBYTE ProcessInfoBuffer = NULL;

    // Get the buffer size we need
    NTSTATUS Status = NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &ResultLength);

    // New process/thread might appear before we call for the actual data.
    // Try to avoid this by retrying several times.
    for (INT Retry = 0; Retry < NT_SYSTEM_QUERY_MAX_RETRY; Retry++)
    {
        // (Re)allocate buffer
        ProcessInfoBufferLength = ResultLength;
        ResultLength = 0;
        if (ProcessInfoBuffer)
        {
            PBYTE NewProcessInfoBuffer = HeapReAlloc(GetProcessHeap(), 0,
                                                     ProcessInfoBuffer,
                                                     ProcessInfoBufferLength);
            if (NewProcessInfoBuffer)
            {
                ProcessInfoBuffer = NewProcessInfoBuffer;
            }
            else
            {
                // out of memory
                ConResMsgPrintf(StdErr, 0, IDS_OUT_OF_MEMORY);
                HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
                return FALSE;
            }
        }
        else
        {
            ProcessInfoBuffer = HeapAlloc(GetProcessHeap(), 0, ProcessInfoBufferLength);
            if (!ProcessInfoBuffer)
            {
                // out of memory
                ConResMsgPrintf(StdErr, 0, IDS_OUT_OF_MEMORY);
                return FALSE;
            }
        }

        // Query information
        Status = NtQuerySystemInformation(SystemProcessInformation,
                                          ProcessInfoBuffer,
                                          ProcessInfoBufferLength,
                                          &ResultLength);
        if (Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        // tried NT_SYSTEM_QUERY_MAX_RETRY times, or failed with some other reason
        ConResMsgPrintf(StdErr, 0, IDS_ENUM_FAILED);
        HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
        return FALSE;
    }

    HINSTANCE hInstance = GetModuleHandleW(NULL);
    assert(hInstance);

    ConPuts(StdOut, L"\n");

    if (!bNoHeader)
    {
        PrintHeader(hInstance);
    }

    PSYSTEM_PROCESS_INFORMATION pSPI;
    pSPI = (PSYSTEM_PROCESS_INFORMATION)ProcessInfoBuffer;
    while (pSPI)
    {
        PrintString(pSPI->UniqueProcessId ? pSPI->ImageName.Buffer : L"System Idle Process", COLUMNWIDTH_IMAGENAME, TRUE);
        PrintSpace(1);
        PrintNum((LONGLONG)(INT_PTR)pSPI->UniqueProcessId, COLUMNWIDTH_PID);
        PrintSpace(1);
        PrintNum((ULONGLONG)pSPI->SessionId, COLUMNWIDTH_SESSION);
        PrintSpace(1);
        PrintMemory(pSPI->WorkingSetSize, COLUMNWIDTH_MEMUSAGE, hInstance);

        ConPuts(StdOut, L"\n");

        if (pSPI->NextEntryOffset == 0)
            break;
        pSPI = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)pSPI + pSPI->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
    return TRUE;
}

INT GetArgumentType(LPCWSTR Argument)
{
    if (Argument[0] != L'/' && Argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    Argument++;

    for (INT i = 0; i < _countof(opList); i++)
    {
        if (!_wcsicmp(opList[i], Argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

BOOL ProcessArguments(INT argc, WCHAR **argv)
{
    BOOL bHasHelp = FALSE, bHasNoHeader = FALSE;

    for (INT i = 1; i < argc; i++)
    {
        INT Argument = GetArgumentType(argv[i]);

        switch (Argument)
        {
            case OP_PARAM_HELP:
            {
                if (bHasHelp)
                {
                    // -? already specified
                    ConResMsgPrintf(StdErr, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                    ConResMsgPrintf(StdErr, 0, IDS_USAGE);
                    return FALSE;
                }
                bHasHelp = TRUE;
                break;
            }
            case OP_PARAM_NOHEADER:
            {
                if (bHasNoHeader)
                {
                    // -nh already specified
                    ConResMsgPrintf(StdErr, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                    ConResMsgPrintf(StdErr, 0, IDS_USAGE);
                    return FALSE;
                }
                bHasNoHeader = TRUE;
                break;
            }
            case OP_PARAM_INVALID:
            default:
            {
                ConResMsgPrintf(StdErr, 0, IDS_INVALID_OPTION);
                ConResMsgPrintf(StdErr, 0, IDS_USAGE);
                return FALSE;
            }
        }
    }

    if (bHasHelp)
    {
        if (argc > 2) // any parameters other than -? is specified
        {
            ConResMsgPrintf(StdErr, 0, IDS_INVALID_SYNTAX);
            ConResMsgPrintf(StdErr, 0, IDS_USAGE);
            return FALSE;
        }
        else
        {
            ConResMsgPrintf(StdOut, 0, IDS_USAGE);
            ConResMsgPrintf(StdOut, 0, IDS_DESCRIPTION);
            return FALSE;
        }
    }
    else
    {
        EnumProcessAndPrint(bHasNoHeader);
    }
    return TRUE;
}

int wmain(int argc, WCHAR **argv)
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (!ProcessArguments(argc, argv))
    {
        return 1;
    }
    return 0;
}
