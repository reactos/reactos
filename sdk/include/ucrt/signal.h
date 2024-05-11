//
// stdio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <signal.h> header.
//
#pragma once
#ifndef _INC_SIGNAL // include guard for 3rd party interop
#define _INC_SIGNAL

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



typedef int sig_atomic_t;

typedef void (__CRTDECL* _crt_signal_t)(int);

#define NSIG            23  // maximum signal number + 1

// Signal types
#define SIGINT          2   // interrupt
#define SIGILL          4   // illegal instruction - invalid function image
#define SIGFPE          8   // floating point exception
#define SIGSEGV         11  // segment violation
#define SIGTERM         15  // Software termination signal from kill
#define SIGBREAK        21  // Ctrl-Break sequence
#define SIGABRT         22  // abnormal termination triggered by abort call

#define SIGABRT_COMPAT  6   // SIGABRT compatible with other platforms, same as SIGABRT

// Signal action codes
#define SIG_DFL ((_crt_signal_t)0)     // default signal action
#define SIG_IGN ((_crt_signal_t)1)     // ignore signal
#define SIG_GET ((_crt_signal_t)2)     // return current value
#define SIG_SGE ((_crt_signal_t)3)     // signal gets error
#define SIG_ACK ((_crt_signal_t)4)     // acknowledge

#ifdef _CORECRT_BUILD
    // Internal use only!  Not valid as an argument to signal().
    #define SIG_DIE ((_crt_signal_t)5) // terminate process
#endif

// Signal error value (returned by signal call on error)
#define SIG_ERR ((_crt_signal_t)-1)    // signal error value



// Pointer to exception information pointers structure
_ACRTIMP void** __cdecl __pxcptinfoptrs(void);
#define _pxcptinfoptrs (*__pxcptinfoptrs())

// Function prototypes
#ifndef _M_CEE_PURE
    _ACRTIMP _crt_signal_t __cdecl signal(_In_ int _Signal, _In_opt_ _crt_signal_t _Function);
#endif

_ACRTIMP int __cdecl raise(_In_ int _Signal);



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_SIGNAL
