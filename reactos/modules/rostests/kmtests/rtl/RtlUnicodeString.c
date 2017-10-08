/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Runtime library unicode string functions test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

static
VOID
TestFindCharInUnicodeString(VOID)
{
#ifdef KMT_USER_MODE
    NTSTATUS Status;
    UNICODE_STRING String = RTL_CONSTANT_STRING(L"I am a string");
    UNICODE_STRING Chars = RTL_CONSTANT_STRING(L"a");
    UNICODE_STRING Chars2 = RTL_CONSTANT_STRING(L"A");
    UNICODE_STRING Chars3 = RTL_CONSTANT_STRING(L"i");
    UNICODE_STRING Chars4 = RTL_CONSTANT_STRING(L"G");
    UNICODE_STRING ZeroLengthString;
    USHORT Position;
    ULONG LongPosition;

    RtlInitUnicodeString(&ZeroLengthString, NULL);

    /* Basic checks. Covered by Winetests */
    Position = 123;
    Status = RtlFindCharInUnicodeString(0, &String, &Chars, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 6);

    Position = 123;
    Status = RtlFindCharInUnicodeString(1, &String, &Chars, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 10);

    Position = 123;
    Status = RtlFindCharInUnicodeString(2, &String, &Chars, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 2);

    Position = 123;
    Status = RtlFindCharInUnicodeString(3, &String, &Chars, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 24);

    /* Show that checks are case sensitive by default */
    Position = 123;
    Status = RtlFindCharInUnicodeString(0, &String, &Chars2, &Position);
    ok_eq_hex(Status, STATUS_NOT_FOUND);
    ok_eq_uint(Position, 0);

    Position = 123;
    Status = RtlFindCharInUnicodeString(1, &String, &Chars2, &Position);
    ok_eq_hex(Status, STATUS_NOT_FOUND);
    ok_eq_uint(Position, 0);

    Position = 123;
    Status = RtlFindCharInUnicodeString(2, &String, &Chars3, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 2);

    Position = 123;
    Status = RtlFindCharInUnicodeString(3, &String, &Chars4, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 24);

    /* Show that 4 means case insensitive */
    Position = 123;
    Status = RtlFindCharInUnicodeString(4, &String, &Chars2, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 6);

    Position = 123;
    Status = RtlFindCharInUnicodeString(5, &String, &Chars2, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 10);

    Position = 123;
    Status = RtlFindCharInUnicodeString(6, &String, &Chars3, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 4);

    Position = 123;
    Status = RtlFindCharInUnicodeString(7, &String, &Chars4, &Position);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Position, 22);

    /* Show that Position is USHORT */
    LongPosition = 0x55555555;
    Status = RtlFindCharInUnicodeString(8, &String, &String, (PUSHORT)&LongPosition);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_ulong(LongPosition, 0x55550000UL);

    /* Invalid flags */
    Position = 123;
    Status = RtlFindCharInUnicodeString(8, &String, &String, &Position);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_uint(Position, 0);

    Position = 123;
    Status = RtlFindCharInUnicodeString(0xFFFFFFF8, &String, &String, &Position);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_uint(Position, 0);

    Position = 123;
    Status = RtlFindCharInUnicodeString(0xFFFFFFFF, &String, &String, &Position);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_uint(Position, 0);

    /* NULL for SearchString */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, NULL, &String, &Position);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_uint(Position, 0);

    /* NULL for SearchString and invalid flags */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(8, NULL, &String, &Position);
    KmtEndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_uint(Position, 0);
    ok_eq_uint(Position, 0);

    /* NULL for SearchString with zero-length MatchString */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, NULL, &ZeroLengthString, &Position);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_uint(Position, 0);

    /* NULL for MatchString */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, &String, NULL, &Position);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_uint(Position, 0);

    /* This crashes in Windows, but not in ROS. I see no reason to add
     * additional code or redesign the function to replicate that */
#if 0
    /* NULL for MatchString with zero-length SearchString */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, &ZeroLengthString, NULL, &Position);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_uint(Position, 0);
#endif

    /* NULL for MatchString and invalid flags */
    Position = 123;
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(8, &String, NULL, &Position);
    KmtEndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_uint(Position, 0);

    /* NULL for Position */
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, &String, &String, NULL);
    KmtEndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* NULL for all three */
    KmtStartSeh()
        Status = RtlFindCharInUnicodeString(0, NULL, NULL, NULL);
    KmtEndSeh(STATUS_SUCCESS);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
#endif
}

static
VOID
TestUpcaseUnicodeString(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING Lower;
    UNICODE_STRING Upper;
    PWCHAR Buffer;

    Buffer = KmtAllocateGuarded(sizeof(WCHAR));

    if (!KmtIsCheckedBuild)
    {
    RtlInitEmptyUnicodeString(&Lower, NULL, 0);
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, 0);
    ok_eq_uint(Upper.MaximumLength, 0);
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    RtlFreeUnicodeString(&Upper);

    RtlInitEmptyUnicodeString(&Lower, Buffer, 0);
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, 0);
    ok_eq_uint(Upper.MaximumLength, 0);
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    RtlFreeUnicodeString(&Upper);

    RtlInitEmptyUnicodeString(&Lower, Buffer, sizeof(WCHAR));
    Buffer[0] = UNICODE_NULL;
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, 0);
    ok_eq_uint(Upper.MaximumLength, 0);
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    RtlFreeUnicodeString(&Upper);
    }

    RtlInitEmptyUnicodeString(&Lower, Buffer, sizeof(WCHAR));
    Lower.Length = sizeof(WCHAR);
    Buffer[0] = UNICODE_NULL;
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, sizeof(WCHAR));
    ok_eq_uint(Upper.MaximumLength, sizeof(WCHAR));
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    ok_eq_hex(Upper.Buffer[0], UNICODE_NULL);
    RtlFreeUnicodeString(&Upper);

    RtlInitEmptyUnicodeString(&Lower, Buffer, sizeof(WCHAR));
    Lower.Length = sizeof(WCHAR);
    Buffer[0] = 'a';
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, sizeof(WCHAR));
    ok_eq_uint(Upper.MaximumLength, sizeof(WCHAR));
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    ok_eq_hex(Upper.Buffer[0], 'A');
    RtlFreeUnicodeString(&Upper);

    RtlInitUnicodeString(&Lower, L"a");
    RtlFillMemory(&Upper, sizeof(Upper), 0x55);
    Status = RtlUpcaseUnicodeString(&Upper, &Lower, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Upper.Length, sizeof(WCHAR));
    ok_eq_uint(Upper.MaximumLength, sizeof(WCHAR));
    ok(Upper.Buffer != NULL, "Buffer = %p\n", Upper.Buffer);
    ok_eq_hex(Upper.Buffer[0], 'A');
    RtlFreeUnicodeString(&Upper);

    KmtFreeGuarded(Buffer);
}

START_TEST(RtlUnicodeString)
{
    TestFindCharInUnicodeString();
    TestUpcaseUnicodeString();
}
