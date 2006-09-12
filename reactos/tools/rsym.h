/* rsym.h */

#ifndef RSYM_H
#define RSYM_H

#define IMAGE_DOS_MAGIC 0x5a4d
#define IMAGE_PE_MAGIC 0x00004550

#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef unsigned char BYTE, *PBYTE;
typedef unsigned char UCHAR;
typedef unsigned short WORD;
typedef unsigned short USHORT;
#if defined(__x86_64__) && defined(linux)
typedef signed int LONG;
typedef unsigned int ULONG;
typedef unsigned int DWORD;
#else
typedef signed long LONG;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
#endif
#if defined(_WIN64)
typedef unsigned __int64 ULONG_PTR;
#else
#if defined(__x86_64__) && defined(linux)
typedef  unsigned int  ULONG_PTR;
#else
typedef  unsigned long ULONG_PTR;
#endif
#endif

#include "bytesex.h"

using namespace ReactosBytesex;

#pragma pack(2)
typedef struct _IMAGE_DOS_HEADER {
  LEWord e_magic;
  LEWord e_cblp;
  LEWord e_cp;
  LEWord e_crlc;
  LEWord e_cparhdr;
  LEWord e_minalloc;
  LEWord e_maxalloc;
  LEWord e_ss;
  LEWord e_sp;
  LEWord e_csum;
  LEWord e_ip;
  LEWord e_cs;
  LEWord e_lfarlc;
  LEWord e_ovno;
  LEWord e_res[4];
  LEWord e_oemid;
  LEWord e_oeminfo;
  LEWord e_res2[10];
  LEDWord e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;
#pragma pack(4)

#define IMAGE_FILE_LINE_NUMS_STRIPPED	4
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED	8
#define IMAGE_FILE_DEBUG_STRIPPED	512

#pragma pack(4)
typedef struct _IMAGE_FILE_HEADER {
  LEWord Machine;
  LEWord NumberOfSections;
  LEDWord TimeDateStamp;
  LEDWord PointerToSymbolTable;
  LEDWord NumberOfSymbols;
  LEWord SizeOfOptionalHeader;
  LEWord Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  LEDWord VirtualAddress;
  LEDWord Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

#define IMAGE_DIRECTORY_ENTRY_BASERELOC	5

typedef struct _IMAGE_OPTIONAL_HEADER {
  LEWord Magic;
  BYTE MajorLinkerVersion;
  BYTE MinorLinkerVersion;
  LEDWord SizeOfCode;
  LEDWord SizeOfInitializedData;
  LEDWord SizeOfUninitializedData;
  LEDWord AddressOfEntryPoint;
  LEDWord BaseOfCode;
  LEDWord BaseOfData;
  LEDWord ImageBase;
  LEDWord SectionAlignment;
  LEDWord FileAlignment;
  LEWord MajorOperatingSystemVersion;
  LEWord MinorOperatingSystemVersion;
  LEWord MajorImageVersion;
  LEWord MinorImageVersion;
  LEWord MajorSubsystemVersion;
  LEWord MinorSubsystemVersion;
  LEDWord Reserved1;
  LEDWord SizeOfImage;
  LEDWord SizeOfHeaders;
  LEDWord CheckSum;
  LEWord Subsystem;
  LEWord DllCharacteristics;
  LEDWord SizeOfStackReserve;
  LEDWord SizeOfStackCommit;
  LEDWord SizeOfHeapReserve;
  LEDWord SizeOfHeapCommit;
  LEDWord LoaderFlags;
  LEDWord NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

#define IMAGE_SCN_TYPE_NOLOAD     0x00000002
#define IMAGE_SCN_LNK_REMOVE      0x00000800
#define IMAGE_SCN_MEM_READ        0x40000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000

typedef struct _IMAGE_SECTION_HEADER {
  BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    LEDWord PhysicalAddress;
    LEDWord VirtualSize;
  } Misc;
  LEDWord VirtualAddress;
  LEDWord SizeOfRawData;
  LEDWord PointerToRawData;
  LEDWord PointerToRelocations;
  LEDWord PointerToLinenumbers;
  LEWord NumberOfRelocations;
  LEWord NumberOfLinenumbers;
  LEDWord Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_BASE_RELOCATION {
	LEDWord VirtualAddress;
	LEDWord SizeOfBlock;
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;


typedef struct {
  LEWord f_magic;         /* magic number             */
  LEWord f_nscns;         /* number of sections       */
  LEDWord  f_timdat;        /* time & date stamp        */
  LEDWord  f_symptr;        /* file pointer to symtab   */
  LEDWord  f_nsyms;         /* number of symtab entries */
  LEWord f_opthdr;        /* sizeof(optional hdr)     */
  LEWord f_flags;         /* flags                    */
} FILHDR;

typedef struct {
  char           s_name[8];  /* section name                     */
  LEDWord  s_paddr;    /* physical address, aliased s_nlib */
  LEDWord  s_vaddr;    /* virtual address                  */
  LEDWord  s_size;     /* section size                     */
  LEDWord  s_scnptr;   /* file ptr to raw data for section */
  LEDWord  s_relptr;   /* file ptr to relocation           */
  LEDWord  s_lnnoptr;  /* file ptr to line numbers         */
  LEWord s_nreloc;   /* number of relocation entries     */
  LEWord s_nlnno;    /* number of line number entries    */
  LEDWord  s_flags;    /* flags                            */
} SCNHDR;
#pragma pack(4)

typedef struct _SYMBOLFILE_HEADER {
  LEDWord SymbolsOffset;
  LEDWord SymbolsLength;
  LEDWord StringsOffset;
  LEDWord StringsLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _STAB_ENTRY {
  LEDWord n_strx;         /* index into string table of name */
  UCHAR n_type;         /* type of symbol */
  UCHAR n_other;        /* misc info (usually empty) */
  LEWord n_desc;        /* description field */
  LEDWord n_value;        /* value of symbol */
} STAB_ENTRY, *PSTAB_ENTRY;

/* http://www.math.utah.edu/docs/info/stabs_12.html */
#define N_GYSM   0x20
#define N_FNAME  0x22
#define N_FUN    0x24
#define N_STSYM  0x26
#define N_LCSYM  0x28
#define N_MAIN   0x2A
#define N_PC     0x30
#define N_NSYMS  0x32
#define N_NOMAP  0x34
#define N_RSYM   0x40
#define N_M2C    0x42
#define N_SLINE  0x44
#define N_DSLINE 0x46
#define N_BSLINE 0x48
#define N_BROWS  0x48
#define N_DEFD   0x4A
#define N_EHDECL 0x50
#define N_MOD2   0x50
#define N_CATCH  0x54
#define N_SSYM   0x60
#define N_SO     0x64
#define N_LSYM   0x80
#define N_BINCL  0x82
#define N_SOL    0x84
#define N_PSYM   0xA0
#define N_EINCL  0xA2
#define N_ENTRY  0xA4
#define N_LBRAC  0xC0
#define N_EXCL   0xC2
#define N_SCOPE  0xC4
#define N_RBRAC  0xE0
#define N_BCOMM  0xE2
#define N_ECOMM  0xE4
#define N_ECOML  0xE8
#define N_LENG   0xFE

/* COFF symbol table */

#define E_SYMNMLEN	8	/* # characters in a symbol name	*/
#define E_FILNMLEN	14	/* # characters in a file name		*/
#define E_DIMNUM	4	/* # array dimensions in auxiliary entry */

#define N_BTMASK	(0xf)
#define N_TMASK		(0x30)
#define N_BTSHFT	(4)
#define N_TSHIFT	(2)

/* derived types, in e_type */
#define DT_NON		(0)	/* no derived type */
#define DT_PTR		(1)	/* pointer */
#define DT_FCN		(2)	/* function */
#define DT_ARY		(3)	/* array */

#define BTYPE(x)	((x) & N_BTMASK)

#define ISPTR(x)	(((x) & N_TMASK) == (DT_PTR << N_BTSHFT))
#define ISFCN(x)	(((x) & N_TMASK) == (DT_FCN << N_BTSHFT))
#define ISARY(x)	(((x) & N_TMASK) == (DT_ARY << N_BTSHFT))
#define ISTAG(x)	((x)==C_STRTAG||(x)==C_UNTAG||(x)==C_ENTAG)
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

#define C_EFCN		0xff	/* physical end of function	*/
#define C_NULL		0
#define C_AUTO		1	/* automatic variable		*/
#define C_EXT		2	/* external symbol		*/
#define C_STAT		3	/* static			*/
#define C_REG		4	/* register variable		*/
#define C_EXTDEF	5	/* external definition		*/
#define C_LABEL		6	/* label			*/
#define C_ULABEL	7	/* undefined label		*/
#define C_MOS		8	/* member of structure		*/
#define C_ARG		9	/* function argument		*/
#define C_STRTAG	10	/* structure tag		*/
#define C_MOU		11	/* member of union		*/
#define C_UNTAG		12	/* union tag			*/
#define C_TPDEF		13	/* type definition		*/
#define C_USTATIC	14	/* undefined static		*/
#define C_ENTAG		15	/* enumeration tag		*/
#define C_MOE		16	/* member of enumeration	*/
#define C_REGPARM	17	/* register parameter		*/
#define C_FIELD		18	/* bit field			*/
#define C_AUTOARG	19	/* auto argument		*/
#define C_LASTENT	20	/* dummy entry (end of block)	*/
#define C_BLOCK		100	/* ".bb" or ".eb"		*/
#define C_FCN		101	/* ".bf" or ".ef"		*/
#define C_EOS		102	/* end of structure		*/
#define C_FILE		103	/* file name			*/
#define C_LINE		104	/* line # reformatted as symbol table entry */
#define C_ALIAS	 	105	/* duplicate tag		*/
#define C_HIDDEN	106	/* ext symbol in dmert public lib */

#pragma pack(1)
typedef struct _COFF_SYMENT
{
  union
    {
      char e_name[E_SYMNMLEN];
      struct
        {
          LEDWord e_zeroes;
          LEDWord e_offset;
        }
      e;
    }
  e;
  LEDWord e_value;
  LEWord e_scnum;
  LEWord e_type;
  UCHAR e_sclass;
  UCHAR e_numaux;
} COFF_SYMENT, *PCOFF_SYMENT;
#pragma pack(4)

typedef struct _ROSSYM_ENTRY {
  LEDWord Address;
  LEDWord FunctionOffset;
  LEDWord FileOffset;
  LEDWord SourceLine;
} ROSSYM_ENTRY, *PROSSYM_ENTRY;

#define ROUND_UP(N, S) (((N) + (S) - 1) & ~((S) - 1))

extern char*
convert_path(const char* origpath);

extern void*
load_file ( const char* file_name, size_t* file_size );

#endif/*RSYM_H*/
