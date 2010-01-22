/*-
 * Copyright (c) 1996-1998 John D. Polstra.
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
 * $FreeBSD: src/sys/sys/elf64.h,v 1.10 2002/05/30 08:32:18 dfr Exp $
 */

#ifndef _SYS_ELF64_H_
#define _SYS_ELF64_H_ 1

/*
 * ELF definitions common to all 64-bit architectures.
 */

typedef ULONG64	Elf64_Addr;
typedef ULONG32	Elf64_Half;
typedef ULONG64	Elf64_Off;
typedef LONG64	Elf64_Sword;
typedef ULONG64	Elf64_Word;
typedef ULONG64	Elf64_Size;
typedef USHORT	Elf64_Quarter;

/*
 * Types of dynamic symbol hash table bucket and chain elements.
 *
 * This is inconsistent among 64 bit architectures, so a machine dependent
 * typedef is required.
 */

#ifdef _ALPHA_
typedef Elf64_Off	Elf64_Hashelt;
#else
typedef Elf64_Half	Elf64_Hashelt;
#endif

/*
 * ELF header.
 */

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* File identification. */
	Elf64_Quarter	e_type;		/* File type. */
	Elf64_Quarter	e_machine;	/* Machine architecture. */
	Elf64_Half	e_version;	/* ELF format version. */
	Elf64_Addr	e_entry;	/* Entry point. */
	Elf64_Off	e_phoff;	/* Program header file offset. */
	Elf64_Off	e_shoff;	/* Section header file offset. */
	Elf64_Half	e_flags;	/* Architecture-specific flags. */
	Elf64_Quarter	e_ehsize;	/* Size of ELF header in bytes. */
	Elf64_Quarter	e_phentsize;	/* Size of program header entry. */
	Elf64_Quarter	e_phnum;	/* Number of program header entries. */
	Elf64_Quarter	e_shentsize;	/* Size of section header entry. */
	Elf64_Quarter	e_shnum;	/* Number of section header entries. */
	Elf64_Quarter	e_shstrndx;	/* Section name strings section. */
} Elf64_Ehdr;

/*
 * Section header.
 */

typedef struct {
	Elf64_Half	sh_name;	/* Section name (index into the
					   section header string table). */
	Elf64_Half	sh_type;	/* Section type. */
	Elf64_Size	sh_flags;	/* Section flags. */
	Elf64_Addr	sh_addr;	/* Address in memory image. */
	Elf64_Off	sh_offset;	/* Offset in file. */
	Elf64_Size	sh_size;	/* Size in bytes. */
	Elf64_Half	sh_link;	/* Index of a related section. */
	Elf64_Half	sh_info;	/* Depends on section type. */
	Elf64_Size	sh_addralign;	/* Alignment in bytes. */
	Elf64_Size	sh_entsize;	/* Size of each entry in section. */
} Elf64_Shdr;

/*
 * Program header.
 */

typedef struct {
	Elf64_Half	p_type;		/* Entry type. */
	Elf64_Half	p_flags;	/* Access permission flags. */
	Elf64_Off	p_offset;	/* File offset of contents. */
	Elf64_Addr	p_vaddr;	/* Virtual address in memory image. */
	Elf64_Addr	p_paddr;	/* Physical address (not used). */
	Elf64_Size	p_filesz;	/* Size of contents in file. */
	Elf64_Size	p_memsz;	/* Size of contents in memory. */
	Elf64_Size	p_align;	/* Alignment in memory and file. */
} Elf64_Phdr;

/*
 * Dynamic structure.  The ".dynamic" section contains an array of them.
 */

typedef struct {
	Elf64_Size	d_tag;		/* Entry type. */
	union {
		Elf64_Size	d_val;	/* Integer value. */
		Elf64_Addr	d_ptr;	/* Address value. */
	} d_un;
} Elf64_Dyn;

/*
 * Relocation entries.
 */

/* Relocations that don't need an addend field. */
typedef struct {
	Elf64_Addr	r_offset;	/* Location to be relocated. */
	Elf64_Size	r_info;		/* Relocation type and symbol index. */
} Elf64_Rel;

/* Relocations that need an addend field. */
typedef struct {
	Elf64_Addr	r_offset;	/* Location to be relocated. */
	Elf64_Size	r_info;		/* Relocation type and symbol index. */
	Elf64_Off	r_addend;	/* Addend. */
} Elf64_Rela;

/* Macros for accessing the fields of r_info. */
#define ELF64_R_SYM(info)	((info) >> 32)
#define ELF64_R_TYPE(info)	((unsigned char)(info))

/* Macro for constructing r_info from field values. */
#define ELF64_R_INFO(sym, type)	(((sym) << 32) + (unsigned char)(type))

/*
 * Symbol table entries.
 */

typedef struct {
	Elf64_Half	st_name;	/* String table index of name. */
	unsigned char	st_info;	/* Type and binding information. */
	unsigned char	st_other;	/* Reserved (not used). */
	Elf64_Quarter	st_shndx;	/* Section index of symbol. */
	Elf64_Addr	st_value;	/* Symbol value. */
	Elf64_Size	st_size;	/* Size of associated object. */
} Elf64_Sym;

/* Macros for accessing the fields of st_info. */
#define ELF64_ST_BIND(info)		((info) >> 4)
#define ELF64_ST_TYPE(info)		((info) & 0xf)

/* Macro for constructing st_info from field values. */
#define ELF64_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

#endif /* !_SYS_ELF64_H_ */
