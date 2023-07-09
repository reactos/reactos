/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

Author:

    KazuM Apr.19.1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define WWSB_NOFE
#include "dispatch.h"
#undef  WWSB_NOFE
#define WWSB_FE
#include "dispatch.h"
#undef  WWSB_FE

#if defined(FE_SB)


NTSTATUS
DoSrvWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_DoSrvWriteConsole(m,ReplyStatus,Console,HandleData);
    else
        return SB_DoSrvWriteConsole(m,ReplyStatus,Console,HandleData);
}

NTSTATUS
WriteOutputString(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PVOID Buffer,
    IN COORD WriteCoord,
    IN ULONG StringType,
    IN OUT PULONG NumRecords, // this value is valid even for error cases
    OUT PULONG NumColumns OPTIONAL
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_WriteOutputString(ScreenInfo, Buffer, WriteCoord, StringType, NumRecords, NumColumns);
    else
        return SB_WriteOutputString(ScreenInfo, Buffer, WriteCoord, StringType, NumRecords, NumColumns);
}


VOID
WriteRectToScreenBuffer(
    PBYTE Source,
    COORD SourceSize,
    PSMALL_RECT SourceRect,
    PSCREEN_INFORMATION ScreenInfo,
    COORD TargetPoint,
    IN UINT Codepage
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        FE_WriteRectToScreenBuffer(Source,SourceSize,SourceRect,ScreenInfo,TargetPoint,Codepage);
    else
        SB_WriteRectToScreenBuffer(Source,SourceSize,SourceRect,ScreenInfo,TargetPoint,Codepage);
}

VOID
WriteToScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    PSMALL_RECT Region    // region is inclusive
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        FE_WriteToScreen(ScreenInfo, Region);
    else
        SB_WriteToScreen(ScreenInfo, Region);
}

VOID
WriteRegionToScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        FE_WriteRegionToScreen(ScreenInfo, Region);
    else
        SB_WriteRegionToScreen(ScreenInfo, Region);
}

NTSTATUS
FillOutput(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD Element,
    IN COORD WriteCoord,
    IN ULONG ElementType,
    IN OUT PULONG Length // this value is valid even for error cases
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_FillOutput(ScreenInfo, Element, WriteCoord, ElementType, Length);
    else
        return SB_FillOutput(ScreenInfo, Element, WriteCoord, ElementType, Length);
}

VOID
FillRectangle(
    IN CHAR_INFO Fill,
    IN OUT PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT TargetRect
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        FE_FillRectangle(Fill, ScreenInfo, TargetRect);
    else
        SB_FillRectangle(Fill, ScreenInfo, TargetRect);
}


ULONG
DoWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN PCONSOLE_INFORMATION Console,
    IN PCSR_THREAD Thread
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_DoWriteConsole(m,Console,Thread);
    else
        return SB_DoWriteConsole(m,Console,Thread);
}


NTSTATUS
WriteChars(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWCHAR lpBufferBackupLimit,
    IN PWCHAR lpBuffer,
    IN PWCHAR lpRealUnicodeString,
    IN OUT PDWORD NumBytes,
    OUT PLONG NumSpaces OPTIONAL,
    IN SHORT OriginalXPosition,
    IN DWORD dwFlags,
    OUT PSHORT ScrollY OPTIONAL
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_WriteChars(ScreenInfo,lpBufferBackupLimit,lpBuffer,lpRealUnicodeString,NumBytes,NumSpaces,OriginalXPosition,dwFlags,ScrollY);
    else
        return SB_WriteChars(ScreenInfo,lpBufferBackupLimit,lpBuffer,lpRealUnicodeString,NumBytes,NumSpaces,OriginalXPosition,dwFlags,ScrollY);
}



NTSTATUS
AdjustCursorPosition(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD CursorPosition,
    IN BOOL KeepCursorVisible,
    OUT PSHORT ScrollY OPTIONAL
    )
{
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_AdjustCursorPosition(ScreenInfo, CursorPosition, KeepCursorVisible, ScrollY);
    else
        return SB_AdjustCursorPosition(ScreenInfo, CursorPosition, KeepCursorVisible, ScrollY);
}

NTSTATUS
TranslateOutputToAnsiUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN OUT PCHAR_INFO OutputBufferR
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_TranslateOutputToAnsiUnicode(Console,OutputBuffer,Size,OutputBufferR);
    else
        return SB_TranslateOutputToAnsiUnicode(Console,OutputBuffer,Size);
}

NTSTATUS
TranslateOutputToUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size)
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_TranslateOutputToUnicode(Console,OutputBuffer,Size);
    else
        return SB_TranslateOutputToUnicode(Console,OutputBuffer,Size);
}

NTSTATUS
TranslateOutputToOemUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN BOOL fRemoveDbcsMark
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_TranslateOutputToOemUnicode(Console,OutputBuffer,Size,fRemoveDbcsMark);
    else
        return SB_TranslateOutputToOemUnicode(Console,OutputBuffer,Size);
}

NTSTATUS
TranslateOutputToOem(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        return FE_TranslateOutputToOem(Console,OutputBuffer,Size);
    else
        return SB_TranslateOutputToOem(Console,OutputBuffer,Size);
}

ULONG
TranslateInputToUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords,
    IN OUT PINPUT_RECORD DBCSLeadByte
    )
{
    if (CONSOLE_IS_DBCS_CP(Console))
        return FE_TranslateInputToUnicode(Console,InputRecords,NumRecords,DBCSLeadByte);
    else
        return SB_TranslateInputToUnicode(Console,InputRecords,NumRecords);
}

ULONG
TranslateInputToOem(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords,    // in : ASCII byte count
    IN ULONG UnicodeLength, // in : Number of events (char count)
    OUT PINPUT_RECORD DbcsLeadInpRec
    )
{
    if (CONSOLE_IS_DBCS_CP(Console))
        return FE_TranslateInputToOem(Console,InputRecords,NumRecords,UnicodeLength,DbcsLeadInpRec);
    else
        return SB_TranslateInputToOem(Console,InputRecords,NumRecords);
}

WCHAR
CharToWchar(
    IN PCONSOLE_INFORMATION Console,
    IN UINT Codepage,
    IN char *Ch
    )
{
    WCHAR wc;
    if (CONSOLE_IS_DBCS_CP(Console))
    {
        if (IsDBCSLeadByteConsole(*Ch, &Console->OutputCPInfo))
        {
            ConvertOutputToUnicode(Console->OutputCP,
                                   Ch,
                                   2,
                                   &wc,
                                   1);
        }
        else
        {
            ConvertOutputToUnicode(Console->OutputCP,
                                   Ch,
                                   1,
                                   &wc,
                                   1);
        }
    }
    else
    {
        wc = SB_CharToWchar(Codepage, *Ch);
    }
    return wc;
}


#ifdef i386
VOID
WriteRegionToScreenHW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    )
{
    if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
        FE_WriteRegionToScreenHW(ScreenInfo,Region);
    else
        SB_WriteRegionToScreenHW(ScreenInfo,Region);
}
#endif

#endif
