#ifndef __INCLUDE_ELF_H
#define __INCLUDE_ELF_H


#ifdef __64BITS__ /* FIXME: how to check for 64 bits? */
# define ELF_ARCH_SIZE 64
#else
# define ELF_ARCH_SIZE 32
#endif


/* 32-bit data types */
typedef unsigned long  ELF32_ADDR;     /* Unsigned program address */
typedef unsigned short ELF32_HALF;     /* Unsigned medium integer */
typedef unsigned long  ELF32_OFF;      /* Unsigned file offset */
typedef unsigned long  ELF32_SWORD;    /* Signed large integer */
typedef unsigned long  ELF32_WORD;     /* Unsigned large integer */
typedef ELF32_OFF      ELF32_HASHELT;  /* Hash element? */

/* Elf data encodings */
#define IMAGE_ELF_DATA_NONE  0  /* Invalid data encoding */
#define IMAGE_ELF_DATA_2LSB  1  /* 2's complement, LSB first */
#define IMAGE_ELF_DATA_2MSB  2  /* 2's complement, MSB first */

/* Elf object file types */
#define IMAGE_ELF_TYPE_NONE  0  /* No file type */
#define IMAGE_ELF_TYPE_REL   1  /* Relocatable file */
#define IMAGE_ELF_TYPE_EXEC  2  /* Executable file */
#define IMAGE_ELF_TYPE_DYN   3  /* Shared object file */
#define IMAGE_ELF_TYPE_CORE  4  /* Core file */

/* Elf machines */
#define IMAGE_ELF_MACHINE_NONE         0   /* No machine */
#define IMAGE_ELF_MACHINE_M32          1   /* AT&T WE 32100 */
#define IMAGE_ELF_MACHINE_SPARC        2   /* SPARC */
#define IMAGE_ELF_MACHINE_386          3   /* Intel 80386 */
#define IMAGE_ELF_MACHINE_68K          4   /* Motorola 68000 */
#define IMAGE_ELF_MACHINE_88K          5   /* Motorola 88000 */
                                           /* 6 is reserved */
#define IMAGE_ELF_MACHINE_860          7   /* Intel 80860 */
#define IMAGE_ELF_MACHINE_MIPS         8   /* MIPS RS3000 (MIPS I) */
#define IMAGE_ELF_MACHINE_S370         9   /* IBM System/370 Processor */
#define IMAGE_ELF_MACHINE_MIPS_RS3_LE  10  /* MIPS RS3000 Little-endian */
                                           /* 11-14 are reserved */
#define IMAGE_ELF_MACHINE_PARISC       15  /* Hewlett-Packard PA-RISC */
                                           /* 16 is reserved */
#define IMAGE_ELF_MACHINE_VPP500       17  /* Fujitsu VPP500 */
#define IMAGE_ELF_MACHINE_SPARC32PLUS  18  /* Enhanced instruction set SPARC */
#define IMAGE_ELF_MACHINE_960          19  /* Intel 80960 */
#define IMAGE_ELF_MACHINE_PPC          20  /* PowerPC */
#define IMAGE_ELF_MACHINE_PPC64        21  /* 64-bit PowerPC */
                                           /* 22-35 are reserved */
