/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * PURPOSE:         Header for PSEH3
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* For additional information see pseh3.c in the related library. */

#pragma once
#define _PSEH3_H_

#include <excpt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CLANG must safe non-volatiles, because it uses a return-twice algorithm */
#if defined(__clang__) && !defined(_SEH3$_FRAME_ALL_NONVOLATILES)
#define _SEH3$_FRAME_ALL_NONVOLATILES 1
#endif

enum
{
    _SEH3$_NESTED_HANDLER = 0,
    _SEH3$_CPP_HANDLER = 1,
    _SEH3$_CLANG_HANDLER = 2,
#ifdef __clang__
    _SEH3$_HANDLER_TYPE = _SEH3$_CLANG_HANDLER,
#elif defined(__cplusplus)
    _SEH3$_HANDLER_TYPE = _SEH3$_CPP_HANDLER,
#else
    _SEH3$_HANDLER_TYPE = _SEH3$_NESTED_HANDLER,
#endif
};

typedef struct _SEH3$_SCOPE_TABLE
{
    void *Target;
    void *Filter;
    unsigned char TryLevel;
    unsigned char HandlerType;
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

    /* Except handler stores the exception code here */
    unsigned long ExceptionCode;

    /* Registers that we need to save */
    unsigned long Esp;
    unsigned long Ebp;

    char* AllocaFrame;
#ifdef _SEH3$_FRAME_ALL_NONVOLATILES
    unsigned long Ebx;
    unsigned long Esi;
    unsigned long Edi;
#endif
#ifdef __clang__
    void *ReturnAddress;
#endif
} SEH3$_REGISTRATION_FRAME ,*PSEH3$_REGISTRATION_FRAME;

/* Prevent gcc from inlining functions that use SEH. */
static inline __attribute__((always_inline)) __attribute__((returns_twice)) void _SEH3$_PreventInlining() {}

/* Unregister the root frame */
extern inline __attribute__((always_inline,gnu_inline))
void _SEH3$_UnregisterFrame(volatile SEH3$_REGISTRATION_FRAME *RegistrationFrame)
{
    asm volatile ("movl %k[NewHead], %%fs:0"
                  : : [NewHead] "ir" (RegistrationFrame->Next) : "memory");
}

/* Unregister a trylevel frame */
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

#ifndef __clang__
/* These are global dummy definitions, that get overwritten in the local context of __finally / __except blocks */
int __cdecl __attribute__((error ("Can only be used inside a __finally block."))) _abnormal_termination(void);
unsigned long __cdecl __attribute__((error("Can only be used inside an exception filter or __except block."))) _exception_code(void);
void * __cdecl __attribute__((error("Can only be used inside an exception filter."))) _exception_info(void);
#endif

/* This attribute allows automatic cleanup of the registered frames */
#define _SEH3$_AUTO_CLEANUP __attribute__((cleanup(_SEH3$_AutoCleanup)))

int
__attribute__((regparm(3)))
__attribute__((returns_twice))
_SEH3$_RegisterFrameWithNonVolatiles(
    volatile SEH3$_REGISTRATION_FRAME* RegistrationFrame,
    const SEH3$_SCOPE_TABLE* ScopeTable,
    void* AllocaFrame);

int
__attribute__((regparm(3)))
__attribute__((returns_twice))
_SEH3$_RegisterTryLevelWithNonVolatiles(
    volatile SEH3$_REGISTRATION_FRAME* RegistrationFrame,
    const SEH3$_SCOPE_TABLE* ScopeTable,
    void* AllocaFrame);

/* CLANG specific definitions! */
#ifdef __clang__

/* CLANG thinks it is smart and optimizes the alloca away if it is 0 and with it the use of a frame register */
#define _SEH3$_EnforceFramePointer() asm volatile ("#\n" : : "m"(*(char*)__builtin_alloca(4)) : "%esp", "memory")

/* CLANG doesn't have asm goto! */
#define _SEH3$_ASM_GOTO(...)

#define _SEH3$_RegisterFrame_(_TrylevelFrame, _DataTable) \
    do { \
        int result = _SEH3$_RegisterFrameWithNonVolatiles(_TrylevelFrame, _DataTable, __builtin_alloca(0)); \
        if (__builtin_expect(result != 0, 0)) \
        { \
            if (result == 1) goto _SEH3$_l_FilterOrFinally; \
            if (result == 2) goto _SEH3$_l_HandlerTarget; \
            goto _SEH3$_l_BeforeFilterOrFinally; \
        } \
    } while(0)

