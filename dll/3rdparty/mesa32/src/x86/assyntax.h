
#ifndef __ASSYNTAX_H__
#define __ASSYNTAX_H__

/*
 * Copyright 1992 Vrije Universiteit, The Netherlands
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the Vrije Universiteit not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The Vrije Universiteit makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * The Vrije Universiteit DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL The Vrije Universiteit BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * assyntax.h
 *
 * Select the syntax appropriate to the 386 assembler being used
 * To add support for more assemblers add more columns to the CHOICE
 * macro.  Note that register names must also have uppercase names
 * to avoid macro recursion. e.g., #define ah %ah recurses!
 *
 * NB 1.  Some of the macros for certain assemblers imply that the code is to
 *	  run in protected mode!!  Caveat emptor.
 *
 * NB 2.  486 specific instructions are not included.  This is to discourage
 *	  their accidental use in code that is intended to run on 386 and 486
 *	  systems.
 *
 * Supported assemblers:
 *
 * (a) AT&T SysVr4 as(1):	define ATT_ASSEMBLER
 * (b) GNU Assembler gas:	define GNU_ASSEMBLER (default)
 * (c) Amsterdam Compiler kit:	define ACK_ASSEMBLER
 * (d) The Netwide Assembler:	define NASM_ASSEMBLER
 * (e) Microsoft Assembler:	define MASM_ASSEMBLER (UNTESTED!)
 *
 * The following naming conventions have been used to identify the various
 * data types:
 *		_SR = segment register version
 *	Integer:
 *		_Q = quadword	= 64 bits
 *		_L = long	= 32 bits
 *		_W = short	= 16 bits
 *		_B = byte	=  8 bits
 *	Floating-point:
 *		_X = m80real	= 80 bits
 *		_D = double	= 64 bits
 *		_S = single	= 32 bits
 *
 * Author: Gregory J. Sharp, Sept 1992
 *         Vrije Universiteit, Amsterdam, The Netherlands
 *
 *         [support for Intel syntax added by Josh Vanderhoof, 1999]
 */

#if !(defined(NASM_ASSEMBLER) || defined(MASM_ASSEMBLER))

/* Default to ATT_ASSEMBLER when SVR4 or SYSV are defined */
#if (defined(SVR4) || defined(SYSV)) && !defined(GNU_ASSEMBLER)
#define ATT_ASSEMBLER
#endif

#if !defined(ATT_ASSEMBLER) && !defined(GNU_ASSEMBLER) && !defined(ACK_ASSEMBLER)
#define GNU_ASSEMBLER
#endif

#if (defined(__STDC__) && !defined(UNIXCPP)) || (defined (sun) && defined (i386) && defined (SVR4) && defined (__STDC__) && !defined (__GNUC__))
#define CONCAT(x, y)		x ## y
#define CONCAT3(x, y, z)	x ## y ## z
#else
#define CONCAT(x, y)		x/**/y
#define CONCAT3(x, y, z)	x/**/y/**/z
#endif

#ifdef ACK_ASSEMBLER

/* Assume we write code for 32-bit protected mode! */

/* Redefine register names for GAS & AT&T assemblers */
#define AL		al
#define AH		ah
#define AX		ax
#define EAX		ax
#define BL		bl
#define BH		bh
#define BX		bx
#define EBX		bx
#define CL		cl
#define CH		ch
#define CX		cx
#define ECX		cx
#define DL		dl
#define DH		dh
#define DX		dx
#define EDX		dx
#define BP		bp
#define EBP		bp
#define SI		si
#define ESI		si
#define DI		di
#define EDI		di
#define SP		sp
#define ESP		sp
#define CS		cs
#define SS		ss
#define DS		ds
#define ES		es
#define FS		fs
#define GS		gs
/* Control Registers */
#define CR0		cr0
#define CR1		cr1
#define CR2		cr2
#define CR3		cr3
/* Debug Registers */
#define DR0		dr0
#define DR1		dr1
#define DR2		dr2
#define DR3		dr3
#define DR4		dr4
#define DR5		dr5
#define DR6		dr6
#define DR7		dr7
/* Floating-point Stack */
#define ST		st

#define AS_BEGIN	.sect .text; .sect .rom; .sect .data; .sect .bss; .sect .text


#define _WTOG		o16	/* word toggle for _W instructions */
#define _LTOG			/* long toggle for _L instructions */
#define ADDR_TOGGLE	a16
#define OPSZ_TOGGLE	o16
#define USE16		.use16
#define USE32		.use32

#define CHOICE(a,b,c)	c

#else /* AT&T or GAS */

/* Redefine register names for GAS & AT&T assemblers */
#define AL		%al
#define AH		%ah
#define AX		%ax
#define EAX		%eax
#define BL		%bl
#define BH		%bh
#define BX		%bx
#define EBX		%ebx
#define CL		%cl
#define CH		%ch
#define CX		%cx
#define ECX		%ecx
#define DL		%dl
#define DH		%dh
#define DX		%dx
#define EDX		%edx
#define BP		%bp
#define EBP		%ebp
#define SI		%si
#define ESI		%esi
#define DI		%di
#define EDI		%edi
#define SP		%sp
#define ESP		%esp
#define CS		%cs
#define SS		%ss
#define DS		%ds
#define ES		%es
#define FS		%fs
#define GS		%gs
/* Control Registers */
#define CR0		%cr0
#define CR1		%cr1
#define CR2		%cr2
#define CR3		%cr3
/* Debug Registers */
#define DR0		%db0
#define DR1		%db1
#define DR2		%db2
#define DR3		%db3
#define DR4		%db4
#define DR5		%db5
#define DR6		%db6
#define DR7		%db7
/* Floating-point Stack */
#define _STX0		%st(0)
#define _STX1		%st(1)
#define _STX2		%st(2)
#define _STX3		%st(3)
#define _STX4		%st(4)
#define _STX5		%st(5)
#define _STX6		%st(6)
#define _STX7		%st(7)
#define ST(x)		CONCAT(_STX,x)
#ifdef GNU_ASSEMBLER
#define ST0		%st(0)
#else
#define ST0		%st
#endif
/* MMX Registers */
#define MM0		%mm0
#define MM1		%mm1
#define MM2		%mm2
#define MM3		%mm3
#define MM4		%mm4
#define MM5		%mm5
#define MM6		%mm6
#define MM7		%mm7
/* SSE Registers */
#define XMM0		%xmm0
#define XMM1		%xmm1
#define XMM2		%xmm2
#define XMM3		%xmm3
#define XMM4		%xmm4
#define XMM5		%xmm5
#define XMM6		%xmm6
#define XMM7		%xmm7

#define AS_BEGIN
#define USE16
#define USE32

#ifdef GNU_ASSEMBLER

#define ADDR_TOGGLE	aword
#define OPSZ_TOGGLE	word

#define CHOICE(a,b,c)	b

#else
/*
 * AT&T ASSEMBLER SYNTAX
 * *********************
 */
#define CHOICE(a,b,c)	a

#define ADDR_TOGGLE	addr16
#define OPSZ_TOGGLE	data16

#endif /* GNU_ASSEMBLER */
#endif /* ACK_ASSEMBLER */


#if defined(__QNX__) || defined(Lynx) || (defined(SYSV) || defined(SVR4)) && !defined(ACK_ASSEMBLER) || defined(__ELF__) || defined(__GNU__) || defined(__GNUC__) && !defined(__DJGPP__) && !defined(__MINGW32__)
#define GLNAME(a)	a
#else
#define GLNAME(a)	CONCAT(_,a)
#endif


	/****************************************/
	/*					*/
	/*	Select the various choices	*/
	/*					*/
	/****************************************/


/* Redefine assembler directives */
/*********************************/
#define GLOBL		CHOICE(.globl, .globl, .extern)
#define GLOBAL		GLOBL
#define EXTERN		GLOBL
#ifndef __AOUT__
#define ALIGNTEXT32	CHOICE(.align 32, .balign 32, .align 32)
#define ALIGNTEXT16	CHOICE(.align 16, .balign 16, .align 16)
#define ALIGNTEXT8	CHOICE(.align 8, .balign 8, .align 8)
#define ALIGNTEXT4	CHOICE(.align 4, .balign 4, .align 4)
#define ALIGNTEXT2	CHOICE(.align 2, .balign 2, .align 2)
/* ALIGNTEXT4ifNOP is the same as ALIGNTEXT4, but only if the space is
 * guaranteed to be filled with NOPs.  Otherwise it does nothing.
 */
#define ALIGNTEXT32ifNOP	CHOICE(.align 32, .balign ARG2(32,0x90), /*can't do it*/)
#define ALIGNTEXT16ifNOP	CHOICE(.align 16, .balign ARG2(16,0x90), /*can't do it*/)
#define ALIGNTEXT8ifNOP	CHOICE(.align 8, .balign ARG2(8,0x90), /*can't do it*/)
#define ALIGNTEXT4ifNOP	CHOICE(.align 4, .balign ARG2(4,0x90), /*can't do it*/)
#define ALIGNDATA32	CHOICE(.align 32, .balign ARG2(32,0x0), .align 32)
#define ALIGNDATA16	CHOICE(.align 16, .balign ARG2(16,0x0), .align 16)
#define ALIGNDATA8	CHOICE(.align 8, .balign ARG2(8,0x0), .align 8)
#define ALIGNDATA4	CHOICE(.align 4, .balign ARG2(4,0x0), .align 4)
#define ALIGNDATA2	CHOICE(.align 2, .balign ARG2(2,0x0), .align 2)
#else
/* 'as -aout' on FreeBSD doesn't have .balign */
#define ALIGNTEXT32	CHOICE(.align 32, .align ARG2(5,0x90), .align 32)
#define ALIGNTEXT16	CHOICE(.align 16, .align ARG2(4,0x90), .align 16)
#define ALIGNTEXT8	CHOICE(.align 8, .align ARG2(3,0x90), .align 8)
#define ALIGNTEXT4	CHOICE(.align 4, .align ARG2(2,0x90), .align 4)
#define ALIGNTEXT2	CHOICE(.align 2, .align ARG2(1,0x90), .align 2)
/* ALIGNTEXT4ifNOP is the same as ALIGNTEXT4, but only if the space is
 * guaranteed to be filled with NOPs.  Otherwise it does nothing.
 */
#define ALIGNTEXT32ifNOP	CHOICE(.align 32, .align ARG2(5,0x90), /*can't do it*/)
#define ALIGNTEXT16ifNOP	CHOICE(.align 16, .align ARG2(4,0x90), /*can't do it*/)
#define ALIGNTEXT8ifNOP	CHOICE(.align 8, .align ARG2(3,0x90), /*can't do it*/)
#define ALIGNTEXT4ifNOP	CHOICE(.align 4, .align ARG2(2,0x90), /*can't do it*/)
#define ALIGNDATA32	CHOICE(.align 32, .align ARG2(5,0x0), .align 32)
#define ALIGNDATA16	CHOICE(.align 16, .align ARG2(4,0x0), .align 16)
#define ALIGNDATA8	CHOICE(.align 8, .align ARG2(3,0x0), .align 8)
#define ALIGNDATA4	CHOICE(.align 4, .align ARG2(2,0x0), .align 4)
#define ALIGNDATA2	CHOICE(.align 2, .align ARG2(1,0x0), .align 2)
#endif /* __AOUT__ */
#define FILE(s)		CHOICE(.file s, .file s, .file s)
#define STRING(s)	CHOICE(.string s, .asciz s, .asciz s)
#define D_LONG		CHOICE(.long, .long, .data4)
#define D_WORD		CHOICE(.value, .short, .data2)
#define D_BYTE		CHOICE(.byte, .byte, .data1)
#define SPACE		CHOICE(.comm, .space, .space)
#define COMM		CHOICE(.comm, .comm, .comm)
#define SEG_DATA	CHOICE(.data, .data, .sect .data)
#define SEG_TEXT	CHOICE(.text, .text, .sect .text)
#define SEG_BSS		CHOICE(.bss, .bss, .sect .bss)

