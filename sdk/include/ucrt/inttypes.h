//
// inttypes.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Various integer format specifier macros for use with the stdio library, and
// various integer conversion and manipulation functions.
//
#pragma once
#ifndef _INTTYPES // include guard for 3rd party interop
#define _INTTYPES

#include <corecrt.h>
#include <stdint.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef struct
{
    intmax_t quot;
    intmax_t rem;
} _Lldiv_t;

typedef _Lldiv_t imaxdiv_t;



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_Check_return_
_ACRTIMP intmax_t __cdecl imaxabs(
    _In_ intmax_t _Number
    );

_Check_return_
_ACRTIMP imaxdiv_t __cdecl imaxdiv(
    _In_ intmax_t _Numerator,
    _In_ intmax_t _Denominator
    );

_ACRTIMP intmax_t __cdecl strtoimax(
    _In_z_                   char const* _String,
    _Out_opt_ _Deref_post_z_ char**      _EndPtr,
    _In_                     int         _Radix
    );

_ACRTIMP intmax_t __cdecl _strtoimax_l(
    _In_z_                   char const* _String,
    _Out_opt_ _Deref_post_z_ char**      _EndPtr,
    _In_                     int         _Radix,
    _In_opt_                 _locale_t   _Locale
    );

_ACRTIMP uintmax_t __cdecl strtoumax(
    _In_z_                   char const* _String,
    _Out_opt_ _Deref_post_z_ char**      _EndPtr,
    _In_                     int         _Radix
    );

_ACRTIMP uintmax_t __cdecl _strtoumax_l(
    _In_z_                   char const* _String,
    _Out_opt_ _Deref_post_z_ char**      _EndPtr,
    _In_                     int         _Radix,
    _In_opt_                 _locale_t   _Locale
    );

_ACRTIMP intmax_t __cdecl wcstoimax(
    _In_z_                   wchar_t const* _String,
    _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
    _In_                     int            _Radix
    );

_ACRTIMP intmax_t __cdecl _wcstoimax_l(
    _In_z_                   wchar_t const* _String,
    _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
    _In_                     int            _Radix,
    _In_opt_                 _locale_t      _Locale
    );

_ACRTIMP uintmax_t __cdecl wcstoumax(
    _In_z_                   wchar_t const* _String,
    _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
    _In_                     int            _Radix
    );

_ACRTIMP uintmax_t __cdecl _wcstoumax_l(
    _In_z_                   wchar_t const* _String,
    _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
    _In_                     int            _Radix,
    _In_opt_                 _locale_t      _Locale
    );



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Output Format Specifier Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define PRId8        "hhd"
#define PRId16       "hd"
#define PRId32       "d"
#define PRId64       "lld"
#define PRIdLEAST8   PRId8
#define PRIdLEAST16  PRId16
#define PRIdLEAST32  PRId32
#define PRIdLEAST64  PRId64
#define PRIdFAST8    PRId8
#define PRIdFAST16   PRId32
#define PRIdFAST32   PRId32
#define PRIdFAST64   PRId64
#define PRIdMAX      PRId64
#ifdef _WIN64
    #define PRIdPTR  PRId64
#else
    #define PRIdPTR  PRId32
#endif

#define PRIi8        "hhi"
#define PRIi16       "hi"
#define PRIi32       "i"
#define PRIi64       "lli"
#define PRIiLEAST8   PRIi8
#define PRIiLEAST16  PRIi16
#define PRIiLEAST32  PRIi32
#define PRIiLEAST64  PRIi64
#define PRIiFAST8    PRIi8
#define PRIiFAST16   PRIi32
#define PRIiFAST32   PRIi32
#define PRIiFAST64   PRIi64
#define PRIiMAX      PRIi64
#ifdef _WIN64
    #define PRIiPTR  PRIi64
#else
    #define PRIiPTR  PRIi32
#endif

#define PRIo8        "hho"
#define PRIo16       "ho"
#define PRIo32       "o"
#define PRIo64       "llo"
#define PRIoLEAST8   PRIo8
#define PRIoLEAST16  PRIo16
#define PRIoLEAST32  PRIo32
#define PRIoLEAST64  PRIo64
#define PRIoFAST8    PRIo8
#define PRIoFAST16   PRIo32
#define PRIoFAST32   PRIo32
#define PRIoFAST64   PRIo64
#define PRIoMAX      PRIo64
#ifdef _WIN64
    #define PRIoPTR  PRIo64
#else
    #define PRIoPTR  PRIo32
#endif

#define PRIu8        "hhu"
#define PRIu16       "hu"
#define PRIu32       "u"
#define PRIu64       "llu"
#define PRIuLEAST8   PRIu8
#define PRIuLEAST16  PRIu16
#define PRIuLEAST32  PRIu32
#define PRIuLEAST64  PRIu64
#define PRIuFAST8    PRIu8
#define PRIuFAST16   PRIu32
#define PRIuFAST32   PRIu32
#define PRIuFAST64   PRIu64
#define PRIuMAX      PRIu64
#ifdef _WIN64
    #define PRIuPTR  PRIu64
