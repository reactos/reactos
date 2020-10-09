/*!
 * \file intsafe.h
 *
 * \brief Windows helper functions for integer overflow prevention
 *
 * \package This file is part of the ReactOS PSDK package.
 *
 * \author
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * \copyright THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * \todo
 * - missing conversion functions
 * - multiplication functions
 * - signed add, sub and multiply functions
 */
#pragma once

#ifndef _INTSAFE_H_INCLUDED_
#define _INTSAFE_H_INCLUDED_

#include <specstrings.h>

/* Handle ntintsafe here too */
#ifdef _NTINTSAFE_H_INCLUDED_
#ifndef _NTDEF_ /* Guard agains redefinition from ntstatus.h */
typedef _Return_type_success_(return >= 0) long NTSTATUS;
#endif
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_INTEGER_OVERFLOW ((NTSTATUS)0xC0000095)
#define INTSAFE_RESULT NTSTATUS
#define INTSAFE_SUCCESS STATUS_SUCCESS
#define INTSAFE_E_ARITHMETIC_OVERFLOW STATUS_INTEGER_OVERFLOW
#define INTSAFE_NAME(name) Rtl##name
#else // _NTINTSAFE_H_INCLUDED_
#ifndef _HRESULT_DEFINED
typedef _Return_type_success_(return >= 0) long HRESULT;
#endif
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define S_OK    ((HRESULT)0L)
#define INTSAFE_RESULT HRESULT
#define INTSAFE_SUCCESS S_OK
#define INTSAFE_E_ARITHMETIC_OVERFLOW ((HRESULT)0x80070216L)
#define INTSAFE_NAME(name) name
#endif // _NTINTSAFE_H_INCLUDED_

#if !defined(_W64)
#if defined(_MSC_VER) && !defined(__midl) && (defined(_M_IX86) || defined(_M_ARM))
#define _W64 __w64
#else
#define _W64
#endif
#endif

/* Static assert */
#ifndef C_ASSERT
#ifdef _MSC_VER
# define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else
# define C_ASSERT(e) extern void __C_ASSERT__(int [(e)?1:-1])
#endif
#endif /* C_ASSERT */

/* Typedefs */
#ifndef _WINNT_
#ifndef _NTDEF_
typedef char CHAR;
typedef unsigned char UCHAR, UINT8;
typedef signed char INT8;
typedef short SHORT;
typedef signed short INT16;
typedef unsigned short USHORT, UINT16;
typedef int INT;
typedef unsigned int UINT32;
typedef signed int INT32;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long LONGLONG, LONG64;
typedef signed long long INT64;
typedef unsigned long long ULONGLONG, DWORDLONG, ULONG64, DWORD64, UINT64;
#ifdef _WIN64
typedef long long INT_PTR, LONG_PTR, SSIZE_T, ptrdiff_t;
typedef unsigned long long UINT_PTR, ULONG_PTR, DWORD_PTR, SIZE_T, size_t;
#else // _WIN64
typedef _W64 int INT_PTR, ptrdiff_t;
typedef _W64 unsigned int UINT_PTR, size_t;
typedef _W64 long LONG_PTR, SSIZE_T;
typedef _W64 unsigned long ULONG_PTR, DWORD_PTR, SIZE_T;
#endif // _WIN64
#endif
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned long DWORD;
#endif // _WINNT_

/* Just to be sure! */
C_ASSERT(sizeof(USHORT) == 2);
C_ASSERT(sizeof(INT) == 4);
C_ASSERT(sizeof(UINT) == 4);
C_ASSERT(sizeof(LONG) == 4);
C_ASSERT(sizeof(ULONG) == 4);
C_ASSERT(sizeof(DWORD) == 4);
C_ASSERT(sizeof(UINT_PTR) == sizeof(ULONG_PTR));

/* Integer range margins (use (x-1) to prevent warnings) */
#define INT8_MIN ((signed char)(-127 - 1))
#define SHORT_MIN (-32768)
#define INT16_MIN ((short)(-32767 - 1))
#define INT_MIN (-2147483647 - 1)
#define INT32_MIN (-2147483647 - 1)
#define LONG_MIN (-2147483647L - 1)
#define LONGLONG_MIN (-9223372036854775807LL - 1)
#define LONG64_MIN (-9223372036854775807LL - 1)
#define INT64_MIN (-9223372036854775807LL - 1)
#define INT128_MIN (-170141183460469231731687303715884105727i128 - 1)
#ifdef _WIN64
#define INT_PTR_MIN (-9223372036854775807LL - 1)
#define LONG_PTR_MIN (-9223372036854775807LL - 1)
#define PTRDIFF_T_MIN (-9223372036854775807LL - 1)
#define SSIZE_T_MIN (-9223372036854775807LL - 1)
#else /* _WIN64 */
#define INT_PTR_MIN (-2147483647 - 1)
#define LONG_PTR_MIN (-2147483647L - 1)
#define PTRDIFF_T_MIN (-2147483647 - 1)
#define SSIZE_T_MIN (-2147483647L - 1)
#endif /* _WIN64 */