#ifdef GNU_ASSEMBLER
#define D_SPACE(n)	. = . + n
#else
#define D_SPACE(n)	.space n
#endif

/* Addressing Modes */
/* Immediate Mode */
#define ADDR(a)		CHOICE(CONCAT($,a), $a, a)
#define CONST(a)	CHOICE(CONCAT($,a), $a, a)

/* Indirect Mode */
#define CONTENT(a)	CHOICE(a, a, (a))	 /* take contents of variable */
#define REGIND(a)	CHOICE((a), (a), (a))	 /* Register a indirect */
/* Register b indirect plus displacement a */
#define REGOFF(a, b)	CHOICE(a(b), a(b), a(b))
/* Reg indirect Base + Index + Displacement  - this is mainly for 16-bit mode
 * which has no scaling
 */
#define REGBID(b,i,d)	CHOICE(d(b,i), d(b,i), d(b)(i))
/* Reg indirect Base + (Index * Scale) */
#define REGBIS(b,i,s)	CHOICE((b,i,s), (b,i,s), (b)(i*s))
/* Reg indirect Base + (Index * Scale) + Displacement */
#define REGBISD(b,i,s,d) CHOICE(d(b,i,s), d(b,i,s), d(b)(i*s))
/* Displaced Scaled Index: */
#define REGDIS(d,i,s)	CHOICE(d(,i,s), d(,i,s), d(i * s))
/* Indexed Base: */
#define REGBI(b,i)	CHOICE((b,i), (b,i), (b)(i))
/* Displaced Base: */
#define REGDB(d,b)	CHOICE(d(b), d(b), d(b))
/* Variable indirect: */
#define VARINDIRECT(var) CHOICE(*var, *var, (var))
/* Use register contents as jump/call target: */
#define CODEPTR(reg)	CHOICE(*reg, *reg, reg)

/* For expressions requiring bracketing
 * eg. (CRT0_PM | CRT_EM)
 */

#define EXPR(a)		CHOICE([a], (a), [a])
#define ENOT(a)		CHOICE(0!a, ~a, ~a)
#define EMUL(a,b)	CHOICE(a\*b, a*b, a*b)
#define EDIV(a,b)	CHOICE(a\/b, a/b, a/b)

/*
 * We have to beat the problem of commas within arguments to choice.
 * eg. choice (add a,b, add b,a) will get argument mismatch.  Luckily ANSI
 * and other known cpp definitions evaluate arguments before substitution
 * so the following works.
 */
#define ARG2(a, b)	a,b
#define ARG3(a,b,c)	a,b,c

/* Redefine assembler commands */
#define AAA		CHOICE(aaa, aaa, aaa)
#define AAD		CHOICE(aad, aad, aad)
#define AAM		CHOICE(aam, aam, aam)
#define AAS		CHOICE(aas, aas, aas)
#define ADC_L(a, b)	CHOICE(adcl ARG2(a,b), adcl ARG2(a,b), _LTOG adc ARG2(b,a))
#define ADC_W(a, b)	CHOICE(adcw ARG2(a,b), adcw ARG2(a,b), _WTOG adc ARG2(b,a))
#define ADC_B(a, b)	CHOICE(adcb ARG2(a,b), adcb ARG2(a,b), adcb ARG2(b,a))
#define ADD_L(a, b)	CHOICE(addl ARG2(a,b), addl ARG2(a,b), _LTOG add ARG2(b,a))
#define ADD_W(a, b)	CHOICE(addw ARG2(a,b), addw ARG2(a,b), _WTOG add ARG2(b,a))
#define ADD_B(a, b)	CHOICE(addb ARG2(a,b), addb ARG2(a,b), addb ARG2(b,a))
#define AND_L(a, b)	CHOICE(andl ARG2(a,b), andl ARG2(a,b), _LTOG and ARG2(b,a))
#define AND_W(a, b)	CHOICE(andw ARG2(a,b), andw ARG2(a,b), _WTOG and ARG2(b,a))
#define AND_B(a, b)	CHOICE(andb ARG2(a,b), andb ARG2(a,b), andb ARG2(b,a))
#define ARPL(a,b)	CHOICE(arpl ARG2(a,b), arpl ARG2(a,b), arpl ARG2(b,a))
#define BOUND_L(a, b)	CHOICE(boundl ARG2(a,b), boundl ARG2(b,a), _LTOG bound ARG2(b,a))
#define BOUND_W(a, b)	CHOICE(boundw ARG2(a,b), boundw ARG2(b,a), _WTOG bound ARG2(b,a))
#define BSF_L(a, b)	CHOICE(bsfl ARG2(a,b), bsfl ARG2(a,b), _LTOG bsf ARG2(b,a))
#define BSF_W(a, b)	CHOICE(bsfw ARG2(a,b), bsfw ARG2(a,b), _WTOG bsf ARG2(b,a))
#define BSR_L(a, b)	CHOICE(bsrl ARG2(a,b), bsrl ARG2(a,b), _LTOG bsr ARG2(b,a))
#define BSR_W(a, b)	CHOICE(bsrw ARG2(a,b), bsrw ARG2(a,b), _WTOG bsr ARG2(b,a))
#define BT_L(a, b)	CHOICE(btl ARG2(a,b), btl ARG2(a,b), _LTOG bt ARG2(b,a))
#define BT_W(a, b)	CHOICE(btw ARG2(a,b), btw ARG2(a,b), _WTOG bt ARG2(b,a))
#define BTC_L(a, b)	CHOICE(btcl ARG2(a,b), btcl ARG2(a,b), _LTOG btc ARG2(b,a))
#define BTC_W(a, b)	CHOICE(btcw ARG2(a,b), btcw ARG2(a,b), _WTOG btc ARG2(b,a))
#define BTR_L(a, b)	CHOICE(btrl ARG2(a,b), btrl ARG2(a,b), _LTOG btr ARG2(b,a))
#define BTR_W(a, b)	CHOICE(btrw ARG2(a,b), btrw ARG2(a,b), _WTOG btr ARG2(b,a))
#define BTS_L(a, b)	CHOICE(btsl ARG2(a,b), btsl ARG2(a,b), _LTOG bts ARG2(b,a))
#define BTS_W(a, b)	CHOICE(btsw ARG2(a,b), btsw ARG2(a,b), _WTOG bts ARG2(b,a))
#define CALL(a)		CHOICE(call a, call a, call a)
#define CALLF(s,a)	CHOICE(lcall ARG2(s,a), lcall ARG2(s,a), callf s:a)
#define CBW		CHOICE(cbtw, cbw, cbw)
#define CWDE		CHOICE(cwtd, cwde, cwde)
#define CLC		CHOICE(clc, clc, clc)
#define CLD		CHOICE(cld, cld, cld)
#define CLI		CHOICE(cli, cli, cli)
#define CLTS		CHOICE(clts, clts, clts)
#define CMC		CHOICE(cmc, cmc, cmc)
#define CMP_L(a, b)	CHOICE(cmpl ARG2(a,b), cmpl ARG2(a,b), _LTOG cmp ARG2(b,a))
#define CMP_W(a, b)	CHOICE(cmpw ARG2(a,b), cmpw ARG2(a,b), _WTOG cmp ARG2(b,a))
#define CMP_B(a, b)	CHOICE(cmpb ARG2(a,b), cmpb ARG2(a,b), cmpb ARG2(b,a))
#define CMPS_L		CHOICE(cmpsl, cmpsl, _LTOG cmps)
#define CMPS_W		CHOICE(cmpsw, cmpsw, _WTOG cmps)
#define CMPS_B		CHOICE(cmpsb, cmpsb, cmpsb)
#define CWD		CHOICE(cwtl, cwd, cwd)
#define CDQ		CHOICE(cltd, cdq, cdq)
#define DAA		CHOICE(daa, daa, daa)
#define DAS		CHOICE(das, das, das)
#define DEC_L(a)	CHOICE(decl a, decl a, _LTOG dec a)
#define DEC_W(a)	CHOICE(decw a, decw a, _WTOG dec a)
#define DEC_B(a)	CHOICE(decb a, decb a, decb a)
#define DIV_L(a)	CHOICE(divl a, divl a, div a)
#define DIV_W(a)	CHOICE(divw a, divw a, div a)
#define DIV_B(a)	CHOICE(divb a, divb a, divb a)
#define ENTER(a,b)	CHOICE(enter ARG2(a,b), enter ARG2(a,b), enter ARG2(b,a))
#define HLT		CHOICE(hlt, hlt, hlt)
#define IDIV_L(a)	CHOICE(idivl a, idivl a, _LTOG idiv a)
#define IDIV_W(a)	CHOICE(idivw a, idivw a, _WTOG idiv a)
#define IDIV_B(a)	CHOICE(idivb a, idivb a, idivb a)
/* More forms than this for imul!! */
#define IMUL_L(a, b)	CHOICE(imull ARG2(a,b), imull ARG2(a,b), _LTOG imul ARG2(b,a))
#define IMUL_W(a, b)	CHOICE(imulw ARG2(a,b), imulw ARG2(a,b), _WTOG imul ARG2(b,a))
#define IMUL_B(a)	CHOICE(imulb a, imulb a, imulb a)
#define IN_L		CHOICE(inl (DX), inl ARG2(DX,EAX), _LTOG in DX)
#define IN_W		CHOICE(inw (DX), inw ARG2(DX,AX), _WTOG in DX)
#define IN_B		CHOICE(inb (DX), inb ARG2(DX,AL), inb DX)
/* Please AS code writer: use the following ONLY, if you refer to ports<256
 * directly, but not in IN1_W(DX), for instance, even if IN1_ looks nicer
 */
