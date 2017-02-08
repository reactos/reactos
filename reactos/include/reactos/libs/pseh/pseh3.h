/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * PURPOSE:         Header for PSEH3
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* For additional information see pseh3.c in the related library. */

#pragma once
#define _PSEH3_H_

#include "excpt.h"

typedef struct _SEH3$_SCOPE_TABLE
{
    void *Target;
    void *Filter;
} SEH3$_SCOPE_TABLE, *PSEH3$_SCOPE_TABLE;

typedef struct _SEH3$_EXCEPTION_POINTERS
{
    struct _EXCEPTION_RECORD *ExceptionRecord;
    struct _CONTEXT *ContextRecord;
} SEH3$_EXCEPTION_POINTERS, *PSEH3$_EXCEPTION_POINTERS;

typedef struct _SEH3$_REGISTRATION_FRAME
{
    /* First the Windows base record. Don't move this! */
    struct _SEH3$_REGISTRATION_FRAME *Next;
    void *Handler;

    /* Points to the end of the internal registration chain */
    struct _SEH3$_REGISTRATION_FRAME *EndOfChain;

    /* Pointer to the static scope table */
    PSEH3$_SCOPE_TABLE ScopeTable;

    /* Except handler stores pointer to exception pointers here */
    PSEH3$_EXCEPTION_POINTERS volatile ExceptionPointers;

    /* Registers that we need to save */
    unsigned long Esp;
    unsigned long Ebp;

} SEH3$_REGISTRATION_FRAME ,*PSEH3$_REGISTRATION_FRAME;

/* Prevent gcc from inlining functions that use SEH. */
static inline __attribute__((always_inline)) __attribute__((returns_twice)) void _SEH3$_PreventInlining() {}

extern inline __attribute__((always_inline,gnu_inline))
void _SEH3$_UnregisterFrame(volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame)
{
    asm volatile ("movl %k[NewHead], %%fs:0"
                  : : [NewHead] "ir" (RegistrationFrame->Next) : "memory");
}

extern inline __attribute__((always_inline,gnu_inline))
void _SEH3$_UnregisterTryLevel(
    volatile SEH3$_REGISTRATION_FRAME *TrylevelFrame)
{
    volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame;
    asm volatile ("movl %%fs:0, %k[RegistrationFrame]"
                  : [RegistrationFrame] "=r" (RegistrationFrame) : );
    RegistrationFrame->EndOfChain = TrylevelFrame->Next;
}

enum
{
    _SEH3$_TryLevel = 0,
};

/* These are global dummy definitions, that get overwritten in the local context of __finally / __except blocks */
int __cdecl __attribute__((error ("Can only be used inside a __finally block."))) _abnormal_termination(void);
unsigned long __cdecl __attribute__((error("Can only be used inside an exception filter or __except block."))) _exception_code(void);
void * __cdecl __attribute__((error("Can only be used inside an exception filter."))) _exception_info(void);

/* Define the registers that get clobbered, when reaching the __except block.
   We specify ebp on optimized builds without frame pointer, since it will be
   used by GCC as a general purpose register then. */
#if defined(__OPTIMIZE__) && defined(_ALLOW_OMIT_FRAME_POINTER)
#define _SEH3$_CLOBBER_ON_EXCEPTION "ebp", "ebx", "ecx", "edx", "esi", "edi", "flags", "memory"
#else
#define _SEH3$_CLOBBER_ON_EXCEPTION "ebx", "ecx", "edx", "esi", "edi", "flags", "memory"
#endif

/* This attribute allows automatic cleanup of the registered frames */
#define _SEH3$_AUTO_CLEANUP __attribute__((cleanup(_SEH3$_AutoCleanup)))

#define _SEH3$_ASM_GOTO(_Asm, _Label, ...) asm goto (_Asm : : : "memory", ## __VA_ARGS__ : _Label)

#define _SEH3$_DECLARE_EXCEPT_INTRINSICS() \
    inline __attribute__((always_inline, gnu_inline)) \
    unsigned long _exception_code() { return _SEH3$_TrylevelFrame.ExceptionPointers->ExceptionRecord->ExceptionCode; }

/* This is an asm wrapper around _SEH3$_RegisterFrame */
#define _SEH3$_RegisterFrame(_TrylevelFrame, _DataTable, _Target) \
    asm goto ("leal %0, %%ecx\n" \
              "call __SEH3$_RegisterFrame\n" \
              : \
              : "m" (*(_TrylevelFrame)), "a" (_DataTable) \
              : "ecx", "edx", "memory" \
              : _Target)