#define INT8_MAX ((signed char)127)
#define UINT8_MAX ((unsigned char)0xffU)
#define BYTE_MAX ((unsigned char)0xff)
#define SHORT_MAX ((short)32767)
#define INT16_MAX ((short)32767)
#define USHORT_MAX ((unsigned short)0xffff)
#define UINT16_MAX ((unsigned short)0xffff)
#define WORD_MAX ((unsigned short)0xffff)
#define INT_MAX 2147483647
#define INT32_MAX 2147483647
#define UINT_MAX 0xffffffff
#define UINT32_MAX 0xffffffffU
#define LONG_MAX 2147483647L
#define ULONG_MAX 0xffffffffUL
#define DWORD_MAX 0xffffffffUL
#define LONGLONG_MAX 9223372036854775807LL
#define LONG64_MAX 9223372036854775807LL
#define INT64_MAX 9223372036854775807LL
#define ULONGLONG_MAX 0xffffffffffffffffULL
#define DWORDLONG_MAX 0xffffffffffffffffULL
#define ULONG64_MAX 0xffffffffffffffffULL
#define DWORD64_MAX 0xffffffffffffffffULL
#define UINT64_MAX 0xffffffffffffffffULL
#define INT128_MAX 170141183460469231731687303715884105727i128
#define UINT128_MAX 0xffffffffffffffffffffffffffffffffui128
#undef SIZE_T_MAX
#ifdef _WIN64
#define INT_PTR_MAX 9223372036854775807LL
#define UINT_PTR_MAX 0xffffffffffffffffULL
#define LONG_PTR_MAX 9223372036854775807LL
#define ULONG_PTR_MAX 0xffffffffffffffffULL
#define DWORD_PTR_MAX 0xffffffffffffffffULL
#define PTRDIFF_T_MAX 9223372036854775807LL
#define SIZE_T_MAX 0xffffffffffffffffULL
#define SSIZE_T_MAX 9223372036854775807LL
#define _SIZE_T_MAX 0xffffffffffffffffULL
#else /* _WIN64 */
#define INT_PTR_MAX 2147483647
#define UINT_PTR_MAX 0xffffffff
#define LONG_PTR_MAX 2147483647L
#define ULONG_PTR_MAX 0xffffffffUL
#define DWORD_PTR_MAX 0xffffffffUL
#define PTRDIFF_T_MAX 2147483647
#define SIZE_T_MAX 0xffffffff
#define SSIZE_T_MAX 2147483647L
#define _SIZE_T_MAX 0xffffffffUL
#endif /* _WIN64 */

/* Error values */
#define INT8_ERROR ((signed char)(-1))
#define UINT8_ERROR ((unsigned char)0xff)
#define BYTE_ERROR ((unsigned char)0xff)
#define SHORT_ERROR ((short)(-1))
#define INT16_ERROR ((short)(-1))
#define USHORT_ERROR ((unsigned short)0xffff)
#define UINT16_ERROR ((unsigned short)0xffff)
#define WORD_ERROR ((unsigned short)0xffff)
#define INT_ERROR (-1)
#define INT32_ERROR (-1)
#define UINT_ERROR 0xffffffffU
#define UINT32_ERROR 0xffffffffU
#define LONG_ERROR (-1L)
#define ULONG_ERROR 0xffffffffUL
#define DWORD_ERROR 0xffffffffUL
#define LONGLONG_ERROR (-1LL)
#define LONG64_ERROR (-1LL)
#define INT64_ERROR (-1LL)
#define ULONGLONG_ERROR 0xffffffffffffffffULL
#define DWORDLONG_ERROR 0xffffffffffffffffULL
#define ULONG64_ERROR 0xffffffffffffffffULL
#define UINT64_ERROR 0xffffffffffffffffULL
#ifdef _WIN64
#define INT_PTR_ERROR (-1LL)
#define UINT_PTR_ERROR 0xffffffffffffffffULL
#define LONG_PTR_ERROR (-1LL)
#define ULONG_PTR_ERROR 0xffffffffffffffffULL
#define DWORD_PTR_ERROR 0xffffffffffffffffULL
#define PTRDIFF_T_ERROR (-1LL)
#define SIZE_T_ERROR 0xffffffffffffffffULL
#define SSIZE_T_ERROR (-1LL)
#define _SIZE_T_ERROR 0xffffffffffffffffULL
#else /* _WIN64 */
#define INT_PTR_ERROR (-1)
#define UINT_PTR_ERROR 0xffffffffU
#define LONG_PTR_ERROR (-1L)
#define ULONG_PTR_ERROR 0xffffffffUL
#define DWORD_PTR_ERROR 0xffffffffUL
#define PTRDIFF_T_ERROR (-1)
#define SIZE_T_ERROR 0xffffffffU
#define SSIZE_T_ERROR (-1L)
#define _SIZE_T_ERROR 0xffffffffUL
#endif /* _WIN64 */