#if defined (sun)
#define IN1_L(a)	CHOICE(inl (a), inl ARG2(a,EAX), _LTOG in a)
#define IN1_W(a)	CHOICE(inw (a), inw ARG2(a,AX), _WTOG in a)
#define IN1_B(a)	CHOICE(inb (a), inb ARG2(a,AL), inb a)
#else
#define IN1_L(a)	CHOICE(inl a, inl ARG2(a,EAX), _LTOG in a)
#define IN1_W(a)	CHOICE(inw a, inw ARG2(a,AX), _WTOG in a)
#define IN1_B(a)	CHOICE(inb a, inb ARG2(a,AL), inb a)
#endif
#define INC_L(a)	CHOICE(incl a, incl a, _LTOG inc a)
#define INC_W(a)	CHOICE(incw a, incw a, _WTOG inc a)
#define INC_B(a)	CHOICE(incb a, incb a, incb a)
#define INS_L		CHOICE(insl, insl, _LTOG ins)
#define INS_W		CHOICE(insw, insw, _WTOG ins)
#define INS_B		CHOICE(insb, insb, insb)
#define INT(a)		CHOICE(int a, int a, int a)
#define INT3		CHOICE(int CONST(3), int3, int CONST(3))
#define INTO		CHOICE(into, into, into)
#define IRET		CHOICE(iret, iret, iret)
#define IRETD		CHOICE(iret, iret, iretd)
#define JA(a)		CHOICE(ja a, ja a, ja a)
#define JAE(a)		CHOICE(jae a, jae a, jae a)
#define JB(a)		CHOICE(jb a, jb a, jb a)
#define JBE(a)		CHOICE(jbe a, jbe a, jbe a)
#define JC(a)		CHOICE(jc a, jc a, jc a)
#define JE(a)		CHOICE(je a, je a, je a)
#define JG(a)		CHOICE(jg a, jg a, jg a)
#define JGE(a)		CHOICE(jge a, jge a, jge a)
#define JL(a)		CHOICE(jl a, jl a, jl a)
#define JLE(a)		CHOICE(jle a, jle a, jle a)
#define JNA(a)		CHOICE(jna a, jna a, jna a)
#define JNAE(a)		CHOICE(jnae a, jnae a, jnae a)
#define JNB(a)		CHOICE(jnb a, jnb a, jnb a)
#define JNBE(a)		CHOICE(jnbe a, jnbe a, jnbe a)
#define JNC(a)		CHOICE(jnc a, jnc a, jnc a)
#define JNE(a)		CHOICE(jne a, jne a, jne a)
#define JNG(a)		CHOICE(jng a, jng a, jng a)
#define JNGE(a)		CHOICE(jnge a, jnge a, jnge a)
#define JNL(a)		CHOICE(jnl a, jnl a, jnl a)
#define JNLE(a)		CHOICE(jnle a, jnle a, jnle a)
#define JNO(a)		CHOICE(jno a, jno a, jno a)
#define JNP(a)		CHOICE(jnp a, jnp a, jnp a)
#define JNS(a)		CHOICE(jns a, jns a, jns a)
#define JNZ(a)		CHOICE(jnz a, jnz a, jnz a)
#define JO(a)		CHOICE(jo a, jo a, jo a)
#define JP(a)		CHOICE(jp a, jp a, jp a)
#define JPE(a)		CHOICE(jpe a, jpe a, jpe a)
#define JPO(a)		CHOICE(jpo a, jpo a, jpo a)
#define JS(a)		CHOICE(js a, js a, js a)
#define JZ(a)		CHOICE(jz a, jz a, jz a)
#define JMP(a)		CHOICE(jmp a, jmp a, jmp a)
#define JMPF(s,a)	CHOICE(ljmp ARG2(s,a), ljmp ARG2(s,a), jmpf s:a)
#define LAHF		CHOICE(lahf, lahf, lahf)
#if !defined(_REAL_MODE) && !defined(_V86_MODE)
#define LAR(a, b)	CHOICE(lar ARG2(a, b), lar ARG2(a, b), lar ARG2(b, a))
#endif
#define LEA_L(a, b)	CHOICE(leal ARG2(a,b), leal ARG2(a,b), _LTOG lea ARG2(b,a))
#define LEA_W(a, b)	CHOICE(leaw ARG2(a,b), leaw ARG2(a,b), _WTOG lea ARG2(b,a))
#define LEAVE		CHOICE(leave, leave, leave)
#define LGDT(a)		CHOICE(lgdt a, lgdt a, lgdt a)
#define LIDT(a)		CHOICE(lidt a, lidt a, lidt a)
#define LDS(a, b)	CHOICE(ldsl ARG2(a,b), lds ARG2(a,b), lds ARG2(b,a))
#define LES(a, b)	CHOICE(lesl ARG2(a,b), les ARG2(a,b), les ARG2(b,a))
#define LFS(a, b)	CHOICE(lfsl ARG2(a,b), lfs ARG2(a,b), lfs ARG2(b,a))
#define LGS(a, b)	CHOICE(lgsl ARG2(a,b), lgs ARG2(a,b), lgs ARG2(b,a))
#define LSS(a, b)	CHOICE(lssl ARG2(a,b), lss ARG2(a,b), lss ARG2(b,a))
#define LLDT(a)		CHOICE(lldt a, lldt a, lldt a)
#define LMSW(a)		CHOICE(lmsw a, lmsw a, lmsw a)
#define LOCK		CHOICE(lock, lock, lock)
#define LODS_L		CHOICE(lodsl, lodsl, _LTOG lods)
#define LODS_W		CHOICE(lodsw, lodsw, _WTOG lods)
#define LODS_B		CHOICE(lodsb, lodsb, lodsb)
#define LOOP(a)		CHOICE(loop a, loop a, loop a)
#define LOOPE(a)	CHOICE(loope a, loope a, loope a)
#define LOOPZ(a)	CHOICE(loopz a, loopz a, loopz a)
#define LOOPNE(a)	CHOICE(loopne a, loopne a, loopne a)
#define LOOPNZ(a)	CHOICE(loopnz a, loopnz a, loopnz a)
#if !defined(_REAL_MODE) && !defined(_V86_MODE)
#define LSL(a, b)	CHOICE(lsl ARG2(a,b), lsl ARG2(a,b), lsl ARG2(b,a))
#endif
#define LTR(a)		CHOICE(ltr a, ltr a, ltr a)
#define MOV_SR(a, b)	CHOICE(movw ARG2(a,b), mov ARG2(a,b), mov ARG2(b,a))
#define MOV_L(a, b)	CHOICE(movl ARG2(a,b), movl ARG2(a,b), _LTOG mov ARG2(b,a))
#define MOV_W(a, b)	CHOICE(movw ARG2(a,b), movw ARG2(a,b), _WTOG mov ARG2(b,a))
#define MOV_B(a, b)	CHOICE(movb ARG2(a,b), movb ARG2(a,b), movb ARG2(b,a))
#define MOVS_L		CHOICE(movsl, movsl, _LTOG movs)
#define MOVS_W		CHOICE(movsw, movsw, _WTOG movs)
#define MOVS_B		CHOICE(movsb, movsb, movsb)
#define MOVSX_BL(a, b)	CHOICE(movsbl ARG2(a,b), movsbl ARG2(a,b), movsx ARG2(b,a))
#define MOVSX_BW(a, b)	CHOICE(movsbw ARG2(a,b), movsbw ARG2(a,b), movsx ARG2(b,a))
#define MOVSX_WL(a, b)	CHOICE(movswl ARG2(a,b), movswl ARG2(a,b), movsx ARG2(b,a))
#define MOVZX_BL(a, b)	CHOICE(movzbl ARG2(a,b), movzbl ARG2(a,b), movzx ARG2(b,a))
#define MOVZX_BW(a, b)	CHOICE(movzbw ARG2(a,b), movzbw ARG2(a,b), movzx ARG2(b,a))
#define MOVZX_WL(a, b)	CHOICE(movzwl ARG2(a,b), movzwl ARG2(a,b), movzx ARG2(b,a))
#define MUL_L(a)	CHOICE(mull a, mull a, _LTOG mul a)
#define MUL_W(a)	CHOICE(mulw a, mulw a, _WTOG mul a)
#define MUL_B(a)	CHOICE(mulb a, mulb a, mulb a)
#define NEG_L(a)	CHOICE(negl a, negl a, _LTOG neg a)
#define NEG_W(a)	CHOICE(negw a, negw a, _WTOG neg a)
#define NEG_B(a)	CHOICE(negb a, negb a, negb a)
#define NOP		CHOICE(nop, nop, nop)
#define NOT_L(a)	CHOICE(notl a, notl a, _LTOG not a)
#define NOT_W(a)	CHOICE(notw a, notw a, _WTOG not a)
#define NOT_B(a)	CHOICE(notb a, notb a, notb a)
#define OR_L(a,b)	CHOICE(orl ARG2(a,b), orl ARG2(a,b), _LTOG or ARG2(b,a))
#define OR_W(a,b)	CHOICE(orw ARG2(a,b), orw ARG2(a,b), _WTOG or ARG2(b,a))
#define OR_B(a,b)	CHOICE(orb ARG2(a,b), orb ARG2(a,b), orb ARG2(b,a))
#define OUT_L		CHOICE(outl (DX), outl ARG2(EAX,DX), _LTOG out DX)
#define OUT_W		CHOICE(outw (DX), outw ARG2(AX,DX), _WTOG out DX)
#define OUT_B		CHOICE(outb (DX), outb ARG2(AL,DX), outb DX)
/* Please AS code writer: use the following ONLY, if you refer to ports<256
 * directly, but not in OUT1_W(DX), for instance, even if OUT1_ looks nicer
 */
