/*
 * Usage: rsym input-file output-file
 *
 * There are two sources of information: the .stab/.stabstr
 * sections of the executable and the COFF symbol table. Most
 * of the information is in the .stab/.stabstr sections.
 * However, most of our asm files don't contain .stab directives,
 * so routines implemented in assembler won't show up in the
 * .stab section. They are present in the COFF symbol table.
 * So, we mostly use the .stab/.stabstr sections, but we augment
 * the info there with info from the COFF symbol table when
 * possible.
 *
 * This is a tool and is compiled using the host compiler,
 * i.e. on Linux gcc and not mingw-gcc (cross-compiler).
 * Therefore we can't include SDK headers and we have to
 * duplicate some definitions here.
 * Also note that the internal functions are "old C-style",
 * returning an int, where a return of 0 means success and
 * non-zero is failure.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IMAGE_DOS_MAGIC 0x5a4d
#define IMAGE_PE_MAGIC 0x00004550

#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef signed long LONG;
typedef unsigned long ULONG;
#if defined(_WIN64)
typedef unsigned __int64 ULONG_PTR;
#else
typedef  unsigned long ULONG_PTR;
#endif

#pragma pack(push,2)
typedef struct _IMAGE_DOS_HEADER {
  WORD e_magic;
  WORD e_cblp;
  WORD e_cp;
  WORD e_crlc;
  WORD e_cparhdr;
  WORD e_minalloc;
  WORD e_maxalloc;
  WORD e_ss;
  WORD e_sp;
  WORD e_csum;
  WORD e_ip;
  WORD e_cs;
  WORD e_lfarlc;
  WORD e_ovno;
  WORD e_res[4];
  WORD e_oemid;
  WORD e_oeminfo;
  WORD e_res2[10];
  LONG e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;
#pragma pack(pop)

#define IMAGE_FILE_LINE_NUMS_STRIPPED	4
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED	8
#define IMAGE_FILE_DEBUG_STRIPPED	512

#pragma pack(push,4)
typedef struct _IMAGE_FILE_HEADER {
  WORD Machine;
  WORD NumberOfSections;
  DWORD TimeDateStamp;
  DWORD PointerToSymbolTable;
  DWORD NumberOfSymbols;
  WORD SizeOfOptionalHeader;
  WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;
  DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

#define IMAGE_DIRECTORY_ENTRY_BASERELOC	5

typedef struct _IMAGE_OPTIONAL_HEADER {
  WORD Magic;
  BYTE MajorLinkerVersion;
  BYTE MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;
  DWORD BaseOfCode;
  DWORD BaseOfData;
  DWORD ImageBase;
  DWORD SectionAlignment;
  DWORD FileAlignment;
  WORD MajorOperatingSystemVersion;
  WORD MinorOperatingSystemVersion;
  WORD MajorImageVersion;
  WORD MinorImageVersion;
  WORD MajorSubsystemVersion;
  WORD MinorSubsystemVersion;
  DWORD Reserved1;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;
  WORD Subsystem;
  WORD DllCharacteristics;
  DWORD SizeOfStackReserve;
  DWORD SizeOfStackCommit;
  DWORD SizeOfHeapReserve;
  DWORD SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

#define IMAGE_SCN_TYPE_NOLOAD     0x00000002
#define IMAGE_SCN_LNK_REMOVE      0x00000800
#define IMAGE_SCN_MEM_READ        0x40000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000

typedef struct _IMAGE_SECTION_HEADER {
  BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD NumberOfRelocations;
  WORD NumberOfLinenumbers;
  DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_BASE_RELOCATION {
	DWORD VirtualAddress;
	DWORD SizeOfBlock;
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;


typedef struct {
  unsigned short f_magic;         /* magic number             */
  unsigned short f_nscns;         /* number of sections       */
  unsigned long  f_timdat;        /* time & date stamp        */
  unsigned long  f_symptr;        /* file pointer to symtab   */
  unsigned long  f_nsyms;         /* number of symtab entries */
  unsigned short f_opthdr;        /* sizeof(optional hdr)     */
  unsigned short f_flags;         /* flags                    */
} FILHDR;

typedef struct {
  char           s_name[8];  /* section name                     */
  unsigned long  s_paddr;    /* physical address, aliased s_nlib */
  unsigned long  s_vaddr;    /* virtual address                  */
  unsigned long  s_size;     /* section size                     */
  unsigned long  s_scnptr;   /* file ptr to raw data for section */
  unsigned long  s_relptr;   /* file ptr to relocation           */
  unsigned long  s_lnnoptr;  /* file ptr to line numbers         */
  unsigned short s_nreloc;   /* number of relocation entries     */
  unsigned short s_nlnno;    /* number of line number entries    */
  unsigned long  s_flags;    /* flags                            */
} SCNHDR;
#pragma pack(pop)

