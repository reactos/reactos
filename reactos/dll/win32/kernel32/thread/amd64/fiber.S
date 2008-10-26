/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/kernel32/thread/i386/fiber.S
 * PURPOSE:     Fiber context switch code for the x86 architecture
 * PROGRAMMERS: Alex Ionescu (alex@relsoft.net)
 *              KJK::Hyperion <noog@libero.it>
 */

#include <ndk/asm.h>

.globl _SwitchToFiber
.intel_syntax noprefix

_SwitchToFiber:
    /* FIXME: TODO */
    ret 4