#define OUT1_L(a)	CHOICE(outl (a), outl ARG2(EAX,a), _LTOG out a)
#define OUT1_W(a)	CHOICE(outw (a), outw ARG2(AX,a), _WTOG out a)
#define OUT1_B(a)	CHOICE(outb (a), outb ARG2(AL,a), outb a)
#define OUTS_L		CHOICE(outsl, outsl, _LTOG outs)
#define OUTS_W		CHOICE(outsw, outsw, _WTOG outs)
#define OUTS_B		CHOICE(outsb, outsb, outsb)
#define POP_SR(a)	CHOICE(pop a, pop a, pop a)
#define POP_L(a)	CHOICE(popl a, popl a, _LTOG pop a)
#define POP_W(a)	CHOICE(popw a, popw a, _WTOG pop a)
#define POPA_L		CHOICE(popal, popal, _LTOG popa)
#define POPA_W		CHOICE(popaw, popaw, _WTOG popa)
#define POPF_L		CHOICE(popfl, popfl, _LTOG popf)
#define POPF_W		CHOICE(popfw, popfw, _WTOG popf)
#define PUSH_SR(a)	CHOICE(push a, push a, push a)
#define PUSH_L(a)	CHOICE(pushl a, pushl a, _LTOG push a)
#define PUSH_W(a)	CHOICE(pushw a, pushw a, _WTOG push a)
#define PUSH_B(a)	CHOICE(push a, pushb a, push a)
#define PUSHA_L		CHOICE(pushal, pushal, _LTOG pusha)
#define PUSHA_W		CHOICE(pushaw, pushaw, _WTOG pusha)
#define PUSHF_L		CHOICE(pushfl, pushfl, _LTOG pushf)
#define PUSHF_W		CHOICE(pushfw, pushfw, _WTOG pushf)
#define RCL_L(a, b)	CHOICE(rcll ARG2(a,b), rcll ARG2(a,b), _LTOG rcl ARG2(b,a))
#define RCL_W(a, b)	CHOICE(rclw ARG2(a,b), rclw ARG2(a,b), _WTOG rcl ARG2(b,a))
#define RCL_B(a, b)	CHOICE(rclb ARG2(a,b), rclb ARG2(a,b), rclb ARG2(b,a))
#define RCR_L(a, b)	CHOICE(rcrl ARG2(a,b), rcrl ARG2(a,b), _LTOG rcr ARG2(b,a))
#define RCR_W(a, b)	CHOICE(rcrw ARG2(a,b), rcrw ARG2(a,b), _WTOG rcr ARG2(b,a))
#define RCR_B(a, b)	CHOICE(rcrb ARG2(a,b), rcrb ARG2(a,b), rcrb ARG2(b,a))
#define ROL_L(a, b)	CHOICE(roll ARG2(a,b), roll ARG2(a,b), _LTOG rol ARG2(b,a))
#define ROL_W(a, b)	CHOICE(rolw ARG2(a,b), rolw ARG2(a,b), _WTOG rol ARG2(b,a))
#define ROL_B(a, b)	CHOICE(rolb ARG2(a,b), rolb ARG2(a,b), rolb ARG2(b,a))
#define ROR_L(a, b)	CHOICE(rorl ARG2(a,b), rorl ARG2(a,b), _LTOG ror ARG2(b,a))
#define ROR_W(a, b)	CHOICE(rorw ARG2(a,b), rorw ARG2(a,b), _WTOG ror ARG2(b,a))
#define ROR_B(a, b)	CHOICE(rorb ARG2(a,b), rorb ARG2(a,b), rorb ARG2(b,a))
#define REP		CHOICE(rep ;, rep ;, repe)
#define REPE		CHOICE(repz ;, repe ;, repe)
#define REPNE		CHOICE(repnz ;, repne ;, repne)
#define REPNZ		REPNE
#define REPZ		REPE
#define RET		CHOICE(ret, ret, ret)
#define SAHF		CHOICE(sahf, sahf, sahf)
#define SAL_L(a, b)	CHOICE(sall ARG2(a,b), sall ARG2(a,b), _LTOG sal ARG2(b,a))
#define SAL_W(a, b)	CHOICE(salw ARG2(a,b), salw ARG2(a,b), _WTOG sal ARG2(b,a))
#define SAL_B(a, b)	CHOICE(salb ARG2(a,b), salb ARG2(a,b), salb ARG2(b,a))
#define SAR_L(a, b)	CHOICE(sarl ARG2(a,b), sarl ARG2(a,b), _LTOG sar ARG2(b,a))
#define SAR_W(a, b)	CHOICE(sarw ARG2(a,b), sarw ARG2(a,b), _WTOG sar ARG2(b,a))
#define SAR_B(a, b)	CHOICE(sarb ARG2(a,b), sarb ARG2(a,b), sarb ARG2(b,a))
#define SBB_L(a, b)	CHOICE(sbbl ARG2(a,b), sbbl ARG2(a,b), _LTOG sbb ARG2(b,a))
#define SBB_W(a, b)	CHOICE(sbbw ARG2(a,b), sbbw ARG2(a,b), _WTOG sbb ARG2(b,a))
#define SBB_B(a, b)	CHOICE(sbbb ARG2(a,b), sbbb ARG2(a,b), sbbb ARG2(b,a))
#define SCAS_L		CHOICE(scasl, scasl, _LTOG scas)
#define SCAS_W		CHOICE(scasw, scasw, _WTOG scas)
#define SCAS_B		CHOICE(scasb, scasb, scasb)
#define SETA(a)		CHOICE(seta a, seta a, seta a)
#define SETAE(a)	CHOICE(setae a, setae a, setae a)
#define SETB(a)		CHOICE(setb a, setb a, setb a)
#define SETBE(a)	CHOICE(setbe a, setbe a, setbe a)
#define SETC(a)		CHOICE(setc a, setb a, setb a)
#define SETE(a)		CHOICE(sete a, sete a, sete a)
#define SETG(a)		CHOICE(setg a, setg a, setg a)
#define SETGE(a)	CHOICE(setge a, setge a, setge a)
#define SETL(a)		CHOICE(setl a, setl a, setl a)
#define SETLE(a)	CHOICE(setle a, setle a, setle a)
#define SETNA(a)	CHOICE(setna a, setna a, setna a)
#define SETNAE(a)	CHOICE(setnae a, setnae a, setnae a)
#define SETNB(a)	CHOICE(setnb a, setnb a, setnb a)
#define SETNBE(a)	CHOICE(setnbe a, setnbe a, setnbe a)
#define SETNC(a)	CHOICE(setnc a, setnb a, setnb a)
#define SETNE(a)	CHOICE(setne a, setne a, setne a)
#define SETNG(a)	CHOICE(setng a, setng a, setng a)
#define SETNGE(a)	CHOICE(setnge a, setnge a, setnge a)
#define SETNL(a)	CHOICE(setnl a, setnl a, setnl a)
#define SETNLE(a)	CHOICE(setnle a, setnle a, setnle a)
#define SETNO(a)	CHOICE(setno a, setno a, setno a)
#define SETNP(a)	CHOICE(setnp a, setnp a, setnp a)
#define SETNS(a)	CHOICE(setns a, setns a, setna a)
#define SETNZ(a)	CHOICE(setnz a, setnz a, setnz a)
#define SETO(a)		CHOICE(seto a, seto a, seto a)
#define SETP(a)		CHOICE(setp a, setp a, setp a)
#define SETPE(a)	CHOICE(setpe a, setpe a, setpe a)
#define SETPO(a)	CHOICE(setpo a, setpo a, setpo a)
#define SETS(a)		CHOICE(sets a, sets a, seta a)
#define SETZ(a)		CHOICE(setz a, setz a, setz a)
#define SGDT(a)		CHOICE(sgdt a, sgdt a, sgdt a)
#define SIDT(a)		CHOICE(sidt a, sidt a, sidt a)
#define SHL_L(a, b)	CHOICE(shll ARG2(a,b), shll ARG2(a,b), _LTOG shl ARG2(b,a))
#define SHL_W(a, b)	CHOICE(shlw ARG2(a,b), shlw ARG2(a,b), _WTOG shl ARG2(b,a))
#define SHL_B(a, b)	CHOICE(shlb ARG2(a,b), shlb ARG2(a,b), shlb ARG2(b,a))
#define SHLD_L(a,b,c)	CHOICE(shldl ARG3(a,b,c), shldl ARG3(a,b,c), _LTOG shld ARG3(c,b,a))
#define SHLD2_L(a,b)	CHOICE(shldl ARG2(a,b), shldl ARG3(CL,a,b), _LTOG shld ARG3(b,a,CL))
#define SHLD_W(a,b,c)	CHOICE(shldw ARG3(a,b,c), shldw ARG3(a,b,c), _WTOG shld ARG3(c,b,a))
#define SHLD2_W(a,b)	CHOICE(shldw ARG2(a,b), shldw ARG3(CL,a,b), _WTOG shld ARG3(b,a,CL))
#define SHR_L(a, b)	CHOICE(shrl ARG2(a,b), shrl ARG2(a,b), _LTOG shr ARG2(b,a))
#define SHR_W(a, b)	CHOICE(shrw ARG2(a,b), shrw ARG2(a,b), _WTOG shr ARG2(b,a))
#define SHR_B(a, b)	CHOICE(shrb ARG2(a,b), shrb ARG2(a,b), shrb ARG2(b,a))
#define SHRD_L(a,b,c)	CHOICE(shrdl ARG3(a,b,c), shrdl ARG3(a,b,c), _LTOG shrd ARG3(c,b,a))
#define SHRD2_L(a,b)	CHOICE(shrdl ARG2(a,b), shrdl ARG3(CL,a,b), _LTOG shrd ARG3(b,a,CL))
#define SHRD_W(a,b,c)	CHOICE(shrdw ARG3(a,b,c), shrdw ARG3(a,b,c), _WTOG shrd ARG3(c,b,a))
#define SHRD2_W(a,b)	CHOICE(shrdw ARG2(a,b), shrdw ARG3(CL,a,b), _WTOG shrd ARG3(b,a,CL))
#define SLDT(a)		CHOICE(sldt a, sldt a, sldt a)
#define SMSW(a)		CHOICE(smsw a, smsw a, smsw a)
#define STC		CHOICE(stc, stc, stc)
#define STD		CHOICE(std, std, std)
#define STI		CHOICE(sti, sti, sti)
#define STOS_L		CHOICE(stosl, stosl, _LTOG stos)
#define STOS_W		CHOICE(stosw, stosw, _WTOG stos)
#define STOS_B		CHOICE(stosb, stosb, stosb)
#define STR(a)		CHOICE(str a, str a, str a)
#define SUB_L(a, b)	CHOICE(subl ARG2(a,b), subl ARG2(a,b), _LTOG sub ARG2(b,a))
#define SUB_W(a, b)	CHOICE(subw ARG2(a,b), subw ARG2(a,b), _WTOG sub ARG2(b,a))
#define SUB_B(a, b)	CHOICE(subb ARG2(a,b), subb ARG2(a,b), subb ARG2(b,a))
#define TEST_L(a, b)	CHOICE(testl ARG2(a,b), testl ARG2(a,b), _LTOG test ARG2(b,a))
#define TEST_W(a, b)	CHOICE(testw ARG2(a,b), testw ARG2(a,b), _WTOG test ARG2(b,a))
#define TEST_B(a, b)	CHOICE(testb ARG2(a,b), testb ARG2(a,b), testb ARG2(b,a))
#define VERR(a)		CHOICE(verr a, verr a, verr a)
#define VERW(a)		CHOICE(verw a, verw a, verw a)
#define WAIT		CHOICE(wait, wait, wait)
#define XCHG_L(a, b)	CHOICE(xchgl ARG2(a,b), xchgl ARG2(a,b), _LTOG xchg ARG2(b,a))
#define XCHG_W(a, b)	CHOICE(xchgw ARG2(a,b), xchgw ARG2(a,b), _WTOG xchg ARG2(b,a))
#define XCHG_B(a, b)	CHOICE(xchgb ARG2(a,b), xchgb ARG2(a,b), xchgb ARG2(b,a))
#define XLAT		CHOICE(xlat, xlat, xlat)
#define XOR_L(a, b)	CHOICE(xorl ARG2(a,b), xorl ARG2(a,b), _LTOG xor ARG2(b,a))
#define XOR_W(a, b)	CHOICE(xorw ARG2(a,b), xorw ARG2(a,b), _WTOG xor ARG2(b,a))
#define XOR_B(a, b)	CHOICE(xorb ARG2(a,b), xorb ARG2(a,b), xorb ARG2(b,a))