typedef struct _SYMBOLFILE_HEADER {
  unsigned long SymbolsOffset;
  unsigned long SymbolsLength;
  unsigned long StringsOffset;
  unsigned long StringsLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _STAB_ENTRY {
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} STAB_ENTRY, *PSTAB_ENTRY;

#define N_FUN 0x24
#define N_SLINE 0x44
#define N_SO 0x64

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

#pragma pack(push,1)
typedef struct _COFF_SYMENT
{
  union
    {
      char e_name[E_SYMNMLEN];
      struct
        {
          unsigned long e_zeroes;
          unsigned long e_offset;
        }
      e;
    }
  e;
  unsigned long e_value;
  short e_scnum;
  unsigned short e_type;
  unsigned char e_sclass;
  unsigned char e_numaux;
} COFF_SYMENT, *PCOFF_SYMENT;
#pragma pack(pop)

typedef struct _ROSSYM_ENTRY {
  ULONG_PTR Address;
  ULONG FunctionOffset;
  ULONG FileOffset;
  ULONG SourceLine;
} ROSSYM_ENTRY, *PROSSYM_ENTRY;

#define ROUND_UP(N, S) (((N) + (S) - 1) & ~((S) - 1))

char* convert_path(char* origpath)
{
   char* newpath;
   int i;

   newpath = strdup(origpath);

   i = 0;
   while (newpath[i] != 0)
     {
#ifdef UNIX_PATHS
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef DOS_PATHS
	if (newpath[i] == '/')
	  {
	     newpath[i] = '\\';
	  }
#endif
#endif
	i++;
     }
   return(newpath);
}

static int
CompareSymEntry(const PROSSYM_ENTRY SymEntry1, const PROSSYM_ENTRY SymEntry2)
{
  if (SymEntry1->Address < SymEntry2->Address)
    {
      return -1;
    }

  if (SymEntry2->Address < SymEntry1->Address)
    {
      return +1;
    }

  return 0;
} 

static int
GetStabInfo(void *FileData, PIMAGE_FILE_HEADER PEFileHeader,
            PIMAGE_SECTION_HEADER PESectionHeaders,
            ULONG *StabSymbolsLength, void **StabSymbolsBase,
            ULONG *StabStringsLength, void **StabStringsBase)
{
  ULONG Idx;

  /* Load .stab and .stabstr sections if available */
  *StabSymbolsBase = NULL;
  *StabSymbolsLength = 0;
  *StabStringsBase = NULL;
  *StabStringsLength = 0;

  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      /* printf("section: '%.08s'\n", PESectionHeaders[Idx].Name); */
      if ((strncmp((char*)PESectionHeaders[Idx].Name, ".stab", 5) == 0)
        && (PESectionHeaders[Idx].Name[5] == 0))
        {
	   /* printf(".stab section found. Size %d\n",
               PESectionHeaders[Idx].SizeOfRawData); */

           *StabSymbolsLength = PESectionHeaders[Idx].SizeOfRawData;
           *StabSymbolsBase = (void *)((char *) FileData + PESectionHeaders[Idx].PointerToRawData);
        }

      if (strncmp((char*)PESectionHeaders[Idx].Name, ".stabstr", 8) == 0)
        {
	   /* printf(".stabstr section found. Size %d\n",
               PESectionHeaders[Idx].SizeOfRawData); */

           *StabStringsLength = PESectionHeaders[Idx].SizeOfRawData;
           *StabStringsBase = (void *)((char *) FileData + PESectionHeaders[Idx].PointerToRawData);
        }
    }

  return 0;
}

static int
GetCoffInfo(void *FileData, PIMAGE_FILE_HEADER PEFileHeader,
            PIMAGE_SECTION_HEADER PESectionHeaders,
            ULONG *CoffSymbolsLength, void **CoffSymbolsBase,
            ULONG *CoffStringsLength, void **CoffStringsBase)
{

  if (0 == PEFileHeader->PointerToSymbolTable || 0 == PEFileHeader->NumberOfSymbols)
    {
      /* No COFF symbol table */
      *CoffSymbolsLength = 0;
      *CoffStringsLength = 0;
    }
  else
    {
      *CoffSymbolsLength = PEFileHeader->NumberOfSymbols * sizeof(COFF_SYMENT);
      *CoffSymbolsBase = (void *)((char *) FileData + PEFileHeader->PointerToSymbolTable);
      *CoffStringsLength = *((ULONG *) ((char *) *CoffSymbolsBase + *CoffSymbolsLength));
      *CoffStringsBase = (void *)((char *) *CoffSymbolsBase + *CoffSymbolsLength);
    }

  return 0;
}

static ULONG
FindOrAddString(char *StringToFind, ULONG *StringsLength, void *StringsBase)
{
  char *Search, *End;

  Search = (char *) StringsBase;
  End = Search + *StringsLength;

  while (Search < End)
    {
      if (0 == strcmp(Search, StringToFind))
        {
          return Search - (char *) StringsBase;
        }
      Search += strlen(Search) + 1;
    }

  strcpy(Search, StringToFind);
  *StringsLength += strlen(StringToFind) + 1;

  return Search - (char *) StringsBase;
}

