//
// new.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C++ memory allocation library functionality
//
#pragma once
#ifndef _INC_NEW // include guard for 3rd party interop
#define _INC_NEW

#include <corecrt.h>
#include <vcruntime_new_debug.h>

#ifdef __cplusplus

    #if !defined _MSC_EXTENSIONS && !defined _CRTBLD && !defined _CORECRT_BUILD
        #include <new>
    #endif

    #if defined _MSC_EXTENSIONS && !defined _CORECRT_BUILD
        #include <crtdefs.h>

        namespace std
        {
            typedef void (__CRTDECL* new_handler)();

            #ifdef _M_CEE
                typedef void (__clrcall* _new_handler_m) ();
            #endif

            _CRTIMP2 new_handler __cdecl set_new_handler(_In_opt_ new_handler _NewHandler) throw();
        }

        #ifdef _M_CEE
            using ::std::_new_handler_m;
        #endif

        using ::std::new_handler;
        using ::std::set_new_handler;
    #endif

#endif // __cplusplus

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



typedef int (__CRTDECL* _PNH)(size_t);

_PNH __cdecl _query_new_handler(void);
_PNH __cdecl _set_new_handler(_In_opt_ _PNH _NewHandler);

// new mode flag -- when set, makes malloc() behave like new()
_ACRTIMP int __cdecl _query_new_mode(void);
_ACRTIMP int __cdecl _set_new_mode(_In_ int _NewMode);



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS

#endif // _INC_NEW
