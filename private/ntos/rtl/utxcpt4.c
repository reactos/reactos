//  utxcpt4.c - user mode seh test #3.

#include <ntos.h>
//
// Define function prototypes.
//

VOID
bar (
    IN NTSTATUS Status,
    IN PULONG Counter
    );

VOID
eret (
    IN NTSTATUS Status,
    IN PULONG Counter
    );

VOID
foo (
    IN NTSTATUS Status
    );

VOID
fret (
    IN PULONG Counter
    );

BOOLEAN
Tkm (
    VOID
    );

//
// Define static storage.
//

PTESTFCN TestFunction = Tkm;

main()
{
    Tkm();
}

BOOLEAN
Tkm (
    )

{

    EXCEPTION_RECORD ExceptionRecord;
    LONG Counter;

    //
    // Announce start of exception test.
    //

    DbgPrint("Start of exception test\n");

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionCode = STATUS_INTEGER_OVERFLOW;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;

    //
    // Simply try statement with a finally clause that is entered sequentially.
    //

    DbgPrint("    test1...");
    Counter = 0;
    try {
        Counter += 1;

    } finally {
        if (abnormal_termination() == FALSE) {
            Counter += 1;
        }
    }

    if (Counter != 2) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is never executed
    // because there is no exception raised in the try clause.
    //

    DbgPrint("    test2...");
    Counter = 0;
    try {
        Counter += 1;

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 1) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception handler that is never executed
    // because the exception expression continues execution.
    //

    DbgPrint("    test3...");
    Counter = 0;
    try {
        Counter -= 1;
        RtlRaiseException(&ExceptionRecord);

    } except (Counter) {
        Counter -= 1;
    }

    if (Counter != - 1) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try statement with an exception clause that is always executed.
    //

    DbgPrint("    test4...");
    Counter = 0;
    try {
        Counter += 1;
        RtlRaiseStatus(STATUS_INTEGER_OVERFLOW);

    } except (Counter) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    DbgPrint("    test5...");
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
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which raises an exception.
    //

    DbgPrint("    test6...");
    Counter = 0;
    try {
        Counter += 1;
        foo(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
    }

    if (Counter != 2) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //

    DbgPrint("    test7...");
    Counter = 0;
    try {
        bar(STATUS_ACCESS_VIOLATION, (PULONG)&Counter);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter -= 1;
    }

    if (Counter != 98) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A try within an except
    //

    DbgPrint("    test8...");
    Counter = 0;
    try {
        foo(STATUS_ACCESS_VIOLATION);

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Counter += 1;
        try {
            foo(STATUS_SUCCESS);

        } except ((GetExceptionCode() == STATUS_SUCCESS) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            if (Counter != 1) {
                DbgPrint("failed...");

            } else {
                DbgPrint("succeeded...");
            }

            Counter += 1;
        }
    }

    if (Counter != 2) {
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally
    //

    DbgPrint("    test9...");
    Counter = 0;
    try {
        try {
            foo(STATUS_ACCESS_VIOLATION);

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
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally
    //

    DbgPrint("    test10...");
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
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an exception clause that needs to pass
    // through a finally into the outer finally clause.
    //

    DbgPrint("    test11...");
    Counter = 0;
    try {
        try {
            try {
                Counter += 1;
                foo(STATUS_INTEGER_OVERFLOW);

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
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A goto from an finally clause that needs to pass
    // through a finally into the outer finally clause.
    //

    DbgPrint("    test12...");
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
        DbgPrint("failed\n");

    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A return from an except clause
    //

    DbgPrint("    test13...");
    Counter = 0;
    try {
        Counter += 1;
        eret(STATUS_ACCESS_VIOLATION, (PULONG)&Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 4) {
        DbgPrint("failed\n");
    } else {
        DbgPrint("succeeded\n");
    }

    //
    // A return from a finally clause
    //

    DbgPrint("    test14...");
    Counter = 0;
    try {
        Counter += 1;
        fret((PULONG)&Counter);

    } finally {
        Counter += 1;
    }

    if (Counter != 5) {
        DbgPrint("failed\n");
    } else {
        DbgPrint("succeeded\n");
    }

    //
    // Announce end of exception test.
    //

    DbgPrint("End of exception test\n");
    return TRUE;
}

VOID
fret(
    IN PULONG Counter
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

VOID
eret(
    IN NTSTATUS Status,
    IN PULONG Counter
    )

{

    try {
        try {
            foo(Status);

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
bar (
    IN NTSTATUS Status,
    IN PULONG Counter
    )
{

    try {
        foo(Status);

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
foo(
    IN NTSTATUS Status
    )

{

    //
    // Raise exception.
    //

    RtlRaiseStatus(Status);
    return;
}