static int
ConvertStabs(ULONG *SymbolsCount, PROSSYM_ENTRY *SymbolsBase,
             ULONG *StringsLength, void *StringsBase,
             ULONG StabSymbolsLength, void *StabSymbolsBase,
             ULONG StabStringsLength, void *StabStringsBase,
             ULONG_PTR ImageBase, PIMAGE_FILE_HEADER PEFileHeader,
             PIMAGE_SECTION_HEADER PESectionHeaders)
{
  PSTAB_ENTRY StabEntry;
  ULONG Count, i;
  ULONG_PTR Address, LastFunctionAddress;
  int First;
  char *Name;
  char FuncName[256];

  StabEntry = StabSymbolsBase;
  Count = StabSymbolsLength / sizeof(STAB_ENTRY);
  *SymbolsBase = malloc(Count * sizeof(ROSSYM_ENTRY));
  if (NULL == *SymbolsBase)
    {
      fprintf(stderr, "Failed to allocate memory for converted .stab symbols\n");
      return 1;
    }
  *SymbolsCount = 0;

  LastFunctionAddress = 0;
  First = 1;
  for (i = 0; i < Count; i++)
    {
      switch (StabEntry[i].n_type)
        {
          case N_SO:
            Name = (char *) StabStringsBase + StabEntry[i].n_strx;
            if ('\0' == *Name || '/' == Name[strlen(Name) - 1]
                || '\\' == Name[strlen(Name) - 1]
                || StabEntry[i].n_value < ImageBase)
              {
                continue;
              }
            Address = StabEntry[i].n_value - ImageBase;
            if (Address != (*SymbolsBase)[*SymbolsCount].Address && ! First)
              {
                (*SymbolsCount)++;
              }
            (*SymbolsBase)[*SymbolsCount].Address = Address;
            (*SymbolsBase)[*SymbolsCount].FileOffset = FindOrAddString((char *) StabStringsBase
                                                                       + StabEntry[i].n_strx,
                                                                       StringsLength,
                                                                       StringsBase);
            (*SymbolsBase)[*SymbolsCount].FunctionOffset = 0;
            (*SymbolsBase)[*SymbolsCount].SourceLine = 0;
            LastFunctionAddress = 0;
	    break;
          case N_FUN:
	    if (0 == StabEntry[i].n_desc || StabEntry[i].n_value < ImageBase) /* line # 0 isn't valid */
              {
                continue;
              }
            Address = StabEntry[i].n_value - ImageBase;
            if (Address != (*SymbolsBase)[*SymbolsCount].Address && ! First)
              {
                (*SymbolsCount)++;
                (*SymbolsBase)[*SymbolsCount].FileOffset = (*SymbolsBase)[*SymbolsCount - 1].FileOffset;
              }
            (*SymbolsBase)[*SymbolsCount].Address = Address;
            if (sizeof(FuncName) <= strlen((char *) StabStringsBase + StabEntry[i].n_strx))
              {
                free(*SymbolsBase);
                fprintf(stderr, "Function name too long\n");
                return 1;
              }
            strcpy(FuncName, (char *) StabStringsBase + StabEntry[i].n_strx);
            Name = strchr(FuncName, ':');
            if (NULL != Name)
              {
                *Name = '\0';
              }
            (*SymbolsBase)[*SymbolsCount].FunctionOffset = FindOrAddString(FuncName,
                                                                           StringsLength,
                                                                           StringsBase);
            (*SymbolsBase)[*SymbolsCount].SourceLine = 0;
            LastFunctionAddress = Address;
	    break;
          case N_SLINE:
            if (0 == LastFunctionAddress)
              {
                Address = StabEntry[i].n_value - ImageBase;
              }
            else
              {
                Address = LastFunctionAddress + StabEntry[i].n_value;
              }
            if (Address != (*SymbolsBase)[*SymbolsCount].Address && ! First)
              {
                (*SymbolsCount)++;
                (*SymbolsBase)[*SymbolsCount].FileOffset = (*SymbolsBase)[*SymbolsCount - 1].FileOffset;
                (*SymbolsBase)[*SymbolsCount].FunctionOffset = (*SymbolsBase)[*SymbolsCount - 1].FunctionOffset;
              }
            (*SymbolsBase)[*SymbolsCount].Address = Address;
            (*SymbolsBase)[*SymbolsCount].SourceLine = StabEntry[i].n_desc;
            break;
          default:
	    continue;
        }
      First = 0;
    }
  (*SymbolsCount)++;

  qsort(*SymbolsBase, *SymbolsCount, sizeof(ROSSYM_ENTRY), (int (*)(const void *, const void *)) CompareSymEntry);

  return 0;
}

