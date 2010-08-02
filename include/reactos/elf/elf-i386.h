/*-
 * Copyright (c) 1996-1997 John D. Polstra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/i386/include/elf.h,v 1.16 2004/08/02 19:12:17 dfr Exp $
 */

#ifndef _MACHINE_ELF_H_
#define	_MACHINE_ELF_H_ 1

/*
 * ELF definitions for the i386 architecture.
 */

#ifdef _REACTOS_ELF_MACHINE_IS_TARGET

#ifndef __ELF_WORD_SIZE
#define	__ELF_WORD_SIZE	32	/* Used by <elf/generic.h> */
#endif

#include <elf/generic.h>

#define	ELF_ARCH	EM_386

#define	ELF_MACHINE_OK(x) ((x) == EM_386 || (x) == EM_486)

/*
 * Auxiliary vector entries for passing information to the interpreter.
 *
 * The i386 supplement to the SVR4 ABI specification names this "auxv_t",
 * but POSIX lays claim to all symbols ending with "_t".
 */

typedef struct {	/* Auxiliary vector entry on initial stack */
	int	a_type;			/* Entry type. */
	union {
		long	a_val;		/* Integer value. */
		void	*a_ptr;		/* Address. */
		void	(*a_fcn)(void);	/* Function pointer (not used). */
	} a_un;
} Elf32_Auxinfo;

#if __ELF_WORD_SIZE == 64
/* Fake for amd64 loader support */
typedef struct {
	int fake;
} Elf64_Auxinfo;
#endif

__ElfType(Auxinfo);

/* Values for a_type. */
#define	AT_NULL		0	/* Terminates the vector. */
#define	AT_IGNORE	1	/* Ignored entry. */
#define	AT_EXECFD	2	/* File descriptor of program to load. */
#define	AT_PHDR		3	/* Program header of program already loaded. */
#define	AT_PHENT	4	/* Size of each program header entry. */
#define	AT_PHNUM	5	/* Number of program header entries. */
#define	AT_PAGESZ	6	/* Page size in bytes. */
#define	AT_BASE		7	/* Interpreter's base address. */
#define	AT_FLAGS	8	/* Flags (unused for i386). */
#define	AT_ENTRY	9	/* Where interpreter should transfer control. */

/*
 * The following non-standard values are used for passing information
 * from John Polstra's testbed program to the dynamic linker.  These
 * are expected to go away soon.
 *
 * Unfortunately, these overlap the Linux non-standard values, so they
 * must not be used in the same context.
 */
#define	AT_BRK		10	/* Starting point for sbrk and brk. */
#define	AT_DEBUG	11	/* Debugging level. */

/*
 * The following non-standard values are used in Linux ELF binaries.
 */
#define	AT_NOTELF	10	/* Program is not ELF ?? */
#define	AT_UID		11	/* Real uid. */
#define	AT_EUID		12	/* Effective uid. */
#define	AT_GID		13	/* Real gid. */
#define	AT_EGID		14	/* Effective gid. */

#define	AT_COUNT	15	/* Count of defined aux entry types. */

/* Define "machine" characteristics */
#define	ELF_TARG_CLASS	ELFCLASS32
#define	ELF_TARG_DATA	ELFDATA2LSB
#define	ELF_TARG_MACH	EM_386
#define	ELF_TARG_VER	1

#endif /* _REACTOS_ELF_MACHINE_IS_TARGET */

/*
 * Relocation types.
 */

#define	R_386_NONE	0	/* No relocation. */
#define	R_386_32	1	/* Add symbol value. */
#define	R_386_PC32	2	/* Add PC-relative symbol value. */
#define	R_386_GOT32	3	/* Add PC-relative GOT offset. */
#define	R_386_PLT32	4	/* Add PC-relative PLT offset. */
#define	R_386_COPY	5	/* Copy data from shared object. */
#define	R_386_GLOB_DAT	6	/* Set GOT entry to data address. */
#define	R_386_JMP_SLOT	7	/* Set GOT entry to code address. */
#define	R_386_RELATIVE	8	/* Add load address of shared object. */
#define	R_386_GOTOFF	9	/* Add GOT-relative symbol address. */
#define	R_386_GOTPC	10	/* Add PC-relative GOT table address. */
#define	R_386_TLS_TPOFF	14	/* Negative offset in static TLS block */
#define	R_386_TLS_IE	15	/* Absolute address of GOT for -ve static TLS */
#define	R_386_TLS_GOTIE	16	/* GOT entry for negative static TLS block */
#define	R_386_TLS_LE	17	/* Negative offset relative to static TLS */
#define	R_386_TLS_GD	18	/* 32 bit offset to GOT (index,off) pair */
#define	R_386_TLS_LDM	19	/* 32 bit offset to GOT (index,zero) pair */
#define	R_386_TLS_GD_32	24	/* 32 bit offset to GOT (index,off) pair */
#define	R_386_TLS_GD_PUSH 25	/* pushl instruction for Sun ABI GD sequence */
#define	R_386_TLS_GD_CALL 26	/* call instruction for Sun ABI GD sequence */
#define	R_386_TLS_GD_POP 27	/* popl instruction for Sun ABI GD sequence */
#define	R_386_TLS_LDM_32 28	/* 32 bit offset to GOT (index,zero) pair */
#define	R_386_TLS_LDM_PUSH 29	/* pushl instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDM_CALL 30	/* call instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDM_POP 31	/* popl instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDO_32 32	/* 32 bit offset from start of TLS block */
#define	R_386_TLS_IE_32	33	/* 32 bit offset to GOT static TLS offset entry */
#define	R_386_TLS_LE_32	34	/* 32 bit offset within static TLS block */
#define	R_386_TLS_DTPMOD32 35	/* GOT entry containing TLS index */
#define	R_386_TLS_DTPOFF32 36	/* GOT entry containing TLS offset */
#define	R_386_TLS_TPOFF32 37	/* GOT entry of -ve static TLS offset */

#define	R_386_COUNT	38	/* Count of defined relocation types. */

#endif /* !_MACHINE_ELF_H_ */