#define IMAGE_ELF_MACHINE_V800         36  /* NEC V800 */
#define IMAGE_ELF_MACHINE_FR20         37  /* Fujitsu FR20 */
#define IMAGE_ELF_MACHINE_RH32         38  /* TRW RH-32 */
#define IMAGE_ELF_MACHINE_RCE          39  /* Motorola RCE */
#define IMAGE_ELF_MACHINE_ARM          40  /* Advanced RISC Machines ARM */
#define IMAGE_ELF_MACHINE_ALPHA        41  /* Digital Alpha */
#define IMAGE_ELF_MACHINE_SH           42  /* Hitachi SH */
#define IMAGE_ELF_MACHINE_SPARCV9      43  /* SPARC Version 9 */
#define IMAGE_ELF_MACHINE_TRICORE      44  /* Siemens Tricore embedded processor */
#define IMAGE_ELF_MACHINE_ARC          45  /* Argonaut RISC Core, Argonaut Technologies Inc. */
#define IMAGE_ELF_MACHINE_H8_300       46  /* Hitachi H8/300 */
#define IMAGE_ELF_MACHINE_H8_300H      47  /* Hitachi H8/300H */
#define IMAGE_ELF_MACHINE_H8S          48  /* Hitachi H8S */
#define IMAGE_ELF_MACHINE_H8_500       49  /* Hitachi H8/500 */
#define IMAGE_ELF_MACHINE_IA_64        50  /* Intel IA-64 processor architecture */
#define IMAGE_ELF_MACHINE_MIPS_X       51  /* Stanford MIPS-X */
#define IMAGE_ELF_MACHINE_COLDFIRE     52  /* Motorola ColdFire */
#define IMAGE_ELF_MACHINE_68HC12       53  /* Motorola M68HC12 */
#define IMAGE_ELF_MACHINE_MMA          54  /* Fujitsu MMA Multimedia Accelerator */
#define IMAGE_ELF_MACHINE_PCP          55  /* Siemens PCP */
#define IMAGE_ELF_MACHINE_NCPU         56  /* Sony nCPU embedded RISC processor */
#define IMAGE_ELF_MACHINE_NDR1         57  /* Denso NDR1 microprocessor */
#define IMAGE_ELF_MACHINE_STARCORE     58  /* Motorola Star*Core processor */
#define IMAGE_ELF_MACHINE_ME16         59  /* Toyota ME16 processor */
#define IMAGE_ELF_MACHINE_ST100        60  /* STMicroelectronics ST100 processor */
#define IMAGE_ELF_MACHINE_TINYJ        61  /* Advanced Logic Corp. TinyJ embedded processor family */
                                           /* 62-65 are reserved */
#define IMAGE_ELF_MACHINE_FX66         66  /* Siemens FX66 microcontroller */
#define IMAGE_ELF_MACHINE_ST9PLUS      67  /* STMicroelectronics ST9+ 8/16 bit microcontroller */
#define IMAGE_ELF_MACHINE_ST7          68  /* STMicroelectronics ST7 8-bit microcontroller */
#define IMAGE_ELF_MACHINE_68HC16       69  /* Motorola MC68HC16 Microcontroller */
#define IMAGE_ELF_MACHINE_68HC11       70  /* Motorola MC68HC11 Microcontroller */
#define IMAGE_ELF_MACHINE_68HC08       71  /* Motorola MC68HC08 Microcontroller */
#define IMAGE_ELF_MACHINE_68HC05       72  /* Motorola MC68HC05 Microcontroller */
#define IMAGE_ELF_MACHINE_SVX          73  /* Silicon Graphics SVx */
#define IMAGE_ELF_MACHINE_ST19         74  /* STMicroelectronics ST19 8-bit microcontroller */
#define IMAGE_ELF_MACHINE_VAX          75  /* Digital VAX */
#define IMAGE_ELF_MACHINE_CRIS         76  /* Axis Communications 32-bit embedded processor */
#define IMAGE_ELF_MACHINE_JAVELIN      77  /* Infineon Technologies 32-bit embedded processor */
#define IMAGE_ELF_MACHINE_FIREPATH     78  /* Element 14 64-bit DSP Processor */
#define IMAGE_ELF_MACHINE_ZSP          79  /* LSI Logic 16-bit DSP Processor */
#define IMAGE_ELF_MACHINE_MMIX         80  /* Donald Knuth's educational 64-bit processor */
#define IMAGE_ELF_MACHINE_HUANY        81  /* Harvard University machine-independent object files */
#define IMAGE_ELF_MACHINE_PRISM        82  /* SiTera Prism */
#define IMAGE_ELF_MACHINE_AVR          83  /* Atmel AVR 8-bit microcontroller */
#define IMAGE_ELF_MACHINE_FR30         84  /* Fujitsu FR30 */
#define IMAGE_ELF_MACHINE_D10V         85  /* Mitsubishi D10V */
#define IMAGE_ELF_MACHINE_D30V         86  /* Mitsubishi D30V */
#define IMAGE_ELF_MACHINE_V850         87  /* NEC v850 */
#define IMAGE_ELF_MACHINE_M32R         88  /* Mitsubishi M32R */
#define IMAGE_ELF_MACHINE_MN10300      89  /* Matsushita MN10300 */
#define IMAGE_ELF_MACHINE_MN10200      90  /* Matsushita MN10200 */
#define IMAGE_ELF_MACHINE_PJ           91  /* picoJava */
#define IMAGE_ELF_MACHINE_OPENRISC     92  /* OpenRISC 32-bit embedded processor */

