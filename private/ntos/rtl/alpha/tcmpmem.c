/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    tcmpmem.c

Abstract:

    This module implements a test of the operation of the RtlCompareMemory
    function by running an exhaustive test of every case of string offset,
    compare length, and return value up to and a little beyond one 32-byte
    cache line. This represents over one million test cases. It is assumed
    any bugs that exist will be found within this range. If only the error
    count summary is desired, type "tcmpmem > nul" instead.

Author:

    Thomas Van Baak (tvb) 11-Jan-1993

Environment:

    User mode.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include "localrtl.h"

#define BUFFER_SIZE (MAX_OFFSET + MAX_LENGTH)

UCHAR String1[BUFFER_SIZE];
UCHAR String2[BUFFER_SIZE];

void
__cdecl
main()
{
    ULONG ErrorCount;
    ULONG Expected;
    ULONG Length;
    ULONG Offset1;
    ULONG Offset2;
    ULONG Result;
    ULONG TestCases;

    fprintf(stderr, "Testing RtlCompareMemory\n");
    ErrorCount = 0;
    TestCases = 0;

    for (Offset1 = 0; Offset1 <= MAX_OFFSET; Offset1 += 1) {

        //
        // Copy the test pattern to Offset1 in String1 and then for each
        // possible offset of String1, for each possible offset of String2,
        // for each possible string compare length, and for each expected
        // return value, make a call RtlCompareMemory.
        //

        FillPattern(&String1[Offset1], MAX_LENGTH);
        for (Offset2 = 0; Offset2 <= MAX_OFFSET; Offset2 += 1) {
            for (Length = 0; Length <= MAX_LENGTH; Length += 1) {
                for (Expected = 0; Expected <= Length; Expected += 1) {

                    //
                    // Copy the test pattern starting at Offset2 in String2,
                    // change one byte at location `Expected', call
                    // RtlCompareMemory, and check that the function value
                    // is in fact the expected value.
                    //

                    FillPattern(&String2[Offset2], MAX_LENGTH);
                    String2[Offset2 + Expected] = ' ';
                    Result = RtlCompareMemory(&String1[Offset1],
                                              &String2[Offset2],
                                              Length);
                    TestCases += 1;
                    if (Result != Expected) {
                        ErrorCount += 1;

                        //
                        // The function failed to return the proper value.
                        //

                        printf("ERROR: Offset1 = %d, Offset2 = %d, Length = %d, Expected = %d, Result = %d\n",
                               Offset1, Offset2, Length, Expected, Result);
                        printf("  String1[Offset1] = %lx: <%.*s>\n",
                               &String1[Offset1], Length, &String1[Offset1]);
                        printf("  String2[Offset2] = %lx: <%.*s>\n",
                               &String2[Offset2], Length, &String2[Offset2]);
                        printf("\n");
                    }
                }
            }
        }
    }

    fprintf(stderr, "Test of RtlCompareMemory completed: ");
    fprintf(stderr, "%d test cases, %d errors found.\n", TestCases, ErrorCount);
}
