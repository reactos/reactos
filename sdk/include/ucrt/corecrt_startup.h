//
// corecrt_startup.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Declarations for the CoreCRT startup functionality, used while initializing
// the CRT and during app startup and termination.
//
#pragma once

#include <corecrt.h>
#include <math.h>
#include <vcruntime_startup.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Exception Filters for main() and DllMain()
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
struct _EXCEPTION_POINTERS;

_ACRTIMP int __cdecl _seh_filter_dll(
    _In_ unsigned long               _ExceptionNum,
    _In_ struct _EXCEPTION_POINTERS* _ExceptionPtr
    );

_ACRTIMP int __cdecl _seh_filter_exe(
    _In_ unsigned long               _ExceptionNum,
    _In_ struct _EXCEPTION_POINTERS* _ExceptionPtr
    );



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Miscellaneous Runtime Support
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef enum _crt_app_type
{
    _crt_unknown_app,
    _crt_console_app,
    _crt_gui_app
} _crt_app_type;

_ACRTIMP _crt_app_type __cdecl _query_app_type(void);

_ACRTIMP void __cdecl _set_app_type(
    _In_ _crt_app_type _Type
    );

typedef int (__cdecl *_UserMathErrorFunctionPointer)(struct _exception *);

_ACRTIMP void __cdecl __setusermatherr(
    _UserMathErrorFunctionPointer _UserMathErrorFunction
    );

int __cdecl _is_c_termination_complete(void);



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Arguments API for main() et al.
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_ACRTIMP errno_t __cdecl _configure_narrow_argv(
    _In_ _crt_argv_mode mode
    );

_ACRTIMP errno_t __cdecl _configure_wide_argv(
    _In_ _crt_argv_mode mode
    );

// There is a linkopt for these to disable environment initialization when using
// the static CRT, so they are not declared _ACRTIMP.
int __CRTDECL _initialize_narrow_environment(void);
int __CRTDECL _initialize_wide_environment(void);

_ACRTIMP char**    __cdecl _get_initial_narrow_environment(void);
_ACRTIMP wchar_t** __cdecl _get_initial_wide_environment(void);

char*    __CRTDECL _get_narrow_winmain_command_line(void);
wchar_t* __CRTDECL _get_wide_winmain_command_line(void);

_ACRTIMP char**    __cdecl __p__acmdln(void);
_ACRTIMP wchar_t** __cdecl __p__wcmdln(void);

#ifdef _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    extern char*    _acmdln;
    extern wchar_t* _wcmdln;
#else
    #define _acmdln (*__p__acmdln())
    #define _wcmdln (*__p__wcmdln())
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Initializer and Terminator Support
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef void (__cdecl* _PVFV)(void);
typedef int  (__cdecl* _PIFV)(void);
typedef void (__cdecl* _PVFI)(int);

#ifndef _M_CEE
    _ACRTIMP void __cdecl _initterm(
        _In_reads_(_Last - _First) _In_ _PVFV*  _First,
        _In_                            _PVFV*  _Last
        );

    _ACRTIMP int  __cdecl _initterm_e(
        _In_reads_(_Last - _First)      _PIFV*  _First,
        _In_                            _PIFV*  _Last
        );
#endif

#ifndef _CRT_ONEXIT_T_DEFINED
    #define _CRT_ONEXIT_T_DEFINED

    typedef int (__CRTDECL* _onexit_t)(void);
    #ifdef _M_CEE
        typedef int (__clrcall* _onexit_m_t)(void);
    #endif
#endif

typedef struct _onexit_table_t
{
    _PVFV* _first;
    _PVFV* _last;
    _PVFV* _end;
} _onexit_table_t;

_ACRTIMP int __cdecl _initialize_onexit_table(
    _In_opt_ _onexit_table_t* _Table
    );

_ACRTIMP int __cdecl _register_onexit_function(
    _In_opt_ _onexit_table_t* _Table,
    _In_opt_ _onexit_t        _Function
    );

_ACRTIMP int __cdecl _execute_onexit_table(
    _In_opt_ _onexit_table_t* _Table
    );

_ACRTIMP int __cdecl _crt_atexit(
    _In_opt_ _PVFV _Function
    );

_ACRTIMP int __cdecl _crt_at_quick_exit(
    _In_opt_ _PVFV _Function
    );



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Static CRT Initialization Support
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#if _CRT_FUNCTIONS_REQUIRED

    _Success_(return != 0)
    __crt_bool __cdecl __acrt_initialize(void);

    _Success_(return != 0)
    __crt_bool __cdecl __acrt_uninitialize(
        _In_ __crt_bool _Terminating
        );

    _Success_(return != 0)
    __crt_bool __cdecl __acrt_uninitialize_critical(
        _In_ __crt_bool _Terminating
        );

    _Success_(return != 0)
    __crt_bool __cdecl __acrt_thread_attach(void);

    _Success_(return != 0)
    __crt_bool __cdecl __acrt_thread_detach(void);

#endif // _CRT_FUNCTIONS_REQUIRED



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
