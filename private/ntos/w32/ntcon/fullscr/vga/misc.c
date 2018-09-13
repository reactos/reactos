/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"
#include "fsvga.h"
#include "fsvgalog.h"


int
ConvertOutputToOem(
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
/*
    Converts SourceLength Unicode characters from Source into
    not more than TargetLength Codepage characters at Target.
    Returns the number characters put in Target. (0 if failure)

    [ntcon\server\misc.c]
*/

{
    NTSTATUS Status;
    int Length;
    UNICODE_STRING SourceUni;
    ANSI_STRING TargetAns;
    CHAR AnsBuf[256];

    SourceUni.MaximumLength =
    SourceUni.Length = SourceLength * sizeof(WCHAR);
    SourceUni.Buffer = Source;

    TargetAns.Length = 0;
    TargetAns.MaximumLength = sizeof(AnsBuf);
    TargetAns.Buffer = AnsBuf;

    // Can do this in place
    Status = RtlUnicodeStringToAnsiString(&TargetAns,
                                          &SourceUni,
                                          FALSE);
    if (NT_SUCCESS(Status)) {
        Length = strlen(AnsBuf);
        if (Length <= TargetLength) {
            RtlMoveMemory(Target, AnsBuf, Length);
            return Length;
        }
        else {
            return 0;
        }
    } else {
        return 0;
    }
}

/***************************************************************************\
* TranslateOutputToOem
*
* routine to translate console PCHAR_INFO to the ASCII from Unicode
*
* [ntcon\server\fe\direct2.c]
\***************************************************************************/
NTSTATUS
TranslateOutputToOem(
    OUT PCHAR_IMAGE_INFO OutputBuffer,
    IN  PCHAR_IMAGE_INFO InputBuffer,
    IN  DWORD Length
    )
{
    CHAR AsciiDbcs[2];
    ULONG NumBytes;

    while (Length--)
    {
        if (InputBuffer->CharInfo.Attributes & COMMON_LVB_LEADING_BYTE)
        {
            if (Length >= 2)    // Safe DBCS in buffer ?
            {
                Length--;
                NumBytes = sizeof(AsciiDbcs);
                NumBytes = ConvertOutputToOem(&InputBuffer->CharInfo.Char.UnicodeChar,
                                              1,
                                              &AsciiDbcs[0],
                                              NumBytes);
                OutputBuffer->CharInfo.Char.AsciiChar = AsciiDbcs[0];
                OutputBuffer->CharInfo.Attributes = InputBuffer->CharInfo.Attributes;
                OutputBuffer++;
                InputBuffer++;
                OutputBuffer->CharInfo.Char.AsciiChar = AsciiDbcs[1];
                OutputBuffer->CharInfo.Attributes = InputBuffer->CharInfo.Attributes;
                OutputBuffer++;
                InputBuffer++;
            }
            else
            {
                OutputBuffer->CharInfo.Char.AsciiChar = ' ';
                OutputBuffer->CharInfo.Attributes = InputBuffer->CharInfo.Attributes & ~COMMON_LVB_SBCSDBCS;
                OutputBuffer++;
                InputBuffer++;
            }
        }
        else if (! (InputBuffer->CharInfo.Attributes & COMMON_LVB_SBCSDBCS))
        {
            ConvertOutputToOem(&InputBuffer->CharInfo.Char.UnicodeChar,
                               1,
                               &OutputBuffer->CharInfo.Char.AsciiChar,
                               1);
            OutputBuffer->CharInfo.Attributes = InputBuffer->CharInfo.Attributes;
            OutputBuffer++;
            InputBuffer++;
        }
    }

    return STATUS_SUCCESS;
}
