/*
	mangle: support defines for preprocessed assembler

	copyright 1995-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	This once started out as mangle.h from MPlayer, but you can't really call it derived work... the small part that in principle stems from MPlayer also being not very special (once you decided to use such a header at all, it's quite obvious material).
*/

#ifndef __MANGLE_H
#define __MANGLE_H

#include "config.h"
#include "intsym.h"

#if (defined OPT_I486)  || (defined OPT_I586) || (defined OPT_I586_DITHER) \
 || (defined OPT_MMX)   || (defined OPT_SSE)  || (defined OPT_3DNOW) || (defined OPT_3DNOWEXT) \
 || (defined OPT_3DNOW_VINTAGE) || (defined OPT_3DNOWEXT_VINTAGE) \
 || (defined OPT_SSE_VINTAGE)
#define OPT_X86
#endif

#ifdef CCALIGN
#define MOVUAPS movaps
#else
#define MOVUAPS movups
#endif

/*
	ALIGNX: align to X bytes
	This differs per compiler/platform in taking the byte count or an exponent for base 2.
	A way out is balign, if the assembler supports it (gas extension).
*/

#ifdef ASMALIGN_BALIGN

#define ALIGN4  .balign 4
#define ALIGN8  .balign 8
#define ALIGN16 .balign 16
#define ALIGN32 .balign 32
#define ALIGN64 .balign 64

#else

#ifdef ASMALIGN_EXP
#define ALIGN4  .align 2
#define ALIGN8  .align 3
#define ALIGN16 .align 4
#define ALIGN32 .align 5
#define ALIGN64 .align 6
#else
#ifdef ASMALIGN_BYTE
#define ALIGN4  .align 4
#define ALIGN8  .align 8
#define ALIGN16 .align 16
#define ALIGN32 .align 32
#define ALIGN64 .align 64
#else
#ifdef ASMALIGN_ARMASM
#define ALIGN4  ALIGN 4
#define ALIGN8  ALIGN 8
#define ALIGN16 ALIGN 16
#define ALIGN32 ALIGN 32
#define ALIGN64 ALIGN 64
#else
#error "Dunno how assembler alignment works. Please specify."
#endif
#endif
#endif

#endif

#define MANGLE_MACROCAT_REALLY(a, b) a ## b
#define MANGLE_MACROCAT(a, b) MANGLE_MACROCAT_REALLY(a, b)
/* Feel free to add more to the list, eg. a.out IMO */
#if defined(__USER_LABEL_PREFIX__)
#define ASM_NAME(a) MANGLE_MACROCAT(__USER_LABEL_PREFIX__,a)
#define ASM_VALUE(a) MANGLE_MACROCAT($,ASM_NAME(a))
#elif defined(__CYGWIN__) || defined(_WIN32) && !defined (_WIN64) && !defined (_M_ARM) || defined(__OS2__) || \
   (defined(__OpenBSD__) && !defined(__ELF__)) || defined(__APPLE__)
#define ASM_NAME(a) MANGLE_MACROCAT(_,a)
#define ASM_VALUE(a) MANGLE_MACROCAT($_,a)
#else
#define ASM_NAME(a) a
#define ASM_VALUE(a) MANGLE_MACROCAT($,a)
#endif

/* Enable position-independent code for certain platforms. */

#if defined(OPT_X86)

#define _EBX_ %ebx

#if defined(PIC) && defined(__ELF__)

/* ELF binaries (Unix/Linux) */
#define LOCAL_VAR(a) a ## @GOTOFF(_EBX_)
#define GLOBAL_VAR(a) ASM_NAME(a) ## @GOTOFF(_EBX_)
#define GLOBAL_VAR_PTR(a) ASM_NAME(a) ## @GOT(_EBX_)
#define FUNC(a) ASM_NAME(a)
#define EXTERNAL_FUNC(a) ASM_NAME(a) ## @PLT
#undef ASM_VALUE
#define ASM_VALUE(a) MANGLE_MACROCAT($,a) ##@GOTOFF
#define GET_GOT \
	call 1f; \
1: \
	pop _EBX_; \
2: \
	addl $_GLOBAL_OFFSET_TABLE_ + (2b-1b), _EBX_
#define PREPARE_GOT pushl _EBX_
#define RESTORE_GOT popl _EBX_

#elif defined(PIC) && defined(__APPLE__)

/* Mach-O binaries (OSX/iOS) */
#define LOCAL_VAR(a) a ## - Lpic_base(_EBX_)
#define GLOBAL_VAR(a) .err This ABI cannot access non-local symbols directly.
#define GLOBAL_VAR_PTR(a) L_ ## a ## - Lpic_base(_EBX_)
#define FUNC(a) L_ ## a
#define EXTERNAL_FUNC(a) L_ ## a
#define GET_GOT \
	call Lpic_base; \
Lpic_base: \
	pop _EBX_
#define PREPARE_GOT pushl _EBX_
#define RESTORE_GOT popl _EBX_

#else

/* Dummies for everyone else. */
#define LOCAL_VAR(a) a
#define GLOBAL_VAR ASM_NAME
#define GLOBAL_VAR_PTR(a) .err Cannot use indirect addressing in non-PIC object.
#define FUNC ASM_NAME
#define EXTERNAL_FUNC ASM_NAME
#define GET_GOT
#define PREPARE_GOT
#define RESTORE_GOT

#endif /* PIC variants */

#endif /* OPT_X86 */

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__)
#define COMM(a,b,c) .comm a,b
#else
#define COMM(a,b,c) .comm a,b,c
#endif
/* more hacks for macosx; no .bss ... */
#ifdef __APPLE__
#define BSS .data
#else
#define BSS .bss
#endif

/* armasm for WIN32 UWP */
#ifdef _M_ARM
#define GLOBAL_SYMBOL EXPORT
#else
#define GLOBAL_SYMBOL .globl
#endif

/* Mark non-executable stack.
   It's mainly for GNU on Linux... who else does (not) like this? */
#if !defined(__SUNPRO_C) && defined(__linux__) && defined(__ELF__)
#if defined(__arm__)
#define NONEXEC_STACK .section .note.GNU-stack,"",%progbits
#else
#define NONEXEC_STACK .section .note.GNU-stack,"",@progbits
#endif
#else
#define NONEXEC_STACK
#endif

#if (defined(__x86_64__) || defined(_M_X64)) && (defined(_WIN64) || defined (__CYGWIN__))
#define IS_MSABI 1 /* Not using SYSV */
#endif

/* Macros for +-4GiB PC-relative addressing on AArch64 */
#ifdef __APPLE__
#define AARCH64_PCREL_HI(label) label@PAGE
#define AARCH64_PCREL_LO(label) label@PAGEOFF
#else
#define AARCH64_PCREL_HI(label) label
#define AARCH64_PCREL_LO(label) :lo12:label
#endif

#ifdef __APPLE__
#define AARCH64_DUP_4S(dst, src, elem) dup.4s dst, src[elem]
#define AARCH64_DUP_2D(dst, src, elem) dup.2d dst, src[elem]
#define AARCH64_SQXTN2_8H(dst, src) sqxtn2.8h dst, src
#else
#define AARCH64_DUP_4S(dst, src, elem) dup dst.4s, src.s[elem]
#define AARCH64_DUP_2D(dst, src, elem) dup dst.2d, src.d[elem]
#define AARCH64_SQXTN2_8H(dst, src) sqxtn2 dst.8h, src.4s
#endif

#endif /* !__MANGLE_H */

