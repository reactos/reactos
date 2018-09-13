/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    tmovmem.c

Abstract:

    This module implements a test of the operation of the RtlMoveMemory
    function by running an exhaustive test of every case of string offset,
    move length, and relative overlap of the two strings up to and a little
    beyond one 32-byte cache line. This represents several hundred thousand
    test cases. It is assumed any bugs that exist will be found within this
    range. If only the error count summary is desired, type "tmovmem > nul"
    instead.

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

//
// Two strings are defined within a large buffer. The target string is
// initially below the source string with a small gap between the two
// strings. A margin around the strings ensures any bytes accidentally
// changed outside the strings are detectable. As the length of the strings
// and the offset of the source string are varied, the target string wanders
// from well below, through, and well above the source string.
//

#define MIN_OVERLAP (-(MAX_LENGTH + MAX_MARGIN))
#define MAX_OVERLAP (MAX_LENGTH + MAX_MARGIN - 1)

#define BUFFER_SIZE (MAX_MARGIN + MAX_LENGTH + MAX_MARGIN + MAX_OFFSET + MAX_LENGTH + MAX_MARGIN + MAX_LENGTH + MAX_MARGIN)

UCHAR Buffer0[BUFFER_SIZE];
UCHAR Buffer1[BUFFER_SIZE];
UCHAR Buffer2[BUFFER_SIZE];

void
__cdecl
main()
{
    ULONG ErrorCount;
    ULONG Length;
    ULONG Offset;
    LONG Overlap;
    ULONG Result;
    ULONG Source;
    ULONG Target;
    ULONG TestCases;

    fprintf(stderr, "Testing RtlMoveMemory\n");
    ErrorCount = 0;
    TestCases = 0;

    //
    // Make a call to RtlMoveMemory for all possible source string offsets
    // within a cache line, for a large set of string lengths, and a wide
    // range of positions of the target string relative to the source string,
    // including all possible overlapping string configurations.
    //

    for (Offset = 0; Offset <= MAX_OFFSET; Offset += 1) {
        for (Length = 0; Length <= MAX_LENGTH; Length += 1) {
            for (Overlap = MIN_OVERLAP; Overlap <= MAX_OVERLAP; Overlap += 1) {

                //
                // The same string configuration is made in two different
                // buffers. RtlMoveMemory is used on the two strings in one
                // buffer and the trusted LocalMoveMemory on the two strings
                // in the other buffer. The entire buffers are compared to
                // determine if the two move functions agree.
                //

                FillPattern(Buffer1, BUFFER_SIZE);
                FillPattern(Buffer2, BUFFER_SIZE);

                Source = MAX_MARGIN + MAX_LENGTH + MAX_MARGIN + Offset;
                Target = Source + Overlap;

                LocalMoveMemory(&Buffer1[Target], &Buffer1[Source], Length);
                RtlMoveMemory(&Buffer2[Target], &Buffer2[Source], Length);

                Result = LocalCompareMemory(Buffer1, Buffer2, BUFFER_SIZE);

                TestCases += 1;
                if (Result != BUFFER_SIZE) {
                    ErrorCount += 1;

                    printf("ERROR: Offset = %d, Length = %d, Overlap = %d\n",
                           Offset, Length, Overlap);
                    printf("RtlMoveMemory( &Buffer[ %d ], &Buffer[ %d ], %d )\n",
                           Target, Source, Length);

                    FillPattern(Buffer0, BUFFER_SIZE);
                    printf("  Original Source = %lx: <%.*s>\n",
                           &Buffer0[Source], Length, &Buffer0[Source]);
                    printf("  Expected Target = %lx: <%.*s>\n",
                           &Buffer1[Target], Length, &Buffer1[Target]);
                    printf("  Actual Target   = %lx: <%.*s>\n",
                           &Buffer2[Target], Length, &Buffer2[Target]);
                    printf("\n");
                    printf("Buffers differ starting at byte %d:\n", Result);
                    printf("Buffer1 = <%*s>\n", BUFFER_SIZE, Buffer1);
                    printf("Buffer2 = <%*s>\n", BUFFER_SIZE, Buffer2);
                    printf("\n");
                }
            }
        }
    }

    fprintf(stderr, "Test of RtlMoveMemory completed: ");
    fprintf(stderr, "%d test cases, %d errors found.\n", TestCases, ErrorCount);
}
