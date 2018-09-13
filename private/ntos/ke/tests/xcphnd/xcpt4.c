/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    xcpt4.c

Abstract:

    This module implements user mode exception tests.

Author:

    David N. Cutler (davec) 18-Sep-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "stdio.h"
#include "nt.h"
#include "ntrtl.h"
#include "setjmpex.h"

#include "float.h"



//
// Define switch constants.
//

#define BLUE 0
#define RED 1

//
// Define function prototypes.
//

VOID
addtwo (
    IN LONG First,
    IN LONG Second,
    IN PLONG Place
    );

VOID
bar1 (
    IN NTSTATUS Status,
    IN PLONG Counter
    );

VOID
bar2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress,
    IN PLONG Counter
    );

VOID
dojump (
    IN jmp_buf JumpBuffer,
    IN PLONG Counter
    );

LONG
Echo(
    IN LONG Value
    );

VOID
eret (
    IN NTSTATUS Status,
    IN PLONG Counter
    );

VOID
except1 (
    IN PLONG Counter
    );

ULONG
except2 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    );

ULONG
except3 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    );

VOID
foo1 (
    IN NTSTATUS Status
    );

VOID
foo2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress
    );

VOID
fret (
    IN PLONG Counter
    );

BOOLEAN
Tkm (
    VOID
    );

VOID
Test61Part2 (
    IN OUT PULONG Counter
    );

VOID
PerformFpTest(
    VOID
    );

double
SquareDouble (
    IN double   op
    );

VOID
SquareDouble17E300 (
    OUT PVOID   ans
    );


VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