static int
ConvertCoffs(ULONG *SymbolsCount, PROSSYM_ENTRY *SymbolsBase,
             ULONG *StringsLength, void *StringsBase,
             ULONG CoffSymbolsLength, void *CoffSymbolsBase,
             ULONG CoffStringsLength, void *CoffStringsBase,
             ULONG_PTR ImageBase, PIMAGE_FILE_HEADER PEFileHeader,
             PIMAGE_SECTION_HEADER PESectionHeaders)
{
  ULONG Count, i;
  PCOFF_SYMENT CoffEntry;
  char FuncName[256];
  char *p;

  CoffEntry = (PCOFF_SYMENT) CoffSymbolsBase;
  Count = CoffSymbolsLength / sizeof(COFF_SYMENT);
  *SymbolsBase = malloc(Count * sizeof(ROSSYM_ENTRY));
  if (NULL == *SymbolsBase)
    {
      fprintf(stderr, "Unable to allocate memory for converted COFF symbols\n");
      return 1;
    }
  *SymbolsCount = 0;

  for (i = 0; i < Count; i++)
    {
      if (ISFCN(CoffEntry[i].e_type) || C_EXT == CoffEntry[i].e_sclass)
        {
          (*SymbolsBase)[*SymbolsCount].Address = CoffEntry[i].e_value;
          if (0 < CoffEntry[i].e_scnum)
            {
              if (PEFileHeader->NumberOfSections < CoffEntry[i].e_scnum)
                {
                  free(*SymbolsBase);
                  fprintf(stderr, "Invalid section number %d in COFF symbols (only %d sections present)\n",
                          CoffEntry[i].e_scnum, PEFileHeader->NumberOfSections);
                  return 1;
                }
              (*SymbolsBase)[*SymbolsCount].Address += PESectionHeaders[CoffEntry[i].e_scnum - 1].VirtualAddress;
            }
          (*SymbolsBase)[*SymbolsCount].FileOffset = 0;
          if (0 == CoffEntry[i].e.e.e_zeroes)
            {
              if (sizeof(FuncName) <= strlen((char *) CoffStringsBase + CoffEntry[i].e.e.e_offset))
                {
                  free(*SymbolsBase);
                  fprintf(stderr, "Function name too long\n");
                  return 1;
                }
              strcpy(FuncName, (char *) CoffStringsBase + CoffEntry[i].e.e.e_offset);
            }
          else
            {
              memcpy(FuncName, CoffEntry[i].e.e_name, E_SYMNMLEN);
              FuncName[E_SYMNMLEN] = '\0';
            }

          /* Name demangling: stdcall */
          p = strrchr(FuncName, '@');
          if (NULL != p)
            {
              *p = '\0';
            }
          p = ('_' == FuncName[0] || '@' == FuncName[0] ? FuncName + 1 : FuncName);
          (*SymbolsBase)[*SymbolsCount].FunctionOffset = FindOrAddString(p,
                                                                         StringsLength,
                                                                         StringsBase);
          (*SymbolsBase)[*SymbolsCount].SourceLine = 0;
          (*SymbolsCount)++;
        }
      i += CoffEntry[i].e_numaux;
    }

  qsort(*SymbolsBase, *SymbolsCount, sizeof(ROSSYM_ENTRY), (int (*)(const void *, const void *)) CompareSymEntry);

  return 0;
}

static int
MergeStabsAndCoffs(ULONG *MergedSymbolCount, PROSSYM_ENTRY *MergedSymbols,
                   ULONG StabSymbolsCount, PROSSYM_ENTRY StabSymbols,
                   ULONG CoffSymbolsCount, PROSSYM_ENTRY CoffSymbols)
{
  ULONG StabIndex, j;
  ULONG CoffIndex;
  ULONG_PTR StabFunctionStartAddress;
  ULONG StabFunctionStringOffset, NewStabFunctionStringOffset;

  *MergedSymbols = malloc(StabSymbolsCount * sizeof(ROSSYM_ENTRY));
  if (NULL == *MergedSymbols)
    {
      fprintf(stderr, "Unable to allocate memory for merged symbols\n");
      return 1;
    }
  *MergedSymbolCount = 0;

  StabFunctionStartAddress = 0;
  StabFunctionStringOffset = 0;
  CoffIndex = 0;
  for (StabIndex = 0; StabIndex < StabSymbolsCount; StabIndex++)
    {
      (*MergedSymbols)[*MergedSymbolCount] = StabSymbols[StabIndex];
      for (j = StabIndex + 1;
           j < StabSymbolsCount && StabSymbols[j].Address == StabSymbols[StabIndex].Address;
           j++)
        {
          if (0 != StabSymbols[j].FileOffset && 0 == (*MergedSymbols)[*MergedSymbolCount].FileOffset)
            {
              (*MergedSymbols)[*MergedSymbolCount].FileOffset = StabSymbols[j].FileOffset;
            }
          if (0 != StabSymbols[j].FunctionOffset && 0 == (*MergedSymbols)[*MergedSymbolCount].FunctionOffset)
            {
              (*MergedSymbols)[*MergedSymbolCount].FunctionOffset = StabSymbols[j].FunctionOffset;
            }
          if (0 != StabSymbols[j].SourceLine && 0 == (*MergedSymbols)[*MergedSymbolCount].SourceLine)
            {
              (*MergedSymbols)[*MergedSymbolCount].SourceLine = StabSymbols[j].SourceLine;
            }
        }
      StabIndex = j - 1;
      while (CoffIndex < CoffSymbolsCount
          && CoffSymbols[CoffIndex + 1].Address <= (*MergedSymbols)[*MergedSymbolCount].Address)
        {
          CoffIndex++;
        }
      NewStabFunctionStringOffset = (*MergedSymbols)[*MergedSymbolCount].FunctionOffset;
      if (CoffSymbols[CoffIndex].Address < (*MergedSymbols)[*MergedSymbolCount].Address
          && StabFunctionStartAddress < CoffSymbols[CoffIndex].Address
          && 0 != CoffSymbols[CoffIndex].FunctionOffset)
        {
          (*MergedSymbols)[*MergedSymbolCount].FunctionOffset = CoffSymbols[CoffIndex].FunctionOffset;
        }
      if (StabFunctionStringOffset != NewStabFunctionStringOffset)
        {
          StabFunctionStartAddress = (*MergedSymbols)[*MergedSymbolCount].Address;
        }
      StabFunctionStringOffset = NewStabFunctionStringOffset;
      (*MergedSymbolCount)++;
    }

  return 0;
}

