//
// mbstring.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions and macros for MBCS string manipulation
//
#pragma once
#ifndef _INC_MBSTRING // include guard for 3rd party interop
#define _INC_MBSTRING

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#ifndef _FILE_DEFINED
    #define _FILE_DEFINED
    typedef struct _iobuf
    {
        void* _Placeholder;
    } FILE;
#endif

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

    #pragma push_macro("_mbsdup")
    #undef _mbsdup

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsdup(
        _In_z_ unsigned char const* _Str
        );

    #pragma pop_macro("_mbsdup")



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // The Multibyte Character String Library Functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbbtombc(
        _In_ unsigned int _C
        );

    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbbtombc_l(
        _In_     unsigned int _C,
        _In_opt_ _locale_t    _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbbtype(
        _In_ unsigned char _C,
        _In_ int           _CType
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbbtype_l(
        _In_     unsigned char _C,
        _In_     int           _CType,
        _In_opt_ _locale_t     _Locale
        );

    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbctombb(
        _In_ unsigned int _C
        );

    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbctombb_l(
        _In_     unsigned int _C,
        _In_opt_ _locale_t    _Locale
        );

    _DCRTIMP int __cdecl _mbsbtype(
        _In_reads_z_(_Pos) unsigned char const* _Str,
        _In_               size_t               _Pos
        );

    _DCRTIMP int __cdecl _mbsbtype_l(
        _In_reads_z_(_Pos) unsigned char const* _Str,
        _In_               size_t               _Pos,
        _In_opt_           _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbscat_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _SizeInBytes,
        _In_z_                          unsigned char const* _Src
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        errno_t, _mbscat_s,
               unsigned char,        _Dst,
        _In_z_ unsigned char const*, _DstSizeInBytes
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbscat,
        _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1), unsigned char,        _Dest,
        _In_z_                                                                    unsigned char const*, _Source
        )

    _DCRTIMP errno_t __cdecl _mbscat_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                               size_t               _DstSizeInBytes,
        _In_z_                             unsigned char const* _Src,
        _In_opt_                           _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbscat_s_l,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(
        unsigned char* , __RETURN_POLICY_DST, _DCRTIMP, _mbscat_l, _mbscat_s_l,
        _Inout_z_  unsigned char,
        _Inout_z_, unsigned char,        _Dest,
        _In_z_     unsigned char const*, _Source,
        _In_opt_   _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbschr(
        _In_z_ unsigned char const* _Str,
        _In_   unsigned int         _C
        );

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbschr_l(
        _In_z_   unsigned char const* _Str,
        _In_     unsigned int         _C,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbscmp(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbscmp_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbscoll(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbscoll_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_opt_ _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbscpy_s(
        _Out_writes_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                         size_t               _SizeInBytes,
        _In_z_                       unsigned char const* _Src
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        errno_t, _mbscpy_s,
               unsigned char,        _Dest,
        _In_z_ unsigned char const*, _Source
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbscpy,
        _Out_writes_z_(_String_length_(_Source) + 1), unsigned char,        _Dest,
        _In_z_                                        unsigned char const*, _Source
        )

    _DCRTIMP errno_t __cdecl _mbscpy_s_l(
        _Out_writes_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _DstSizeInBytes,
        _In_z_                          unsigned char const* _Src,
        _In_opt_                        _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbscpy_s,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbscpy_l, _mbscpy_s_l,
        _Pre_notnull_ _Post_z_  unsigned char,
        _Pre_notnull_ _Post_z_, unsigned char,        _Dest,
        _In_z_                  unsigned char const*, _Source,
        _In_opt_                _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP size_t __cdecl _mbscspn(
        _In_z_ unsigned char const* _String,
        _In_z_ unsigned char const* _Control
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbscspn_l(
        _In_z_   unsigned char const* _String,
        _In_z_   unsigned char const* _Control,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsdec(
        _In_reads_z_(_Pos - _Start + 1) unsigned char const* _Start,
        _In_z_                          unsigned char const* _Pos
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsdec_l(
        _In_reads_z_(_Pos-_Start+1) unsigned char const* _Start,
        _In_z_                      unsigned char const* _Pos,
        _In_opt_                    _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsicmp(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsicmp_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsicoll(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsicoll_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsinc(
        _In_z_ unsigned char const* _Ptr
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsinc_l(
        _In_z_   unsigned char const* _Ptr,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _Post_satisfies_(return <= _String_length_(_String))
    _DCRTIMP size_t __cdecl _mbslen(
        _In_z_ unsigned char const* _String
        );

    _Check_return_
    _Post_satisfies_(return <= _String_length_(_String))
    _DCRTIMP size_t __cdecl _mbslen_l(
        _In_z_   unsigned char const* _String,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _Post_satisfies_(return <= _String_length_(_String) && return <= _MaxCount)
    _DCRTIMP size_t __cdecl _mbsnlen(
        _In_z_ unsigned char const* _String,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _Post_satisfies_(return <= _String_length_(_String) && return <= _MaxCount)
    _DCRTIMP size_t __cdecl _mbsnlen_l(
        _In_z_   unsigned char const* _String,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbslwr_s(
        _Inout_updates_opt_z_(_SizeInBytes) unsigned char* _String,
        _In_                                size_t         _SizeInBytes
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        errno_t, _mbslwr_s,
        _Inout_updates_z_(_Size) unsigned char, _String
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbslwr,
        _Inout_z_, unsigned char, _String
        )

    _DCRTIMP errno_t __cdecl _mbslwr_s_l(
        _Inout_updates_opt_z_(_SizeInBytes) unsigned char* _String,
        _In_                                size_t         _SizeInBytes,
        _In_opt_                            _locale_t      _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        errno_t, _mbslwr_s_l,
                 unsigned char, _String,
        _In_opt_ _locale_t,     _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbslwr_l, _mbslwr_s_l,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char, _String,
        _In_opt_                 _locale_t,     _Locale
        )

    _DCRTIMP errno_t __cdecl _mbsnbcat_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _SizeInBytes,
        _In_z_                          unsigned char const* _Src,
        _In_                            size_t               _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsnbcat_s,
               unsigned char,        _Dest,
        _In_z_ unsigned char const*, _Source,
        _In_   size_t,               _Count
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbcat,
        _Inout_z_, unsigned char,        _Dest,
        _In_z_     unsigned char const*, _Source,
        _In_       size_t,               _Count
        )

    _DCRTIMP errno_t __cdecl _mbsnbcat_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                               size_t               _DstSizeInBytes,
        _In_z_                             unsigned char const* _Src,
        _In_                               size_t               _MaxCount,
        _In_opt_                           _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsnbcat_s_l,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_     size_t,               _Count,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbcat_l, _mbsnbcat_s_l,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char,        _Dest,
        _In_z_                   unsigned char const*, _Source,
        _In_                     size_t,               _Count,
        _In_opt_                 _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbcmp(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbcmp_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbcoll(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbcoll_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsnbcnt(
        _In_reads_or_z_(_MaxCount) unsigned char const* _String,
        _In_                       size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsnbcnt_l(
        _In_reads_or_z_(_MaxCount) unsigned char const* _String,
        _In_                       size_t               _MaxCount,
        _In_opt_                   _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsnbcpy_s(
        _Out_writes_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                         size_t               _SizeInBytes,
        _In_z_                       unsigned char const* _Src,
        _In_                         size_t               _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsnbcpy_s,
               unsigned char,        _Dest,
        _In_z_ unsigned char const*, _Source,
        _In_   size_t,               _Count
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2(
        _Success_(return != 0) unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbcpy,
        _Out_writes_(_Count) _Post_maybez_, unsigned char,        _Dest,
        _In_z_                              unsigned char const*, _Source,
        _In_                                size_t,               _Count
        )

    _DCRTIMP errno_t __cdecl _mbsnbcpy_s_l(
        _Out_writes_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _DstSizeInBytes,
        _In_z_                          unsigned char const* _Src,
        _In_                            size_t               _MaxCount,
        _In_opt_                        _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsnbcpy_s_l,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_     size_t,               _Count,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        _Success_(return != 0) unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbcpy_l, _mbsnbcpy_s_l,
        _Out_writes_z_(_Size)               unsigned char,
        _Out_writes_(_Count) _Post_maybez_, unsigned char,        _Dest,
        _In_z_                              unsigned char const*, _Source,
        _In_                                size_t,               _Count,
        _In_opt_                            _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbicmp(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbicmp_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbicoll(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnbicoll_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsnbset_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char* _Dst,
        _In_                            size_t         _SizeInBytes,
        _In_                            unsigned int   _C,
        _In_                            size_t         _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsnbset_s,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val,
        _In_        size_t,        _MaxCount
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbset, _mbsnbset_s,
        _Inout_updates_z_(_Size)      unsigned char,
        _Inout_updates_z_(_MaxCount), unsigned char, _String,
        _In_                          unsigned int,  _Val,
        _In_                          size_t,        _MaxCount
        )

    _DCRTIMP errno_t __cdecl _mbsnbset_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char* _Dst,
        _In_                               size_t         _DstSizeInBytes,
        _In_                               unsigned int   _Ch,
        _In_                               size_t         _MaxCount,
        _In_opt_                           _locale_t      _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsnbset_s_l,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val,
        _In_        size_t,        _MaxCount,
        _In_opt_    _locale_t,     _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnbset_l, _mbsnbset_s_l,
        _Inout_updates_z_(_Size)      unsigned char,
        _Inout_updates_z_(_MaxCount), unsigned char, _String,
        _In_                          unsigned int,  _Val,
        _In_                          size_t,        _MaxCount,
        _In_opt_                      _locale_t,     _Locale
        )

    _DCRTIMP errno_t __cdecl _mbsncat_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _SizeInBytes,
        _In_z_                          unsigned char const* _Src,
        _In_                            size_t               _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsncat_s,
               unsigned char,        _Dest,
        _In_z_ unsigned char const*, _Source,
        _In_   size_t,               _Count
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsncat,
        _Inout_z_, unsigned char,        _Dest,
        _In_z_     unsigned char const*, _Source,
        _In_       size_t,               _Count
        )

    _DCRTIMP errno_t __cdecl _mbsncat_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                               size_t               _DstSizeInBytes,
        _In_z_                             unsigned char const* _Src,
        _In_                               size_t               _MaxCount,
        _In_opt_                           _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsncat_s_l,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_     size_t,               _Count,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsncat_l, _mbsncat_s_l,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char,        _Dest,
        _In_z_                   unsigned char const*, _Source,
        _In_                     size_t,               _Count,
        _In_opt_                 _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsnccnt(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str,
        _In_                       size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsnccnt_l(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str,
        _In_                       size_t               _MaxCount,
        _In_opt_                   _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsncmp(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_z_                     unsigned char const* _Str2,
        _In_                       size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsncmp_l(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_z_                     unsigned char const* _Str2,
        _In_                       size_t               _MaxCount,
        _In_opt_                   _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsncoll(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsncoll_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsncpy_s(
        _Out_writes_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                         size_t               _SizeInBytes,
        _In_z_                       unsigned char const* _Src,
        _In_                         size_t               _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsncpy_s,
               unsigned char,        _Dest,
        _In_z_ unsigned char const*, _Source,
        _In_   size_t,               _Count
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsncpy,
        _Pre_notnull_ _Out_writes_(2 * _Count) _Post_maybez_, unsigned char,        _Dest,
        _In_z_                                                unsigned char const*, _Source,
        _In_                                                  size_t,               _Count
        )

    _DCRTIMP errno_t __cdecl _mbsncpy_s_l(
        _Out_writes_z_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                            size_t               _DstSizeInBytes,
        _In_z_                          unsigned char const* _Src,
        _In_                            size_t               _MaxCount,
        _In_opt_                        _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsncpy_s_l,
                 unsigned char,        _Dest,
        _In_z_   unsigned char const*, _Source,
        _In_     size_t,               _Count,
        _In_opt_ _locale_t,            _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsncpy_l, _mbsncpy_s_l,
        _Out_writes_z_(_Size)               unsigned char,
        _Out_writes_(_Count) _Post_maybez_, unsigned char,        _Dest,
        _In_z_                              unsigned char const*, _Source,
        _In_                                size_t,               _Count,
        _In_opt_                            _locale_t,            _Locale
        )

    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbsnextc(
        _In_z_ unsigned char const* _String
        );

    _Check_return_
    _DCRTIMP unsigned int __cdecl _mbsnextc_l(
        _In_z_   unsigned char const* _String,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnicmp(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnicmp_l(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnicoll(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str2,
        _In_   size_t               _MaxCount
        );

    _Check_return_
    _DCRTIMP int __cdecl _mbsnicoll_l(
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str1,
        _In_reads_or_z_(_MaxCount) unsigned char const* _Str2,
        _In_     size_t               _MaxCount,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP unsigned char*  __cdecl _mbsninc(
        _In_reads_or_z_(_Count) unsigned char const* _String,
        _In_                    size_t               _Count
        );

    _Check_return_
    _DCRTIMP unsigned char*  __cdecl _mbsninc_l(
        _In_reads_or_z_(_Count) unsigned char const* _String,
        _In_                    size_t               _Count,
        _In_opt_                _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsnset_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char* _Dst,
        _In_                            size_t         _SizeInBytes,
        _In_                            unsigned int   _Val,
        _In_                            size_t         _MaxCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsnset_s,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val,
        _In_        size_t,        _MaxCount
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsnset, _mbsnset_s,
        _Inout_updates_z_(_Size)      unsigned char,
        _Inout_updates_z_(_MaxCount), unsigned char, _String,
        _In_                          unsigned int,  _Val,
        _In_                          size_t,        _MaxCount
        )

    _DCRTIMP errno_t __cdecl _mbsnset_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char* _Dst,
        _In_                               size_t         _DstSizeInBytes,
        _In_                               unsigned int   _Val,
        _In_                               size_t         _MaxCount,
        _In_opt_                          _locale_t       _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbsnset_s_l,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val,
        _In_        size_t,        _MaxCount,
        _In_opt_    _locale_t,     _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(
        unsigned char* , __RETURN_POLICY_DST, _DCRTIMP, _mbsnset_l, _mbsnset_s_l,
        _Inout_updates_z_(_Size)      unsigned char,
        _Inout_updates_z_(_MaxCount), unsigned char, _String,
        _In_                          unsigned int,  _Val,
        _In_                          size_t,        _MaxCount,
        _In_opt_                      _locale_t,     _Locale
        )

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbspbrk(
        _In_z_ unsigned char const* _String,
        _In_z_ unsigned char const* _Control
        );

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbspbrk_l(
        _In_z_   unsigned char const* _String,
        _In_z_   unsigned char const* _Control,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbsrchr(
        _In_z_ unsigned char const* _String,
        _In_   unsigned int         _C
        );

    _Check_return_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbsrchr_l(
        _In_z_   unsigned char const* _String,
        _In_     unsigned int         _C,
        _In_opt_ _locale_t            _Locale
        );

    _DCRTIMP unsigned char* __cdecl _mbsrev(
        _Inout_z_ unsigned char* _String
        );

    _DCRTIMP unsigned char* __cdecl _mbsrev_l(
        _Inout_z_ unsigned char* _String,
        _In_opt_  _locale_t      _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsset_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char* _Dst,
        _In_                            size_t         _SizeInBytes,
        _In_                            unsigned int   _Val
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        errno_t, _mbsset_s,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsset, _mbsset_s,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char, _String,
        _In_                     unsigned int,  _Val
        )

    _DCRTIMP errno_t __cdecl _mbsset_s_l(
        _Inout_updates_z_(_DstSizeInBytes) unsigned char* _Dst,
        _In_                               size_t         _DstSizeInBytes,
        _In_                               unsigned int   _Val,
        _In_opt_                           _locale_t      _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbsset_s_l,
        _Prepost_z_ unsigned char, _Dest,
        _In_        unsigned int,  _Val,
        _In_opt_    _locale_t,     _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsset_l, _mbsset_s_l,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char, _String,
        _In_                     unsigned int,  _Val,
        _In_opt_                 _locale_t,     _Locale
        )

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsspn(
        _In_z_ unsigned char const* _Str,
        _In_z_ unsigned char const* _Control
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbsspn_l(
        _In_z_   unsigned char const* _Str,
        _In_z_   unsigned char const* _Control,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsspnp(
        _In_z_ unsigned char const* _Str1,
        _In_z_ unsigned char const* _Str2
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbsspnp_l(
        _In_z_   unsigned char const* _Str1,
        _In_z_   unsigned char const* _Str2,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_ _Ret_maybenull_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbsstr(
        _In_z_ unsigned char const* _Str,
        _In_z_ unsigned char const* _Substr
        );

    _Check_return_ _Ret_maybenull_
    _DCRTIMP _CONST_RETURN unsigned char* __cdecl _mbsstr_l(
        _In_z_   unsigned char const* _Str,
        _In_z_   unsigned char const* _Substr,
        _In_opt_ _locale_t            _Locale
        );

    _Check_return_ _CRT_INSECURE_DEPRECATE(_mbstok_s)
    _DCRTIMP unsigned char* __cdecl _mbstok(
        _Inout_opt_z_ unsigned char*       _Str,
        _In_z_        unsigned char const* _Delim
        );

    _Check_return_ _CRT_INSECURE_DEPRECATE(_mbstok_s_l)
    _DCRTIMP unsigned char* __cdecl _mbstok_l(
        _Inout_opt_z_ unsigned char*       _Str,
        _In_z_        unsigned char const* _Delim,
        _In_opt_      _locale_t            _Locale
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbstok_s(
        _Inout_opt_z_                 unsigned char*       _Str,
        _In_z_                        unsigned char const* _Delim,
        _Inout_ _Deref_prepost_opt_z_ unsigned char**      _Context
        );

    _Check_return_
    _DCRTIMP unsigned char* __cdecl _mbstok_s_l(
        _Inout_opt_z_                 unsigned char*       _Str,
        _In_z_                        unsigned char const* _Delim,
        _Inout_ _Deref_prepost_opt_z_ unsigned char**      _Context,
        _In_opt_                      _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbsupr_s(
        _Inout_updates_z_(_SizeInBytes) unsigned char* _Str,
        _In_                            size_t         _SizeInBytes
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        errno_t, _mbsupr_s,
        _Inout_updates_z_(_Size) unsigned char, _String
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsupr,
        _Inout_z_, unsigned char, _String
        )

    _DCRTIMP errno_t __cdecl _mbsupr_s_l(
        _Inout_updates_z_(_SizeInBytes) unsigned char* _Str,
        _In_                            size_t         _SizeInBytes,
        _In_opt_                        _locale_t      _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        errno_t, _mbsupr_s_l,
                 unsigned char, _String,
        _In_opt_ _locale_t,     _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(
        unsigned char*, __RETURN_POLICY_DST, _DCRTIMP, _mbsupr_l, _mbsupr_s_l,
        _Inout_updates_z_(_Size) unsigned char,
        _Inout_z_,               unsigned char, _String,
        _In_opt_                 _locale_t,     _Locale
        )

    _Check_return_
    _DCRTIMP size_t __cdecl _mbclen(
        _In_z_ unsigned char const* _String
        );

    _Check_return_
    _DCRTIMP size_t __cdecl _mbclen_l(
        _In_z_   unsigned char const* _String,
        _In_opt_ _locale_t            _Locale
        );

    _CRT_INSECURE_DEPRECATE(_mbccpy_s)
    _DCRTIMP void __cdecl _mbccpy(
        _Out_writes_bytes_(2) unsigned char*       _Dst,
        _In_z_                unsigned char const* _Src
        );

    _CRT_INSECURE_DEPRECATE(_mbccpy_s_l)
    _DCRTIMP void __cdecl _mbccpy_l(
        _Out_writes_bytes_(2) unsigned char*       _Dst,
        _In_z_                unsigned char const* _Src,
        _In_opt_              _locale_t            _Locale
        );

    _DCRTIMP errno_t __cdecl _mbccpy_s(
        _Out_writes_z_(_SizeInBytes) unsigned char*       _Dst,
        _In_                         size_t               _SizeInBytes,
        _Out_opt_                    int*                 _PCopied,
        _In_z_                       unsigned char const* _Src
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        errno_t, _mbccpy_s,
                  unsigned char,        _Dest,
        _Out_opt_ int*,                 _PCopied,
        _In_z_    unsigned char const*, _Source
        )

    _DCRTIMP errno_t __cdecl _mbccpy_s_l(
        _Out_writes_bytes_(_DstSizeInBytes) unsigned char*       _Dst,
        _In_                                size_t               _DstSizeInBytes,
        _Out_opt_                           int*                 _PCopied,
        _In_z_                              unsigned char const* _Src,
        _In_opt_                            _locale_t            _Locale
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        errno_t, _mbccpy_s_l,
                  unsigned char,        _Dest,
        _Out_opt_ int*,                 _PCopied,
        _In_z_    unsigned char const*, _Source,
        _In_opt_  _locale_t,            _Locale
        )

    #define _mbccmp(_cpc1, _cpc2) _mbsncmp((_cpc1), (_cpc2), 1)



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Const-Correct C++ Overloads
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    #ifndef _EXCLUDE_MBSTRING_CONST_CORRECT_OVERLOADS
    #ifdef __cplusplus
    extern "C++" {

        _Check_return_
        inline unsigned char* __CRTDECL _mbschr(
            _In_z_ unsigned char* const _String,
            _In_   unsigned int   const _Char
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbschr(_ConstString, _Char));
        }

        _Check_return_
        inline unsigned char* __CRTDECL _mbschr_l(
            _In_z_   unsigned char* const _String,
            _In_     unsigned int   const _Char,
            _In_opt_ _locale_t      const _Locale
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbschr_l(_ConstString, _Char, _Locale));
        }

        _Check_return_
        inline unsigned char* __CRTDECL _mbspbrk(
            _In_z_ unsigned char*       const _String,
            _In_z_ unsigned char const* const _CharSet
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbspbrk(_ConstString, _CharSet));
        }

        _Check_return_
        inline unsigned char* __CRTDECL _mbspbrk_l(
            _In_z_   unsigned char*       const _String,
            _In_z_   unsigned char const* const _CharSet,
            _In_opt_ _locale_t            const _Locale
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbspbrk_l(_ConstString, _CharSet, _Locale));
        }

        _Check_return_
        inline unsigned char* __CRTDECL _mbsrchr(
            _In_z_ unsigned char* const _String,
            _In_   unsigned int   const _Char
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbsrchr(_ConstString, _Char));
        }

        _Check_return_
        inline unsigned char* __CRTDECL _mbsrchr_l(
            _In_z_   unsigned char* const _String,
            _In_     unsigned int   const _Char,
            _In_opt_ _locale_t      const _Locale
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbsrchr_l(_ConstString, _Char, _Locale));
        }

        _Check_return_ _Ret_maybenull_
        inline unsigned char* __CRTDECL _mbsstr(
            _In_z_ unsigned char*       const _String,
            _In_z_ unsigned char const* const _Match
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbsstr(_ConstString, _Match));
        }

        _Check_return_ _Ret_maybenull_
        inline unsigned char* __CRTDECL _mbsstr_l(
            _In_z_   unsigned char*       const _String,
            _In_z_   unsigned char const* const _Match,
            _In_opt_ _locale_t            const _Locale
            )
        {
            unsigned char const* const _ConstString = _String;
            return const_cast<unsigned char*>(_mbsstr_l(_ConstString, _Match, _Locale));
        }

    } // extern "C++"
    #endif // __cplusplus
    #endif // _EXCLUDE_MBSTRING_CONST_CORRECT_OVERLOADS



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Multibyte Character Classification and Conversion Functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_ _DCRTIMP int __cdecl _ismbcalnum(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcalpha(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcdigit(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcgraph(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbclegal(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbclower(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcprint(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcpunct(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcblank(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcspace(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcupper(_In_ unsigned int _C);

    _Check_return_ _DCRTIMP int __cdecl _ismbcalnum_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcalpha_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcdigit_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcgraph_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbclegal_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbclower_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcprint_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcpunct_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcblank_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcspace_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcupper_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);

    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctolower(_In_ unsigned int _Ch);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctoupper(_In_ unsigned int _Ch);

    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctolower_l(_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctoupper_l(_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Multibyte Character Kanji Classification and Conversion Functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_ _DCRTIMP int __cdecl _ismbchira  (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbckata  (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcsymbol(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl0    (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl1    (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl2    (_In_ unsigned int _C);

    _Check_return_ _DCRTIMP int __cdecl _ismbchira_l  (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbckata_l  (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcsymbol_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl0_l    (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl1_l    (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbcl2_l    (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);

    _Check_return_ _DCRTIMP unsigned int __cdecl _mbcjistojms(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbcjmstojis(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctohira  (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctokata  (_In_ unsigned int _C);

    _Check_return_ _DCRTIMP unsigned int __cdecl _mbcjistojms_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbcjmstojis_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctohira_l  (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP unsigned int __cdecl _mbctokata_l  (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);

#endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP



#if _CRT_FUNCTIONS_REQUIRED
    // Lead and trail bytes do not apply correctly to all encodings, including UTF-8.  Applications
    // are recommended to use the system codepage conversion APIs and not attempt to reverse
    // engineer the behavior of any particular encoding.  Lead and trail are always FALSE for UTF-8.
    _When_(_Ch == 0, _Post_equal_to_(0))
    _Check_return_ _DCRTIMP int __cdecl _ismbblead (_In_ unsigned int _Ch);
    _Check_return_ _DCRTIMP int __cdecl _ismbbtrail(_In_ unsigned int _Ch);

    _When_(_Ch == 0, _Post_equal_to_(0))
    _Check_return_ _DCRTIMP int __cdecl _ismbblead_l (_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbtrail_l(_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);

    _Check_return_
    _DCRTIMP int __cdecl _ismbslead(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos
        );

    _Check_return_
    _DCRTIMP int __cdecl _ismbslead_l(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos,
        _In_opt_                         _locale_t            _Locale
        );

    _Check_return_
    _ACRTIMP int __cdecl _ismbstrail(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos
        );

    _Check_return_
    _ACRTIMP int __cdecl _ismbstrail_l(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos,
        _In_opt_                         _locale_t            _Locale
        );
#endif // _CRT_FUNCTIONS_REQUIRED



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_MBSTRING
