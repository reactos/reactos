/*
 * PROJECT:     ReactOS Tasklist Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays a list of currently running processes on the computer.
 * COPYRIGHT:   Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "tasklist.h"


VOID PrintSplitLine(INT Length)
{
    for (INT i = 0; i < Length; i++)
    {
        ConPrintf(StdOut, L"=");
    }
}

// Print spaces
BOOL PrintSpace(INT SpaceNum)
{
    for (; SpaceNum; SpaceNum--)
    {
        ConPrintf(StdOut, L" ");
    }
    return TRUE;
}

// Print a string.
// if bAlignLeft == TRUE then aligned to left, otherwise aligned to right
// MaxWidth is the width for printing.
INT PrintString(LPCWSTR String, INT MaxWidth, BOOL bAlignLeft)
{
    return ConPrintf(StdOut, bAlignLeft ? L"%-*.*ls" : L"%*.*ls", MaxWidth, MaxWidth, String);
}

// Print a string from resource
// if bAlignLeft == TRUE then aligned to left, otherwise aligned to right
// MaxWidth is the width for printing.
// The string will be truncated if it's longer than RES_STR_MAXLEN
INT PrintResString(HINSTANCE hInstance, UINT uid, INT MaxWidth, BOOL bAlignLeft)
{
    WCHAR StringBuffer[RES_STR_MAXLEN];
    LoadStringW(hInstance, uid, StringBuffer, _countof(StringBuffer));
    return PrintString(StringBuffer, MaxWidth, bAlignLeft);
}

// Print a number, aligned to right.
// MaxWidth is the width for printing.
// return FALSE if number length is longer than MaxWidth
BOOL PrintNum(LONGLONG Number, INT MaxWidth)
{
    // calculate the length needed to print.
    INT PrintLength = 0;
    LONGLONG Tmp = Number;

    if (Tmp < 0)
    {
        Tmp = -Tmp;
        PrintLength++;
    }

    do
    {
        Tmp /= 10;
        PrintLength += 1;
    } while (Tmp);

    if (PrintLength > MaxWidth)
    {
        return FALSE;
    }
    ConPrintf(StdOut, L"%*lld", MaxWidth, Number);

    return TRUE;
}

// Print memory size using appropriate unit, with comma-separated number, aligned to right.
// MaxWidth is the width for printing.
// StartingUnit is the minimum memory unit used, 0 for Byte, 1 for KB, 2 for MB ...
// return FALSE when failed to format.
BOOL PrintMemory(SIZE_T MemorySizeByte, INT MaxWidth, INT StartingUnit)
{
    WCHAR *MemoryUnit[] = {L"B", L"K", L"M", L"G", L"T", L"P"};

    MaxWidth -= 2; // a space and a unit will occupy 2 length.
    if (MaxWidth <= 0)
    {
        return FALSE;
    }

    SIZE_T MemorySize = MemorySizeByte;

    for (INT i = 0; i < _countof(MemoryUnit); i++)
    {
        if (i >= StartingUnit)
        {
            // calculate the length needed to print.
            INT PrintLength = 0;
            SIZE_T Tmp = MemorySize;
            do
            {
                Tmp /= 10;
                PrintLength += 1;
            } while (Tmp);

            if (PrintLength + (PrintLength - 1) / 3 <= MaxWidth) // (PrintLength - 1) / 3 is the length comma will take.
            {
                // enough to hold

                // print padding space
                PrintSpace(MaxWidth - (PrintLength + (PrintLength - 1) / 3));

                INT Mod = 1;
                for (INT i = 0; i < PrintLength - 1; i++)
                {
                    Mod *= 10;
                }

                for (INT i = PrintLength - 1; i >= 0; i--)
                {
                    ConPrintf(StdOut, L"%d", MemorySize / Mod);
                    MemorySize %= Mod;
                    if (i && i % 3 == 0)
                    {
                        ConPrintf(StdOut, L",");
                    }
                    Mod /= 10;
                }

                // print the unit.
                ConPrintf(StdOut, L" %ls", MemoryUnit[i]);

                return TRUE;
            }
        }
        
        MemorySize >>= 10; // MemorySize /= 1024;
    }

    // out of MemoryUnit range.
    return FALSE;
}

VOID PrintHeader(VOID)
{
    ConPrintf(StdOut, L"\n");

    HINSTANCE hInstance = GetModuleHandleW(NULL);
    PrintResString(hInstance, IDS_HEADER_IMAGENAME, COLUMNWIDTH_IMAGENAME, TRUE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_PID,       COLUMNWIDTH_PID,       FALSE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_SESSION,   COLUMNWIDTH_SESSION,   FALSE);
    PrintSpace(1);
    PrintResString(hInstance, IDS_HEADER_MEMUSAGE,  COLUMNWIDTH_MEMUSAGE,  FALSE);

    ConPrintf(StdOut, L"\n");

    PrintSplitLine(COLUMNWIDTH_IMAGENAME);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_PID);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_SESSION);
    PrintSpace(1);
    PrintSplitLine(COLUMNWIDTH_MEMUSAGE);

    ConPrintf(StdOut, L"\n");
}

BOOL EnumProcessAndPrint(BOOL bVerbose)
{
    // Call NtQuerySystemInformation for the process information
    ULONG ProcessInfoBufferLength = 0;
    ULONG ResultLength = 0;
    PBYTE ProcessInfoBuffer = 0;

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
                // out of memory ?
                ConResMsgPrintf(StdOut, 0, IDS_OUT_OF_MEMORY);
                HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
                return FALSE;
            }
        }
        else
        {
            ProcessInfoBuffer = HeapAlloc(GetProcessHeap(), 0, ProcessInfoBufferLength);
            if (!ProcessInfoBuffer)
            {
                // out of memory ?
                ConResMsgPrintf(StdOut, 0, IDS_OUT_OF_MEMORY);
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
        ConResMsgPrintf(StdOut, 0, IDS_ENUM_FAILED);
        HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
        return FALSE;
    }

    PrintHeader();

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
        PrintMemory(pSPI->WorkingSetSize, COLUMNWIDTH_MEMUSAGE, 1);

        ConPrintf(StdOut, L"\n");

        if (pSPI->NextEntryOffset == 0)
            break;
        pSPI = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)pSPI + pSPI->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, ProcessInfoBuffer);
    return TRUE;
}

INT GetArgumentType(WCHAR* argument)
{
    INT i;

    if (argument[0] != L'/' && argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    argument++;

    for (i = 0; i < _countof(opList); i++)
    {
        if (!wcsicmp(opList[i], argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

BOOL ProcessArguments(INT argc, WCHAR *argv[])
{
    INT i;
    BOOL bHasHelp = FALSE, bHasVerbose = FALSE;
    for (i = 1; i < argc; i++)
    {
        INT Argument = GetArgumentType(argv[i]);

        switch (Argument)
        {
            case OP_PARAM_HELP:
            {
                if (bHasHelp)
                {
                    // -? already specified
                    ConResMsgPrintf(StdOut, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                    ConResMsgPrintf(StdOut, 0, IDS_USAGE);
                    return FALSE;
                }
                bHasHelp = TRUE;
                break;
            }
            case OP_PARAM_VERBOSE:
            {
                if (bHasVerbose)
                {
                    // -V already specified
                    ConResMsgPrintf(StdOut, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                    ConResMsgPrintf(StdOut, 0, IDS_USAGE);
                    return FALSE;
                }
                bHasVerbose = TRUE;
                break;
            }
            case OP_PARAM_INVALID:
            default:
            {
                ConResMsgPrintf(StdOut, 0, IDS_INVALID_OPTION);
                ConResMsgPrintf(StdOut, 0, IDS_USAGE);
                return FALSE;
            }
        }
    }

    if (bHasHelp)
    {
        if (argc > 2) // any parameters other than -? is specified
        {
            ConResMsgPrintf(StdOut, 0, IDS_INVALID_SYNTAX);
            ConResMsgPrintf(StdOut, 0, IDS_USAGE);
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
        EnumProcessAndPrint(bHasVerbose);
    }
    return TRUE;
}

int wmain(int argc, WCHAR *argv[])
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (!ProcessArguments(argc, argv))
    {
        return 1;
    }
    return 0;
}