/* Elf versions */
#define IMAGE_ELF_VERSION_NONE     0  /* Invalid version */
#define IMAGE_ELF_VERSION_CURRENT  1  /* Current version */

/* Elf identification */
#define IMAGE_ELF_SIZEOF_IDENT      16
#define IMAGE_ELF_IDENT_MAGIC0      0   /* Magic */
#define IMAGE_ELF_IDENT_MAGIC1      1
#define IMAGE_ELF_IDENT_MAGIC2      2
#define IMAGE_ELF_IDENT_MAGIC3      3
#define IMAGE_ELF_IDENT_CLASS       4   /* File class */
#define IMAGE_ELF_IDENT_DATA        5   /* Data encoding */
#define IMAGE_ELF_IDENT_VERSION     6   /* File version */
#define IMAGE_ELF_IDENT_OSABI       7   /* Operating system/ABI identification */
#define IMAGE_ELF_IDENT_ABIVERSION  8   /* ABI version */
#define IMAGE_ELF_IDENT_PAD         9   /* Start of padding bytes */

/* Magic numbers */
#define IMAGE_ELF_MAGIC0  0x7f
#define IMAGE_ELF_MAGIC1  'E'
#define IMAGE_ELF_MAGIC2  'L'
#define IMAGE_ELF_MAGIC3  'F'

/* Elf file classes */
#define IMAGE_ELF_CLASS_NONE  0  /* Invalid class */
#define IMAGE_ELF_CLASS_32    1  /* 32-bit object */
#define IMAGE_ELF_CLASS_64    2  /* 64-bit object */

/* Check elf magic */
#define IMAGE_IS_ELF(hdr)  ((hdr).Ident[IMAGE_ELF_IDENT_MAGIC0] == IMAGE_ELF_MAGIC0 && \
                            (hdr).Ident[IMAGE_ELF_IDENT_MAGIC1] == IMAGE_ELF_MAGIC1 && \
                            (hdr).Ident[IMAGE_ELF_IDENT_MAGIC2] == IMAGE_ELF_MAGIC2 && \
                            (hdr).Ident[IMAGE_ELF_IDENT_MAGIC3] == IMAGE_ELF_MAGIC3)



/* 32-bit Elf header */
typedef struct _IMAGE_ELF32_HEADER {
	unsigned char Ident[IMAGE_ELF_SIZEOF_IDENT]; /* Identification */
	ELF32_HALF    Type;                          /* Object file type */
	ELF32_HALF    Machine;                       /* Required architecture */
	ELF32_WORD    Version;                       /* Object file version */
	ELF32_ADDR    Entry;                         /* Virtual address of entry point */
	ELF32_OFF     PhOff;                         /* Program header table offset in file */
	ELF32_OFF     ShOff;                         /* Section header offset in file */
	ELF32_WORD    Flags;                         /* Processor specific flags - zero for SPARC and x86 */
	ELF32_HALF    EhSize;                        /* Elf header size in bytes (this struct) */
	ELF32_HALF    PhEntSize;                     /* Size of an entry in the program header table */
	ELF32_HALF    PhNum;                         /* Number of entries in the program header table */
	ELF32_HALF    ShEntSize;                     /* Size of an entry in the section header table */
	ELF32_HALF    ShNum;                         /* Number of entries in the section header table */
	ELF32_HALF    ShStrNdx;                      /* Index into the section header table for the entry of the section name string table */
} IMAGE_ELF32_HEADER, *PIMAGE_ELF32_HEADER;





/* Special section indexes */
#define IMAGE_ELF_SECTION_INDEX_UNDEF      0
/*#define IMAGE_ELF_SECTION_INDEX_LORESERVE  0xff00
#define IMAGE_ELF_SECTION_INDEX_LOPROC     0xff00
#define IMAGE_ELF_SECTION_INDEX_HIPROC     0xff1f*/
#define IMAGE_ELF_SECTION_INDEX_ABS        0xfff1
#define IMAGE_ELF_SECTION_INDEX_COMMON     0xfff2
/*#define IMAGE_ELF_SECTION_INDEX_HIRESERVE  0xffff*/

