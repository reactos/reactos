/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabinet.cpp
 * PURPOSE:     Cabinet routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Colin Finck <mail@colinfinck.de>
 * NOTES:       Define CAB_READ_ONLY for read only version
 * REVISIONS:
 *   CSH 21/03-2001 Created
 *   CSH 15/08-2003 Made it portable
 *   CF  04/05-2007 Reformatted the code to be more consistent and use TABs instead of spaces
 *   CF  04/05-2007 Made it compatible with 64-bit operating systems
 *   CF  18/08-2007 Use typedefs64.h and the Windows types for compatibility with 64-bit operating systems
 * TODO:
 *   - Checksum of datablocks should be calculated
 *   - EXTRACT.EXE complains if a disk is created manually
 *   - Folders that are created manually and span disks will result in a damaged cabinet
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
# include <sys/stat.h>
#endif // __FreeBSD__
#include "cabinet.h"
#include "raw.h"
#include "mszip.h"

#if defined(WIN32)
#define GetSizeOfFile(handle) _GetSizeOfFile(handle)
static LONG _GetSizeOfFile(FILEHANDLE handle)
{
	ULONG size = GetFileSize(handle, NULL);
	if (size == INVALID_FILE_SIZE)
		return -1;

	return size;
}
#define ReadFileData(handle, buffer, size, bytesread) _ReadFileData(handle, buffer, size, bytesread)
static bool _ReadFileData(FILEHANDLE handle, void* buffer, ULONG size, PULONG bytesread)
{
	return ReadFile(handle, buffer, size, (LPDWORD)bytesread, NULL) != 0;
}
#else
#define GetSizeOfFile(handle) _GetSizeOfFile(handle)
static LONG _GetSizeOfFile(FILEHANDLE handle)
{
	LONG size;
	fseek(handle, 0, SEEK_END);
	size = ftell(handle);
	fseek(handle, 0, SEEK_SET);
	return size;
}
#define ReadFileData(handle, buffer, size, bytesread) _ReadFileData(handle, buffer, size, bytesread)
static bool _ReadFileData(FILEHANDLE handle, void* buffer, ULONG size, PULONG bytesread)
{
	*bytesread = fread(buffer, 1, size, handle);
	return *bytesread == size;
}
#endif

#ifndef CAB_READ_ONLY

#if 0
#ifdef DBG

void DumpBuffer(void* Buffer, ULONG Size)
{
	FILEHANDLE FileHandle;
	ULONG BytesWritten;

	/* Create file, overwrite if it already exists */
	FileHandle = CreateFile("dump.bin", // Create this file
		GENERIC_WRITE,                  // Open for writing
		0,                              // No sharing
		NULL,                           // No security
		CREATE_ALWAYS,                  // Create or overwrite
		FILE_ATTRIBUTE_NORMAL,          // Normal file
		NULL);                          // No attribute template
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		DPRINT(MID_TRACE, ("ERROR OPENING '%d'.\n", (ULONG)GetLastError()));
		return;
	}

	if (!WriteFile(FileHandle, Buffer, Size, &BytesWritten, NULL))
	{
		DPRINT(MID_TRACE, ("ERROR WRITING '%d'.\n", (ULONG)GetLastError()));
	}

	CloseFile(FileHandle);
}

#endif /* DBG */
#endif

/* CCFDATAStorage */

CCFDATAStorage::CCFDATAStorage()
/*
 * FUNCTION: Default constructor
 */
{
	FileCreated = false;
}


CCFDATAStorage::~CCFDATAStorage()
/*
 * FUNCTION: Default destructor
 */
{
	ASSERT(!FileCreated);
}


ULONG CCFDATAStorage::Create(char* FileName)
/*
 * FUNCTION: Creates the file
 * ARGUMENTS:
 *     FileName = Pointer to name of file
 * RETURNS:
 *     Status of operation
 */
{
	ASSERT(!FileCreated);

#if defined(WIN32)
	if (GetTempPath(MAX_PATH, FullName) == 0)
		return CAB_STATUS_CANNOT_CREATE;

	strcat(FullName, FileName);

	/* Create file, overwrite if it already exists */
	FileHandle = CreateFile(FullName,   // Create this file
		GENERIC_READ | GENERIC_WRITE,   // Open for reading/writing
		0,                              // No sharing
		NULL,                           // No security
		CREATE_ALWAYS,                  // Create or overwrite
		FILE_FLAG_SEQUENTIAL_SCAN |     // Optimize for sequential scans
		FILE_FLAG_DELETE_ON_CLOSE |     // Delete file when closed
		FILE_ATTRIBUTE_TEMPORARY,       // Temporary file
		NULL);                          // No attribute template
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		DPRINT(MID_TRACE, ("ERROR '%d'.\n", (ULONG)GetLastError()));
		return CAB_STATUS_CANNOT_CREATE;
	}
#else /* !WIN32 */
	/*if (tmpnam(FullName) == NULL)*/
	if ((FileHandle = tmpfile()) == NULL)
		return CAB_STATUS_CANNOT_CREATE;
		/*
	FileHandle = fopen(FullName, "w+b");
	if (FileHandle == NULL) {
		DPRINT(MID_TRACE, ("ERROR '%d'.\n", (ULONG)errno));
		return CAB_STATUS_CANNOT_CREATE;
	}
		*/
#endif

	FileCreated = true;

	return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::Destroy()
/*
 * FUNCTION: Destroys the file
 * RETURNS:
 *     Status of operation
 */
{
	ASSERT(FileCreated);

	CloseFile(FileHandle);

	FileCreated = false;

	return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::Truncate()
/*
 * FUNCTION: Truncate the scratch file to zero bytes
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	if (!SetFilePointer(FileHandle, 0, NULL, FILE_BEGIN))
		return CAB_STATUS_FAILURE;
	if (!SetEndOfFile(FileHandle))
		return CAB_STATUS_FAILURE;
#else /* !WIN32 */
	fclose(FileHandle);
	FileHandle = tmpfile();
	if (FileHandle == NULL)
	{
		DPRINT(MID_TRACE, ("ERROR '%d'.\n", (ULONG)errno));
		return CAB_STATUS_FAILURE;
	}
#endif
	return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::Position()
/*
 * FUNCTION: Returns current position in file
 * RETURNS:
 *     Current position
 */
{
#if defined(WIN32)
	return SetFilePointer(FileHandle, 0, NULL, FILE_CURRENT);
#else
	return (ULONG)ftell(FileHandle);
#endif
}


ULONG CCFDATAStorage::Seek(LONG Position)
/*
 * FUNCTION: Seeks to an absolute position
 * ARGUMENTS:
 *     Position = Absolute position to seek to
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	if (SetFilePointer(FileHandle,
		Position,
		NULL,
		FILE_BEGIN) == 0xFFFFFFFF)
		return CAB_STATUS_FAILURE;
	else
		return CAB_STATUS_SUCCESS;
#else
	if (fseek(FileHandle, (off_t)Position, SEEK_SET) != 0)
		return CAB_STATUS_FAILURE;
	else
		return CAB_STATUS_SUCCESS;
#endif
}


ULONG CCFDATAStorage::ReadBlock(PCFDATA Data, void* Buffer, PULONG BytesRead)
/*
 * FUNCTION: Reads a CFDATA block from the file
 * ARGUMENTS:
 *     Data         = Pointer to CFDATA block for the buffer
 *     Buffer       = Pointer to buffer to store data read
 *     BytesWritten = Pointer to buffer to write number of bytes read
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	if (!ReadFile(FileHandle, Buffer, Data->CompSize, (LPDWORD)BytesRead, NULL))
		return CAB_STATUS_CANNOT_READ;
#else

	*BytesRead = fread(Buffer, 1, Data->CompSize, FileHandle);
	if (*BytesRead != Data->CompSize)
		return CAB_STATUS_CANNOT_READ;
#endif
	return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::WriteBlock(PCFDATA Data, void* Buffer, PULONG BytesWritten)
/*
 * FUNCTION: Writes a CFDATA block to the file
 * ARGUMENTS:
 *     Data         = Pointer to CFDATA block for the buffer
 *     Buffer       = Pointer to buffer with data to write
 *     BytesWritten = Pointer to buffer to write number of bytes written
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	if (!WriteFile(FileHandle, Buffer, Data->CompSize, (LPDWORD)BytesWritten, NULL))
		return CAB_STATUS_CANNOT_WRITE;
#else
	*BytesWritten = fwrite(Buffer, 1, Data->CompSize, FileHandle);
	if (*BytesWritten != Data->CompSize)
		return CAB_STATUS_CANNOT_WRITE;
#endif
	return CAB_STATUS_SUCCESS;
}

#endif /* CAB_READ_ONLY */


/* CCabinet */

CCabinet::CCabinet()
/*
 * FUNCTION: Default constructor
 */
{
	*CabinetName = '\0';
	*CabinetPrev = '\0';
	*DiskPrev = '\0';
	*CabinetNext = '\0';
	*DiskNext = '\0';
	*DestPath = '\0';
	*CabinetReservedFile = '\0';

	FileOpen = false;
	CabinetReservedFileBuffer = NULL;
	CabinetReservedFileSize = 0;

	FolderListHead = NULL;
	FolderListTail = NULL;
	FileListHead   = NULL;
	FileListTail   = NULL;

	Codec         = new CRawCodec();
	CodecId       = CAB_CODEC_RAW;
	CodecSelected = true;

	OutputBuffer = NULL;
	InputBuffer  = NULL;
	MaxDiskSize  = 0;
	BlockIsSplit = false;
	ScratchFile  = NULL;

	FolderUncompSize = 0;
	BytesLeftInBlock = 0;
	ReuseBlock       = false;
	CurrentDataNode  = NULL;
}


CCabinet::~CCabinet()
/*
 * FUNCTION: Default destructor
 */
{
	if (CabinetReservedFileBuffer != NULL)
	{
		FreeMemory(CabinetReservedFileBuffer);
		CabinetReservedFileBuffer = NULL;
		CabinetReservedFileSize = 0;
	}

	if (CodecSelected)
		delete Codec;
}

bool CCabinet::IsSeparator(char Char)
/*
 * FUNCTION: Determines if a character is a separator
 * ARGUMENTS:
 *     Char = Character to check
 * RETURNS:
 *     Wether it is a separator
 */
{
	if ((Char == '\\') || (Char == '/'))
		return true;
	else
		return false;
}

char* CCabinet::ConvertPath(char* Path, bool Allocate)
/*
 * FUNCTION: Replaces \ or / with the one used be the host environment
 * ARGUMENTS:
 *     Path     = Pointer to string with pathname
 *     Allocate = Specifies wther to allocate memory for the new
 *                string or to change the existing buffer
 * RETURNS:
 *     Pointer to new path
 */
{
	char *newpath;
	int i;

	if (Allocate)
		newpath = strdup(Path);
	else
		newpath = Path;

	i = 0;
	while (Path[i] != 0)
	{
#if defined(WIN32)
		if (Path[i] == '/')
			newpath[i] = '\\';
		else
#else
		if (Path[i] == '\\')
			newpath[i] = '/';
		else
#endif
			newpath[i] = Path[i];

		i++;
	}
	newpath[i] = 0;

	return(newpath);
}


char* CCabinet::GetFileName(char* Path)
/*
 * FUNCTION: Returns a pointer to file name
 * ARGUMENTS:
 *     Path = Pointer to string with pathname
 * RETURNS:
 *     Pointer to filename
 */
{
	ULONG i, j;

	j = i = (Path[0] ? (Path[1] == ':' ? 2 : 0) : 0);

	while (Path [i++])
		if (IsSeparator(Path [i - 1]))
			j = i;

	return Path + j;
}


void CCabinet::RemoveFileName(char* Path)
/*
 * FUNCTION: Removes a file name from a path
 * ARGUMENTS:
 *     Path = Pointer to string with path
 */
{
	char* FileName;
	ULONG i;

	i = (Path [0] ? (Path[1] == ':' ? 2 : 0) : 0);
	FileName = GetFileName(Path + i);

	if ((FileName != (Path + i)) && (IsSeparator(FileName [-1])))
		FileName--;
	if ((FileName == (Path + i)) && (IsSeparator(FileName [0])))
		FileName++;
	FileName[0] = 0;
}


bool CCabinet::NormalizePath(char* Path,
                             ULONG Length)
/*
 * FUNCTION: Normalizes a path
 * ARGUMENTS:
 *     Path   = Pointer to string with pathname
 *     Length = Number of bytes in Path
 * RETURNS:
 *     true if there was enough room in Path, or false
 */
{
	ULONG n;
	bool OK = true;

	if ((n = (ULONG)strlen(Path)) &&
		(!IsSeparator(Path[n - 1])) &&
		(OK = ((n + 1) < Length)))
	{
		Path[n]     = DIR_SEPARATOR_CHAR;
		Path[n + 1] = 0;
	}
	return OK;
}


char* CCabinet::GetCabinetName()
/*
 * FUNCTION: Returns pointer to cabinet file name
 * RETURNS:
 *     Pointer to string with name of cabinet
 */
{
	return CabinetName;
}


void CCabinet::SetCabinetName(char* FileName)
/*
 * FUNCTION: Sets cabinet file name
 * ARGUMENTS:
 *     FileName = Pointer to string with name of cabinet
 */
{
	strcpy(CabinetName, FileName);
}


void CCabinet::SetDestinationPath(char* DestinationPath)
/*
 * FUNCTION: Sets destination path
 * ARGUMENTS:
 *    DestinationPath = Pointer to string with name of destination path
 */
{
	strcpy(DestPath, DestinationPath);
	ConvertPath(DestPath, false);
	if (strlen(DestPath) > 0)
		NormalizePath(DestPath, MAX_PATH);
}


char* CCabinet::GetDestinationPath()
/*
 * FUNCTION: Returns destination path
 * RETURNS:
 *    Pointer to string with name of destination path
 */
{
	return DestPath;
}


bool CCabinet::SetCabinetReservedFile(char* FileName)
/*
 * FUNCTION: Sets cabinet reserved file
 * ARGUMENTS:
 *    FileName = Pointer to string with name of cabinet reserved file
 */
{
	FILEHANDLE FileHandle;
	ULONG BytesRead;

#if defined(WIN32)
	FileHandle = CreateFile(ConvertPath(FileName, true),  // Open this file
		GENERIC_READ,                    // Open for reading
		FILE_SHARE_READ,                 // Share for reading
		NULL,                            // No security
		OPEN_EXISTING,                   // Existing file only
		FILE_ATTRIBUTE_NORMAL,           // Normal file
		NULL);                           // No attribute template
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		DPRINT(MID_TRACE, ("Cannot open cabinet reserved file.\n"));
		return false;
	}
#else /* !WIN32 */
	FileHandle = fopen(ConvertPath(FileName, true), "rb");
	if (FileHandle == NULL)
	{
		DPRINT(MID_TRACE, ("Cannot open cabinet reserved file.\n"));
		return false;
	}
#endif

	CabinetReservedFileSize = GetSizeOfFile(FileHandle);
	if (CabinetReservedFileSize == (ULONG)-1)
	{
		DPRINT(MIN_TRACE, ("Cannot read from cabinet reserved file.\n"));
		return false;
	}

	if (CabinetReservedFileSize == 0)
	{
		CloseFile(FileHandle);
		return false;
	}

	CabinetReservedFileBuffer = AllocateMemory(CabinetReservedFileSize);
	if (!CabinetReservedFileBuffer)
	{
		CloseFile(FileHandle);
		return false;
	}

	if (!ReadFileData(FileHandle, CabinetReservedFileBuffer, CabinetReservedFileSize, &BytesRead))
	{
		CloseFile(FileHandle);
		return false;
	}

	CloseFile(FileHandle);

	strcpy(CabinetReservedFile, FileName);

	return true;
}


