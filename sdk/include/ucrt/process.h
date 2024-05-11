//
// process.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the process control functionality (e.g., the exec and
// spawn families of functions).
//
#pragma once
#ifndef _INC_PROCESS // include guard for 3rd party interop
#define _INC_PROCESS

#include <corecrt.h>
#include <corecrt_startup.h>
#include <corecrt_wprocess.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



// Flag values for the _spawn family of functions
#define _P_WAIT         0
#define _P_NOWAIT       1
#define _OLD_P_OVERLAY  2
#define _P_NOWAITO      3
#define _P_DETACH       4
#define _P_OVERLAY      2

// Action codes for _cwait(). The action code argument to _cwait() is ignored on
// Win32.  The parameter only exists so that we do not break existing code.
#define _WAIT_CHILD      0
#define _WAIT_GRANDCHILD 1



#if _CRT_FUNCTIONS_REQUIRED

    _ACRTIMP __declspec(noreturn) void __cdecl exit(_In_ int _Code);
    _ACRTIMP __declspec(noreturn) void __cdecl _exit(_In_ int _Code);
    _ACRTIMP __declspec(noreturn) void __cdecl _Exit(_In_ int _Code);
    _ACRTIMP __declspec(noreturn) void __cdecl quick_exit(_In_ int _Code);
    _ACRTIMP __declspec(noreturn) void __cdecl abort(void);

    _DCRTIMP int __cdecl system(_In_opt_z_ char const* _Command);

    _ACRTIMP void __cdecl _cexit(void);
    _ACRTIMP void __cdecl _c_exit(void);

    typedef void (__stdcall *_tls_callback_type)(void *, unsigned long, void *);
    _ACRTIMP void __cdecl _register_thread_local_exe_atexit_callback(_In_ _tls_callback_type _Callback);

#endif // _CRT_FUNCTIONS_REQUIRED

// Declare DLL notification (initialization/termination) routines.  The preferred
// method is for the CRT client to define DllMain(), which will automatically be
// called by the DLL entry point defined by the CRT.  If the CRT client wants to
// define the DLL entry point, the client entry point must call _CRT_INIT on all
// types of notifications, as the very first thing on attach notifications and as
// the very last thing on detach notifications.
#ifdef _DECL_DLLMAIN

    int __stdcall DllMain(
        _In_     void*         _DllHandle,
        _In_     unsigned long _Reason,
        _In_opt_ void*         _Reserved
        );

    int __stdcall _CRT_INIT(
        _In_     void*         _DllHandle,
        _In_     unsigned long _Reason,
        _In_opt_ void*         _Reserved
        );

    extern int (__stdcall* const _pRawDllMain)(void*, unsigned long, void*);

#endif



typedef void     (__cdecl*   _beginthread_proc_type  )(void*);
typedef unsigned (__stdcall* _beginthreadex_proc_type)(void*);

_ACRTIMP uintptr_t __cdecl _beginthread(
    _In_     _beginthread_proc_type _StartAddress,
    _In_     unsigned               _StackSize,
    _In_opt_ void*                  _ArgList
    );

_ACRTIMP void __cdecl _endthread(void);

_Success_(return != 0)
_ACRTIMP uintptr_t __cdecl _beginthreadex(
    _In_opt_  void*                    _Security,
    _In_      unsigned                 _StackSize,
    _In_      _beginthreadex_proc_type _StartAddress,
    _In_opt_  void*                    _ArgList,
    _In_      unsigned                 _InitFlag,
    _Out_opt_ unsigned*                _ThrdAddr
    );

_ACRTIMP void __cdecl _endthreadex(
    _In_ unsigned _ReturnCode
    );