/* Section types */
#define IMAGE_ELF_SECTION_TYPE_NULL      0   /* Incactive section header */
#define IMAGE_ELF_SECTION_TYPE_PROGBITS  1   /* Program defined section */
#define IMAGE_ELF_SECTION_TYPE_SYMTAB    2   /* Symbol table (for link editing) */
#define IMAGE_ELF_SECTION_TYPE_STRTAB    3   /* String table */
#define IMAGE_ELF_SECTION_TYPE_RELA      4   /* Relocation table (with explicit addends) */
#define IMAGE_ELF_SECTION_TYPE_HASH      5   /* Symbol hash table */
#define IMAGE_ELF_SECTION_TYPE_DYNAMIC   6   /* Information for dynamic linking */
#define IMAGE_ELF_SECTION_TYPE_NOTE      7   /* Note section ;-) */
#define IMAGE_ELF_SECTION_TYPE_NOBITS    8   /* Occupies no space in the file, otherwise like PROGBITS */
#define IMAGE_ELF_SECTION_TYPE_REL       9   /* Relocation table (without explicit addends) */
#define IMAGE_ELF_SECTION_TYPE_SHLIB     10  /* Reserved, unspecified */
#define IMAGE_ELF_SECTION_TYPE_DYNSYM    11  /* Symbol table (for dynamic linking) */
/*#define IMAGE_ELF_SECTION_TYPE_LOPROC  0x70000000
#define IMAGE_ELF_SECTION_TYPE_HIPROC    0x7fffffff
#define IMAGE_ELF_SECTION_TYPE_LOUSER    0x80000000
#define IMAGE_ELF_SECTION_TYPE_HIUSER    0xffffffff*/

/* Section flags/attributes */
#define SHF_WRITE      0x1  /* Section must be writeable */
#define SHF_ALLOC      0x2  /* Section must be loaded/mapped into memory */
#define SHF_EXECINSTR  0x4  /* The section contains executable code */
/*#define SHF_MASKPROC   0xf0000000*/

/* 32-bit Section header entry */
typedef struct _IMAGE_ELF32_SECTION_HEADER {
	ELF32_WORD  Name;       /* Name of the section (index into the section header string table) */
	ELF32_WORD  Type;       /* Type of section */
	ELF32_WORD  Flags;      /* Attributes */
	ELF32_ADDR  Addr;       /* Virtual address to load section at */
	ELF32_OFF   Offset;     /* Offset into the file of the section's data */
	ELF32_WORD  Size;       /* Size of the section */
	ELF32_WORD  Link;       /* Section header table index link... */
	ELF32_WORD  Info;       /* Extra information... */
	ELF32_WORD  AddrAlign;  /* Required alignment */
	ELF32_WORD  EntSize;    /* Size of entries in the table */
} IMAGE_ELF32_SECTION_HEADER, *PIMAGE_ELF32_SECTION_HEADER;





/* Symbol table indexes */
#define IMAGE_ELF_SYMBOL_INDEX_UNDEF  0  /* Undefined symbol */

/* Symbol binding/types */
#define IMAGE_ELF32_SYMBOL_BIND(Info)        ((Info) >> 4)
#define IMAGE_ELF32_SYMBOL_TYPE(Info)        ((Info) & 0x0f)
#define IMAGE_ELF32_SYMBOL_INFO(Bind, Type)  (((Bind) << 4) | ((Type) & 0x0f))

#define IMAGE_ELF_SYMBOL_BINDING_LOCAL   0  /* Local ("static") symbol */
#define IMAGE_ELF_SYMBOL_BINDING_GLOBAL  1  /* Global symbol */
#define IMAGE_ELF_SYMBOL_BINDING_WEAK    2  /* Weak symbol... */
/*#define IMAGE_ELF_SYMBOL_BINDING_LOPROC  13
#define IMAGE_ELF_SYMBOL_BINDING_HIPROC  15*/

