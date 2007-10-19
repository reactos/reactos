/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS text-mode setup
 * FILE:        subsys/system/usetup/cabinet.c
 * PURPOSE:     Cabinet routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15/08-2003 Created
 */

#include "usetup.h"
#include <zlib.h>

#define NDEBUG
#include <debug.h>

#define SEEK_BEGIN    0
#define SEEK_CURRENT  1
#ifndef SEEK_END
#define SEEK_END      2
#endif

typedef struct __DOSTIME
{
  WORD Second:5;
  WORD Minute:6;
  WORD Hour:5;
} DOSTIME, *PDOSTIME;


typedef struct __DOSDATE
{
  WORD Day:5;
  WORD Month:4;
  WORD Year:5;
} DOSDATE, *PDOSDATE;

static WCHAR CabinetName[256];          // Filename of current cabinet
static WCHAR CabinetPrev[256];          // Filename of previous cabinet
static WCHAR DiskPrev[256];             // Label of cabinet in file CabinetPrev
static WCHAR CabinetNext[256];          // Filename of next cabinet
static WCHAR DiskNext[256];             // Label of cabinet in file CabinetNext
static ULONG FolderUncompSize = 0;      // Uncompressed size of folder
static ULONG BytesLeftInBlock = 0;      // Number of bytes left in current block
static WCHAR DestPath[MAX_PATH];
static HANDLE FileHandle;
static HANDLE FileSectionHandle;
static PUCHAR FileBuffer;
static SIZE_T DestFileSize;
static SIZE_T FileSize;
static BOOL FileOpen = FALSE;
static PCFHEADER PCABHeader;
static PCFFOLDER CabinetFolders;
static ULONG CabinetReserved = 0;
static ULONG FolderReserved = 0;
static ULONG DataReserved = 0;
static ULONG CodecId;
static PCABINET_CODEC_UNCOMPRESS CodecUncompress = NULL;
static BOOL CodecSelected = FALSE;
static ULONG LastFileOffset = 0;          // Uncompressed offset of last extracted file
static PCABINET_OVERWRITE OverwriteHandler = NULL;
static PCABINET_EXTRACT ExtractHandler = NULL;
static PCABINET_DISK_CHANGE DiskChangeHandler = NULL;
static z_stream ZStream;
static PVOID CabinetReservedArea = NULL;


/* Needed by zlib, but we don't want the dependency on msvcrt.dll */

/* round to 16 bytes + alloc at minimum 16 bytes */
#define ROUND_SIZE(size) (max(16, ROUND_UP(size, 16)))

void* __cdecl malloc(size_t _size)
{
   size_t nSize = ROUND_SIZE(_size);

   if (nSize<_size)
       return NULL;

   return RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, nSize);
}


void __cdecl free(void* _ptr)
{
  RtlFreeHeap(ProcessHeap, 0, _ptr);
}

void* __cdecl calloc(size_t _nmemb, size_t _size)
{
  return (void*)RtlAllocateHeap (ProcessHeap, HEAP_ZERO_MEMORY, _size);
}

/* RAW codec */

