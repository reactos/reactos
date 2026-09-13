/*
 * oleaut32 thunking
 *
 * Copyright 2019, 2024 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#if 0
#pragma makedep arm64ec_x64
#endif

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "wine/asm.h"

#ifdef __i386__

__ASM_GLOBAL_FUNC( call_method,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-4\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-8\n\t")
                   "movl 12(%ebp),%edx\n\t"
                   "movl %esp,%edi\n\t"
                   "shll $2,%edx\n\t"
                   "jz 1f\n\t"
                   "subl %edx,%edi\n\t"
                   "andl $~15,%edi\n\t"
                   "movl %edi,%esp\n\t"
                   "movl 12(%ebp),%ecx\n\t"
                   "movl 16(%ebp),%esi\n\t"
                   "cld\n\t"
                   "rep; movsl\n"
                   "1:\tcall *8(%ebp)\n\t"
                   "subl %esp,%edi\n\t"
                   "movl 20(%ebp),%ecx\n\t"
                   "movl %edi,(%ecx)\n\t"
                   "leal -8(%ebp),%esp\n\t"
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
__ASM_GLOBAL_FUNC( call_double_method,
                   "jmp " __ASM_NAME("call_method") )

#elif defined __x86_64__

__ASM_GLOBAL_FUNC( call_method,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_SEH(".seh_pushreg %rsi\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_SEH(".seh_pushreg %rdi\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movq %rcx,%rax\n\t"
                   "movq $4,%rcx\n\t"
                   "cmp %rcx,%rdx\n\t"
                   "cmovgq %rdx,%rcx\n\t"
                   "leaq 0(,%rcx,8),%rdx\n\t"
                   "subq %rdx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %r8,%rsi\n\t"
                   "rep; movsq\n\t"
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "movq 0(%rsp),%xmm0\n\t"
                   "movq 8(%rsp),%xmm1\n\t"
                   "movq 16(%rsp),%xmm2\n\t"
                   "movq 24(%rsp),%xmm3\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )
__ASM_GLOBAL_FUNC( call_double_method,
                   "jmp " __ASM_NAME("call_method") )

#elif defined __aarch64__

__ASM_GLOBAL_FUNC( call_method,
                   "stp x29, x30, [sp, #-16]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   "mov x29, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "sub sp, sp, x1, lsl #3\n\t"
                   "cbz x1, 2f\n"
                   "1:\tsub x1, x1, #1\n\t"
                   "ldr x4, [x2, x1, lsl #3]\n\t"
                   "str x4, [sp, x1, lsl #3]\n\t"
                   "cbnz x1, 1b\n"
                   "2:\tmov x16, x0\n\t"
                   "mov x9, x3\n\t"
                   "ldp d0, d1, [x9]\n\t"
                   "ldp d2, d3, [x9, #0x10]\n\t"
                   "ldp d4, d5, [x9, #0x20]\n\t"
                   "ldp d6, d7, [x9, #0x30]\n\t"
                   "ldp x0, x1, [x9, #0x40]\n\t"
                   "ldp x2, x3, [x9, #0x50]\n\t"
                   "ldp x4, x5, [x9, #0x60]\n\t"
                   "ldp x6, x7, [x9, #0x70]\n\t"
                   "ldr x8, [x9, #0x80]\n\t"
                   "blr x16\n\t"
                   "mov sp, x29\n\t"
                   "ldp x29, x30, [sp], #16\n\t"
                   "ret" )
__ASM_GLOBAL_FUNC( call_float_method,
                   "b " __ASM_NAME("call_method") )
__ASM_GLOBAL_FUNC( call_double_method,
                   "b " __ASM_NAME("call_method") )

#elif defined __arm__

__ASM_GLOBAL_FUNC( call_method,
                   /* r0 = *func
                    * r1 = nb_stk_args
                    * r2 = *stk_args (pointer to 'nb_stk_args' DWORD values to push on stack)
                    * r3 = *reg_args (pointer to 8, 64-bit d0-d7 (double) values OR as 16, 32-bit s0-s15 (float) values, followed by 4, 32-bit (DWORD) r0-r3 values)
                    */
                   "push {fp, lr}\n\t"
                   ".seh_save_regs_w {fp,lr}\n\t"
                   "mov fp, sp\n\t"
                   ".seh_save_sp fp\n\t"
                   ".seh_endprologue\n\t"
                   "lsls r1, r1, #2\n\t"           /* r1 = nb_stk_args * sizeof(DWORD) */
                   "beq 1f\n\t"                    /* Skip allocation if no stack args */
                   "add r2, r2, r1\n"              /* Calculate ending address of incoming stack data */
                   "2:\tldr ip, [r2, #-4]!\n\t"    /* Get next value */
                   "str ip, [sp, #-4]!\n\t"        /* Push it on the stack */
                   "subs r1, r1, #4\n\t"           /* Decrement count */
                   "bgt 2b\n\t"                    /* Loop till done */
                   "1:\n\t"
                   "vldm r3!, {s0-s15}\n\t"        /* Load the s0-s15/d0-d7 arguments */
                   "mov ip, r0\n\t"                /* Save the function call address to ip before we nuke r0 with arguments to pass */
                   "ldm r3, {r0-r3}\n\t"           /* Load the r0-r3 arguments */
                   "blx ip\n\t"                    /* Call the target function */
                   "mov sp, fp\n\t"                /* Clean the stack using fp */
                   "pop {fp, pc}" )                /* Restore fp and return */
__ASM_GLOBAL_FUNC( call_float_method,
                   "b " __ASM_NAME("call_method") )
__ASM_GLOBAL_FUNC( call_double_method,
                   "b " __ASM_NAME("call_method") )

#endif
