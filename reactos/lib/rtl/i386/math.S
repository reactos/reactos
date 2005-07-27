/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/math.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 *                    Eric Kohl (ekohl@rz-online.de)
 * REVISION HISTORY:  27/07/2005 Created
 */
 
/* GLOBALS ****************************************************************/

.globl __ftol
.globl __aullshr
.globl __allrem
.globl __aulldiv
.globl __allshr
.globl __allshl
.globl __aullrem
.globl __allmul
.globl __alldiv

/* FUNCTIONS ***************************************************************/

/*
 * long long
 * __alldiv(long long Dividend, long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - long long Dividend
 *   [ESP+0Ch] - long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long quotient (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
__alldiv:
	call	___divdi3
	ret		$0x10

/*
 * long long
 * __allmul(long long Multiplier, long long Multiplicand);
 *
 * Parameters:
 *   [ESP+04h] - long long Multiplier
 *   [ESP+0Ch] - long long Multiplicand
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long product (Multiplier*Multiplicand)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
__allmul:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$12, %esp
	movl	16(%ebp), %ebx
	movl	8(%ebp), %eax
	mull	%ebx
	movl	20(%ebp), %ecx
	movl	%eax, -24(%ebp)
	movl	8(%ebp), %eax
	movl	%edx, %esi
	imull	%ecx, %eax
	addl	%eax, %esi
	movl	12(%ebp), %eax
	imull	%eax, %ebx
	leal	(%ebx,%esi), %eax
	movl	%eax, -20(%ebp)
	movl	-24(%ebp), %eax
	movl	-20(%ebp), %edx
	addl	$12, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret		$0x10

/*
 * unsigned long long
 * __aullrem(unsigned long long Dividend, unsigned long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - unsigned long long Dividend
 *   [ESP+0Ch] - unsigned long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - unsigned long long remainder (Dividend%Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
__aullrem:
	call	___umoddi3
	ret	$16

/*
 * long long
 * __allshl(long long Value, unsigned char Shift);
 *
 * Parameters:
 *   EDX:EAX - signed long long value to be shifted left
 *   CL      - number of bits to shift by
 * Registers:
 *   Destroys CL
 * Returns:
 *   EDX:EAX - shifted value
 */
__allshl:
	shldl	%cl, %eax, %edx
	sall	%cl, %eax
	andl	$32, %ecx
	je		1f
	movl	%eax, %edx
	xorl	%eax, %eax
1:
	ret

/*
 * long long
 * __allshr(long long Value, unsigned char Shift);
 *
 * Parameters:
 *   EDX:EAX - signed long long value to be shifted right
 *   CL      - number of bits to shift by
 * Registers:
 *   Destroys CL
 * Returns:
 *   EDX:EAX - shifted value
 */
__allshr:
	shrdl	%cl, %edx, %eax
	sarl	%cl, %edx
	andl	$32, %ecx
	je		1f
	movl	%edx, %eax
	sarl	$31, %edx
1:
	ret

/*
 * unsigned long long
 * __aulldiv(unsigned long long Dividend, unsigned long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - unsigned long long Dividend
 *   [ESP+0Ch] - unsigned long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - unsigned long long quotient (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
__aulldiv:
	call	___udivdi3
	ret	$16

/*
 * unsigned long long
 * __aullshr(unsigned long long Value, unsigned char Shift);
 *
 * Parameters:
 *   EDX:EAX - unsigned long long value to be shifted right
 *   CL      - number of bits to shift by
 * Registers:
 *   Destroys CL
 * Returns:
 *   EDX:EAX - shifted value
 */
__aullshr:
	shrdl	%cl, %edx, %eax
	shrl	%cl, %edx
	andl	$32, %ecx
	je		1f
	movl	%edx, %eax
1:
	ret
	
/*
 * long long
 * __allrem(long long Dividend, long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - long long Dividend
 *   [ESP+0Ch] - long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long remainder (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
__allrem:
	call	___moddi3
	ret		$16
	
.intel_syntax noprefix

__ftol:
    /* Set up stack frame */
    push ebp
    mov ebp, esp
    
    /* Set "round towards zero" mode */
    fstcw [ebp-2]
    wait
    mov ax, [ebp-2]
    or ah, 0xC
    fldcw [ebp-4]
    
    /* Do the conversion */
    fistp qword ptr [ebp-8]
    
    /* Restore rounding mode */
    fldcw [ebp-2]
    
    /* Return value */
    mov eax, [ebp-12]
    mov edx, [ebp-8]
    
    /* Remove stack frame and return*/
    leave
    ret