/* special definitons (the CHAR ones should not be defined here!) */
#define _INTSAFE_CHAR CHAR
#define _INTSAFE_CHAR_ERROR ((signed char)(-1))
#ifdef _CHAR_UNSIGNED
 #define _INTSAFE_CHAR_MIN ((unsigned char)0)
 #define _INTSAFE_CHAR_MAX ((unsigned char)0xff)
#else
 #define _INTSAFE_CHAR_MIN ((signed char)(-128))
 #define _INTSAFE_CHAR_MAX ((signed char)127)
#endif /* _CHAR_UNSIGNED */

#define size_t_ERROR SIZE_T_ERROR
#define UCHAR_ERROR '\0'
#define CHAR_ERROR '\0'

/* 32 bit x 32 bit to 64 bit unsigned multiplication */
#ifndef UInt32x32To64
#define UInt32x32To64(a,b) ((unsigned __int64)(unsigned int)(a)*(unsigned __int64)(unsigned int)(b))
#endif

/* Convert unsigned to signed or unsigned */
#define DEFINE_SAFE_CONVERT_UTOX(_Name, _TypeFrom, _TypeTo) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _TypeFrom Input, \
    _Out_ _Deref_out_range_(==, Input) _TypeTo *pOutput) \
{ \
    if (Input <= _TypeTo ## _MAX) \
    { \
        *pOutput = (_TypeTo)Input; \
        return INTSAFE_SUCCESS; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_UTOX(ByteToChar, BYTE, _INTSAFE_CHAR)
DEFINE_SAFE_CONVERT_UTOX(ByteToInt8, BYTE, INT8)
DEFINE_SAFE_CONVERT_UTOX(UInt8ToChar, UINT8, _INTSAFE_CHAR)
DEFINE_SAFE_CONVERT_UTOX(UInt8ToInt8, UINT8, INT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToChar, USHORT, _INTSAFE_CHAR)
DEFINE_SAFE_CONVERT_UTOX(UShortToUChar, USHORT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToInt8, USHORT, INT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToUInt8, USHORT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToShort, USHORT, SHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToUChar, UINT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UIntToInt8, UINT, INT8)
DEFINE_SAFE_CONVERT_UTOX(UIntToUInt8, UINT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UIntToShort, UINT, SHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToUShort, UINT, USHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToInt, UINT, INT)
DEFINE_SAFE_CONVERT_UTOX(UIntToLong, UINT, LONG)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToUChar, UINT_PTR, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToInt8, UINT_PTR, INT8)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToUInt8, UINT_PTR, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToShort, UINT_PTR, SHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToUShort, UINT_PTR, USHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToInt16, UINT_PTR, INT16)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToUInt16, UINT_PTR, UINT16)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToInt, UINT_PTR, INT)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToLong, UINT_PTR, LONG)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToIntPtr, UINT_PTR, INT_PTR)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToLongPtr, UINT_PTR, LONG_PTR)
DEFINE_SAFE_CONVERT_UTOX(ULongToUChar, ULONG, UINT8)
DEFINE_SAFE_CONVERT_UTOX(ULongToUInt8, ULONG, UINT8)
DEFINE_SAFE_CONVERT_UTOX(ULongToShort, ULONG, SHORT)
DEFINE_SAFE_CONVERT_UTOX(ULongToUShort, ULONG, USHORT)
DEFINE_SAFE_CONVERT_UTOX(ULongToInt, ULONG, INT)
DEFINE_SAFE_CONVERT_UTOX(ULongToUInt, ULONG, UINT)
DEFINE_SAFE_CONVERT_UTOX(ULongToIntPtr, ULONG, INT_PTR)
DEFINE_SAFE_CONVERT_UTOX(ULongToUIntPtr, ULONG, UINT_PTR)
DEFINE_SAFE_CONVERT_UTOX(ULongToLongPtr, ULONG, LONG_PTR)
DEFINE_SAFE_CONVERT_UTOX(ULongPtrToULong, ULONG_PTR, ULONG)
DEFINE_SAFE_CONVERT_UTOX(ULongLongToUInt, ULONGLONG, UINT)
DEFINE_SAFE_CONVERT_UTOX(ULongLongToULong, ULONGLONG, ULONG)
DEFINE_SAFE_CONVERT_UTOX(ULongLongToULongPtr, ULONGLONG, ULONG_PTR)