/* Floating Point Instructions */
#define F2XM1		CHOICE(f2xm1, f2xm1, f2xm1)
#define FABS		CHOICE(fabs, fabs, fabs)
#define FADD_D(a)	CHOICE(faddl a, faddl a, faddd a)
#define FADD_S(a)	CHOICE(fadds a, fadds a, fadds a)
#define FADD2(a, b)	CHOICE(fadd ARG2(a,b), fadd ARG2(a,b), fadd ARG2(b,a))
#define FADDP(a, b)	CHOICE(faddp ARG2(a,b), faddp ARG2(a,b), faddp ARG2(b,a))
#define FIADD_L(a)	CHOICE(fiaddl a, fiaddl a, fiaddl a)
#define FIADD_W(a)	CHOICE(fiadd a, fiadds a, fiadds a)
#define FBLD(a)		CHOICE(fbld a, fbld a, fbld a)
#define FBSTP(a)	CHOICE(fbstp a, fbstp a, fbstp a)
#define FCHS		CHOICE(fchs, fchs, fchs)
#define FCLEX		CHOICE(fclex, wait; fnclex, wait; fclex)
#define FNCLEX		CHOICE(fnclex, fnclex, fclex)
#define FCOM(a)		CHOICE(fcom a, fcom a, fcom a)
#define FCOM_D(a)	CHOICE(fcoml a, fcoml a, fcomd a)
#define FCOM_S(a)	CHOICE(fcoms a, fcoms a, fcoms a)
#define FCOMP(a)	CHOICE(fcomp a, fcomp a, fcomp a)
#define FCOMP_D(a)	CHOICE(fcompl a, fcompl a, fcompd a)
#define FCOMP_S(a)	CHOICE(fcomps a, fcomps a, fcomps a)
#define FCOMPP		CHOICE(fcompp, fcompp, fcompp)
#define FCOS		CHOICE(fcos, fcos, fcos)
#define FDECSTP		CHOICE(fdecstp, fdecstp, fdecstp)
#define FDIV_D(a)	CHOICE(fdivl a, fdivl a, fdivd a)
#define FDIV_S(a)	CHOICE(fdivs a, fdivs a, fdivs a)
#define FDIV2(a, b)	CHOICE(fdiv ARG2(a,b), fdiv ARG2(a,b), fdiv ARG2(b,a))
#define FDIVP(a, b)	CHOICE(fdivp ARG2(a,b), fdivp ARG2(a,b), fdivp ARG2(b,a))
#define FIDIV_L(a)	CHOICE(fidivl a, fidivl a, fidivl a)
#define FIDIV_W(a)	CHOICE(fidiv a, fidivs a, fidivs a)
#define FDIVR_D(a)	CHOICE(fdivrl a, fdivrl a, fdivrd a)
#define FDIVR_S(a)	CHOICE(fdivrs a, fdivrs a, fdivrs a)
#define FDIVR2(a, b)	CHOICE(fdivr ARG2(a,b), fdivr ARG2(a,b), fdivr ARG2(b,a))
#define FDIVRP(a, b)	CHOICE(fdivrp ARG2(a,b), fdivrp ARG2(a,b), fdivrp ARG2(b,a))
#define FIDIVR_L(a)	CHOICE(fidivrl a, fidivrl a, fidivrl a)
#define FIDIVR_W(a)	CHOICE(fidivr a, fidivrs a, fidivrs a)
#define FFREE(a)	CHOICE(ffree a, ffree a, ffree a)
#define FICOM_L(a)	CHOICE(ficoml a, ficoml a, ficoml a)
#define FICOM_W(a)	CHOICE(ficom a, ficoms a, ficoms a)
#define FICOMP_L(a)	CHOICE(ficompl a, ficompl a, ficompl a)
#define FICOMP_W(a)	CHOICE(ficomp a, ficomps a, ficomps a)
#define FILD_Q(a)	CHOICE(fildll a, fildq a, fildq a)
#define FILD_L(a)	CHOICE(fildl a, fildl a, fildl a)
#define FILD_W(a)	CHOICE(fild a, filds a, filds a)
#define FINCSTP		CHOICE(fincstp, fincstp, fincstp)
#define FINIT		CHOICE(finit, wait; fninit, wait; finit)
#define FNINIT		CHOICE(fninit, fninit, finit)
#define FIST_L(a)	CHOICE(fistl a, fistl a, fistl a)
#define FIST_W(a)	CHOICE(fist a, fists a, fists a)
#define FISTP_Q(a)	CHOICE(fistpll a, fistpq a, fistpq a)
#define FISTP_L(a)	CHOICE(fistpl a, fistpl a, fistpl a)
#define FISTP_W(a)	CHOICE(fistp a, fistps a, fistps a)
#define FLD_X(a)	CHOICE(fldt a, fldt a, fldx a) /* 80 bit data type! */
#define FLD_D(a)	CHOICE(fldl a, fldl a, fldd a)
#define FLD_S(a)	CHOICE(flds a, flds a, flds a)
#define FLD1		CHOICE(fld1, fld1, fld1)
#define FLDL2T		CHOICE(fldl2t, fldl2t, fldl2t)
#define FLDL2E		CHOICE(fldl2e, fldl2e, fldl2e)
#define FLDPI		CHOICE(fldpi, fldpi, fldpi)
#define FLDLG2		CHOICE(fldlg2, fldlg2, fldlg2)
#define FLDLN2		CHOICE(fldln2, fldln2, fldln2)
#define FLDZ		CHOICE(fldz, fldz, fldz)
#define FLDCW(a)	CHOICE(fldcw a, fldcw a, fldcw a)
#define FLDENV(a)	CHOICE(fldenv a, fldenv a, fldenv a)
#define FMUL_S(a)	CHOICE(fmuls a, fmuls a, fmuls a)
#define FMUL_D(a)	CHOICE(fmull a, fmull a, fmuld a)
#define FMUL2(a, b)	CHOICE(fmul ARG2(a,b), fmul ARG2(a,b), fmul ARG2(b,a))
#define FMULP(a, b)	CHOICE(fmulp ARG2(a,b), fmulp ARG2(a,b), fmulp ARG2(b,a))
#define FIMUL_L(a)	CHOICE(fimull a, fimull a, fimull a)
#define FIMUL_W(a)	CHOICE(fimul a, fimuls a, fimuls a)
#define FNOP		CHOICE(fnop, fnop, fnop)
#define FPATAN		CHOICE(fpatan, fpatan, fpatan)
#define FPREM		CHOICE(fprem, fprem, fprem)
#define FPREM1		CHOICE(fprem1, fprem1, fprem1)
#define FPTAN		CHOICE(fptan, fptan, fptan)
#define FRNDINT		CHOICE(frndint, frndint, frndint)
#define FRSTOR(a)	CHOICE(frstor a, frstor a, frstor a)
#define FSAVE(a)	CHOICE(fsave a, wait; fnsave a, wait; fsave a)
#define FNSAVE(a)	CHOICE(fnsave a, fnsave a, fsave a)
#define FSCALE		CHOICE(fscale, fscale, fscale)
#define FSIN		CHOICE(fsin, fsin, fsin)
#define FSINCOS		CHOICE(fsincos, fsincos, fsincos)
#define FSQRT		CHOICE(fsqrt, fsqrt, fsqrt)
#define FST_D(a)	CHOICE(fstl a, fstl a, fstd a)
#define FST_S(a)	CHOICE(fsts a, fsts a, fsts a)
#define FSTP_X(a)	CHOICE(fstpt a, fstpt a, fstpx a)
#define FSTP_D(a)	CHOICE(fstpl a, fstpl a, fstpd a)
#define FSTP_S(a)	CHOICE(fstps a, fstps a, fstps a)
#define FSTP(a)		CHOICE(fstp a, fstp a, fstp a)
#define FSTCW(a)	CHOICE(fstcw a, wait; fnstcw a, wait; fstcw a)
#define FNSTCW(a)	CHOICE(fnstcw a, fnstcw a, fstcw a)
#define FSTENV(a)	CHOICE(fstenv a, wait; fnstenv a, fstenv a)
#define FNSTENV(a)	CHOICE(fnstenv a, fnstenv a, fstenv a)
#define FSTSW(a)	CHOICE(fstsw a, wait; fnstsw a, wait; fstsw a)
#define FNSTSW(a)	CHOICE(fnstsw a, fnstsw a, fstsw a)
#define FSUB_S(a)	CHOICE(fsubs a, fsubs a, fsubs a)
#define FSUB_D(a)	CHOICE(fsubl a, fsubl a, fsubd a)
#define FSUB2(a, b)	CHOICE(fsub ARG2(a,b), fsub ARG2(a,b), fsub ARG2(b,a))
#define FSUBP(a, b)	CHOICE(fsubp ARG2(a,b), fsubp ARG2(a,b), fsubp ARG2(b,a))
#define FISUB_L(a)	CHOICE(fisubl a, fisubl a, fisubl a)
#define FISUB_W(a)	CHOICE(fisub a, fisubs a, fisubs a)
#define FSUBR_S(a)	CHOICE(fsubrs a, fsubrs a, fsubrs a)
#define FSUBR_D(a)	CHOICE(fsubrl a, fsubrl a, fsubrd a)
#define FSUBR2(a, b)	CHOICE(fsubr ARG2(a,b), fsubr ARG2(a,b), fsubr ARG2(b,a))
#define FSUBRP(a, b)	CHOICE(fsubrp ARG2(a,b), fsubrp ARG2(a,b), fsubrp ARG2(b,a))
#define FISUBR_L(a)	CHOICE(fisubrl a, fisubrl a, fisubrl a)
#define FISUBR_W(a)	CHOICE(fisubr a, fisubrs a, fisubrs a)
#define FTST		CHOICE(ftst, ftst, ftst)
#define FUCOM(a)	CHOICE(fucom a, fucom a, fucom a)
#define FUCOMP(a)	CHOICE(fucomp a, fucomp a, fucomp a)
#define FUCOMPP		CHOICE(fucompp, fucompp, fucompp)
#define FWAIT		CHOICE(wait, wait, wait)
#define FXAM		CHOICE(fxam, fxam, fxam)
#define FXCH(a)		CHOICE(fxch a, fxch a, fxch a)
#define FXTRACT		CHOICE(fxtract, fxtract, fxtract)
#define FYL2X		CHOICE(fyl2x, fyl2x, fyl2x)
#define FYL2XP1		CHOICE(fyl2xp1, fyl2xp1, fyl2xp1)

/* New instructions */
#define CPUID		CHOICE(D_BYTE ARG2(15, 162), cpuid, D_BYTE ARG2(15, 162))
#define RDTSC		CHOICE(D_BYTE ARG2(15, 49), rdtsc, D_BYTE ARG2(15, 49))

#else /* NASM_ASSEMBLER || MASM_ASSEMBLER is defined */

	/****************************************/
	/*					*/
	/*	Intel style assemblers.		*/
	/*	(NASM and MASM)			*/
	/*					*/
	/****************************************/

#define P_EAX		EAX
#define L_EAX		EAX
#define W_AX		AX
#define B_AH		AH
#define B_AL		AL

#define P_EBX		EBX
#define L_EBX		EBX
#define W_BX		BX
#define B_BH		BH
#define B_BL		BL

#define P_ECX		ECX
#define L_ECX		ECX
#define W_CX		CX
#define B_CH		CH
#define B_CL		CL

#define P_EDX		EDX
#define L_EDX		EDX
#define W_DX		DX
#define B_DH		DH
#define B_DL		DL

#define P_EBP		EBP
#define L_EBP		EBP
#define W_BP		BP

#define P_ESI		ESI
#define L_ESI		ESI
#define W_SI		SI

#define P_EDI		EDI
#define L_EDI		EDI
#define W_DI		DI

#define P_ESP		ESP
#define L_ESP		ESP
#define W_SP		SP

#define W_CS		CS
#define W_SS		SS
#define W_DS		DS
#define W_ES		ES
#define W_FS		FS
#define W_GS		GS

#define X_ST		ST
#define D_ST		ST
#define L_ST		ST

#define P_MM0		mm0
#define P_MM1		mm1
#define P_MM2		mm2
#define P_MM3		mm3
#define P_MM4		mm4
#define P_MM5		mm5
#define P_MM6		mm6
#define P_MM7		mm7

#define P_XMM0		xmm0
#define P_XMM1		xmm1
#define P_XMM2		xmm2
#define P_XMM3		xmm3
#define P_XMM4		xmm4
#define P_XMM5		xmm5
#define P_XMM6		xmm6
#define P_XMM7		xmm7

#define CONCAT(x, y)		x ## y
#define CONCAT3(x, y, z)	x ## y ## z

#if defined(NASM_ASSEMBLER)

#define ST(n)		st ## n
#define ST0		st0

#define TBYTE_PTR	tword
#define QWORD_PTR	qword
#define DWORD_PTR	dword
#define WORD_PTR	word
#define BYTE_PTR	byte

#define OFFSET

#define GLOBL			GLOBAL
#define ALIGNTEXT32		ALIGN 32
#define ALIGNTEXT16		ALIGN 16
#define ALIGNTEXT8		ALIGN 8
#define ALIGNTEXT4		ALIGN 4
#define ALIGNTEXT2		ALIGN 2
#define ALIGNTEXT32ifNOP	ALIGN 32
#define ALIGNTEXT16ifNOP	ALIGN 16
#define ALIGNTEXT8ifNOP		ALIGN 8
#define ALIGNTEXT4ifNOP		ALIGN 4
#define ALIGNDATA32		ALIGN 32
#define ALIGNDATA16		ALIGN 16
#define ALIGNDATA8		ALIGN 8
#define ALIGNDATA4		ALIGN 4
#define ALIGNDATA2		ALIGN 2
#define FILE(s)
#define STRING(s)	db s
#define D_LONG		dd
#define D_WORD		dw
#define D_BYTE		db
/* #define SPACE */
/* #define COMM */
#if defined(__WATCOMC__)
SECTION _TEXT public align=16 class=CODE use32 flat
SECTION _DATA public align=16 class=DATA use32 flat
#define SEG_TEXT	SECTION _TEXT
#define SEG_DATA	SECTION _DATA
#define SEG_BSS		SECTION .bss
#else
#define SEG_DATA	SECTION .data
#define SEG_TEXT	SECTION .text
#define SEG_BSS		SECTION .bss
#endif

#define D_SPACE(n)	db n REP 0

#define AS_BEGIN

/* Jcc's should be handled better than this... */
#define NEAR		near

#else /* MASM */

#define TBYTE_PTR	tbyte ptr
#define QWORD_PTR	qword ptr
#define DWORD_PTR	dword ptr
#define WORD_PTR	word ptr
#define BYTE_PTR	byte ptr

#define OFFSET		offset

#define GLOBL			GLOBAL
#define ALIGNTEXT32		ALIGN 32
#define ALIGNTEXT16		ALIGN 16
#define ALIGNTEXT8		ALIGN 8
#define ALIGNTEXT4		ALIGN 4
#define ALIGNTEXT2		ALIGN 2
#define ALIGNTEXT32ifNOP	ALIGN 32
#define ALIGNTEXT16ifNOP	ALIGN 16
#define ALIGNTEXT8ifNOP		ALIGN 8
#define ALIGNTEXT4ifNOP		ALIGN 4
#define ALIGNDATA32		ALIGN 32
#define ALIGNDATA16		ALIGN 16
#define ALIGNDATA8		ALIGN 8
#define ALIGNDATA4		ALIGN 4
#define ALIGNDATA2		ALIGN 2
#define FILE(s)
#define STRING(s)	db s
#define D_LONG		dd
#define D_WORD		dw
#define D_BYTE		db
/* #define SPACE */
/* #define COMM */
#define SEG_DATA	.DATA
#define SEG_TEXT	.CODE
#define SEG_BSS		.DATA

#define D_SPACE(n)	db n REP 0

#define AS_BEGIN

#define NEAR

#endif

#if defined(Lynx) || (defined(SYSV) || defined(SVR4)) \
 || (defined(__linux__) || defined(__OS2ELF__)) && defined(__ELF__) \
 || defined(__FreeBSD__) && __FreeBSD__ >= 3
#define GLNAME(a)	a
#else
#define GLNAME(a)	CONCAT(_, a)
#endif

/*
 *	Addressing Modes
 */