#define _SEH3$_RegisterTryLevel_(_TrylevelFrame, _DataTable) \
    do { \
        int result = _SEH3$_RegisterTryLevelWithNonVolatiles(_TrylevelFrame, _DataTable, __builtin_alloca(0)); \
        if (__builtin_expect(result != 0, 0)) \
        { \
            if (result == 1) goto _SEH3$_l_FilterOrFinally; \
            if (result == 2) goto _SEH3$_l_HandlerTarget; \
            goto _SEH3$_l_BeforeFilterOrFinally; \
        } \
    } while(0)

#define _SEH3$_SCARE_GCC()

#else /* !__clang__ */

/* This will make GCC use ebp, even if it was disabled by -fomit-frame-pointer */
#define _SEH3$_EnforceFramePointer() asm volatile ("#\n" : : "m"(*(char*)__builtin_alloca(0)) : "%esp", "memory")

#define _SEH3$_ASM_GOTO(...) asm goto ("#\n" : : : "memory" : __VA_ARGS__)

#ifdef __cplusplus
#define _SEH3$_CALL_WRAPPER(_Function, _TrylevelFrame, _DataTable) \
    asm goto ("leal %0, %%eax\n\t" \
              "leal %1, %%edx\n\t" \
              "call " #_Function "WithStackLayout" \
              : \
              : "m" (*(_TrylevelFrame)), "m" (*(_DataTable)), "c" (__builtin_alloca(0)), "p" (_SEH3$_RegisterFrameWithNonVolatiles) \
              : "eax", "edx", "memory" \
              : _SEH3$_l_BeforeTry, _SEH3$_l_HandlerTarget, _SEH3$_l_OnException, _SEH3$_l_BeforeFilterOrFinally, _SEH3$_l_FilterOrFinally)

#else
#define _SEH3$_CALL_WRAPPER(_Function, _TrylevelFrame, _DataTable) \
    asm goto ("leal %0, %%eax\n\t" \
              "leal %1, %%edx\n\t" \
              "call " #_Function \
              : \
              : "m" (*(_TrylevelFrame)), "m" (*(_DataTable)), "p" (_SEH3$_RegisterFrameWithNonVolatiles) \
              : "eax", "edx", "ecx", "memory" \
              : _SEH3$_l_BeforeTry, _SEH3$_l_HandlerTarget, _SEH3$_l_OnException, _SEH3$_l_BeforeFilterOrFinally, _SEH3$_l_FilterOrFinally)
#endif

/* This is an asm wrapper around _SEH3$_RegisterFrame */
#define _SEH3$_RegisterFrame_(_TrylevelFrame, _DataTable) \
     _SEH3$_CALL_WRAPPER(__SEH3$_RegisterFrame, _TrylevelFrame, _DataTable)

/* This is an asm wrapper around _SEH3$_RegisterTryLevel */
#define _SEH3$_RegisterTryLevel_(_TrylevelFrame, _DataTable) \
     _SEH3$_CALL_WRAPPER(__SEH3$_RegisterTryLevel, _TrylevelFrame, _DataTable)

/* This construct scares GCC so much, that it will stop moving code
   around into places that are never executed. */
#define _SEH3$_SCARE_GCC() \
        void *plabel; \
        _SEH3$_ASM_GOTO(_SEH3$_l_BeforeTry, _SEH3$_l_HandlerTarget, _SEH3$_l_OnException, _SEH3$_l_BeforeFilterOrFinally, _SEH3$_l_FilterOrFinally); \
        asm volatile ("#" : "=a"(plabel) : "p"(&&_SEH3$_l_BeforeTry), "p"(&&_SEH3$_l_HandlerTarget), "p"(&&_SEH3$_l_OnException), "p"(&&_SEH3$_l_FilterOrFinally) \
                      : "ebx", "ecx", "edx", "esi", "edi", "flags", "memory" ); \
        goto _SEH3$_l_OnException;

#endif /* __clang__ */

/* Neither CLANG nor C++ support nested functions */
#if defined(__cplusplus) || defined(__clang__)

/* Use the global unregister function */
void
__attribute__((regparm(1)))
_SEH3$_AutoCleanup(
    volatile SEH3$_REGISTRATION_FRAME *Frame);

/* These are only dummies here */
#define _SEH3$_DECLARE_CLEANUP_FUNC(_Name)
#define _SEH3$_DEFINE_CLEANUP_FUNC(_Name)
#define _SEH3$_DECLARE_FILTER_FUNC(_Name)
#define _SEH3$_DEFINE_DUMMY_FINALLY(_Name)