#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

    _ACRTIMP int __cdecl _getpid(void);

    _DCRTIMP intptr_t __cdecl _cwait(
        _Out_opt_ int*     _TermStat,
        _In_      intptr_t _ProcHandle,
        _In_      int      _Action
        );

    _DCRTIMP intptr_t __cdecl _execl(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _execle(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _execlp(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _execlpe(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _execv(
        _In_z_ char const*        _FileName,
        _In_z_ char const* const* _Arguments
        );

    _DCRTIMP intptr_t __cdecl _execve(
        _In_z_     char const*        _FileName,
        _In_z_     char const* const* _Arguments,
        _In_opt_z_ char const* const* _Environment
        );

    _DCRTIMP intptr_t __cdecl _execvp(
        _In_z_ char const*        _FileName,
        _In_z_ char const* const* _Arguments
        );

    _DCRTIMP intptr_t __cdecl _execvpe(
        _In_z_     char const*        _FileName,
        _In_z_     char const* const* _Arguments,
        _In_opt_z_ char const* const* _Environment
        );

    _DCRTIMP intptr_t __cdecl _spawnl(
        _In_   int         _Mode,
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _spawnle(
        _In_   int         _Mode,
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _spawnlp(
        _In_   int         _Mode,
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _spawnlpe(
        _In_   int         _Mode,
        _In_z_ char const* _FileName,
        _In_z_ char const* _Arguments,
        ...);

    _DCRTIMP intptr_t __cdecl _spawnv(
        _In_   int                _Mode,
        _In_z_ char const*        _FileName,
        _In_z_ char const* const* _Arguments
        );

    _DCRTIMP intptr_t __cdecl _spawnve(
        _In_       int                _Mode,
        _In_z_     char const*        _FileName,
        _In_z_     char const* const* _Arguments,
        _In_opt_z_ char const* const* _Environment
        );

    _DCRTIMP intptr_t __cdecl _spawnvp(
        _In_   int                _Mode,
        _In_z_ char const*        _FileName,
        _In_z_ char const* const* _Arguments
        );

    _DCRTIMP intptr_t __cdecl _spawnvpe(
        _In_       int                _Mode,
        _In_z_     char const*        _FileName,
        _In_z_     char const* const* _Arguments,
        _In_opt_z_ char const* const* _Environment
        );

    _CRT_OBSOLETE(LoadLibrary)
    _DCRTIMP intptr_t __cdecl _loaddll(
        _In_z_ char* _FileName
        );

    _CRT_OBSOLETE(FreeLibrary)
    _DCRTIMP int __cdecl _unloaddll(
        _In_ intptr_t _Handle
        );

    typedef int (__cdecl* _GetDllProcAddrProcType)(void);

    _CRT_OBSOLETE(GetProcAddress)
    _DCRTIMP _GetDllProcAddrProcType __cdecl _getdllprocaddr(
        _In_       intptr_t _Handle,
        _In_opt_z_ char*    _ProcedureName,
        _In_       intptr_t _Ordinal
        );

#endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    #define P_WAIT          _P_WAIT
    #define P_NOWAIT        _P_NOWAIT
    #define P_OVERLAY       _P_OVERLAY
    #define OLD_P_OVERLAY   _OLD_P_OVERLAY
    #define P_NOWAITO       _P_NOWAITO
    #define P_DETACH        _P_DETACH
    #define WAIT_CHILD      _WAIT_CHILD
    #define WAIT_GRANDCHILD _WAIT_GRANDCHILD

    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

        _CRT_NONSTDC_DEPRECATE(_cwait)
        _DCRTIMP intptr_t __cdecl cwait(
            _Out_opt_ int*     _TermStat,
            _In_      intptr_t _ProcHandle,
            _In_      int      _Action
            );

        _CRT_NONSTDC_DEPRECATE(_execl)
        _DCRTIMP intptr_t __cdecl execl(
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_execle)
        _DCRTIMP intptr_t __cdecl execle(
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_execlp)
        _DCRTIMP intptr_t __cdecl execlp(
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_execlpe)
        _DCRTIMP intptr_t __cdecl execlpe(
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_execv)
        _DCRTIMP intptr_t __cdecl execv(
            _In_z_ char const*        _FileName,
            _In_z_ char const* const* _Arguments
            );

        _CRT_NONSTDC_DEPRECATE(_execve)
        _DCRTIMP intptr_t __cdecl execve(
            _In_z_     char const*        _FileName,
            _In_z_     char const* const* _Arguments,
            _In_opt_z_ char const* const* _Environment
            );

        _CRT_NONSTDC_DEPRECATE(_execvp)
        _DCRTIMP intptr_t __cdecl execvp(
            _In_z_ char const*        _FileName,
            _In_z_ char const* const* _Arguments
            );

        _CRT_NONSTDC_DEPRECATE(_execvpe)
        _DCRTIMP intptr_t __cdecl execvpe(
            _In_z_     char const*        _FileName,
            _In_z_     char const* const* _Arguments,
            _In_opt_z_ char const* const* _Environment
            );

        _CRT_NONSTDC_DEPRECATE(_spawnl)
        _DCRTIMP intptr_t __cdecl spawnl(
            _In_   int         _Mode,
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_spawnle)
        _DCRTIMP intptr_t __cdecl spawnle(
            _In_   int         _Mode,
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_spawnlp)
        _DCRTIMP intptr_t __cdecl spawnlp(
            _In_   int         _Mode,
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_spawnlpe)
        _DCRTIMP intptr_t __cdecl spawnlpe(
            _In_   int         _Mode,
            _In_z_ char const* _FileName,
            _In_z_ char const* _Arguments,
            ...);

        _CRT_NONSTDC_DEPRECATE(_spawnv)
        _DCRTIMP intptr_t __cdecl spawnv(
            _In_   int                _Mode,
            _In_z_ char const*        _FileName,
            _In_z_ char const* const* _Arguments);

        _CRT_NONSTDC_DEPRECATE(_spawnve)
        _DCRTIMP intptr_t __cdecl spawnve(
            _In_       int                _Mode,
            _In_z_     char const*        _FileName,
            _In_z_     char const* const* _Arguments,
            _In_opt_z_ char const* const* _Environment
            );

        _CRT_NONSTDC_DEPRECATE(_spawnvp)
        _DCRTIMP intptr_t __cdecl spawnvp(
            _In_   int                _Mode,
            _In_z_ char const*        _FileName,
            _In_z_ char const* const* _Arguments
            );

        _CRT_NONSTDC_DEPRECATE(_spawnvpe)
        _DCRTIMP intptr_t __cdecl spawnvpe(
            _In_       int                _Mode,
            _In_z_     char const*        _FileName,
            _In_z_     char const* const* _Arguments,
            _In_opt_z_ char const* const* _Environment
            );

        _CRT_NONSTDC_DEPRECATE(_getpid)
        _ACRTIMP int __cdecl getpid(void);

    #endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#endif // _CRT_INTERNAL_NONSTDC_NAMES



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_PROCESS