/* Immediate Mode */
#define P_ADDR(a)		OFFSET a
#define X_ADDR(a)		OFFSET a
#define D_ADDR(a)		OFFSET a
#define L_ADDR(a)		OFFSET a
#define W_ADDR(a)		OFFSET a
#define B_ADDR(a)		OFFSET a

#define P_CONST(a)		a
#define X_CONST(a)		a
#define D_CONST(a)		a
#define L_CONST(a)		a
#define W_CONST(a)		a
#define B_CONST(a)		a

/* Indirect Mode */
#ifdef NASM_ASSEMBLER
#define P_CONTENT(a)		[a]
#define X_CONTENT(a)		TBYTE_PTR [a]
#define D_CONTENT(a)		QWORD_PTR [a]
#define L_CONTENT(a)		DWORD_PTR [a]
#define W_CONTENT(a)		WORD_PTR [a]
#define B_CONTENT(a)		BYTE_PTR [a]
#else
#define P_CONTENT(a)		a
#define X_CONTENT(a)		TBYTE_PTR a
#define D_CONTENT(a)		QWORD_PTR a
#define L_CONTENT(a)		DWORD_PTR a
#define W_CONTENT(a)		WORD_PTR a
#define B_CONTENT(a)		BYTE_PTR a
#endif

/* Register a indirect */
#define P_REGIND(a)		[a]
#define X_REGIND(a)		TBYTE_PTR [a]
#define D_REGIND(a)		QWORD_PTR [a]
#define L_REGIND(a)		DWORD_PTR [a]
#define W_REGIND(a)		WORD_PTR [a]
#define B_REGIND(a)		BYTE_PTR [a]

/* Register b indirect plus displacement a */
#define P_REGOFF(a, b)		[b + a]
#define X_REGOFF(a, b)		TBYTE_PTR [b + a]
#define D_REGOFF(a, b)		QWORD_PTR [b + a]
#define L_REGOFF(a, b)		DWORD_PTR [b + a]
#define W_REGOFF(a, b)		WORD_PTR [b + a]
#define B_REGOFF(a, b)		BYTE_PTR [b + a]

/* Reg indirect Base + Index + Displacement  - this is mainly for 16-bit mode
 * which has no scaling
 */
#define P_REGBID(b, i, d)	[b + i + d]
#define X_REGBID(b, i, d)	TBYTE_PTR [b + i + d]
#define D_REGBID(b, i, d)	QWORD_PTR [b + i + d]
#define L_REGBID(b, i, d)	DWORD_PTR [b + i + d]
#define W_REGBID(b, i, d)	WORD_PTR [b + i + d]
#define B_REGBID(b, i, d)	BYTE_PTR [b + i + d]

/* Reg indirect Base + (Index * Scale) */
#define P_REGBIS(b, i, s)	[b + i * s]
#define X_REGBIS(b, i, s)	TBYTE_PTR [b + i * s]
#define D_REGBIS(b, i, s)	QWORD_PTR [b + i * s]
#define L_REGBIS(b, i, s)	DWORD_PTR [b + i * s]
#define W_REGBIS(b, i, s)	WORD_PTR [b + i * s]
#define B_REGBIS(b, i, s)	BYTE_PTR [b + i * s]

/* Reg indirect Base + (Index * Scale) + Displacement */
#define P_REGBISD(b, i, s, d)	[b + i * s + d]
#define X_REGBISD(b, i, s, d)	TBYTE_PTR [b + i * s + d]
#define D_REGBISD(b, i, s, d)	QWORD_PTR [b + i * s + d]
#define L_REGBISD(b, i, s, d)	DWORD_PTR [b + i * s + d]
#define W_REGBISD(b, i, s, d)	WORD_PTR [b + i * s + d]
#define B_REGBISD(b, i, s, d)	BYTE_PTR [b + i * s + d]

/* Displaced Scaled Index: */
#define P_REGDIS(d, i, s)	[i * s + d]
#define X_REGDIS(d, i, s)	TBYTE_PTR [i * s + d]
#define D_REGDIS(d, i, s)	QWORD_PTR [i * s + d]
#define L_REGDIS(d, i, s)	DWORD_PTR [i * s + d]
#define W_REGDIS(d, i, s)	WORD_PTR [i * s + d]
#define B_REGDIS(d, i, s)	BYTE_PTR [i * s + d]

/* Indexed Base: */
#define P_REGBI(b, i)		[b + i]
#define X_REGBI(b, i)		TBYTE_PTR [b + i]
#define D_REGBI(b, i)		QWORD_PTR [b + i]
#define L_REGBI(b, i)		DWORD_PTR [b + i]
#define W_REGBI(b, i)		WORD_PTR [b + i]
#define B_REGBI(b, i)		BYTE_PTR [b + i]

/* Displaced Base: */
#define P_REGDB(d, b)		[b + d]
#define X_REGDB(d, b)		TBYTE_PTR [b + d]
#define D_REGDB(d, b)		QWORD_PTR [b + d]
#define L_REGDB(d, b)		DWORD_PTR [b + d]
#define W_REGDB(d, b)		WORD_PTR [b + d]
#define B_REGDB(d, b)		BYTE_PTR [b + d]

/* Variable indirect: */
#define VARINDIRECT(var)	[var]

/* Use register contents as jump/call target: */
#define CODEPTR(reg)		P_(reg)

/*
 * Redefine assembler commands
 */

#define P_(a)			P_ ## a
#define X_(a)			X_ ## a
#define D_(a)			D_ ## a
#define SR_(a)			W_ ## a
#define S_(a)			L_ ## a
#define L_(a)			L_ ## a
#define W_(a)			W_ ## a
#define B_(a)			B_ ## a

#define AAA			aaa
#define AAD			aad
#define AAM			aam
#define AAS			aas
#define ADC_L(a, b)		adc L_(b), L_(a)
#define ADC_W(a, b)		adc W_(b), W_(a)
#define ADC_B(a, b)		adc B_(b), B_(a)
#define ADD_L(a, b)		add L_(b), L_(a)
#define ADD_W(a, b)		add W_(b), W_(a)
#define ADD_B(a, b)		add B_(b), B_(a)
#define AND_L(a, b)		and L_(b), L_(a)
#define AND_W(a, b)		and W_(b), W_(a)
#define AND_B(a, b)		and B_(b), B_(a)
#define ARPL(a,b)		arpl W_(b), a
#define BOUND_L(a, b)		bound L_(b), L_(a)
#define BOUND_W(a, b)		bound W_(b), W_(a)
#define BSF_L(a, b)		bsf L_(b), L_(a)
#define BSF_W(a, b)		bsf W_(b), W_(a)
#define BSR_L(a, b)		bsr L_(b), L_(a)
#define BSR_W(a, b)		bsr W_(b), W_(a)
#define BT_L(a, b)		bt L_(b), L_(a)
#define BT_W(a, b)		bt W_(b), W_(a)
#define BTC_L(a, b)		btc L_(b), L_(a)
#define BTC_W(a, b)		btc W_(b), W_(a)
#define BTR_L(a, b)		btr L_(b), L_(a)
#define BTR_W(a, b)		btr W_(b), W_(a)
#define BTS_L(a, b)		bts L_(b), L_(a)
#define BTS_W(a, b)		bts W_(b), W_(a)
#define CALL(a)			call a
#define CALLF(s,a)		call far s:a
#define CBW			cbw
#define CWDE			cwde
#define CLC			clc
#define CLD			cld
#define CLI			cli
#define CLTS			clts
#define CMC			cmc
#define CMP_L(a, b)		cmp L_(b), L_(a)
#define CMP_W(a, b)		cmp W_(b), W_(a)
#define CMP_B(a, b)		cmp B_(b), B_(a)
#define CMPS_L			cmpsd
#define CMPS_W			cmpsw
#define CMPS_B			cmpsb
#define CPUID			cpuid
#define CWD			cwd
#define CDQ			cdq
#define DAA			daa
#define DAS			das
#define DEC_L(a)		dec L_(a)
#define DEC_W(a)		dec W_(a)
#define DEC_B(a)		dec B_(a)
#define DIV_L(a)		div L_(a)
#define DIV_W(a)		div W_(a)
#define DIV_B(a)		div B_(a)
#define ENTER(a,b)		enter b, a
#define HLT			hlt
#define IDIV_L(a)		idiv L_(a)
#define IDIV_W(a)		idiv W_(a)
#define IDIV_B(a)		idiv B_(a)
#define IMUL_L(a, b)		imul L_(b), L_(a)
#define IMUL_W(a, b)		imul W_(b), W_(a)
#define IMUL_B(a)		imul B_(a)
#define IN_L			in EAX, DX
#define IN_W			in AX, DX
#define IN_B			in AL, DX
#define IN1_L(a)		in1 L_(a)
#define IN1_W(a)		in1 W_(a)
#define IN1_B(a)		in1 B_(a)
#define INC_L(a)		inc L_(a)
#define INC_W(a)		inc W_(a)
#define INC_B(a)		inc B_(a)
#define INS_L			ins
#define INS_W			ins
#define INS_B			ins
#define INT(a)			int B_(a)
#define INT3			int3
#define INTO			into
#define IRET			iret
#define IRETD			iretd
#define JA(a)			ja NEAR a
#define JAE(a)			jae NEAR a
#define JB(a)			jb NEAR a
#define JBE(a)			jbe NEAR a
#define JC(a)			jc NEAR a
#define JE(a)			je NEAR a
#define JG(a)			jg NEAR a
#define JGE(a)			jge NEAR a
#define JL(a)			jl NEAR a
#define JLE(a)			jle NEAR a
#define JNA(a)			jna NEAR a
#define JNAE(a)			jnae NEAR a
#define JNB(a)			jnb NEAR a
#define JNBE(a)			jnbe NEAR a
#define JNC(a)			jnc NEAR a
#define JNE(a)			jne NEAR a
#define JNG(a)			jng NEAR a
#define JNGE(a)			jnge NEAR a
#define JNL(a)			jnl NEAR a
#define JNLE(a)			jnle NEAR a
#define JNO(a)			jno NEAR a
#define JNP(a)			jnp NEAR a
#define JNS(a)			jns NEAR a
#define JNZ(a)			jnz NEAR a
#define JO(a)			jo NEAR a
#define JP(a)			jp NEAR a
#define JPE(a)			jpe NEAR a
#define JPO(a)			jpo NEAR a
#define JS(a)			js NEAR a
#define JZ(a)			jz NEAR a
#define JMP(a)			jmp a
#define JMPF(s,a)		jmp far s:a
#define LAHF			lahf
#define LAR(a, b)		lar b, a
#define LEA_L(a, b)		lea P_(b), P_(a)
#define LEA_W(a, b)		lea P_(b), P_(a)
#define LEAVE			leave
#define LGDT(a)			lgdt a
#define LIDT(a)			lidt a
#define LDS(a, b)		lds b, P_(a)
#define LES(a, b)		les b, P_(a)
#define LFS(a, b)		lfs b, P_(a)
#define LGS(a, b)		lgs b, P_(a)
#define LSS(a, b)		lss b, P_(a)
#define LLDT(a)			lldt a
#define LMSW(a)			lmsw a
#define LOCK			lock
#define LODS_L			lodsd
#define LODS_W			lodsw
#define LODS_B			lodsb
#define LOOP(a)			loop a
#define LOOPE(a)		loope a
#define LOOPZ(a)		loopz a
#define LOOPNE(a)		loopne a
#define LOOPNZ(a)		loopnz a
#define LSL(a, b)		lsl b, a
#define LTR(a)			ltr a
#define MOV_SR(a, b)		mov SR_(b), SR_(a)
#define MOV_L(a, b)		mov L_(b), L_(a)
#define MOV_W(a, b)		mov W_(b), W_(a)
#define MOV_B(a, b)		mov B_(b), B_(a)
#define MOVS_L			movsd
#define MOVS_W			movsw
#define MOVS_B			movsb
#define MOVSX_BL(a, b)		movsx B_(b), B_(a)
#define MOVSX_BW(a, b)		movsx B_(b), B_(a)
#define MOVSX_WL(a, b)		movsx W_(b), W_(a)
#define MOVZX_BL(a, b)		movzx B_(b), B_(a)
#define MOVZX_BW(a, b)		movzx B_(b), B_(a)
#define MOVZX_WL(a, b)		movzx W_(b), W_(a)
#define MUL_L(a)		mul L_(a)
#define MUL_W(a)		mul W_(a)
#define MUL_B(a)		mul B_(a)
#define NEG_L(a)		neg L_(a)
#define NEG_W(a)		neg W_(a)
#define NEG_B(a)		neg B_(a)
#define NOP			nop
#define NOT_L(a)		not L_(a)
#define NOT_W(a)		not W_(a)
#define NOT_B(a)		not B_(a)
#define OR_L(a,b)		or L_(b), L_(a)
#define OR_W(a,b)		or W_(b), W_(a)
#define OR_B(a,b)		or B_(b), B_(a)
#define OUT_L			out DX, EAX
#define OUT_W			out DX, AX
#define OUT_B			out DX, AL
#define OUT1_L(a)		out1 L_(a)
#define OUT1_W(a)		out1 W_(a)
#define OUT1_B(a)		out1 B_(a)
#define OUTS_L			outsd
#define OUTS_W			outsw
#define OUTS_B			outsb
#define POP_SR(a)		pop SR_(a)
#define POP_L(a)		pop L_(a)
#define POP_W(a)		pop W_(a)
#define POPA_L			popad
#define POPA_W			popa
#define POPF_L			popfd
#define POPF_W			popf
#define PUSH_SR(a)		push SR_(a)
#define PUSH_L(a)		push L_(a)
#define PUSH_W(a)		push W_(a)
#define PUSH_B(a)		push B_(a)
#define PUSHA_L			pushad
#define PUSHA_W			pusha
#define PUSHF_L			pushfd
#define PUSHF_W			pushf
#define RCL_L(a, b)		rcl L_(b), L_(a)
#define RCL_W(a, b)		rcl W_(b), W_(a)
#define RCL_B(a, b)		rcl B_(b), B_(a)
#define RCR_L(a, b)		rcr L_(b), L_(a)
#define RCR_W(a, b)		rcr W_(b), W_(a)
#define RCR_B(a, b)		rcr B_(b), B_(a)
#define RDTSC			rdtsc
#define ROL_L(a, b)		rol L_(b), L_(a)
#define ROL_W(a, b)		rol W_(b), W_(a)
#define ROL_B(a, b)		rol B_(b), B_(a)
#define ROR_L(a, b)		ror L_(b), L_(a)
#define ROR_W(a, b)		ror W_(b), W_(a)
#define ROR_B(a, b)		ror B_(b), B_(a)
#define REP			rep
#define REPE			repe
#define REPNE			repne
#define REPNZ			REPNE
#define REPZ			REPE
#define RET			ret
#define SAHF			sahf
#define SAL_L(a, b)		sal L_(b), B_(a)
#define SAL_W(a, b)		sal W_(b), B_(a)
#define SAL_B(a, b)		sal B_(b), B_(a)
#define SAR_L(a, b)		sar L_(b), B_(a)
#define SAR_W(a, b)		sar W_(b), B_(a)
#define SAR_B(a, b)		sar B_(b), B_(a)
#define SBB_L(a, b)		sbb L_(b), L_(a)
#define SBB_W(a, b)		sbb W_(b), W_(a)
#define SBB_B(a, b)		sbb B_(b), B_(a)
#define SCAS_L			scas
#define SCAS_W			scas
#define SCAS_B			scas
#define SETA(a)			seta a
#define SETAE(a)		setae a
#define SETB(a)			setb a
#define SETBE(a)		setbe a
#define SETC(a)			setc a
#define SETE(a)			sete a
#define SETG(a)			setg a
#define SETGE(a)		setge a
#define SETL(a)			setl a
#define SETLE(a)		setle a
#define SETNA(a)		setna a
#define SETNAE(a)		setnae a
#define SETNB(a)		setnb a
#define SETNBE(a)		setnbe a
#define SETNC(a)		setnc a
#define SETNE(a)		setne a
#define SETNG(a)		setng a
#define SETNGE(a)		setnge a
#define SETNL(a)		setnl a
#define SETNLE(a)		setnle a
#define SETNO(a)		setno a
#define SETNP(a)		setnp a
#define SETNS(a)		setns a
#define SETNZ(a)		setnz a
#define SETO(a)			seto a
#define SETP(a)			setp a
#define SETPE(a)		setpe a
#define SETPO(a)		setpo a
#define SETS(a)			sets a
#define SETZ(a)			setz a
#define SGDT(a)			sgdt a
#define SIDT(a)			sidt a
#define SHL_L(a, b)		shl L_(b), B_(a)
#define SHL_W(a, b)		shl W_(b), B_(a)
#define SHL_B(a, b)		shl B_(b), B_(a)
#define SHLD_L(a,b,c)		shld
#define SHLD2_L(a,b)		shld L_(b), L_(a)
#define SHLD_W(a,b,c)		shld
#define SHLD2_W(a,b)		shld W_(b), W_(a)
#define SHR_L(a, b)		shr L_(b), B_(a)
#define SHR_W(a, b)		shr W_(b), B_(a)
#define SHR_B(a, b)		shr B_(b), B_(a)
#define SHRD_L(a,b,c)		shrd
#define SHRD2_L(a,b)		shrd L_(b), L_(a)
#define SHRD_W(a,b,c)		shrd
#define SHRD2_W(a,b)		shrd W_(b), W_(a)
#define SLDT(a)			sldt a
#define SMSW(a)			smsw a
#define STC			stc
#define STD			std
#define STI			sti
#define STOS_L			stosd
#define STOS_W			stosw
#define STOS_B			stosb
#define STR(a)			str a
#define SUB_L(a, b)		sub L_(b), L_(a)
#define SUB_W(a, b)		sub W_(b), W_(a)
#define SUB_B(a, b)		sub B_(b), B_(a)
#define TEST_L(a, b)		test L_(b), L_(a)
#define TEST_W(a, b)		test W_(b), W_(a)
#define TEST_B(a, b)		test B_(b), B_(a)
#define VERR(a)			verr a
#define VERW(a)			verw a
#define WAIT			wait
#define XCHG_L(a, b)		xchg L_(b), L_(a)
#define XCHG_W(a, b)		xchg W_(b), W_(a)
#define XCHG_B(a, b)		xchg B_(b), B_(a)
#define XLAT			xlat
#define XOR_L(a, b)		xor L_(b), L_(a)
#define XOR_W(a, b)		xor W_(b), W_(a)
#define XOR_B(a, b)		xor B_(b), B_(a)


