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

#if defined(__GNUC__) && !defined(__forceinline)
#define __forceinline extern __inline __attribute((always_inline))
#endif

/* Handle ntintsafe here too */
#ifdef _NTINTSAFE_H_INCLUDED_
#ifndef NT_SUCCESS /* Guard agains redefinition from ntstatus.h */
typedef _Return_type_success_(return >= 0) long NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif
#define INTSAFE_RESULT NTSTATUS
#define INTSAFE_SUCCESS STATUS_SUCCESS
#define INTSAFE_E_ARITHMETIC_OVERFLOW STATUS_INTEGER_OVERFLOW
#define INTSAFE_NAME(name) Rtl##name
#else // _NTINTSAFE_H_INCLUDED_
#ifndef SUCCEEDED /* Guard agains redefinition from winerror.h */
typedef _Return_type_success_(return >= 0) long HRESULT;
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define S_OK    ((HRESULT)0L)
#endif
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
typedef char CHAR;
typedef signed char INT8;
typedef unsigned char UCHAR, UINT8, BYTE;
typedef short SHORT;
typedef signed short INT16;
typedef unsigned short USHORT, UINT16, WORD;
typedef int INT;
typedef signed int INT32;
typedef unsigned int UINT, UINT32;
typedef long LONG;
typedef unsigned long ULONG, DWORD;
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
#endif // _WINNT_

/* Just to be sure! */
C_ASSERT(sizeof(USHORT) == 2);
C_ASSERT(sizeof(INT) == 4);
C_ASSERT(sizeof(UINT) == 4);
C_ASSERT(sizeof(LONG) == 4);
C_ASSERT(sizeof(ULONG) == 4);
C_ASSERT(sizeof(UINT_PTR) == sizeof(ULONG_PTR));

/* Integer range margins (use (x-1) to prevent warnings) */
#define INT8_MIN (-128)
#define SHORT_MIN (-32768)
#define INT16_MIN (-32768)
#define INT_MIN (-2147483647 - 1)
#define INT32_MIN (-2147483647 - 1)
#define LONG_MIN (-2147483647L - 1)
#define LONGLONG_MIN (-9223372036854775808LL)
#define LONG64_MIN (-9223372036854775808LL)
#define INT64_MIN (-9223372036854775808LL)
//#define INT128_MIN (-170141183460469231731687303715884105728)
#ifdef _WIN64
#define INT_PTR_MIN INT64_MIN
#define LONG_PTR_MIN LONG64_MIN
#define PTRDIFF_T_MIN INT64_MIN
#define SSIZE_T_MIN INT64_MIN
#else // _WIN64
#define INT_PTR_MIN INT_MIN
#define LONG_PTR_MIN LONG_MIN
#define PTRDIFF_T_MIN INT_MIN
#define SSIZE_T_MIN INT_MIN
#endif // _WIN64

#define INT8_MAX 127
#define UINT8_MAX 0xff
#define UCHAR_MAX 0xff
#define BYTE_MAX 0xff
#define SHORT_MAX 32767
#define INT16_MAX 32767
#define USHORT_MAX 0xffff
#define UINT16_MAX 0xffff
#define WORD_MAX 0xffff
#define INT_MAX 2147483647
#define INT32_MAX 2147483647
#define UINT_MAX 0xffffffff
#define UINT32_MAX 0xffffffff
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
#define INT128_MAX 170141183460469231731687303715884105727
#define UINT128_MAX 0xffffffffffffffffffffffffffffffff
#undef SIZE_T_MAX
#ifdef _WIN64
#define INT_PTR_MAX INT64_MAX
#define UINT_PTR_MAX UINT64_MAX
#define LONG_PTR_MAX LONG64_MAX
#define ULONG_PTR_MAX ULONG64_MAX
#define DWORD_PTR_MAX DWORD64_MAX
#define PTRDIFF_T_MAX INT64_MAX
#define SIZE_T_MAX UINT64_MAX
#define SSIZE_T_MAX INT64_MAX
#define _SIZE_T_MAX UINT64_MAX
#else // _WIN64
#define INT_PTR_MAX INT_MAX
#define UINT_PTR_MAX UINT_MAX
#define LONG_PTR_MAX LONG_MAX
#define ULONG_PTR_MAX ULONG_MAX
#define DWORD_PTR_MAX DWORD_MAX
#define PTRDIFF_T_MAX INT_MAX
#define SIZE_T_MAX UINT_MAX
#define SSIZE_T_MAX INT_MAX
#define _SIZE_T_MAX UINT_MAX
#endif // _WIN64

#ifndef CHAR_MIN
#ifdef _CHAR_UNSIGNED
#define CHAR_MIN 0
#define CHAR_MAX 0xff
#else
#define CHAR_MIN (-128)
#define CHAR_MAX 127
#endif
#endif

