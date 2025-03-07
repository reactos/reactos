//
// corecrt_wstdlib.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) C Standard Library functions
// that are declared by both <stdlib.h> and <wchar.h>.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



// Maximum number of elements, including null terminator (and negative sign
// where appropriate), needed for integer-to-string conversions for several
// bases and integer types.
#define _MAX_ITOSTR_BASE16_COUNT   (8  + 1)
#define _MAX_ITOSTR_BASE10_COUNT   (1 + 10 + 1)
#define _MAX_ITOSTR_BASE8_COUNT    (11 + 1)
#define _MAX_ITOSTR_BASE2_COUNT    (32 + 1)

#define _MAX_LTOSTR_BASE16_COUNT   (8  + 1)
#define _MAX_LTOSTR_BASE10_COUNT   (1 + 10 + 1)
#define _MAX_LTOSTR_BASE8_COUNT    (11 + 1)
#define _MAX_LTOSTR_BASE2_COUNT    (32 + 1)

#define _MAX_ULTOSTR_BASE16_COUNT  (8  + 1)
#define _MAX_ULTOSTR_BASE10_COUNT  (10 + 1)
#define _MAX_ULTOSTR_BASE8_COUNT   (11 + 1)
#define _MAX_ULTOSTR_BASE2_COUNT   (32 + 1)

#define _MAX_I64TOSTR_BASE16_COUNT (16 + 1)
#define _MAX_I64TOSTR_BASE10_COUNT (1 + 19 + 1)
#define _MAX_I64TOSTR_BASE8_COUNT  (22 + 1)
#define _MAX_I64TOSTR_BASE2_COUNT  (64 + 1)

#define _MAX_U64TOSTR_BASE16_COUNT (16 + 1)
#define _MAX_U64TOSTR_BASE10_COUNT (20 + 1)
#define _MAX_U64TOSTR_BASE8_COUNT  (22 + 1)
#define _MAX_U64TOSTR_BASE2_COUNT  (64 + 1)


