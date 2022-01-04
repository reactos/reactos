
#pragma once

/* Declare our global trampoline function for filter and unwinder */
__asm__(
    ".p2align 4, 0x90\n"
    ".seh_proc __seh2_global_filter_func\n"
    "__seh2_global_filter_func:\n"
    "\tpush %rbp\n"
    "\t.seh_pushreg %rbp\n"
    "\tsub $32, %rsp\n"
    "\t.seh_stackalloc 32\n"
    "\t.seh_endprologue\n"
    /* Restore frame pointer. */
    "\tmov %rdx, %rbp\n"
    /* Actually execute the filter funclet */
    "\tjmp *%rax\n"
    "__seh2_global_filter_func_exit:\n"
    "\t.p2align 4\n"
    "\tadd $32, %rsp\n"
    "\tpop %rbp\n"
    "\tret\n"
    "\t.seh_endproc");

#define _SEH2_TRY                                                                   \
{                                                                                   \
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
__seh2$$end_try__:                                                                              \
    /* Call our home-made pragma */                                                             \
    _Pragma("REACTOS seh(except)")                                                              \
    if (0)                                                                                      \
    {                                                                                           \
        __label__ __seh2$$leave_scope__;                                                        \
        __label__ __seh2$$filter__;                                                             \
        __label__ __seh2$$begin_except__;                                                       \
        LONG __MINGW_ATTRIB_UNUSED __seh2$$exception_code__ = 0;                                \
        /* Add our handlers to the list */                                                      \
        __asm__ __volatile__ goto ("\n"                                                         \
            "\t.seh_handlerdata\n"                                                              \
            "\t.rva %l0\n" /* Begin of tried code */                                            \
            "\t.rva %l1 + 1\n" /* End of tried code */                                          \
            "\t.rva %l2\n" /* Filter function */                                                \
            "\t.rva %l3\n" /* Called on except */                                               \
            "\t.seh_code\n"                                                                     \
                : /* No output */                                                               \
                : /* No input */                                                                \
                : /* No clobber */                                                              \
                : __seh2$$begin_try__,                                                          \
                    __seh2$$end_try__,                                                          \
                    __seh2$$filter__,                                                           \
                    __seh2$$begin_except__);                                                    \
        if (0)                                                                                  \
        {                                                                                       \
            /* Jump to the global filter. Tell it where the filter funclet lies */              \
            __label__ __seh2$$filter_funclet__;                                                 \
            __seh2$$filter__:                                                                   \
            __asm__ __volatile__ goto(                                                          \
                "\tleaq %l0(%%rip), %%rax\n"                                                    \
                "\tjmp __seh2_global_filter_func\n"                                             \
                : /* No output */                                                               \
                : /* No input */                                                                \
                : "%rax"                                                                        \
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
    /* Call our home-made pragma */                                                         \
    _Pragma("REACTOS seh(finally)")                                                         \
    if (1)                                                                                  \
    {                                                                                       \
        __label__ __seh2$$finally__;                                                        \
        __label__ __seh2$$begin_finally__;                                                  \
        __label__ __seh2$$leave_scope__;                                                    \
        int __seh2$$abnormal_termination__;                                                 \
        /* Add our handlers to the list */                                                  \
        __asm__ __volatile__ goto ("\n"                                                     \
            "\t.seh_handlerdata\n"                                                          \
            "\t.rva %l0\n" /* Begin of tried code */                                        \
            "\t.rva %l1 + 1\n" /* End of tried code */                                      \
            "\t.rva %l2\n" /* Filter function */                                            \
            "\t.long 0\n" /* Nothing for unwind code */                                     \
            "\t.seh_code\n"                                                                 \
                : /* No output */                                                           \
                : /* No input */                                                            \
                : /* No clobber */                                                          \
                : __seh2$$begin_try__,                                                      \
                    __seh2$$end_try__,                                                      \
                    __seh2$$finally__);                                                     \
        if (0)                                                                              \
        {                                                                                   \
            /* Jump to the global trampoline. Tell it where the unwind code really lies */  \
            __seh2$$finally__:                                                              \
            __asm__ __volatile__ goto(                                                      \
                "\tleaq %l0(%%rip), %%rax\n"                                                \
                "\tjmp __seh2_global_filter_func\n"                                         \
                : /* No output */                                                           \
                : /* No input */                                                            \
                : /* No clobber */                                                          \
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