#define IMAGE_ELF_SYMBOL_TYPE_NOTYPE   0   /* Unspecified symbol type */
#define IMAGE_ELF_SYMBOL_TYPE_OBJECT   1   /* Data object (i.e. an array, variable, ...) */
#define IMAGE_ELF_SYMBOL_TYPE_FUNC     2   /* Function (or other executable code) */
#define IMAGE_ELF_SYMBOL_TYPE_SECTION  3   /* Symbol for relocating (usually has local binding) */
#define IMAGE_ELF_SYMBOL_TYPE_FILE     4   /* Name of the associated source file */
#define IMAGE_ELF_SYMBOL_TYPE_LOPROC   13  
#define IMAGE_ELF_SYMBOL_TYPE_HIPROC   15  

/* 32-bit Symbol entry */
typedef struct _IMAGE_ELF32_SYMBOL {
	ELF32_WORD     Name;   /* Symbol name (index into the symbol string table) */
	ELF32_ADDR     Value;  /* Value of symbol */
	ELF32_WORD     Size;   /* Size of symbol (0 means unknown) */
	unsigned char  Info;   /* Type and binding attributes */
	unsigned char  Other;  /* Unused - 0 */
	ELF32_HALF     Shndx;  /* Section index */
} IMAGE_ELF32_SYMBOL, *PIMAGE_ELF32_SYMBOL;





/* Relocation macros */
#define IMAGE_ELF32_RELOC_SYM(Info)       ((Info) >> 8)  
#define IMAGE_ELF32_RELOC_TYPE(Info)      ((unsigned char)(Info))  
#define IMAGE_ELF32_RELOC_INFO(Sym,Type)  (((Sym) << 8) | (unsigned char)(Type))  

/* 386 Relocation types */
#define IMAGE_ELF_RELOC_386_NONE      0   /* none	*/
#define IMAGE_ELF_RELOC_386_32        1
#define IMAGE_ELF_RELOC_386_PC32      2
#define IMAGE_ELF_RELOC_386_GOT32     3
#define IMAGE_ELF_RELOC_386_PLT32     4
#define IMAGE_ELF_RELOC_386_COPY      5
#define IMAGE_ELF_RELOC_386_GLOB_DAT  6
#define IMAGE_ELF_RELOC_386_JMP_SLOT  7
#define IMAGE_ELF_RELOC_386_RELATIVE  8
#define IMAGE_ELF_RELOC_386_GOTOFF    9
#define IMAGE_ELF_RELOC_386_GOTPC     10

/* 386 TLS Relocation types */
#define IMAGE_ELF_RELOC_386_TLS_GD_PLT    12
#define IMAGE_ELF_RELOC_386_TLS_LDM_PLT   13
#define IMAGE_ELF_RELOC_386_TLS_TPOFF     14
#define IMAGE_ELF_RELOC_386_TLS_IE        15
#define IMAGE_ELF_RELOC_386_TLS_GOTIE     16
#define IMAGE_ELF_RELOC_386_TLS_LE        17
#define IMAGE_ELF_RELOC_386_TLS_GD        18
#define IMAGE_ELF_RELOC_386_TLS_LDM       19
#define IMAGE_ELF_RELOC_386_TLS_LDO_32    32
#define IMAGE_ELF_RELOC_386_TLS_DTPMOD32  35
#define IMAGE_ELF_RELOC_386_TLS_DTPOFF32  36

/* 32-bit Relocation entries */
typedef struct _IMAGE_ELF32_RELOC {
	ELF32_ADDR  Offset;  /* Section offset/virtual address */
	ELF32_WORD  Info;    /* Symbol table index/relocation type */
} IMAGE_ELF32_RELOC, *PIMAGE_ELF32_RELOC;
  
typedef struct _IMAGE_ELF32_RELOCA {
	ELF32_ADDR   Offset;  /* Section offset/virtual address */
	ELF32_WORD   Info;    /* Symbol table index/relocation type */
	ELF32_SWORD  Addend;  /* Addend */
} IMAGE_ELF32_RELOCA, *PIMAGE_ELF32_RELOCA;





