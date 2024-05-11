//
// corecrt.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Declarations used throughout the CoreCRT library.
//
#pragma once

#include <vcruntime.h>

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Windows API Partitioning and ARM Desktop Support
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
    #ifdef WINAPI_FAMILY
        #include <winapifamily.h>
        #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
            #define _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
        #else
            #ifdef WINAPI_FAMILY_PHONE_APP
                #if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
                    #define _CRT_USE_WINAPI_FAMILY_PHONE_APP
                #endif
            #endif

            #ifdef WINAPI_FAMILY_GAMES
                #if WINAPI_FAMILY == WINAPI_FAMILY_GAMES
                    #define _CRT_USE_WINAPI_FAMILY_GAMES
                #endif
            #endif
        #endif
    #else
        #define _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
    #endif
#endif

#ifndef _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
    #define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1
#endif

#ifndef _CRT_BUILD_DESKTOP_APP
    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
        #define _CRT_BUILD_DESKTOP_APP 1
    #else
        #define _CRT_BUILD_DESKTOP_APP 0
    #endif
#endif

// Verify that the ARM Desktop SDK is available when building an ARM Desktop app
#ifdef _M_ARM
    #if _CRT_BUILD_DESKTOP_APP && !_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
        #error Compiling Desktop applications for the ARM platform is not supported.
    #endif
#endif

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Warning Suppression
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// C4412: function signature contains type '_locale_t';
//        C++ objects are unsafe to pass between pure code and mixed or native. (/Wall)
#ifndef _UCRT_DISABLED_WARNING_4412
    #ifdef _M_CEE_PURE
        #define _UCRT_DISABLED_WARNING_4412 4412
    #else
        #define _UCRT_DISABLED_WARNING_4412
    #endif
#endif

// Use _UCRT_EXTRA_DISABLED_WARNINGS to add additional warning suppressions to UCRT headers.
#ifndef _UCRT_EXTRA_DISABLED_WARNINGS
    #define _UCRT_EXTRA_DISABLED_WARNINGS
#endif

// C4324: structure was padded due to __declspec(align()) (/W4)
// C4514: unreferenced inline function has been removed (/Wall)
// C4574: 'MACRO' is defined to be '0': did you mean to use '#if MACRO'? (/Wall)
// C4668: '__cplusplus' is not defined as a preprocessor macro (/Wall)
// C4710: function not inlined (/Wall)
// C4793: 'function' is compiled as native code (/Wall and /W1 under /clr:pure)
// C4820: padding after data member (/Wall)
// C4995: name was marked #pragma deprecated
// C4996: __declspec(deprecated)
// C28719: Banned API, use a more robust and secure replacement.
// C28726: Banned or deprecated API, use a more robust and secure replacement.
// C28727: Banned API.
#ifndef _UCRT_DISABLED_WARNINGS
    #define _UCRT_DISABLED_WARNINGS 4324 _UCRT_DISABLED_WARNING_4412 4514 4574 4710 4793 4820 4995 4996 28719 28726 28727 _UCRT_EXTRA_DISABLED_WARNINGS
#endif

#ifndef _UCRT_DISABLE_CLANG_WARNINGS
    #ifdef __clang__
    // warning: declspec(deprecated) [-Wdeprecated-declarations]
    // warning: __declspec attribute 'allocator' is not supported [-Wignored-attributes]
    // warning: '#pragma optimize' is not supported [-Wignored-pragma-optimize]
    // warning: unknown pragma ignored [-Wunknown-pragmas]
        #define _UCRT_DISABLE_CLANG_WARNINGS                                  \
            _Pragma("clang diagnostic push")                                  \
            _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"") \
            _Pragma("clang diagnostic ignored \"-Wignored-attributes\"")      \
            _Pragma("clang diagnostic ignored \"-Wignored-pragma-optimize\"") \
            _Pragma("clang diagnostic ignored \"-Wunknown-pragmas\"")
    #else // __clang__
        #define _UCRT_DISABLE_CLANG_WARNINGS
    #endif // __clang__
#endif // _UCRT_DISABLE_CLANG_WARNINGS

#ifndef _UCRT_RESTORE_CLANG_WARNINGS
    #ifdef __clang__
        #define _UCRT_RESTORE_CLANG_WARNINGS _Pragma("clang diagnostic pop")
    #else // __clang__
        #define _UCRT_RESTORE_CLANG_WARNINGS
    #endif // __clang__
