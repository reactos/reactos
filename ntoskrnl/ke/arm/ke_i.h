/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/ke_i.h
 * PURPOSE:         Implements macro-generated system call portable wrappers
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// First, cleanup after any previous invocation
//
#undef _1 
#undef _2 
#undef _3 
#undef _4 
#undef _5 
#undef _6 
#undef _7 
#undef _8 
#undef _9 
#undef a 
#undef b 
#undef c 
#undef d 
#undef e
#undef f
#undef _10
#undef _11
#undef SYSCALL

//
// Are we building the typedef prototypes?
//
#ifdef PROTO
    //
    // Then, each parameter is actually a prototype argument
    //
    #define _1 PVOID
    #define _2 PVOID
    #define _3 PVOID
    #define _4 PVOID
    #define _5 PVOID
    #define _6 PVOID
    #define _7 PVOID
    #define _8 PVOID
    #define _9 PVOID
    #define a PVOID
    #define b PVOID
    #define c PVOID
    #define d PVOID
    #define e PVOID
    #define f PVOID
    #define _10 PVOID
    #define _11 PVOID

    //
    // And we generate the typedef
    //
    #define SYSCALL(x, y) typedef NTSTATUS (*PKI_SYSCALL_##x##PARAM)y;

    //
    // Cleanup for next run
    //
    #undef PROTO
#else
    //
    // Each parameter is actually an argument for the system call
    //
    #define _1 g[0x00]
    #define _2 g[0x01]
    #define _3 g[0x02]
    #define _4 g[0x03]
    #define _5 g[0x04]
    #define _6 g[0x05]
    #define _7 g[0x06]
    #define _8 g[0x07]
    #define _9 g[0x08]
    #define a g[0x09]
    #define b g[0x0A]
    #define c g[0x0B]
    #define d g[0x0C]
    #define e g[0x0D]
    #define f g[0x0E]
    #define _10 g[0x0F]
    #define _11 g[0x10]

    //
    // And we generate the actual system call
    //
    #define SYSCALL(x, y)                        \
        NTSTATUS                                 \
        KiSyscall##x##Param(                     \
            IN PVOID p,                          \
            IN PVOID *g                          \
        )                                        \
        {                                        \
            return ((PKI_SYSCALL_##x##PARAM)p)y; \
        }

    //
    // Cleanup for next run
    //
    #undef FUNC
#endif