char* CCabinet::GetCabinetReservedFile()
/*
 * FUNCTION: Returns cabionet reserved file
 * RETURNS:
 *    Pointer to string with name of cabinet reserved file
 */
{
	return CabinetReservedFile;
}


ULONG CCabinet::GetCurrentDiskNumber()
/*
 * FUNCTION: Returns current disk number
 * RETURNS:
 *     Current disk number
 */
{
	return CurrentDiskNumber;
}


ULONG CCabinet::Open()
/*
 * FUNCTION: Opens a cabinet file
 * RETURNS:
 *     Status of operation
 */
{
	PCFFOLDER_NODE FolderNode;
	ULONG Status;
	ULONG Index;

	if (!FileOpen)
	{
		ULONG BytesRead;
		ULONG Size;

		OutputBuffer = AllocateMemory(CAB_BLOCKSIZE + 12);    // This should be enough
		if (!OutputBuffer)
			return CAB_STATUS_NOMEMORY;

#if defined(WIN32)
		FileHandle = CreateFile(CabinetName, // Open this file
			GENERIC_READ,                    // Open for reading
			FILE_SHARE_READ,                 // Share for reading
			NULL,                            // No security
			OPEN_EXISTING,                   // Existing file only
			FILE_ATTRIBUTE_NORMAL,           // Normal file
			NULL);                           // No attribute template

		if (FileHandle == INVALID_HANDLE_VALUE)
		{
			DPRINT(MID_TRACE, ("Cannot open file.\n"));
			return CAB_STATUS_CANNOT_OPEN;
		}
#else /* !WIN32 */
		FileHandle = fopen(CabinetName, "rb");
		if (FileHandle == NULL)
		{
			DPRINT(MID_TRACE, ("Cannot open file.\n"));
			return CAB_STATUS_CANNOT_OPEN;
		}
#endif

		FileOpen = true;

		/* Load CAB header */
		if ((Status = ReadBlock(&CABHeader, sizeof(CFHEADER), &BytesRead))
			!= CAB_STATUS_SUCCESS)
		{
			DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
			return CAB_STATUS_INVALID_CAB;
		}

		/* Check header */
		if ((BytesRead                 != sizeof(CFHEADER)) ||
			(CABHeader.Signature       != CAB_SIGNATURE   ) ||
			(CABHeader.Version         != CAB_VERSION     ) ||
			(CABHeader.FolderCount     == 0               ) ||
			(CABHeader.FileCount       == 0               ) ||
			(CABHeader.FileTableOffset < sizeof(CFHEADER)))
		{
			CloseCabinet();
			DPRINT(MID_TRACE, ("File has invalid header.\n"));
			return CAB_STATUS_INVALID_CAB;
		}

		Size = 0;

		/* Read/skip any reserved bytes */
		if (CABHeader.Flags & CAB_FLAG_RESERVE)
		{
			if ((Status = ReadBlock(&Size, sizeof(ULONG), &BytesRead))
				!= CAB_STATUS_SUCCESS)
			{
				DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
				return CAB_STATUS_INVALID_CAB;
			}
			CabinetReserved = Size & 0xFFFF;
			FolderReserved  = (Size >> 16) & 0xFF;
			DataReserved    = (Size >> 24) & 0xFF;

#if defined(WIN32)
			SetFilePointer(FileHandle, CabinetReserved, NULL, FILE_CURRENT);
			if (GetLastError() != NO_ERROR)
			{
				DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
				return CAB_STATUS_FAILURE;
			}
#else
			if (fseek(FileHandle, (off_t)CabinetReserved, SEEK_CUR) != 0)
			{
				DPRINT(MIN_TRACE, ("fseek() failed.\n"));
				return CAB_STATUS_FAILURE;
			}
#endif
		}

		if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0)
		{
			/* Read name of previous cabinet */
			Status = ReadString(CabinetPrev, 256);
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
			/* Read label of previous disk */
			Status = ReadString(DiskPrev, 256);
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
		}
		else
		{
			strcpy(CabinetPrev, "");
			strcpy(DiskPrev,    "");
		}

		if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0)
		{
			/* Read name of next cabinet */
			Status = ReadString(CabinetNext, 256);
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
			/* Read label of next disk */
			Status = ReadString(DiskNext, 256);
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
		}
		else
		{
			strcpy(CabinetNext, "");
			strcpy(DiskNext,    "");
		}

		/* Read all folders */
		for (Index = 0; Index < CABHeader.FolderCount; Index++)
		{
			FolderNode = NewFolderNode();
			if (!FolderNode)
			{
				DPRINT(MIN_TRACE, ("Insufficient resources.\n"));
				return CAB_STATUS_NOMEMORY;
			}

			if (Index == 0)
				FolderNode->UncompOffset = FolderUncompSize;

			FolderNode->Index = Index;

			if ((Status = ReadBlock(&FolderNode->Folder,
				sizeof(CFFOLDER), &BytesRead)) != CAB_STATUS_SUCCESS)
			{
				DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
				return CAB_STATUS_INVALID_CAB;
			}
		}

		/* Read file entries */
		Status = ReadFileTable();
		if (Status != CAB_STATUS_SUCCESS)
		{
			DPRINT(MIN_TRACE, ("ReadFileTable() failed (%d).\n", (ULONG)Status));
			return Status;
		}

		/* Read data blocks for all folders */
		FolderNode = FolderListHead;
		while (FolderNode != NULL)
		{
			Status = ReadDataBlocks(FolderNode);
			if (Status != CAB_STATUS_SUCCESS)
			{
				DPRINT(MIN_TRACE, ("ReadDataBlocks() failed (%d).\n", (ULONG)Status));
				return Status;
			}
			FolderNode = FolderNode->Next;
		}
	}
	return CAB_STATUS_SUCCESS;
}


void CCabinet::Close()
/*
 * FUNCTION: Closes the cabinet file
 */
{
	if (FileOpen)
	{
		CloseFile(FileHandle);
		FileOpen = false;
	}
}


ULONG CCabinet::FindFirst(char* FileName,
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
	RestartSearch = false;
	strncpy(Search->Search, FileName, MAX_PATH);
	Search->Next = FileListHead;
	return FindNext(Search);
}


