/* $Id: setjmplongjmp.s,v 1.1 2003/04/01 08:38:33 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           FreeType implementation for ReactOS
 * PURPOSE:           Implementation of _setjmp/longjmp
 * FILE:              thirdparty/freetype/i386/setjmplongjmp.s
 * PROGRAMMER:        Ge van Geldorp (ge@gse.nl)
 * NOTES:             Copied from glibc.
 *                    I have the feeling this could be implemented using the SEH
 *                    routines, but if it's good enough for glibc it's propably
 *                    good enough for me...
 *                    The MingW headers define jmp_buf to be an array of 16 ints,
 *                    based on the jmp_buf used by MSCVRT. We're using only 6 of
 *                    them, so plenty of space.
 */

#define JB_BX  0
#define JB_SI  1
#define JB_DI  2
#define JB_BP  3
#define JB_SP  4
#define JB_PC  5

#define PCOFF  0

#define JMPBUF 4

/*
 * int
 * _setjmp(jmp_buf env);
 *
 * Parameters:
 *   [ESP+04h] - jmp_buf env
 * Registers:
 *   None
 * Returns:
 *   0
 * Notes:
 *   Sets up the jmp_buf
 */
.globl __setjmp
__setjmp:
    xorl %eax, %eax
    movl JMPBUF(%esp), %edx

    /* Save registers.  */
    movl %ebx, (JB_BX*4)(%edx)
    movl %esi, (JB_SI*4)(%edx)
    movl %edi, (JB_DI*4)(%edx)
    leal JMPBUF(%esp), %ecx    /* Save SP as it will be after we return.  */
    movl %ecx, (JB_SP*4)(%edx)
    movl PCOFF(%esp), %ecx     /* Save PC we are returning to now.  */
    movl %ecx, (JB_PC*4)(%edx)
    movl %ebp, (JB_BP*4)(%edx) /* Save caller's frame pointer.  */
    ret

#define VAL 8

/*
 * void
 * longjmp(jmp_buf env, int value);
 *
 * Parameters:
 *   [ESP+04h] - jmp_buf setup by _setjmp
 *   [ESP+08h] - int     value to return
 * Registers:
 *   None
 * Returns:
 *   Doesn't return
 * Notes:
 *   Non-local goto
 */
.globl _longjmp
_longjmp:
    movl JMPBUF(%esp), %ecx   /* User's jmp_buf in %ecx.  */

    movl VAL(%esp), %eax      /* Second argument is return value.  */
    /* Save the return address now.  */
    movl (JB_PC*4)(%ecx), %edx
    /* Restore registers.  */
    movl (JB_BX*4)(%ecx), %ebx
    movl (JB_SI*4)(%ecx), %esi
    movl (JB_DI*4)(%ecx), %edi
    movl (JB_BP*4)(%ecx), %ebp
    movl (JB_SP*4)(%ecx), %esp
    /* Jump to saved PC.  */
    jmp *%edx