ULONG
RawCodecUncompress(PVOID OutputBuffer,
				   PVOID InputBuffer,
				   PLONG InputLength,
				   PLONG OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer before, and amount consumed after
 *                    Negative to indicate that this is not the start of a new block
 *     OutputLength = Length of output buffer before, amount filled after
 *                    Negative to indicate that this is not the end of the block
 */
{
  LONG In = abs(*InputLength), Out = abs(*OutputLength);
  memcpy(OutputBuffer, InputBuffer, In < Out ? In : Out);
  *InputLength = *OutputLength = In < Out ? In : Out;
  return CS_SUCCESS;
}


/* MSZIP codec */

ULONG
MSZipCodecUncompress(PVOID OutputBuffer,
					 PVOID InputBuffer,
					 PLONG InputLength,
					 PLONG OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer before, and amount consumed after
 *                    Negative to indicate that this is not the start of a new block
 *     OutputLength = Length of output buffer before, amount filled after
 *                    Negative to indicate that this is not the end of the block
 */
{
  USHORT Magic;
  INT Status;

  DPRINT("MSZipCodecUncompress( OutputBuffer = %x, InputBuffer = %x, InputLength = %d, OutputLength = %d.\n", OutputBuffer, InputBuffer, *InputLength, *OutputLength);
  if( *InputLength > 0 )
	{
	  Magic = *((PUSHORT)InputBuffer);

	  if (Magic != MSZIP_MAGIC)
		{
		  DPRINT("Bad MSZIP block header magic (0x%X)\n", Magic);
		  return CS_BADSTREAM;
		}

	  ZStream.next_in   = (PUCHAR)((ULONG)InputBuffer + 2);
	  ZStream.avail_in  = *InputLength - 2;
	  ZStream.next_out  = (PUCHAR)OutputBuffer;
	  ZStream.avail_out = abs(*OutputLength);

	  /* WindowBits is passed < 0 to tell that there is no zlib header.
	   * Note that in this case inflate *requires* an extra "dummy" byte
	   * after the compressed stream in order to complete decompression and
	   * return Z_STREAM_END.
	   */
	  Status = inflateInit2(&ZStream, -MAX_WBITS);
	  if (Status != Z_OK)
		{
		  DPRINT("inflateInit2() returned (%d).\n", Status);
		  return CS_BADSTREAM;
		}
	  ZStream.total_in = 2;
	}
  else {
	ZStream.avail_in = -*InputLength;
	ZStream.next_in = (PUCHAR)InputBuffer;
	ZStream.next_out = (PUCHAR)OutputBuffer;
	ZStream.avail_out = abs(*OutputLength);
	ZStream.total_in = 0;
  }
  ZStream.total_out = 0;
  Status = inflate(&ZStream, Z_SYNC_FLUSH );
  if (Status != Z_OK && Status != Z_STREAM_END)
	{
	  DPRINT("inflate() returned (%d) (%s).\n", Status, ZStream.msg);
	  if (Status == Z_MEM_ERROR)
		return CS_NOMEMORY;
	  return CS_BADSTREAM;
	}

  if( *OutputLength > 0 )
	{
	  Status = inflateEnd(&ZStream);
	  if (Status != Z_OK)
		{
		  DPRINT("inflateEnd() returned (%d).\n", Status);
		  return CS_BADSTREAM;
		}
	}
  *OutputLength = ZStream.total_out;
  *InputLength = ZStream.total_in;
  return CS_SUCCESS;
}



/* Memory functions */

voidpf MSZipAlloc(voidpf opaque, uInt items, uInt size)
{
  return (voidpf)RtlAllocateHeap (ProcessHeap, 0, items * size);
}

void MSZipFree (voidpf opaque, voidpf address)
{
  RtlFreeHeap(ProcessHeap, 0, address);
}

static BOOL
ConvertSystemTimeToFileTime(
  CONST SYSTEMTIME *  lpSystemTime,
  LPFILETIME  lpFileTime)
{
  TIME_FIELDS TimeFields;
  LARGE_INTEGER liTime;

  TimeFields.Year = lpSystemTime->wYear;
  TimeFields.Month = lpSystemTime->wMonth;
  TimeFields.Day = lpSystemTime->wDay;
  TimeFields.Hour = lpSystemTime->wHour;
  TimeFields.Minute = lpSystemTime->wMinute;
  TimeFields.Second = lpSystemTime->wSecond;
  TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

  if (RtlTimeFieldsToTime(&TimeFields, &liTime))
    {
      lpFileTime->dwLowDateTime = liTime.u.LowPart;
      lpFileTime->dwHighDateTime = liTime.u.HighPart;
      return TRUE;
    }
  return FALSE;
}


static BOOL
ConvertDosDateTimeToFileTime(
  WORD wFatDate,
  WORD wFatTime,
  LPFILETIME lpFileTime)
{
  PDOSTIME  pdtime = (PDOSTIME) &wFatTime;
  PDOSDATE  pddate = (PDOSDATE) &wFatDate;
  SYSTEMTIME SystemTime;

  if (lpFileTime == NULL)
    return FALSE;

  SystemTime.wMilliseconds = 0;
  SystemTime.wSecond = pdtime->Second;
  SystemTime.wMinute = pdtime->Minute;
  SystemTime.wHour = pdtime->Hour;

  SystemTime.wDay = pddate->Day;
  SystemTime.wMonth = pddate->Month;
  SystemTime.wYear = 1980 + pddate->Year;

  ConvertSystemTimeToFileTime(&SystemTime,lpFileTime);

  return TRUE;
}


static PWCHAR
GetFileName(PWCHAR Path)
/*
 * FUNCTION: Returns a pointer to file name
 * ARGUMENTS:
 *     Path = Pointer to string with pathname
 * RETURNS:
 *     Pointer to filename
 */
{
  ULONG i, j;

  j = i = 0;

  while (Path [i++])
    {
      if (Path[i - 1] == L'\\') j = i;
    }
  return Path + j;
}


static VOID
RemoveFileName(PWCHAR Path)
/*
 * FUNCTION: Removes a file name from a path
 * ARGUMENTS:
 *     Path = Pointer to string with path
 */
{
  PWCHAR FileName;
  DWORD i;

  i = 0;
  FileName = GetFileName(Path + i);

  if ((FileName != (Path + i)) && (FileName [-1] == L'\\'))
    FileName--;
  if ((FileName == (Path + i)) && (FileName [0] == L'\\'))
    FileName++;
  FileName[0] = 0;
}


static BOOL
SetAttributesOnFile(PCFFILE File, HANDLE hFile)
/*
 * FUNCTION: Sets attributes on a file
 * ARGUMENTS:
 *      File = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
  FILE_BASIC_INFORMATION FileBasic;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS NtStatus;
  ULONG Attributes = 0;

  if (File->Attributes & CAB_ATTRIB_READONLY)
    Attributes |= FILE_ATTRIBUTE_READONLY;

  if (File->Attributes & CAB_ATTRIB_HIDDEN)
    Attributes |= FILE_ATTRIBUTE_HIDDEN;

  if (File->Attributes & CAB_ATTRIB_SYSTEM)
    Attributes |= FILE_ATTRIBUTE_SYSTEM;

  if (File->Attributes & CAB_ATTRIB_DIRECTORY)
    Attributes |= FILE_ATTRIBUTE_DIRECTORY;

  if (File->Attributes & CAB_ATTRIB_ARCHIVE)
    Attributes |= FILE_ATTRIBUTE_ARCHIVE;

  NtStatus = NtQueryInformationFile(hFile,
    &IoStatusBlock,
    &FileBasic,
    sizeof(FILE_BASIC_INFORMATION),
    FileBasicInformation);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("NtQueryInformationFile() failed (%x).\n", NtStatus);
    }
  else
    {
      FileBasic.FileAttributes = Attributes;

      NtStatus = NtSetInformationFile(hFile,
        &IoStatusBlock,
        &FileBasic,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("NtSetInformationFile() failed (%x).\n", NtStatus);
        }
    }

  return NT_SUCCESS(NtStatus);
}

static ULONG
CloseCabinet(VOID)
/*
 * FUNCTION: Closes the current cabinet
 * RETURNS:
 *     Status of operation
 */
{
  if (FileBuffer)
    {
      NtUnmapViewOfSection( NtCurrentProcess(), FileBuffer );
	  NtClose( FileSectionHandle );
	  NtClose( FileHandle );
      FileBuffer = NULL;
    }

  return 0;
}


VOID
CabinetInitialize(VOID)
/*
 * FUNCTION: Initialize archiver
 */
{
  ZStream.zalloc = MSZipAlloc;
  ZStream.zfree  = MSZipFree;
  ZStream.opaque = (voidpf)0;

  FileOpen = FALSE;
  wcscpy(DestPath, L"");

  CodecId       = CAB_CODEC_RAW;
  CodecSelected = TRUE;

  FolderUncompSize = 0;
  BytesLeftInBlock = 0;
  CabinetReserved = 0;
  FolderReserved = 0;
  DataReserved = 0;
  CabinetReservedArea = NULL;
  LastFileOffset = 0;
}


VOID
CabinetCleanup(VOID)
/*
 * FUNCTION: Cleanup archiver
 */
{
  CabinetClose();
}


BOOL
CabinetNormalizePath(PWCHAR Path,
  ULONG Length)
/*
 * FUNCTION: Normalizes a path
 * ARGUMENTS:
 *     Path   = Pointer to string with pathname
 *     Length = Number of characters in Path
 * RETURNS:
 *     TRUE if there was enough room in Path, or FALSE
 */
{
  ULONG n;
  BOOL OK = TRUE;

  if ((n = wcslen(Path)) &&
    (Path[n - 1] != L'\\') &&
    (OK = ((n + 1) < Length)))
    {
      Path[n]     = L'\\';
      Path[n + 1] = 0;
    }
  return OK;
}


PWCHAR
CabinetGetCabinetName()
/*
 * FUNCTION: Returns pointer to cabinet file name
 * RETURNS:
 *     Pointer to string with name of cabinet
 */
{
  return CabinetName;
}


VOID
CabinetSetCabinetName(PWCHAR FileName)
/*
 * FUNCTION: Sets cabinet file name
 * ARGUMENTS:
 *     FileName = Pointer to string with name of cabinet
 */
{
  wcscpy(CabinetName, FileName);
}


VOID
CabinetSetDestinationPath(PWCHAR DestinationPath)
/*
 * FUNCTION: Sets destination path
 * ARGUMENTS:
 *    DestinationPath = Pointer to string with name of destination path
 */
{
  wcscpy(DestPath, DestinationPath);
  if (wcslen(DestPath) > 0)
    CabinetNormalizePath(DestPath, MAX_PATH);
}


PWCHAR
CabinetGetDestinationPath()
/*
 * FUNCTION: Returns destination path
 * RETURNS:
 *    Pointer to string with name of destination path
 */
{
  return DestPath;
}


ULONG
CabinetOpen(VOID)
/*
 * FUNCTION: Opens a cabinet file
 * RETURNS:
 *     Status of operation
 */
{
  PUCHAR Buffer;
  UNICODE_STRING ustring;
  ANSI_STRING astring;

  if (!FileOpen)
    {
      OBJECT_ATTRIBUTES ObjectAttributes;
      IO_STATUS_BLOCK IoStatusBlock;
      UNICODE_STRING FileName;
      NTSTATUS NtStatus;
      ULONG Size;

      RtlInitUnicodeString(&FileName,
        CabinetName);

      InitializeObjectAttributes(&ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

      NtStatus = NtOpenFile(&FileHandle,
							GENERIC_READ | SYNCHRONIZE,
							&ObjectAttributes,
							&IoStatusBlock,
							FILE_SHARE_READ,
							FILE_SYNCHRONOUS_IO_NONALERT);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("Cannot open file (%S) (%x).\n", CabinetName, NtStatus);
          return CAB_STATUS_CANNOT_OPEN;
        }
      FileOpen = TRUE;

	  NtStatus = NtCreateSection(&FileSectionHandle,
								 SECTION_ALL_ACCESS,
								 0,
								 0,
								 PAGE_READONLY,
								 SEC_COMMIT,
								 FileHandle);
      if(!NT_SUCCESS(NtStatus))
		{
		  DPRINT("NtCreateSection failed: %x\n", NtStatus);
		  return CAB_STATUS_NOMEMORY;
		}
	  FileBuffer = 0;
	  FileSize = 0;
	  NtStatus = NtMapViewOfSection(FileSectionHandle,
									NtCurrentProcess(),
									(PVOID *)&FileBuffer,
									0,
									0,
									0,
									&FileSize,
									ViewUnmap,
									0,
									PAGE_READONLY);
	  if(!NT_SUCCESS(NtStatus))
		{
		  DPRINT("NtMapViewOfSection failed: %x\n", NtStatus);
		  return CAB_STATUS_NOMEMORY;
		}
	  DPRINT( "Cabinet file %S opened and mapped to %x\n", CabinetName, FileBuffer );
	  PCABHeader = (PCFHEADER)FileBuffer;

      /* Check header */
      if(FileSize <= sizeof(CFHEADER) ||
		 PCABHeader->Signature != CAB_SIGNATURE ||
		 PCABHeader->Version != CAB_VERSION ||
		 PCABHeader->FolderCount == 0 ||
		 PCABHeader->FileCount == 0 ||
		 PCABHeader->FileTableOffset < sizeof(CFHEADER))
		{
		  CloseCabinet();
		  DPRINT("File has invalid header.\n");
		  return CAB_STATUS_INVALID_CAB;
		}

      Size = 0;
	  Buffer = (PUCHAR)(PCABHeader+1);
      /* Read/skip any reserved bytes */
      if (PCABHeader->Flags & CAB_FLAG_RESERVE)
        {
		  CabinetReserved = *(PUSHORT)Buffer;
		  Buffer += 2;
          FolderReserved = *Buffer;
		  Buffer++;
          DataReserved = *Buffer;
		  Buffer++;
          if (CabinetReserved > 0)
            {
			  CabinetReservedArea = Buffer;
			  Buffer += CabinetReserved;
            }
        }

      if (PCABHeader->Flags & CAB_FLAG_HASPREV)
        {
          /* The previous cabinet file is in the same directory as the current */
          wcscpy(CabinetPrev, CabinetName);
          RemoveFileName(CabinetPrev);
          CabinetNormalizePath(CabinetPrev, 256);
		  RtlInitAnsiString( &astring, (LPSTR)Buffer );
		  ustring.Length = wcslen( CabinetPrev );
		  ustring.Buffer = CabinetPrev + ustring.Length;
		  ustring.MaximumLength = sizeof( CabinetPrev ) - ustring.Length;
		  RtlAnsiStringToUnicodeString( &ustring, &astring, FALSE );
		  Buffer += astring.Length + 1;

          /* Read label of prev disk */
		  RtlInitAnsiString( &astring, (LPSTR)Buffer );
		  ustring.Length = 0;
		  ustring.Buffer = DiskPrev;
		  ustring.MaximumLength = sizeof( DiskPrev );
		  RtlAnsiStringToUnicodeString( &ustring, &astring, FALSE );
		  Buffer += astring.Length + 1;
        }
      else
        {
          wcscpy(CabinetPrev, L"");
          wcscpy(DiskPrev, L"");
        }

      if (PCABHeader->Flags & CAB_FLAG_HASNEXT)
        {
          /* The next cabinet file is in the same directory as the previous */
          wcscpy(CabinetNext, CabinetName);
          RemoveFileName(CabinetNext);
          CabinetNormalizePath(CabinetNext, 256);
		  RtlInitAnsiString( &astring, (LPSTR)Buffer );
		  ustring.Length = wcslen( CabinetNext );
		  ustring.Buffer = CabinetNext + ustring.Length;
		  ustring.MaximumLength = sizeof( CabinetNext ) - ustring.Length;
		  RtlAnsiStringToUnicodeString( &ustring, &astring, FALSE );
		  Buffer += astring.Length + 1;

          /* Read label of next disk */
		  RtlInitAnsiString( &astring, (LPSTR)Buffer );
		  ustring.Length = 0;
		  ustring.Buffer = DiskNext;
		  ustring.MaximumLength = sizeof( DiskNext );
		  RtlAnsiStringToUnicodeString( &ustring, &astring, FALSE );
		  Buffer += astring.Length + 1;
        }
      else
        {
          wcscpy(CabinetNext, L"");
          wcscpy(DiskNext,    L"");
        }
	  CabinetFolders = (PCFFOLDER)Buffer;
    }
  DPRINT( "CabinetOpen returning SUCCESS\n" );
  return CAB_STATUS_SUCCESS;
}


VOID
CabinetClose(VOID)
/*
 * FUNCTION: Closes the cabinet file
 */
{
  if (FileOpen)
    {
      CloseCabinet();

      FileOpen = FALSE;
    }
}


ULONG
CabinetFindFirst(PWCHAR FileName,
				 PCAB_SEARCH Search)
/*
 * FUNCTION: Finds the first file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     FileName = Pointer to search criteria
 *     Search   = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
{
  DPRINT( "CabinetFindFirst( FileName = %S )\n", FileName );
  wcsncpy(Search->Search, FileName, MAX_PATH);
  wcsncpy(Search->Cabinet, CabinetName, MAX_PATH);
  Search->File = 0;
  return CabinetFindNext(Search);
}


ULONG
CabinetFindNext(PCAB_SEARCH Search)
/*
 * FUNCTION: Finds next file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     Search = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
{
  ULONG Status;
  PCFFILE Prev;
  ANSI_STRING AnsiString;
  UNICODE_STRING UnicodeString;
  WCHAR FileName[MAX_PATH];

  if( wcscmp( Search->Cabinet, CabinetName ) != 0 )
	Search->File = 0;    // restart search of cabinet has changed since last find
  if( !Search->File )
	{
	  // starting new search or cabinet
	  Search->File = (PCFFILE)(FileBuffer + PCABHeader->FileTableOffset);
	  Search->Index = 0;
	  Prev = 0;
	}
  else Prev = Search->File;
  while(1)
	{
	  // look at each file in the archive and see if we found a match
	  if( Search->File->FolderIndex == 0xFFFD || Search->File->FolderIndex == 0xFFFF )
		{
		  // skip files continued from previous cab
		  DPRINT("Skipping file (%s)  FileOffset (0x%X)  LastFileOffset (0x%X).\n",
				 (char *)(Search->File + 1), Search->File->FileOffset, LastFileOffset);
		}
	  else {
		// FIXME: check for match against search criteria
		if( Search->File != Prev )
		  {
			// don't match the file we started with
			if( wcscmp( Search->Search, L"*" ) == 0 )
			  {
				// take any file
				break;
			  }
			else {
			  // otherwise, try to match the exact file name
			  RtlInitAnsiString( &AnsiString, Search->File->FileName );
			  UnicodeString.Buffer = FileName;
			  UnicodeString.Length = 0;
			  UnicodeString.MaximumLength = sizeof( FileName );
			  RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );
			  if( wcscmp( Search->Search, UnicodeString.Buffer ) == 0 )
				break;
			}
		  }
	  }
	  // if we make it here we found no match, so move to the next file
	  Search->Index++;
	  if( Search->Index >= PCABHeader->FileCount )
		{
		  // we have reached the end of this cabinet, try to open the next
		  DPRINT( "End of cabinet reached\n" );
		  if (wcslen(DiskNext) > 0)
			{
			  CloseCabinet();

			  CabinetSetCabinetName(CabinetNext);
			  wcscpy( Search->Cabinet, CabinetName );

			  if (DiskChangeHandler != NULL)
				{
				  DiskChangeHandler(CabinetNext, DiskNext);
				}

			  Status = CabinetOpen();
			  if (Status != CAB_STATUS_SUCCESS)
				return Status;

			}
		  else
			{
			  return CAB_STATUS_NOFILE;
			}
		  // starting new search or cabinet
		  Search->File = (PCFFILE)(FileBuffer + PCABHeader->FileTableOffset);
		  Search->Index = 0;
		  Prev = 0;
		}
	  else Search->File = (PCFFILE)(strchr( (char *)(Search->File + 1), 0 ) + 1);
	}
  DPRINT( "Found file %s\n", Search->File->FileName );
  return CAB_STATUS_SUCCESS;
}


int Validate()
{
  return (int)RtlValidateHeap(ProcessHeap, 0, 0);
}

ULONG CabinetExtractFile( PCAB_SEARCH Search )
/*
 * FUNCTION: Extracts a file from the cabinet
 * ARGUMENTS:
 *     Search = Pointer to PCAB_SEARCH structure used to locate the file
 * RETURNS
 *     Status of operation
 */
{
  ULONG Size;                // remaining file bytes to decompress
  ULONG CurrentOffset;       // current uncompressed offset within the folder
  PUCHAR CurrentBuffer;      // current pointer to compressed data in the block
  LONG RemainingBlock;       // remaining comp data in the block
  HANDLE DestFile;
  HANDLE DestFileSection;
  PVOID DestFileBuffer;      // mapped view of dest file
  PVOID CurrentDestBuffer;   // pointer to the current position in the dest view
  PCFDATA CFData;            // current data block
  ULONG Status;
  FILETIME FileTime;
  WCHAR DestName[MAX_PATH];
  NTSTATUS NtStatus;
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;
  IO_STATUS_BLOCK IoStatusBlock;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_BASIC_INFORMATION FileBasic;
  PCFFOLDER CurrentFolder;
  LARGE_INTEGER MaxDestFileSize;
  LONG InputLength, OutputLength;
  char Junk[512];

  if( wcscmp( Search->Cabinet, CabinetName ) != 0 )
	{
	  // the file is not in the current cabinet
	  DPRINT( "File is not in this cabinet ( %S != %S )\n", Search->Cabinet, CabinetName );
	  return CAB_STATUS_NOFILE;
	}
  // look up the folder that the file specifies
  if( Search->File->FolderIndex == 0xFFFD || Search->File->FolderIndex == 0xFFFF )
	{
	  // folder is continued from previous cabinet, that shouldn't happen here
	  return CAB_STATUS_NOFILE;
	}
  else if( Search->File->FolderIndex == 0xFFFE )
	{
	  // folder is the last in this cabinet and continues into next
	  CurrentFolder = &CabinetFolders[PCABHeader->FolderCount-1];
	}
  else {
	// folder is completely contained within this cabinet
	CurrentFolder = &CabinetFolders[Search->File->FolderIndex];
  }
  switch (CurrentFolder->CompressionType & CAB_COMP_MASK)
    {
	case CAB_COMP_NONE:
	  CabinetSelectCodec(CAB_CODEC_RAW);
	  break;
	case CAB_COMP_MSZIP:
	  CabinetSelectCodec(CAB_CODEC_MSZIP);
	  break;
	default:
	  return CAB_STATUS_UNSUPPCOMP;
    }

  DPRINT("Extracting file at uncompressed offset (0x%X)  Size (%d bytes)).\n",
		 (UINT)Search->File->FileOffset,
		 (UINT)Search->File->FileSize);
  RtlInitAnsiString( &AnsiString, Search->File->FileName );
  wcscpy( DestName, DestPath );
  UnicodeString.MaximumLength = sizeof( DestName ) - wcslen( DestName );
  UnicodeString.Buffer = DestName + wcslen( DestName );
  UnicodeString.Length = 0;
  RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, FALSE );

  /* Create destination file, fail if it already exists */
  RtlInitUnicodeString(&UnicodeString,
					   DestName);


  InitializeObjectAttributes(&ObjectAttributes,
							 &UnicodeString,
							 OBJ_CASE_INSENSITIVE,
							 NULL,
							 NULL);

  NtStatus = NtCreateFile(&DestFile,
						  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
						  &ObjectAttributes,
						  &IoStatusBlock,
						  NULL,
						  FILE_ATTRIBUTE_NORMAL,
						  0,
						  FILE_CREATE,
						  FILE_SYNCHRONOUS_IO_NONALERT,
						  NULL,
						  0);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("NtCreateFile() failed (%S) (%x).\n", DestName, NtStatus);

      /* If file exists, ask to overwrite file */
      if (OverwriteHandler == NULL || OverwriteHandler(Search->File, DestName))
        {
          /* Create destination file, overwrite if it already exists */
          NtStatus = NtCreateFile(&DestFile,
								  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
								  &ObjectAttributes,
								  &IoStatusBlock,
								  NULL,
								  FILE_ATTRIBUTE_NORMAL,
								  0,
								  FILE_OVERWRITE,
								  FILE_SYNCHRONOUS_IO_ALERT,
								  NULL,
								  0);
          if (!NT_SUCCESS(NtStatus))
            {
              DPRINT("NtCreateFile() failed 2 (%S) (%x).\n", DestName, NtStatus);
              return CAB_STATUS_CANNOT_CREATE;
            }
        }
      else
        {
          DPRINT("File (%S) exists.\n", DestName);
          return CAB_STATUS_FILE_EXISTS;
        }
    }
  MaxDestFileSize.QuadPart = Search->File->FileSize;
  NtStatus = NtCreateSection(&DestFileSection,
							 SECTION_ALL_ACCESS,
							 0,
							 &MaxDestFileSize,
							 PAGE_READWRITE,
							 SEC_COMMIT,
							 DestFile);
  if(!NT_SUCCESS(NtStatus))
	{
	  DPRINT("NtCreateSection failed: %x\n", NtStatus);
	  Status = CAB_STATUS_NOMEMORY;
	  goto CloseDestFile;
	}
  DestFileBuffer = 0;
  DestFileSize = 0;
  NtStatus = NtMapViewOfSection(DestFileSection,
								NtCurrentProcess(),
								&DestFileBuffer,
								0,
								0,
								0,
								&DestFileSize,
								ViewUnmap,
								0,
								PAGE_READWRITE);
  if(!NT_SUCCESS(NtStatus))
	{
	  DPRINT("NtMapViewOfSection failed: %x\n", NtStatus);
	  Status = CAB_STATUS_NOMEMORY;
	  goto CloseDestFileSection;
	}
  CurrentDestBuffer = DestFileBuffer;
  if (!ConvertDosDateTimeToFileTime(Search->File->FileDate, Search->File->FileTime, &FileTime))
    {
      DPRINT("DosDateTimeToFileTime() failed.\n");
      Status = CAB_STATUS_CANNOT_WRITE;
	  goto UnmapDestFile;
    }

  NtStatus = NtQueryInformationFile(DestFile,
									&IoStatusBlock,
									&FileBasic,
									sizeof(FILE_BASIC_INFORMATION),
									FileBasicInformation);
  if (!NT_SUCCESS(NtStatus))
    {
      DPRINT("NtQueryInformationFile() failed (%x).\n", NtStatus);
    }
  else
    {
      memcpy(&FileBasic.LastAccessTime, &FileTime, sizeof(FILETIME));

      NtStatus = NtSetInformationFile(DestFile,
									  &IoStatusBlock,
									  &FileBasic,
									  sizeof(FILE_BASIC_INFORMATION),
									  FileBasicInformation);
      if (!NT_SUCCESS(NtStatus))
        {
          DPRINT("NtSetInformationFile() failed (%x).\n", NtStatus);
        }
    }

  SetAttributesOnFile(Search->File, DestFile);

  /* Call extract event handler */
  if (ExtractHandler != NULL)
    {
      ExtractHandler(Search->File, DestName);
    }
  // find the starting block of the file
  // start with the first data block of the folder
  CFData = (PCFDATA)(CabinetFolders[Search->File->FolderIndex].DataOffset + FileBuffer);
  CurrentOffset = 0;
  while( CurrentOffset + CFData->UncompSize <= Search->File->FileOffset )
	{
	  // walk the data blocks until we reach the one containing the start of the file
	  CurrentOffset += CFData->UncompSize;
	  CFData = (PCFDATA)((char *)(CFData+1) + DataReserved + CFData->CompSize);
	}
  // now decompress and discard any data in the block before the start of the file
  CurrentBuffer = ((unsigned char *)(CFData+1)) + DataReserved; // start of comp data
  RemainingBlock = CFData->CompSize;
  InputLength = RemainingBlock;
  while( CurrentOffset < Search->File->FileOffset )
	{
	  // compute remaining uncomp bytes to start of file, bounded by sizeof junk
	  OutputLength = Search->File->FileOffset - CurrentOffset;
	  if( OutputLength > (LONG)sizeof( Junk ) )
		OutputLength = sizeof( Junk );
	  OutputLength = -OutputLength;     // negate to signal NOT end of block
	  CodecUncompress( Junk,
					   CurrentBuffer,
					   &InputLength,
					   &OutputLength );
	  CurrentOffset += OutputLength;   // add the uncomp bytes extracted to current folder offset
	  CurrentBuffer += InputLength;    // add comp bytes consumed to CurrentBuffer
	  RemainingBlock -= InputLength;   // subtract bytes consumed from bytes remaining in block
	  InputLength = -RemainingBlock;   // neg for resume decompression of the same block
	}
  // now CurrentBuffer points to the first comp byte of the file, so we can begin decompressing

  Size = Search->File->FileSize;   // Size = remaining uncomp bytes of the file to decompress
  while(Size > 0)
    {
	  OutputLength = Size;
	  DPRINT( "Decompressing block at %x with RemainingBlock = %d, Size = %d\n", CurrentBuffer, RemainingBlock, Size );
	  Status = CodecUncompress(CurrentDestBuffer,
							   CurrentBuffer,
							   &InputLength,
							   &OutputLength);
	  if (Status != CS_SUCCESS)
		{
		  DPRINT("Cannot uncompress block.\n");
		  if(Status == CS_NOMEMORY)
			Status = CAB_STATUS_NOMEMORY;
		  Status = CAB_STATUS_INVALID_CAB;
		  goto UnmapDestFile;
		}
	  CurrentDestBuffer = (PVOID)((ULONG_PTR)CurrentDestBuffer + OutputLength);  // advance dest buffer by bytes produced
	  CurrentBuffer += InputLength;       // advance src buffer by bytes consumed
	  Size -= OutputLength;               // reduce remaining file bytes by bytes produced
	  RemainingBlock -= InputLength;      // reduce remaining block size by bytes consumed
	  if( RemainingBlock == 0 )
		{
		  // used up this block, move on to the next
		  DPRINT( "Out of block data\n" );
		  CFData = (PCFDATA)CurrentBuffer;
		  RemainingBlock = CFData->CompSize;
		  CurrentBuffer = ((unsigned char *)(CFData+1) + DataReserved);
		  InputLength = RemainingBlock;
		}
	}
  Status = CAB_STATUS_SUCCESS;
 UnmapDestFile:
  NtUnmapViewOfSection(NtCurrentProcess(), DestFileBuffer);
 CloseDestFileSection:
  NtClose(DestFileSection);
 CloseDestFile:
  NtClose(DestFile);

  return Status;
}


