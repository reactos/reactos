/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for RtlDispatchException
* PROGRAMMER:      Timo Kreuzer
*/

#include "precomp.h"

// ASM function in UnwindStubs.S
VOID
LeafFunction(
    PVOID* p,
    PVOID x);

LONG ExceptionFilter1(
    PEXCEPTION_POINTERS ExceptionInfo,
    ULONG ExceptionCode)
{
    return EXCEPTION_EXECUTE_HANDLER;
}

void
Test_LeafFunction()
{
    volatile INT Status = 0;

    _SEH2_TRY
    {
        LeafFunction(NULL, NULL);
    }
    _SEH2_EXCEPT(ExceptionFilter1(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_int(Status, STATUS_ACCESS_VIOLATION);

}

DECLSPEC_NOINLINE
void
CorruptStackFunction()
{
    *(PULONG64)_AddressOfReturnAddress() = -1;
    LeafFunction(NULL, 0);
}

void
Test_CorruptStack()
{
    volatile INT Status = 0;

    _SEH2_TRY
    {
        CorruptStackFunction();
    }
    _SEH2_EXCEPT(ExceptionFilter1(_SEH2_GetExceptionInformation(), _SEH2_GetExceptionCode()))
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_int(Status, STATUS_ACCESS_VIOLATION);

}

void
Test_TailCall()
{
    return;
}

START_TEST(RtlDispatchException)
{
    Test_LeafFunction();
    Test_CorruptStack();
    Test_TailCall();
}