/* Program header/segment types */
#define IMAGE_ELF_SEGMENT_TYPE_NULL     0  /* Unused array entry */
#define IMAGE_ELF_SEGMENT_TYPE_LOAD     1  /* Loadable segment */
#define IMAGE_ELF_SEGMENT_TYPE_DYNAMIC  2  /* Dynamic linking info... */
#define IMAGE_ELF_SEGMENT_TYPE_INTERP   3  /* Interpreter */
#define IMAGE_ELF_SEGMENT_TYPE_NOTE     4  /* Note... */
#define IMAGE_ELF_SEGMENT_TYPE_SHLIB    5  /* Reserved but unspecified */
#define IMAGE_ELF_SEGMENT_TYPE_PHDR     6  /* Program header table */
#define IMAGE_ELF_SEGMENT_TYPE_TLS      7  /* Thread local storage */

#define IMAGE_ELF_SEGMENT_TYPE_GNU_EH_FRAME  0x6474e550  /* GCC .eh_frame_hdr segment */
#define IMAGE_ELF_SEGMENT_TYPE_GNU_STACK     0x6474e551  /* Indicates stack executability */
/*#define IMAGE_ELF_SEGMENT_TYPE_LOPROC  0x70000000
#define IMAGE_ELF_SEGMENT_TYPE_HIPROC  0x7fffffff*/

/* Program header/segment flags */
#define IMAGE_ELF_SEGMENT_FLAG_EXEC   0x01
#define IMAGE_ELF_SEGMENT_FLAG_WRITE  0x02
#define IMAGE_ELF_SEGMENT_FLAG_READ   0x04
/*#define IMAGE_ELF_SEGMENT_FLAG_MASKPROC 0xf0000000*/

/* 32-bit Program header entry */
typedef struct _IMAGE_ELF32_PROGRAM_HEADER {
	ELF32_WORD  Type;  	 /* Type of segment */
	ELF32_OFF   Offset;  /* File offset of segment data */
	ELF32_ADDR  VAddr;   /* Virtual address to load segment at */
	ELF32_ADDR  PAddr;   /* Physical address to load segment at */
	ELF32_WORD  FileSz;  /* Size in file of segment */
	ELF32_WORD  MemSz;   /* Size in memory of segment */
	ELF32_WORD  Flags;   /* Flags of segment */
	ELF32_WORD  Align;   /* Required alignment */
} IMAGE_ELF32_PROGRAM_HEADER, *PIMAGE_ELF32_PROGRAM_HEADER;





/* Dynamic array tags */
#define IMAGE_ELF_DYNAMIC_TAG_NULL      0   /* End of array */
#define IMAGE_ELF_DYNAMIC_TAG_NEEDED    1   /* Dependency */
#define IMAGE_ELF_DYNAMIC_TAG_PLTRELSZ  2   /* Size in bytes of the relocation entries associated with the plt */
#define IMAGE_ELF_DYNAMIC_TAG_PLTGOT    3   /* Address associated with got/plt */
#define IMAGE_ELF_DYNAMIC_TAG_HASH      4   /* Hash table for symbol table indicated by SYMTAB */
#define IMAGE_ELF_DYNAMIC_TAG_STRTAB    5   /* Address of string table */
#define IMAGE_ELF_DYNAMIC_TAG_SYMTAB    6   /* Address of the symbol table */
#define IMAGE_ELF_DYNAMIC_TAG_RELA      7   /* Address of reloc table with addends */
#define IMAGE_ELF_DYNAMIC_TAG_RELASZ    8   /* Size of reloc table in bytes */
#define IMAGE_ELF_DYNAMIC_TAG_RELAENT   9   /* Size of reloc table entry in bytes? */
#define IMAGE_ELF_DYNAMIC_TAG_STRSZ     10  /* Size of the string table in bytes */
#define IMAGE_ELF_DYNAMIC_TAG_SYMENT    11  /* Size of symbol table entry in bytes */
#define IMAGE_ELF_DYNAMIC_TAG_INIT      12  /* Address of initialization function */
#define IMAGE_ELF_DYNAMIC_TAG_FINI      13  /* Address of termination function */
#define IMAGE_ELF_DYNAMIC_TAG_SONAME    14  /* Name of the shared object (string table offset) */
#define IMAGE_ELF_DYNAMIC_TAG_RPATH     15  /* Library search path (string table offset) */
#define IMAGE_ELF_DYNAMIC_TAG_SYMBOLIC  16  /* Alter runtime-linkers symbol resolution... */
#define IMAGE_ELF_DYNAMIC_TAG_REL       17  /* Address of reloc table without addends */
#define IMAGE_ELF_DYNAMIC_TAG_RELSZ     18  /* Size of reloc table in bytes */
#define IMAGE_ELF_DYNAMIC_TAG_RELENT    19  /* Size of reloc table entry in bytes? */
#define IMAGE_ELF_DYNAMIC_TAG_PLTREL    20  /* Type of relocation entry for plt (...TAG_REL or ...TAG_RELA) */
#define IMAGE_ELF_DYNAMIC_TAG_DEBUG     21  /* Used for debugging */
#define IMAGE_ELF_DYNAMIC_TAG_TEXTREL   22  /* If present informs the linker that a relocation might update a non-writable segment */
#define IMAGE_ELF_DYNAMIC_TAG_JMPREL    23  /* Address of relocation entries associated solely with the plt */
#define IMAGE_ELF_DYNAMIC_TAG_FILTER    24  /* Specifies the name of a shared objects for which this one acts as a filter */