ULONG CCabinet::FindNext(PCAB_SEARCH Search)
/*
 * FUNCTION: Finds next file in the cabinet that matches a search criteria
 * ARGUMENTS:
 *     Search = Pointer to search structure
 * RETURNS:
 *     Status of operation
 */
{
	ULONG Status;

	if (RestartSearch)
	{
		Search->Next  = FileListHead;

		/* Skip split files already extracted */
		while ((Search->Next) &&
			(Search->Next->File.FileControlID > CAB_FILE_MAX_FOLDER) &&
			(Search->Next->File.FileOffset <= LastFileOffset))
		{
			DPRINT(MAX_TRACE, ("Skipping file (%s)  FileOffset (0x%lX)  LastFileOffset (0x%lX).\n",
				Search->Next->FileName, Search->Next->File.FileOffset, LastFileOffset));
			Search->Next = Search->Next->Next;
		}

		RestartSearch = false;
	}

	/* FIXME: Check search criteria */

	if (!Search->Next)
	{
		if (strlen(DiskNext) > 0)
		{
			CloseCabinet();

			SetCabinetName(CabinetNext);

			OnDiskChange(CabinetNext, DiskNext);

			Status = Open();
			if (Status != CAB_STATUS_SUCCESS)
				return Status;

			Search->Next = FileListHead;
			if (!Search->Next)
				return CAB_STATUS_NOFILE;
		}
		else
			return CAB_STATUS_NOFILE;
	}

	Search->File     = &Search->Next->File;
	Search->FileName = Search->Next->FileName;
	Search->Next     = Search->Next->Next;
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::ExtractFile(char* FileName)
/*
 * FUNCTION: Extracts a file from the cabinet
 * ARGUMENTS:
 *     FileName = Pointer to buffer with name of file
 * RETURNS
 *     Status of operation
 */
{
	ULONG Size;
	ULONG Offset;
	ULONG BytesRead;
	ULONG BytesToRead;
	ULONG BytesWritten;
	ULONG BytesSkipped;
	ULONG BytesToWrite;
	ULONG TotalBytesRead;
	ULONG CurrentOffset;
	unsigned char* Buffer;
	unsigned char* CurrentBuffer;
	FILEHANDLE DestFile;
	PCFFILE_NODE File;
	CFDATA CFData;
	ULONG Status;
	bool Skip;
#if defined(WIN32)
	FILETIME FileTime;
#endif
	char DestName[MAX_PATH];
	char TempName[MAX_PATH];

	Status = LocateFile(FileName, &File);
	if (Status != CAB_STATUS_SUCCESS)
	{
		DPRINT(MID_TRACE, ("Cannot locate file (%d).\n", (ULONG)Status));
		return Status;
	}

	LastFileOffset = File->File.FileOffset;

	switch (CurrentFolderNode->Folder.CompressionType & CAB_COMP_MASK)
	{
		case CAB_COMP_NONE:
			SelectCodec(CAB_CODEC_RAW);
			break;

		case CAB_COMP_MSZIP:
			SelectCodec(CAB_CODEC_MSZIP);
			break;

		default:
			return CAB_STATUS_UNSUPPCOMP;
	}

	DPRINT(MAX_TRACE, ("Extracting file at uncompressed offset (0x%X)  Size (%d bytes)  AO (0x%X)  UO (0x%X).\n",
		(ULONG)File->File.FileOffset,
		(ULONG)File->File.FileSize,
		(ULONG)File->DataBlock->AbsoluteOffset,
		(ULONG)File->DataBlock->UncompOffset));

	strcpy(DestName, DestPath);
	strcat(DestName, FileName);

	/* Create destination file, fail if it already exists */
#if defined(WIN32)
	DestFile = CreateFile(DestName,      // Create this file
		GENERIC_WRITE,                   // Open for writing
		0,                               // No sharing
		NULL,                            // No security
		CREATE_NEW,                      // New file only
		FILE_ATTRIBUTE_NORMAL,           // Normal file
		NULL);                           // No attribute template
	if (DestFile == INVALID_HANDLE_VALUE)
	{
		/* If file exists, ask to overwrite file */
		if (((Status = GetLastError()) == ERROR_FILE_EXISTS) &&
			(OnOverwrite(&File->File, FileName)))
		{
			/* Create destination file, overwrite if it already exists */
			DestFile = CreateFile(DestName, // Create this file
				GENERIC_WRITE,              // Open for writing
				0,                          // No sharing
				NULL,                       // No security
				TRUNCATE_EXISTING,          // Truncate the file
				FILE_ATTRIBUTE_NORMAL,      // Normal file
				NULL);                      // No attribute template
			if (DestFile == INVALID_HANDLE_VALUE)
				return CAB_STATUS_CANNOT_CREATE;
		}
		else
		{
			if (Status == ERROR_FILE_EXISTS)
				return CAB_STATUS_FILE_EXISTS;
			else
				return CAB_STATUS_CANNOT_CREATE;
		}
	}
#else /* !WIN32 */
	DestFile = fopen(DestName, "rb");
	if (DestFile != NULL)
	{
		fclose(DestFile);
		/* If file exists, ask to overwrite file */
		if (OnOverwrite(&File->File, FileName))
		{
			DestFile = fopen(DestName, "w+b");
			if (DestFile == NULL)
				return CAB_STATUS_CANNOT_CREATE;
		}
		else
			return CAB_STATUS_FILE_EXISTS;
	}
	else
	{
		DestFile = fopen(DestName, "w+b");
		if (DestFile == NULL)
			return CAB_STATUS_CANNOT_CREATE;
	}
#endif
#if defined(WIN32)
	if (!DosDateTimeToFileTime(File->File.FileDate, File->File.FileTime, &FileTime))
	{
		CloseFile(DestFile);
		DPRINT(MIN_TRACE, ("DosDateTimeToFileTime() failed (%lu).\n", GetLastError()));
		return CAB_STATUS_CANNOT_WRITE;
	}

	SetFileTime(DestFile, NULL, &FileTime, NULL);
#else
	//DPRINT(MIN_TRACE, ("FIXME: DosDateTimeToFileTime\n"));
#endif
	SetAttributesOnFile(File);

	Buffer = (unsigned char*)AllocateMemory(CAB_BLOCKSIZE + 12); // This should be enough
	if (!Buffer)
	{
		CloseFile(DestFile);
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}

	/* Call OnExtract event handler */
	OnExtract(&File->File, FileName);

	/* Search to start of file */
#if defined(WIN32)
	Offset = SetFilePointer(FileHandle,
		File->DataBlock->AbsoluteOffset,
		NULL,
		FILE_BEGIN);
	if (GetLastError() != NO_ERROR)
	{
		DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
		return CAB_STATUS_INVALID_CAB;
	}
#else
	if (fseek(FileHandle, (off_t)File->DataBlock->AbsoluteOffset, SEEK_SET) != 0)
	{
		DPRINT(MIN_TRACE, ("fseek() failed.\n"));
		return CAB_STATUS_FAILURE;
	}
	Offset = ftell(FileHandle);
#endif

	Size   = File->File.FileSize;
	Offset = File->File.FileOffset;
	CurrentOffset = File->DataBlock->UncompOffset;

	Skip = true;

	ReuseBlock = (CurrentDataNode == File->DataBlock);
	if (Size > 0)
	{
		do
		{
			DPRINT(MAX_TRACE, ("CO (0x%lX)    ReuseBlock (%d)    Offset (0x%lX)   Size (%ld)  BytesLeftInBlock (%ld)\n",
				File->DataBlock->UncompOffset, (ULONG)ReuseBlock, Offset, Size,
				BytesLeftInBlock));

			if (/*(CurrentDataNode != File->DataBlock) &&*/ (!ReuseBlock) || (BytesLeftInBlock <= 0))
			{
				DPRINT(MAX_TRACE, ("Filling buffer. ReuseBlock (%d)\n", (ULONG)ReuseBlock));

				CurrentBuffer  = Buffer;
				TotalBytesRead = 0;
				do
				{
					DPRINT(MAX_TRACE, ("Size (%lu bytes).\n", Size));

					if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) !=
						CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA)))
					{
						CloseFile(DestFile);
						FreeMemory(Buffer);
						DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
						return CAB_STATUS_INVALID_CAB;
					}

					DPRINT(MAX_TRACE, ("Data block: Checksum (0x%X)  CompSize (%d bytes)  UncompSize (%d bytes)\n",
						(ULONG)CFData.Checksum,
						(ULONG)CFData.CompSize,
						(ULONG)CFData.UncompSize));

					ASSERT(CFData.CompSize <= CAB_BLOCKSIZE + 12);

					BytesToRead = CFData.CompSize;

					DPRINT(MAX_TRACE, ("Read: (0x%lX,0x%lX).\n",
						(_W64 unsigned long)CurrentBuffer, (_W64 unsigned long)Buffer));

					if (((Status = ReadBlock(CurrentBuffer, BytesToRead, &BytesRead)) !=
						CAB_STATUS_SUCCESS) || (BytesToRead != BytesRead))
					{
						CloseFile(DestFile);
						FreeMemory(Buffer);
						DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
						return CAB_STATUS_INVALID_CAB;
					}

					/* FIXME: Does not work with files generated by makecab.exe */
/*
					if (CFData.Checksum != 0)
					{
						ULONG Checksum = ComputeChecksum(CurrentBuffer, BytesRead, 0);
						if (Checksum != CFData.Checksum)
						{
							CloseFile(DestFile);
							FreeMemory(Buffer);
							DPRINT(MIN_TRACE, ("Bad checksum (is 0x%X, should be 0x%X).\n",
								Checksum, CFData.Checksum));
							return CAB_STATUS_INVALID_CAB;
						}
					}
*/
					TotalBytesRead += BytesRead;

					CurrentBuffer += BytesRead;

					if (CFData.UncompSize == 0)
					{
						if (strlen(DiskNext) == 0)
							return CAB_STATUS_NOFILE;

						/* CloseCabinet() will destroy all file entries so in case
						   FileName refers to the FileName field of a CFFOLDER_NODE
						   structure, we have to save a copy of the filename */
						strcpy(TempName, FileName);

						CloseCabinet();

						SetCabinetName(CabinetNext);

						OnDiskChange(CabinetNext, DiskNext);

						Status = Open();
						if (Status != CAB_STATUS_SUCCESS)
							return Status;

						/* The first data block of the file will not be
						   found as it is located in the previous file */
						Status = LocateFile(TempName, &File);
						if (Status == CAB_STATUS_NOFILE)
						{
							DPRINT(MID_TRACE, ("Cannot locate file (%d).\n", (ULONG)Status));
							return Status;
						}

						/* The file is continued in the first data block in the folder */
						File->DataBlock = CurrentFolderNode->DataListHead;

						/* Search to start of file */
#if defined(WIN32)
						SetFilePointer(FileHandle,
							File->DataBlock->AbsoluteOffset,
							NULL,
							FILE_BEGIN);
						if (GetLastError() != NO_ERROR)
						{
							DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
							return CAB_STATUS_INVALID_CAB;
						}
#else
						if (fseek(FileHandle, (off_t)File->DataBlock->AbsoluteOffset, SEEK_SET) != 0)
						{
							DPRINT(MIN_TRACE, ("fseek() failed.\n"));
							return CAB_STATUS_INVALID_CAB;
						}
#endif

						DPRINT(MAX_TRACE, ("Continuing extraction of file at uncompressed offset (0x%X)  Size (%d bytes)  AO (0x%X)  UO (0x%X).\n",
							(ULONG)File->File.FileOffset,
							(ULONG)File->File.FileSize,
							(ULONG)File->DataBlock->AbsoluteOffset,
							(ULONG)File->DataBlock->UncompOffset));

						CurrentDataNode = File->DataBlock;
						ReuseBlock = true;

						RestartSearch = true;
					}
				} while (CFData.UncompSize == 0);

				DPRINT(MAX_TRACE, ("TotalBytesRead (%lu).\n", TotalBytesRead));

				Status = Codec->Uncompress(OutputBuffer, Buffer, TotalBytesRead, &BytesToWrite);
				if (Status != CS_SUCCESS)
				{
					CloseFile(DestFile);
					FreeMemory(Buffer);
					DPRINT(MID_TRACE, ("Cannot uncompress block.\n"));
					if (Status == CS_NOMEMORY)
						return CAB_STATUS_NOMEMORY;
					return CAB_STATUS_INVALID_CAB;
				}

				if (BytesToWrite != CFData.UncompSize)
				{
					DPRINT(MID_TRACE, ("BytesToWrite (%lu) != CFData.UncompSize (%d)\n",
						BytesToWrite, CFData.UncompSize));
					return CAB_STATUS_INVALID_CAB;
				}

				BytesLeftInBlock = BytesToWrite;
			}
			else
			{
				DPRINT(MAX_TRACE, ("Using same buffer. ReuseBlock (%d)\n", (ULONG)ReuseBlock));

				BytesToWrite = BytesLeftInBlock;

				DPRINT(MAX_TRACE, ("Seeking to absolute offset 0x%lX.\n",
					CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
					CurrentDataNode->Data.CompSize));

				if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) !=
					CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA)))
				{
					CloseFile(DestFile);
					FreeMemory(Buffer);
					DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
					return CAB_STATUS_INVALID_CAB;
				}

				DPRINT(MAX_TRACE, ("CFData.CompSize 0x%X  CFData.UncompSize 0x%X.\n",
					CFData.CompSize, CFData.UncompSize));

				/* Go to next data block */
#if defined(WIN32)
				SetFilePointer(FileHandle,
					CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
					CurrentDataNode->Data.CompSize,
					NULL,
					FILE_BEGIN);
				if (GetLastError() != NO_ERROR)
				{
					DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
					return CAB_STATUS_INVALID_CAB;
				}
