/*
 * Usage: raddr2line input-file address/offset
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

#include "rsym.h"

size_t fixup_offset ( size_t ImageBase, size_t offset )
{
	if ( offset >= ImageBase )
		offset -= ImageBase;
	return offset;
}

long
my_atoi ( const char* a )
{
	int i = 0;
	const char* fmt = "%x";

	if ( *a == '0' )
	{
		switch ( *++a )
		{
		case 'x':
			fmt = "%x";
			++a;
			break;
		case 'd':
			fmt = "%d";
			++a;
			break;
		default:
			fmt = "%o";
			break;
		}
	}
	sscanf ( a, fmt, &i );
	return i;
}

PIMAGE_SECTION_HEADER
find_rossym_section ( PIMAGE_FILE_HEADER PEFileHeader,
	PIMAGE_SECTION_HEADER PESectionHeaders )
{
	size_t i;
	for ( i = 0; i < PEFileHeader->NumberOfSections; i++ )
	{
		if ( 0 == strcmp ( (char*)PESectionHeaders[i].Name, ".rossym" ) )
			return &PESectionHeaders[i];
	}
	return NULL;
}

int
find_and_print_offset (
	void* data,
	size_t offset )
{
	PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER)data;
	PROSSYM_ENTRY Entries = (PROSSYM_ENTRY)((char*)data + RosSymHeader->SymbolsOffset);
	char* Strings = (char*)data + RosSymHeader->StringsOffset;
	size_t symbols = RosSymHeader->SymbolsLength / sizeof(ROSSYM_ENTRY);
	size_t i;

	//if ( RosSymHeader->SymbolsOffset )

	for ( i = 0; i < symbols; i++ )
	{
		if ( Entries[i].Address > offset )
		{
			if ( !i-- )
				return 1;
			else
			{
				PROSSYM_ENTRY e = &Entries[i];
				printf ( "%s:%u (%s)\n",
					&Strings[e->FileOffset],
					(unsigned int)e->SourceLine,
					&Strings[e->FunctionOffset] );
				return 0;
			}
		}
	}
	return 1;
}

int
process_data ( const void* FileData, size_t FileSize, size_t offset )
{
	PIMAGE_DOS_HEADER PEDosHeader;
	PIMAGE_FILE_HEADER PEFileHeader;
	PIMAGE_OPTIONAL_HEADER PEOptHeader;
	PIMAGE_SECTION_HEADER PESectionHeaders;
	PIMAGE_SECTION_HEADER PERosSymSectionHeader;
	size_t ImageBase;
	int res;

	/* Check if MZ header exists  */
	PEDosHeader = (PIMAGE_DOS_HEADER)FileData;
	if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
	{
		perror("Input file is not a PE image.\n");
		return 1;
	}

	/* Locate PE file header  */
	/* sizeof(ULONG) = sizeof(MAGIC) */
	PEFileHeader = (PIMAGE_FILE_HEADER)((char *)FileData + PEDosHeader->e_lfanew + sizeof(ULONG));

	/* Locate optional header */
	PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
	ImageBase = PEOptHeader->ImageBase;

	/* Locate PE section headers  */
	PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *) PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

	/* make sure offset is what we want */
	offset = fixup_offset ( ImageBase, offset );

	/* find rossym section */
	PERosSymSectionHeader = find_rossym_section (
		PEFileHeader, PESectionHeaders );
	if ( !PERosSymSectionHeader )
	{
		fprintf ( stderr, "Couldn't find rossym section in executable\n" );
		return 1;
	}
	res = find_and_print_offset ( (char*)FileData + PERosSymSectionHeader->PointerToRawData,
		offset );
	if ( res )
		printf ( "??:0\n" );
	return res;
}

int
process_file ( const char* file_name, size_t offset )
{
	void* FileData;
	size_t FileSize;
	int res = 1;

	FileData = load_file ( file_name, &FileSize );
	if ( !FileData )
	{
		fprintf ( stderr, "An error occured loading '%s'\n", file_name );
	}
	else
	{
		res = process_data ( FileData, FileSize, offset );
		free ( FileData );
	}

	return res;
}

int main ( int argc, const char** argv )
{
	char* path;
	size_t offset;
	int res;

	if ( argc != 3 )
	{
		fprintf(stderr, "Usage: raddr2line <exefile> <offset>\n");
		exit(1);
	}

	path = convert_path ( argv[1] );
	offset = my_atoi ( argv[2] );

	res = process_file ( path, offset );

	free ( path );

	return res;
}