#else
    #define PRIuPTR  PRIu32
#endif

#define PRIx8        "hhx"
#define PRIx16       "hx"
#define PRIx32       "x"
#define PRIx64       "llx"
#define PRIxLEAST8   PRIx8
#define PRIxLEAST16  PRIx16
#define PRIxLEAST32  PRIx32
#define PRIxLEAST64  PRIx64
#define PRIxFAST8    PRIx8
#define PRIxFAST16   PRIx32
#define PRIxFAST32   PRIx32
#define PRIxFAST64   PRIx64
#define PRIxMAX      PRIx64
#ifdef _WIN64
    #define PRIxPTR  PRIx64
#else
    #define PRIxPTR  PRIx32
#endif

#define PRIX8        "hhX"
#define PRIX16       "hX"
#define PRIX32       "X"
#define PRIX64       "llX"
#define PRIXLEAST8   PRIX8
#define PRIXLEAST16  PRIX16
#define PRIXLEAST32  PRIX32
#define PRIXLEAST64  PRIX64
#define PRIXFAST8    PRIX8
#define PRIXFAST16   PRIX32
#define PRIXFAST32   PRIX32
#define PRIXFAST64   PRIX64
#define PRIXMAX      PRIX64
#ifdef _WIN64
    #define PRIXPTR  PRIX64
#else
    #define PRIXPTR  PRIX32
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Input Format Specifier Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define SCNd8        "hhd"
#define SCNd16       "hd"
#define SCNd32       "d"
#define SCNd64       "lld"
#define SCNdLEAST8   SCNd8
#define SCNdLEAST16  SCNd16
#define SCNdLEAST32  SCNd32
#define SCNdLEAST64  SCNd64
#define SCNdFAST8    SCNd8
#define SCNdFAST16   SCNd32
#define SCNdFAST32   SCNd32
#define SCNdFAST64   SCNd64
#define SCNdMAX      SCNd64
#ifdef _WIN64
    #define SCNdPTR  SCNd64
#else
    #define SCNdPTR  SCNd32
#endif

#define SCNi8        "hhi"
#define SCNi16       "hi"
#define SCNi32       "i"
#define SCNi64       "lli"
#define SCNiLEAST8   SCNi8
#define SCNiLEAST16  SCNi16
#define SCNiLEAST32  SCNi32
#define SCNiLEAST64  SCNi64
#define SCNiFAST8    SCNi8
#define SCNiFAST16   SCNi32
#define SCNiFAST32   SCNi32
#define SCNiFAST64   SCNi64
#define SCNiMAX      SCNi64
#ifdef _WIN64
    #define SCNiPTR  SCNi64
#else
    #define SCNiPTR  SCNi32
#endif

#define SCNo8        "hho"
#define SCNo16       "ho"
#define SCNo32       "o"
#define SCNo64       "llo"
#define SCNoLEAST8   SCNo8
#define SCNoLEAST16  SCNo16
#define SCNoLEAST32  SCNo32
#define SCNoLEAST64  SCNo64
#define SCNoFAST8    SCNo8
#define SCNoFAST16   SCNo32
#define SCNoFAST32   SCNo32
#define SCNoFAST64   SCNo64
#define SCNoMAX      SCNo64
#ifdef _WIN64
    #define SCNoPTR  SCNo64
#else
    #define SCNoPTR  SCNo32
#endif

#define SCNu8        "hhu"
#define SCNu16       "hu"
#define SCNu32       "u"
#define SCNu64       "llu"
#define SCNuLEAST8   SCNu8
#define SCNuLEAST16  SCNu16
#define SCNuLEAST32  SCNu32
#define SCNuLEAST64  SCNu64
#define SCNuFAST8    SCNu8
#define SCNuFAST16   SCNu32
#define SCNuFAST32   SCNu32
#define SCNuFAST64   SCNu64
#define SCNuMAX      SCNu64
#ifdef _WIN64
    #define SCNuPTR  SCNu64
#else
    #define SCNuPTR  SCNu32
#endif

#define SCNx8        "hhx"
#define SCNx16       "hx"
#define SCNx32       "x"
#define SCNx64       "llx"
#define SCNxLEAST8   SCNx8
#define SCNxLEAST16  SCNx16
#define SCNxLEAST32  SCNx32
#define SCNxLEAST64  SCNx64
#define SCNxFAST8    SCNx8
#define SCNxFAST16   SCNx32
#define SCNxFAST32   SCNx32
#define SCNxFAST64   SCNx64
#define SCNxMAX      SCNx64
#ifdef _WIN64
    #define SCNxPTR  SCNx64
#else
    #define SCNxPTR  SCNx32
#endif



_CRT_END_C_HEADER

/*
 * Copyright (c) 1992-2010 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.30:0009 */

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INTTYPES
