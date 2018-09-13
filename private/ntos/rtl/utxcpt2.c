//  utxcpt2.c - user mode structured exception handling test 2
//
//  Exception test from Markl.
//

#include <ntos.h>

VOID
ExceptionTest (
    )

//
// This routine tests the structured exception handling capabilities of the
// MS C compiler and the NT exception handling facilities.
//

{

    EXCEPTION_RECORD ExceptionRecord;
    LONG Counter;
    ULONG rv;

    //
    // Announce start of exception test.
    //

    DbgPrint("Start of exception test\n");

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionCode = (NTSTATUS)49;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 1;
    ExceptionRecord.ExceptionInformation[0] = 9;

    //
    // Simply try statement with a finally clause that is entered sequentially.
    //
    DbgPrint("t1...");
    Counter = 0;
    try {
        Counter += 1;
    } finally {
        if (abnormal_termination() == 0) {
            Counter += 1;
        }
    }
    if (Counter != 2) {
        DbgPrint("BUG  Finally clause executed as result of unwind\n");
    }
    DbgPrint("done\n");

    //
    // Simple try statement with an exception clause that is never executed
    // because there is no exception raised in the try clause.
    //
//  goto a;
    DbgPrint("t2...");
    Counter = 0;
    try {
//a:	    Counter += 1;
	  Counter += 1;
    } except (Counter) {
        Counter += 1;
    }
    if (Counter != 1) {
        DbgPrint("BUG  Exception clause executed when it shouldn't be\n");
    }
    DbgPrint("done\n");

    //
    // Simple try statement with an exception handler that is never executed
    // because the exception expression continues execution.
    //
    DbgPrint("t3...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = 0;
    try {
        Counter -= 1;
        RtlRaiseException(&ExceptionRecord);
    } except (Counter) {
        Counter -= 1;
    }
    if (Counter != - 1) {
        DbgPrint("BUG  Exception clause executed when it shouldn't be\n");
    }
    DbgPrint("done\n");

    //
    // Simple try statement with an exception clause that is always executed.
    //
    DbgPrint("t4...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        Counter += 1;
        RtlRaiseException(&ExceptionRecord);
    } except (Counter) {
        Counter += 1;
    }
    if (Counter != 2) {
        DbgPrint("BUG  Exception clause not executed when it should be\n");
    }
    DbgPrint("done\n");

    //
    // Simply try statement with a finally clause that is entered as the
    // result of an exception.
    //

    DbgPrint("t5...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = 0;
    try {
        try {
            Counter += 1;
            RtlRaiseException(&ExceptionRecord);
        } finally {
            if (abnormal_termination() != 0) {
                Counter += 1;
            }
        }
    } except (Counter) {
        if (Counter == 2) {
            Counter += 1;
        }
    }
    if (Counter != 3) {
        DbgPrint("BUG  Finally clause executed as result of sequential exit\n");
    }
    DbgPrint("done\n");

    //
    // Simple try that calls a function which raises an exception.
    //
    DbgPrint("t6...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        VOID foo(IN NTSTATUS Status);

        Counter += 1;
        foo(STATUS_ACCESS_VIOLATION);
    } except (exception_code() == STATUS_ACCESS_VIOLATION) {
        Counter += 1;
    }
    if (Counter != 2) {
        DbgPrint("BUG  Exception clause not executed when it should be\n");
    }
    DbgPrint("done\n");

    //
    // Simple try that calls a function which calls a function that
    // raises an exception. The first function has a finally clause
    // that must be executed for this test to work.
    //
    DbgPrint("t7...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        VOID bar(IN NTSTATUS Status, IN PULONG Counter);

        bar(STATUS_ACCESS_VIOLATION, &Counter);

    } except (exception_code() == STATUS_ACCESS_VIOLATION) {
        if (Counter != 99) {
            DbgPrint("BUG  finally in called procedure not executed\n");
        }
    }
    DbgPrint("done\n");

    //
    // A try within an except
    //
    DbgPrint("t8...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {

        foo(STATUS_ACCESS_VIOLATION);

    } except (exception_code() == STATUS_ACCESS_VIOLATION) {

        Counter++;

        try {

            foo(STATUS_SUCCESS);

        } except (exception_code() == STATUS_SUCCESS) {
            if ( Counter != 1 ) {
                DbgPrint("BUG  Previous Handler not Entered\n");
            }
            Counter++;

        }
    }
    if (Counter != 2) {
        DbgPrint("BUG Both Handlers not entered\n");
    }
    DbgPrint("done\n");

    //
    // A goto from an exception clause that needs to pass
    // through a finally
    //
    DbgPrint("t9...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        try {
            foo(STATUS_ACCESS_VIOLATION);
        } except (exception_code() == STATUS_ACCESS_VIOLATION) {
            Counter++;
            goto t9;
        }
    } finally {
        Counter++;
    }
t9:
    if (Counter != 2) {
        DbgPrint("BUG Finally and Exception Handlers not entered\n");
    }
    DbgPrint("done\n");

    //
    // A goto from an exception clause that needs to pass
    // through a finally
    //
    DbgPrint("t10...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    try {
        try {
            Counter++;
        } finally {
            Counter++;
            goto t10;
        }
    } finally {
        Counter++;
    }
t10:
    if (Counter != 3) {
        DbgPrint("BUG Both Finally Handlers not entered\n");
    }
    DbgPrint("done\n");

    //
    // A return from an except clause
    //
    DbgPrint("t11...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    try {
        ULONG eret(IN NTSTATUS Status, IN PULONG Counter);

        Counter++;
        rv = eret(STATUS_ACCESS_VIOLATION, &Counter);
    } finally {
        Counter++;
    }

    if (Counter != 4) {
        DbgPrint("BUG Both Finally Handlers and Exception Handler not entered\n");
    }
    if (rv != 0xDEADBEEF) {
        DbgPrint("BUG rv is wrong\n");
    }
    DbgPrint("done\n");

    //
    // A return from a finally clause
    //
    DbgPrint("t12...");
    Counter = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    try {
        VOID fret(IN PULONG Counter);

        Counter++;
        fret(&Counter);
    } finally {
        Counter++;
    }

    if (Counter != 5) {
        DbgPrint("BUG All three Finally Handlers not entered\n");
    }
    DbgPrint("done\n");
    //
    // Announce end of exception test.
    //

    DbgPrint("End of exception test\n");

    return;
}

main()
{
    ExceptionTest ();
}


NTSTATUS
ZwLastChance (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )
{
    DbgPrint("ZwLastChance Entered\n");;
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
}
ULONG
eret(
    IN NTSTATUS Status,
    IN PULONG Counter
    )
{

    EXCEPTION_RECORD ExceptionRecord;

    try {

        try {
            foo(Status);
        } except (exception_code() == Status) {
            *Counter += 1;
            return 0xDEADBEEF;
        }
    } finally {
        *Counter += 1;
    }
}
VOID
bar(
    IN NTSTATUS Status,
    IN PULONG Counter
    )
{

    EXCEPTION_RECORD ExceptionRecord;

    try {
        foo(Status);
    }

    finally {
        if (abnormal_termination() != 0) {
            *Counter = 99;
        } else {
            *Counter = 100;
        }
    }
}

VOID
foo(
    IN NTSTATUS Status
    )
{
    EXCEPTION_RECORD ExceptionRecord;
    LONG Counter;

    //
    // Initialize exception record.
    //

    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionCode = Status;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 0;
    RtlRaiseException(&ExceptionRecord);
}
