/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    tzermem.c

Abstract:

    This module implements a test of the operation of the RtlZeroMemory
    function by running an exhaustive test of every case of string offset
    and length up to and a little beyond one 32-byte cache line. This
    represents several thousand test cases. It is assumed any bugs that
    exist will be found within this range. If only the error count summary
    is desired, type "tzermem > nul" instead.

Author:

    Thomas Van Baak (tvb) 13-Jan-1993

Environment:

    User mode.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include "localrtl.h"

#define BUFFER_SIZE (MAX_MARGIN + MAX_OFFSET + MAX_LENGTH + MAX_MARGIN)

UCHAR Buffer1[BUFFER_SIZE];
UCHAR Buffer2[BUFFER_SIZE];

void
__cdecl
main()
{
    ULONG ErrorCount;
    ULONG Length;
    ULONG Offset;
    ULONG Result;
    ULONG TestCases;

    fprintf(stderr, "Testing RtlZeroMemory\n");
    ErrorCount = 0;
    TestCases = 0;

    for (Offset = 0; Offset <= MAX_OFFSET; Offset += 1) {
        for (Length = 0; Length <= MAX_LENGTH; Length += 1) {

            FillPattern(Buffer1, BUFFER_SIZE);
            FillPattern(Buffer2, BUFFER_SIZE);
            LocalZeroMemory(&Buffer1[Offset], Length);
            RtlZeroMemory(&Buffer2[Offset], Length);

            Result = LocalCompareMemory(Buffer1, Buffer2, BUFFER_SIZE);

            TestCases += 1;
            if (Result != BUFFER_SIZE) {
                ErrorCount += 1;

                printf("ERROR: Offset = %d, Length = %d\n", Offset, Length);
                printf("Buffers differ starting at byte %d:\n", Result);
                printf("Buffer1 = <%*s>\n", BUFFER_SIZE, Buffer1);
                printf("Buffer2 = <%*s>\n", BUFFER_SIZE, Buffer2);
                printf("\n");
            }
        }
    }

    fprintf(stderr, "Test of RtlZeroMemory completed: ");
    fprintf(stderr, "%d test cases, %d errors found.\n", TestCases, ErrorCount);
}
