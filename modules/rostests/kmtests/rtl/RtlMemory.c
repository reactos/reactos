/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Runtime library memory functions test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <stddef.h>
__declspec(dllimport) void __stdcall RtlMoveMemory(void *, const void *, size_t);
__declspec(dllimport) void __stdcall RtlFillMemory(void *, size_t, unsigned char);

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wnonnull"
#endif /* defined __GNUC__ */

static
VOID
MakeBuffer(
    OUT PVOID Buffer,
    ...)
{
    PUCHAR OutBuffer = Buffer;
    INT Count;
    INT Value;
    va_list Arguments;

    va_start(Arguments, Buffer);

    while (1)
    {
        Count = va_arg(Arguments, INT);
        if (!Count)
            break;
        ASSERT(Count > 0);

        Value = va_arg(Arguments, INT);
        while (Count--)
            *OutBuffer++ = Value;
    }

    va_end(Arguments);
}

static
BOOLEAN
CheckBuffer(
    IN const VOID *Buffer,
    ...)
{
    CONST UCHAR *OutBuffer = Buffer;
    INT Count;
    INT Value;
    va_list Arguments;

    va_start(Arguments, Buffer);

    while (1)
    {
        Count = va_arg(Arguments, INT);
        if (!Count)
            break;
        ASSERT(Count > 0);

        Value = va_arg(Arguments, INT);
        while (Count--)
            if (*OutBuffer++ != Value)
            {
                --OutBuffer;
                trace("CheckBuffer failed at offset %d, value %x, expected %x\n", OutBuffer - (CONST UCHAR*)Buffer, *OutBuffer, Value);
                return FALSE;
            }
    }

    va_end(Arguments);
    return TRUE;
}

static
VOID
MakePattern(
    OUT PVOID Buffer,
    ...)
{
    PUCHAR OutBuffer = Buffer;
    INT Count, Repeat, i;
    INT Values[16];
    va_list Arguments;

    va_start(Arguments, Buffer);

    while (1)
    {
        Count = va_arg(Arguments, INT);
        if (!Count)
            break;
        ASSERT(Count > 0 && Count < sizeof Values / sizeof Values[0]);

        Repeat = va_arg(Arguments, INT);
        ASSERT(Repeat > 0);

        for (i = 0; i < Count; ++i)
            Values[i] = va_arg(Arguments, INT);

        while (Repeat--)
            for (i = 0; i < Count; ++i)
                *OutBuffer++ = Values[i];
    }

    va_end(Arguments);
}

static
BOOLEAN
CheckPattern(
    IN const VOID *Buffer,
    ...)
{
    CONST UCHAR *OutBuffer = Buffer;
    INT Count, Repeat, i;
    INT Values[16];
    va_list Arguments;

    va_start(Arguments, Buffer);

    while (1)
    {
        Count = va_arg(Arguments, INT);
        if (!Count)
            break;
        ASSERT(Count > 0 && Count < sizeof Values / sizeof Values[0]);

        Repeat = va_arg(Arguments, INT);
        ASSERT(Repeat > 0);

        for (i = 0; i < Count; ++i)
            Values[i] = va_arg(Arguments, INT);

        while (Repeat--)
            for (i = 0; i < Count; ++i)
                if (*OutBuffer++ != Values[i])
                {
                    --OutBuffer;
                    trace("CheckPattern failed at offset %d, value %x, expected %x\n", OutBuffer - (CONST UCHAR*)Buffer, *OutBuffer, Values[i]);
                    return FALSE;
                }
    }

    va_end(Arguments);
    return TRUE;
}

