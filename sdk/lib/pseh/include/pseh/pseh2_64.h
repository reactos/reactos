
#pragma once

/* Declare our global trampoline function for filter and unwinder */
__asm__(
    ".p2align 4, 0x90\n"
    ".seh_proc __seh2_global_filter_func\n"
    "__seh2_global_filter_func:\n"
    /* r8 is rbp - frame-offset. Calculate the negative frame-offset */
    "\tsub %rbp, %rax\n"
    "\tpush %rbp\n"
    "\t.seh_pushreg %rbp\n"
    "\tsub $32, %rsp\n"
    "\t.seh_stackalloc 32\n"
    "\t.seh_endprologue\n"
    /* rdx is the original stack pointer, fix it up to be the frame pointer */
    "\tsub %rax, %rdx\n"
    /* Restore frame pointer. */
    "\tmov %rdx, %rbp\n"
    /* Actually execute the filter funclet */
    "\tjmp *%r8\n"
    "__seh2_global_filter_func_exit:\n"
    "\t.p2align 4\n"
    "\tadd $32, %rsp\n"
    "\tpop %rbp\n"
    "\tret\n"
    "\t.seh_endproc");

#define STRINGIFY(a) #a
#define EMIT_PRAGMA_(params) \
    _Pragma( STRINGIFY(params) )
#define EMIT_PRAGMA(type,line) \
    EMIT_PRAGMA_(REACTOS seh(type,line))

#define _SEH3$_EMIT_DEFS_AND_PRAGMA__(Line, Type)                                   \
    /* Emit assembler constants with line number to be individual */                \
    __asm__ __volatile__ goto ("\n"                                                 \
        "\t__seh2$$begin_try__" #Line "=%l0\n" /* Begin of tried code */            \
        "\t__seh2$$end_try__" #Line "=%l1 + 1\n" /* End of tried code */            \
        "\t__seh2$$filter__" #Line "=%l2\n" /* Filter function */                   \
        "\t__seh2$$begin_except__" #Line "=%l3\n" /* Called on except */            \
            : /* No output */                                                       \
            : /* No input */                                                        \
            : /* No clobber */                                                      \
            : __seh2$$begin_try__,                                                  \
              __seh2$$end_try__,                                                    \
              __seh2$$filter__,                                                     \
              __seh2$$begin_except__);                                              \
    /* Call our home-made pragma */                                                 \
    EMIT_PRAGMA(Type,Line)

#define _SEH3$_EMIT_DEFS_AND_PRAGMA_(Line, Type) _SEH3$_EMIT_DEFS_AND_PRAGMA__(Line, Type)
#define _SEH3$_EMIT_DEFS_AND_PRAGMA(Type) _SEH3$_EMIT_DEFS_AND_PRAGMA_(__LINE__, Type)

