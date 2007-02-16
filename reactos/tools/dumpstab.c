/*
 * Usage: dumpstab input-file
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
#include <assert.h>

#include "rsym.h"

const char*
stab_type_name ( int stab_type )
{
	static char buf[32];
	switch ( stab_type )
	{
#define X(n) case n: return #n;
		X(N_GYSM)
		X(N_FNAME)
		X(N_FUN)
		X(N_STSYM)
		X(N_LCSYM)
		X(N_MAIN)
		X(N_PC)
		X(N_NSYMS)
		X(N_NOMAP)
		X(N_RSYM)
		X(N_M2C)
		X(N_SLINE)
		X(N_DSLINE)
		X(N_BSLINE)
		//X(N_BROWS)
		X(N_DEFD)
		X(N_EHDECL)
		//X(N_MOD2)
		X(N_CATCH)
		X(N_SSYM)
		X(N_SO)
		X(N_LSYM)
		X(N_BINCL)
		X(N_SOL)
		X(N_PSYM)
		X(N_EINCL)
		X(N_ENTRY)
		X(N_LBRAC)
		X(N_EXCL)
		X(N_SCOPE)
		X(N_RBRAC)
		X(N_BCOMM)
		X(N_ECOMM)
		X(N_ECOML)
		X(N_LENG)
	}
	sprintf ( buf, "%lu", stab_type );
	return buf;
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

static void
IterateStabs(ULONG StabSymbolsLength, void *StabSymbolsBase,
             ULONG StabStringsLength, void *StabStringsBase,
             ULONG_PTR ImageBase, PIMAGE_FILE_HEADER PEFileHeader,
             PIMAGE_SECTION_HEADER PESectionHeaders)
{
  PSTAB_ENTRY e;
  ULONG Count, i;

  e = StabSymbolsBase;
  Count = StabSymbolsLength / sizeof(STAB_ENTRY);
  if (Count == 0) /* No symbol info */
    return;

  printf ( "type,other,desc,value,str\n" );
  for (i = 0; i < Count; i++)
    {
	  printf ( "%s,%lu(0x%x),%lu(0x%x),%lu(0x%x),%s\n",
		  stab_type_name(e[i].n_type),
		  e[i].n_other,
		  e[i].n_other,
		  e[i].n_desc,
		  e[i].n_desc,
		  e[i].n_value,
		  e[i].n_value,
		  (char *) StabStringsBase + e[i].n_strx );
    }
}

int main(int argc, char* argv[])
{
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  ULONG ImageBase;
  void *StabBase;
  ULONG StabsLength;
  void *StabStringBase;
  ULONG StabStringsLength;
  char* path1;
  size_t FileSize;
  void *FileData;

  if (2 != argc)
    {
      fprintf(stderr, "Usage: dumpstabs <exefile>\n");
      exit(1);
    }

  path1 = convert_path(argv[1]);

  FileData = load_file ( path1, &FileSize );
  if ( !FileData )
  {
    fprintf ( stderr, "An error occured loading '%s'\n", path1 );
    exit(1);
  }

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
  assert(sizeof(ULONG) == 4);
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

  IterateStabs( StabsLength, StabBase, StabStringsLength, StabStringBase,
                ImageBase, PEFileHeader, PESectionHeaders);

  free(FileData);

  return 0;
}