#else
				if (fseek(FileHandle, (off_t)CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
					CurrentDataNode->Data.CompSize, SEEK_SET) != 0)
				{
					DPRINT(MIN_TRACE, ("fseek() failed.\n"));
					return CAB_STATUS_INVALID_CAB;
				}
#endif

				ReuseBlock = false;
			}

			if (Skip)
				BytesSkipped = (Offset - CurrentOffset);
			else
				BytesSkipped = 0;

			BytesToWrite -= BytesSkipped;

			if (Size < BytesToWrite)
				BytesToWrite = Size;

			DPRINT(MAX_TRACE, ("Offset (0x%X)  CurrentOffset (0x%X)  ToWrite (%d)  Skipped (%d)(%d)  Size (%d).\n",
				(ULONG)Offset,
				(ULONG)CurrentOffset,
				(ULONG)BytesToWrite,
				(ULONG)BytesSkipped, (ULONG)Skip,
				(ULONG)Size));

#if defined(WIN32)
			if (!WriteFile(DestFile, (void*)((unsigned long *)OutputBuffer + BytesSkipped),
				BytesToWrite, (LPDWORD)&BytesWritten, NULL) ||
				(BytesToWrite != BytesWritten))
			{
						DPRINT(MIN_TRACE, ("Status 0x%lX.\n", GetLastError()));
#else
			BytesWritten = BytesToWrite;
			if (fwrite((void*)((unsigned long *)OutputBuffer + BytesSkipped),
				BytesToWrite, 1, DestFile) < 1)
			{
#endif
				CloseFile(DestFile);
				FreeMemory(Buffer);
				DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
				return CAB_STATUS_CANNOT_WRITE;
			}
			Size -= BytesToWrite;

			CurrentOffset += BytesToWrite;

			/* Don't skip any more bytes */
			Skip = false;
		} while (Size > 0);
	}

	CloseFile(DestFile);

	FreeMemory(Buffer);

	return CAB_STATUS_SUCCESS;
}


void CCabinet::SelectCodec(ULONG Id)
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

		CodecSelected = false;
		delete Codec;
	}

	switch (Id)
	{
		case CAB_CODEC_RAW:
#if 0
			Codec = new CRawCodec();
#endif
			break;

		case CAB_CODEC_MSZIP:
			Codec = new CMSZipCodec();
			break;

		default:
			return;
	}

	CodecId       = Id;
	CodecSelected = true;
}


#ifndef CAB_READ_ONLY

/* CAB write methods */

ULONG CCabinet::NewCabinet()
/*
 * FUNCTION: Creates a new cabinet
 * RETURNS:
 *     Status of operation
 */
{
	ULONG Status;

	CurrentDiskNumber = 0;

	OutputBuffer = AllocateMemory(CAB_BLOCKSIZE + 12); // This should be enough
	InputBuffer  = AllocateMemory(CAB_BLOCKSIZE + 12); // This should be enough
	if ((!OutputBuffer) || (!InputBuffer))
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}
	CurrentIBuffer     = InputBuffer;
	CurrentIBufferSize = 0;

	CABHeader.Signature     = CAB_SIGNATURE;
	CABHeader.Reserved1     = 0;            // Not used
	CABHeader.CabinetSize   = 0;            // Not yet known
	CABHeader.Reserved2     = 0;            // Not used
	CABHeader.Reserved3     = 0;            // Not used
	CABHeader.Version       = CAB_VERSION;
	CABHeader.FolderCount   = 0;            // Not yet known
	CABHeader.FileCount     = 0;            // Not yet known
	CABHeader.Flags         = 0;            // Not yet known
	// FIXME: Should be random
	CABHeader.SetID         = 0x534F;
	CABHeader.CabinetNumber = 0;


	TotalFolderSize = 0;
	TotalFileSize   = 0;

	DiskSize = sizeof(CFHEADER);

	InitCabinetHeader();

	// NextFolderNumber is 0-based
	NextFolderNumber = 0;

	CurrentFolderNode = NULL;
	Status = NewFolder();
	if (Status != CAB_STATUS_SUCCESS)
		return Status;

	CurrentFolderNode->Folder.DataOffset = DiskSize - TotalHeaderSize;

	ScratchFile = new CCFDATAStorage;
	if (!ScratchFile)
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}

	Status = ScratchFile->Create("~CAB.tmp");

	CreateNewFolder = false;

	CreateNewDisk = false;

	PrevCabinetNumber = 0;

	return Status;
}


ULONG CCabinet::NewDisk()
/*
 * FUNCTION: Forces a new disk to be created
 * RETURNS:
 *     Status of operation
 */
{
	// NextFolderNumber is 0-based
	NextFolderNumber = 1;

	CreateNewDisk = false;

	DiskSize = sizeof(CFHEADER) + TotalFolderSize + TotalFileSize;

	InitCabinetHeader();

	CurrentFolderNode->TotalFolderSize = 0;

	CurrentFolderNode->Folder.DataBlockCount = 0;

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::NewFolder()
/*
 * FUNCTION: Forces a new folder to be created
 * RETURNS:
 *     Status of operation
 */
{
	DPRINT(MAX_TRACE, ("Creating new folder.\n"));

	CurrentFolderNode = NewFolderNode();
	if (!CurrentFolderNode)
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}

	switch (CodecId) {
		case CAB_CODEC_RAW:
			CurrentFolderNode->Folder.CompressionType = CAB_COMP_NONE;
			break;

		case CAB_CODEC_MSZIP:
			CurrentFolderNode->Folder.CompressionType = CAB_COMP_MSZIP;
			break;

		default:
			return CAB_STATUS_UNSUPPCOMP;
	}

	/* FIXME: This won't work if no files are added to the new folder */

	DiskSize += sizeof(CFFOLDER);

	TotalFolderSize += sizeof(CFFOLDER);

	NextFolderNumber++;

	CABHeader.FolderCount++;

	LastBlockStart = 0;

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteFileToScratchStorage(PCFFILE_NODE FileNode)
/*
 * FUNCTION: Writes a file to the scratch file
 * ARGUMENTS:
 *     FileNode = Pointer to file node
 * RETURNS:
 *     Status of operation
 */
{
	ULONG BytesToRead;
	ULONG BytesRead;
	ULONG Status;
	ULONG Size;

	if (!ContinueFile)
	{
		/* Try to open file */
#if defined(WIN32)
		SourceFile = CreateFile(
			FileNode->FileName,      // Open this file
			GENERIC_READ,            // Open for reading
			FILE_SHARE_READ,         // Share for reading
			NULL,                    // No security
			OPEN_EXISTING,           // File must exist
			FILE_ATTRIBUTE_NORMAL,   // Normal file
			NULL);                   // No attribute template
		if (SourceFile == INVALID_HANDLE_VALUE)
		{
			DPRINT(MID_TRACE, ("File not found (%s).\n", FileNode->FileName));
			return CAB_STATUS_NOFILE;
		}
#else /* !WIN32 */
		SourceFile = fopen(FileNode->FileName, "rb");
		if (SourceFile == NULL)
		{
			DPRINT(MID_TRACE, ("Cannot open cabinet reserved file.\n"));
			return CAB_STATUS_NOFILE;
		}
#endif

		if (CreateNewFolder)
		{
			/* There is always a new folder after
			   a split file is completely stored */
			Status = NewFolder();
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
			CreateNewFolder = false;
		}

		/* Call OnAdd event handler */
		OnAdd(&FileNode->File, FileNode->FileName);

		TotalBytesLeft = FileNode->File.FileSize;

		FileNode->File.FileOffset        = CurrentFolderNode->UncompOffset;
		CurrentFolderNode->UncompOffset += TotalBytesLeft;
		FileNode->File.FileControlID     = (USHORT)(NextFolderNumber - 1);
		CurrentFolderNode->Commit        = true;
		PrevCabinetNumber				 = CurrentDiskNumber;

		Size = sizeof(CFFILE) + (ULONG)strlen(GetFileName(FileNode->FileName)) + 1;
		CABHeader.FileTableOffset += Size;
		TotalFileSize += Size;
		DiskSize += Size;
	}

	FileNode->Commit = true;

	if (TotalBytesLeft > 0)
	{
		do
		{
			if (TotalBytesLeft > (ULONG)CAB_BLOCKSIZE - CurrentIBufferSize)
				BytesToRead = CAB_BLOCKSIZE - CurrentIBufferSize;
			else
				BytesToRead = TotalBytesLeft;

			if (!ReadFileData(SourceFile, CurrentIBuffer, BytesToRead, &BytesRead) || (BytesToRead != BytesRead))
			{
				DPRINT(MIN_TRACE, ("Cannot read from file. BytesToRead (%lu)  BytesRead (%lu)  CurrentIBufferSize (%lu).\n",
					BytesToRead, BytesRead, CurrentIBufferSize));
				return CAB_STATUS_INVALID_CAB;
			}

			*(unsigned char**)&CurrentIBuffer += BytesRead;

			CurrentIBufferSize += (USHORT)BytesRead;

			if (CurrentIBufferSize == CAB_BLOCKSIZE)
			{
				Status = WriteDataBlock();
				if (Status != CAB_STATUS_SUCCESS)
					return Status;
			}
			TotalBytesLeft -= BytesRead;
		} while ((TotalBytesLeft > 0) && (!CreateNewDisk));
	}

	if (TotalBytesLeft == 0)
	{
		CloseFile(SourceFile);
		FileNode->Delete = true;

		if (FileNode->File.FileControlID > CAB_FILE_MAX_FOLDER)
		{
			FileNode->File.FileControlID = CAB_FILE_CONTINUED;
			CurrentFolderNode->Delete = true;

			if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0))
			{
				Status = WriteDataBlock();
				if (Status != CAB_STATUS_SUCCESS)
					return Status;
			}

			CreateNewFolder = true;
		}
	}
	else
	{
		if (FileNode->File.FileControlID <= CAB_FILE_MAX_FOLDER)
			FileNode->File.FileControlID = CAB_FILE_SPLIT;
		else
			FileNode->File.FileControlID = CAB_FILE_PREV_NEXT;
	}

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteDisk(ULONG MoreDisks)
/*
 * FUNCTION: Forces the current disk to be written
 * ARGUMENTS:
 *     MoreDisks = true if there is one or more disks after this disk
 * RETURNS:
 *     Status of operation
 */
{
	PCFFILE_NODE FileNode;
	ULONG Status;

	ContinueFile = false;
	FileNode = FileListHead;
	while (FileNode != NULL)
	{
		Status = WriteFileToScratchStorage(FileNode);
		if (Status != CAB_STATUS_SUCCESS)
			return Status;

		if (CreateNewDisk)
		{
			/* A data block could span more than two
			   disks if MaxDiskSize is very small */
			while (CreateNewDisk)
			{
				DPRINT(MAX_TRACE, ("Creating new disk.\n"));
				CommitDisk(true);
				CloseDisk();
				NewDisk();

				ContinueFile = true;
				CreateNewDisk = false;

				DPRINT(MAX_TRACE, ("First on new disk. CurrentIBufferSize (%lu)  CurrentOBufferSize (%lu).\n",
					CurrentIBufferSize, CurrentOBufferSize));

				if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0))
				{
					Status = WriteDataBlock();
					if (Status != CAB_STATUS_SUCCESS)
						return Status;
				}
			}
		}
		else
		{
			ContinueFile = false;
			FileNode = FileNode->Next;
		}
	}

	if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0))
	{
		/* A data block could span more than two
		   disks if MaxDiskSize is very small */

		ASSERT(CreateNewDisk == false);

		do
		{
			if (CreateNewDisk)
			{
				DPRINT(MID_TRACE, ("Creating new disk 2.\n"));
				CommitDisk(true);
				CloseDisk();
				NewDisk();
				CreateNewDisk = false;

				ASSERT(FileNode == FileListHead);
			}

			if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0))
			{
				Status = WriteDataBlock();
				if (Status != CAB_STATUS_SUCCESS)
					return Status;
			}
		} while (CreateNewDisk);
	}
	CommitDisk(MoreDisks);

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::CommitDisk(ULONG MoreDisks)
/*
 * FUNCTION: Commits the current disk
 * ARGUMENTS:
 *     MoreDisks = true if there is one or more disks after this disk
 * RETURNS:
 *     Status of operation
 */
{
	PCFFOLDER_NODE FolderNode;
	ULONG Status;

	OnCabinetName(CurrentDiskNumber, CabinetName);

	/* Create file, fail if it already exists */
#if defined(WIN32)
	FileHandle = CreateFile(CabinetName, // Create this file
		GENERIC_WRITE,                   // Open for writing
		0,                               // No sharing
		NULL,                            // No security
		CREATE_NEW,                      // New file only
		FILE_ATTRIBUTE_NORMAL,           // Normal file
		NULL);                           // No attribute template
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		ULONG Status;
		/* If file exists, ask to overwrite file */
		if (((Status = GetLastError()) == ERROR_FILE_EXISTS) &&
			(OnOverwrite(NULL, CabinetName)))
		{

			/* Create cabinet file, overwrite if it already exists */
			FileHandle = CreateFile(CabinetName, // Create this file
				GENERIC_WRITE,                   // Open for writing
				0,                               // No sharing
				NULL,                            // No security
				TRUNCATE_EXISTING,               // Truncate the file
				FILE_ATTRIBUTE_NORMAL,           // Normal file
				NULL);                           // No attribute template
			if (FileHandle == INVALID_HANDLE_VALUE)
				return CAB_STATUS_CANNOT_CREATE;
		}
		else
		{
			if (Status == ERROR_FILE_EXISTS)
				return CAB_STATUS_FILE_EXISTS;
			else
				return CAB_STATUS_CANNOT_CREATE;
		}
	}