/* Error values */
#define INT8_ERROR (-1)
#define UINT8_ERROR 0xff
#define BYTE_ERROR 0xff
#define SHORT_ERROR (-1)
#define INT16_ERROR (-1)
#define USHORT_ERROR 0xffff
#define UINT16_ERROR 0xffff
#define WORD_ERROR 0xffff
#define INT_ERROR (-1)
#define INT32_ERROR (-1)
#define UINT_ERROR 0xffffffff
#define UINT32_ERROR 0xffffffff
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
#else // _WIN64
#define INT_PTR_ERROR (-1)
#define UINT_PTR_ERROR 0xffffffff
#define LONG_PTR_ERROR (-1L)
#define ULONG_PTR_ERROR 0xffffffffUL
#define DWORD_PTR_ERROR 0xffffffffUL
#define PTRDIFF_T_ERROR (-1)
#define SIZE_T_ERROR 0xffffffff
#define SSIZE_T_ERROR (-1L)
#define _SIZE_T_ERROR 0xffffffffUL
#endif // _WIN64

#define size_t_ERROR SIZE_T_ERROR
#define UCHAR_ERROR '\0'
#define CHAR_ERROR '\0'


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
        return S_OK; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_UTOX(ByteToChar, BYTE, CHAR)
DEFINE_SAFE_CONVERT_UTOX(ByteToInt8, BYTE, INT8)
DEFINE_SAFE_CONVERT_UTOX(UInt8ToChar, UINT8, CHAR)
DEFINE_SAFE_CONVERT_UTOX(UInt8ToInt8, UINT8, INT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToChar, USHORT, CHAR)
DEFINE_SAFE_CONVERT_UTOX(UShortToUChar, USHORT, UCHAR)
DEFINE_SAFE_CONVERT_UTOX(UShortToInt8, USHORT, INT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToUInt8, USHORT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UShortToShort, USHORT, SHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToUChar, UINT, UCHAR)
DEFINE_SAFE_CONVERT_UTOX(UIntToInt8, UINT, INT8)
DEFINE_SAFE_CONVERT_UTOX(UIntToUInt8, UINT, UINT8)
DEFINE_SAFE_CONVERT_UTOX(UIntToShort, UINT, SHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToUShort, UINT, USHORT)
DEFINE_SAFE_CONVERT_UTOX(UIntToInt, UINT, INT)
DEFINE_SAFE_CONVERT_UTOX(UIntToLong, UINT, LONG)
DEFINE_SAFE_CONVERT_UTOX(UIntPtrToUChar, UINT_PTR, UCHAR)
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


