/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		IP/TCP/UDP checksumming routines
 *
 * Authors:	Jorge Cwik, <jorge@laser.satlink.net>
 *		Arnt Gulbrandsen, <agulbra@nvg.unit.no>
 *		Tom May, <ftom@netcom.com>
 *              Pentium Pro/II routines:
 *              Alexander Kjeldaas <astor@guardian.no>
 *              Finn Arne Gangstad <finnag@guardian.no>
 *		Lots of code moved from tcp.c and ip.c; see those files
 *		for more names.
 *
 * Changes:     Ingo Molnar, converted csum_partial_copy() to 2.1 exception
 *			     handling.
 *		Andi Kleen,  add zeroing on error
 *                   converted to pure assembler
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

/*
 * computes a partial checksum, e.g. for TCP/UDP fragments
 */

/*
unsigned int csum_partial(const unsigned char * buff, int len, unsigned int sum)
 */

#include <asm.inc>

.code
.align 4
PUBLIC _csum_partial

#ifndef CONFIG_X86_USE_PPRO_CHECKSUM

	  /*
	   * Experiments with Ethernet and SLIP connections show that buff
	   * is aligned on either a 2-byte or 4-byte boundary.  We get at
	   * least a twofold speedup on 486 and Pentium if it is 4-byte aligned.
	   * Fortunately, it is easy to convert 2-byte alignment to 4-byte
	   * alignment for the unrolled loop.
	   */
_csum_partial:
	push esi
	push ebx
	mov eax, [esp + 20]	// Function arg: unsigned int sum
	mov ecx, [esp + 16]	// Function arg: int len
	mov esi, [esp + 12]	// Function arg: unsigned char *buff
	test esi, 3	    	// Check alignment.
	jz m2			// Jump if alignment is ok.
	test esi, 1		// Check alignment.
	jz l10			// Jump if alignment is boundary of 2bytes.

	// buf is odd
	dec ecx
	jl l8
	movzx ebx, byte ptr [esi]
	adc eax, ebx
	rol eax, 8
	inc esi
	test esi, 2
	jz m2
l10:
	sub ecx, 2		// Alignment uses up two bytes.
	jae m1			// Jump if we had at least two bytes.
	add ecx, 2		// ecx was < 2.  Deal with it.
	jmp l4
m1:	mov bx, [esi]
	add esi, 2
	add ax, bx
	adc eax, 0
m2:
	mov edx, ecx
	shr ecx, 5
	jz l2
	test esi, esi
l1:	mov ebx, [esi]
	adc eax, ebx
	mov ebx, [esi + 4]
	adc eax, ebx
	mov ebx, [esi + 8]
	adc eax, ebx
	mov ebx, [esi + 12]
	adc eax, ebx
	mov ebx, [esi + 16]
	adc eax, ebx
	mov ebx, [esi + 20]
	adc eax, ebx
	mov ebx, [esi + 24]
	adc eax, ebx
	mov ebx, [esi + 28]
	adc eax, ebx
	lea esi, [esi + 32]
	dec ecx
	jne l1
	adc eax, 0
l2:	mov ecx, edx
	and edx, HEX(1c)
	je l4
	shr edx, 2		// This clears CF
l3:	adc eax, [esi]
	lea esi, [esi + 4]
	dec edx
	jne l3
	adc eax, 0
l4:	and ecx, 3
	jz l7
	cmp ecx, 2
	jb l5
	mov cx, [esi]
	lea esi, [esi + 2]
	je l6
	shl ecx, 16
l5:	mov cl, [esi]
l6:	add eax, ecx
	adc eax, 0
l7:
	test dword ptr [esp + 12], 1
	jz l8
	rol eax, 8
l8:
	pop ebx
	pop esi
	ret

#else

/* Version for PentiumII/PPro */

csum_partial:
	push esi
	push ebx
	mov eax, [esp + 20]	# Function arg: unsigned int sum
	mov ecx, [esp + 16]	# Function arg: int len
	mov esi, [esp + 12]	# Function arg:	const unsigned char *buf

	test esi, 3
	jnz l25f
l10:
	mov edx, ecx
	mov ebx, ecx
	and ebx, HEX(7c)
	shr ecx, 7
	add esi, ebx
	shr ebx, 2
	neg ebx
	lea ebx, l45[ebx + ebx * 2]
	test esi, esi
	jmp dword ptr [ebx]

	// Handle 2-byte-aligned regions
l20: add ax, [esi]
	lea esi, [esi + 2]
	adc eax, 0
	jmp l10b
l25:
	test esi, 1
	jz l30f
	// buf is odd
	dec ecx
	jl l90
	movzb ebx, [esi]
	add eax, ebx
	adc eax, 0
	rol eax, 8
	inc esi
	test esi, 2
	jz l10b

l30: sub ecx, 2
	ja l20
	je l32
	add ecx, 2
	jz l80
	movzb ebx, [esi]	// csumming 1 byte, 2-aligned
	add eax, ebx
	adc eax, 0
	jmp l80
l32:
	add ax, [esi]	// csumming 2 bytes, 2-aligned
	adc eax, 0
	jmp l80

l40:
	add eax, [esi -128]
	adc eax, [esi -124]
	adc eax, [esi -120]
	adc eax, [esi -116]
	adc eax, [esi -112]
	adc eax, [esi -108]
	adc eax, [esi -104]
	adc eax, [esi -100]
	adc eax, [esi -96]
	adc eax, [esi -92]
	adc eax, [esi -88]
	adc eax, [esi -84]
	adc eax, [esi -80]
	adc eax, [esi -76]
	adc eax, [esi -72]
	adc eax, [esi -68]
	adc eax, [esi -64]
	adc eax, [esi -60]
	adc eax, [esi -56]
	adc eax, [esi -52]
	adc eax, [esi -48]
	adc eax, [esi -44]
	adc eax, [esi -40]
	adc eax, [esi -36]
	adc eax, [esi -32]
	adc eax, [esi -28]
	adc eax, [esi -24]
	adc eax, [esi -20]
	adc eax, [esi -16]
	adc eax, [esi -12]
	adc eax, [esi -8]
	adc eax, [esi -4]
l45:
	lea esi, [esi + 128]
	adc eax, 0
	dec ecx
	jge l40
	mov ecx, edx
l50:	and ecx, 3
	jz l80

	// Handle the last 1-3 bytes without jumping
	not ecx		// 1->2, 2->1, 3->0, higher bits are masked
	mov ebx, HEX(ffffff)	// by the shll and shrl instructions
	shl ecx, 3
	shr ebx, cl
	and ebx, [esi -128]	// esi is 4-aligned so should be ok
	add eax, ebx
	adc eax, 0
l80:
	test dword ptr [esp + 12], 1
	jz l90
	rol eax, 8
l90:
	pop ebx
	pop esi
	ret

#endif

END
