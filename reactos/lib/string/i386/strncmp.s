/* 
 * $Id: strncmp.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * int *strncmp (const char* s1, const char* s2, size_t n)
 */

.globl	_strncmp

_strncmp:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%esi		// s1
	mov	0xc(%ebp),%edi          // s2
	mov	0x10(%ebp),%ecx         // n
	xor	%eax,%eax
	cld
.L1:
	dec	%ecx
	js	.L2
	lodsb
	scasb
	jne	.L3
	test	%eax,%eax		// bit 8..31 are set to 0
	jne	.L1
.L2:	
	xor	%eax,%eax
	jmp	.L4
.L3:
	sbb	%eax,%eax
	or	$1,%al		
.L4:
	pop	%edi
	pop	%esi
	leave
	ret

