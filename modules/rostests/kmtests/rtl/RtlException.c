/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Exception test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

static
VOID
PossiblyRaise(
    _In_ BOOLEAN Raise)
{
    if (Raise)
    {
        ExRaiseStatus(STATUS_ASSERTION_FAILURE);
    }
}

static
VOID
InnerFunction(
    _Inout_ PULONG State,
    _In_ BOOLEAN Raise)
{
    _SEH2_VOLATILE INT Var = 123;
    static _SEH2_VOLATILE INT *AddressOfVar;

    AddressOfVar = &Var;
    ok_eq_ulong(*State, 1);
    _SEH2_TRY
    {
        *State = 2;
        PossiblyRaise(Raise);
        ok_eq_ulong(*State, 2);
        *State = 3;
    }
    _SEH2_FINALLY
    {
        ok_eq_int(Var, 123);
        ok_eq_pointer(&Var, AddressOfVar);
        if (Raise)
            ok_eq_ulong(*State, 2);
        else
            ok_eq_ulong(*State, 3);
        *State = 4;
    }
    _SEH2_END;

    ok_eq_int(Var, 123);
    ok_eq_pointer(&Var, AddressOfVar);
    ok_eq_ulong(*State, 4);
    *State = 5;
}

static
VOID
OuterFunction(
    _Inout_ PULONG State,
    _In_ BOOLEAN Raise)
{
    _SEH2_VOLATILE INT Var = 456;
    static _SEH2_VOLATILE INT *AddressOfVar;

    AddressOfVar = &Var;
    ok_eq_ulong(*State, 0);
    _SEH2_TRY
    {
        *State = 1;
        InnerFunction(State, Raise);
        ok_eq_ulong(*State, 5);
        *State = 6;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok_eq_int(Var, 456);
        ok_eq_pointer(&Var, AddressOfVar);
        ok_eq_ulong(*State, 4);
        *State = 7;
    }
    _SEH2_END;

    ok_eq_int(Var, 456);
    ok_eq_pointer(&Var, AddressOfVar);
    if (Raise)
        ok_eq_ulong(*State, 7);
    else
        ok_eq_ulong(*State, 6);
    *State = 8;
}

static
VOID
TestNestedExceptionHandler(VOID)
{
    ULONG State;

    State = 0;
    OuterFunction(&State, FALSE);
    ok_eq_ulong(State, 8);

    State = 0;
    OuterFunction(&State, TRUE);
    ok_eq_ulong(State, 8);
}

START_TEST(RtlException)
{
    PCHAR Buffer[128];

    /* Access a valid pointer - must not trigger SEH */
    KmtStartSeh()
        RtlFillMemory(Buffer, sizeof(Buffer), 0x12);
    KmtEndSeh(STATUS_SUCCESS);

    /* Read from a NULL pointer - must cause an access violation */
    KmtStartSeh()
        (void)*(volatile CHAR *)NULL;
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    /* Write to a NULL pointer - must cause an access violation */
    KmtStartSeh()
        *(volatile CHAR *)NULL = 5;
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    /* TODO: Find where MmBadPointer is defined - gives an unresolved external */
#if 0 //def KMT_KERNEL_MODE
    /* Read from MmBadPointer - must cause an access violation */
    KmtStartSeh()
        (void)*(volatile CHAR *)MmBadPointer;
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    /* Write to MmBadPointer - must cause an access violation */
    KmtStartSeh()
        *(volatile CHAR *)MmBadPointer = 5;
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
#endif

    KmtStartSeh()
        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    KmtStartSeh()
        ExRaiseStatus(STATUS_TIMEOUT);
    KmtEndSeh(STATUS_TIMEOUT);

    KmtStartSeh()
        ExRaiseStatus(STATUS_STACK_OVERFLOW);
    KmtEndSeh(STATUS_STACK_OVERFLOW);

    KmtStartSeh()
        ExRaiseStatus(STATUS_GUARD_PAGE_VIOLATION);
    KmtEndSeh(STATUS_GUARD_PAGE_VIOLATION);

    TestNestedExceptionHandler();

    /* We cannot test this in kernel mode easily - the stack is just "somewhere"
     * in system space, and there's no guard page below it */
#ifdef KMT_USER_MODE
    /* Overflow the stack - must cause a special exception */
    KmtStartSeh()
        volatile CHAR *Pointer;

        while (1)
        {
            Pointer = _alloca(1024);
            *Pointer = 5;
        }
    KmtEndSeh(STATUS_STACK_OVERFLOW);
#endif
}