VOID
CabinetSelectCodec(ULONG Id)
/*
 * FUNCTION: Selects codec engine to use
 * ARGUMENTS:
 *     Id = Codec identifier
 */
{
  if (CodecSelected)
    {
      if (Id == CodecId)
        return;

      CodecSelected = FALSE;
    }

  switch (Id)
    {
      case CAB_CODEC_RAW:
        CodecUncompress = RawCodecUncompress;
        break;
      case CAB_CODEC_MSZIP:
        CodecUncompress = MSZipCodecUncompress;
        break;
      default:
        return;
    }

  CodecId = Id;
  CodecSelected = TRUE;
}


VOID
CabinetSetEventHandlers(PCABINET_OVERWRITE Overwrite,
  PCABINET_EXTRACT Extract,
  PCABINET_DISK_CHANGE DiskChange)
/*
 * FUNCTION: Set event handlers
 * ARGUMENTS:
 *     Overwrite = Handler called when a file is to be overwritten
 *     Extract = Handler called when a file is to be extracted
 *     DiskChange = Handler called when changing the disk
 */
{
  OverwriteHandler = Overwrite;
  ExtractHandler = Extract;
  DiskChangeHandler = DiskChange;
}


PVOID
CabinetGetCabinetReservedArea(PULONG Size)
/*
 * FUNCTION: Get pointer to cabinet reserved area. NULL if none
 */
{
  if (CabinetReservedArea != NULL)
    {
      if (Size != NULL)
        {
          *Size = CabinetReserved;
        }
      return CabinetReservedArea;
    }
  else
    {
      if (Size != NULL)
        {
          *Size = 0;
        }
      return NULL;
    }
}