/* This is an asm wrapper around _SEH3$_EnterTryLevel */
#define _SEH3$_RegisterTryLevel(_TrylevelFrame, _DataTable, _Target) \
    asm goto ("leal %0, %%ecx\n" \
              "call __SEH3$_RegisterTryLevel\n" \
              : \
              : "m" (*(_TrylevelFrame)), "a" (_DataTable) \
              : "ecx", "edx", "memory" \
              : _Target)

/* On GCC the filter function is a nested function with __fastcall calling
   convention. The eax register contains a base address the function uses
   to address the callers stack frame. __fastcall is chosen, because it gives
   us an effective was of passing one argument to the function, that we need
   to tell the function in a first pass to return informtion about the frame
   base address. Since this is something GCC chooses arbitrarily, we call
   the function with an arbitrary base address in eax first and then use the
   result to calculate the correct address for a second call to the function. */
#define _SEH3$_DECLARE_FILTER_FUNC(_Name) \
    auto int __fastcall _Name(int Action)

#define _SEH3$_NESTED_FUNC_OPEN(_Name) \
    int __fastcall _Name(int Action) \
    { \
        /* This is a fancy way to get information about the frame layout */ \
        if (Action == 0) return (int)&_SEH3$_TrylevelFrame;

#define _SEH3$_DEFINE_FILTER_FUNC(_Name, expression) \
    _SEH3$_NESTED_FUNC_OPEN(_Name) \
        /* Declare the intrinsics for exception filters */ \
        inline __attribute__((always_inline, gnu_inline)) \
        unsigned long _exception_code() { return _SEH3$_TrylevelFrame.ExceptionPointers->ExceptionRecord->ExceptionCode; } \
        inline __attribute__((always_inline, gnu_inline)) \
        void * _exception_info() { return _SEH3$_TrylevelFrame.ExceptionPointers; } \
\
        /* Now handle the actual filter expression */ \
        return (expression); \
    }

#define _SEH3$_FINALLY_FUNC_OPEN(_Name) \
    _SEH3$_NESTED_FUNC_OPEN(_Name) \
        /* Declare the intrinsics for the finally function */ \
        inline __attribute__((always_inline, gnu_inline)) \
        int _abnormal_termination() { return (_SEH3$_TrylevelFrame.ScopeTable != 0); }

#define _SEH3$_FILTER(_Filter, _FilterExpression) \
    (__builtin_constant_p(_FilterExpression) ? (void*)(unsigned long)(unsigned char)(unsigned long)(_FilterExpression) : _Filter)

#define _SEH3$_DEFINE_DUMMY_FINALLY(_Name) \
    auto inline __attribute__((always_inline,gnu_inline)) int _Name(int Action) { (void)Action; return 0; }

#define _SEH3$_DECLARE_CLEANUP_FUNC(_Name) \
    auto inline __attribute__((always_inline,gnu_inline)) void _Name(volatile SEH3$_REGISTRATION_FRAME *p)

#define _SEH3$_DEFINE_CLEANUP_FUNC(_Name) \
    _SEH3$_DECLARE_CLEANUP_FUNC(_Name) \
    { \
        (void)p; \
        /* Unregister the frame */ \
        if (_SEH3$_TryLevel == 1) _SEH3$_UnregisterFrame(&_SEH3$_TrylevelFrame); \
        else _SEH3$_UnregisterTryLevel(&_SEH3$_TrylevelFrame); \
\
        /* Invoke the finally function (an inline dummy in the __except case) */ \
        _SEH3$_FinallyFunction(1); \
    }

/* This construct scares GCC so much, that it will stop moving code
   around into places that are never executed. */
#define _SEH3$_SCARE_GCC() \
        void *plabel; \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_BeforeTry); \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_HandlerTarget); \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_OnException); \
        asm volatile ("#" : "=a"(plabel) : "p"(&&_SEH3$_l_BeforeTry), "p"(&&_SEH3$_l_HandlerTarget), "p"(&&_SEH3$_l_OnException) \
                      : _SEH3$_CLOBBER_ON_EXCEPTION ); \
        goto _SEH3$_l_OnException;


