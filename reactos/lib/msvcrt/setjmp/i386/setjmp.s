/* $Id: setjmp.s,v 1.1 2003/04/06 12:40:55 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _setjmp/longjmp
 * FILE:              lib/msvcrt/i386/setjmp.s
 * PROGRAMMER:        Ge van Geldorp (ge@gse.nl)
 * NOTES:             Implementation is not complete, see Wine source for a more
 *                    complete implementation
 */

#define JB_BP  0
#define JB_BX  1
#define JB_DI  2
#define JB_SI  3
#define JB_SP  4
#define JB_IP  5

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
    movl %ebp, (JB_BP*4)(%edx) /* Save caller's frame pointer.  */
    movl %ebx, (JB_BX*4)(%edx)
    movl %edi, (JB_DI*4)(%edx)
    movl %esi, (JB_SI*4)(%edx)
    leal JMPBUF(%esp), %ecx    /* Save SP as it will be after we return.  */
    movl %ecx, (JB_SP*4)(%edx)
    movl PCOFF(%esp), %ecx     /* Save PC we are returning to now.  */
    movl %ecx, (JB_IP*4)(%edx)
    ret

/*
 * int
 * _setjmp3(jmp_buf env, int nb_args, ...);
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
.globl __setjmp3
__setjmp3:
    xorl %eax, %eax
    movl JMPBUF(%esp), %edx

    /* Save registers.  */
    movl %ebp, (JB_BP*4)(%edx) /* Save caller's frame pointer.  */
    movl %ebx, (JB_BX*4)(%edx)
    movl %edi, (JB_DI*4)(%edx)
    movl %esi, (JB_SI*4)(%edx)
    leal JMPBUF(%esp), %ecx    /* Save SP as it will be after we return.  */
    movl %ecx, (JB_SP*4)(%edx)
    movl PCOFF(%esp), %ecx     /* Save PC we are returning to now.  */
    movl %ecx, (JB_IP*4)(%edx)
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
    movl (JB_IP*4)(%ecx), %edx
    /* Restore registers.  */
    movl (JB_BP*4)(%ecx), %ebp
    movl (JB_BX*4)(%ecx), %ebx
    movl (JB_DI*4)(%ecx), %edi
    movl (JB_SI*4)(%ecx), %esi
    movl (JB_SP*4)(%ecx), %esp
    /* Jump to saved PC.  */
    jmp *%edx