static PIMAGE_SECTION_HEADER
FindSectionForRVA(DWORD RVA, unsigned NumberOfSections, PIMAGE_SECTION_HEADER SectionHeaders)
{
  unsigned Section;

  for (Section = 0; Section < NumberOfSections; Section++)
    {
      if (SectionHeaders[Section].VirtualAddress <= RVA &&
          RVA < SectionHeaders[Section].VirtualAddress + SectionHeaders[Section].Misc.VirtualSize)
        {
          return SectionHeaders + Section;
        }
    }

  return NULL;
}

static int
IncludeRelocationsForSection(PIMAGE_SECTION_HEADER SectionHeader)
{
  static char *BlacklistedSections[] =
    {
      ".idata",
      ".reloc"
    };
  char SectionName[IMAGE_SIZEOF_SHORT_NAME];
  unsigned i;

  if (0 != (SectionHeader->Characteristics & IMAGE_SCN_LNK_REMOVE))
    {
      return 0;
    }

  for (i = 0; i < sizeof(BlacklistedSections) / sizeof(BlacklistedSections[0]); i++)
    {
      strncpy(SectionName, BlacklistedSections[i], IMAGE_SIZEOF_SHORT_NAME);
      if (0 == memcmp(SectionName, SectionHeader->Name, IMAGE_SIZEOF_SHORT_NAME))
        {
          return 0;
        }
    }

  return 1;
}

