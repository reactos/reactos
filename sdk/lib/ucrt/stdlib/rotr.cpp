//
// rotr.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _lrotr(), _rotr(), and _rotr64(), which perform a rotate-right on an
// integer.
//
#include <stdlib.h>
#include <limits.h>



#ifdef __clang__
/* Clang reserves these names as Microsoft builtins, so use private C++
 * identifiers while emitting the documented CRT ABI symbols. */
#define _UCRT_STRINGIZE2(x) #x
#define _UCRT_STRINGIZE(x)  _UCRT_STRINGIZE2(x)
#define _UCRT_SYMBOL_NAME(name) _UCRT_STRINGIZE(__USER_LABEL_PREFIX__) #name
#define _UCRT_DEFINE_ROTATE(name, impl, type) \
    extern "C" type __cdecl impl(type value, int shift) __asm__(_UCRT_SYMBOL_NAME(name)); \
    extern "C" type __cdecl impl(type value, int shift)
#else
#pragma function(_lrotr, _rotr, _rotr64)
#define _UCRT_DEFINE_ROTATE(name, impl, type) \
    extern "C" type __cdecl name(type value, int shift)
#endif

#if UINT_MAX != 0xffffffff
    #error This source file assumes 32-bit integers
#endif

#if UINT_MAX != ULONG_MAX
    #error This source file assumes sizeof(int) == sizeof(long)
#endif



_UCRT_DEFINE_ROTATE(_lrotr, ucrt_lrotr, unsigned long)
{
    shift &= 0x1f;
    value = (value << (0x20 - shift)) | (value >> shift);
    return value;
}

_UCRT_DEFINE_ROTATE(_rotr, ucrt_rotr, unsigned)
{
    shift &= 0x1f;
    value = (value << (0x20 - shift)) | (value >> shift);
    return value;
}

_UCRT_DEFINE_ROTATE(_rotr64, ucrt_rotr64, unsigned __int64)
{
    shift &= 0x3f;
    value = (value << (0x40 - shift)) | (value >> shift);
    return value;
}

#undef _UCRT_DEFINE_ROTATE
#ifdef __clang__
#undef _UCRT_SYMBOL_NAME
#undef _UCRT_STRINGIZE
#undef _UCRT_STRINGIZE2
#endif
