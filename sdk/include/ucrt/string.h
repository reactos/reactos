//
// string.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <string.h> header.
//
#pragma once
#ifndef _INC_STRING // include guard for 3rd party interop
#define _INC_STRING

#include <corecrt.h>
#include <corecrt_memory.h>
#include <corecrt_wstring.h>
#include <vcruntime_string.h>

#ifndef __midl

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#define _NLSCMPERROR _CRT_INT_MAX // currently == INT_MAX

#if __STDC_WANT_SECURE_LIB__

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl strcpy_s(
        _Out_writes_z_(_SizeInBytes) char*       _Destination,
        _In_                         rsize_t     _SizeInBytes,
        _In_z_                       char const* _Source
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl strcat_s(
        _Inout_updates_z_(_SizeInBytes) char*       _Destination,
        _In_                            rsize_t     _SizeInBytes,
        _In_z_                          char const* _Source
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl strerror_s(
        _Out_writes_z_(_SizeInBytes) char*  _Buffer,
        _In_                         size_t _SizeInBytes,
        _In_                         int    _ErrorNumber);

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl strncat_s(
        _Inout_updates_z_(_SizeInBytes) char*       _Destination,
        _In_                            rsize_t     _SizeInBytes,
        _In_reads_or_z_(_MaxCount)      char const* _Source,
        _In_                            rsize_t     _MaxCount
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl strncpy_s(
        _Out_writes_z_(_SizeInBytes) char*       _Destination,
        _In_                         rsize_t     _SizeInBytes,
        _In_reads_or_z_(_MaxCount)   char const* _Source,
        _In_                         rsize_t     _MaxCount
        );

    _Check_return_
    _ACRTIMP char*  __cdecl strtok_s(
        _Inout_opt_z_                 char*       _String,
        _In_z_                        char const* _Delimiter,
        _Inout_ _Deref_prepost_opt_z_ char**      _Context
        );

#endif // __STDC_WANT_SECURE_LIB__

_ACRTIMP void* __cdecl _memccpy(
    _Out_writes_bytes_opt_(_MaxCount) void*       _Dst,
    _In_                              void const* _Src,
    _In_                              int         _Val,
    _In_                              size_t      _MaxCount
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, strcat_s,
           char,        _Destination,
    _In_z_ char const*, _Source
    )

#ifndef RC_INVOKED

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(
        char*, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, strcat,
        _Inout_updates_z_(_String_length_(_Destination) + _String_length_(_Source) + 1), char,        _Destination,
        _In_z_                                                                           char const*, _Source
        )

#endif // RC_INVOKED

_Check_return_
int __cdecl strcmp(
    _In_z_ char const* _Str1,
    _In_z_ char const* _Str2
    );

_Check_return_
_ACRTIMP int __cdecl _strcmpi(
    _In_z_ char const* _String1,
    _In_z_ char const* _String2
    );

_Check_return_
_ACRTIMP int __cdecl strcoll(
    _In_z_ char const* _String1,
    _In_z_ char const* _String2
    );

_Check_return_
_ACRTIMP int __cdecl _strcoll_l(
    _In_z_   char const* _String1,
    _In_z_   char const* _String2,
    _In_opt_ _locale_t   _Locale
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, strcpy_s,
    _Post_z_ char,        _Destination,
    _In_z_   char const*, _Source
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(
    char*, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, strcpy,
    _Out_writes_z_(_String_length_(_Source) + 1), char,        _Destination,
    _In_z_                                        char const*, _Source
    )

_Check_return_
_ACRTIMP size_t __cdecl strcspn(
    _In_z_ char const* _Str,
    _In_z_ char const* _Control
    );

#if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
    #pragma push_macro("_strdup")
    #undef _strdup
#endif

_Check_return_
_ACRTIMP _CRTALLOCATOR char* __cdecl _strdup(
    _In_opt_z_ char const* _Source
    );

#if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
    #pragma pop_macro("_strdup")
#endif

_Ret_z_
_Success_(return != 0)
_Check_return_ _CRT_INSECURE_DEPRECATE(_strerror_s)
_ACRTIMP char*  __cdecl _strerror(
    _In_opt_z_ char const* _ErrorMessage
    );

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strerror_s(
    _Out_writes_z_(_SizeInBytes) char*       _Buffer,
    _In_                         size_t      _SizeInBytes,
    _In_opt_z_                   char const* _ErrorMessage
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, _strerror_s,
               char,        _Buffer,
    _In_opt_z_ char const*, _ErrorMessage
    )

_Ret_z_
_Check_return_ _CRT_INSECURE_DEPRECATE(strerror_s)
_ACRTIMP char* __cdecl strerror(
    _In_ int _ErrorMessage
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, strerror_s,
        char, _Buffer,
    _In_ int, _ErrorMessage
    )

_Check_return_
_ACRTIMP int __cdecl _stricmp(
    _In_z_ char const* _String1,
    _In_z_ char const* _String2
    );

_Check_return_
_ACRTIMP int __cdecl _stricoll(
    _In_z_ char const* _String1,
    _In_z_ char const* _String2
    );

_Check_return_
_ACRTIMP int __cdecl _stricoll_l(
    _In_z_   char const* _String1,
    _In_z_   char const* _String2,
    _In_opt_ _locale_t   _Locale
    );

_Check_return_
_ACRTIMP int __cdecl _stricmp_l(
    _In_z_   char const* _String1,
    _In_z_   char const* _String2,
    _In_opt_ _locale_t   _Locale
    );

_Check_return_
size_t __cdecl strlen(
    _In_z_ char const* _Str
    );

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strlwr_s(
    _Inout_updates_z_(_Size) char*  _String,
    _In_                     size_t _Size
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
    errno_t, _strlwr_s,
    _Prepost_z_ char, _String
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
    char*, __RETURN_POLICY_DST, _ACRTIMP, _strlwr,
    _Inout_z_, char, _String
    )

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strlwr_s_l(
    _Inout_updates_z_(_Size) char*     _String,
    _In_                     size_t    _Size,
    _In_opt_                 _locale_t _Locale
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, _strlwr_s_l,
    _Prepost_z_ char,      _String,
    _In_opt_    _locale_t, _Locale
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(
    char*, __RETURN_POLICY_DST, _ACRTIMP, _strlwr_l, _strlwr_s_l,
    _Inout_updates_z_(_Size) char,
    _Inout_z_,               char,      _String,
    _In_opt_                 _locale_t, _Locale
    )

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
    errno_t, strncat_s,
    _Prepost_z_             char,        _Destination,
    _In_reads_or_z_(_Count) char const*, _Source,
    _In_                    size_t,      _Count
    )

__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(
    char*, __RETURN_POLICY_DST, _ACRTIMP, strncat, strncat_s,
    _Inout_updates_z_(_Size)   char,
    _Inout_updates_z_(_Count), char,        _Destination,
    _In_reads_or_z_(_Count)    char const*, _Source,
    _In_                       size_t,      _Count
    )

_Check_return_
_ACRTIMP int __cdecl strncmp(
    _In_reads_or_z_(_MaxCount) char const* _Str1,
    _In_reads_or_z_(_MaxCount) char const* _Str2,
    _In_                       size_t      _MaxCount
    );

_Check_return_
_ACRTIMP int __cdecl _strnicmp(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount
    );

_Check_return_
_ACRTIMP int __cdecl _strnicmp_l(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount,
    _In_opt_                   _locale_t   _Locale
    );

_Check_return_
_ACRTIMP int __cdecl _strnicoll(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount
    );

_Check_return_
_ACRTIMP int __cdecl _strnicoll_l(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount,
    _In_opt_                   _locale_t   _Locale
    );

_Check_return_
_ACRTIMP int __cdecl _strncoll(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount
    );

_Check_return_
_ACRTIMP int __cdecl _strncoll_l(
    _In_reads_or_z_(_MaxCount) char const* _String1,
    _In_reads_or_z_(_MaxCount) char const* _String2,
    _In_                       size_t      _MaxCount,
    _In_opt_                   _locale_t   _Locale
    );

_ACRTIMP size_t __cdecl __strncnt(
    _In_reads_or_z_(_Count) char const* _String,
    _In_                    size_t      _Count
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
    errno_t, strncpy_s,
                            char,        _Destination,
    _In_reads_or_z_(_Count) char const*, _Source,
    _In_                    size_t,      _Count
    )

__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(
    char*, __RETURN_POLICY_DST, _ACRTIMP, strncpy, strncpy_s,
    _Out_writes_z_(_Size)               char,
    _Out_writes_(_Count) _Post_maybez_, char,        _Destination,
    _In_reads_or_z_(_Count)             char const*, _Source,
    _In_                                size_t,      _Count
    )

_Check_return_
_When_(
    _MaxCount > _String_length_(_String),
    _Post_satisfies_(return == _String_length_(_String))
)
_When_(
    _MaxCount <= _String_length_(_String),
    _Post_satisfies_(return == _MaxCount)
)
_ACRTIMP size_t __cdecl strnlen(
    _In_reads_or_z_(_MaxCount) char const* _String,
    _In_                       size_t      _MaxCount
    );

#if __STDC_WANT_SECURE_LIB__ && !defined __midl

    _Check_return_
    _When_(
        _MaxCount > _String_length_(_String),
        _Post_satisfies_(return == _String_length_(_String))
    )
    _When_(
        _MaxCount <= _String_length_(_String),
        _Post_satisfies_(return == _MaxCount)
    )
    static __inline size_t __CRTDECL strnlen_s(
        _In_reads_or_z_(_MaxCount) char const* _String,
        _In_                       size_t      _MaxCount
        )
    {
        return _String == 0 ? 0 : strnlen(_String, _MaxCount);
    }

#endif

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strnset_s(
    _Inout_updates_z_(_SizeInBytes) char*  _String,
    _In_                            size_t _SizeInBytes,
    _In_                            int    _Value,
    _In_                            size_t _MaxCount
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
    errno_t, _strnset_s,
    _Prepost_z_ char,   _Destination,
    _In_        int,    _Value,
    _In_        size_t, _Count
    )

__DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(
    char*, __RETURN_POLICY_DST, _ACRTIMP, _strnset, _strnset_s,
    _Inout_updates_z_(_Size)   char,
    _Inout_updates_z_(_Count), char,   _Destination,
    _In_                       int,    _Value,
    _In_                       size_t, _Count
    )

_Check_return_
_ACRTIMP char _CONST_RETURN* __cdecl strpbrk(
    _In_z_ char const* _Str,
    _In_z_ char const* _Control
    );

_ACRTIMP char* __cdecl _strrev(
    _Inout_z_ char* _Str
    );

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strset_s(
    _Inout_updates_z_(_DestinationSize) char*  _Destination,
    _In_                                size_t _DestinationSize,
    _In_                                int    _Value
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, _strset_s,
    _Prepost_z_ char, _Destination,
    _In_        int,  _Value
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(
    char*, __RETURN_POLICY_DST, __EMPTY_DECLSPEC, _strset,
    _Inout_z_, char, _Destination,
    _In_       int,  _Value
    )

_Check_return_
_ACRTIMP size_t __cdecl strspn(
    _In_z_ char const* _Str,
    _In_z_ char const* _Control
    );

_Check_return_ _CRT_INSECURE_DEPRECATE(strtok_s)
_ACRTIMP char* __cdecl strtok(
    _Inout_opt_z_ char*       _String,
    _In_z_        char const* _Delimiter
    );

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strupr_s(
    _Inout_updates_z_(_Size) char*  _String,
    _In_                     size_t _Size
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
    errno_t, _strupr_s,
    _Prepost_z_ char, _String
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
    char*, __RETURN_POLICY_DST, _ACRTIMP, _strupr,
    _Inout_z_, char, _String
    )

_Check_return_wat_
_ACRTIMP errno_t __cdecl _strupr_s_l(
    _Inout_updates_z_(_Size) char*     _String,
    _In_                     size_t    _Size,
    _In_opt_                 _locale_t _Locale
    );

__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
    errno_t, _strupr_s_l,
    _Prepost_z_ char,      _String,
    _In_opt_    _locale_t, _Locale
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(
    char*, __RETURN_POLICY_DST, _ACRTIMP, _strupr_l, _strupr_s_l,
    _Inout_updates_z_(_Size) char,
    _Inout_z_,               char,      _String,
    _In_opt_                 _locale_t, _Locale
    )

_Success_(return < _MaxCount)
_Check_return_opt_
_ACRTIMP size_t __cdecl strxfrm(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ char*       _Destination,
    _In_z_                                    char const* _Source,
    _In_ _In_range_(<=,_CRT_INT_MAX)          size_t      _MaxCount
    );

_Success_(return < _MaxCount)
_Check_return_opt_
_ACRTIMP size_t __cdecl _strxfrm_l(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ char*       _Destination,
    _In_z_                                    char const* _Source,
    _In_ _In_range_(<=,_CRT_INT_MAX)          size_t      _MaxCount,
    _In_opt_                                  _locale_t   _Locale
    );



#ifdef __cplusplus
extern "C++"
{
    _Check_return_
    inline char* __CRTDECL strchr(_In_z_ char* const _String, _In_ int const _Ch)
    {
        return const_cast<char*>(strchr(static_cast<char const*>(_String), _Ch));
    }

    _Check_return_
    inline char* __CRTDECL strpbrk(_In_z_ char* const _String, _In_z_ char const* const _Control)
    {
        return const_cast<char*>(strpbrk(static_cast<char const*>(_String), _Control));
    }

    _Check_return_
    inline char* __CRTDECL strrchr(_In_z_ char* const _String, _In_ int const _Ch)
    {
        return const_cast<char*>(strrchr(static_cast<char const*>(_String), _Ch));
    }

    _Check_return_ _Ret_maybenull_
    inline char* __CRTDECL strstr(_In_z_ char* const _String, _In_z_ char const* const _SubString)
    {
        return const_cast<char*>(strstr(static_cast<char const*>(_String), _SubString));
    }
}
#endif // __cplusplus



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    #pragma push_macro("strdup")
    #undef strdup
    _Check_return_ _CRT_NONSTDC_DEPRECATE(_strdup)
    _ACRTIMP char* __cdecl strdup(
        _In_opt_z_ char const* _String
        );
    #pragma pop_macro("strdup")

    // Declarations of functions defined in oldnames.lib:
    _Check_return_ _CRT_NONSTDC_DEPRECATE(_strcmpi)
    _ACRTIMP int __cdecl strcmpi(
        _In_z_ char const* _String1,
        _In_z_ char const* _String2
        );

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_stricmp)
    _ACRTIMP int __cdecl stricmp(
        _In_z_ char const* _String1,
        _In_z_ char const* _String2
        );

    _CRT_NONSTDC_DEPRECATE(_strlwr)
    _ACRTIMP char* __cdecl strlwr(
        _Inout_z_ char* _String
        );

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_strnicmp)
    _ACRTIMP int __cdecl strnicmp(
        _In_reads_or_z_(_MaxCount) char const* _String1,
        _In_reads_or_z_(_MaxCount) char const* _String2,
        _In_                       size_t      _MaxCount
        );

    _CRT_NONSTDC_DEPRECATE(_strnset)
    _ACRTIMP char* __cdecl strnset(
        _Inout_updates_z_(_MaxCount) char*  _String,
        _In_                         int    _Value,
        _In_                         size_t _MaxCount
        );

    _CRT_NONSTDC_DEPRECATE(_strrev)
    _ACRTIMP char* __cdecl strrev(
        _Inout_z_ char* _String
        );

    _CRT_NONSTDC_DEPRECATE(_strset)
    char* __cdecl strset(
        _Inout_z_ char* _String,
        _In_      int   _Value);

    _CRT_NONSTDC_DEPRECATE(_strupr)
    _ACRTIMP char* __cdecl strupr(
        _Inout_z_ char* _String
        );

#endif // _CRT_INTERNAL_NONSTDC_NAMES



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // !__midl
#endif // _INC_STRING