/* Floating Point Instructions */
#define F2XM1			f2xm1
#define FABS			fabs
#define FADD_D(a)		fadd D_(a)
#define FADD_S(a)		fadd S_(a)
#define FADD2(a, b)		fadd b, a
#define FADDP(a, b)		faddp b, a
#define FIADD_L(a)		fiadd L_(a)
#define FIADD_W(a)		fiadd W_(a)
#define FBLD(a)			fbld a
#define FBSTP(a)		fbstp a
#define FCHS			fchs
#define FCLEX			fclex
#define FNCLEX			fnclex
#define FCOM(a)			fcom a
#define FCOM_D(a)		fcom D_(a)
#define FCOM_S(a)		fcom S_(a)
#define FCOMP(a)		fcomp a
#define FCOMP_D(a)		fcomp D_(a)
#define FCOMP_S(a)		fcomp S_(a)
#define FCOMPP			fcompp
#define FCOS			fcos
#define FDECSTP			fdecstp
#define FDIV_D(a)		fdiv D_(a)
#define FDIV_S(a)		fdiv S_(a)
#define FDIV2(a, b)		fdiv b, a
#define FDIVP(a, b)		fdivp b, a
#define FIDIV_L(a)		fidiv L_(a)
#define FIDIV_W(a)		fidiv W_(a)
#define FDIVR_D(a)		fdivr D_(a)
#define FDIVR_S(a)		fdivr S_(a)
#define FDIVR2(a, b)		fdivr b, a
#define FDIVRP(a, b)		fdivrp b, a
#define FIDIVR_L(a)		fidivr L_(a)
#define FIDIVR_W(a)		fidivr W_(a)
#define FFREE(a)		ffree a
#define FICOM_L(a)		ficom L_(a)
#define FICOM_W(a)		ficom W_(a)
#define FICOMP_L(a)		ficomp L_(a)
#define FICOMP_W(a)		ficomp W_(a)
#define FILD_Q(a)		fild D_(a)
#define FILD_L(a)		fild L_(a)
#define FILD_W(a)		fild W_(a)
#define FINCSTP			fincstp
#define FINIT			finit
#define FNINIT			fninit
#define FIST_L(a)		fist L_(a)
#define FIST_W(a)		fist W_(a)
#define FISTP_Q(a)		fistp D_(a)
#define FISTP_L(a)		fistp L_(a)
#define FISTP_W(a)		fistp W_(a)
#define FLD_X(a)		fld X_(a)
#define FLD_D(a)		fld D_(a)
#define FLD_S(a)		fld S_(a)
#define FLD1			fld1
#define FLDL2T			fldl2t
#define FLDL2E			fldl2e
#define FLDPI			fldpi
#define FLDLG2			fldlg2
#define FLDLN2			fldln2
#define FLDZ			fldz
#define FLDCW(a)		fldcw a
#define FLDENV(a)		fldenv a
#define FMUL_S(a)		fmul S_(a)
#define FMUL_D(a)		fmul D_(a)
#define FMUL2(a, b)		fmul b, a
#define FMULP(a, b)		fmulp b, a
#define FIMUL_L(a)		fimul L_(a)
#define FIMUL_W(a)		fimul W_(a)
#define FNOP			fnop
#define FPATAN			fpatan
#define FPREM			fprem
#define FPREM1			fprem1
#define FPTAN			fptan
#define FRNDINT			frndint
#define FRSTOR(a)		frstor a
#define FSAVE(a)		fsave a
#define FNSAVE(a)		fnsave a
#define FSCALE			fscale
#define FSIN			fsin
#define FSINCOS			fsincos
#define FSQRT			fsqrt
#define FST_D(a)		fst D_(a)
#define FST_S(a)		fst S_(a)
#define FSTP_X(a)		fstp X_(a)
#define FSTP_D(a)		fstp D_(a)
#define FSTP_S(a)		fstp S_(a)
#define FSTP(a)			fstp a
#define FSTCW(a)		fstcw a
#define FNSTCW(a)		fnstcw a
#define FSTENV(a)		fstenv a
#define FNSTENV(a)		fnstenv a
#define FSTSW(a)		fstsw a
#define FNSTSW(a)		fnstsw a
#define FSUB_S(a)		fsub S_(a)
#define FSUB_D(a)		fsub D_(a)
#define FSUB2(a, b)		fsub b, a
#define FSUBP(a, b)		fsubp b, a
#define FISUB_L(a)		fisub L_(a)
#define FISUB_W(a)		fisub W_(a)
#define FSUBR_S(a)		fsubr S_(a)
#define FSUBR_D(a)		fsubr D_(a)
#define FSUBR2(a, b)		fsubr b, a
#define FSUBRP(a, b)		fsubrp b, a
#define FISUBR_L(a)		fisubr L_(a)
#define FISUBR_W(a)		fisubr W_(a)
#define FTST			ftst
#define FUCOM(a)		fucom a
#define FUCOMP(a)		fucomp a
#define FUCOMPP			fucompp
#define FWAIT			fwait
#define FXAM			fxam
#define FXCH(a)			fxch a
#define FXTRACT			fxtract
#define FYL2X			fyl2x
#define FYL2XP1			fyl2xp1

#endif /* NASM_ASSEMBLER, MASM_ASSEMBLER */

	/****************************************/
	/*					*/
	/*	Extensions to x86 insn set -	*/
	/*	MMX, 3DNow!			*/
	/*					*/
	/****************************************/