#else /* !WIN32 */
	FileHandle = fopen(CabinetName, "rb");
	if (FileHandle != NULL)
	{
		fclose(FileHandle);
		/* If file exists, ask to overwrite file */
		if (OnOverwrite(NULL, CabinetName))
		{
			FileHandle = fopen(CabinetName, "w+b");
			if (FileHandle == NULL)
				return CAB_STATUS_CANNOT_CREATE;
		}
		else
			return CAB_STATUS_FILE_EXISTS;

	}
	else
	{
		FileHandle = fopen(CabinetName, "w+b");
		if (FileHandle == NULL)
			return CAB_STATUS_CANNOT_CREATE;
	}
#endif

	WriteCabinetHeader(MoreDisks != 0);

	Status = WriteFolderEntries();
	if (Status != CAB_STATUS_SUCCESS)
		return Status;

	/* Write file entries */
	WriteFileEntries();

	/* Write data blocks */
	FolderNode = FolderListHead;
	while (FolderNode != NULL)
	{
		if (FolderNode->Commit)
		{
			Status = CommitDataBlocks(FolderNode);
			if (Status != CAB_STATUS_SUCCESS)
				return Status;
			/* Remove data blocks for folder */
			DestroyDataNodes(FolderNode);
		}
		FolderNode = FolderNode->Next;
	}

	CloseFile(FileHandle);

	ScratchFile->Truncate();

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::CloseDisk()
/*
 * FUNCTION: Closes the current disk
 * RETURNS:
 *     Status of operation
 */
{
	DestroyDeletedFileNodes();

	/* Destroy folder nodes that are completely stored */
	DestroyDeletedFolderNodes();

	CurrentDiskNumber++;

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::CloseCabinet()
/*
 * FUNCTION: Closes the current cabinet
 * RETURNS:
 *     Status of operation
 */
{
	ULONG Status;

	DestroyFileNodes();

	DestroyFolderNodes();

	if (InputBuffer)
	{
		FreeMemory(InputBuffer);
		InputBuffer = NULL;
	}

	if (OutputBuffer)
	{
		FreeMemory(OutputBuffer);
		OutputBuffer = NULL;
	}

	Close();

	if (ScratchFile)
	{
		Status = ScratchFile->Destroy();
		delete ScratchFile;
		return Status;
	}

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::AddFile(char* FileName)
/*
 * FUNCTION: Adds a file to the current disk
 * ARGUMENTS:
 *     FileName = Pointer to string with file name (full path)
 * RETURNS:
 *     Status of operation
 */
{
	FILEHANDLE SrcFile;
	PCFFILE_NODE FileNode;
	char* NewFileName;

	NewFileName = (char*)AllocateMemory(strlen(FileName) + 1);
	if (!NewFileName)
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}
	strcpy(NewFileName, FileName);
	ConvertPath(NewFileName, false);

	/* Try to open file */
#if defined(WIN32)
	SrcFile = CreateFile(
		NewFileName,             // Open this file
		GENERIC_READ,            // Open for reading
		FILE_SHARE_READ,         // Share for reading
		NULL,                    // No security
		OPEN_EXISTING,           // File must exist
		FILE_ATTRIBUTE_NORMAL,   // Normal file
		NULL);                   // No attribute template
	if (SrcFile == INVALID_HANDLE_VALUE)
	{
		DPRINT(MID_TRACE, ("File not found (%s).\n", NewFileName));
		FreeMemory(NewFileName);
		return CAB_STATUS_CANNOT_OPEN;
	}
#else /* !WIN32 */
	SrcFile = fopen(NewFileName, "rb");
	if (SrcFile == NULL)
	{
		DPRINT(MID_TRACE, ("File not found (%s).\n", NewFileName));
		FreeMemory(NewFileName);
		return CAB_STATUS_CANNOT_OPEN;
	}
#endif

	FileNode = NewFileNode();
	if (!FileNode)
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		FreeMemory(NewFileName);
		return CAB_STATUS_NOMEMORY;
	}

	FileNode->FolderNode = CurrentFolderNode;
	FileNode->FileName = NewFileName;

	/* FIXME: Check for and handle large files (>= 2GB) */
	FileNode->File.FileSize = GetSizeOfFile(SrcFile);
	if (FileNode->File.FileSize == (ULONG)-1)
	{
		DPRINT(MIN_TRACE, ("Cannot read from file.\n"));
		FreeMemory(NewFileName);
		return CAB_STATUS_CANNOT_READ;
	}

	GetFileTimes(SrcFile, FileNode);

	GetAttributesOnFile(FileNode);

	CloseFile(SrcFile);

	return CAB_STATUS_SUCCESS;
}


void CCabinet::SetMaxDiskSize(ULONG Size)
/*
 * FUNCTION: Sets the maximum size of the current disk
 * ARGUMENTS:
 *     Size = Maximum size of current disk (0 means no maximum size)
 */
{
	MaxDiskSize = Size;
}

#endif /* CAB_READ_ONLY */


/* Default event handlers */

bool CCabinet::OnOverwrite(PCFFILE File,
                           char* FileName)
/*
 * FUNCTION: Called when extracting a file and it already exists
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 * RETURNS
 *     true if the file should be overwritten, false if not
 */
{
	return false;
}


void CCabinet::OnExtract(PCFFILE File,
                         char* FileName)
/*
 * FUNCTION: Called just before extracting a file
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
}


void CCabinet::OnDiskChange(char* CabinetName,
                            char* DiskLabel)
/*
 * FUNCTION: Called when a new disk is to be processed
 * ARGUMENTS:
 *     CabinetName = Pointer to buffer with name of cabinet
 *     DiskLabel   = Pointer to buffer with label of disk
 */
{
}


#ifndef CAB_READ_ONLY

void CCabinet::OnAdd(PCFFILE File,
                     char* FileName)
/*
 * FUNCTION: Called just before adding a file to a cabinet
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being added
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
}


bool CCabinet::OnDiskLabel(ULONG Number, char* Label)
/*
 * FUNCTION: Called when a disk needs a label
 * ARGUMENTS:
 *     Number = Cabinet number that needs a label
 *     Label  = Pointer to buffer to place label of disk
 * RETURNS:
 *     true if a disk label was returned, false if not
 */
{
	return false;
}


bool CCabinet::OnCabinetName(ULONG Number, char* Name)
/*
 * FUNCTION: Called when a cabinet needs a name
 * ARGUMENTS:
 *     Number = Disk number that needs a name
 *     Name   = Pointer to buffer to place name of cabinet
 * RETURNS:
 *     true if a cabinet name was returned, false if not
 */
{
	return false;
}

#endif /* CAB_READ_ONLY */

PCFFOLDER_NODE CCabinet::LocateFolderNode(ULONG Index)
/*
 * FUNCTION: Locates a folder node
 * ARGUMENTS:
 *     Index = Folder index
 * RETURNS:
 *     Pointer to folder node or NULL if the folder node was not found
 */
{
	PCFFOLDER_NODE Node;

	switch (Index)
	{
		case CAB_FILE_SPLIT:
			return FolderListTail;

		case CAB_FILE_CONTINUED:
		case CAB_FILE_PREV_NEXT:
			return FolderListHead;
	}

	Node = FolderListHead;
	while (Node != NULL)
	{
		if (Node->Index == Index)
			return Node;
		Node = Node->Next;
	}
	return NULL;
}


ULONG CCabinet::GetAbsoluteOffset(PCFFILE_NODE File)
/*
 * FUNCTION: Returns the absolute offset of a file
 * ARGUMENTS:
 *     File = Pointer to CFFILE_NODE structure for file
 * RETURNS:
 *     Status of operation
 */
{
	PCFDATA_NODE Node;

	DPRINT(MAX_TRACE, ("FileName '%s'  FileOffset (0x%X)  FileSize (%d).\n",
		(char*)File->FileName,
		(ULONG)File->File.FileOffset,
		(ULONG)File->File.FileSize));

	Node = CurrentFolderNode->DataListHead;
	while (Node != NULL)
	{
		DPRINT(MAX_TRACE, ("GetAbsoluteOffset(): Comparing (0x%X, 0x%X) (%d).\n",
			(ULONG)Node->UncompOffset,
			(ULONG)Node->UncompOffset + Node->Data.UncompSize,
			(ULONG)Node->Data.UncompSize));

		/* Node->Data.UncompSize will be 0 if the block is split
		   (ie. it is the last block in this cabinet) */
		if ((Node->Data.UncompSize == 0) ||
			((File->File.FileOffset >= Node->UncompOffset) &&
			(File->File.FileOffset < Node->UncompOffset +
			Node->Data.UncompSize)))
		{
				File->DataBlock = Node;
				return CAB_STATUS_SUCCESS;
		}

		Node = Node->Next;
	}
	return CAB_STATUS_INVALID_CAB;
}


ULONG CCabinet::LocateFile(char* FileName,
                              PCFFILE_NODE *File)
/*
 * FUNCTION: Locates a file in the cabinet
 * ARGUMENTS:
 *     FileName = Pointer to string with name of file to locate
 *     File     = Address of pointer to CFFILE_NODE structure to fill
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Current folder is set to the folder of the file
 */
{
	PCFFILE_NODE Node;
	ULONG Status;

	DPRINT(MAX_TRACE, ("FileName '%s'\n", FileName));

	Node = FileListHead;
	while (Node != NULL)
	{
		if (strcasecmp(FileName, Node->FileName) == 0)
		{
			CurrentFolderNode = LocateFolderNode(Node->File.FileControlID);
			if (!CurrentFolderNode)
			{
				DPRINT(MID_TRACE, ("Folder with index number (%d) not found.\n",
					(ULONG)Node->File.FileControlID));
				return CAB_STATUS_INVALID_CAB;
			}

			if (Node->DataBlock == NULL)
				Status = GetAbsoluteOffset(Node);
			else
				Status = CAB_STATUS_SUCCESS;

			*File = Node;
			return Status;
		}
		Node = Node->Next;
	}
	return CAB_STATUS_NOFILE;
}