#define DEFINE_SAFE_CONVERT_ITOU(_Name, _TypeFrom, _TypeTo) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _TypeFrom Input, \
    _Out_ _Deref_out_range_(==, Input) _TypeTo *pOutput) \
{ \
    if ((Input >= 0) && (Input <= _TypeTo ## _MAX)) \
    { \
        *pOutput = (_TypeTo)Input; \
        return S_OK; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_ITOU(Int8ToUChar, INT8, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(Int8ToUInt8, INT8, UINT8)
DEFINE_SAFE_CONVERT_ITOU(Int8ToUShort, INT8, USHORT)
DEFINE_SAFE_CONVERT_ITOU(Int8ToUInt, INT8, UINT)
DEFINE_SAFE_CONVERT_ITOU(Int8ToULong, INT8, ULONG)
DEFINE_SAFE_CONVERT_ITOU(Int8ToUIntPtr, INT8, UINT_PTR)
DEFINE_SAFE_CONVERT_ITOU(Int8ToULongPtr, INT8, ULONG_PTR)
DEFINE_SAFE_CONVERT_ITOU(Int8ToULongLong, INT8, ULONGLONG)
DEFINE_SAFE_CONVERT_ITOU(ShortToUChar, SHORT, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(ShortToUInt8, SHORT, UINT8)
DEFINE_SAFE_CONVERT_ITOU(ShortToUShort, SHORT, USHORT)
DEFINE_SAFE_CONVERT_ITOU(ShortToUInt, SHORT, UINT)
DEFINE_SAFE_CONVERT_ITOU(ShortToULong, SHORT, ULONG)
DEFINE_SAFE_CONVERT_ITOU(ShortToUIntPtr, SHORT, UINT_PTR)
DEFINE_SAFE_CONVERT_ITOU(ShortToULongPtr, SHORT, ULONG_PTR)
DEFINE_SAFE_CONVERT_ITOU(ShortToDWordPtr, SHORT, DWORD_PTR)
DEFINE_SAFE_CONVERT_ITOU(ShortToULongLong, SHORT, ULONGLONG)
DEFINE_SAFE_CONVERT_ITOU(IntToUChar, INT, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(IntToUInt8, INT, UINT8)
DEFINE_SAFE_CONVERT_ITOU(IntToUShort, INT, USHORT)
DEFINE_SAFE_CONVERT_ITOU(IntToUInt, INT, UINT)
DEFINE_SAFE_CONVERT_ITOU(IntToULong, INT, ULONG)
DEFINE_SAFE_CONVERT_ITOU(IntToULongLong, INT, ULONGLONG)
DEFINE_SAFE_CONVERT_ITOU(LongToUChar, LONG, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(LongToUInt8, LONG, UINT8)
DEFINE_SAFE_CONVERT_ITOU(LongToUShort, LONG, USHORT)
DEFINE_SAFE_CONVERT_ITOU(LongToUInt, LONG, UINT)
DEFINE_SAFE_CONVERT_ITOU(LongToULong, LONG, ULONG)
DEFINE_SAFE_CONVERT_ITOU(LongToUIntPtr, LONG, UINT_PTR)
DEFINE_SAFE_CONVERT_ITOU(LongToULongPtr, LONG, ULONG_PTR)
DEFINE_SAFE_CONVERT_ITOU(LongToULongLong, LONG, ULONGLONG)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToUChar, INT_PTR, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToUInt8, INT_PTR, UINT8)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToUShort, INT_PTR, USHORT)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToUInt, INT_PTR, UINT)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToULong, INT_PTR, ULONG)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToUIntPtr, INT_PTR, UINT_PTR)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToULongPtr, INT_PTR, ULONG_PTR)
DEFINE_SAFE_CONVERT_ITOU(IntPtrToULongLong, INT_PTR, ULONGLONG)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToUChar, LONG_PTR, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToUInt8, LONG_PTR, UINT8)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToUShort, LONG_PTR, USHORT)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToUInt, LONG_PTR, UINT)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToULong, LONG_PTR, ULONG)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToUIntPtr, LONG_PTR, UINT_PTR)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToULongPtr, LONG_PTR, ULONG_PTR)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToULongLong, LONG_PTR, ULONGLONG)
#ifdef _CHAR_UNSIGNED
DEFINE_SAFE_CONVERT_ITOU(ShortToChar, SHORT, UCHAR)
DEFINE_SAFE_CONVERT_ITOU(LongPtrToChar, LONG_PTR, UCHAR)
#endif


#define DEFINE_SAFE_CONVERT_ITOI(_Name, _TypeFrom, _TypeTo) \
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
        return S_OK; \
    } \
    else \
    { \
        *pOutput = _TypeTo ## _ERROR; \
        return INTSAFE_E_ARITHMETIC_OVERFLOW; \
    } \
}

DEFINE_SAFE_CONVERT_ITOI(ShortToInt8, SHORT, INT8)
DEFINE_SAFE_CONVERT_ITOI(IntToInt8, INT, INT8)
DEFINE_SAFE_CONVERT_ITOI(IntToShort, INT, SHORT)
DEFINE_SAFE_CONVERT_ITOI(LongToInt8, LONG, INT8)
DEFINE_SAFE_CONVERT_ITOI(LongToShort, LONG, SHORT)
DEFINE_SAFE_CONVERT_ITOI(LongToInt, LONG, INT)
DEFINE_SAFE_CONVERT_ITOI(IntPtrToInt8, INT_PTR, INT8)
DEFINE_SAFE_CONVERT_ITOI(IntPtrToShort, INT_PTR, SHORT)
DEFINE_SAFE_CONVERT_ITOI(IntPtrToInt, INT_PTR, INT)
DEFINE_SAFE_CONVERT_ITOI(IntPtrToLong, INT_PTR, LONG)
DEFINE_SAFE_CONVERT_ITOI(IntPtrToLongPtr, INT_PTR, LONG_PTR)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToInt8, LONG_PTR, INT8)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToShort, LONG_PTR, SHORT)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToInt, LONG_PTR, INT)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToLong, LONG_PTR, LONG)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToIntPtr, LONG_PTR, INT_PTR)
#ifndef _CHAR_UNSIGNED
DEFINE_SAFE_CONVERT_ITOI(ShortToChar, SHORT, CHAR)
DEFINE_SAFE_CONVERT_ITOI(LongPtrToChar, LONG_PTR, CHAR)
#endif

#define DEFINE_SAFE_ADD(_Name, _Type) \
_Must_inspect_result_ \
__forceinline \
INTSAFE_RESULT \
INTSAFE_NAME(_Name)( \
    _In_ _Type Augend, \
    _In_ _Type Addend, \
    _Out_ _Deref_out_range_(==, Augend + Addend) _Type *pOutput) \
{ \
    if ((Augend + Addend) >= Augend) \
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
DEFINE_SAFE_ADD(UlongAdd, ULONG)
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


#endif // !_INTSAFE_H_INCLUDED_