#if _CRT_FUNCTIONS_REQUIRED

    _Success_(return == 0)
    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _itow_s(
        _In_                         int      _Value,
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         size_t   _BufferCount,
        _In_                         int      _Radix
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(
        errno_t, _itow_s,
        _In_ int,     _Value,
             wchar_t, _Buffer,
        _In_ int,     _Radix
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(
        wchar_t*, __RETURN_POLICY_DST, _ACRTIMP, _itow,
        _In_                    int,     _Value,
        _Pre_notnull_ _Post_z_, wchar_t, _Buffer,
        _In_                    int,     _Radix
        )

    _Success_(return == 0)
    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _ltow_s(
        _In_                         long     _Value,
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         size_t   _BufferCount,
        _In_                         int      _Radix
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(
        errno_t, _ltow_s,
        _In_ long,    _Value,
             wchar_t, _Buffer,
        _In_ int,     _Radix
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(
        wchar_t*, __RETURN_POLICY_DST, _ACRTIMP, _ltow,
        _In_                    long,    _Value,
        _Pre_notnull_ _Post_z_, wchar_t, _Buffer,
        _In_                    int,     _Radix
        )

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _ultow_s(
        _In_                         unsigned long _Value,
        _Out_writes_z_(_BufferCount) wchar_t*      _Buffer,
        _In_                         size_t        _BufferCount,
        _In_                         int           _Radix
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(
        errno_t, _ultow_s,
        _In_ unsigned long, _Value,
             wchar_t,       _Buffer,
        _In_ int,           _Radix
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(
        wchar_t*, __RETURN_POLICY_DST, _ACRTIMP, _ultow,
        _In_                    unsigned long, _Value,
        _Pre_notnull_ _Post_z_, wchar_t,       _Buffer,
        _In_                    int,           _Radix
        )

    _Check_return_
    _ACRTIMP double __cdecl wcstod(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr
        );

    _Check_return_
    _ACRTIMP double __cdecl _wcstod_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP long __cdecl wcstol(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP long __cdecl _wcstol_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP long long __cdecl wcstoll(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP long long __cdecl _wcstoll_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP unsigned long __cdecl wcstoul(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP unsigned long __cdecl _wcstoul_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP unsigned long long __cdecl wcstoull(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP unsigned long long __cdecl _wcstoull_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP long double __cdecl wcstold(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr
        );

    _Check_return_
    _ACRTIMP long double __cdecl _wcstold_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP float __cdecl wcstof(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr
        );

    _Check_return_
    _ACRTIMP float __cdecl _wcstof_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP double __cdecl _wtof(
        _In_z_ wchar_t const* _String
        );

    _Check_return_
    _ACRTIMP double __cdecl _wtof_l(
        _In_z_   wchar_t const* _String,
        _In_opt_ _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP int __cdecl _wtoi(
        _In_z_ wchar_t const* _String
        );

    _Check_return_
    _ACRTIMP int __cdecl _wtoi_l(
        _In_z_   wchar_t const* _String,
        _In_opt_ _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP long __cdecl _wtol(
        _In_z_ wchar_t const* _String
        );

    _Check_return_
    _ACRTIMP long __cdecl _wtol_l(
        _In_z_   wchar_t const* _String,
        _In_opt_ _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP long long __cdecl _wtoll(
        _In_z_ wchar_t const* _String
        );

    _Check_return_
    _ACRTIMP long long __cdecl _wtoll_l(
        _In_z_   wchar_t const* _String,
        _In_opt_ _locale_t      _Locale
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _i64tow_s(
        _In_                         __int64  _Value,
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         size_t   _BufferCount,
        _In_                         int      _Radix
        );

    _CRT_INSECURE_DEPRECATE(_i64tow_s)
    _ACRTIMP wchar_t* __cdecl _i64tow(
        _In_                   __int64  _Value,
        _Pre_notnull_ _Post_z_ wchar_t* _Buffer,
        _In_                   int      _Radix
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _ui64tow_s(
        _In_                         unsigned __int64 _Value,
        _Out_writes_z_(_BufferCount) wchar_t*         _Buffer,
        _In_                         size_t           _BufferCount,
        _In_                         int              _Radix
        );

    _CRT_INSECURE_DEPRECATE(_ui64tow_s)
    _ACRTIMP wchar_t* __cdecl _ui64tow(
        _In_                   unsigned __int64 _Value,
        _Pre_notnull_ _Post_z_ wchar_t*         _Buffer,
        _In_                   int              _Radix
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _wtoi64(
        _In_z_ wchar_t const* _String
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _wtoi64_l(
        _In_z_   wchar_t const* _String,
        _In_opt_ _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _wcstoi64(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _wcstoi64_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    _Check_return_
    _ACRTIMP unsigned __int64 __cdecl _wcstoui64(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix
        );

    _Check_return_
    _ACRTIMP unsigned __int64 __cdecl _wcstoui64_l(
        _In_z_                   wchar_t const* _String,
        _Out_opt_ _Deref_post_z_ wchar_t**      _EndPtr,
        _In_                     int            _Radix,
        _In_opt_                 _locale_t      _Locale
        );

    #pragma push_macro("_wfullpath")
    #undef _wfullpath

    _Success_(return != 0)
    _Check_return_
    _ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wfullpath(
        _Out_writes_opt_z_(_BufferCount) wchar_t*       _Buffer,
        _In_z_                           wchar_t const* _Path,
        _In_                             size_t         _BufferCount
        );

    #pragma pop_macro("_wfullpath")

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _wmakepath_s(
        _Out_writes_z_(_BufferCount) wchar_t*       _Buffer,
        _In_                         size_t         _BufferCount,
        _In_opt_z_                   wchar_t const* _Drive,
        _In_opt_z_                   wchar_t const* _Dir,
        _In_opt_z_                   wchar_t const* _Filename,
        _In_opt_z_                   wchar_t const* _Ext
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_4(
        errno_t, _wmakepath_s,
                   wchar_t,        _Buffer,
        _In_opt_z_ wchar_t const*, _Drive,
        _In_opt_z_ wchar_t const*, _Dir,
        _In_opt_z_ wchar_t const*, _Filename,
        _In_opt_z_ wchar_t const*, _Ext
        )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4(
        void, __RETURN_POLICY_VOID, _ACRTIMP, _wmakepath,
        _Pre_notnull_ _Post_z_, wchar_t,        _Buffer,
        _In_opt_z_              wchar_t const*, _Drive,
        _In_opt_z_              wchar_t const*, _Dir,
        _In_opt_z_              wchar_t const*, _Filename,
        _In_opt_z_              wchar_t const*, _Ext
        )

    _ACRTIMP void __cdecl _wperror(
        _In_opt_z_ wchar_t const* _ErrorMessage
        );

    _CRT_INSECURE_DEPRECATE(_wsplitpath_s)
    _ACRTIMP void __cdecl _wsplitpath(
        _In_z_                   wchar_t const* _FullPath,
        _Pre_maybenull_ _Post_z_ wchar_t*       _Drive,
        _Pre_maybenull_ _Post_z_ wchar_t*       _Dir,
        _Pre_maybenull_ _Post_z_ wchar_t*       _Filename,
        _Pre_maybenull_ _Post_z_ wchar_t*       _Ext
        );

    _ACRTIMP errno_t __cdecl _wsplitpath_s(
        _In_z_                             wchar_t const* _FullPath,
        _Out_writes_opt_z_(_DriveCount)    wchar_t*       _Drive,
        _In_                               size_t         _DriveCount,
        _Out_writes_opt_z_(_DirCount)      wchar_t*       _Dir,
        _In_                               size_t         _DirCount,
        _Out_writes_opt_z_(_FilenameCount) wchar_t*       _Filename,
        _In_                               size_t         _FilenameCount,
        _Out_writes_opt_z_(_ExtCount)      wchar_t*       _Ext,
        _In_                               size_t         _ExtCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_SPLITPATH(
        errno_t, _wsplitpath_s,
        wchar_t, _Path
        )

        #pragma push_macro("_wdupenv_s")
        #undef _wdupenv_s

        _Check_return_wat_
        _DCRTIMP errno_t __cdecl _wdupenv_s(
            _Outptr_result_buffer_maybenull_(*_BufferCount) _Outptr_result_maybenull_z_ wchar_t**      _Buffer,
            _Out_opt_                                                                   size_t*        _BufferCount,
            _In_z_                                                                      wchar_t const* _VarName
            );

        #pragma pop_macro("_wdupenv_s")

        _Check_return_ _CRT_INSECURE_DEPRECATE(_wdupenv_s)
        _DCRTIMP wchar_t* __cdecl _wgetenv(
            _In_z_ wchar_t const* _VarName
            );

        _Success_(return == 0)
        _Check_return_wat_
        _DCRTIMP errno_t __cdecl _wgetenv_s(
            _Out_                            size_t*        _RequiredCount,
            _Out_writes_opt_z_(_BufferCount) wchar_t*       _Buffer,
            _In_                             size_t         _BufferCount,
            _In_z_                           wchar_t const* _VarName
            );

        __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(
            _Success_(return == 0)
            errno_t, _wgetenv_s,
            _Out_  size_t*,        _RequiredCount,
                   wchar_t,        _Buffer,
            _In_z_ wchar_t const*, _VarName
            )

        _Check_return_
        _DCRTIMP int __cdecl _wputenv(
            _In_z_ wchar_t const* _EnvString
            );

        _Check_return_wat_
        _DCRTIMP errno_t __cdecl _wputenv_s(
            _In_z_ wchar_t const* _Name,
            _In_z_ wchar_t const* _Value
            );

        _DCRTIMP errno_t __cdecl _wsearchenv_s(
            _In_z_                       wchar_t const* _Filename,
            _In_z_                       wchar_t const* _VarName,
            _Out_writes_z_(_BufferCount) wchar_t*       _Buffer,
            _In_                         size_t         _BufferCount
            );

        __DEFINE_CPP_OVERLOAD_SECURE_FUNC_2_0(
            errno_t, _wsearchenv_s,
            _In_z_ wchar_t const*, _Filename,
            _In_z_ wchar_t const*, _VarName,
                   wchar_t,        _ResultPath
                   )

        __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0(
            void, __RETURN_POLICY_VOID, _DCRTIMP, _wsearchenv,
            _In_z_                  wchar_t const*, _Filename,
            _In_z_                  wchar_t const*, _VarName,
            _Pre_notnull_ _Post_z_, wchar_t,        _ResultPath
            )

        _DCRTIMP int __cdecl _wsystem(
            _In_opt_z_ wchar_t const* _Command
            );

#endif // _CRT_FUNCTIONS_REQUIRED



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