static int
ProcessRelocations(ULONG *ProcessedRelocsLength, void **ProcessedRelocs,
                   void *RawData, PIMAGE_OPTIONAL_HEADER OptHeader,
                   unsigned NumberOfSections, PIMAGE_SECTION_HEADER SectionHeaders)
{
  PIMAGE_SECTION_HEADER RelocSectionHeader, TargetSectionHeader;
  PIMAGE_BASE_RELOCATION BaseReloc, End, AcceptedRelocs;
  int Found;

  if (OptHeader->NumberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_BASERELOC
      || 0 == OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
    {
      /* No relocation entries */
      *ProcessedRelocsLength = 0;
      *ProcessedRelocs = NULL;
      return 0;
    }

  RelocSectionHeader = FindSectionForRVA(OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                                         NumberOfSections, SectionHeaders);
  if (NULL == RelocSectionHeader)
    {
      fprintf(stderr, "Can't find section header for relocation data\n");
      return 1;
    }

  *ProcessedRelocs = malloc(RelocSectionHeader->SizeOfRawData);
  if (NULL == *ProcessedRelocs)
    {
      fprintf(stderr, "Failed to allocate %lu bytes for relocations\n", RelocSectionHeader->SizeOfRawData);
      return 1;
    }
  *ProcessedRelocsLength = 0;

  BaseReloc = (PIMAGE_BASE_RELOCATION) ((char *) RawData
                                        + RelocSectionHeader->PointerToRawData
                                        + (OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress -
                                           RelocSectionHeader->VirtualAddress));
  End = (PIMAGE_BASE_RELOCATION) ((char *) BaseReloc
                                  + OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);

  while (BaseReloc < End && 0 < BaseReloc->SizeOfBlock)
    {
      TargetSectionHeader = FindSectionForRVA(BaseReloc->VirtualAddress, NumberOfSections,
                                              SectionHeaders);
      if (NULL != TargetSectionHeader && IncludeRelocationsForSection(TargetSectionHeader))
        {
          AcceptedRelocs = *ProcessedRelocs;
          Found = 0;
          while (AcceptedRelocs < (PIMAGE_BASE_RELOCATION) ((char *) *ProcessedRelocs +
                                                            *ProcessedRelocsLength)
                 && ! Found)
            {
              Found = BaseReloc->SizeOfBlock == AcceptedRelocs->SizeOfBlock
                      && 0 == memcmp(BaseReloc, AcceptedRelocs, AcceptedRelocs->SizeOfBlock);
              AcceptedRelocs= (PIMAGE_BASE_RELOCATION) ((char *) AcceptedRelocs +
                                                        AcceptedRelocs->SizeOfBlock);
            }
          if (! Found)
            {
              memcpy((char *) *ProcessedRelocs + *ProcessedRelocsLength,
                     BaseReloc, BaseReloc->SizeOfBlock);
              *ProcessedRelocsLength += BaseReloc->SizeOfBlock;
            }
        }
      BaseReloc = (PIMAGE_BASE_RELOCATION)((char *) BaseReloc + BaseReloc->SizeOfBlock);
    }

  return 0;
}

static int
CreateOutputFile(FILE *OutFile, void *InData,
                 PIMAGE_DOS_HEADER InDosHeader, PIMAGE_FILE_HEADER InFileHeader,
                 PIMAGE_OPTIONAL_HEADER InOptHeader, PIMAGE_SECTION_HEADER InSectionHeaders,
                 ULONG RosSymLength, void *RosSymSection)
{
  ULONG StartOfRawData;
  unsigned Section;
  void *OutHeader, *ProcessedRelocs, *PaddedRosSym, *Data;
  PIMAGE_DOS_HEADER OutDosHeader;
  PIMAGE_FILE_HEADER OutFileHeader;
  PIMAGE_OPTIONAL_HEADER OutOptHeader;
  PIMAGE_SECTION_HEADER OutSectionHeaders, CurrentSectionHeader;
  DWORD CheckSum;
  ULONG Length, i;
  ULONG ProcessedRelocsLength;
  ULONG RosSymOffset, RosSymFileLength;
  int InRelocSectionIndex;
  PIMAGE_SECTION_HEADER OutRelocSection;

  StartOfRawData = 0;
  for (Section = 0; Section < InFileHeader->NumberOfSections; Section++)
    {
      if ((0 == StartOfRawData
           || InSectionHeaders[Section].PointerToRawData < StartOfRawData)
          && 0 != InSectionHeaders[Section].PointerToRawData
          && 0 == (InSectionHeaders[Section].Characteristics & IMAGE_SCN_LNK_REMOVE))
        {
          StartOfRawData = InSectionHeaders[Section].PointerToRawData;
        }
    }
  OutHeader = malloc(StartOfRawData);
  if (NULL == OutHeader)
    {
      fprintf(stderr, "Failed to allocate %lu bytes for output file header\n", StartOfRawData);
      return 1;
    }
  memset(OutHeader, '\0', StartOfRawData);

  OutDosHeader = (PIMAGE_DOS_HEADER) OutHeader;
  memcpy(OutDosHeader, InDosHeader, InDosHeader->e_lfanew + sizeof(ULONG));

  OutFileHeader = (PIMAGE_FILE_HEADER)((char *) OutHeader + OutDosHeader->e_lfanew + sizeof(ULONG));
  memcpy(OutFileHeader, InFileHeader, sizeof(IMAGE_FILE_HEADER));
  OutFileHeader->PointerToSymbolTable = 0;
  OutFileHeader->NumberOfSymbols = 0;
  OutFileHeader->Characteristics &= ~(IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_LOCAL_SYMS_STRIPPED |
                                      IMAGE_FILE_DEBUG_STRIPPED);

  OutOptHeader = (PIMAGE_OPTIONAL_HEADER)(OutFileHeader + 1);
  memcpy(OutOptHeader, InOptHeader, sizeof(IMAGE_OPTIONAL_HEADER));
  OutOptHeader->CheckSum = 0;

  OutSectionHeaders = (PIMAGE_SECTION_HEADER)((char *) OutOptHeader + OutFileHeader->SizeOfOptionalHeader);

  if (ProcessRelocations(&ProcessedRelocsLength, &ProcessedRelocs, InData, InOptHeader,
                         InFileHeader->NumberOfSections, InSectionHeaders))
    {
      return 1;
    }
  if (InOptHeader->NumberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_BASERELOC
      || 0 == InOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
    {
      InRelocSectionIndex = -1;
    }
  else
    {
      InRelocSectionIndex = FindSectionForRVA(InOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                                              InFileHeader->NumberOfSections, InSectionHeaders) - InSectionHeaders;
    }

  OutFileHeader->NumberOfSections = 0;
  CurrentSectionHeader = OutSectionHeaders;
  OutOptHeader->SizeOfImage = 0;
  RosSymOffset = 0;
  OutRelocSection = NULL;
  for (Section = 0; Section < InFileHeader->NumberOfSections; Section++)
    {
      if (0 == (InSectionHeaders[Section].Characteristics & IMAGE_SCN_LNK_REMOVE))
        {
          *CurrentSectionHeader = InSectionHeaders[Section];
          CurrentSectionHeader->PointerToLinenumbers = 0;
          CurrentSectionHeader->NumberOfLinenumbers = 0;
          if (OutOptHeader->SizeOfImage < CurrentSectionHeader->VirtualAddress +
                                          CurrentSectionHeader->Misc.VirtualSize)
            {
              OutOptHeader->SizeOfImage = ROUND_UP(CurrentSectionHeader->VirtualAddress +
                                                   CurrentSectionHeader->Misc.VirtualSize,
                                                   OutOptHeader->SectionAlignment);
            }
          if (RosSymOffset < CurrentSectionHeader->PointerToRawData + CurrentSectionHeader->SizeOfRawData)
            {
              RosSymOffset = CurrentSectionHeader->PointerToRawData + CurrentSectionHeader->SizeOfRawData;
            }
          if (Section == InRelocSectionIndex)
            {
              OutRelocSection = CurrentSectionHeader;
            }
          (OutFileHeader->NumberOfSections) ++;
          CurrentSectionHeader++;
        }
    }

  if (OutRelocSection == CurrentSectionHeader - 1)
    {
      OutOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = ProcessedRelocsLength;
      if (OutOptHeader->SizeOfImage == OutRelocSection->VirtualAddress +
                                       ROUND_UP(OutRelocSection->Misc.VirtualSize,
                                                OutOptHeader->SectionAlignment))
        {
          OutOptHeader->SizeOfImage = OutRelocSection->VirtualAddress +
                                      ROUND_UP(ProcessedRelocsLength,
                                               OutOptHeader->SectionAlignment);
        }
      OutRelocSection->Misc.VirtualSize = ProcessedRelocsLength;
      if (RosSymOffset == OutRelocSection->PointerToRawData
                          + OutRelocSection->SizeOfRawData)
        {
          RosSymOffset = OutRelocSection->PointerToRawData +
                         ROUND_UP(ProcessedRelocsLength, OutOptHeader->FileAlignment);
        }
      OutRelocSection->SizeOfRawData = ROUND_UP(ProcessedRelocsLength,
                                                OutOptHeader->FileAlignment);
    }

  RosSymFileLength = ROUND_UP(RosSymLength, OutOptHeader->FileAlignment);
  memcpy(CurrentSectionHeader->Name, ".rossym", 8); /* We're lucky: string is exactly 8 bytes long */
  CurrentSectionHeader->Misc.VirtualSize = RosSymLength;
  CurrentSectionHeader->VirtualAddress = OutOptHeader->SizeOfImage;
  CurrentSectionHeader->SizeOfRawData = RosSymFileLength;
  CurrentSectionHeader->PointerToRawData = RosSymOffset;
  CurrentSectionHeader->PointerToRelocations = 0;
  CurrentSectionHeader->PointerToLinenumbers = 0;
  CurrentSectionHeader->NumberOfRelocations = 0;
  CurrentSectionHeader->NumberOfLinenumbers = 0;
  CurrentSectionHeader->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE
                                          | IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_TYPE_NOLOAD;
  OutOptHeader->SizeOfImage = ROUND_UP(CurrentSectionHeader->VirtualAddress +
                                       CurrentSectionHeader->Misc.VirtualSize,
                                       OutOptHeader->SectionAlignment);
  (OutFileHeader->NumberOfSections)++;

  PaddedRosSym = malloc(RosSymFileLength);
  if (NULL == PaddedRosSym)
    {
      fprintf(stderr, "Failed to allocate %lu bytes for padded .rossym\n", RosSymFileLength);
      return 1;
    }
  memcpy(PaddedRosSym, RosSymSection, RosSymLength);
  memset((char *) PaddedRosSym + RosSymLength, '\0', RosSymFileLength - RosSymLength);

  CheckSum = 0;
  for (i = 0; i < StartOfRawData / 2; i++)
   {
      CheckSum += ((unsigned short*) OutHeader)[i];
      CheckSum = 0xffff & (CheckSum + (CheckSum >> 16));
   }
  Length = StartOfRawData;
  for (Section = 0; Section < OutFileHeader->NumberOfSections; Section++)
    {
      if (OutRelocSection == OutSectionHeaders + Section)
        {
          Data = (void *) ProcessedRelocs;
        }
      else if (Section + 1 == OutFileHeader->NumberOfSections)
        {
          Data = (void *) PaddedRosSym;
        }
      else
        {
          Data = (void *) ((char *) InData + OutSectionHeaders[Section].PointerToRawData);
        }
      for (i = 0; i < OutSectionHeaders[Section].SizeOfRawData / 2; i++)
        {
          CheckSum += ((unsigned short*) Data)[i];
          CheckSum = 0xffff & (CheckSum + (CheckSum >> 16));
        }
      Length += OutSectionHeaders[Section].SizeOfRawData;
    }
  CheckSum += Length;
  OutOptHeader->CheckSum = CheckSum;

  if (fwrite(OutHeader, 1, StartOfRawData, OutFile) != StartOfRawData)
    {
      perror("Error writing output header\n");
      free(OutHeader);
      return 1;
    }

  for (Section = 0; Section < OutFileHeader->NumberOfSections; Section++)
    {
      if (0 != OutSectionHeaders[Section].SizeOfRawData)
        {
          fseek(OutFile, OutSectionHeaders[Section].PointerToRawData, SEEK_SET);
          if (OutRelocSection == OutSectionHeaders + Section)
            {
              Data = (void *) ProcessedRelocs;
            }
          else if (Section + 1 == OutFileHeader->NumberOfSections)
            {
              Data = (void *) PaddedRosSym;
            }
          else
            {
              Data = (void *) ((char *) InData + OutSectionHeaders[Section].PointerToRawData);
            }
          if (fwrite(Data, 1, OutSectionHeaders[Section].SizeOfRawData, OutFile) !=
              OutSectionHeaders[Section].SizeOfRawData)
            {
              perror("Error writing section data\n");
              free(PaddedRosSym);
              free(OutHeader);
              return 1;
            }
        }
   }

  free(PaddedRosSym);
  free(OutHeader);

  return 0;
}

int main(int argc, char* argv[])
{
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  ULONG ImageBase;
  void *StabBase;
  ULONG StabsLength;
  void *StabStringBase;
  ULONG StabStringsLength;
  void *CoffBase;
  ULONG CoffsLength;
  void *CoffStringBase;
  ULONG CoffStringsLength;
  char* path1;
  char* path2;
  FILE* in;
  FILE* out;
  int n_in;
  void *StringBase;
  ULONG StringsLength;
  ULONG StabSymbolsCount;
  PROSSYM_ENTRY StabSymbols;
  ULONG CoffSymbolsCount;
  PROSSYM_ENTRY CoffSymbols;
  ULONG MergedSymbolsCount;
  PROSSYM_ENTRY MergedSymbols;
  long FileSize;
  void *FileData;
  ULONG RosSymLength;
  void *RosSymSection;

  if (3 != argc)
    {
      fprintf(stderr, "Usage: rsym <exefile> <symfile>\n");
      exit(1);
    }

  path1 = convert_path(argv[1]);
  path2 = convert_path(argv[2]);

  in = fopen(path1, "rb");
  if (in == NULL)
    {
      perror("Cannot open input file");
      exit(1);
    }
  fseek(in, 0L, SEEK_END);
  FileSize = ftell(in);
  fseek(in, 0L, SEEK_SET);
  FileData = malloc(FileSize);
  if (NULL == FileData)
    {
      fclose(in);
      fprintf(stderr, "Can't allocate %ld bytes to read input file\n", FileSize);
      exit(1);
    }
  n_in = fread(FileData, 1, FileSize, in);
  if (n_in != FileSize)
    { 
      perror("Error reading from input file");
      free(FileData);
      fclose(in);
      exit(1);
    }
  fclose(in);

  /* Check if MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) FileData;
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
    {
      perror("Input file is not a PE image.\n");
      free(FileData);
      exit(1);
    }

  /* Locate PE file header  */
  /* sizeof(ULONG) = sizeof(MAGIC) */
  PEFileHeader = (PIMAGE_FILE_HEADER)((char *) FileData + PEDosHeader->e_lfanew + sizeof(ULONG));

  /* Locate optional header */
  PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
  ImageBase = PEOptHeader->ImageBase;

  /* Locate PE section headers  */
  PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *) PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

  if (GetStabInfo(FileData, PEFileHeader, PESectionHeaders, &StabsLength, &StabBase,
                  &StabStringsLength, &StabStringBase))
    {
      free(FileData);
      exit(1);
    }

  if (GetCoffInfo(FileData, PEFileHeader, PESectionHeaders, &CoffsLength, &CoffBase,
                  &CoffStringsLength, &CoffStringBase))
    {
      free(FileData);
      exit(1);
    }

  StringBase = malloc(1 + StabStringsLength + CoffStringsLength +
                      (CoffsLength / sizeof(ROSSYM_ENTRY)) * (E_SYMNMLEN + 1));
  if (NULL == StringBase)
    {
      free(FileData);
      fprintf(stderr, "Failed to allocate memory for strings table\n");
      exit(1);
    }
  /* Make offset 0 into an empty string */
  *((char *) StringBase) = '\0';
  StringsLength = 1;

  if (ConvertStabs(&StabSymbolsCount, &StabSymbols, &StringsLength, StringBase,
                   StabsLength, StabBase, StabStringsLength, StabStringBase,
                   ImageBase, PEFileHeader, PESectionHeaders))
    {
      free(StringBase);
      free(FileData);
      fprintf(stderr, "Failed to allocate memory for strings table\n");
      exit(1);
    }

  if (ConvertCoffs(&CoffSymbolsCount, &CoffSymbols, &StringsLength, StringBase,
                   CoffsLength, CoffBase, CoffStringsLength, CoffStringBase,
                   ImageBase, PEFileHeader, PESectionHeaders))
    {
      free(StabSymbols);
      free(StringBase);
      free(FileData);
      exit(1);
    }

  if (MergeStabsAndCoffs(&MergedSymbolsCount, &MergedSymbols,
                         StabSymbolsCount, StabSymbols,
                         CoffSymbolsCount, CoffSymbols))
    {
      free(CoffSymbols);
      free(StabSymbols);
      free(StringBase);
      free(FileData);
      exit(1);
    }

  free(CoffSymbols);
  free(StabSymbols);

  RosSymLength = sizeof(SYMBOLFILE_HEADER) + MergedSymbolsCount * sizeof(ROSSYM_ENTRY)
                 + StringsLength;
  RosSymSection = malloc(RosSymLength);
  if (NULL == RosSymSection)
    {
      free(MergedSymbols);
      free(StringBase);
      free(FileData);
      fprintf(stderr, "Unable to allocate memory for .rossym section\n");
      exit(1);
    }
  memset(RosSymSection, '\0', RosSymLength);

  SymbolFileHeader = (PSYMBOLFILE_HEADER) RosSymSection;
  SymbolFileHeader->SymbolsOffset = sizeof(SYMBOLFILE_HEADER);
  SymbolFileHeader->SymbolsLength = MergedSymbolsCount * sizeof(ROSSYM_ENTRY);
  SymbolFileHeader->StringsOffset = SymbolFileHeader->SymbolsOffset + SymbolFileHeader->SymbolsLength;
  SymbolFileHeader->StringsLength = StringsLength;

  memcpy((char *) RosSymSection + SymbolFileHeader->SymbolsOffset, MergedSymbols,
         SymbolFileHeader->SymbolsLength);
  memcpy((char *) RosSymSection + SymbolFileHeader->StringsOffset, StringBase,
         SymbolFileHeader->StringsLength);

  free(MergedSymbols);
  free(StringBase);

  out = fopen(path2, "wb");
  if (out == NULL)
    {
      perror("Cannot open output file");
      free(RosSymSection);
      free(FileData);
      exit(1);
    }

  if (CreateOutputFile(out, FileData, PEDosHeader, PEFileHeader, PEOptHeader,
                       PESectionHeaders, RosSymLength, RosSymSection))
    {
      fclose(out);
      free(RosSymSection);
      free(FileData);
      exit(1);
    }

  fclose(out);
  free(RosSymSection);
  free(FileData);

  return 0;
}

/* EOF */