/* On invocation, the AllocaFrame field is loaded with the return esp value */
#define _SEH3$_NESTED_FUNC_RETURN(_Result) \
        /* Restore esp and return to the caller */ \
        asm volatile ("movl %[FixedEsp], %%esp\n\tret" \
            : : "a" (_Result), [FixedEsp] "m" (_SEH3$_TrylevelFrame.AllocaFrame) : "ebx", "ecx", "edx", "esi", "edi", "flags", "memory")

/* The filter "function" */
#define _SEH3$_DEFINE_FILTER_FUNC(_Name, expression) \
    { \
        /* Evaluate and return the filter expression */ \
        asm volatile ("#\n" : : : "eax", "ebx", "ecx", "edx", "esi", "edi", "flags", "memory"); \
        _SEH3$_NESTED_FUNC_RETURN((expression)); \
    }

#define _SEH3$_FINALLY_FUNC_OPEN(_Name) \
    { \
        asm volatile ("#\n" : : : "eax", "ebx", "ecx", "edx", "esi", "edi", "flags", "memory"); \
        /* This construct makes sure that the finally function returns */ \
        /* a proper value at the end */ \
        for (; ; (void)({_SEH3$_NESTED_FUNC_RETURN(0); 0;}))

#define _SEH3$_FILTER(_Filter, _FilterExpression) (&&_SEH3$_l_FilterOrFinally)
#define _SEH3$_FINALLY(_Finally) (&&_SEH3$_l_FilterOrFinally)

#define _SEH3$_DECLARE_EXCEPT_INTRINSICS()

/* Since we cannot use nested functions, we declare these globally as macros */
#define _abnormal_termination() (_SEH3$_TrylevelFrame.ExceptionPointers != 0)
#define _exception_code() (_SEH3$_TrylevelFrame.ExceptionCode)
#define _exception_info() (_SEH3$_TrylevelFrame.ExceptionPointers)

#else /* __cplusplus || __clang__ */

#define _SEH3$_DECLARE_EXCEPT_INTRINSICS() \
    inline __attribute__((always_inline, gnu_inline)) \
    unsigned long _exception_code() { return _SEH3$_TrylevelFrame.ExceptionCode; }

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
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wshadow\"") \
        inline __attribute__((always_inline, gnu_inline)) \
        unsigned long _exception_code() { return _SEH3$_TrylevelFrame.ExceptionCode; } \
        inline __attribute__((always_inline, gnu_inline)) \
        void * _exception_info() { return _SEH3$_TrylevelFrame.ExceptionPointers; } \
_Pragma("GCC diagnostic pop") \
\
        /* Now handle the actual filter expression */ \
        return (expression); \
    }

#define _SEH3$_FINALLY_FUNC_OPEN(_Name) \
    _SEH3$_NESTED_FUNC_OPEN(_Name) \
        /* Declare the intrinsics for the finally function */ \
        inline __attribute__((always_inline, gnu_inline)) \
        int _abnormal_termination() { return (_SEH3$_TrylevelFrame.ExceptionPointers != 0); } \
\
        /* This construct makes sure that the finally function returns */ \
        /* a proper value at the end */ \
        for (; ; (void)({return 0; 0;}))

#define _SEH3$_FILTER(_Filter, _FilterExpression) \
    (__builtin_constant_p(_FilterExpression) ? (void*)(unsigned long)(unsigned char)(unsigned long)(_FilterExpression) : _Filter)

#define _SEH3$_FINALLY(_Finally) (_Finally)

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

#endif /* __cplusplus || __clang__ */



#define _SEH3_TRY \
    _SEH3$_PreventInlining(); \
    /* Enter the outer scope */ \
    if (1) { \
        /* Declare local labels */ \
        __label__ _SEH3$_l_BeforeTry; \
        __label__ _SEH3$_l_DoTry; \
        __label__ _SEH3$_l_AfterTry; \
        __label__ _SEH3$_l_EndTry; \
        __label__ _SEH3$_l_HandlerTarget; \
        __label__ _SEH3$_l_OnException; \
        __label__ _SEH3$_l_BeforeFilterOrFinally; \
        __label__ _SEH3$_l_FilterOrFinally; \
        (void)&&_SEH3$_l_OnException; \
        (void)&&_SEH3$_l_BeforeFilterOrFinally; \
        (void)&&_SEH3$_l_FilterOrFinally; \
\
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wdeclaration-after-statement\"") \
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
_Pragma("GCC diagnostic pop") \
\
        goto _SEH3$_l_BeforeTry; \
        /* Silence warning */ goto _SEH3$_l_AfterTry; \
\
    _SEH3$_l_DoTry: \
        if (1)