ULONG CCabinet::ReadString(char* String, ULONG MaxLength)
/*
 * FUNCTION: Reads a NULL-terminated string from the cabinet
 * ARGUMENTS:
 *     String    = Pointer to buffer to place string
 *     MaxLength = Maximum length of string
 * RETURNS:
 *     Status of operation
 */
{
	ULONG BytesRead;
	ULONG Offset;
	ULONG Status;
	ULONG Size;
	bool Found;

	Offset = 0;
	Found  = false;
	do
	{
		Size = ((Offset + 32) <= MaxLength)? 32 : MaxLength - Offset;

		if (Size == 0)
		{
			DPRINT(MIN_TRACE, ("Too long a filename.\n"));
			return CAB_STATUS_INVALID_CAB;
		}

		Status = ReadBlock(&String[Offset], Size, &BytesRead);
		if ((Status != CAB_STATUS_SUCCESS) || (BytesRead != Size))
		{
			DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
			return CAB_STATUS_INVALID_CAB;
		}

		for (Size = Offset; Size < Offset + BytesRead; Size++)
		{
			if (String[Size] == '\0')
			{
				Found = true;
				break;
			}
		}

		Offset += BytesRead;

	} while (!Found);

	/* Back up some bytes */
	Size = (BytesRead - Size) - 1;
#if defined(WIN32)
	SetLastError(NO_ERROR);
	(ULONG)SetFilePointer(FileHandle,
		-(LONG)Size,
		NULL,
		FILE_CURRENT);
	if (GetLastError() != NO_ERROR)
	{
		DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
		return CAB_STATUS_INVALID_CAB;
	}
#else
	if (fseek(FileHandle, (off_t)(-(LONG)Size), SEEK_CUR) != 0)
	{
		DPRINT(MIN_TRACE, ("fseek() failed.\n"));
		return CAB_STATUS_INVALID_CAB;
	}
#endif
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::ReadFileTable()
/*
 * FUNCTION: Reads the file table from the cabinet file
 * RETURNS:
 *     Status of operation
 */
{
	ULONG i;
	ULONG Status;
	ULONG BytesRead;
	PCFFILE_NODE File;

	DPRINT(MAX_TRACE, ("Reading file table at absolute offset (0x%lX).\n",
		CABHeader.FileTableOffset));

	/* Seek to file table */
#if defined(WIN32)
	SetLastError(NO_ERROR);
	SetFilePointer(FileHandle,
		CABHeader.FileTableOffset,
		NULL,
		FILE_BEGIN);
	if (GetLastError() != NO_ERROR)
	{
		DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
		DPRINT(MIN_TRACE, ("Error: %lu\n", GetLastError()));
		return CAB_STATUS_INVALID_CAB;
	}
#else
	if (fseek(FileHandle, (off_t)CABHeader.FileTableOffset, SEEK_SET) != 0)
	{
		DPRINT(MIN_TRACE, ("fseek() failed.\n"));
		return CAB_STATUS_INVALID_CAB;
	}
#endif

	for (i = 0; i < CABHeader.FileCount; i++)
	{
		File = NewFileNode();
		if (!File)
		{
			DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
			return CAB_STATUS_NOMEMORY;
		}

		if ((Status = ReadBlock(&File->File, sizeof(CFFILE),
			&BytesRead)) != CAB_STATUS_SUCCESS)
		{
			DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
			return CAB_STATUS_INVALID_CAB;
		}

		File->FileName = (char*)AllocateMemory(MAX_PATH);
		if (!File->FileName)
		{
			DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
			return CAB_STATUS_NOMEMORY;
		}

		/* Read file name */
		Status = ReadString(File->FileName, MAX_PATH);
		if (Status != CAB_STATUS_SUCCESS)
			return Status;

		DPRINT(MAX_TRACE, ("Found file '%s' at uncompressed offset (0x%X).  Size (%d bytes)  ControlId (0x%X).\n",
			(char*)File->FileName,
			(ULONG)File->File.FileOffset,
			(ULONG)File->File.FileSize,
			(ULONG)File->File.FileControlID));

	}
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::ReadDataBlocks(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Reads all CFDATA blocks for a folder from the cabinet file
 * ARGUMENTS:
 *     FolderNode = Pointer to CFFOLDER_NODE structure for folder
 * RETURNS:
 *     Status of operation
 */
{
	ULONG AbsoluteOffset;
	ULONG UncompOffset;
	PCFDATA_NODE Node;
	ULONG BytesRead;
	ULONG Status;
	ULONG i;

	DPRINT(MAX_TRACE, ("Reading data blocks for folder (%lu)  at absolute offset (0x%lX).\n",
		FolderNode->Index, FolderNode->Folder.DataOffset));

	AbsoluteOffset = FolderNode->Folder.DataOffset;
	UncompOffset   = FolderNode->UncompOffset;

	for (i = 0; i < FolderNode->Folder.DataBlockCount; i++)
	{
		Node = NewDataNode(FolderNode);
		if (!Node)
		{
			DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
			return CAB_STATUS_NOMEMORY;
		}

		/* Seek to data block */
#if defined(WIN32)
		SetLastError(NO_ERROR);
		SetFilePointer(FileHandle,
			AbsoluteOffset,
			NULL,
			FILE_BEGIN);
		if (GetLastError() != NO_ERROR)
		{
			DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
			return CAB_STATUS_INVALID_CAB;
		}
#else
		if (fseek(FileHandle, (off_t)AbsoluteOffset, SEEK_SET) != 0)
		{
			DPRINT(MIN_TRACE, ("fseek() failed.\n"));
			return CAB_STATUS_INVALID_CAB;
		}
#endif

		if ((Status = ReadBlock(&Node->Data, sizeof(CFDATA),
			&BytesRead)) != CAB_STATUS_SUCCESS)
		{
			DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (ULONG)Status));
			return CAB_STATUS_INVALID_CAB;
		}

		DPRINT(MAX_TRACE, ("AbsOffset (0x%X)  UncompOffset (0x%X)  Checksum (0x%X)  CompSize (%d)  UncompSize (%d).\n",
			(ULONG)AbsoluteOffset,
			(ULONG)UncompOffset,
			(ULONG)Node->Data.Checksum,
			(ULONG)Node->Data.CompSize,
			(ULONG)Node->Data.UncompSize));

		Node->AbsoluteOffset = AbsoluteOffset;
		Node->UncompOffset   = UncompOffset;

		AbsoluteOffset += sizeof(CFDATA) + Node->Data.CompSize;
		UncompOffset   += Node->Data.UncompSize;
	}

	FolderUncompSize = UncompOffset;

	return CAB_STATUS_SUCCESS;
}


PCFFOLDER_NODE CCabinet::NewFolderNode()
/*
 * FUNCTION: Creates a new folder node
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
	PCFFOLDER_NODE Node;

	Node = (PCFFOLDER_NODE)AllocateMemory(sizeof(CFFOLDER_NODE));
	if (!Node)
		return NULL;

	memset(Node, 0, sizeof(CFFOLDER_NODE));

	Node->Folder.CompressionType = CAB_COMP_NONE;

	Node->Prev = FolderListTail;

	if (FolderListTail != NULL)
		FolderListTail->Next = Node;
	else
		FolderListHead = Node;

	FolderListTail = Node;

	return Node;
}


PCFFILE_NODE CCabinet::NewFileNode()
/*
 * FUNCTION: Creates a new file node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node to bind file to
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
	PCFFILE_NODE Node;

	Node = (PCFFILE_NODE)AllocateMemory(sizeof(CFFILE_NODE));
	if (!Node)
		return NULL;

	memset(Node, 0, sizeof(CFFILE_NODE));

	Node->Prev = FileListTail;

	if (FileListTail != NULL)
		FileListTail->Next = Node;
	else
		FileListHead = Node;

	FileListTail = Node;

	return Node;
}


PCFDATA_NODE CCabinet::NewDataNode(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Creates a new data block node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node to bind data block to
 * RETURNS:
 *     Pointer to node if there was enough free memory available, otherwise NULL
 */
{
	PCFDATA_NODE Node;

	Node = (PCFDATA_NODE)AllocateMemory(sizeof(CFDATA_NODE));
	if (!Node)
		return NULL;

	memset(Node, 0, sizeof(CFDATA_NODE));

	Node->Prev = FolderNode->DataListTail;

	if (FolderNode->DataListTail != NULL)
		FolderNode->DataListTail->Next = Node;
	else
		FolderNode->DataListHead = Node;

	FolderNode->DataListTail = Node;

	return Node;
}


void CCabinet::DestroyDataNodes(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Destroys data block nodes bound to a folder node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
	PCFDATA_NODE PrevNode;
	PCFDATA_NODE NextNode;

	NextNode = FolderNode->DataListHead;
	while (NextNode != NULL)
	{
		PrevNode = NextNode->Next;
		FreeMemory(NextNode);
		NextNode = PrevNode;
	}
	FolderNode->DataListHead = NULL;
	FolderNode->DataListTail = NULL;
}


void CCabinet::DestroyFileNodes()
/*
 * FUNCTION: Destroys file nodes
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
	PCFFILE_NODE PrevNode;
	PCFFILE_NODE NextNode;

	NextNode = FileListHead;
	while (NextNode != NULL)
	{
		PrevNode = NextNode->Next;
		if (NextNode->FileName)
			FreeMemory(NextNode->FileName);
		FreeMemory(NextNode);
		NextNode = PrevNode;
	}
	FileListHead = NULL;
	FileListTail = NULL;
}


void CCabinet::DestroyDeletedFileNodes()
/*
 * FUNCTION: Destroys file nodes that are marked for deletion
 */
{
	PCFFILE_NODE CurNode;
	PCFFILE_NODE NextNode;

	CurNode = FileListHead;
	while (CurNode != NULL)
	{
		NextNode = CurNode->Next;

		if (CurNode->Delete)
		{
			if (CurNode->Prev != NULL)
				CurNode->Prev->Next = CurNode->Next;
			else
			{
				FileListHead = CurNode->Next;
				if (FileListHead)
					FileListHead->Prev = NULL;
			}

			if (CurNode->Next != NULL)
				CurNode->Next->Prev = CurNode->Prev;
			else
			{
				FileListTail = CurNode->Prev;
				if (FileListTail)
					FileListTail->Next = NULL;
			}

			DPRINT(MAX_TRACE, ("Deleting file: '%s'\n", CurNode->FileName));

			TotalFileSize -= (sizeof(CFFILE) + (ULONG)strlen(GetFileName(CurNode->FileName)) + 1);

			if (CurNode->FileName)
				FreeMemory(CurNode->FileName);
			FreeMemory(CurNode);
		}
		CurNode = NextNode;
	}
}


void CCabinet::DestroyFolderNodes()
/*
 * FUNCTION: Destroys folder nodes
 */
{
	PCFFOLDER_NODE PrevNode;
	PCFFOLDER_NODE NextNode;

	NextNode = FolderListHead;
	while (NextNode != NULL)
	{
		PrevNode = NextNode->Next;
		DestroyDataNodes(NextNode);
		FreeMemory(NextNode);
		NextNode = PrevNode;
	}
	FolderListHead = NULL;
	FolderListTail = NULL;
}


void CCabinet::DestroyDeletedFolderNodes()
/*
 * FUNCTION: Destroys folder nodes that are marked for deletion
 */
{
	PCFFOLDER_NODE CurNode;
	PCFFOLDER_NODE NextNode;

	CurNode = FolderListHead;
	while (CurNode != NULL)
	{
		NextNode = CurNode->Next;

		if (CurNode->Delete)
		{
			if (CurNode->Prev != NULL)
				CurNode->Prev->Next = CurNode->Next;
			else
			{
				FolderListHead = CurNode->Next;
				if (FolderListHead)
					FolderListHead->Prev = NULL;
			}

			if (CurNode->Next != NULL)
				CurNode->Next->Prev = CurNode->Prev;
			else
			{
				FolderListTail = CurNode->Prev;
				if (FolderListTail)
					FolderListTail->Next = NULL;
			}

			DestroyDataNodes(CurNode);
			FreeMemory(CurNode);

			TotalFolderSize -= sizeof(CFFOLDER);
		}
		CurNode = NextNode;
	}
}


ULONG CCabinet::ComputeChecksum(void* Buffer,
                                   ULONG Size,
                                   ULONG Seed)
/*
 * FUNCTION: Computes checksum for data block
 * ARGUMENTS:
 *     Buffer = Pointer to data buffer
 *     Size   = Length of data buffer
 *     Seed   = Previously computed checksum
 * RETURNS:
 *     Checksum of buffer
 */
{
	int UlongCount; // Number of ULONGs in block
	ULONG Checksum; // Checksum accumulator
	unsigned char* pb;
	ULONG ul;

	/* FIXME: Doesn't seem to be correct. EXTRACT.EXE
	   won't accept checksums computed by this routine */

	DPRINT(MIN_TRACE, ("Checksumming buffer (0x%p)  Size (%d)\n", Buffer, Size));

	UlongCount = Size / 4;              // Number of ULONGs
	Checksum   = Seed;                  // Init checksum
	pb         = (unsigned char*)Buffer;         // Start at front of data block

	/* Checksum integral multiple of ULONGs */
	while (UlongCount-- > 0)
	{
		/* NOTE: Build ULONG in big/little-endian independent manner */
		ul = *pb++;                     // Get low-order byte
		ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
		ul |= (((ULONG)(*pb++)) << 16); // Add 3nd byte
		ul |= (((ULONG)(*pb++)) << 24); // Add 4th byte

		Checksum ^= ul;                 // Update checksum
	}

	/* Checksum remainder bytes */
	ul = 0;
	switch (Size % 4)
	{
		case 3:
			ul |= (((ULONG)(*pb++)) << 16); // Add 3rd byte
		case 2:
			ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
		case 1:
			ul |= *pb++;                    // Get low-order byte
		default:
			break;
	}
	Checksum ^= ul;                         // Update checksum

	/* Return computed checksum */
	return Checksum;
}


ULONG CCabinet::ReadBlock(void* Buffer,
                             ULONG Size,
                             PULONG BytesRead)
/*
 * FUNCTION: Read a block of data from file
 * ARGUMENTS:
 *     Buffer    = Pointer to data buffer
 *     Size      = Length of data buffer
 *     BytesRead = Pointer to ULONG that on return will contain
 *                 number of bytes read
 * RETURNS:
 *     Status of operation
 */
{
	if (!ReadFileData(FileHandle, Buffer, Size, BytesRead))
		return CAB_STATUS_INVALID_CAB;
	return CAB_STATUS_SUCCESS;
}

#ifndef CAB_READ_ONLY

ULONG CCabinet::InitCabinetHeader()
/*
 * FUNCTION: Initializes cabinet header and optional fields
 * RETURNS:
 *     Status of operation
 */
{
	ULONG TotalSize;
	ULONG Size;

	CABHeader.FileTableOffset = 0;    // Not known yet
	CABHeader.FolderCount     = 0;    // Not known yet
	CABHeader.FileCount       = 0;    // Not known yet
	CABHeader.Flags           = 0;    // Not known yet

	CABHeader.CabinetNumber = (USHORT)CurrentDiskNumber;

	if ((CurrentDiskNumber > 0) && (OnCabinetName(PrevCabinetNumber, CabinetPrev)))
	{
		CABHeader.Flags |= CAB_FLAG_HASPREV;
		if (!OnDiskLabel(PrevCabinetNumber, DiskPrev))
			strcpy(CabinetPrev, "");
	}

	if (OnCabinetName(CurrentDiskNumber + 1, CabinetNext))
	{
		CABHeader.Flags |= CAB_FLAG_HASNEXT;
		if (!OnDiskLabel(CurrentDiskNumber + 1, DiskNext))
			strcpy(DiskNext, "");
	}

	TotalSize = 0;

	if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0)
	{

		DPRINT(MAX_TRACE, ("CabinetPrev '%s'.\n", CabinetPrev));

		/* Calculate size of name of previous cabinet */
		TotalSize += (ULONG)strlen(CabinetPrev) + 1;

		/* Calculate size of label of previous disk */
		TotalSize += (ULONG)strlen(DiskPrev) + 1;
	}

	if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0)
	{

		DPRINT(MAX_TRACE, ("CabinetNext '%s'.\n", CabinetNext));

		/* Calculate size of name of next cabinet */
		Size = (ULONG)strlen(CabinetNext) + 1;
		TotalSize += Size;
		NextFieldsSize = Size;

		/* Calculate size of label of next disk */
		Size = (ULONG)strlen(DiskNext) + 1;
		TotalSize += Size;
		NextFieldsSize += Size;
	}
	else
		NextFieldsSize = 0;

	/* Add cabinet reserved area size if present */
	if (CabinetReservedFileSize > 0)
	{
		CABHeader.Flags |= CAB_FLAG_RESERVE;
		TotalSize += CabinetReservedFileSize;
		TotalSize += sizeof(ULONG); /* For CabinetResSize, FolderResSize, and FileResSize fields */
	}

	DiskSize += TotalSize;

	TotalHeaderSize = sizeof(CFHEADER) + TotalSize;

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteCabinetHeader(bool MoreDisks)
/*
 * FUNCTION: Writes the cabinet header and optional fields
 * ARGUMENTS:
 *     MoreDisks = true if next cabinet name should be included
 * RETURNS:
 *     Status of operation
 */
{
	PCFFOLDER_NODE FolderNode;
	PCFFILE_NODE FileNode;
	ULONG BytesWritten;
	ULONG Size;

	if (MoreDisks)
	{
		CABHeader.Flags |= CAB_FLAG_HASNEXT;
		Size = TotalHeaderSize;
	}
	else
	{
		CABHeader.Flags &= ~CAB_FLAG_HASNEXT;
		DiskSize -= NextFieldsSize;
		Size = TotalHeaderSize - NextFieldsSize;
	}

	/* Set absolute folder offsets */
	BytesWritten = Size + TotalFolderSize + TotalFileSize;
	CABHeader.FolderCount = 0;
	FolderNode = FolderListHead;
	while (FolderNode != NULL)
	{
		FolderNode->Folder.DataOffset = BytesWritten;

		BytesWritten += FolderNode->TotalFolderSize;

		CABHeader.FolderCount++;

		FolderNode = FolderNode->Next;
	}

	/* Set absolute offset of file table */
	CABHeader.FileTableOffset = Size + TotalFolderSize;

	/* Count number of files to be committed */
	CABHeader.FileCount = 0;
	FileNode = FileListHead;
	while (FileNode != NULL)
	{
		if (FileNode->Commit)
			CABHeader.FileCount++;
		FileNode = FileNode->Next;
	}

	CABHeader.CabinetSize = DiskSize;

	/* Write header */
#if defined(WIN32)
	if (!WriteFile(FileHandle, &CABHeader, sizeof(CFHEADER), (LPDWORD)&BytesWritten, NULL))
	{
		DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
		return CAB_STATUS_CANNOT_WRITE;
	}
#else
	BytesWritten = sizeof(CFHEADER);
	if (fwrite(&CABHeader, sizeof(CFHEADER), 1, FileHandle) < 1)
	{
		DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
		return CAB_STATUS_CANNOT_WRITE;
	}
#endif

	/* Write per-cabinet reserved area if present */
	if (CABHeader.Flags & CAB_FLAG_RESERVE)
	{
		ULONG ReservedSize;

		ReservedSize = CabinetReservedFileSize & 0xffff;
		ReservedSize |= (0 << 16); /* Folder reserved area size */
		ReservedSize |= (0 << 24); /* Folder reserved area size */
#if defined(WIN32)
		if (!WriteFile(FileHandle, &ReservedSize, sizeof(ULONG), (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = sizeof(ULONG);
		if (fwrite(&ReservedSize, sizeof(ULONG), 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif

#if defined(WIN32)
		if (!WriteFile(FileHandle, CabinetReservedFileBuffer, CabinetReservedFileSize, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = CabinetReservedFileSize;
		if (fwrite(CabinetReservedFileBuffer, CabinetReservedFileSize, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif
	}

	if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0)
	{
		DPRINT(MAX_TRACE, ("CabinetPrev '%s'.\n", CabinetPrev));

		/* Write name of previous cabinet */
		Size = (ULONG)strlen(CabinetPrev) + 1;
#if defined(WIN32)
		if (!WriteFile(FileHandle, CabinetPrev, Size, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = Size;
		if (fwrite(CabinetPrev, Size, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif

		DPRINT(MAX_TRACE, ("DiskPrev '%s'.\n", DiskPrev));

		/* Write label of previous disk */
		Size = (ULONG)strlen(DiskPrev) + 1;
#if defined(WIN32)
		if (!WriteFile(FileHandle, DiskPrev, Size, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = Size;
		if (fwrite(DiskPrev, Size, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif
	}

	if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0)
	{
		DPRINT(MAX_TRACE, ("CabinetNext '%s'.\n", CabinetNext));

		/* Write name of next cabinet */
		Size = (ULONG)strlen(CabinetNext) + 1;
#if defined(WIN32)
		if (!WriteFile(FileHandle, CabinetNext, Size, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = Size;
		if (fwrite(CabinetNext, Size, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif

		DPRINT(MAX_TRACE, ("DiskNext '%s'.\n", DiskNext));

		/* Write label of next disk */
		Size = (ULONG)strlen(DiskNext) + 1;
#if defined(WIN32)
		if (!WriteFile(FileHandle, DiskNext, Size, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = Size;
		if (fwrite(DiskNext, Size, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif
	}

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteFolderEntries()
/*
 * FUNCTION: Writes folder entries
 * RETURNS:
 *     Status of operation
 */
{
	PCFFOLDER_NODE FolderNode;
	ULONG BytesWritten;

	DPRINT(MAX_TRACE, ("Writing folder table.\n"));

	FolderNode = FolderListHead;
	while (FolderNode != NULL)
	{
		if (FolderNode->Commit)
		{
			DPRINT(MAX_TRACE, ("Writing folder entry. CompressionType (0x%X)  DataBlockCount (%d)  DataOffset (0x%lX).\n",
				FolderNode->Folder.CompressionType, FolderNode->Folder.DataBlockCount, FolderNode->Folder.DataOffset));

#if defined(WIN32)
			if (!WriteFile(FileHandle,
						&FolderNode->Folder,
						sizeof(CFFOLDER),
						(LPDWORD)&BytesWritten,
						NULL))
			{
				DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
				return CAB_STATUS_CANNOT_WRITE;
			}
#else
			BytesWritten = sizeof(CFFOLDER);
			if (fwrite(&FolderNode->Folder, sizeof(CFFOLDER), 1, FileHandle) < 1)
			{
				DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
				return CAB_STATUS_CANNOT_WRITE;
			}
#endif
		}
		FolderNode = FolderNode->Next;
	}

	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteFileEntries()
/*
 * FUNCTION: Writes file entries for all files
 * RETURNS:
 *     Status of operation
 */
{
	PCFFILE_NODE File;
	ULONG BytesWritten;
	bool SetCont = false;

	DPRINT(MAX_TRACE, ("Writing file table.\n"));

	File = FileListHead;
	while (File != NULL)
	{
		if (File->Commit)
		{
			/* Remove any continued files that ends in this disk */
			if (File->File.FileControlID == CAB_FILE_CONTINUED)
				File->Delete = true;

			/* The file could end in the last (split) block and should therefore
			   appear in the next disk too */

			if ((File->File.FileOffset + File->File.FileSize >= LastBlockStart) &&
				(File->File.FileControlID <= CAB_FILE_MAX_FOLDER) && (BlockIsSplit))
			{
				File->File.FileControlID = CAB_FILE_SPLIT;
				File->Delete = false;
				SetCont = true;
			}

			DPRINT(MAX_TRACE, ("Writing file entry. FileControlID (0x%X)  FileOffset (0x%lX)  FileSize (%lu)  FileName (%s).\n",
				File->File.FileControlID, File->File.FileOffset, File->File.FileSize, File->FileName));

#if defined(WIN32)
			if (!WriteFile(FileHandle,
				&File->File,
				sizeof(CFFILE),
				(LPDWORD)&BytesWritten,
				NULL))
				return CAB_STATUS_CANNOT_WRITE;
#else
			BytesWritten = sizeof(CFFILE);
			if (fwrite(&File->File, sizeof(CFFILE), 1, FileHandle) < 1)
			{
				DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
				return CAB_STATUS_CANNOT_WRITE;
			}
#endif

#if defined(WIN32)
			if (!WriteFile(FileHandle,
				GetFileName(File->FileName),
				(DWORD)strlen(GetFileName(File->FileName)) + 1,
				(LPDWORD)&BytesWritten,
				NULL))
				return CAB_STATUS_CANNOT_WRITE;
#else
			BytesWritten = strlen(GetFileName(File->FileName)) + 1;
			if (fwrite(GetFileName(File->FileName), strlen(GetFileName(File->FileName)) + 1, 1, FileHandle) < 1)
			{
				DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
				return CAB_STATUS_CANNOT_WRITE;
			}
#endif

			if (SetCont)
			{
				File->File.FileControlID = CAB_FILE_CONTINUED;
				SetCont = false;
			}
		}

		File = File->Next;
	}
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::CommitDataBlocks(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Writes data blocks to the cabinet
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node containing the data blocks
 * RETURNS:
 *     Status of operation
 */
{
	PCFDATA_NODE DataNode;
	ULONG BytesWritten;
	ULONG BytesRead;
	ULONG Status;

	DataNode = FolderNode->DataListHead;
	if (DataNode != NULL)
		Status = ScratchFile->Seek(DataNode->ScratchFilePosition);

	while (DataNode != NULL)
	{
		DPRINT(MAX_TRACE, ("Reading block at (0x%lX)  CompSize (%d)  UncompSize (%d).\n",
			DataNode->ScratchFilePosition,
			DataNode->Data.CompSize,
			DataNode->Data.UncompSize));

		/* InputBuffer is free for us to use here, so we use it and avoid a
		   memory allocation. OutputBuffer can't be used here because it may
		   still contain valid data (if a data block spans two or more disks) */
		Status = ScratchFile->ReadBlock(&DataNode->Data, InputBuffer, &BytesRead);
		if (Status != CAB_STATUS_SUCCESS)
		{
			DPRINT(MIN_TRACE, ("Cannot read from scratch file (%d).\n", (ULONG)Status));
			return Status;
		}

#if defined(WIN32)
		if (!WriteFile(FileHandle, &DataNode->Data,
			sizeof(CFDATA), (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = sizeof(CFDATA);
		if (fwrite(&DataNode->Data, sizeof(CFDATA), 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif

#if defined(WIN32)
		if (!WriteFile(FileHandle, InputBuffer,
			DataNode->Data.CompSize, (LPDWORD)&BytesWritten, NULL))
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#else
		BytesWritten = DataNode->Data.CompSize;
		if (fwrite(InputBuffer, DataNode->Data.CompSize, 1, FileHandle) < 1)
		{
			DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
			return CAB_STATUS_CANNOT_WRITE;
		}
#endif

		DataNode = DataNode->Next;
	}
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteDataBlock()
/*
 * FUNCTION: Writes the current data block to the scratch file
 * RETURNS:
 *     Status of operation
 */
{
	ULONG Status;
	ULONG BytesWritten;
	PCFDATA_NODE DataNode;

	if (!BlockIsSplit)
	{
		Status = Codec->Compress(OutputBuffer,
			InputBuffer,
			CurrentIBufferSize,
			&TotalCompSize);

		DPRINT(MAX_TRACE, ("Block compressed. CurrentIBufferSize (%lu)  TotalCompSize(%lu).\n",
			CurrentIBufferSize, TotalCompSize));

		CurrentOBuffer     = OutputBuffer;
		CurrentOBufferSize = TotalCompSize;
	}

	DataNode = NewDataNode(CurrentFolderNode);
	if (!DataNode)
	{
		DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
		return CAB_STATUS_NOMEMORY;
	}

	DiskSize += sizeof(CFDATA);

	if (MaxDiskSize > 0)
		/* Disk size is limited */
		BlockIsSplit = (DiskSize + CurrentOBufferSize > MaxDiskSize);
	else
		BlockIsSplit = false;

	if (BlockIsSplit)
	{
		DataNode->Data.CompSize   = (USHORT)(MaxDiskSize - DiskSize);
		DataNode->Data.UncompSize = 0;
		CreateNewDisk = true;
	}
	else
	{
		DataNode->Data.CompSize   = (USHORT)CurrentOBufferSize;
		DataNode->Data.UncompSize = (USHORT)CurrentIBufferSize;
	}

	DataNode->Data.Checksum = 0;
	DataNode->ScratchFilePosition = ScratchFile->Position();

	// FIXME: MAKECAB.EXE does not like this checksum algorithm
	//DataNode->Data.Checksum = ComputeChecksum(CurrentOBuffer, DataNode->Data.CompSize, 0);

	DPRINT(MAX_TRACE, ("Writing block. Checksum (0x%X)  CompSize (%d)  UncompSize (%d).\n",
		(ULONG)DataNode->Data.Checksum,
		(ULONG)DataNode->Data.CompSize,
		(ULONG)DataNode->Data.UncompSize));

	Status = ScratchFile->WriteBlock(&DataNode->Data,
		CurrentOBuffer, &BytesWritten);
	if (Status != CAB_STATUS_SUCCESS)
		return Status;

	DiskSize += BytesWritten;

	CurrentFolderNode->TotalFolderSize += (BytesWritten + sizeof(CFDATA));
	CurrentFolderNode->Folder.DataBlockCount++;

	*(unsigned char**)&CurrentOBuffer += DataNode->Data.CompSize;
	CurrentOBufferSize     -= DataNode->Data.CompSize;

	LastBlockStart += DataNode->Data.UncompSize;

	if (!BlockIsSplit)
	{
		CurrentIBufferSize = 0;
		CurrentIBuffer     = InputBuffer;
	}

	return CAB_STATUS_SUCCESS;
}

#if !defined(WIN32)

void CCabinet::ConvertDateAndTime(time_t* Time,
                                  PUSHORT DosDate,
                                  PUSHORT DosTime)
/*
 * FUNCTION: Returns file times of a file
 * ARGUMENTS:
 *      FileHandle = File handle of file to get file times from
 *      File       = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
	struct tm *timedef;

	timedef = localtime(Time);

	DPRINT(MAX_TRACE, ("day: %d, mon: %d, year:%d, hour: %d, min: %d, sec: %d\n",
		timedef->tm_mday, timedef->tm_mon, timedef->tm_year,
		timedef->tm_sec, timedef->tm_min, timedef->tm_hour));

	*DosDate = ((timedef->tm_mday + 1) << 0)
		| ((timedef->tm_mon + 1) << 5)
		| (((timedef->tm_year + 1900) - 1980) << 9);

	*DosTime = (timedef->tm_sec << 0)
		| (timedef->tm_min << 5)
		| (timedef->tm_hour << 11);
}

#endif // !WIN32


ULONG CCabinet::GetFileTimes(FILEHANDLE FileHandle, PCFFILE_NODE File)
/*
 * FUNCTION: Returns file times of a file
 * ARGUMENTS:
 *      FileHandle = File handle of file to get file times from
 *      File       = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	FILETIME FileTime;

	if (GetFileTime(FileHandle, NULL, NULL, &FileTime))
		FileTimeToDosDateTime(&FileTime,
			&File->File.FileDate,
			&File->File.FileTime);
#else
	struct stat stbuf;
	char buf[MAX_PATH];

	// Check for an absolute path
	if (IsSeparator(File->FileName[0]))
		strcpy(buf, File->FileName);
	else
	{
		getcwd(buf, sizeof(buf));
		strcat(buf, DIR_SEPARATOR_STRING);
		strcat(buf, File->FileName);
	}

	if (stat(buf, &stbuf) == -1)
		return CAB_STATUS_CANNOT_READ;

	ConvertDateAndTime(&stbuf.st_mtime, &File->File.FileDate, &File->File.FileTime);
#endif
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::GetAttributesOnFile(PCFFILE_NODE File)
/*
 * FUNCTION: Returns attributes on a file
 * ARGUMENTS:
 *      File = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	LONG Attributes;

	Attributes = GetFileAttributes(File->FileName);
	if (Attributes == -1)
		return CAB_STATUS_CANNOT_READ;

	if (Attributes & FILE_ATTRIBUTE_READONLY)
		File->File.Attributes |= CAB_ATTRIB_READONLY;

	if (Attributes & FILE_ATTRIBUTE_HIDDEN)
		File->File.Attributes |= CAB_ATTRIB_HIDDEN;

	if (Attributes & FILE_ATTRIBUTE_SYSTEM)
		File->File.Attributes |= CAB_ATTRIB_SYSTEM;

	if (Attributes & FILE_ATTRIBUTE_DIRECTORY)
		File->File.Attributes |= CAB_ATTRIB_DIRECTORY;

	if (Attributes & FILE_ATTRIBUTE_ARCHIVE)
		File->File.Attributes |= CAB_ATTRIB_ARCHIVE;
#else
	struct stat stbuf;
	char buf[MAX_PATH];

	// Check for an absolute path
	if (IsSeparator(File->FileName[0]))
		strcpy(buf, File->FileName);
	else
	{
		getcwd(buf, sizeof(buf));
		strcat(buf, DIR_SEPARATOR_STRING);
		strcat(buf, File->FileName);
	}

	if (stat(buf, &stbuf) == -1)
		return CAB_STATUS_CANNOT_READ;

#if 0
	File->File.Attributes |= CAB_ATTRIB_READONLY;
	File->File.Attributes |= CAB_ATTRIB_HIDDEN;
	File->File.Attributes |= CAB_ATTRIB_SYSTEM;
#endif

	if (stbuf.st_mode & S_IFDIR)
		File->File.Attributes |= CAB_ATTRIB_DIRECTORY;

	File->File.Attributes |= CAB_ATTRIB_ARCHIVE;

#endif
	return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::SetAttributesOnFile(PCFFILE_NODE File)
/*
 * FUNCTION: Sets attributes on a file
 * ARGUMENTS:
 *      File = Pointer to CFFILE node for file
 * RETURNS:
 *     Status of operation
 */
{
#if defined(WIN32)
	ULONG Attributes = 0;

	if (File->File.Attributes & CAB_ATTRIB_READONLY)
		Attributes |= FILE_ATTRIBUTE_READONLY;

	if (File->File.Attributes & CAB_ATTRIB_HIDDEN)
		Attributes |= FILE_ATTRIBUTE_HIDDEN;

	if (File->File.Attributes & CAB_ATTRIB_SYSTEM)
		Attributes |= FILE_ATTRIBUTE_SYSTEM;

	if (File->File.Attributes & CAB_ATTRIB_DIRECTORY)
		Attributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (File->File.Attributes & CAB_ATTRIB_ARCHIVE)
		Attributes |= FILE_ATTRIBUTE_ARCHIVE;

	SetFileAttributes(File->FileName, Attributes);

	return CAB_STATUS_SUCCESS;
#else
	//DPRINT(MIN_TRACE, ("FIXME: SetAttributesOnFile() is unimplemented\n"));
	return CAB_STATUS_SUCCESS;
#endif
}

#endif /* CAB_READ_ONLY */

/* EOF */