#endif // _UCRT_RESTORE_CLANG_WARNINGS

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Annotation Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _ACRTIMP
    #if defined _CRTIMP && !defined _VCRT_DEFINED_CRTIMP
        #define _ACRTIMP _CRTIMP
    #elif !defined _CORECRT_BUILD && defined _DLL
        #define _ACRTIMP __declspec(dllimport)
    #else
        #define _ACRTIMP
    #endif
#endif

// If you need the ability to remove __declspec(import) from an API, to support static replacement,
// declare the API using _ACRTIMP_ALT instead of _ACRTIMP.
#ifndef _ACRTIMP_ALT
    #define _ACRTIMP_ALT _ACRTIMP
#endif

#ifndef _DCRTIMP
    #if defined _CRTIMP && !defined _VCRT_DEFINED_CRTIMP
        #define _DCRTIMP _CRTIMP
    #elif !defined _CORECRT_BUILD && defined _DLL
        #define _DCRTIMP __declspec(dllimport)
    #else
        #define _DCRTIMP
    #endif
#endif

#if defined _CRT_SUPPRESS_RESTRICT || defined _CORECRT_BUILD
    #define _CRTRESTRICT
#else
    #define _CRTRESTRICT __declspec(restrict)
#endif

#if defined _MSC_VER && _MSC_VER >= 1900 && !defined _CORECRT_BUILD
    #define _CRTALLOCATOR __declspec(allocator)
#else
    #define _CRTALLOCATOR
#endif

#if defined _M_CEE && defined _M_X64
    // This is only needed when managed code is calling the native APIs,
    // targeting the 64-bit runtime.
    #define _CRT_JIT_INTRINSIC __declspec(jitintrinsic)
#else
    #define _CRT_JIT_INTRINSIC
#endif

// __declspec(guard(overflow)) enabled by /sdl compiler switch for CRT allocators
#ifdef _GUARDOVERFLOW_CRT_ALLOCATORS
    #define _CRT_GUARDOVERFLOW __declspec(guard(overflow))
#else
    #define _CRT_GUARDOVERFLOW
#endif

#if defined _DLL && (defined _M_HYBRID || defined _M_ARM64EC) && (defined _CORECRT_BUILD || defined _VCRT_BUILD)
    #define _CRT_HYBRIDPATCHABLE __declspec(hybrid_patchable)
#else
    #define _CRT_HYBRIDPATCHABLE
#endif

// The CLR requires code calling other SecurityCritical code or using SecurityCritical types
// to be marked as SecurityCritical.
// _CRT_SECURITYCRITICAL_ATTRIBUTE covers this for internal function definitions.
// _CRT_INLINE_PURE_SECURITYCRITICAL_ATTRIBUTE is for inline pure functions defined in the header.
// This is clr:pure-only because for mixed mode we compile inline functions as native.
#ifdef _M_CEE_PURE
    #define _CRT_INLINE_PURE_SECURITYCRITICAL_ATTRIBUTE [System::Security::SecurityCritical]
#else
    #define _CRT_INLINE_PURE_SECURITYCRITICAL_ATTRIBUTE
#endif

#ifndef _CONST_RETURN
    #ifdef __cplusplus
        #define _CONST_RETURN const
        #define _CRT_CONST_CORRECT_OVERLOADS
    #else
        #define _CONST_RETURN
    #endif
#endif

#define _WConst_return _CONST_RETURN // For backwards compatibility

#ifndef _CRT_ALIGN
    #ifdef __midl
        #define _CRT_ALIGN(x)
    #else
        #define _CRT_ALIGN(x) __declspec(align(x))
    #endif
#endif

#if defined _PREFAST_ && defined _CA_SHOULD_CHECK_RETURN
    #define _Check_return_opt_ _Check_return_
#else
    #define _Check_return_opt_
#endif

#if defined _PREFAST_ && defined _CA_SHOULD_CHECK_RETURN_WER
    #define _Check_return_wat_ _Check_return_
#else
    #define _Check_return_wat_
#endif

#if !defined __midl && !defined MIDL_PASS && defined _PREFAST_
    #define __crt_typefix(ctype) __declspec("SAL_typefix(" _CRT_STRINGIZE(ctype) ")")
#else
    #define __crt_typefix(ctype)
#endif

#ifndef _CRT_NOEXCEPT
    #ifdef __cplusplus
        #define _CRT_NOEXCEPT noexcept
    #else
        #define _CRT_NOEXCEPT
    #endif