#define _SEH3_EXCEPT(...) \
    /* End of the try block */ \
    _SEH3$_l_AfterTry: (void)0; \
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_BeforeTry: (void)0; \
        _SEH3$_ASM_GOTO(_SEH3$_l_OnException); \
\
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wdeclaration-after-statement\"") \
\
        /* Forward declaration of the filter function */ \
        _SEH3$_DECLARE_FILTER_FUNC(_SEH3$_FilterFunction); \
\
        /* Create a static data table that contains the jump target and filter function */ \
        static const SEH3$_SCOPE_TABLE _SEH3$_ScopeTable = { &&_SEH3$_l_HandlerTarget, _SEH3$_FILTER(&_SEH3$_FilterFunction, (__VA_ARGS__)), _SEH3$_TryLevel, _SEH3$_HANDLER_TYPE }; \
\
        /* Register the registration record. */ \
        if (_SEH3$_TryLevel == 1) _SEH3$_RegisterFrame_(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable); \
        else _SEH3$_RegisterTryLevel_(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable); \
\
        /* Define an empty inline finally function */ \
        _SEH3$_DEFINE_DUMMY_FINALLY(_SEH3$_FinallyFunction) \
\
        /* Allow intrinsics for __except to be used */ \
        _SEH3$_DECLARE_EXCEPT_INTRINSICS(); \
\
        goto _SEH3$_l_DoTry; \
\
    _SEH3$_l_BeforeFilterOrFinally: (void)0; \
        /* Make sure the filter function doesn't use esp */ \
        _SEH3$_EnforceFramePointer(); \
\
    _SEH3$_l_FilterOrFinally: (void)0; \
        /* Emit the filter function */ \
        _SEH3$_DEFINE_FILTER_FUNC(_SEH3$_FilterFunction, (__VA_ARGS__)) \
\
    _SEH3$_l_HandlerTarget: (void)0; \
        _SEH3$_EnforceFramePointer(); \
\
        if (1) \
        { \
            /* Prevent this block from being optimized away */ \
            asm volatile ("#\n"); \
            if (1)


#define _SEH3_FINALLY \
    /* End of the try block */ \
    _SEH3$_l_AfterTry: (void)0; \
        /* Set ExceptionPointers to 0, this is used by _abnormal_termination() */ \
         _SEH3$_TrylevelFrame.ExceptionPointers = 0; \
\
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_BeforeTry: (void)0; \
        _SEH3$_ASM_GOTO(_SEH3$_l_OnException); \
\
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wdeclaration-after-statement\"") \
\
        /* Forward declaration of the finally function */ \
        _SEH3$_DECLARE_FILTER_FUNC(_SEH3$_FinallyFunction); \
\
        /* Create a static data table that contains the finally function */ \
        static const SEH3$_SCOPE_TABLE _SEH3$_ScopeTable = { 0, _SEH3$_FINALLY(&_SEH3$_FinallyFunction), _SEH3$_TryLevel, _SEH3$_HANDLER_TYPE }; \
\
        /* Register the registration record. */ \
        if (_SEH3$_TryLevel == 1) _SEH3$_RegisterFrame_(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable); \
        else _SEH3$_RegisterTryLevel_(&_SEH3$_TrylevelFrame, &_SEH3$_ScopeTable); \
        _SEH3$_TrylevelFrame.ExceptionPointers = (PSEH3$_EXCEPTION_POINTERS)1; \
\
        goto _SEH3$_l_DoTry; \
\
    _SEH3$_l_HandlerTarget: (void)0; \
        _SEH3$_EnforceFramePointer(); \
\
    _SEH3$_l_BeforeFilterOrFinally: (void)0; \
        _SEH3$_EnforceFramePointer(); \
    _SEH3$_l_FilterOrFinally: (void)0; \
        _SEH3$_FINALLY_FUNC_OPEN(_SEH3$_FinallyFunction)


#define _SEH3_END \
        }; \
        goto _SEH3$_l_EndTry; \
\
    _SEH3$_l_OnException: (void)0; \
        /* Force GCC to create proper code pathes */ \
        _SEH3$_SCARE_GCC(); \
\
    _SEH3$_l_EndTry:(void)0; \
        _SEH3$_ASM_GOTO(_SEH3$_l_OnException); \
\
        /* Implementation of the auto cleanup function */ \
        _SEH3$_DEFINE_CLEANUP_FUNC(_SEH3$_AutoCleanup); \
\
_Pragma("GCC diagnostic pop") \
\
    /* Close the outer scope */ \
    }

#define _SEH3_LEAVE goto _SEH3$_l_AfterTry

#define _SEH3_VOLATILE volatile


#ifdef __cplusplus
}; // extern "C"
#endif
