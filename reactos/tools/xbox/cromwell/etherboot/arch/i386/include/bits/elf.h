#ifndef I386_BITS_ELF_H
#define I386_BITS_ELF_H

#include "cpu.h"

#ifdef CONFIG_X86_64
/* ELF Defines for the 64bit version of the current architecture */
#define EM_CURRENT_64	EM_X86_64
#define EM_CURRENT_64_PRESENT ( \
	CPU_FEATURE_P(cpu_info.x86_capability, LM) && \
	CPU_FEATURE_P(cpu_info.x86_capability, PAE) && \
	CPU_FEATURE_P(cpu_info.x86_capability, PSE))
			
#define ELF_CHECK_X86_64_ARCH(x) \
	(EM_CURRENT_64_PRESENT && ((x).e_machine == EM_X86_64))
#define __unused_i386
#else
#define ELF_CHECK_X86_64_ARCH(x) 0
#define __unused_i386 __unused
#endif


/* ELF Defines for the current architecture */
#define	EM_CURRENT	EM_386
#define ELFDATA_CURRENT	ELFDATA2LSB

#define ELF_CHECK_I386_ARCH(x) \
	(((x).e_machine == EM_386) || ((x).e_machine == EM_486))

#define ELF_CHECK_ARCH(x) \
	((ELF_CHECK_I386_ARCH(x) || ELF_CHECK_X86_64_ARCH(x)) && \
		((x).e_entry <= 0xffffffffUL))

#ifdef  IMAGE_FREEBSD
/*
 * FreeBSD has this rather strange "feature" of its design.
 * At some point in its evolution, FreeBSD started to rely
 * externally on private/static/debug internal symbol information.
 * That is, some of the interfaces that software uses to access
 * and work with the FreeBSD kernel are made available not
 * via the shared library symbol information (the .DYNAMIC section)
 * but rather the debug symbols.  This means that any symbol, not
 * just publicly defined symbols can be (and are) used by system
 * tools to make the system work.  (such as top, swapinfo, swapon,
 * etc)
 *
 * Even worse, however, is the fact that standard ELF loaders do
 * not know how to load the symbols since they are not within
 * an ELF PT_LOAD section.  The kernel needs these symbols to
 * operate so the following changes/additions to the boot
 * loading of EtherBoot have been made to get the kernel to load.
 * All of the changes are within IMAGE_FREEBSD such that the
 * extra/changed code only compiles when FREEBSD support is
 * enabled.
 */

/*
 * Section header for FreeBSD (debug symbol kludge!) support
 */
typedef struct {
	Elf32_Word	sh_name;	/* Section name (index into the
					   section header string table). */
	Elf32_Word	sh_type;	/* Section type. */
	Elf32_Word	sh_flags;	/* Section flags. */
	Elf32_Addr	sh_addr;	/* Address in memory image. */
	Elf32_Off	sh_offset;	/* Offset in file. */
	Elf32_Size	sh_size;	/* Size in bytes. */
	Elf32_Word	sh_link;	/* Index of a related section. */
	Elf32_Word	sh_info;	/* Depends on section type. */
	Elf32_Size	sh_addralign;	/* Alignment in bytes. */
	Elf32_Size	sh_entsize;	/* Size of each entry in section. */
} Elf32_Shdr;

/* sh_type */
#define SHT_SYMTAB	2		/* symbol table section */
#define SHT_STRTAB	3		/* string table section */

/*
 * Module information subtypes (for the metadata that we need to build)
 */
#define MODINFO_END		0x0000		/* End of list */
#define MODINFO_NAME		0x0001		/* Name of module (string) */
#define MODINFO_TYPE		0x0002		/* Type of module (string) */
#define MODINFO_METADATA	0x8000		/* Module-specfic */

#define MODINFOMD_SSYM		0x0003		/* start of symbols */
#define MODINFOMD_ESYM		0x0004		/* end of symbols */

#endif	/* IMAGE_FREEBSD */

#endif /* I386_BITS_ELF_H */