#endif


//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Miscellaneous Stuff
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef __cplusplus
extern "C++"
{
    template<bool _Enable, typename _Ty>
    struct _CrtEnableIf;

    template<typename _Ty>
    struct _CrtEnableIf<true, _Ty>
    {
        typedef _Ty _Type;
    };
}
#endif

#if defined __cplusplus
    typedef bool  __crt_bool;
#elif defined __midl
    // MIDL understands neither bool nor _Bool.  Use char as a best-fit
    // replacement (the differences won't matter in practice).
    typedef char __crt_bool;
#else
    typedef _Bool __crt_bool;
#endif

#define _ARGMAX   100
#define _TRUNCATE ((size_t)-1)
#define _CRT_INT_MAX 2147483647
#define _CRT_SIZE_MAX ((size_t)-1)

#define __FILEW__     _CRT_WIDE(__FILE__)
#define __FUNCTIONW__ _CRT_WIDE(__FUNCTION__)

#ifdef __cplusplus
    #ifndef _STATIC_ASSERT
        #define _STATIC_ASSERT(expr) static_assert((expr), #expr)
    #endif
#else
    #ifndef _STATIC_ASSERT
        #ifdef __clang__
            #define _STATIC_ASSERT(expr) _Static_assert((expr), #expr)
        #else
            #define _STATIC_ASSERT(expr) typedef char __static_assert_t[(expr) != 0]
        #endif
    #endif
#endif

#ifndef NULL
    #ifdef __cplusplus
        #define NULL 0
    #else
        #define NULL ((void *)0)
    #endif
#endif

// CRT headers are included into some kinds of source files where only data type
// definitions and macro definitions are required but function declarations and
// inline function definitions are not.  These files include assembly files, IDL
// files, and resource files.  The tools that process these files often have a
// limited ability to process C and C++ code.  The _CRT_FUNCTIONS_REQUIRED macro
// is defined to 1 when we are compiling a file that actually needs functions to
// be declared (and defined, where applicable), and to 0 when we are compiling a
// file that does not.  This allows us to suppress declarations and definitions
// that are not compilable with the aforementioned tools.
#if !defined _CRT_FUNCTIONS_REQUIRED
    #if defined __assembler || defined __midl || defined RC_INVOKED
        #define _CRT_FUNCTIONS_REQUIRED 0
    #else
        #define _CRT_FUNCTIONS_REQUIRED 1
    #endif
#endif

#if !defined _NO_INLINING && !_CRT_FUNCTIONS_REQUIRED
        #define _NO_INLINING // Suppress <tchar.h> inlines
#endif

#ifndef _CRT_UNUSED
    #define _CRT_UNUSED(x) (void)x
#endif

#ifndef _CRT_HAS_CXX17
 #ifdef _MSVC_LANG
  #if _MSVC_LANG > 201402
   #define _CRT_HAS_CXX17   1
  #else /* _MSVC_LANG > 201402 */
   #define _CRT_HAS_CXX17   0
  #endif /* _MSVC_LANG > 201402 */
 #else /* _MSVC_LANG */
  #if defined __cplusplus && __cplusplus > 201402
   #define _CRT_HAS_CXX17   1
  #else /* __cplusplus > 201402 */
   #define _CRT_HAS_CXX17   0
  #endif /* __cplusplus > 201402 */
 #endif /* _MSVC_LANG */
#endif /* _CRT_HAS_CXX17 */

#ifndef _CRT_HAS_C11
 #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
   #define _CRT_HAS_C11 1
 #else /* defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L */
   #define _CRT_HAS_C11 0
 #endif /* defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L */
#endif /* _CRT_HAS_C11 */

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Invalid Parameter Handler
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef _DEBUG
    _ACRTIMP void __cdecl _invalid_parameter(
        _In_opt_z_ wchar_t const*,
        _In_opt_z_ wchar_t const*,
        _In_opt_z_ wchar_t const*,
        _In_       unsigned int,
        _In_       uintptr_t
        );
#endif

_ACRTIMP_ALT void __cdecl _invalid_parameter_noinfo(void);
_ACRTIMP __declspec(noreturn) void __cdecl _invalid_parameter_noinfo_noreturn(void);

__declspec(noreturn)
_ACRTIMP void __cdecl _invoke_watson(
    _In_opt_z_ wchar_t const* _Expression,
    _In_opt_z_ wchar_t const* _FunctionName,
    _In_opt_z_ wchar_t const* _FileName,
    _In_       unsigned int _LineNo,
    _In_       uintptr_t _Reserved);