{

    PLONG BadAddress;
    PCHAR BadByte;
    PLONG BlackHole;
    ULONG Index1;
    ULONG Index2 = RED;
    jmp_buf JumpBuffer;
    LONG Counter;
    EXCEPTION_RECORD ExceptionRecord;
    double  doubleresult;

    //
    // Announce start of exception test.
    //

    printf("Start of exception test\n");

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionCode = STATUS_INTEGER_OVERFLOW;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;

    //
    // Initialize pointers.
    //

    BadAddress = (PLONG)NULL;
    BadByte = (PCHAR)NULL;
    BadByte += 1;
    BlackHole = &Counter;

    //
    // Simply try statement with a finally clause that is entered sequentially.
    //

    printf("    test1...");
    Counter = 0;
    try {
        Counter += 1;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 1;
        }
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is never executed
    // because there is no exception raised in the try clause.
    //

    printf("    test2...");
    Counter = 0;
    try {
        Counter += 1;

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 1) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try statement with an exception handler that is never executed
    // because the exception expression continues execution.
    //

    printf("    test3...");
    Counter = 0;
    try {
        Counter -= 1;
        RtlRaiseException(&ExceptionRecord);

    } except (Counter) {
        Counter -= 1;
    }

    if (Counter != - 1) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    printf("    test4...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    printf("    test5...");
    Counter = 0;
    try {
        Counter += 1;
        *BlackHole += *BadAddress;

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    printf("    test6...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            RtlRaiseException(&ExceptionRecord);

        } finally {
            if (abnormal_termination() != FALSE) {
                Counter += 1;
            }
        }

    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    printf("    test7...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            *BlackHole += *BadAddress;

        } finally {
            if (abnormal_termination() != FALSE) {
                Counter += 1;
            }
        }

    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that calls a function which raises an exception.
    //

    printf("    test8...");
    Counter = 0;
    try {
        Counter += 1;
        foo1(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that calls a function which raises an exception.
    //

    printf("    test9...");
    Counter = 0;
    try {
        Counter += 1;
        foo2(BlackHole, BadAddress);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //

    printf("    test10...");
    Counter = 0;
    try {
        bar1(STATUS_ACCESS_VIOLATION, &Counter);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter -= 1;
    }

    if (Counter != 98) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //

    printf("    test11...");
    Counter = 0;
    try {
        bar2(BlackHole, BadAddress, &Counter);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter -= 1;
    }

    if (Counter != 98) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A try within an except
    //

    printf("    test12...");
    Counter = 0;
    try {
        foo1(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
        try {
            foo1(STATUS_SUCCESS);

        } except ((GetExceptionCode() == STATUS_SUCCESS) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (Counter != 1) {
                printf("failed, count = %d\n", Counter);

            } else {
                printf("succeeded...");
            }

            Counter += 1;
        }
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A try within an except
    //

    printf("    test13...");
    Counter = 0;
    try {
        foo2(BlackHole, BadAddress);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
        try {
            foo1(STATUS_SUCCESS);

        } except ((GetExceptionCode() == STATUS_SUCCESS) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (Counter != 1) {
                printf("failed, count = %d\n", Counter);

            } else {
                printf("succeeded...");
            }

            Counter += 1;
        }
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally
    //

    printf("    test14...");
    Counter = 0;
    try {
        try {
            foo1(STATUS_ACCESS_VIOLATION);

        } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            Counter += 1;
            goto t9;
        }

    } finally {
        Counter += 1;
    }

t9:;
    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally
    //

    printf("    test15...");
    Counter = 0;
    try {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            goto t10;
        }

    } finally {
        Counter += 1;
    }

t10:;
    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally into the outer finally clause.
    //

    printf("    test16...");
    Counter = 0;
    try {
        try {
            try {
                Counter += 1;
                foo1(STATUS_INTEGER_OVERFLOW);

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 1;
                goto t11;
            }

        } finally {
            Counter += 1;
        }
t11:;
    } finally {
        Counter += 1;
    }

    if (Counter != 4) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally into the outer finally clause.
    //

    printf("    test17...");
    Counter = 0;
    try {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            goto t12;
        }
t12:;
    } finally {
        Counter += 1;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A return from an except clause
    //

    printf("    test18...");
    Counter = 0;
    try {
        Counter += 1;
        eret(STATUS_ACCESS_VIOLATION, &Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 4) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A return from a finally clause
    //

    printf("    test19...");
    Counter = 0;
    try {
        Counter += 1;
        fret(&Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 5) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A simple set jump followed by a long jump.
    //

    printf("    test20...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        Counter += 1;
        longjmp(JumpBuffer, 1);

    } else {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump followed by a long jump out of a finally clause that is
    // sequentially executed.
    //

    printf("    test21...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            Counter += 1;

        } finally {
            Counter += 1;
            longjmp(JumpBuffer, 1);
        }

    } else {
        Counter += 1;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump within a try clause followed by a long jump out of a
    // finally clause that is sequentially executed.
    //

    printf("    test22...");
    Counter = 0;
    try {
        if (setjmp(JumpBuffer) == 0) {
            Counter += 1;

        } else {
            Counter += 1;
        }

    } finally {
        Counter += 1;
        if (Counter == 2) {
            Counter += 1;
            longjmp(JumpBuffer, 1);
        }
    }

    if (Counter != 5) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally where
    // the try body of the try/finally raises an exception that is handled
    // by the try/excecpt which causes the try/finally to do a long jump out
    // of a finally clause. This will create a collided unwind.
    //

    printf("    test23...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                Counter += 1;
                RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

            } finally {
                Counter += 1;
                longjmp(JumpBuffer, 1);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a several nested
    // try/finally's where the inner try body of the try/finally raises an
    // exception that is handled by the try/except which causes the
    // try/finally to do a long jump out of a finally clause. This will
    // create a collided unwind.
    //

    printf("    test24...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    try {
                        Counter += 1;
                        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

                    } finally {
                        Counter += 1;
                    }

                } finally {
                    Counter += 1;
                    longjmp(JumpBuffer, 1);
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 5) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally which
    // calls a subroutine which contains a try finally that raises an
    // exception that is handled to the try/except.
    //

    printf("    test25...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    Counter += 1;
                    dojump(JumpBuffer, &Counter);

                } finally {
                    Counter += 1;
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 7) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // A set jump followed by a try/except, followed by a try/finally which
    // calls a subroutine which contains a try finally that raises an
    // exception that is handled to the try/except.
    //

    printf("    test26...");
    Counter = 0;
    if (setjmp(JumpBuffer) == 0) {
        try {
            try {
                try {
                    try {
                        Counter += 1;
                        dojump(JumpBuffer, &Counter);

                    } finally {
                        Counter += 1;
                    }

                } finally {
                    Counter += 1;
                    longjmp(JumpBuffer, 1);
                }

            } finally {
                Counter += 1;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Counter += 1;
        }

    } else {
        Counter += 1;
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Test nested exceptions.
    //

    printf("    test27...");
    Counter = 0;
    try {
        try {
            Counter += 1;
            except1(&Counter);

        } except(except2(GetExceptionInformation(), &Counter)) {
            Counter += 2;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Counter += 3;
    }

    if (Counter != 55) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that causes an integer overflow exception.
    //

    printf("    test28...");
    Counter = 0;
    try {
        Counter += 1;
        addtwo(0x7fff0000, 0x10000, &Counter);

    } except ((GetExceptionCode() == STATUS_INTEGER_OVERFLOW) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Simple try that raises an misaligned data exception.
    //

#ifndef i386
    printf("    test29...");
    Counter = 0;
    try {
        Counter += 1;
        foo2(BlackHole, (PLONG)BadByte);

    } except ((GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

#endif

    //
    // Continue from a try body with an exception clause in a loop.
    //

    printf("    test30...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                continue;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
    }

    if (Counter != 15) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from a try body with an finally clause in a loop.
    //

    printf("    test31...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                continue;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 40) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from doubly nested try body with an exception clause in a
    // loop.
    //

    printf("    test32...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    continue;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 30) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from doubly nested try body with an finally clause in a loop.
    //

    printf("    test33...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    continue;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 105) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from a finally clause in a loop.
    //

    printf("    test34...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 0) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            continue;
        }

        Counter += 4;
    }

    if (Counter != 25) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from a doubly nested finally clause in a loop.
    //

    printf("    test35...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                continue;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 75) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Continue from a doubly nested finally clause in a loop.
    //

    printf("    test36...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 0) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            continue;
        }

        Counter += 6;
    }

    if (Counter != 115) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a try body with an exception clause in a loop.
    //

    printf("    test37...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a try body with an finally clause in a loop.
    //

    printf("    test38...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from doubly nested try body with an exception clause in a
    // loop.
    //

    printf("    test39...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 6) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from doubly nested try body with an finally clause in a loop.
    //

    printf("    test40...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 21) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a finally clause in a loop.
    //

    printf("    test41...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            if ((Index1 & 0x1) == 1) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            break;
        }

        Counter += 4;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a loop.
    //

    printf("    test42...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                break;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 7) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a loop.
    //

    printf("    test43...");
    Counter = 0;
    for (Index1 = 0; Index1 < 10; Index1 += 1) {
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            break;
        }

        Counter += 6;
    }

    if (Counter != 11) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a try body with an exception clause in a switch.
    //

    printf("    test44...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 40;
        }

        Counter += 2;
        break;
    }

    if (Counter != 0) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a try body with an finally clause in a switch.
    //

    printf("    test45...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                break;

            } else {
                Counter += 1;
            }

        } finally {
            Counter += 2;
        }

        Counter += 3;
    }

    if (Counter != 2) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from doubly nested try body with an exception clause in a
    // switch.
    //

    printf("    test46...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {
                Counter += 10;
            }

            Counter += 2;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Counter += 20;
        }

        Counter += 3;
    }

    if (Counter != 0) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from doubly nested try body with an finally clause in a switch.
    //

    printf("    test47...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    break;

                } else {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 3;

        } finally {
            Counter += 4;
        }

        Counter += 5;
    }

    if (Counter != 6) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a finally clause in a switch.
    //

    printf("    test48...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            if ((Index1 & 0x1) == 1) {
                Counter += 1;
            }

        } finally {
            Counter += 2;
            break;
        }

        Counter += 4;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a switch.
    //

    printf("    test49...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
                break;
            }

            Counter += 4;

        } finally {
            Counter += 5;
        }

        Counter += 6;
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Break from a doubly nested finally clause in a switch.
    //

    printf("    test50...");
    Counter = 0;
    Index1 = 1;
    switch (Index2) {
    case BLUE:
        Counter += 100;
        break;

    case RED:
        try {
            try {
                if ((Index1 & 0x1) == 1) {
                    Counter += 1;
                }

            } finally {
                Counter += 2;
            }

            Counter += 4;

        } finally {
            Counter += 5;
            break;
        }

        Counter += 6;
    }

    if (Counter != 12) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leave from an if in a simple try/finally.
    //

    printf("    test51...");
    Counter = 0;
    try {
        if (Echo(Counter) == Counter) {
            Counter += 3;
            leave;

        } else {
            Counter += 100;
        }

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leave from a loop in a simple try/finally.
    //

    printf("    test52...");
    Counter = 0;
    try {
        for (Index1 = 0; Index1 < 10; Index1 += 1) {
            if (Echo(Index1) == Index1) {
                Counter += 3;
                leave;
            }

            Counter += 100;
        }

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leave from a switch in a simple try/finally.
    //

    printf("    test53...");
    Counter = 0;
    try {
        switch (Index2) {
        case BLUE:
            break;

        case RED:
            Counter += 3;
            leave;
        }

        Counter += 100;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 8) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leave from an if in doubly nested try/finally followed by a leave
    // from an if in the outer try/finally.
    //

    printf("    test54...");
    Counter = 0;
    try {
        try {
            if (Echo(Counter) == Counter) {
                Counter += 3;
                leave;

            } else {
                Counter += 100;
            }

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
            }
        }

        if (Echo(Counter) == Counter) {
            Counter += 3;
            leave;

         } else {
            Counter += 100;
         }


    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 16) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Leave from an if in doubly nested try/finally followed by a leave
    // from the finally of the outer try/finally.
    //

    printf("    test55...");
    Counter = 0;
    try {
        try {
            if (Echo(Counter) == Counter) {
                Counter += 3;
                leave;

            } else {
                Counter += 100;
            }

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
                leave;
            }
        }

        Counter += 100;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 5;
        }
    }

    if (Counter != 13) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Try/finally within the except clause of a try/except that is always
    // executed.
    //

    printf("    test56...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        try {
            Counter += 3;

        } finally {
            if (abnormal_termination() == FALSE) {
                Counter += 5;
            }
        }
    }

    if (Counter != 9) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Try/finally within the finally clause of a try/finally.
    //

    printf("    test57...");
    Counter = 0;
    try {
        Counter += 1;

    } finally {
        if (abnormal_termination() == FALSE) {
            try {
                Counter += 3;

            } finally {
                if (abnormal_termination() == FALSE) {
                    Counter += 5;
                }
            }
        }
    }

    if (Counter != 9) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Try/except within the finally clause of a try/finally.
    //
/*
    printf("    test58...");
    Counter = 0;
    try {
        Counter -= 1;

    } finally {
        try {
            Counter += 2;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } except (Counter) {
            try {
                Counter += 3;

            } finally {
                if (abnormal_termination() == FALSE) {
                    Counter += 5;
                }
            }
        }
    }

    if (Counter != 9) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }
*/
    //
    // Try/except within the except clause of a try/except that is always
    // executed.
    //

    printf("    test59...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        try {
            Counter += 3;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } except(Counter - 3) {
            Counter += 5;
        }
    }

    if (Counter != 9) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Try with a Try which exits the scope with a goto
    //

    printf("    test60...");
    Counter = 0;
    try {
        try {
            goto outside;

        } except(1) {
            Counter += 1;
        }

outside:
    RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except(1) {
        Counter += 3;
    }

    if (Counter != 3) {
        printf("failed, count = %d\n", Counter);
    } else {
        printf("succeeded\n");
    }

    //
    // Try/except which gets an exception from a subfunction within
    // a try/finally which has a try/except in the finally clause
    //

    printf("    test61...");
    Counter = 0;
    try {
        Test61Part2 (&Counter);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Counter += 11;
    }

    if (Counter != 24) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    //
    // Check for precision of floating point exception
    //

    printf("    test62...");

    /* enable floating point overflow */
    _controlfp(_controlfp(0,0) & ~EM_OVERFLOW, _MCW_EM);

    Counter = 0;
    try {
        doubleresult = SquareDouble (1.7e300);

        try {
            doubleresult = SquareDouble (1.0);

        } except (1) {
            Counter += 3;
        }

    } except (1) {
        Counter += 1;
    }

    if (Counter != 1) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    _clearfp ();

    //
    // Callout for test #63 due to bogus compiler behaviour caused by test #62.
    //

    PerformFpTest ();

    //
    // Announce end of exception test.
    //

    printf("End of exception test\n");
    return;
}

VOID
PerformFpTest()
{
    LONG Counter;
    double doubleresult;

    //
    // Check for precision of floating point exception in subfunction
    //

    printf("    test63...");

    Counter = 0;
    try {
        SquareDouble17E300 ((PVOID) &doubleresult);

        try {
            SquareDouble17E300 ((PVOID) &doubleresult);

        } except (1) {
            Counter += 3;
        }

    } except (1) {
        Counter += 1;
    }

    if (Counter != 1) {
        printf("failed, count = %d\n", Counter);

    } else {
        printf("succeeded\n");
    }

    _clearfp ();

}

VOID
addtwo (
    long First,
    long Second,
    long *Place
    )

{

    RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);
    *Place = First + Second;
    return;
}

VOID
bar1 (
    IN NTSTATUS Status,
    IN PLONG Counter
    )
{

    try {
        foo1(Status);

    } finally {
        if (abnormal_termination() != FALSE) {
            *Counter = 99;

        } else {
            *Counter = 100;
        }
    }

    return;
}

VOID
bar2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress,
    IN PLONG Counter
    )
{

    try {
        foo2(BlackHole, BadAddress);

    } finally {
        if (abnormal_termination() != FALSE) {
            *Counter = 99;

        } else {
            *Counter = 100;
        }
    }

    return;
}

VOID
dojump (
    IN jmp_buf JumpBuffer,
    IN PLONG Counter
    )

{

    try {
        try {
            *Counter += 1;
            RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

        } finally {
            *Counter += 1;
        }

    } finally {
        *Counter += 1;
        longjmp(JumpBuffer, 1);
    }
}

VOID
eret(
    IN NTSTATUS Status,
    IN PLONG Counter
    )

{

    try {
        try {
            foo1(Status);

        } except ((GetExceptionCode() == Status) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            *Counter += 1;
            return;
        }

    } finally {
        *Counter += 1;
    }

    return;
}

VOID
except1 (
    IN PLONG Counter
    )

{

    try {
        *Counter += 5;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (except3(GetExceptionInformation(), Counter)) {
        *Counter += 7;
    }

    *Counter += 9;
    return;
}

ULONG
except2 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    )

{

    PEXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    if ((ExceptionRecord->ExceptionCode == STATUS_UNSUCCESSFUL) &&
       ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) == 0)) {
        *Counter += 11;
        return EXCEPTION_EXECUTE_HANDLER;

    } else {
        *Counter += 13;
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

ULONG
except3 (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLONG Counter
    )

{

    PEXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    if ((ExceptionRecord->ExceptionCode == STATUS_INTEGER_OVERFLOW) &&
       ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) == 0)) {
        *Counter += 17;
        RtlRaiseStatus(STATUS_UNSUCCESSFUL);

    } else if ((ExceptionRecord->ExceptionCode == STATUS_UNSUCCESSFUL) &&
        ((ExceptionRecord->ExceptionFlags & EXCEPTION_NESTED_CALL) != 0)) {
        *Counter += 19;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    *Counter += 23;
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
foo1 (
    IN NTSTATUS Status
    )

{

    //
    // Raise exception.
    //

    RtlRaiseStatus(Status);
    return;
}

VOID
foo2 (
    IN PLONG BlackHole,
    IN PLONG BadAddress
    )

{

    //
    // Raise exception.
    //

    *BlackHole += *BadAddress;
    return;
}

VOID
fret(
    IN PLONG Counter
    )

{

    try {
        try {
            *Counter += 1;

        } finally {
            *Counter += 1;
            return;
        }
    } finally {
        *Counter += 1;
    }

    return;
}

LONG
Echo(
    IN LONG Value
    )

{
    return Value;
}

VOID
Test61Part2 (
    IN OUT PULONG Counter
    )
{

    try {
        *Counter -= 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);
    } finally {
        *Counter += 2;
        *Counter += 5;
        *Counter += 7;
    }
}


double
SquareDouble (
    IN double   op
    )
{
    return op * op;
}

VOID
SquareDouble17E300 (
    OUT PVOID   output
    )
{
    double  ans;

    ans = SquareDouble (1.7e300);
    *(double *) output = ans;
}