#define IMAGE_ELF_DYNAMIC_TAG_RUNPATH   29  /* String table offset of a null-terminated library search path string. */
#define	IMAGE_ELF_DYNAMIC_TAG_FLAGS	    30  /* Object specific flag values. */

/* Dynamic flags (for IMAGE_ELF_DYNAMIC_TAG_FLAGS) */
#define	IMAGE_ELF_DYNAMIC_FLAG_ORIGIN      0x0001  /* Indicates that the object being loaded may make reference to the $ORIGIN substitution string.*/
#define	IMAGE_ELF_DYNAMIC_FLAG_SYMBOLIC    0x0002  /* Indicates "symbolic" linking. */
#define	IMAGE_ELF_DYNAMIC_FLAG_TEXTREL     0x0004  /* Indicates there may be relocations in non-writable segments. */
#define	IMAGE_ELF_DYNAMIC_FLAG_BIND_NOW    0x0008  /* Indicates that the dynamic linker should process all relocations for the object
                                                      containing this entry before transferring control to the program. */
#define	IMAGE_ELF_DYNAMIC_FLAG_STATIC_TLS  0x0010  /* Indicates that the shared object or executable contains code using a static
                                                      thread-local storage scheme. */

/* Dynamic array entry */
typedef struct _IMAGE_ELF32_DYNAMIC {
	ELF32_SWORD  Tag;
	union {
		ELF32_WORD  Val;
		ELF32_ADDR  Ptr;
	} Un;
} IMAGE_ELF32_DYNAMIC, *PIMAGE_ELF32_DYNAMIC;





/* Auxiliary types */
#define	IMAGE_ELF_AUX_TYPE_NULL    0   /* Terminates the vector. */
#define	IMAGE_ELF_AUX_TYPE_IGNORE  1   /* Ignored entry. */
#define	IMAGE_ELF_AUX_TYPE_EXECFD  2   /* File descriptor of program to load. */
#define	IMAGE_ELF_AUX_TYPE_PHDR    3   /* Program header of program already loaded. */
#define	IMAGE_ELF_AUX_TYPE_PHENT   4   /* Size of each program header entry. */
#define	IMAGE_ELF_AUX_TYPE_PHNUM   5   /* Number of program header entries. */
#define	IMAGE_ELF_AUX_TYPE_PAGESZ  6   /* Page size in bytes. */
#define	IMAGE_ELF_AUX_TYPE_BASE    7   /* Interpreter's base address. */
#define	IMAGE_ELF_AUX_TYPE_FLAGS   8   /* Flags (unused for i386). */
#define	IMAGE_ELF_AUX_TYPE_ENTRY   9   /* Where interpreter should transfer control. */

/*
 * The following non-standard values are used for passing information
 * from John Polstra's testbed program to the dynamic linker.  These
 * are expected to go away soon.
 *
 * Unfortunately, these overlap the Linux non-standard values, so they
 * must not be used in the same context.
 */
#define	IMAGE_ELF_AUX_TYPE_BRK     10  /* Starting point for sbrk and brk. */
#define	IMAGE_ELF_AUX_TYPE_DEBUG   11  /* Debugging level. */