/* Convert signed to unsigned */
#define DEFINE_SAFE_CONVERT_STOU(_Name, _TypeFrom, _TypeTo) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _TypeFrom Input, \
    _Out_ _Deref_out_range_(==, Input) _TypeTo *pOutput) \
{ \
    if ((Input >= 0) && \
        ((sizeof(_TypeFrom) <= sizeof(_TypeTo)) || (Input <= (_TypeFrom)_TypeTo ## _MAX))) \
    { \
        *pOutput = (_TypeTo)Input; \
        return INTSAFE_SUCCESS; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_STOU(Int8ToUChar, INT8, UINT8)
DEFINE_SAFE_CONVERT_STOU(Int8ToUInt8, INT8, UINT8)
DEFINE_SAFE_CONVERT_STOU(Int8ToUShort, INT8, USHORT)
DEFINE_SAFE_CONVERT_STOU(Int8ToUInt, INT8, UINT)
DEFINE_SAFE_CONVERT_STOU(Int8ToULong, INT8, ULONG)
DEFINE_SAFE_CONVERT_STOU(Int8ToUIntPtr, INT8, UINT_PTR)
DEFINE_SAFE_CONVERT_STOU(Int8ToULongPtr, INT8, ULONG_PTR)
DEFINE_SAFE_CONVERT_STOU(Int8ToULongLong, INT8, ULONGLONG)
DEFINE_SAFE_CONVERT_STOU(ShortToUChar, SHORT, UINT8)
DEFINE_SAFE_CONVERT_STOU(ShortToUInt8, SHORT, UINT8)
DEFINE_SAFE_CONVERT_STOU(ShortToUShort, SHORT, USHORT)
DEFINE_SAFE_CONVERT_STOU(ShortToUInt, SHORT, UINT)
DEFINE_SAFE_CONVERT_STOU(ShortToULong, SHORT, ULONG)
DEFINE_SAFE_CONVERT_STOU(ShortToUIntPtr, SHORT, UINT_PTR)
DEFINE_SAFE_CONVERT_STOU(ShortToULongPtr, SHORT, ULONG_PTR)
DEFINE_SAFE_CONVERT_STOU(ShortToDWordPtr, SHORT, DWORD_PTR)
DEFINE_SAFE_CONVERT_STOU(ShortToULongLong, SHORT, ULONGLONG)
DEFINE_SAFE_CONVERT_STOU(IntToUChar, INT, UINT8)
DEFINE_SAFE_CONVERT_STOU(IntToUInt8, INT, UINT8)
DEFINE_SAFE_CONVERT_STOU(IntToUShort, INT, USHORT)
DEFINE_SAFE_CONVERT_STOU(IntToUInt, INT, UINT)
DEFINE_SAFE_CONVERT_STOU(IntToULong, INT, ULONG)
DEFINE_SAFE_CONVERT_STOU(IntToULongLong, INT, ULONGLONG)
DEFINE_SAFE_CONVERT_STOU(LongToUChar, LONG, UINT8)
DEFINE_SAFE_CONVERT_STOU(LongToUInt8, LONG, UINT8)
DEFINE_SAFE_CONVERT_STOU(LongToUShort, LONG, USHORT)
DEFINE_SAFE_CONVERT_STOU(LongToUInt, LONG, UINT)
DEFINE_SAFE_CONVERT_STOU(LongToULong, LONG, ULONG)
DEFINE_SAFE_CONVERT_STOU(LongToUIntPtr, LONG, UINT_PTR)
DEFINE_SAFE_CONVERT_STOU(LongToULongPtr, LONG, ULONG_PTR)
DEFINE_SAFE_CONVERT_STOU(LongToULongLong, LONG, ULONGLONG)
DEFINE_SAFE_CONVERT_STOU(IntPtrToUChar, INT_PTR, UINT8)
DEFINE_SAFE_CONVERT_STOU(IntPtrToUInt8, INT_PTR, UINT8)
DEFINE_SAFE_CONVERT_STOU(IntPtrToUShort, INT_PTR, USHORT)
DEFINE_SAFE_CONVERT_STOU(IntPtrToUInt, INT_PTR, UINT)
DEFINE_SAFE_CONVERT_STOU(IntPtrToULong, INT_PTR, ULONG)
DEFINE_SAFE_CONVERT_STOU(IntPtrToUIntPtr, INT_PTR, UINT_PTR)
DEFINE_SAFE_CONVERT_STOU(IntPtrToULongPtr, INT_PTR, ULONG_PTR)
DEFINE_SAFE_CONVERT_STOU(IntPtrToULongLong, INT_PTR, ULONGLONG)
DEFINE_SAFE_CONVERT_STOU(LongPtrToUChar, LONG_PTR, UINT8)
DEFINE_SAFE_CONVERT_STOU(LongPtrToUInt8, LONG_PTR, UINT8)
DEFINE_SAFE_CONVERT_STOU(LongPtrToUShort, LONG_PTR, USHORT)
DEFINE_SAFE_CONVERT_STOU(LongPtrToUInt, LONG_PTR, UINT)
DEFINE_SAFE_CONVERT_STOU(LongPtrToULong, LONG_PTR, ULONG)
DEFINE_SAFE_CONVERT_STOU(LongPtrToUIntPtr, LONG_PTR, UINT_PTR)
DEFINE_SAFE_CONVERT_STOU(LongPtrToULongPtr, LONG_PTR, ULONG_PTR)
DEFINE_SAFE_CONVERT_STOU(LongPtrToULongLong, LONG_PTR, ULONGLONG)
#ifdef _CHAR_UNSIGNED
DEFINE_SAFE_CONVERT_STOU(ShortToChar, SHORT, UINT8)
DEFINE_SAFE_CONVERT_STOU(LongPtrToChar, LONG_PTR, UINT8)
#endif


/* Convert signed to signed */
#define DEFINE_SAFE_CONVERT_STOS(_Name, _TypeFrom, _TypeTo) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _TypeFrom Input, \
    _Out_ _Deref_out_range_(==, Input) _TypeTo *pOutput) \
{ \
    if ((Input >= _TypeTo ## _MIN) && (Input <= _TypeTo ## _MAX)) \
    { \
        *pOutput = (_TypeTo)Input; \
        return INTSAFE_SUCCESS; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_STOS(ShortToInt8, SHORT, INT8)
DEFINE_SAFE_CONVERT_STOS(IntToInt8, INT, INT8)
DEFINE_SAFE_CONVERT_STOS(IntToShort, INT, SHORT)
DEFINE_SAFE_CONVERT_STOS(LongToInt8, LONG, INT8)
DEFINE_SAFE_CONVERT_STOS(LongToShort, LONG, SHORT)
DEFINE_SAFE_CONVERT_STOS(LongToInt, LONG, INT)
DEFINE_SAFE_CONVERT_STOS(IntPtrToInt8, INT_PTR, INT8)
DEFINE_SAFE_CONVERT_STOS(IntPtrToShort, INT_PTR, SHORT)
DEFINE_SAFE_CONVERT_STOS(IntPtrToInt, INT_PTR, INT)
DEFINE_SAFE_CONVERT_STOS(IntPtrToLong, INT_PTR, LONG)
DEFINE_SAFE_CONVERT_STOS(IntPtrToLongPtr, INT_PTR, LONG_PTR)
DEFINE_SAFE_CONVERT_STOS(LongPtrToInt8, LONG_PTR, INT8)
DEFINE_SAFE_CONVERT_STOS(LongPtrToShort, LONG_PTR, SHORT)
DEFINE_SAFE_CONVERT_STOS(LongPtrToInt, LONG_PTR, INT)
DEFINE_SAFE_CONVERT_STOS(LongPtrToLong, LONG_PTR, LONG)
DEFINE_SAFE_CONVERT_STOS(LongPtrToIntPtr, LONG_PTR, INT_PTR)
DEFINE_SAFE_CONVERT_STOS(LongLongToInt, LONGLONG, INT)
DEFINE_SAFE_CONVERT_STOS(LongLongToLong, LONGLONG, LONG)
DEFINE_SAFE_CONVERT_STOS(LongLongToIntPtr, LONGLONG, INT_PTR)
DEFINE_SAFE_CONVERT_STOS(LongLongToLongPtr, LONGLONG, LONG_PTR)
DEFINE_SAFE_CONVERT_STOS(ShortToChar, SHORT, _INTSAFE_CHAR)
DEFINE_SAFE_CONVERT_STOS(LongPtrToChar, LONG_PTR, _INTSAFE_CHAR)


#ifdef _NTINTSAFE_H_INCLUDED_

#define RtlInt8ToByte RtlInt8ToUInt8
#define RtlInt8ToUInt16 RtlInt8ToUShort
#define RtlInt8ToWord RtlInt8ToUShort
#define RtlInt8ToUInt32 RtlInt8ToUInt
#define RtlInt8ToDWord RtlInt8ToULong
#define RtlInt8ToDWordPtr RtlInt8ToULongPtr
#define RtlInt8ToDWordLong RtlInt8ToULongLong
#define RtlInt8ToULong64 RtlInt8ToULongLong
#define RtlInt8ToDWord64 RtlInt8ToULongLong
#define RtlInt8ToUInt64 RtlInt8ToULongLong
#define RtlInt8ToSizeT RtlInt8ToUIntPtr
#define RtlInt8ToSIZET RtlInt8ToULongPtr
#define RtlIntToSizeT RtlIntToUIntPtr
#define RtlIntToSIZET RtlIntToULongPtr
#define RtlULongToSSIZET RtlULongToLongPtr
#define RtlULongToByte RtlULongToUInt8
#define RtlULongLongToInt64 RtlULongLongToLongLong
#define RtlULongLongToLong64 RtlULongLongToLongLong
#define RtlULongLongToPtrdiffT RtlULongLongToIntPtr
#define RtlULongLongToSizeT RtlULongLongToUIntPtr
#define RtlULongLongToSSIZET RtlULongLongToLongPtr
#define RtlULongLongToSIZET RtlULongLongToULongPtr
#define RtlSIZETToULong RtlULongPtrToULong
#define RtlSSIZETToULongLong RtlLongPtrToULongLong
#define RtlSSIZETToULong RtlLongPtrToULong
#ifdef _WIN64
#define RtlIntToUIntPtr RtlIntToULongLong
#define RtlULongLongToIntPtr RtlULongLongToLongLong
#else
#define RtlIntToUIntPtr RtlIntToUInt
#define RtlULongLongToIntPtr RtlULongLongToInt
#define RtlULongLongToUIntPtr RtlULongLongToUInt
#define RtlULongLongToULongPtr RtlULongLongToULong
#endif

#else // _NTINTSAFE_H_INCLUDED_

#define Int8ToByte Int8ToUInt8
#define Int8ToUInt16 Int8ToUShort
#define Int8ToWord Int8ToUShort
#define Int8ToUInt32 Int8ToUInt
#define Int8ToDWord Int8ToULong
#define Int8ToDWordPtr Int8ToULongPtr
#define Int8ToDWordLong Int8ToULongLong
#define Int8ToULong64 Int8ToULongLong
#define Int8ToDWord64 Int8ToULongLong
#define Int8ToUInt64 Int8ToULongLong
#define Int8ToSizeT Int8ToUIntPtr
#define Int8ToSIZET Int8ToULongPtr
#define IntToSizeT IntToUIntPtr
#define IntToSIZET IntToULongPtr
#define ULongToSSIZET ULongToLongPtr
#define ULongToByte ULongToUInt8
#define ULongLongToInt64 ULongLongToLongLong
#define ULongLongToLong64 ULongLongToLongLong
#define ULongLongToPtrdiffT ULongLongToIntPtr
#define ULongLongToSizeT ULongLongToUIntPtr
#define ULongLongToSSIZET ULongLongToLongPtr
#define ULongLongToSIZET ULongLongToULongPtr
#define SIZETToULong ULongPtrToULong
#define SSIZETToULongLong LongPtrToULongLong
#define SSIZETToULong LongPtrToULong
#ifdef _WIN64
#define IntToUIntPtr IntToULongLong
#define ULongLongToIntPtr ULongLongToLongLong
#else
#define IntToUIntPtr IntToUInt
#define ULongLongToIntPtr ULongLongToInt
#define ULongLongToUIntPtr ULongLongToUInt
#define ULongLongToULongPtr ULongLongToULong
#endif

#endif // _NTINTSAFE_H_INCLUDED_


#define DEFINE_SAFE_ADD(_Name, _Type) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type Augend, \
    _In_ _Type Addend, \
    _Out_ _Deref_out_range_(==, Augend + Addend) _Type *pOutput) \
{ \
    if ((_Type)(Augend + Addend) >= Augend) \
    { \
        *pOutput = Augend + Addend; \
        return INTSAFE_SUCCESS; \
    } \
    else \
    { \
        *pOutput = _Type ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_ADD(UInt8Add, UINT8)
DEFINE_SAFE_ADD(UShortAdd, USHORT)
DEFINE_SAFE_ADD(UIntAdd, UINT)
DEFINE_SAFE_ADD(ULongAdd, ULONG)
DEFINE_SAFE_ADD(UIntPtrAdd, UINT_PTR)
DEFINE_SAFE_ADD(ULongPtrAdd, ULONG_PTR)
DEFINE_SAFE_ADD(DWordPtrAdd, DWORD_PTR)
DEFINE_SAFE_ADD(SizeTAdd, size_t)
DEFINE_SAFE_ADD(SIZETAdd, SIZE_T)
DEFINE_SAFE_ADD(ULongLongAdd, ULONGLONG)


#define DEFINE_SAFE_SUB(_Name, _Type) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type Minuend, \
    _In_ _Type Subtrahend, \
    _Out_ _Deref_out_range_(==, Minuend - Subtrahend) _Type* pOutput) \
{ \
    if (Minuend >= Subtrahend) \
    { \
        *pOutput = Minuend - Subtrahend; \
        return INTSAFE_SUCCESS; \
    } \
    else \
    { \
        *pOutput = _Type ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_SUB(UInt8Sub, UINT8)
DEFINE_SAFE_SUB(UShortSub, USHORT)
DEFINE_SAFE_SUB(UIntSub, UINT)
DEFINE_SAFE_SUB(UIntPtrSub, UINT_PTR)
DEFINE_SAFE_SUB(ULongSub, ULONG)
DEFINE_SAFE_SUB(ULongPtrSub, ULONG_PTR)
DEFINE_SAFE_SUB(DWordPtrSub, DWORD_PTR)
DEFINE_SAFE_SUB(SizeTSub, size_t)
DEFINE_SAFE_SUB(SIZETSub, SIZE_T)
DEFINE_SAFE_SUB(ULongLongSub, ULONGLONG)

#ifdef ENABLE_INTSAFE_SIGNED_FUNCTIONS
_Must_inspect_result_
__forceinline
INTSAFE_RESULT
INTSAFE_NAME(LongLongAdd)(
    _In_ LONGLONG Augend,
    _In_ LONGLONG Addend,
    _Out_ _Deref_out_range_(==, Augend + Addend) LONGLONG* pResult)
{
    LONGLONG Result = Augend + Addend;

    /* The only way the result can overflow, is when the sign of the augend
       and the addend are the same. In that case the result is expected to
       have the same sign as the two, otherwise it overflowed.
       Sign equality is checked with a binary xor operation. */
    if ( ((Augend ^ Addend) >= 0) && ((Augend ^ Result) < 0) )
    {
        *pResult = LONGLONG_ERROR;
        return INTSAFE_E_ARITHMETIC_OVERFLOW;
    }
    else
    {
        *pResult = Result;
        return INTSAFE_SUCCESS;
    }
}


#define DEFINE_SAFE_ADD_S(_Name, _Type1, _Type2, _Convert) \
C_ASSERT(sizeof(_Type2) > sizeof(_Type1)); \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type1 Augend, \
    _In_ _Type1 Addend, \
    _Out_ _Deref_out_range_(==, Augend + Addend) _Type1* pOutput) \
{ \
    return INTSAFE_NAME(_Convert)(((_Type2)Augend) + ((_Type2)Addend), pOutput); \
}

DEFINE_SAFE_ADD_S(Int8Add, INT8, SHORT, ShortToInt8)
DEFINE_SAFE_ADD_S(ShortAdd, SHORT, INT, IntToShort)
DEFINE_SAFE_ADD_S(IntAdd, INT, LONGLONG, LongLongToInt)
DEFINE_SAFE_ADD_S(LongAdd, LONG, LONGLONG, LongLongToLong)
#ifndef _WIN64
DEFINE_SAFE_ADD_S(IntPtrAdd, INT_PTR, LONGLONG, LongLongToIntPtr)
DEFINE_SAFE_ADD_S(LongPtrAdd, LONG_PTR, LONGLONG, LongLongToLongPtr)
#endif

_Must_inspect_result_
__forceinline
INTSAFE_RESULT
INTSAFE_NAME(LongLongSub)(
    _In_ LONGLONG Minuend,
    _In_ LONGLONG Subtrahend,
    _Out_ _Deref_out_range_(==, Minuend - Subtrahend) LONGLONG* pResult)
{
    LONGLONG Result = Minuend - Subtrahend;

    /* The only way the result can overflow, is when the sign of the minuend
       and the subtrahend differ. In that case the result is expected to
       have the same sign as the minuend, otherwise it overflowed.
       Sign equality is checked with a binary xor operation. */
    if ( ((Minuend ^ Subtrahend) < 0) && ((Minuend ^ Result) < 0) )
    {
        *pResult = LONGLONG_ERROR;
        return INTSAFE_E_ARITHMETIC_OVERFLOW;
    }
    else
    {
        *pResult = Result;
        return INTSAFE_SUCCESS;
    }
}


#define DEFINE_SAFE_SUB_S(_Name, _Type1, _Type2, _Convert) \
C_ASSERT(sizeof(_Type2) > sizeof(_Type1)); \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type1 Minuend, \
    _In_ _Type1 Subtrahend, \
    _Out_ _Deref_out_range_(==, Minuend - Subtrahend) _Type1* pOutput) \
{ \
    return INTSAFE_NAME(_Convert)(((_Type2)Minuend) - ((_Type2)Subtrahend), pOutput); \
}

DEFINE_SAFE_SUB_S(LongSub, LONG, LONGLONG, LongLongToLong)
#ifndef _WIN64
DEFINE_SAFE_SUB_S(IntPtrSub, INT_PTR, LONGLONG, LongLongToIntPtr)
DEFINE_SAFE_SUB_S(LongPtrSub, LONG_PTR, LONGLONG, LongLongToLongPtr)
#endif

#endif /* ENABLE_INTSAFE_SIGNED_FUNCTIONS */

_Must_inspect_result_
__forceinline
INTSAFE_RESULT
INTSAFE_NAME(ULongLongMult)(
    _In_ ULONGLONG Multiplicand,
    _In_ ULONGLONG Multiplier,
    _Out_ _Deref_out_range_(==, Multiplicand * Multiplier) ULONGLONG* pOutput)
{
    /* We can split the 64 bit numbers in low and high parts:
        M1 = M1Low + M1Hi * 0x100000000
        M2 = M2Low + M2Hi * 0x100000000

       Then the multiplication looks like this:
        M1 * M2 = (M1Low + M1Hi * 0x100000000) * (M2Low + M2Hi * 0x100000000)
                = M1Low * M2Low
                  + M1Low * M2Hi * 0x100000000
                  + M2Low * M1Hi * 0x100000000
                  + M1Hi * M2Hi * 0x100000000 * 0x100000000

        We get an overflow when
            a) M1Hi * M2Hi != 0, so when M1Hi and M2Hi are both not 0
            b) The product of the nonzero high part and the other low part
               is larger than 32 bits.
            c) The addition of the product from b) shifted left by 32 and
               M1Low * M2Low is larger than 64 bits
    */
    ULONG M1Low = Multiplicand & 0xffffffff;
    ULONG M2Low = Multiplier & 0xffffffff;
    ULONG M1Hi = Multiplicand >> 32;
    ULONG M2Hi = Multiplier >> 32;
    ULONGLONG Temp;

    if (M1Hi == 0)
    {
        Temp = UInt32x32To64(M1Low, M2Hi);
    }
    else if (M2Hi == 0)
    {
        Temp = UInt32x32To64(M1Hi, M2Low);
    }
    else
    {
        *pOutput = ULONGLONG_ERROR;
        return INTSAFE_E_ARITHMETIC_OVERFLOW;
    }

    if (Temp > ULONG_MAX)
    {
        *pOutput = ULONGLONG_ERROR;
        return INTSAFE_E_ARITHMETIC_OVERFLOW;
    }

    return INTSAFE_NAME(ULongLongAdd)(Temp << 32, UInt32x32To64(M1Low, M2Low), pOutput);
}


#define DEFINE_SAFE_MULT_U32(_Name, _Type, _Convert) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type Multiplicand, \
    _In_ _Type Multiplier, \
    _Out_ _Deref_out_range_(==, Multiplicand * Multiplier) _Type* pOutput) \
{ \
    ULONGLONG Result = UInt32x32To64(Multiplicand, Multiplier); \
    return INTSAFE_NAME(_Convert)(Result, pOutput); \
}

DEFINE_SAFE_MULT_U32(ULongMult, ULONG, ULongLongToULong)
#ifndef _WIN64
DEFINE_SAFE_MULT_U32(SizeTMult, size_t, ULongLongToSizeT)
DEFINE_SAFE_MULT_U32(SIZETMult, SIZE_T, ULongLongToSIZET)
#endif

#define DEFINE_SAFE_MULT_U16(_Name, _Type, _Convert) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type Multiplicand, \
    _In_ _Type Multiplier, \
    _Out_ _Deref_out_range_(==, Multiplicand * Multiplier) _Type* pOutput) \
{ \
    ULONG Result = ((ULONG)Multiplicand) * ((ULONG)Multiplier); \
    return INTSAFE_NAME(_Convert)(Result, pOutput); \
}

DEFINE_SAFE_MULT_U16(UShortMult, USHORT, ULongToUShort)


#ifdef _NTINTSAFE_H_INCLUDED_

#define RtlUInt16Add RtlUShortAdd
#define RtlWordAdd RtlUShortAdd
#define RtlUInt32Add RtlUIntAdd
#define RtlDWordAdd RtlULongAdd
#define RtlDWordLongAdd RtlULongLongAdd
#define RtlULong64Add RtlULongLongAdd
#define RtlDWord64Add RtlULongLongAdd
#define RtlUInt64Add RtlULongLongAdd
#define RtlUInt16Sub RtlUShortSub
#define RtlWordSub RtlUShortSub
#define RtlUInt32Sub RtlUIntSub
#define RtlDWordSub RtlULongSub
#define RtlDWordLongSub RtlULongLongSub
#define RtlULong64Sub RtlULongLongSub
#define RtlDWord64Sub RtlULongLongSub
#define RtlUInt64Sub RtlULongLongSub
#define RtlUInt16Mult RtlUShortMult
#define RtlWordMult RtlUShortMult
#ifdef _WIN64
#define RtlIntPtrAdd RtlLongLongAdd
#define RtlLongPtrAdd RtlLongLongAdd
#define RtlIntPtrSub RtlLongLongSub
#define RtlLongPtrSub RtlLongLongSub
#define RtlSizeTMult RtlULongLongMult
#define RtlSIZETMult RtlULongLongMult
#else
#endif

#else // _NTINTSAFE_H_INCLUDED_

#define UInt16Add UShortAdd
#define WordAdd UShortAdd
#define UInt32Add UIntAdd
#define DWordAdd ULongAdd
#define DWordLongAdd ULongLongAdd
#define ULong64Add ULongLongAdd
#define DWord64Add ULongLongAdd
#define UInt64Add ULongLongAdd
#define UInt16Sub UShortSub
#define WordSub UShortSub
#define UInt32Sub UIntSub
#define DWordSub ULongSub
#define DWordLongSub ULongLongSub
#define ULong64Sub ULongLongSub
#define DWord64Sub ULongLongSub
#define UInt64Sub ULongLongSub
#define UInt16Mult UShortMult
#define WordMult UShortMult
#ifdef _WIN64
#define IntPtrAdd LongLongAdd
#define LongPtrAdd LongLongAdd
#define IntPtrSub LongLongSub
#define LongPtrSub LongLongSub
#define SizeTMult ULongLongMult
#define SIZETMult ULongLongMult
#else
#endif

#undef _INTSAFE_CHAR_MIN
#undef _INTSAFE_CHAR_MAX
#undef _INTSAFE_CHAR_ERROR

#endif // _NTINTSAFE_H_INCLUDED_

#endif // !_INTSAFE_H_INCLUDED_
