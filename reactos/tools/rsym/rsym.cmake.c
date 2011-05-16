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
#include <assert.h>

#include "rsym.h"

int main(int argc, char* argv[])
{
  int i;
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  char* path1;
  char* path2;
  FILE* out;
  size_t FileSize;
  void *FileData;
  char elfhdr[] = { '\377', 'E', 'L', 'F' };

  if (3 != argc)
    {
      fprintf(stderr, "Usage: rsym <exefile> <symfile>\n");
      exit(1);
    }

  path1 = convert_path(argv[1]);
  path2 = convert_path(argv[2]);

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
      /* Ignore elf */
      if (!memcmp(PEDosHeader, elfhdr, sizeof(elfhdr)))
	exit(0);
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

  /* Locate PE section headers  */
  PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *) PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

  for (i = 0; i < PEFileHeader->NumberOfSections; i++) {
	  if (PESectionHeaders[i].Name[0] == '/') {
		  PESectionHeaders[i].Characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
		  PESectionHeaders[i].Characteristics &= ~(IMAGE_SCN_MEM_PURGEABLE | IMAGE_SCN_MEM_DISCARDABLE);
	  }
  }

  PESectionHeaders[PEFileHeader->NumberOfSections-1].SizeOfRawData =
	  FileSize - PESectionHeaders[PEFileHeader->NumberOfSections-1].PointerToRawData;
  if (PESectionHeaders[PEFileHeader->NumberOfSections-1].SizeOfRawData >
	  PESectionHeaders[PEFileHeader->NumberOfSections-1].Misc.VirtualSize) {
	  PESectionHeaders[PEFileHeader->NumberOfSections-1].Misc.VirtualSize =
		  ROUND_UP(PESectionHeaders[PEFileHeader->NumberOfSections-1].SizeOfRawData, 
				   PEOptHeader->SectionAlignment);
	  PEOptHeader->SizeOfImage = PESectionHeaders[PEFileHeader->NumberOfSections-1].VirtualAddress + PESectionHeaders[PEFileHeader->NumberOfSections-1].Misc.VirtualSize;
  }

  out = fopen(path2, "wb");
  if (out == NULL)
    {
      perror("Cannot open output file");
      free(FileData);
      exit(1);
    }

  fwrite(FileData, 1, FileSize, out);
  fclose(out);
  free(FileData);

  return 0;
}

/* EOF */