#ifndef _CRT_SECURE_INVALID_PARAMETER
    #ifdef _DEBUG
        #define _CRT_SECURE_INVALID_PARAMETER(expr) \
            ::_invalid_parameter(_CRT_WIDE(#expr), __FUNCTIONW__, __FILEW__, __LINE__, 0)
    #else
        // By default, _CRT_SECURE_INVALID_PARAMETER in retail invokes
        // _invalid_parameter_noinfo_noreturn(), which is marked
        // __declspec(noreturn) and does not return control to the application.
        // Even if _set_invalid_parameter_handler() is used to set a new invalid
        // parameter handler which does return control to the application,
        // _invalid_parameter_noinfo_noreturn() will terminate the application
        // and invoke Watson. You can overwrite the definition of
        // _CRT_SECURE_INVALID_PARAMETER if you need.
        //
        // _CRT_SECURE_INVALID_PARAMETER is used in the Standard C++ Libraries
        // and the SafeInt library.
        #define _CRT_SECURE_INVALID_PARAMETER(expr) \
            ::_invalid_parameter_noinfo_noreturn()
    #endif
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Deprecation and Warnings
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define _CRT_WARNING_MESSAGE(NUMBER, MESSAGE) \
    __FILE__ "(" _CRT_STRINGIZE(__LINE__) "): warning " NUMBER ": " MESSAGE

#if ( defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || \
    (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__                 )
    #define _CRT_INTERNAL_NONSTDC_NAMES 1
#else
    #define _CRT_INTERNAL_NONSTDC_NAMES 0
#endif

#if defined _CRT_NONSTDC_NO_DEPRECATE && !defined _CRT_NONSTDC_NO_WARNINGS
    #define _CRT_NONSTDC_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_DEPRECATE
    #ifdef _CRT_NONSTDC_NO_WARNINGS
        #define _CRT_NONSTDC_DEPRECATE(_NewName)
    #else
        #define _CRT_NONSTDC_DEPRECATE(_NewName) _CRT_DEPRECATE_TEXT(             \
            "The POSIX name for this item is deprecated. Instead, use the ISO C " \
            "and C++ conformant name: " #_NewName ". See online help for details.")
    #endif
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Managed CRT Support
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _PGLOBAL
    #ifdef _M_CEE
        #ifdef __cplusplus_cli
            #define _PGLOBAL __declspec(process)
        #else
            #define _PGLOBAL
        #endif
    #else
        #define _PGLOBAL
    #endif
#endif

#ifndef _AGLOBAL
    #ifdef _M_CEE
        #define _AGLOBAL __declspec(appdomain)
    #else
        #define _AGLOBAL
    #endif
#endif

#if defined _M_CEE && !defined _M_CEE_PURE
    #define _M_CEE_MIXED
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// SecureCRT Configuration
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#if defined _CRTBLD || defined _CORECRT_BUILD || defined _VCRT_BUILD
    // Disable C++ overloads internally:
    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES       0
    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 0
    #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES         0
#endif

#if !_CRT_FUNCTIONS_REQUIRED
    // If we don't require function declarations at all, we need not define the
    // overloads (MIDL and RC do not need the C++ overloads).
    #undef  _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
    #undef  _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
    #undef  _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES
    #undef  _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY
    #undef  _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY

    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 0
    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 0
    #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
    #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY 0
    #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY 0
#endif

#define __STDC_SECURE_LIB__ 200411L
#define __GOT_SECURE_LIB__ __STDC_SECURE_LIB__ // For backwards compatibility

#ifndef __STDC_WANT_SECURE_LIB__
    #define __STDC_WANT_SECURE_LIB__ 1
#endif

#if !__STDC_WANT_SECURE_LIB__ && !defined _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef RC_INVOKED
    #if defined _CRT_SECURE_NO_DEPRECATE_GLOBALS && !defined _CRT_SECURE_NO_WARNINGS_GLOBALS
        #define _CRT_SECURE_NO_WARNINGS_GLOBALS
    #endif
#endif

#ifndef _CRT_INSECURE_DEPRECATE_GLOBALS
    #ifdef RC_INVOKED
        #define _CRT_INSECURE_DEPRECATE_GLOBALS(replacement)
    #else
        #ifdef _CRT_SECURE_NO_WARNINGS_GLOBALS
            #define _CRT_INSECURE_DEPRECATE_GLOBALS(replacement)
        #else
            #define _CRT_INSECURE_DEPRECATE_GLOBALS(replacement) _CRT_INSECURE_DEPRECATE(replacement)
        #endif
    #endif
#endif

#if defined _CRT_MANAGED_HEAP_NO_DEPRECATE && !defined _CRT_MANAGED_HEAP_NO_WARNINGS
    #define _CRT_MANAGED_HEAP_NO_WARNINGS
#endif

#define _SECURECRT_FILL_BUFFER_PATTERN 0xFE

#if defined _CRT_OBSOLETE_NO_DEPRECATE && !defined _CRT_OBSOLETE_NO_WARNINGS
    #define _CRT_OBSOLETE_NO_WARNINGS
#endif

#ifndef _CRT_OBSOLETE
    #ifdef _CRT_OBSOLETE_NO_WARNINGS
        #define _CRT_OBSOLETE(_NewItem)
    #else
        #define _CRT_OBSOLETE(_NewItem) _CRT_DEPRECATE_TEXT(                   \
            "This function or variable has been superceded by newer library "  \
            "or operating system functionality. Consider using " #_NewItem " " \
            "instead. See online help for details.")
    #endif
#endif

#ifndef RC_INVOKED
    #ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
        #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 0
    #else
        #if !__STDC_WANT_SECURE_LIB__ && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
            #error Cannot use Secure CRT C++ overloads when __STDC_WANT_SECURE_LIB__ is 0
        #endif
    #endif

    #ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
        // _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT is ignored if
        // _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES is set to 0
        #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 0
    #else
        #if !__STDC_WANT_SECURE_LIB__ && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT
            #error Cannot use Secure CRT C++ overloads when __STDC_WANT_SECURE_LIB__ is 0
        #endif
    #endif

    #ifndef _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES
        #if __STDC_WANT_SECURE_LIB__
              #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
        #else
              #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
        #endif
    #else
        #if !__STDC_WANT_SECURE_LIB__ && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES
            #error Cannot use Secure CRT C++ overloads when __STDC_WANT_SECURE_LIB__ is 0
        #endif
    #endif

    #ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY
        #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY 0
    #else
        #if !__STDC_WANT_SECURE_LIB__ && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY
            #error Cannot use Secure CRT C++ overloads when __STDC_WANT_SECURE_LIB__ is 0
        #endif
    #endif

    #ifndef _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY
        #define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY 0
    #else
        #if !__STDC_WANT_SECURE_LIB__ && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY
           #error Cannot use Secure CRT C++ overloads when __STDC_WANT_SECURE_LIB__ is 0
        #endif
    #endif
#endif

#ifndef _CRT_SECURE_CPP_NOTHROW
    #define _CRT_SECURE_CPP_NOTHROW throw()
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Basic Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef int                           errno_t;
typedef unsigned short                wint_t;
typedef unsigned short                wctype_t;
typedef long                          __time32_t;
typedef __int64                       __time64_t;

typedef struct __crt_locale_data_public
{
      unsigned short const* _locale_pctype;
    _Field_range_(1, 2) int _locale_mb_cur_max;
               unsigned int _locale_lc_codepage;
} __crt_locale_data_public;

typedef struct __crt_locale_pointers
{
    struct __crt_locale_data*    locinfo;
    struct __crt_multibyte_data* mbcinfo;
} __crt_locale_pointers;

typedef __crt_locale_pointers* _locale_t;

typedef struct _Mbstatet
{ // state of a multibyte translation
    unsigned long _Wchar;
    unsigned short _Byte, _State;
} _Mbstatet;

typedef _Mbstatet mbstate_t;

#if defined _USE_32BIT_TIME_T && defined _WIN64
    #error You cannot use 32-bit time_t (_USE_32BIT_TIME_T) with _WIN64
#endif

#if defined _VCRT_BUILD || defined _CORECRT_BUILD
    #define _CRT_NO_TIME_T
#endif

#ifndef _CRT_NO_TIME_T
    #ifdef _USE_32BIT_TIME_T
        typedef __time32_t time_t;
    #else
        typedef __time64_t time_t;
    #endif
#endif

// Indicate that these common types are defined
#ifndef _TIME_T_DEFINED
    #define _TIME_T_DEFINED
#endif

#if __STDC_WANT_SECURE_LIB__
    typedef size_t rsize_t;
#endif




//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// C++ Secure Overload Generation Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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



#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _SalAttributeDst, _DstType, _Dst)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _FuncName##_s, _VFuncName, _VFuncName##_s, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _VFuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE(_DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

#define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE(_DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)



#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _SalAttributeDst, _DstType, _Dst)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _SalAttributeDst, _DstType, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _FuncName##_s, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, __cdecl, _FuncName, _FuncName##_s, _VFuncName, _VFuncName##_s, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE(_DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

#define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE(_DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _FuncName##_s, _DstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// C++ Standard Overload Generation Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef RC_INVOKED
    #if defined __cplusplus && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES

        #define __RETURN_POLICY_SAME(_FunctionCall, _Dst) return (_FunctionCall)
        #define __RETURN_POLICY_DST(_FunctionCall, _Dst)  return ((_FunctionCall) == 0 ? _Dst : 0)
        #define __RETURN_POLICY_VOID(_FunctionCall, _Dst) (_FunctionCall); return
        #define __EMPTY_DECLSPEC

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst) \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst); \
                return _FuncName(_Dst); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(_ReturnType, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst) \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst); \
                return _FuncName(_Dst); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW \
            { \
                size_t _SizeRead = 0; \
                errno_t _Err = _FuncName##_s(_Dst + 2, (_Size - 2) < ((size_t)_Dst[0]) ? (_Size - 2) : ((size_t)_Dst[0]), &_SizeRead); \
                _Dst[1] = (_DstType)(_SizeRead); \
                return (_Err == 0 ? _Dst + 2 : 0); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1]) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName((_DstType *)_Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName<2>(_DstType (&_Dst)[2]) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName((_DstType *)_Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1) \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1); \
                return _FuncName(_Dst, _TArg1); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2); \
                return _FuncName(_Dst, _TArg1, _TArg2); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1, _TArg2), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1, _TArg2), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3); \
                return _FuncName(_Dst, _TArg1, _TArg2, _TArg3); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2, _TArg3); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1, _TArg2, _TArg3), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1, _TArg2, _TArg3), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) \
            { \
                _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4); \
                return _FuncName(_Dst, _TArg1, _TArg2, _TArg3, _TArg4); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3, _TArg4); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3, _TArg4); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2, _TArg3, _TArg4); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1, _TArg2, _TArg3, _TArg4), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1, _TArg2, _TArg3, _TArg4), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_HType1 _HArg1, _SalAttributeDst _DstType *_Dst, _TType1 _TArg1) \
            { \
                _ReturnType __cdecl _FuncName(_HType1 _HArg1, _SalAttributeDst _DstType *_Dst, _TType1 _TArg1); \
                return _FuncName(_HArg1, _Dst, _TArg1); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, const _T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _SalAttributeDst _DstType * &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, _Dst, _TArg1); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _DstType (&_Dst)[_Size], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_HArg1, _Dst, _Size, _TArg1), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_HType1 _HArg1, _DstType (&_Dst)[1], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_HArg1, _Dst, 1, _TArg1), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType *_Dst) \
            { \
                _ReturnType __cdecl _FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType *_Dst); \
                return _FuncName(_HArg1, _HArg2, _Dst); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _HType2 _HArg2, _T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, _HArg2, static_cast<_DstType *>(_Dst)); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _HType2 _HArg2, const _T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, _HArg2, static_cast<_DstType *>(_Dst)); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType * &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_HArg1, _HArg2, _Dst); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_HType1 _HArg1, _HType2 _HArg2, _DstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_HArg1, _HArg2, _Dst, _Size), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_HType1 _HArg1, _HType2 _HArg2, _DstType (&_Dst)[1]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_HArg1, _HArg2, _Dst, 1), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName, _VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, va_list _ArgList) \
            { \
                _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, va_list _ArgList); \
                return _VFuncName(_Dst, _TArg1, _ArgList); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg1); \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _ArgList); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg1); \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _ArgList); \
            } \
                \
                template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg1); \
                return __insecure_##_VFuncName(_Dst, _TArg1, _ArgList); \
            } \
                \
                template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg1); \
                _ReturnPolicy(_SecureVFuncName(_Dst, _Size, _TArg1, _ArgList), _Dst); \
            } \
                \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg1); \
                _ReturnPolicy(_SecureVFuncName(_Dst, 1, _TArg1, _ArgList), _Dst); \
            } \
                \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(_T &_Dst, _TType1 _TArg1, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _ArgList); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(const _T &_Dst, _TType1 _TArg1, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _ArgList); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(_SalAttributeDst _DstType *&_Dst, _TType1 _TArg1, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(_Dst, _TArg1, _ArgList); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _VFuncName(_DstType (&_Dst)[_Size], _TType1 _TArg1, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureVFuncName(_Dst, _Size, _TArg1, _ArgList), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureVFuncName(_Dst, 1, _TArg1, _ArgList), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SecureVFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) \
            { \
                _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList); \
                return _VFuncName(_Dst, _TArg1, _TArg2, _ArgList); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg2); \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _ArgList); \
            } \
            template <typename _T> \
            inline \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg2); \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _ArgList); \
            } \
                \
            template <> \
            inline \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg2); \
                return __insecure_##_VFuncName(_Dst, _TArg1, _TArg2, _ArgList); \
            } \
                \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg2); \
                _ReturnPolicy(_SecureVFuncName(_Dst, _Size, _TArg1, _TArg2, _ArgList), _Dst); \
            } \
                \
            template <> \
            inline \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, ...) _CRT_SECURE_CPP_NOTHROW \
            { \
                va_list _ArgList; \
                __crt_va_start(_ArgList, _TArg2); \
                _ReturnPolicy(_SecureVFuncName(_Dst, 1, _TArg1, _TArg2, _ArgList), _Dst); \
            } \
                \
            template <typename _T> \
            inline \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _ArgList); \
            } \
            template <typename _T> \
            inline \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _ArgList); \
            } \
            template <> \
            inline \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName(_SalAttributeDst _DstType *&_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_VFuncName(_Dst, _TArg1, _TArg2, _ArgList); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _VFuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureVFuncName(_Dst, _Size, _TArg1, _TArg2, _ArgList), _Dst); \
            } \
            template <> \
            inline \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) \
            _ReturnType __CRTDECL _VFuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, va_list _ArgList) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureVFuncName(_Dst, 1, _TArg1, _TArg2, _ArgList), _Dst); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            __inline \
            size_t __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2) \
            { \
                _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2); \
                return _FuncName(_Dst, _TArg1, _TArg2); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2); \
            } \
            template <size_t _Size> \
            inline \
            size_t __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                size_t _Ret = 0; \
                _SecureFuncName(&_Ret, _Dst, _Size, _TArg1, _TArg2); \
                return (_Ret > 0 ? (_Ret - 1) : _Ret); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                size_t _Ret = 0; \
                _SecureFuncName(&_Ret, _Dst, 1, _TArg1, _TArg2); \
                return (_Ret > 0 ? (_Ret - 1) : _Ret); \
            } \
            }

        #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            __inline \
            size_t __CRTDECL __insecure_##_FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) \
            { \
                _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3); \
                return _FuncName(_Dst, _TArg1, _TArg2, _TArg3); \
            } \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2, _TArg3); \
            } \
            template <size_t _Size> \
            inline \
            size_t __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                size_t _Ret = 0; \
                _SecureFuncName(&_Ret, _Dst, _Size, _TArg1, _TArg2, _TArg3); \
                return (_Ret > 0 ? (_Ret - 1) : _Ret); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            size_t __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                size_t _Ret = 0; \
                _SecureFuncName(&_Ret, _Dst, 1, _TArg1, _TArg2, _TArg3); \
                return (_Ret > 0 ? (_Ret - 1) : _Ret); \
            } \
            }

        #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_DstType *_Dst)

        #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst)); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_DstType * &_Dst) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1]) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1), _Dst); \
            } \
            }

        #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_DstType *_Dst, _TType1 _TArg1)

        #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_DstType * &_Dst, _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1), _Dst); \
            } \
            }

        #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2)

        #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1, _TArg2), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1, _TArg2), _Dst); \
            } \
            }

        #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            __inline \
            _ReturnType __CRTDECL __insecure_##_FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3)

        #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
            extern "C++" \
            { \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <typename _T> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(const _T &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(static_cast<_DstType *>(_Dst), _TArg1, _TArg2, _TArg3); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName(_SalAttributeDst _DstType * &_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                return __insecure_##_FuncName(_Dst, _TArg1, _TArg2, _TArg3); \
            } \
            template <size_t _Size> \
            inline \
            _ReturnType __CRTDECL _FuncName(_SecureDstType (&_Dst)[_Size], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, _Size, _TArg1, _TArg2, _TArg3), _Dst); \
            } \
            template <> \
            inline \
            _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
            _ReturnType __CRTDECL _FuncName<1>(_DstType (&_Dst)[1], _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3) _CRT_SECURE_CPP_NOTHROW \
            { \
                _ReturnPolicy(_SecureFuncName(_Dst, 1, _TArg1, _TArg2, _TArg3), _Dst); \
            } \
            }

        #if _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_CGETS(_ReturnType, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(_ReturnType, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName, _VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName, _VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, __cdecl, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _VFuncName##_s, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)


            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
                __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType _DstType, _Dst)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
                __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
                __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType _DstType, _Dst, _TType1, _TArg1)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
                __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

        #else // ^^^ _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT ^^^ // vvv _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT vvv //

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst) \
                    _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_GETS(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _DstType, _Dst) \
                    _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType __cdecl _FuncName(_DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName,_VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, ...); \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, ...); \
                _CRT_INSECURE_DEPRECATE(_VFuncName##_s) _DeclSpec _ReturnType __cdecl _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, ...); \
                _CRT_INSECURE_DEPRECATE(_VFuncName##_s) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);


            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

        #endif // !_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT

    #else  // ^^^ _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES ^^^ // vvv !_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES vvv //

        #define __RETURN_POLICY_SAME(_FunctionCall)
        #define __RETURN_POLICY_DST(_FunctionCall)
        #define __RETURN_POLICY_VOID(_FunctionCall)
        #define __EMPTY_DECLSPEC

        #if _CRT_FUNCTIONS_REQUIRED

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(_ReturnType, _DeclSpec, _FuncName, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName, _VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, ...); \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SecureVFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, ...); \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_GETS(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType __cdecl _FuncName(_DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3, _TType4, _TArg4) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3, _TType4 _TArg4);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _SalAttributeDst _DstType *_Dst, _TType1 _TArg1);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _SecureFuncName, _HType1, _HArg1, _HType2, _HArg2, _SalAttributeDst, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType __cdecl _FuncName(_HType1 _HArg1, _HType2 _HArg2, _SalAttributeDst _DstType *_Dst);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _SecureFuncName, _VFuncName, _SecureVFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, ...); \
                _CRT_INSECURE_DEPRECATE(_SecureVFuncName) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST(_ReturnType, _ReturnPolicy, _DeclSpec, _FuncName, _VFuncName, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, ...); \
                _CRT_INSECURE_DEPRECATE(_VFuncName##_s) _DeclSpec _ReturnType __cdecl _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(_ReturnType, _ReturnPolicy, _DeclSpec, _CC, _FuncName, _VFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_FuncName##_s) _DeclSpec _ReturnType _CC _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, ...); \
                _CRT_INSECURE_DEPRECATE(_VFuncName##_s) _DeclSpec _ReturnType _CC _VFuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, va_list _Args);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2);

            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE_EX(_DeclSpec, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) _DeclSpec size_t __cdecl _FuncName(_SalAttributeDst _DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3);


            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst)

            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst)

            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1)

            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1)

            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2)

            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3)

            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2)

            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3) \
                _CRT_INSECURE_DEPRECATE(_SecureFuncName) \
                __inline \
                _ReturnType __CRTDECL _FuncName(_DstType *_Dst, _TType1 _TArg1, _TType2 _TArg2, _TType3 _TArg3)

            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(_ReturnType, _ReturnPolicy, _FuncName, _SecureFuncName, _SecureDstType, _SalAttributeDst, _DstType, _Dst, _TType1, _TArg1, _TType2, _TArg2, _TType3, _TArg3)

        #else // ^^^ _CRT_FUNCTIONS_REQUIRED ^^^ // vvv !_CRT_FUNCTIONS_REQUIRED vvv //

            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_4_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_1_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_2_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_3_SIZE_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_0_GETS(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_4_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_1_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_2_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_1_ARGLIST_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_SIZE_EX(...)
            #define __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_0_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_1_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_2_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_FUNC_0_3_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_0_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_1_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_2_EX(...)
            #define __DECLARE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(...)
            #define __DEFINE_CPP_OVERLOAD_INLINE_NFUNC_0_3_EX(...)

        #endif // !_CRT_FUNCTIONS_REQUIRED
    #endif // !_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#endif

_CRT_END_C_HEADER

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