#define _SEH2_TRY                                                                   \
{                                                                                   \
    __label__ __seh2$$filter__;                                                     \
    __label__ __seh2$$begin_except__;                                               \
    __label__ __seh2$$begin_try__;                                                  \
    __label__ __seh2$$end_try__;                                                    \
    /*                                                                              \
     * We close the current SEH block for this function and install our own.        \
     * At this point GCC emitted its prologue, and if it saves more                 \
     * registers, the relevant instruction will be valid for our scope as well.     \
     * We also count the number of try blocks at assembly level                     \
     * to properly set the handler data when we're done.                            \
     */                                                                             \
__seh2$$begin_try__:                                                                \
    {                                                                               \
        __label__ __seh2$$leave_scope__;

#define _SEH2_EXCEPT(...)                                                                       \
__seh2$$leave_scope__: __MINGW_ATTRIB_UNUSED;                                                   \
    }                                                                                           \
__seh2$$end_try__:(void)0;                                                                      \
    /* Call our home-made pragma */                                                             \
    _SEH3$_EMIT_DEFS_AND_PRAGMA(__seh$$except);                                                 \
    if (0)                                                                                      \
    {                                                                                           \
        __label__ __seh2$$leave_scope__;                                                        \
        LONG __MINGW_ATTRIB_UNUSED __seh2$$exception_code__;                                    \
        /* Add our handlers to the list */                                                      \
        if (0)                                                                                  \
        {                                                                                       \
            /* Jump to the global filter. Tell it where the filter funclet lies */              \
            __label__ __seh2$$filter_funclet__;                                                 \
            __seh2$$filter__:                                                                   \
            __asm__ __volatile__ goto(                                                          \
                "\tleaq %l1(%%rip), %%r8\n"                                                     \
                "\tjmp __seh2_global_filter_func\n"                                             \
                : /* No output */                                                               \
                : "a"(__builtin_frame_address(0))                                               \
                : "%r8"                                                                         \
                : __seh2$$filter_funclet__);                                                    \
            /* Actually declare our filter funclet */                                           \
            struct _EXCEPTION_POINTERS* __seh2$$exception_ptr__;                                \
            __seh2$$filter_funclet__:                                                           \
            /* At this point, the compiler can't count on any register being valid */           \
            __asm__ __volatile__(""                                                             \
                : "=c"(__seh2$$exception_ptr__) /* First argument of the filter function */     \
                : /* No input */                                                                \
                : /* Everything */                                                              \
                "%rax", "%rbx","%rdx", "%rdi", "%rsi",                                          \
                "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15");                  \
            /* Save exception code */                                                           \
            __seh2$$exception_code__ = __seh2$$exception_ptr__->ExceptionRecord->ExceptionCode; \
            /* Actually evaluate our filter */                                                  \
            register long __MINGW_ATTRIB_UNUSED __seh2$$filter_funclet_ret __asm__("eax") =     \
                ((__VA_ARGS__));                                                                \
            /* Go back to the global filter function */                                         \
            __asm__("jmp __seh2_global_filter_func_exit");                                      \
        }                                                                                       \
        /* Protect us from emitting instructions to jump back to the filter function */         \
        enum                                                                                    \
        {                                                                                       \
            __seh2$$abnormal_termination__ = 0                                                  \
        };                                                                                      \
        __seh2$$begin_except__:

#define _SEH2_FINALLY                                                                       \
__seh2$$leave_scope__: __MINGW_ATTRIB_UNUSED;                                               \
    }                                                                                       \
__seh2$$end_try__:                                                                          \
__seh2$$begin_except__: __MINGW_ATTRIB_UNUSED;                                              \
    /* Call our home-made pragma */                                                         \
    _SEH3$_EMIT_DEFS_AND_PRAGMA(__seh$$finally);                                            \
    if (1)                                                                                  \
    {                                                                                       \
        __label__ __seh2$$finally__;                                                        \
        __label__ __seh2$$begin_finally__;                                                  \
        __label__ __seh2$$leave_scope__;                                                    \
        __asm__ __volatile__ goto("" : : : : __seh2$$finally__);                            \
        int __seh2$$abnormal_termination__;                                                 \
        if (0)                                                                              \
        {                                                                                   \
            /* Jump to the global trampoline. Tell it where the unwind code really lies */  \
            __seh2$$filter__: __MINGW_ATTRIB_UNUSED;                                        \
            __seh2$$finally__: __MINGW_ATTRIB_UNUSED;                                       \
            __asm__ __volatile__ goto(                                                      \
                "\t\n"                                                                      \
                "\tleaq %l1(%%rip), %%r8\n"                                                 \
                "\tjmp __seh2_global_filter_func\n"                                         \
                : /* No output */                                                           \
                : "a"(__builtin_frame_address(0))                                           \
                : "%r8"                                                                     \
                : __seh2$$begin_finally__);                                                 \
        }                                                                                   \
                                                                                            \
        /* Zero-out rcx to indicate normal termination */                                   \
        __asm__ __volatile__("xor %%rcx, %%rcx"                                             \
            : /* No output */                                                               \
            : /* No input */                                                                \
            : /* Everything - We might come from __C_specific_handler here */               \
            "%rax", "%rbx", "%rcx", "%rdx", "%rdi", "%rsi",                                 \
            "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15");                  \
        /* Actually declare our finally funclet */                                          \
        __seh2$$begin_finally__:                                                            \
        __asm__ __volatile__(""                                                             \
            : "=c" (__seh2$$abnormal_termination__));

#define _SEH2_END                                                                   \
        __seh2$$leave_scope__: __MINGW_ATTRIB_UNUSED;                               \
        if (__seh2$$abnormal_termination__)                                         \
        {                                                                           \
            __asm__("jmp __seh2_global_filter_func_exit");                          \
        }                                                                           \
    }                                                                               \
}

#define _SEH2_GetExceptionInformation() __seh2$$exception_ptr__
#define _SEH2_GetExceptionCode() __seh2$$exception_code__
#define _SEH2_AbnormalTermination() __seh2$$abnormal_termination__
#define _SEH2_LEAVE goto __seh2$$leave_scope__
#define _SEH2_YIELD(__stmt) __stmt
#define _SEH2_VOLATILE volatile

#ifndef __try // Conflict with GCC's STL
#define __try _SEH2_TRY
#define __except _SEH2_EXCEPT
#define __finally _SEH2_FINALLY
#define __endtry _SEH2_END
#define __leave goto __seh2$$leave_scope__
#define _exception_info() __seh2$$exception_ptr__
#define _exception_code() __seh2$$exception_code__
#define _abnormal_termination() __seh2$$abnormal_termination__
#endif