START_TEST(RtlMemory)
{
    NTSTATUS Status;
    UCHAR Buffer[513];
    const SIZE_T Size = 512;
    const SIZE_T HalfSize = Size / 2;
    SIZE_T RetSize;
    KIRQL Irql;
    SIZE_T i;

    KeRaiseIrql(HIGH_LEVEL, &Irql);
    /* zero everything behind 'Size'. Tests will check that this wasn't changed.
     * TODO: use guarded memory for this! */
    MakeBuffer(Buffer + Size, sizeof Buffer - Size, 0, 0);

    /* test our helper functions first */
    MakeBuffer(Buffer, HalfSize, 0x55, HalfSize, 0xAA, 0);
    for (i = 0; i < HalfSize; ++i)
        ok_eq_uint(Buffer[i], 0x55);
    for (i = HalfSize; i < Size; ++i)
        ok_eq_uint(Buffer[i], 0xAA);
    ok_bool_true(CheckBuffer(Buffer, HalfSize, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");

    MakePattern(Buffer, 3, 20, 0x11, 0x22, 0x33, 1, 4, 0x44, 0);
    for (i = 0; i < 60; i += 3)
    {
        ok_eq_uint(Buffer[i+0], 0x11);
        ok_eq_uint(Buffer[i+1], 0x22);
        ok_eq_uint(Buffer[i+2], 0x33);
    }
    for (i = 60; i < 64; ++i)
        ok_eq_uint(Buffer[i], 0x44);
    for (i = 64; i < HalfSize; ++i)
        ok_eq_uint(Buffer[i], 0x55);
    for (i = HalfSize; i < Size; ++i)
        ok_eq_uint(Buffer[i], 0xAA);
    ok_bool_true(CheckPattern(Buffer, 3, 20, 0x11, 0x22, 0x33, 1, 4, 0x44, 0), "CheckPattern");

    /* RtlMoveMemory */
    MakePattern(Buffer, 2, 64, 0x12, 0x34, 2, 192, 0x56, 0x78, 0);
    RtlMoveMemory(Buffer + 13, Buffer + 62, 95);
    ok_bool_true(CheckPattern(Buffer, 2, 6, 0x12, 0x34, 1, 1, 0x12, 2, 33, 0x12, 0x34, 2, 14, 0x56, 0x78, 1, 1, 0x56, 2, 10, 0x12, 0x34, 2, 192, 0x56, 0x78, 1, 1, 0, 0), "CheckPattern");

    MakePattern(Buffer, 2, 32, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 192, 0x9A, 0xAB, 0);
    RtlMoveMemory(Buffer + 78, Buffer + 43, 107);
    ok_bool_true(CheckPattern(Buffer, 2, 32, 0x12, 0x34, 2, 7, 0x56, 0x78, 1, 1, 0x34, 2, 10, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 11, 0x9A, 0xAB, 1, 1, 0xAB, 2, 163, 0x9A, 0xAB, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlMoveMemory(NULL, NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

#undef RtlMoveMemory
    /* RtlMoveMemory export */
    MakePattern(Buffer, 2, 64, 0x12, 0x34, 2, 192, 0x56, 0x78, 0);
    RtlMoveMemory(Buffer + 13, Buffer + 62, 95);
    ok_bool_true(CheckPattern(Buffer, 2, 6, 0x12, 0x34, 1, 1, 0x12, 2, 33, 0x12, 0x34, 2, 14, 0x56, 0x78, 1, 1, 0x56, 2, 10, 0x12, 0x34, 2, 192, 0x56, 0x78, 1, 1, 0, 0), "CheckPattern");

    MakePattern(Buffer, 2, 32, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 192, 0x9A, 0xAB, 0);
    RtlMoveMemory(Buffer + 78, Buffer + 43, 107);
    ok_bool_true(CheckPattern(Buffer, 2, 32, 0x12, 0x34, 2, 7, 0x56, 0x78, 1, 1, 0x34, 2, 10, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 11, 0x9A, 0xAB, 1, 1, 0xAB, 2, 163, 0x9A, 0xAB, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlMoveMemory(NULL, NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlCopyMemory */
    MakePattern(Buffer, 2, 64, 0x12, 0x34, 2, 192, 0x56, 0x78, 0);
    RtlCopyMemory(Buffer + 13, Buffer + 62, 95);
    ok_bool_true(CheckPattern(Buffer, 2, 6, 0x12, 0x34, 1, 1, 0x12, 2, 33, 0x12, 0x34, 2, 14, 0x56, 0x78, 1, 1, 0x56, 2, 10, 0x12, 0x34, 2, 192, 0x56, 0x78, 1, 1, 0, 0), "CheckPattern");

    MakePattern(Buffer, 2, 32, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 192, 0x9A, 0xAB, 0);
    RtlCopyMemory(Buffer + 78, Buffer + 43, 107);
    ok_bool_true(CheckPattern(Buffer, 2, 32, 0x12, 0x34, 2, 7, 0x56, 0x78, 1, 1, 0x34, 2, 10, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 11, 0x9A, 0xAB, 1, 1, 0xAB, 2, 163, 0x9A, 0xAB, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlCopyMemory(NULL, NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlCopyMemoryNonTemporal */
    MakePattern(Buffer, 2, 64, 0x12, 0x34, 2, 192, 0x56, 0x78, 0);
    RtlCopyMemoryNonTemporal(Buffer + 13, Buffer + 62, 95);
    ok_bool_true(CheckPattern(Buffer, 2, 6, 0x12, 0x34, 1, 1, 0x12, 2, 33, 0x12, 0x34, 2, 14, 0x56, 0x78, 1, 1, 0x56, 2, 10, 0x12, 0x34, 2, 192, 0x56, 0x78, 1, 1, 0, 0), "CheckPattern");

    MakePattern(Buffer, 2, 32, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 192, 0x9A, 0xAB, 0);
    RtlCopyMemoryNonTemporal(Buffer + 78, Buffer + 43, 107);
    ok_bool_true(CheckPattern(Buffer, 2, 32, 0x12, 0x34, 2, 7, 0x56, 0x78, 1, 1, 0x34, 2, 10, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 11, 0x9A, 0xAB, 1, 1, 0xAB, 2, 163, 0x9A, 0xAB, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlCopyMemoryNonTemporal(NULL, NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlCopyBytes */
    MakePattern(Buffer, 2, 64, 0x12, 0x34, 2, 192, 0x56, 0x78, 0);
    RtlCopyBytes(Buffer + 13, Buffer + 62, 95);
    ok_bool_true(CheckPattern(Buffer, 2, 6, 0x12, 0x34, 1, 1, 0x12, 2, 33, 0x12, 0x34, 2, 14, 0x56, 0x78, 1, 1, 0x56, 2, 10, 0x12, 0x34, 2, 192, 0x56, 0x78, 1, 1, 0, 0), "CheckPattern");

    MakePattern(Buffer, 2, 32, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 192, 0x9A, 0xAB, 0);
    RtlCopyBytes(Buffer + 78, Buffer + 43, 107);
    ok_bool_true(CheckPattern(Buffer, 2, 32, 0x12, 0x34, 2, 7, 0x56, 0x78, 1, 1, 0x34, 2, 10, 0x12, 0x34, 2, 32, 0x56, 0x78, 2, 11, 0x9A, 0xAB, 1, 1, 0xAB, 2, 163, 0x9A, 0xAB, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlCopyBytes(NULL, NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlEqualMemory */
    MakePattern(Buffer, 8, HalfSize / 8 - 1, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                        1, 1,                0x12,
                        8, HalfSize / 8,     0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                        1, 7,                0x12, 0);

    ok_bool_true(RtlEqualMemory((PVOID)1, (PVOID)2, 0),
                 "RtlEqualMemory returned");
    ok_bool_true(RtlEqualMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8),
                 "RtlEqualMemory returned");
    ok_bool_true(RtlEqualMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8 + 1),
                 "RtlEqualMemory returned");
    ok_bool_false(RtlEqualMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8 + 2),
                  "RtlEqualMemory returned");

    /* RtlCompareMemory */
    MakePattern(Buffer, 8, HalfSize / 8 - 1, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                        1, 1,                0x12,
                        8, HalfSize / 8,     0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                        1, 7,                0x12, 0);

    RetSize = RtlCompareMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8);
    ok_eq_size(RetSize, HalfSize - 8);
    RetSize = RtlCompareMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8 + 1);
    ok_eq_size(RetSize, HalfSize - 8 + 1);
    RetSize = RtlCompareMemory(Buffer, Buffer + HalfSize - 7, HalfSize - 8 + 2);
    ok_eq_size(RetSize, HalfSize - 8 + 1);

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RetSize = RtlCompareMemory(Buffer, Buffer + HalfSize - 7, SIZE_MAX);
        ok_eq_size(RetSize, HalfSize - 8 + 1);
        RetSize = RtlCompareMemory(NULL, NULL, 0);
        ok_eq_size(RetSize, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlCompareMemoryUlong */
    MakeBuffer(Buffer, 8, 0x55, Size - 8, 0, 0);
    RetSize = RtlCompareMemoryUlong(Buffer, sizeof(ULONG), 0x55555555LU);
    ok_eq_size(RetSize, 4);
    RetSize = RtlCompareMemoryUlong(Buffer + 1, sizeof(ULONG), 0x55555555LU);
    ok_eq_size(RetSize, 4);
    RetSize = RtlCompareMemoryUlong(Buffer + 2, sizeof(ULONG), 0x55555555LU);
    ok_eq_size(RetSize, 4);
    RetSize = RtlCompareMemoryUlong(Buffer + 3, sizeof(ULONG), 0x55555555LU);
    ok_eq_size(RetSize, 4);
    RetSize = RtlCompareMemoryUlong(Buffer + 5, sizeof(ULONG), 0x55555555LU);
    ok_eq_size(RetSize, 0);
    RetSize = RtlCompareMemoryUlong(Buffer + 5, sizeof(ULONG), 0x00555555LU);
    ok_eq_size(RetSize, 4);
    RetSize = RtlCompareMemoryUlong(Buffer, 1, 0x55555555LU);
    ok_eq_size(RetSize, 0);
    RetSize = RtlCompareMemoryUlong(Buffer, 2, 0x55555555LU);
    ok_eq_size(RetSize, 0);
    RetSize = RtlCompareMemoryUlong(Buffer, 3, 0x55555555LU);
    ok_eq_size(RetSize, 0);
    RetSize = RtlCompareMemoryUlong(Buffer, 5, 0x55555555LU);
    ok_eq_size(RetSize, 4);

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RetSize = RtlCompareMemoryUlong(NULL, 0, 0x55555555LU);
        ok_eq_size(RetSize, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlZeroMemory */
    MakeBuffer(Buffer, Size, 0x11, 0);
    RtlZeroMemory(Buffer, 1);
    ok_bool_true(CheckBuffer(Buffer, 1, 0, Size - 1, 0x11, 1, 0, 0), "CheckBuffer");
    Buffer[0] = 0x11;
    RtlZeroMemory(Buffer, Size - 1);
    ok_bool_true(CheckBuffer(Buffer, Size - 1, 0, 1, 0x11, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlZeroMemory(NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlSecureZeroMemory */
    MakeBuffer(Buffer, Size, 0x11, 0);
    RtlSecureZeroMemory(Buffer, 1);
    ok_bool_true(CheckBuffer(Buffer, 1, 0, Size - 1, 0x11, 1, 0, 0), "CheckBuffer");
    Buffer[0] = 0x11;
    RtlSecureZeroMemory(Buffer, Size - 1);
    ok_bool_true(CheckBuffer(Buffer, Size - 1, 0, 1, 0x11, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlSecureZeroMemory(NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlZeroBytes */
    MakeBuffer(Buffer, Size, 0x11, 0);
    RtlZeroBytes(Buffer, 1);
    ok_bool_true(CheckBuffer(Buffer, 1, 0, Size - 1, 0x11, 1, 0, 0), "CheckBuffer");
    Buffer[0] = 0x11;
    RtlZeroBytes(Buffer, Size - 1);
    ok_bool_true(CheckBuffer(Buffer, Size - 1, 0, 1, 0x11, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlZeroBytes(NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlFillMemory */
    MakeBuffer(Buffer, Size, 0, 0);
    RtlFillMemory(Buffer, HalfSize, 0x55);
    RtlFillMemory(Buffer + HalfSize, HalfSize, 0xAA);
    ok_bool_true(CheckBuffer(Buffer, HalfSize, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");
    RtlFillMemory(Buffer + 3, 7, 0x88);
    ok_bool_true(CheckBuffer(Buffer, 3, 0x55, 7, 0x88, HalfSize - 10, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
#if defined(__GNUC__) && __GNUC__ >= 5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmemset-transposed-args"
#endif
        RtlFillMemory(NULL, 0, 0x55);
#if defined(__GNUC__) && __GNUC__ >= 5
#pragma GCC diagnostic pop
#endif
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

#undef RtlFillMemory
    /* RtlFillMemory export */
    MakeBuffer(Buffer, Size, 0, 0);
    RtlFillMemory(Buffer, HalfSize, 0x55);
    RtlFillMemory(Buffer + HalfSize, HalfSize, 0xAA);
    ok_bool_true(CheckBuffer(Buffer, HalfSize, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");
    RtlFillMemory(Buffer + 3, 7, 0x88);
    ok_bool_true(CheckBuffer(Buffer, 3, 0x55, 7, 0x88, HalfSize - 10, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlFillMemory(NULL, 0, 0x55);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* TODO: fix NDK. This should work! */
#if !defined _M_AMD64 || defined KMT_KERNEL_MODE
    /* RtlFillMemoryUlong */
    MakeBuffer(Buffer, Size, 0, 0);
    RtlFillMemoryUlong(Buffer, HalfSize, 0x01234567LU);
    RtlFillMemoryUlong(Buffer + HalfSize, HalfSize, 0x89ABCDEFLU);
    ok_bool_true(CheckPattern(Buffer, 4, HalfSize / 4, 0x67, 0x45, 0x23, 0x01, 4, HalfSize / 4, 0xEF, 0xCD, 0xAB, 0x89, 1, 1, 0, 0), "CheckPattern");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        MakeBuffer(Buffer, Size, 0, 0);
        RtlFillMemoryUlong(Buffer + 1, sizeof(ULONG), 0xAAAAAAAALU);
        ok_bool_true(CheckBuffer(Buffer, 1, 0, sizeof(ULONG), 0xAA, Size - sizeof(ULONG) - 1, 0, 1, 0, 0), "CheckBuffer");

        RtlFillMemoryUlong(NULL, 0, 0x55555555LU);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);
#endif

    /* RtlFillMemoryUlonglong */
    /* TODO: this function doesn't exist in 2k3/x86? wdm.h error? */

    /* RtlFillBytes */
    MakeBuffer(Buffer, Size, 0, 0);
    RtlFillBytes(Buffer, HalfSize, 0x55);
    RtlFillBytes(Buffer + HalfSize, HalfSize, 0xAA);
    ok_bool_true(CheckBuffer(Buffer, HalfSize, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");
    RtlFillBytes(Buffer + 3, 7, 0x88);
    ok_bool_true(CheckBuffer(Buffer, 3, 0x55, 7, 0x88, HalfSize - 10, 0x55, HalfSize, 0xAA, 1, 0, 0), "CheckBuffer");

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlFillBytes(NULL, 0, 0x55);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    /* RtlPrefetchMemoryNonTemporal */
    RtlPrefetchMemoryNonTemporal(Buffer, Size);

    KeLowerIrql(Irql);
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        RtlPrefetchMemoryNonTemporal(NULL, 0);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    KeLowerIrql(Irql);
}
