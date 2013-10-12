/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Guarded Memory example test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wnonnull"
#endif /* defined __GNUC__ */

START_TEST(GuardedMemory)
{
    NTSTATUS Status;
    SIZE_T Size = 123;
    PCHAR *Buffer;

    /* access some invalid memory to test SEH */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        RtlFillMemory(NULL, 1, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);

    /* get guarded mem */
    Buffer = KmtAllocateGuarded(Size);

    if (skip(Buffer != NULL, "Failed to allocate guarded memory\n"))
        return;

    /* access to guarded mem should be fine */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        RtlFillMemory(Buffer, Size, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* access one byte behind guarded mem must cause an access violation! */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        RtlFillMemory(Buffer + Size, 1, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);

    KmtFreeGuarded(Buffer);
}
