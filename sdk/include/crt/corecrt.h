/*
 * PROJECT:     ReactOS CRT headers
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Declarations used throughout the CoreCRT library.
 * COPYRIGHT:   Copyright (c) Microsoft Corporation. All rights reserved.
 */

#pragma once

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRTRESTRICT
#define _CRTRESTRICT
#endif

#ifndef DEFINED_localeinfo_struct
typedef struct localeinfo_struct
{
    pthreadlocinfo locinfo;
    pthreadmbcinfo mbcinfo;
} _locale_tstruct, *_locale_t;
#define DEFINED_localeinfo_struct 1
#endif

#ifndef RC_INVOKED
    #if defined __cplusplus && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(_ReturnType, _FuncName, _DstType, _Dst)     \
            extern "C++"                                                                          \
            {                                                                                     \
                template <size_t _Size>                                                           \
                inline                                                                            \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW  \
                {                                                                                 \
                    return _FuncName(_Dst, _Size);                                                \
                }                                                                                 \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1)   \
            extern "C++"                                                                                         \
            {                                                                                                    \
                template <size_t _Size>                                                                          \
                inline                                                                                           \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                \
                    return _FuncName(_Dst, _Size, _TArg1);                                                       \
                }                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)  \
            extern "C++"                                                                                                         \
            {                                                                                                                    \
                template <size_t _Size>                                                                                          \
                inline                                                                                                           \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                                \
                    return _FuncName(_Dst, _Size, _TArg1, _TArg2);                                                               \
                }                                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            extern "C++"                                                                                                                         \
            {                                                                                                                                    \
                template <size_t _Size>                                                                                                          \
                inline                                                                                                                           \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                                                \
                    return _FuncName(_Dst, _Size, _TArg1, _TArg2, _TArg3);                                                                       \
                }                                                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_4(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
            extern "C++"                                                                                                                                          \
            {                                                                                                                                                     \
                template <size_t _Size>                                                                                                                           \
                inline                                                                                                                                            \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW  \
                {                                                                                                                                                 \
                    return _FuncName(_Dst, _Size, _TArg1, _TArg2, _TArg3, _TArg4);                                                                                \
                }                                                                                                                                                 \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1)  \
            extern "C++"                                                                                                         \
            {                                                                                                                    \
                template <size_t _Size>                                                                                          \
                inline                                                                                                           \
                _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _DstType (&_Dst)[_Size], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                                \
                    return _FuncName(_HArg1, _Dst, _Size, _TArg1);                                                               \
                }                                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_2(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            extern "C++"                                                                                                                         \
            {                                                                                                                                    \
                template <size_t _Size>                                                                                                          \
                inline                                                                                                                           \
                _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                                                \
                    return _FuncName(_HArg1, _Dst, _Size, _TArg1, _TArg2);                                                                       \
                }                                                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            extern "C++"                                                                                                                                          \
            {                                                                                                                                                     \
                template <size_t _Size>                                                                                                                           \
                inline                                                                                                                                            \
                _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW  \
                {                                                                                                                                                 \
                    return _FuncName(_HArg1, _Dst, _Size, _TArg1, _TArg2, _TArg3);                                                                                \
                }                                                                                                                                                 \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_2_0(_ReturnType, _FuncName, _HType1, _HArg1, _HType2, _HArg2, _DstType, _Dst)  \
            extern "C++"                                                                                                         \
            {                                                                                                                    \
                template <size_t _Size>                                                                                          \
                inline                                                                                                           \
                _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _HType2 _HArg2, _DstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW \
                {                                                                                                                \
                    return _FuncName(_HArg1, _HArg2, _Dst, _Size);                                                               \
                }                                                                                                                \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(_ReturnType, _FuncName, _VFuncName, _DstType, _Dst, _TType1, _TArg1) \
            extern "C++"                                                                                                           \
            {                                                                                                                      \
                template <size_t _Size>                                                                                            \
                inline                                                                                                             \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW              \
                {                                                                                                                  \
                    va_list _ArgList;                                                                                              \
                    __crt_va_start(_ArgList, _TArg1);                                                                              \
                    return _VFuncName(_Dst, _Size, _TArg1, _ArgList);                                                              \
                }                                                                                                                  \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(_ReturnType, _FuncName, _VFuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            extern "C++"                                                                                                                            \
            {                                                                                                                                       \
                template <size_t _Size>                                                                                                             \
                inline                                                                                                                              \
                _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW               \
                {                                                                                                                                   \
                    va_list _ArgList;                                                                                                               \
                    __crt_va_start(_ArgList, _TArg2);                                                                                               \
                    return _VFuncName(_Dst, _Size, _TArg1, _TArg2, _ArgList);                                                                       \
                }                                                                                                                                   \
            }

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_SPLITPATH(_ReturnType, _FuncName, _DstType, _Src)               \
            extern "C++"                                                                                          \
            {                                                                                                     \
                template <size_t _DriveSize, size_t _DirSize, size_t _NameSize, size_t _ExtSize>                  \
                inline                                                                                            \
                _ReturnType __CRTDECL _FuncName(                                                                  \
                    _In_z_ _DstType const* _Src,                                                                  \
                    _Post_z_ _DstType (&_Drive)[_DriveSize],                                                      \
                    _Post_z_ _DstType (&_Dir)[_DirSize],                                                          \
                    _Post_z_ _DstType (&_Name)[_NameSize],                                                        \
                    _Post_z_ _DstType (&_Ext)[_ExtSize]                                                           \
                    ) _CRT_SECURE_CPP_NOTHROW                                                                     \
                {                                                                                                 \
                    return _FuncName(_Src, _Drive, _DriveSize, _Dir, _DirSize, _Name, _NameSize, _Ext, _ExtSize); \
                }                                                                                                 \
            }

    #else  // ^^^ _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES ^^^ // vvv !_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES vvv //

        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(_ReturnType, _FuncName, _DstType, _Dst)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_4(_ReturnType, _FuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_1(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_2(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(_ReturnType, _FuncName, _HType1, _HArg1, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_2_0(_ReturnType, _FuncName, _HType1, _HArg1, _HType2, _HArg2, _DstType, _Dst)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(_ReturnType, _FuncName, _VFuncName, _DstType, _Dst, _TType1, _TArg1)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(_ReturnType, _FuncName, _VFuncName, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)
        #define __DEFINE_CPP_OVERLOAD_SECURE_FUNC_SPLITPATH(_ReturnType, _FuncName, _DstType, _Src)

    #endif // !_CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES
#endif

#ifdef __cplusplus
} // extern "C"
#endif
