/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Exception test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

#define StartSeh()                  ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus) }    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok_eq_hex(ExceptionStatus, ExpectedStatus)

START_TEST(RtlException)
{
    NTSTATUS ExceptionStatus;
    PCHAR Buffer[128];
    CHAR Value;

    /* Access a valid pointer - must not trigger SEH */
    StartSeh()
        RtlFillMemory(Buffer, sizeof(Buffer), 0x12);
    EndSeh(STATUS_SUCCESS);

    /* Read from a NULL pointer - must cause an access violation */
    StartSeh()
        Value = *(volatile CHAR *)NULL;
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Write to a NULL pointer - must cause an access violation */
    StartSeh()
        *(volatile CHAR *)NULL = 5;
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* TODO: Find where MmBadPointer is defined - gives an unresolved external */
#if 0 //def KMT_KERNEL_MODE
    /* Read from MmBadPointer - must cause an access violation */
    StartSeh()
        Value = *(volatile CHAR *)MmBadPointer;
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Write to MmBadPointer - must cause an access violation */
    StartSeh()
        *(volatile CHAR *)MmBadPointer = 5;
    EndSeh(STATUS_ACCESS_VIOLATION);
#endif

    /* We cannot test this in kernel mode easily - the stack is just "somewhere"
     * in system space, and there's no guard page below it */
#ifdef KMT_USER_MODE
    /* Overflow the stack - must cause a special exception */
    StartSeh()
        PCHAR Pointer;

        while (1)
        {
            Pointer = _alloca(1024);
            *Pointer = 5;
        }
    EndSeh(STATUS_STACK_OVERFLOW);
#endif
}