#define _SEH3_TRY \
    _SEH3$_PreventInlining(); \
    /* Enter the outer scope */ \
    do { \
        /* Declare local labels */ \
        __label__ _SEH3$_l_BeforeTry; \
        __label__ _SEH3$_l_DoTry; \
        __label__ _SEH3$_l_AfterTry; \
        __label__ _SEH3$_l_EndTry; \
        __label__ _SEH3$_l_HandlerTarget; \
        __label__ _SEH3$_l_OnException; \
\
        /* Count the try level. Outside of any __try, _SEH3$_TryLevel is 0 */ \
        enum { \
            _SEH3$_PreviousTryLevel = _SEH3$_TryLevel, \
            _SEH3$_TryLevel = _SEH3$_PreviousTryLevel + 1, \
        }; \
\
        /* Forward declaration of the auto cleanup function */ \
        _SEH3$_DECLARE_CLEANUP_FUNC(_SEH3$_AutoCleanup); \
\
        /* Allocate a registration frame */ \
        volatile SEH3$_REGISTRATION_FRAME _SEH3$_AUTO_CLEANUP _SEH3$_TrylevelFrame; \
\
        goto _SEH3$_l_BeforeTry; \
        /* Silence warning */ goto _SEH3$_l_AfterTry; \
\
    _SEH3$_l_DoTry: \
        do


#define _SEH3_EXCEPT(...) \
        /* End the try block */ \
        while (0); \
    _SEH3$_l_AfterTry: (void)0; \
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_BeforeTry: (void)0; \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_OnException); \
\
        /* Forward declaration of the filter function */ \
        _SEH3$_DECLARE_FILTER_FUNC(_SEH3$_FilterFunction); \
\
        /* Create a static data table that contains the jump target and filter function */ \
        static const SEH3$_SCOPE_TABLE _SEH3$_ScopeTable = { &&_SEH3$_l_HandlerTarget, _SEH3$_FILTER(&_SEH3$_FilterFunction, (__VA_ARGS__)) }; \
\
        /* Register the registration record. */ \
        if (_SEH3$_TryLevel == 1) _SEH3$_RegisterFrame(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable, _SEH3$_l_HandlerTarget); \
        else _SEH3$_RegisterTryLevel(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable, _SEH3$_l_HandlerTarget); \
\
        /* Emit the filter function */ \
        _SEH3$_DEFINE_FILTER_FUNC(_SEH3$_FilterFunction, (__VA_ARGS__)) \
\
        /* Define an empty inline finally function */ \
        _SEH3$_DEFINE_DUMMY_FINALLY(_SEH3$_FinallyFunction) \
\
        /* Allow intrinsics for __except to be used */ \
        _SEH3$_DECLARE_EXCEPT_INTRINSICS() \
\
        goto _SEH3$_l_DoTry; \
\
    _SEH3$_l_HandlerTarget: (void)0; \
\
        if (1) \
        { \
            do


#define _SEH3_FINALLY \
        /* End the try block */ \
        while (0); \
    _SEH3$_l_AfterTry: (void)0; \
        /* Set ScopeTable to 0, this is used by _abnormal_termination() */ \
         _SEH3$_TrylevelFrame.ScopeTable = 0; \
\
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_BeforeTry: (void)0; \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_OnException); \
\
        /* Forward declaration of the finally function */ \
        _SEH3$_DECLARE_FILTER_FUNC(_SEH3$_FinallyFunction); \
\
        /* Create a static data table that contains the finally function */ \
        static const SEH3$_SCOPE_TABLE _SEH3$_ScopeTable = { 0, &_SEH3$_FinallyFunction }; \
\
        /* Register the registration record. */ \
        if (_SEH3$_TryLevel == 1) _SEH3$_RegisterFrame(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable, _SEH3$_l_HandlerTarget); \
        else _SEH3$_RegisterTryLevel(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable, _SEH3$_l_HandlerTarget); \
\
        goto _SEH3$_l_DoTry; \
\
    _SEH3$_l_HandlerTarget: (void)0; \
\
        _SEH3$_FINALLY_FUNC_OPEN(_SEH3$_FinallyFunction) \
            /* This construct makes sure that the finally function returns */ \
            /* a proper value at the end */ \
            for (; ; (void)({return 0; 0;}))


#define _SEH3_END \
            while (0); \
        }; \
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_OnException: (void)0; \
        /* Force GCC to create proper code pathes */ \
        _SEH3$_SCARE_GCC() \
\
    _SEH3$_l_EndTry:(void)0; \
        _SEH3$_ASM_GOTO("#\n", _SEH3$_l_OnException); \
\
        /* Implementation of the auto cleanup function */ \
        _SEH3$_DEFINE_CLEANUP_FUNC(_SEH3$_AutoCleanup); \
\
    /* Close the outer scope */ \
    } while (0);

#define _SEH3_LEAVE goto _SEH3$_l_AfterTry

#define _SEH3_VOLATILE volatile
