/*
 * Usage: rsym input-file output-file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IMAGE_DOS_MAGIC 0x5a4d
#define IMAGE_PE_MAGIC 0x00004550

#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef void* PVOID;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef signed long LONG;
typedef unsigned long ULONG;

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
#pragma pack(push,4)
typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;
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
typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
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
#pragma pack(pop)

typedef struct _SYMBOLFILE_HEADER {
  unsigned long StabsOffset;
  unsigned long StabsLength;
  unsigned long StabstrOffset;
  unsigned long StabstrLength;
  unsigned long SymbolsOffset;
  unsigned long SymbolsLength;
  unsigned long SymbolstrOffset;
  unsigned long SymbolstrLength;
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

typedef struct
{
   unsigned long OldOffset;
   unsigned long NewOffset;
   char* Name;
   unsigned long Length;
} STR_ENTRY, *PSTR_ENTRY;

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
typedef struct _EXTERNAL_SYMENT
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
} EXTERNAL_SYMENT, *PEXTERNAL_SYMENT;
#pragma pack(pop)

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

static void
RelocateString(ULONG *Offset, PSTR_ENTRY StrEntry, ULONG *StrCount, PVOID SymbolStringsBase)
{
  ULONG i;

  for (i = 0; i < *StrCount; i++)
    {
      if (*Offset == StrEntry[i].OldOffset)
        {
          *Offset = StrEntry[i].NewOffset;
          return;
        }
    }

  StrEntry[*StrCount].OldOffset = *Offset;
  StrEntry[*StrCount].Name = (char*) SymbolStringsBase + StrEntry[*StrCount].OldOffset;
  StrEntry[*StrCount].Length = strlen(StrEntry[*StrCount].Name) + 1;
  if (*StrCount == 0)
    {
      StrEntry[*StrCount].NewOffset = 0;
    }
  else
    {
      StrEntry[*StrCount].NewOffset = StrEntry[*StrCount - 1].NewOffset
                                      + StrEntry[*StrCount - 1].Length;
    }
  *Offset = StrEntry[*StrCount].NewOffset;
  (*StrCount)++;
}

static int
CompareSyment(const PEXTERNAL_SYMENT SymEntry1, const PEXTERNAL_SYMENT SymEntry2)
{
  if (SymEntry1->e_value < SymEntry2->e_value)
    {
      return -1;
    }

  if (SymEntry2->e_value < SymEntry1->e_value)
    {
      return +1;
    }

  return 0;
} 

#define TRANSFER_SIZE      (65536)

int main(int argc, char* argv[])
{
  SYMBOLFILE_HEADER SymbolFileHeader;
  IMAGE_DOS_HEADER PEDosHeader;
  IMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  ULONG ImageBase;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
  PVOID CoffSymbolsBase;
  ULONG CoffSymbolsLength;
  PVOID CoffSymbolStringsBase;
  ULONG CoffSymbolStringsLength;
  ULONG Idx;
  char* path1;
  char* path2;
  FILE* in;
  FILE* out;
  int n_in;
  int n_out;
  PSTAB_ENTRY StabEntry;
  ULONG Count;
  ULONG i;
  ULONG SymbolsCount;
  PSTR_ENTRY StrEntry;
  ULONG StrCount;
  PSTR_ENTRY CoffStrEntry;
  ULONG CoffStrCount;
  PEXTERNAL_SYMENT SymEntry;
  unsigned NumAux;

  if (argc != 3)
    {
      fprintf(stderr, "Too many arguments\n");
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

  out = fopen(path2, "wb");
  if (out == NULL)
    {
      perror("Cannot open output file");
      fclose(in);
      exit(1);
    }

  /* Check if MZ header exists  */
  n_in = fread(&PEDosHeader, 1, sizeof(PEDosHeader), in);
  if (PEDosHeader.e_magic != IMAGE_DOS_MAGIC && PEDosHeader.e_lfanew != 0L)
    {
      perror("Input file is not a PE image.\n");
    }

  /* Read PE file header  */
  /* sizeof(ULONG) = sizeof(MAGIC) */
  fseek(in, PEDosHeader.e_lfanew + sizeof(ULONG), SEEK_SET);
  n_in = fread(&PEFileHeader, 1, sizeof(PEFileHeader), in);

  /* Read optional header */
  PEOptHeader = malloc(PEFileHeader.SizeOfOptionalHeader);
  fread ( PEOptHeader, 1, PEFileHeader.SizeOfOptionalHeader, in );
  ImageBase = PEOptHeader->ImageBase;

  /* Read PE section headers  */
  PESectionHeaders = malloc(PEFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
  fseek(in, PEDosHeader.e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER)
    + sizeof(IMAGE_OPTIONAL_HEADER), SEEK_SET);
  n_in = fread(PESectionHeaders, 1, PEFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER), in);

  /* Copy .stab and .stabstr sections if available */
  SymbolsBase = NULL;
  SymbolsLength = 0;
  SymbolStringsBase = NULL;
  SymbolStringsLength = 0;

  for (Idx = 0; Idx < PEFileHeader.NumberOfSections; Idx++)
    {
      //printf("section: '%.08s'\n", PESectionHeaders[Idx].Name);
      if ((strncmp((char*)PESectionHeaders[Idx].Name, ".stab", 5) == 0)
        && (PESectionHeaders[Idx].Name[5] == 0))
        {
	   //printf(".stab section found. Size %d\n",
           //  PESectionHeaders[Idx].SizeOfRawData);

           SymbolsLength = PESectionHeaders[Idx].SizeOfRawData;
           SymbolsBase = malloc(SymbolsLength);

           fseek(in, PESectionHeaders[Idx].PointerToRawData, SEEK_SET);
           n_in = fread(SymbolsBase, 1, SymbolsLength, in);
        }

      if (strncmp((char*)PESectionHeaders[Idx].Name, ".stabstr", 8) == 0)
        {
	   //printf(".stabstr section found. Size %d\n",
           //  PESectionHeaders[Idx].SizeOfRawData);

           SymbolStringsLength = PESectionHeaders[Idx].SizeOfRawData;
           SymbolStringsBase = malloc(SymbolStringsLength);

           fseek(in, PESectionHeaders[Idx].PointerToRawData, SEEK_SET);
           n_in = fread(SymbolStringsBase, 1, SymbolStringsLength, in);
        }
    }

  StabEntry = SymbolsBase;
  SymbolsCount = SymbolsLength / sizeof(STAB_ENTRY);
  Count = 0;

  for (i = 0; i < SymbolsCount; i++)
    {
      switch ( StabEntry[i].n_type )
      {
      case N_FUN:
	if ( StabEntry[i].n_desc == 0 ) // line # 0 isn't valid
	  continue;
	break;
      case N_SLINE:
	break;
      case N_SO:
	break;
      default:
	continue;
      }
      memmove(&StabEntry[Count], &StabEntry[i], sizeof(STAB_ENTRY));
      if ( StabEntry[Count].n_value >= ImageBase )
	    StabEntry[Count].n_value -= ImageBase;
      Count++;
    }

  StrEntry = malloc(sizeof(STR_ENTRY) * Count);
  StrCount = 0;

  for (i = 0; i < Count; i++)
    {
      RelocateString(&StabEntry[i].n_strx, StrEntry, &StrCount, SymbolStringsBase);
    }

  SymbolFileHeader.StabsOffset = sizeof(SYMBOLFILE_HEADER);
  SymbolFileHeader.StabsLength = Count * sizeof(STAB_ENTRY);
  SymbolFileHeader.StabstrOffset = SymbolFileHeader.StabsOffset + SymbolFileHeader.StabsLength;
  SymbolFileHeader.StabstrLength = StrEntry[StrCount-1].NewOffset + StrEntry[StrCount-1].Length;

  if (0 == PEFileHeader.PointerToSymbolTable || 0 == PEFileHeader.NumberOfSymbols)
    {
      /* No COFF symbol table */
      SymbolFileHeader.SymbolsOffset = 0;
      SymbolFileHeader.SymbolsLength = 0;
      SymbolFileHeader.SymbolstrOffset = 0;
      SymbolFileHeader.SymbolstrLength = 0;
    }
  else
    {
      CoffSymbolsLength = PEFileHeader.NumberOfSymbols * sizeof(EXTERNAL_SYMENT);
      CoffSymbolsBase = malloc(CoffSymbolsLength);
      if (NULL == CoffSymbolsBase)
        {
          fprintf(stderr, "Unable to allocate %u bytes for COFF symbols\n",
                  (unsigned) CoffSymbolsLength);
	  exit(1);
        }
      fseek(in, PEFileHeader.PointerToSymbolTable, SEEK_SET);
      n_in = fread(CoffSymbolsBase, 1, CoffSymbolsLength, in);

      SymEntry = CoffSymbolsBase;
      Count = 0;
      for (i = 0; i < PEFileHeader.NumberOfSymbols; i++)
        {
          NumAux = SymEntry[i].e_numaux;
          if (ISFCN(SymEntry[i].e_type))
            {
              SymEntry[Count] = SymEntry[i];
              if (0 < SymEntry[Count].e_scnum)
                {
                  if (PEFileHeader.NumberOfSections < SymEntry[Count].e_scnum)
                    {
                      fprintf(stderr, "Invalid section number %d in COFF symbols (only %d sections present)\n",
                              SymEntry[Count].e_scnum, PEFileHeader.NumberOfSections);
                      exit(1);
                    }
                  SymEntry[Count].e_value += PESectionHeaders[SymEntry[Count].e_scnum - 1].VirtualAddress;
                  SymEntry[Count].e_scnum = -3;
                  SymEntry[Count].e_numaux = 0;
                }
              Count++;
            }
          i += NumAux;
        }

      qsort(CoffSymbolsBase, Count, sizeof(EXTERNAL_SYMENT), (int (*)(const void *, const void *)) CompareSyment);

      n_in = fread(&CoffSymbolStringsLength, 1, sizeof(ULONG), in);
      CoffSymbolStringsBase = malloc(CoffSymbolStringsLength);
      if (NULL == CoffSymbolStringsBase)
        {
          fprintf(stderr, "Unable to allocate %u bytes for COFF symbol strings\n",
                  (unsigned) CoffSymbolStringsLength);
	  exit(1);
        }
      n_in = fread((char *) CoffSymbolStringsBase + sizeof(ULONG), 1, CoffSymbolStringsLength - 4, in);

      CoffStrEntry = malloc(sizeof(STR_ENTRY) * Count);
      CoffStrCount = 0;

      for (i = 0; i < Count; i++)
        {
          if (0 == SymEntry[i].e.e.e_zeroes)
            {
              RelocateString(&SymEntry[i].e.e.e_offset, CoffStrEntry, &CoffStrCount, CoffSymbolStringsBase);
            }
        }

      SymbolFileHeader.SymbolsOffset = SymbolFileHeader.StabstrOffset + SymbolFileHeader.StabstrLength;
      SymbolFileHeader.SymbolsLength = Count * sizeof(EXTERNAL_SYMENT);
      SymbolFileHeader.SymbolstrOffset = SymbolFileHeader.SymbolsOffset + SymbolFileHeader.SymbolsLength;
      SymbolFileHeader.SymbolstrLength = CoffStrEntry[CoffStrCount - 1].NewOffset
                                         + CoffStrEntry[CoffStrCount - 1].Length;
    }

  n_out = fwrite(&SymbolFileHeader, 1, sizeof(SYMBOLFILE_HEADER), out);
  n_out = fwrite(SymbolsBase, 1, SymbolFileHeader.StabsLength, out);
  for (i = 0; i < StrCount; i++)
    {
      fwrite(StrEntry[i].Name, 1, StrEntry[i].Length, out);
    }
  n_out = fwrite(CoffSymbolsBase, 1, SymbolFileHeader.SymbolsLength, out);
  for (i = 0; i < CoffStrCount; i++)
    {
      fwrite(CoffStrEntry[i].Name, 1, CoffStrEntry[i].Length, out);
    }

  fclose(out);
  exit(0);
}
