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
 * $FreeBSD: src/sys/powerpc/include/elf.h,v 1.16 2004/08/02 19:12:17 dfr Exp $
 */

#ifndef _MACHINE_ELF_H_
#define	_MACHINE_ELF_H_ 1

/*
 * ELF definitions for the powerpc architecture.
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
 * The powerpc supplement to the SVR4 ABI specification names this "auxv_t",
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
#define	AT_FLAGS	8	/* Flags (unused for powerpc). */
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

#define R_PPC_NONE            0
#define R_PPC_ADDR32          1
#define R_PPC_ADDR24          2
#define R_PPC_ADDR16          3
#define R_PPC_ADDR16_LO       4
#define R_PPC_ADDR16_HI       5
#define R_PPC_ADDR16_HA       6
#define R_PPC_ADDR14          7
#define R_PPC_ADDR14_BRTAKEN  8
#define R_PPC_ADDR14_BRNTAKEN 9
#define R_PPC_REL24           10
#define R_PPC_REL14           11
#define R_PPC_REL14_BRTAKEN   12
#define R_PPC_REL14_BRNTAKEN  13
#define R_PPC_GOT16           14
#define R_PPC_GOT16_LO        15
#define R_PPC_GOT16_HI        16
#define R_PPC_GOT16_HA        17
#define R_PPC_PLTREL24        18
#define R_PPC_COPY            19
#define R_PPC_GLOB_DAT        20
#define R_PPC_JMP_SLOT        21
#define R_PPC_RELATIVE        22
#define R_PPC_LOCAL24PC       23
#define R_PPC_UADDR32         24
#define R_PPC_UADDR16         25
#define R_PPC_REL32           26
#define R_PPC_PLT32           27
#define R_PPC_PLTREL32        28
#define R_PPC_PLT16_LO        29
#define R_PPC_PLT16_HI        30
#define R_PPC_PLT16_HA        31
#define R_PPC_SDAREL16        32
#define R_PPC_SECTOFF         33
#define R_PPC_SECTOFF_LO      34
#define R_PPC_SECTOFF_HI      35
#define R_PPC_SECTOFF_HA      36
#define	R_PPC_COUNT	      37

#endif /* !_MACHINE_ELF_H_ */
