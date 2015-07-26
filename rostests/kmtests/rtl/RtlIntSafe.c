/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for ntintsafe.h functions
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>
#include <ntintsafe.h>

START_TEST(RtlIntSafe)
{
    NTSTATUS Status;
    INT8 Int8Result;
    UINT8 UInt8Result;
    INT IntResult;
    UINT UIntResult;
    USHORT UShortResult;
    SHORT ShortResult;

#define TEST_CONVERSION(FromName, FromType, ToName, ToType, Print, Value, Expected, ExpectedStatus) \
    do                                                                                              \
    {                                                                                               \
        ToName ## Result = (ToType)0xfedcba9876543210;                                              \
        Status = Rtl ## FromName ## To ## ToName(Value,                                             \
                                                 &ToName ## Result);                                \
        ok_eq_hex(Status, ExpectedStatus);                                                          \
        ok_eq_ ## Print(ToName ## Result, Expected);                                                \
    } while (0)

    TEST_CONVERSION(UInt8, UINT8, Int8,   INT8,   int,  0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(UInt8, UINT8, Int8,   INT8,   int,  5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(UInt8, UINT8, Int8,   INT8,   int,  INT8_MAX,           INT8_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(UInt8, UINT8, Int8,   INT8,   int,  INT8_MAX + 1,       (INT8)-1,       STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(UInt8, UINT8, Int8,   INT8,   int,  (UINT8)-1,          (INT8)-1,       STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(ULong, ULONG, UShort, USHORT, uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UShort, USHORT, uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UShort, USHORT, uint, USHORT_MAX,         USHORT_MAX,     STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UShort, USHORT, uint, USHORT_MAX + 1,     (USHORT)-1,     STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(ULong, ULONG, UShort, USHORT, uint, (ULONG)-1,          (USHORT)-1,     STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(ULong, ULONG, Int,    INT,    int,  0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, Int,    INT,    int,  5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, Int,    INT,    int,  INT_MAX,            INT_MAX,        STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, Int,    INT,    int,  (ULONG)INT_MAX + 1, (INT)-1,        STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(ULong, ULONG, Int,    INT,    int,  (ULONG)-1,          (INT)-1,        STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(ULong, ULONG, UInt,   UINT,   uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UInt,   UINT,   uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UInt,   UINT,   uint, UINT_MAX,           UINT_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(ULong, ULONG, UInt,   UINT,   uint, (ULONG)-1,          (UINT)-1,       STATUS_SUCCESS);

    TEST_CONVERSION(Int8,  INT8,  UInt8,  UINT8,  uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UInt8,  UINT8,  uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UInt8,  UINT8,  uint, INT8_MAX,           INT8_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UInt8,  UINT8,  uint, -1,                 (UINT8)-1,      STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int8,  INT8,  UInt8,  UINT8,  uint, INT8_MIN,           (UINT8)-1,      STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(Int8,  INT8,  UShort, USHORT, uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UShort, USHORT, uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UShort, USHORT, uint, INT8_MAX,           INT8_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(Int8,  INT8,  UShort, USHORT, uint, -1,                 (USHORT)-1,     STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int8,  INT8,  UShort, USHORT, uint, INT8_MIN,           (USHORT)-1,     STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, USHORT_MAX,         USHORT_MAX,     STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, USHORT_MAX + 1,     (USHORT)-1,     STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, LONG_MAX,           (USHORT)-1,     STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, -1,                 (USHORT)-1,     STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Long,  LONG,  UShort, USHORT, uint, LONG_MIN,           (USHORT)-1,     STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(Long,  LONG,  UInt,   UINT,   uint, 0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UInt,   UINT,   uint, 5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UInt,   UINT,   uint, LONG_MAX,           LONG_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(Long,  LONG,  UInt,   UINT,   uint, -1,                 (UINT)-1,       STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Long,  LONG,  UInt,   UINT,   uint, LONG_MIN,           (UINT)-1,       STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT8_MAX,           INT8_MAX,       STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT8_MAX + 1,       (INT8)-1,       STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT_MAX,            (INT8)-1,       STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT8_MIN,           INT8_MIN,       STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT8_MIN - 1,       (INT8)-1,       STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Int8,   INT8,   int,  INT_MIN,            (INT8)-1,       STATUS_INTEGER_OVERFLOW);

    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  0,                  0,              STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  5,                  5,              STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  SHORT_MAX,          SHORT_MAX,      STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  SHORT_MAX + 1,      (SHORT)-1,      STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  INT_MAX,            (SHORT)-1,      STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  SHORT_MIN,          SHORT_MIN,      STATUS_SUCCESS);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  SHORT_MIN - 1,      (SHORT)-1,      STATUS_INTEGER_OVERFLOW);
    TEST_CONVERSION(Int,   INT,   Short,  SHORT,  int,  INT_MIN,            (SHORT)-1,      STATUS_INTEGER_OVERFLOW);

#define TEST_ADD(_Name, _Type, _Print, _Value1, _Value2, _Expected, _Status)  do \
    {                                                                       \
        _Name ## Result = (_Type)0xfedcba9876543210;                        \
        Status = Rtl ## _Name ## Add(_Value1, _Value2, & _Name ## Result);  \
        ok_eq_hex(Status, _Status);                                         \
        ok_eq_ ## _Print(_Name ## Result, _Expected);                       \
    } while (0)

    TEST_ADD(UInt8,     UINT8,      uint,       0,                  0,              0,              STATUS_SUCCESS);
    TEST_ADD(UInt8,     UINT8,      uint,       5,                  5,              10,             STATUS_SUCCESS);
    TEST_ADD(UInt8,     UINT8,      uint,       0,                  UINT8_MAX,      UINT8_MAX,      STATUS_SUCCESS);
    TEST_ADD(UInt8,     UINT8,      uint,       UINT8_MAX,          0,              UINT8_MAX,      STATUS_SUCCESS);
    TEST_ADD(UInt8,     UINT8,      uint,       UINT8_MAX - 1,      1,              UINT8_MAX,      STATUS_SUCCESS);
    TEST_ADD(UInt8,     UINT8,      uint,       UINT8_MAX,          1,              (UINT8)-1,      STATUS_INTEGER_OVERFLOW);
    TEST_ADD(UInt8,     UINT8,      uint,       UINT8_MAX,          UINT8_MAX,      (UINT8)-1,      STATUS_INTEGER_OVERFLOW);
}