/*
 * The following non-standard values are used in Linux ELF binaries.
 */
#define	IMAGE_ELF_AUX_TYPE_NOTELF  10  /* Program is not ELF ?? */
#define	IMAGE_ELF_AUX_TYPE_UID     11  /* Real uid. */
#define	IMAGE_ELF_AUX_TYPE_EUID    12  /* Effective uid. */
#define	IMAGE_ELF_AUX_TYPE_GID     13  /* Real gid. */
#define	IMAGE_ELF_AUX_TYPE_EGID    14  /* Effective gid. */

#define	IMAGE_ELF_AUX_TYPE_COUNT   15  /* Count of defined aux entry types. */


/* Auxiliary vector entry on initial stack */
typedef struct _IMAGE_ELF32_AUXINFO {
	ELF32_SWORD  Type;            /* Entry type. */
	union {
		ELF32_SWORD  Val;         /* Integer value. */
		ELF32_ADDR   Ptr;         /* Address. */
		void       (*Fcn)(void);  /* Function pointer (not used). */
	} Un;
} IMAGE_ELF32_AUXINFO, *PIMAGE_ELF32_AUXINFO;





/* arch data types */

#if ELF_ARCH_SIZE == 32

#define IMAGE_ELF_SYMBOL_BIND(args...) IMAGE_ELF32_SYMBOL_BIND(args)
#define IMAGE_ELF_SYMBOL_TYPE(args...) IMAGE_ELF32_SYMBOL_TYPE(args)
#define IMAGE_ELF_SYMBOL_INFO(args...) IMAGE_ELF32_SYMBOL_INFO(args)

#define IMAGE_ELF_RELOC_SYM(args...)  IMAGE_ELF32_RELOC_SYM(args)
#define IMAGE_ELF_RELOC_TYPE(args...) IMAGE_ELF32_RELOC_TYPE(args)
#define IMAGE_ELF_RELOC_INFO(args...) IMAGE_ELF32_RELOC_INFO(args)

typedef ELF32_ADDR     ELF_ADDR;
typedef ELF32_HALF     ELF_HALF;
typedef ELF32_OFF      ELF_OFF;
typedef ELF32_SWORD    ELF_SWORD;
typedef ELF32_WORD     ELF_WORD;
typedef ELF32_HASHELT  ELF_HASHELT;

typedef IMAGE_ELF32_HEADER          IMAGE_ELF_HEADER, *PIMAGE_ELF_HEADER;
typedef IMAGE_ELF32_SECTION_HEADER  IMAGE_ELF_SECTION_HEADER, *PIMAGE_ELF_SECTION_HEADER;
typedef IMAGE_ELF32_SYMBOL          IMAGE_ELF_SYMBOL, *PIMAGE_ELF_SYMBOL;
typedef IMAGE_ELF32_RELOC           IMAGE_ELF_RELOC, *PIMAGE_ELF_RELOC;
typedef IMAGE_ELF32_RELOCA          IMAGE_ELF_RELOCA, *PIMAGE_ELF_RELOCA;
typedef IMAGE_ELF32_PROGRAM_HEADER  IMAGE_ELF_PROGRAM_HEADER, *PIMAGE_ELF_PROGRAM_HEADER;
typedef IMAGE_ELF32_DYNAMIC         IMAGE_ELF_DYNAMIC, *PIMAGE_ELF_DYNAMIC;
typedef IMAGE_ELF32_AUXINFO         IMAGE_ELF_AUXINFO, *PIMAGE_ELF_AUXINFO;

#elif ELF_ARCH_SIZE == 64
# error 64 bits unsupported
#else
# error Undefined architecture size!
#endif




/* target macros */
#ifdef _M_IX86
# define IMAGE_ELF_TARGET_CLASS    IMAGE_ELF_CLASS_32
# define IMAGE_ELF_TARGET_DATA     IMAGE_ELF_DATA_2LSB
# define IMAGE_ELF_TARGET_MACHINE  IMAGE_ELF_MACHINE_386
# define IMAGE_ELF_TARGET_VERSION  1
#else
# error Unsupported architecture!
#endif

#undef ELF_ARCH_SIZE

#endif /* __INCLUDE_ELF_H */

