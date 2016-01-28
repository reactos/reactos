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
#error "Dunno how assembler alignment works. Please specify."
#endif
#endif

#endif

#define MANGLE_MACROCAT_REALLY(a, b) a ## b
#define MANGLE_MACROCAT(a, b) MANGLE_MACROCAT_REALLY(a, b)
/* Feel free to add more to the list, eg. a.out IMO */
#if defined(__USER_LABEL_PREFIX__)
#define ASM_NAME(a) MANGLE_MACROCAT(__USER_LABEL_PREFIX__,a)
#define ASM_VALUE(a) MANGLE_MACROCAT($,ASM_NAME(a))
#elif defined(__CYGWIN__) || defined(_WIN32) && !defined (_WIN64) || defined(__OS2__) || \
   (defined(__OpenBSD__) && !defined(__ELF__)) || defined(__APPLE__)
#define ASM_NAME(a) MANGLE_MACROCAT(_,a)
#define ASM_VALUE(a) MANGLE_MACROCAT($_,a)
#else
#define ASM_NAME(a) a
#define ASM_VALUE(a) MANGLE_MACROCAT($,a)
#endif

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

#if defined(__x86_64__) && (defined(_WIN64) || defined (__CYGWIN__))
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