#if defined(NASM_ASSEMBLER) || defined(MASM_ASSEMBLER)
#define P_ARG1(a)		P_ ## a
#define P_ARG2(a, b)		P_ ## b, P_ ## a
#define P_ARG3(a, b, c)		P_ ## c, P_ ## b, P_ ## a
#else
#define P_ARG1(a)		a
#define P_ARG2(a, b)		a, b
#define P_ARG3(a, b, c)		a, b, c
#endif

/* MMX */
#define MOVD(a, b)		movd P_ARG2(a, b)
#define MOVQ(a, b)		movq P_ARG2(a, b)

#define PADDB(a, b)		paddb P_ARG2(a, b)
#define PADDW(a, b)		paddw P_ARG2(a, b)
#define PADDD(a, b)		paddd P_ARG2(a, b)

#define PADDSB(a, b)		paddsb P_ARG2(a, b)
#define PADDSW(a, b)		paddsw P_ARG2(a, b)

#define PADDUSB(a, b)		paddusb P_ARG2(a, b)
#define PADDUSW(a, b)		paddusw P_ARG2(a, b)

#define PSUBB(a, b)		psubb P_ARG2(a, b)
#define PSUBW(a, b)		psubw P_ARG2(a, b)
#define PSUBD(a, b)		psubd P_ARG2(a, b)

#define PSUBSB(a, b)		psubsb P_ARG2(a, b)
#define PSUBSW(a, b)		psubsw P_ARG2(a, b)

#define PSUBUSB(a, b)		psubusb P_ARG2(a, b)
#define PSUBUSW(a, b)		psubusw P_ARG2(a, b)

#define PCMPEQB(a, b)		pcmpeqb P_ARG2(a, b)
#define PCMPEQW(a, b)		pcmpeqw P_ARG2(a, b)
#define PCMPEQD(a, b)		pcmpeqd P_ARG2(a, b)

#define PCMPGTB(a, b)		pcmpgtb P_ARG2(a, b)
#define PCMPGTW(a, b)		pcmpgtw P_ARG2(a, b)
#define PCMPGTD(a, b)		pcmpgtd P_ARG2(a, b)

#define PMULHW(a, b)		pmulhw P_ARG2(a, b)
#define PMULLW(a, b)		pmullw P_ARG2(a, b)

#define PMADDWD(a, b)		pmaddwd P_ARG2(a, b)

#define PAND(a, b)		pand P_ARG2(a, b)

#define PANDN(a, b)		pandn P_ARG2(a, b)

#define POR(a, b)		por P_ARG2(a, b)

#define PXOR(a, b)		pxor P_ARG2(a, b)

#define PSRAW(a, b)		psraw P_ARG2(a, b)
#define PSRAD(a, b)		psrad P_ARG2(a, b)

#define PSRLW(a, b)		psrlw P_ARG2(a, b)
#define PSRLD(a, b)		psrld P_ARG2(a, b)
#define PSRLQ(a, b)		psrlq P_ARG2(a, b)

#define PSLLW(a, b)		psllw P_ARG2(a, b)
#define PSLLD(a, b)		pslld P_ARG2(a, b)
#define PSLLQ(a, b)		psllq P_ARG2(a, b)

#define PACKSSWB(a, b)		packsswb P_ARG2(a, b)
#define PACKSSDW(a, b)		packssdw P_ARG2(a, b)
#define PACKUSWB(a, b)		packuswb P_ARG2(a, b)

#define PUNPCKHBW(a, b)		punpckhbw P_ARG2(a, b)
#define PUNPCKHWD(a, b)		punpckhwd P_ARG2(a, b)
#define PUNPCKHDQ(a, b)		punpckhdq P_ARG2(a, b)
#define PUNPCKLBW(a, b)		punpcklbw P_ARG2(a, b)
#define PUNPCKLWD(a, b)		punpcklwd P_ARG2(a, b)
#define PUNPCKLDQ(a, b)		punpckldq P_ARG2(a, b)

#define EMMS			emms

/* AMD 3DNow! */
#define PAVGUSB(a, b)		pavgusb P_ARG2(a, b)
#define PFADD(a, b)		pfadd P_ARG2(a, b)
#define PFSUB(a, b)		pfsub P_ARG2(a, b)
#define PFSUBR(a, b)		pfsubr P_ARG2(a, b)
#define PFACC(a, b)		pfacc P_ARG2(a, b)
#define PFCMPGE(a, b)		pfcmpge P_ARG2(a, b)
#define PFCMPGT(a, b)		pfcmpgt P_ARG2(a, b)
#define PFCMPEQ(a, b)		pfcmpeq P_ARG2(a, b)
#define PFMIN(a, b)		pfmin P_ARG2(a, b)
#define PFMAX(a, b)		pfmax P_ARG2(a, b)
#define PI2FD(a, b)		pi2fd P_ARG2(a, b)
#define PF2ID(a, b)		pf2id P_ARG2(a, b)
#define PFRCP(a, b)		pfrcp P_ARG2(a, b)
#define PFRSQRT(a, b)		pfrsqrt P_ARG2(a, b)
#define PFMUL(a, b)		pfmul P_ARG2(a, b)
#define PFRCPIT1(a, b)		pfrcpit1 P_ARG2(a, b)
#define PFRSQIT1(a, b)		pfrsqit1 P_ARG2(a, b)
#define PFRCPIT2(a, b)		pfrcpit2 P_ARG2(a, b)
#define PMULHRW(a, b)		pmulhrw P_ARG2(a, b)

#define FEMMS			femms
#define PREFETCH(a)		prefetch P_ARG1(a)
#define PREFETCHW(a)		prefetchw P_ARG1(a)

/* Intel SSE */
#define ADDPS(a, b)		addps P_ARG2(a, b)
#define ADDSS(a, b)		addss P_ARG2(a, b)
#define ANDNPS(a, b)		andnps P_ARG2(a, b)
#define ANDPS(a, b)		andps P_ARG2(a, b)
/* NASM only knows the pseudo ops for these.
#define CMPPS(a, b, c)		cmpps P_ARG3(a, b, c)
#define CMPSS(a, b, c)		cmpss P_ARG3(a, b, c)
*/
#define CMPEQPS(a, b)		cmpeqps P_ARG2(a, b)
#define CMPLTPS(a, b)		cmpltps P_ARG2(a, b)
#define CMPLEPS(a, b)		cmpleps P_ARG2(a, b)
#define CMPUNORDPS(a, b)	cmpunordps P_ARG2(a, b)
#define CMPNEQPS(a, b)		cmpneqps P_ARG2(a, b)
#define CMPNLTPS(a, b)		cmpnltps P_ARG2(a, b)
#define CMPNLEPS(a, b)		cmpnleps P_ARG2(a, b)
#define CMPORDPS(a, b)		cmpordps P_ARG2(a, b)
#define CMPEQSS(a, b)		cmpeqss P_ARG2(a, b)
#define CMPLTSS(a, b)		cmpltss P_ARG2(a, b)
#define CMPLESS(a, b)		cmpless P_ARG2(a, b)
#define CMPUNORDSS(a, b)	cmpunordss P_ARG2(a, b)
#define CMPNEQSS(a, b)		cmpneqss P_ARG2(a, b)
#define CMPNLTSS(a, b)		cmpnltss P_ARG2(a, b)
#define CMPNLESS(a, b)		cmpnless P_ARG2(a, b)
#define CMPORDSS(a, b)		cmpordss P_ARG2(a, b)
#define COMISS(a, b)		comiss P_ARG2(a, b)
#define CVTPI2PS(a, b)		cvtpi2ps P_ARG2(a, b)
#define CVTPS2PI(a, b)		cvtps2pi P_ARG2(a, b)
#define CVTSI2SS(a, b)		cvtsi2ss P_ARG2(a, b)
#define CVTSS2SI(a, b)		cvtss2si P_ARG2(a, b)
#define CVTTPS2PI(a, b)		cvttps2pi P_ARG2(a, b)
#define CVTTSS2SI(a, b)		cvttss2si P_ARG2(a, b)
#define DIVPS(a, b)		divps P_ARG2(a, b)
#define DIVSS(a, b)		divss P_ARG2(a, b)
#define FXRSTOR(a)		fxrstor P_ARG1(a)
#define FXSAVE(a)		fxsave P_ARG1(a)
#define LDMXCSR(a)		ldmxcsr P_ARG1(a)
#define MAXPS(a, b)		maxps P_ARG2(a, b)
#define MAXSS(a, b)		maxss P_ARG2(a, b)
#define MINPS(a, b)		minps P_ARG2(a, b)
#define MINSS(a, b)		minss P_ARG2(a, b)
#define MOVAPS(a, b)		movaps P_ARG2(a, b)
#define MOVHLPS(a, b)		movhlps P_ARG2(a, b)
#define MOVHPS(a, b)		movhps P_ARG2(a, b)
#define MOVLHPS(a, b)		movlhps P_ARG2(a, b)
#define MOVLPS(a, b)		movlps P_ARG2(a, b)
#define MOVMSKPS(a, b)		movmskps P_ARG2(a, b)
#define MOVNTPS(a, b)		movntps P_ARG2(a, b)
#define MOVNTQ(a, b)		movntq P_ARG2(a, b)
#define MOVSS(a, b)		movss P_ARG2(a, b)
#define MOVUPS(a, b)		movups P_ARG2(a, b)
#define MULPS(a, b)		mulps P_ARG2(a, b)
#define MULSS(a, b)		mulss P_ARG2(a, b)
#define ORPS(a, b)		orps P_ARG2(a, b)
#define RCPPS(a, b)		rcpps P_ARG2(a, b)
#define RCPSS(a, b)		rcpss P_ARG2(a, b)
#define RSQRTPS(a, b)		rsqrtps P_ARG2(a, b)
#define RSQRTSS(a, b)		rsqrtss P_ARG2(a, b)
#define SHUFPS(a, b, c)		shufps P_ARG3(a, b, c)
#define SQRTPS(a, b)		sqrtps P_ARG2(a, b)
#define SQRTSS(a, b)		sqrtss P_ARG2(a, b)
#define STMXCSR(a)		stmxcsr P_ARG1(a)
#define SUBPS(a, b)		subps P_ARG2(a, b)
#define UCOMISS(a, b)		ucomiss P_ARG2(a, b)
#define UNPCKHPS(a, b)		unpckhps P_ARG2(a, b)
#define UNPCKLPS(a, b)		unpcklps P_ARG2(a, b)
#define XORPS(a, b)		xorps P_ARG2(a, b)

#define PREFETCHNTA(a)		prefetchnta P_ARG1(a)
#define PREFETCHT0(a)		prefetcht0 P_ARG1(a)
#define PREFETCHT1(a)		prefetcht1 P_ARG1(a)
#define PREFETCHT2(a)		prefetcht2 P_ARG1(a)
#define SFENCE			sfence

/* Added by BrianP for FreeBSD (per David Dawes) */
#if !defined(NASM_ASSEMBLER) && !defined(MASM_ASSEMBLER) && !defined(__bsdi__)
#define LLBL(a)		CONCAT(.L,a)
#define LLBL2(a,b)	CONCAT3(.L,a,b)
#else
#define LLBL(a)		a
#define LLBL2(a,b)	CONCAT(a,b)
#endif

/* Segment overrides */
#define SEGCS		D_BYTE	46
#define SEGDS		D_BYTE	62
#define SEGES		D_BYTE	38
#define SEGFS		D_BYTE	100
#define SEGGS		D_BYTE	101

/* Temporary labels: valid until next non-local label */
#ifdef NASM_ASSEMBLER
#define TLBL(a)		CONCAT(.,a)
#else
#define TLBL(a)		CONCAT(a,$)
#endif

/* Hidden symbol visibility support.
 * If we build with gcc's -fvisibility=hidden flag, we'll need to change
 * the symbol visibility mode to 'default'.
 */
#if defined(GNU_ASSEMBLER) && !defined(__DJGPP__) && !defined(__MINGW32__)
#  define HIDDEN(x) .hidden x
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303 && !defined(__DJGPP__) && !defined(__MINGW32__)
#  pragma GCC visibility push(default)
#  define HIDDEN(x) .hidden x
#else
#  define HIDDEN(x)
#endif

#endif /* __ASSYNTAX_H__ */
