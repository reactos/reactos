/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/cabinet.cpp
 * PURPOSE:     Cabinet routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:       Define CAB_READ_ONLY for read only version
 * REVISIONS:
 *   CSH 21/03-2001 Created
 * TODO:
 *   - Checksum of datablocks should be calculated
 *   - EXTRACT.EXE complains if a disk is created manually
 *   - Folders that are created manually and span disks will result in a damaged cabinet
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "cabinet.h"
#include "raw.h"
#include "mszip.h"


#ifndef CAB_READ_ONLY

#ifdef DBG

VOID DumpBuffer(PVOID Buffer, DWORD Size)
{
    HANDLE FileHandle;
    DWORD BytesWritten;

    /* Create file, overwrite if it already exists */
    FileHandle = CreateFile("dump.bin", // Create this file
        GENERIC_WRITE,                  // Open for writing
        0,                              // No sharing
        NULL,                           // No security
        CREATE_ALWAYS,                  // Create or overwrite
        FILE_ATTRIBUTE_NORMAL,          // Normal file 
        NULL);                          // No attribute template
    if (FileHandle == INVALID_HANDLE_VALUE) {
        DPRINT(MID_TRACE, ("ERROR OPENING '%d'.\n", (UINT)GetLastError()));
        return;
    }

    if (!WriteFile(FileHandle, Buffer, Size, &BytesWritten, NULL)) {
        DPRINT(MID_TRACE, ("ERROR WRITING '%d'.\n", (UINT)GetLastError()));
    }

    CloseHandle(FileHandle);
}

#endif /* DBG */


/* CCFDATAStorage */

CCFDATAStorage::CCFDATAStorage()
/*
 * FUNCTION: Default constructor
 */
{
    FileCreated = FALSE;
}


CCFDATAStorage::~CCFDATAStorage()
/*
 * FUNCTION: Default destructor
 */
{
    ASSERT(!FileCreated);
}


ULONG CCFDATAStorage::Create(LPTSTR FileName)
/*
 * FUNCTION: Creates the file
 * ARGUMENTS:
 *     FileName = Pointer to name of file
 * RETURNS:
 *     Status of operation
 */
{
    TCHAR FullName[MAX_PATH];

    ASSERT(!FileCreated);

    if (GetTempPath(MAX_PATH, FullName) == 0)
        return CAB_STATUS_CANNOT_CREATE;

    lstrcat(FullName, FileName);

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
    if (FileHandle == INVALID_HANDLE_VALUE) {
        DPRINT(MID_TRACE, ("ERROR '%d'.\n", (UINT)GetLastError()));
        return CAB_STATUS_CANNOT_CREATE;
    }

    FileCreated = TRUE;

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

    CloseHandle(FileHandle);

    FileCreated = FALSE;

    return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::Truncate()
/*
 * FUNCTION: Truncate the scratch file to zero bytes
 * RETURNS:
 *     Status of operation
 */
{
    if (!SetFilePointer(FileHandle, 0, NULL, FILE_BEGIN))
        return CAB_STATUS_FAILURE;
    if (!SetEndOfFile(FileHandle))
        return CAB_STATUS_FAILURE;
    return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::Position()
/*
 * FUNCTION: Returns current position in file
 * RETURNS:
 *     Current position
 */
{
    return SetFilePointer(FileHandle, 0, NULL, FILE_CURRENT);
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
    if (SetFilePointer(FileHandle,
        Position,
        NULL,
        FILE_BEGIN) == 0xFFFFFFFF)
        return CAB_STATUS_FAILURE;
    else
        return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::ReadBlock(PCFDATA Data, PVOID Buffer, PDWORD BytesRead)
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
    if (!ReadFile(FileHandle, Buffer, Data->CompSize, BytesRead, NULL))
        return CAB_STATUS_CANNOT_READ;

    return CAB_STATUS_SUCCESS;
}


ULONG CCFDATAStorage::WriteBlock(PCFDATA Data, PVOID Buffer, PDWORD BytesWritten)
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
    if (!WriteFile(FileHandle, Buffer, Data->CompSize, BytesWritten, NULL))
        return CAB_STATUS_CANNOT_WRITE;

    return CAB_STATUS_SUCCESS;
}

#endif /* CAB_READ_ONLY */


/* CCabinet */

CCabinet::CCabinet()
/*
 * FUNCTION: Default constructor
 */
{
    FileOpen = FALSE;
    lstrcpy(DestPath, "");

    FolderListHead = NULL;
    FolderListTail = NULL;
    FileListHead   = NULL;
    FileListTail   = NULL;

    Codec         = new CRawCodec();
    CodecId       = CAB_CODEC_RAW;
    CodecSelected = TRUE;

    OutputBuffer = NULL;
    InputBuffer  = NULL;
    MaxDiskSize  = 0;
    BlockIsSplit = FALSE;
    ScratchFile  = NULL;

    FolderUncompSize = 0;
    BytesLeftInBlock = 0;
    ReuseBlock       = FALSE;
    CurrentDataNode  = NULL;
}


CCabinet::~CCabinet()
/*
 * FUNCTION: Default destructor
 */
{
    if (CodecSelected)
        delete Codec;
}


LPTSTR CCabinet::GetFileName(LPTSTR Path)
/*
 * FUNCTION: Returns a pointer to file name
 * ARGUMENTS:
 *     Path = Pointer to string with pathname
 * RETURNS:
 *     Pointer to filename
 */
{
    DWORD i, j;

    j = i = (Path[0] ? (Path[1] == ':' ? 2 : 0) : 0);

    while (Path [i++]) {
        if (Path [i - 1] == '\\') j = i;
    }
    return Path + j;
}


VOID CCabinet::RemoveFileName(LPTSTR Path)
/*
 * FUNCTION: Removes a file name from a path
 * ARGUMENTS:
 *     Path = Pointer to string with path
 */
{
    LPTSTR FileName;
    DWORD i;

    i = (Path [0] ? (Path[1] == ':' ? 2 : 0) : 0);
    FileName = GetFileName(Path + i);

    if ((FileName != (Path + i)) && (FileName [-1] == '\\'))
        FileName--;
    if ((FileName == (Path + i)) && (FileName [0] == '\\'))
        FileName++;
    FileName[0] = 0;
}


BOOL CCabinet::NormalizePath(LPTSTR Path,
                             DWORD Length)
/*
 * FUNCTION: Normalizes a path
 * ARGUMENTS:
 *     Path   = Pointer to string with pathname
 *     Length = Number of bytes in Path
 * RETURNS:
 *     TRUE if there was enough room in Path, or FALSE
 */
{
    DWORD n;
    BOOL OK = TRUE;

    if ((n = lstrlen(Path)) &&
        (Path[n - 1] != '\\') &&
        (OK = ((n + 1) < Length))) {
        Path[n]     = '\\';
        Path[n + 1] = 0;
    }
    return OK;
}


LPTSTR CCabinet::GetCabinetName()
/*
 * FUNCTION: Returns pointer to cabinet file name
 * RETURNS:
 *     Pointer to string with name of cabinet
 */
{
    return CabinetName;
}


VOID CCabinet::SetCabinetName(LPTSTR FileName)
/*
 * FUNCTION: Sets cabinet file name
 * ARGUMENTS:
 *     FileName = Pointer to string with name of cabinet
 */
{
    lstrcpy(CabinetName, FileName);
}


VOID CCabinet::SetDestinationPath(LPTSTR DestinationPath)
/*
 * FUNCTION: Sets destination path
 * ARGUMENTS:
 *    DestinationPath = Pointer to string with name of destination path
 */
{
    lstrcpy(DestPath, DestinationPath);
    if (lstrlen(DestPath) > 0)
        NormalizePath(DestPath, MAX_PATH);
}


LPTSTR CCabinet::GetDestinationPath()
/*
 * FUNCTION: Returns destination path
 * RETURNS:
 *    Pointer to string with name of destination path
 */
{
    return DestPath;
}


DWORD CCabinet::GetCurrentDiskNumber()
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
    DWORD Index;

    if (!FileOpen) {
        DWORD BytesRead;
        DWORD Size;

        OutputBuffer = HeapAlloc(GetProcessHeap(),
            0, CAB_BLOCKSIZE + 12);    // This should be enough
        if (!OutputBuffer)
            return CAB_STATUS_NOMEMORY;

        FileHandle = CreateFile(CabinetName, // Open this file
            GENERIC_READ,                    // Open for reading 
            FILE_SHARE_READ,                 // Share for reading 
            NULL,                            // No security 
            OPEN_EXISTING,                   // Existing file only 
            FILE_ATTRIBUTE_NORMAL,           // Normal file 
            NULL);                           // No attribute template 
 
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DPRINT(MID_TRACE, ("Cannot open file.\n"));
            return CAB_STATUS_CANNOT_OPEN;
        }

        FileOpen = TRUE;

        /* Load CAB header */
        if ((Status = ReadBlock(&CABHeader, sizeof(CFHEADER), &BytesRead))
            != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
            return CAB_STATUS_INVALID_CAB;
        }

        /* Check header */
        if ((BytesRead                 != sizeof(CFHEADER)) ||
            (CABHeader.Signature       != CAB_SIGNATURE   ) ||
            (CABHeader.Version         != CAB_VERSION     ) ||
            (CABHeader.FolderCount     == 0               ) ||
            (CABHeader.FileCount       == 0               ) ||
            (CABHeader.FileTableOffset < sizeof(CFHEADER))) {
            CloseCabinet();
            DPRINT(MID_TRACE, ("File has invalid header.\n"));
            return CAB_STATUS_INVALID_CAB;
        }

        Size = 0;

        /* Read/skip any reserved bytes */
        if (CABHeader.Flags & CAB_FLAG_RESERVE) {
            if ((Status = ReadBlock(&Size, sizeof(DWORD), &BytesRead))
                != CAB_STATUS_SUCCESS) {
                DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
                return CAB_STATUS_INVALID_CAB;
            }
            CabinetReserved = Size & 0xFFFF;
            FolderReserved  = (Size >> 16) & 0xFF;
            DataReserved    = (Size >> 24) & 0xFF;

            SetFilePointer(FileHandle, CabinetReserved, NULL, FILE_CURRENT);
            if (GetLastError() != NO_ERROR) {
                DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
                return CAB_STATUS_NOMEMORY;
            }
        }

        if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0) {
            /* Read name of previous cabinet */
            Status = ReadString(CabinetPrev, 256);
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
            /* Read label of previous disk */
            Status = ReadString(DiskPrev, 256);
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
        } else {
            lstrcpy(CabinetPrev, "");
            lstrcpy(DiskPrev,    "");
        }
  
        if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0) {
            /* Read name of next cabinet */
            Status = ReadString(CabinetNext, 256);
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
            /* Read label of next disk */
            Status = ReadString(DiskNext, 256);
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
        } else {
            lstrcpy(CabinetNext, "");
            lstrcpy(DiskNext,    "");
        }

        /* Read all folders */
        for (Index = 0; Index < CABHeader.FolderCount; Index++) {
            FolderNode = NewFolderNode();
            if (!FolderNode) {
                DPRINT(MIN_TRACE, ("Insufficient resources.\n"));
                return CAB_STATUS_NOMEMORY;
            }

            if (Index == 0)
                FolderNode->UncompOffset = FolderUncompSize;

            FolderNode->Index = Index;

            if ((Status = ReadBlock(&FolderNode->Folder,
                sizeof(CFFOLDER), &BytesRead)) != CAB_STATUS_SUCCESS) {
                DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
                return CAB_STATUS_INVALID_CAB;
            }
        }

        /* Read file entries */
        Status = ReadFileTable();
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("ReadFileTable() failed (%d).\n", (UINT)Status));
            return Status;
        }

        /* Read data blocks for all folders */
        FolderNode = FolderListHead;
        while (FolderNode != NULL) {
            Status = ReadDataBlocks(FolderNode);
            if (Status != CAB_STATUS_SUCCESS) {
                DPRINT(MIN_TRACE, ("ReadDataBlocks() failed (%d).\n", (UINT)Status));
                return Status;
            }
            FolderNode = FolderNode->Next;
        }
    }
    return CAB_STATUS_SUCCESS;
}


VOID CCabinet::Close()
/*
 * FUNCTION: Closes the cabinet file
 */
{
    if (FileOpen) {
        CloseHandle(FileHandle);
        FileOpen = FALSE;
    }
}


ULONG CCabinet::FindFirst(LPTSTR FileName,
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
    RestartSearch = FALSE;
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

    if (RestartSearch) {
        Search->Next  = FileListHead;

        /* Skip split files already extracted */
        while ((Search->Next) &&
            (Search->Next->File.FileControlID > CAB_FILE_MAX_FOLDER) &&
            (Search->Next->File.FileOffset <= LastFileOffset)) {
            DPRINT(MAX_TRACE, ("Skipping file (%s)  FileOffset (0x%X)  LastFileOffset (0x%X).\n",
                Search->Next->FileName, Search->Next->File.FileOffset, LastFileOffset));
            Search->Next = Search->Next->Next;
        }

        RestartSearch = FALSE;
    }

    /* FIXME: Check search criteria */

    if (!Search->Next) {
        if (lstrlen(DiskNext) > 0) {
            CloseCabinet();

            SetCabinetName(CabinetNext);

            OnDiskChange(CabinetNext, DiskNext);

            Status = Open();
            if (Status != CAB_STATUS_SUCCESS) 
                return Status;

            Search->Next = FileListHead;
            if (!Search->Next)
                return CAB_STATUS_NOFILE;
        } else
            return CAB_STATUS_NOFILE;
    }

    Search->File     = &Search->Next->File;
    Search->FileName = Search->Next->FileName;
    Search->Next     = Search->Next->Next;
    return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::ExtractFile(LPTSTR FileName)
/*
 * FUNCTION: Extracts a file from the cabinet
 * ARGUMENTS:
 *     FileName = Pointer to buffer with name of file
 * RETURNS
 *     Status of operation
 */
{
    DWORD Size;
    DWORD Offset;
    DWORD BytesRead;
    DWORD BytesToRead;
    DWORD BytesWritten;
    DWORD BytesSkipped;
    DWORD BytesToWrite;
    DWORD TotalBytesRead;
    DWORD CurrentOffset;
    PUCHAR Buffer;
    PUCHAR CurrentBuffer;
    HANDLE DestFile;
    PCFFILE_NODE File;
    CFDATA CFData;
    ULONG Status;
    BOOL Skip;
    FILETIME FileTime;
    TCHAR DestName[MAX_PATH];
    TCHAR TempName[MAX_PATH];

    Status = LocateFile(FileName, &File);
    if (Status != CAB_STATUS_SUCCESS) {
        DPRINT(MID_TRACE, ("Cannot locate file (%d).\n", (UINT)Status));
        return Status;
    }

    LastFileOffset = File->File.FileOffset;

    switch (CurrentFolderNode->Folder.CompressionType & CAB_COMP_MASK) {
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
        (UINT)File->File.FileOffset,
        (UINT)File->File.FileSize,
        (UINT)File->DataBlock->AbsoluteOffset,
        (UINT)File->DataBlock->UncompOffset));

    lstrcpy(DestName, DestPath);
    lstrcat(DestName, FileName);

    /* Create destination file, fail if it already exists */
    DestFile = CreateFile(DestName,      // Create this file
        GENERIC_WRITE,                   // Open for writing
        0,                               // No sharing
        NULL,                            // No security
        CREATE_NEW,                      // New file only
        FILE_ATTRIBUTE_NORMAL,           // Normal file
        NULL);                           // No attribute template
    if (DestFile == INVALID_HANDLE_VALUE) {
        /* If file exists, ask to overwrite file */
        if (((Status = GetLastError()) == ERROR_FILE_EXISTS) &&
            (OnOverwrite(&File->File, FileName))) {
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
        } else {
            if (Status == ERROR_FILE_EXISTS)
                return CAB_STATUS_FILE_EXISTS;
            else
                return CAB_STATUS_CANNOT_CREATE;
        }
    }

    if (!DosDateTimeToFileTime(File->File.FileDate, File->File.FileTime, &FileTime)) {
        CloseHandle(DestFile);
        DPRINT(MIN_TRACE, ("DosDateTimeToFileTime() failed (%d).\n", GetLastError()));
        return CAB_STATUS_CANNOT_WRITE;
    }

    SetFileTime(DestFile, NULL, &FileTime, NULL);

    SetAttributesOnFile(File);

    Buffer = (PUCHAR)HeapAlloc(GetProcessHeap(),
        0, CAB_BLOCKSIZE + 12); // This should be enough
    if (!Buffer) {
        CloseHandle(DestFile);
        DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
        return CAB_STATUS_NOMEMORY; 
    }

    /* Call OnExtract event handler */
    OnExtract(&File->File, FileName);

	/* Search to start of file */
	Offset = SetFilePointer(FileHandle,
		File->DataBlock->AbsoluteOffset,
		NULL,
		FILE_BEGIN);
	if (GetLastError() != NO_ERROR) {
		DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
		return CAB_STATUS_INVALID_CAB;
	}

    Size   = File->File.FileSize;
    Offset = File->File.FileOffset;
    CurrentOffset = File->DataBlock->UncompOffset;

    Skip = TRUE;

	ReuseBlock = (CurrentDataNode == File->DataBlock);
    if (Size > 0) {
		do {
			DPRINT(MAX_TRACE, ("CO (0x%X)    ReuseBlock (%d)    Offset (0x%X)   Size (%d)  BytesLeftInBlock (%d)\n",
				File->DataBlock->UncompOffset, (UINT)ReuseBlock, Offset, Size,
				BytesLeftInBlock));

			if (/*(CurrentDataNode != File->DataBlock) &&*/ (!ReuseBlock) || (BytesLeftInBlock <= 0)) {

				DPRINT(MAX_TRACE, ("Filling buffer. ReuseBlock (%d)\n", (UINT)ReuseBlock));

                CurrentBuffer  = Buffer;
                TotalBytesRead = 0;
                do {
                    DPRINT(MAX_TRACE, ("Size (%d bytes).\n", Size));

                    if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) != 
                        CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA))) {
                        CloseHandle(DestFile);
                        HeapFree(GetProcessHeap(), 0, Buffer);
                        DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
                        return CAB_STATUS_INVALID_CAB;
                    }

                    DPRINT(MAX_TRACE, ("Data block: Checksum (0x%X)  CompSize (%d bytes)  UncompSize (%d bytes)  Offset (0x%X).\n",
                        (UINT)CFData.Checksum,
                        (UINT)CFData.CompSize,
                        (UINT)CFData.UncompSize,
						(UINT)SetFilePointer(FileHandle, 0, NULL, FILE_CURRENT)));

                    ASSERT(CFData.CompSize <= CAB_BLOCKSIZE + 12);

                    BytesToRead = CFData.CompSize;

					DPRINT(MAX_TRACE, ("Read: (0x%X,0x%X).\n",
						CurrentBuffer, Buffer));

                    if (((Status = ReadBlock(CurrentBuffer, BytesToRead, &BytesRead)) != 
                        CAB_STATUS_SUCCESS) || (BytesToRead != BytesRead)) {
                        CloseHandle(DestFile);
                        HeapFree(GetProcessHeap(), 0, Buffer);
                        DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
                        return CAB_STATUS_INVALID_CAB;
                    }

                    /* FIXME: Does not work with files generated by makecab.exe */
/*
                    if (CFData.Checksum != 0) {
                        DWORD Checksum = ComputeChecksum(CurrentBuffer, BytesRead, 0);
                        if (Checksum != CFData.Checksum) {
                            CloseHandle(DestFile);
                            HeapFree(GetProcessHeap(), 0, Buffer);
                            DPRINT(MIN_TRACE, ("Bad checksum (is 0x%X, should be 0x%X).\n",
                                Checksum, CFData.Checksum));
                            return CAB_STATUS_INVALID_CAB;
                        }
                    }
*/
                    TotalBytesRead += BytesRead;

                    CurrentBuffer += BytesRead;

                    if (CFData.UncompSize == 0) {
                        if (lstrlen(DiskNext) == 0)
                            return CAB_STATUS_NOFILE;

                        /* CloseCabinet() will destroy all file entries so in case
                           FileName refers to the FileName field of a CFFOLDER_NODE
                           structure, we have to save a copy of the filename */
                        lstrcpy(TempName, FileName);

                        CloseCabinet();

                        SetCabinetName(CabinetNext);

                        OnDiskChange(CabinetNext, DiskNext);

                        Status = Open();
                        if (Status != CAB_STATUS_SUCCESS) 
                            return Status;

                        /* The first data block of the file will not be
                           found as it is located in the previous file */
                        Status = LocateFile(TempName, &File);
                        if (Status == CAB_STATUS_NOFILE) {
                            DPRINT(MID_TRACE, ("Cannot locate file (%d).\n", (UINT)Status));
                            return Status;
                        }

                        /* The file is continued in the first data block in the folder */
                        File->DataBlock = CurrentFolderNode->DataListHead;

                        /* Search to start of file */
                        SetFilePointer(FileHandle,
                            File->DataBlock->AbsoluteOffset,
                            NULL,
                            FILE_BEGIN);
                        if (GetLastError() != NO_ERROR) {
                            DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
                            return CAB_STATUS_INVALID_CAB;
                        }

                        DPRINT(MAX_TRACE, ("Continuing extraction of file at uncompressed offset (0x%X)  Size (%d bytes)  AO (0x%X)  UO (0x%X).\n",
                            (UINT)File->File.FileOffset,
                            (UINT)File->File.FileSize,
                            (UINT)File->DataBlock->AbsoluteOffset,
                            (UINT)File->DataBlock->UncompOffset));

                        CurrentDataNode = File->DataBlock;
                        ReuseBlock = TRUE;

                        RestartSearch = TRUE;
                    }
                } while (CFData.UncompSize == 0);

                DPRINT(MAX_TRACE, ("TotalBytesRead (%d).\n", TotalBytesRead));

                Status = Codec->Uncompress(OutputBuffer, Buffer, TotalBytesRead, &BytesToWrite);
                if (Status != CS_SUCCESS) {
                    CloseHandle(DestFile);
                    HeapFree(GetProcessHeap(), 0, Buffer);
                    DPRINT(MID_TRACE, ("Cannot uncompress block.\n"));
                    if (Status == CS_NOMEMORY)
                        return CAB_STATUS_NOMEMORY;
                    return CAB_STATUS_INVALID_CAB;
                }

                if (BytesToWrite != CFData.UncompSize) {
                    DPRINT(MID_TRACE, ("BytesToWrite (%d) != CFData.UncompSize (%d)\n",
                        BytesToWrite, CFData.UncompSize));
                    return CAB_STATUS_INVALID_CAB;
                }

                BytesLeftInBlock = BytesToWrite;
            } else {
                DPRINT(MAX_TRACE, ("Using same buffer. ReuseBlock (%d)\n", (UINT)ReuseBlock));

                BytesToWrite = BytesLeftInBlock;

				DPRINT(MAX_TRACE, ("Seeking to absolute offset 0x%X.\n",
					CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
                    CurrentDataNode->Data.CompSize));

				if (((Status = ReadBlock(&CFData, sizeof(CFDATA), &BytesRead)) != 
					CAB_STATUS_SUCCESS) || (BytesRead != sizeof(CFDATA))) {
					CloseHandle(DestFile);
					HeapFree(GetProcessHeap(), 0, Buffer);
					DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
					return CAB_STATUS_INVALID_CAB;
				}

				DPRINT(MAX_TRACE, ("CFData.CompSize 0x%X  CFData.UncompSize 0x%X.\n",
					CFData.CompSize, CFData.UncompSize));

                /* Go to next data block */
                SetFilePointer(FileHandle,
                    CurrentDataNode->AbsoluteOffset + sizeof(CFDATA) +
                    CurrentDataNode->Data.CompSize,
                    NULL,
                    FILE_BEGIN);
                if (GetLastError() != NO_ERROR) {
                    DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
                    return CAB_STATUS_INVALID_CAB;
                }

                ReuseBlock = FALSE;
            }

            if (Skip)
                BytesSkipped = (Offset - CurrentOffset);
            else
                BytesSkipped = 0;

			BytesToWrite -= BytesSkipped;

			if (Size < BytesToWrite)
                BytesToWrite = Size;

            DPRINT(MAX_TRACE, ("Offset (0x%X)  CurrentOffset (0x%X)  ToWrite (%d)  Skipped (%d)(%d)  Size (%d).\n",
                (UINT)Offset,
                (UINT)CurrentOffset,
                (UINT)BytesToWrite,
                (UINT)BytesSkipped, (UINT)Skip,
                (UINT)Size));

            if (!WriteFile(DestFile, (PVOID)((ULONG)OutputBuffer + BytesSkipped),
                BytesToWrite, &BytesWritten, NULL) ||
                (BytesToWrite != BytesWritten)) {

				DPRINT(MIN_TRACE, ("Status 0x%X.\n", GetLastError()));

                CloseHandle(DestFile);
                HeapFree(GetProcessHeap(), 0, Buffer);
                DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
                return CAB_STATUS_CANNOT_WRITE;
            }
            Size -= BytesToWrite;

			CurrentOffset += BytesToWrite;

            /* Don't skip any more bytes */
            Skip = FALSE;
        } while (Size > 0);
    }

    CloseHandle(DestFile);

    HeapFree(GetProcessHeap(), 0, Buffer);

    return CAB_STATUS_SUCCESS;
}


VOID CCabinet::SelectCodec(ULONG Id)
/*
 * FUNCTION: Selects codec engine to use
 * ARGUMENTS:
 *     Id = Codec identifier
 */
{
    if (CodecSelected) {
        if (Id == CodecId)
            return;

        CodecSelected = FALSE;
        delete Codec;
    }

    switch (Id) {
    case CAB_CODEC_RAW:
        Codec = new CRawCodec();
        break;
    case CAB_CODEC_MSZIP:
        Codec = new CMSZipCodec();
        break;
    default:
        return;
    }

    CodecId       = Id;
    CodecSelected = TRUE;
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

    OutputBuffer = HeapAlloc(GetProcessHeap(), 0, CAB_BLOCKSIZE + 12); // This should be enough
    InputBuffer  = HeapAlloc(GetProcessHeap(), 0, CAB_BLOCKSIZE + 12); // This should be enough
    if ((!OutputBuffer) || (!InputBuffer)) {
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
    if (!ScratchFile) {
        DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
        return CAB_STATUS_NOMEMORY;
    }

    Status = ScratchFile->Create("~CAB.tmp");

    CreateNewFolder = FALSE;

    CreateNewDisk = FALSE;

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

    CreateNewDisk = FALSE;

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
    if (!CurrentFolderNode) {
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
    DWORD BytesToRead;
    DWORD BytesRead;
    ULONG Status;
    DWORD Size;

    if (!ContinueFile) {
        /* Try to open file */
        SourceFile = CreateFile(
            FileNode->FileName,      // Open this file
            GENERIC_READ,            // Open for reading
            FILE_SHARE_READ,         // Share for reading
            NULL,                    // No security
            OPEN_EXISTING,           // File must exist
            FILE_ATTRIBUTE_NORMAL,   // Normal file 
            NULL);                   // No attribute template
        if (SourceFile == INVALID_HANDLE_VALUE) {
            DPRINT(MID_TRACE, ("File not found (%s).\n", FileNode->FileName));
            return CAB_STATUS_NOFILE;
        }

        if (CreateNewFolder) {
            /* There is always a new folder after
               a split file is completely stored */
            Status = NewFolder();
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
            CreateNewFolder = FALSE;
        }

        /* Call OnAdd event handler */
        OnAdd(&FileNode->File, FileNode->FileName);

        TotalBytesLeft = FileNode->File.FileSize;

        FileNode->File.FileOffset        = CurrentFolderNode->UncompOffset;
        CurrentFolderNode->UncompOffset += TotalBytesLeft;
        FileNode->File.FileControlID     = NextFolderNumber - 1;
        CurrentFolderNode->Commit        = TRUE;
        PrevCabinetNumber				 = CurrentDiskNumber;

        Size = sizeof(CFFILE) + lstrlen(GetFileName(FileNode->FileName)) + 1;
        CABHeader.FileTableOffset += Size;
        TotalFileSize += Size;
        DiskSize += Size;
    }

    FileNode->Commit = TRUE;

    if (TotalBytesLeft > 0) {
        do {
            if (TotalBytesLeft > (DWORD)CAB_BLOCKSIZE - CurrentIBufferSize)
                BytesToRead = CAB_BLOCKSIZE - CurrentIBufferSize;
            else
                BytesToRead = TotalBytesLeft;

            if ((!ReadFile(SourceFile, CurrentIBuffer, BytesToRead, &BytesRead, NULL)
                != CAB_STATUS_SUCCESS) || (BytesToRead != BytesRead)) {
                DPRINT(MIN_TRACE, ("Cannot read from file. BytesToRead (%d)  BytesRead (%d)  CurrentIBufferSize (%d).\n",
                    BytesToRead, BytesRead, CurrentIBufferSize));
                return CAB_STATUS_INVALID_CAB;
            }

            (PUCHAR)CurrentIBuffer += BytesRead;

            CurrentIBufferSize += (WORD)BytesRead;

            if (CurrentIBufferSize == CAB_BLOCKSIZE) {
                Status = WriteDataBlock();
                if (Status != CAB_STATUS_SUCCESS)
                    return Status;
            }
            TotalBytesLeft -= BytesRead;
        } while ((TotalBytesLeft > 0) && (!CreateNewDisk));
    }

    if (TotalBytesLeft == 0) {
        CloseHandle(SourceFile);
        FileNode->Delete = TRUE;

        if (FileNode->File.FileControlID > CAB_FILE_MAX_FOLDER) {
            FileNode->File.FileControlID = CAB_FILE_CONTINUED;
            CurrentFolderNode->Delete = TRUE;

            if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0)) {
                Status = WriteDataBlock();
                if (Status != CAB_STATUS_SUCCESS)
                    return Status;
            }

            CreateNewFolder = TRUE;
        }
    } else {
        if (FileNode->File.FileControlID <= CAB_FILE_MAX_FOLDER)
            FileNode->File.FileControlID = CAB_FILE_SPLIT;
        else
            FileNode->File.FileControlID = CAB_FILE_PREV_NEXT;
    }

    return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteDisk(DWORD MoreDisks)
/*
 * FUNCTION: Forces the current disk to be written
 * ARGUMENTS:
 *     MoreDisks = TRUE if there is one or more disks after this disk
 * RETURNS:
 *     Status of operation
 */
{
    PCFFILE_NODE FileNode;
    ULONG Status;

    ContinueFile = FALSE;
    FileNode = FileListHead;
    while (FileNode != NULL) {

        Status = WriteFileToScratchStorage(FileNode);
        if (Status != CAB_STATUS_SUCCESS)
            return Status;
        if (CreateNewDisk) {
            /* A data block could span more than two
               disks if MaxDiskSize is very small */
            while (CreateNewDisk) {
                DPRINT(MAX_TRACE, ("Creating new disk.\n"));
                CommitDisk(TRUE);
                CloseDisk();
                NewDisk();

                ContinueFile = TRUE;
                CreateNewDisk = FALSE;

                DPRINT(MAX_TRACE, ("First on new disk. CurrentIBufferSize (%d)  CurrentOBufferSize (%d).\n", 
                    CurrentIBufferSize, CurrentOBufferSize));

                if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0)) {
                    Status = WriteDataBlock();
                    if (Status != CAB_STATUS_SUCCESS)
                        return Status;
                }
            }
        } else {
            ContinueFile = FALSE;
            FileNode = FileNode->Next;
        }
    }

    if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0)) {
        /* A data block could span more than two
           disks if MaxDiskSize is very small */

        ASSERT(CreateNewDisk == FALSE);

        do {
            if (CreateNewDisk) {
                DPRINT(MID_TRACE, ("Creating new disk 2.\n"));
                CommitDisk(TRUE);
                CloseDisk();
                NewDisk();
                CreateNewDisk = FALSE;

                ASSERT(FileNode == FileListHead);
            }

            if ((CurrentIBufferSize > 0) || (CurrentOBufferSize > 0)) {
                Status = WriteDataBlock();
                if (Status != CAB_STATUS_SUCCESS)
                    return Status;
            }
        } while (CreateNewDisk);
    }
    CommitDisk(MoreDisks);

    return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::CommitDisk(DWORD MoreDisks)
/*
 * FUNCTION: Commits the current disk
 * ARGUMENTS:
 *     MoreDisks = TRUE if there is one or more disks after this disk
 * RETURNS:
 *     Status of operation
 */
{
    PCFFOLDER_NODE FolderNode;
    ULONG Status;

    OnCabinetName(CurrentDiskNumber, CabinetName);

    /* Create file, fail if it already exists */
    FileHandle = CreateFile(CabinetName, // Create this file
        GENERIC_WRITE,                   // Open for writing
        0,                               // No sharing
        NULL,                            // No security
        CREATE_NEW,                      // New file only
        FILE_ATTRIBUTE_NORMAL,           // Normal file
        NULL);                           // No attribute template
    if (FileHandle == INVALID_HANDLE_VALUE) {
        DWORD Status;
        /* If file exists, ask to overwrite file */
        if (((Status = GetLastError()) == ERROR_FILE_EXISTS) &&
            (OnOverwrite(NULL, CabinetName))) {

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
        } else {
            if (Status == ERROR_FILE_EXISTS)
                return CAB_STATUS_FILE_EXISTS;
            else
                return CAB_STATUS_CANNOT_CREATE;
        }
    }

    WriteCabinetHeader(MoreDisks);

    Status = WriteFolderEntries();
    if (Status != CAB_STATUS_SUCCESS)
        return Status;

    /* Write file entries */
    WriteFileEntries();

    /* Write data blocks */
    FolderNode = FolderListHead;
    while (FolderNode != NULL) {
        if (FolderNode->Commit) {
            Status = CommitDataBlocks(FolderNode);
            if (Status != CAB_STATUS_SUCCESS)
                return Status;
            /* Remove data blocks for folder */
            DestroyDataNodes(FolderNode);
        }
        FolderNode = FolderNode->Next;
    }

    CloseHandle(FileHandle);

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
    PCFFOLDER_NODE PrevNode;
    PCFFOLDER_NODE NextNode;
    ULONG Status;

    DestroyFileNodes();

    DestroyFolderNodes();

    if (InputBuffer) {
        HeapFree(GetProcessHeap(), 0, InputBuffer);
        InputBuffer = NULL;
    }

    if (OutputBuffer) {
        HeapFree(GetProcessHeap(), 0, OutputBuffer);
        OutputBuffer = NULL;
    }

    Close();

    if (ScratchFile) {
        Status = ScratchFile->Destroy();
        delete ScratchFile;
        return Status;
    }

    return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::AddFile(LPTSTR FileName)
/*
 * FUNCTION: Adds a file to the current disk
 * ARGUMENTS:
 *     FileName = Pointer to string with file name (full path)
 * RETURNS:
 *     Status of operation
 */
{
    HANDLE SrcFile;
    FILETIME FileTime;
    PCFFILE_NODE FileNode;

    FileNode = NewFileNode();
    if (!FileNode) {
        DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
        return CAB_STATUS_NOMEMORY;
    }

	FileNode->FolderNode = CurrentFolderNode;

    FileNode->FileName = (LPTSTR)HeapAlloc(GetProcessHeap(),
        0, lstrlen(FileName) + 1);
    lstrcpy(FileNode->FileName, FileName);

    /* Try to open file */
    SrcFile = CreateFile(
        FileNode->FileName,      // Open this file
        GENERIC_READ,            // Open for reading
        FILE_SHARE_READ,         // Share for reading
        NULL,                    // No security
        OPEN_EXISTING,           // File must exist
        FILE_ATTRIBUTE_NORMAL,   // Normal file 
        NULL);                   // No attribute template
    if (SrcFile == INVALID_HANDLE_VALUE) {
        DPRINT(MID_TRACE, ("File not found (%s).\n", FileNode->FileName));
        return CAB_STATUS_CANNOT_OPEN;
    }

    /* FIXME: Check for and handle large files (>= 2GB) */
    FileNode->File.FileSize = GetFileSize(SrcFile, NULL);
    if (GetLastError() != NO_ERROR) {
        DPRINT(MIN_TRACE, ("Cannot read from file.\n"));
        return CAB_STATUS_CANNOT_READ;
    }

    if (GetFileTime(SrcFile, NULL, &FileTime, NULL))
        FileTimeToDosDateTime(&FileTime,
            &FileNode->File.FileDate,
            &FileNode->File.FileTime);

    GetAttributesOnFile(FileNode);

    CloseHandle(SrcFile);

    return CAB_STATUS_SUCCESS;
}


VOID CCabinet::SetMaxDiskSize(DWORD Size)
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

BOOL CCabinet::OnOverwrite(PCFFILE File,
                           LPTSTR FileName)
/*
 * FUNCTION: Called when extracting a file and it already exists
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 * RETURNS
 *     TRUE if the file should be overwritten, FALSE if not
 */
{
    return FALSE;
}


VOID CCabinet::OnExtract(PCFFILE File,
                         LPTSTR FileName)
/*
 * FUNCTION: Called just before extracting a file
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
}


VOID CCabinet::OnDiskChange(LPTSTR CabinetName,
                            LPTSTR DiskLabel)
/*
 * FUNCTION: Called when a new disk is to be processed
 * ARGUMENTS:
 *     CabinetName = Pointer to buffer with name of cabinet
 *     DiskLabel   = Pointer to buffer with label of disk
 */
{
}


#ifndef CAB_READ_ONLY

VOID CCabinet::OnAdd(PCFFILE File,
                     LPTSTR FileName)
/*
 * FUNCTION: Called just before adding a file to a cabinet
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being added
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
}


BOOL CCabinet::OnDiskLabel(ULONG Number, LPTSTR Label)
/*
 * FUNCTION: Called when a disk needs a label
 * ARGUMENTS:
 *     Number = Cabinet number that needs a label
 *     Label  = Pointer to buffer to place label of disk
 * RETURNS:
 *     TRUE if a disk label was returned, FALSE if not
 */
{
    return FALSE;
}


BOOL CCabinet::OnCabinetName(ULONG Number, LPTSTR Name)
/*
 * FUNCTION: Called when a cabinet needs a name
 * ARGUMENTS:
 *     Number = Disk number that needs a name
 *     Name   = Pointer to buffer to place name of cabinet
 * RETURNS:
 *     TRUE if a cabinet name was returned, FALSE if not
 */
{
    return FALSE;
}

#endif /* CAB_READ_ONLY */

PCFFOLDER_NODE CCabinet::LocateFolderNode(DWORD Index)
/*
 * FUNCTION: Locates a folder node
 * ARGUMENTS:
 *     Index = Folder index
 * RETURNS:
 *     Pointer to folder node or NULL if the folder node was not found
 */
{
    PCFFOLDER_NODE Node;

    switch (Index) {
    case CAB_FILE_SPLIT:
        return FolderListTail;

    case CAB_FILE_CONTINUED:
    case CAB_FILE_PREV_NEXT:
        return FolderListHead;
    }

    Node = FolderListHead;
    while (Node != NULL) {
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
        (LPTSTR)File->FileName,
        (UINT)File->File.FileOffset,
        (UINT)File->File.FileSize));

    Node = CurrentFolderNode->DataListHead;
    while (Node != NULL) {

        DPRINT(MAX_TRACE, ("GetAbsoluteOffset(): Comparing (0x%X, 0x%X) (%d).\n",
            (UINT)Node->UncompOffset,
            (UINT)Node->UncompOffset + Node->Data.UncompSize,
            (UINT)Node->Data.UncompSize));

        /* Node->Data.UncompSize will be 0 if the block is split
           (ie. it is the last block in this cabinet) */
        if ((Node->Data.UncompSize == 0) ||
            ((File->File.FileOffset >= Node->UncompOffset) &&
            (File->File.FileOffset < Node->UncompOffset +
            Node->Data.UncompSize))) {
                File->DataBlock = Node;
                return CAB_STATUS_SUCCESS;
        }

        Node = Node->Next;
    }
    return CAB_STATUS_INVALID_CAB;
}


ULONG CCabinet::LocateFile(LPTSTR FileName,
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
    while (Node != NULL) {
        if (lstrcmpi(FileName, Node->FileName) == 0) {

            CurrentFolderNode = LocateFolderNode(Node->File.FileControlID);
            if (!CurrentFolderNode) {
                DPRINT(MID_TRACE, ("Folder with index number (%d) not found.\n",
                    (UINT)Node->File.FileControlID));
                return CAB_STATUS_INVALID_CAB;
            }

            if (Node->DataBlock == NULL) {
                Status = GetAbsoluteOffset(Node);
            } else
                Status = CAB_STATUS_SUCCESS;
            *File = Node;
            return Status;
        }
        Node = Node->Next;
    }
    return CAB_STATUS_NOFILE;
}


ULONG CCabinet::ReadString(LPTSTR String, DWORD MaxLength)
/*
 * FUNCTION: Reads a NULL-terminated string from the cabinet
 * ARGUMENTS:
 *     String    = Pointer to buffer to place string
 *     MaxLength = Maximum length of string
 * RETURNS:
 *     Status of operation
 */
{
    DWORD BytesRead;
    DWORD Offset;
    ULONG Status;
    DWORD Size;
    BOOL Found;

    Offset = 0;
    Found  = FALSE;
    do {
        Size = ((Offset + 32) <= MaxLength)? 32 : MaxLength - Offset;

        if (Size == 0) {
            DPRINT(MIN_TRACE, ("Too long a filename.\n"));
            return CAB_STATUS_INVALID_CAB;
        }

        Status = ReadBlock(&String[Offset], Size, &BytesRead);
        if ((Status != CAB_STATUS_SUCCESS) || (BytesRead != Size)) {
            DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
            return CAB_STATUS_INVALID_CAB;
        }

        for (Size = Offset; Size < Offset + BytesRead; Size++) {
            if (String[Size] == '\0') {
                Found = TRUE;
                break;
            }
        }

        Offset += BytesRead;

    } while (!Found);

    /* Back up some bytes */
    Size = (BytesRead - Size) - 1;
    SetFilePointer(FileHandle, -(LONG)Size, NULL, FILE_CURRENT);
    if (GetLastError() != NO_ERROR) {
        DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
        return CAB_STATUS_INVALID_CAB;
    }
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
    DWORD BytesRead;
    PCFFILE_NODE File;

    DPRINT(MAX_TRACE, ("Reading file table at absolute offset (0x%X).\n",
        CABHeader.FileTableOffset));

    /* Seek to file table */
    SetFilePointer(FileHandle, CABHeader.FileTableOffset, NULL, FILE_BEGIN);
    if (GetLastError() != NO_ERROR) {
        DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
        return CAB_STATUS_INVALID_CAB;
    }

    for (i = 0; i < CABHeader.FileCount; i++) {
        File = NewFileNode();
        if (!File) {
            DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
            return CAB_STATUS_NOMEMORY;
        }

        if ((Status = ReadBlock(&File->File, sizeof(CFFILE),
            &BytesRead)) != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
            return CAB_STATUS_INVALID_CAB;
        }

        File->FileName = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
        if (!File->FileName) {
            DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
            return CAB_STATUS_NOMEMORY;
        }

        /* Read file name */
        Status = ReadString(File->FileName, MAX_PATH);
        if (Status != CAB_STATUS_SUCCESS)
            return Status;

        DPRINT(MAX_TRACE, ("Found file '%s' at uncompressed offset (0x%X).  Size (%d bytes)  ControlId (0x%X).\n",
            (LPTSTR)File->FileName,
            (UINT)File->File.FileOffset,
            (UINT)File->File.FileSize,
            (UINT)File->File.FileControlID));

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
    DWORD AbsoluteOffset;
    DWORD UncompOffset;
    PCFDATA_NODE Node;
    DWORD BytesRead;
    ULONG Status;
    ULONG i;

    DPRINT(MAX_TRACE, ("Reading data blocks for folder (%d)  at absolute offset (0x%X).\n",
        FolderNode->Index, FolderNode->Folder.DataOffset));

    AbsoluteOffset = FolderNode->Folder.DataOffset;
    UncompOffset   = FolderNode->UncompOffset;

    for (i = 0; i < FolderNode->Folder.DataBlockCount; i++) {
        Node = NewDataNode(FolderNode);
        if (!Node) {
            DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
            return CAB_STATUS_NOMEMORY;
        }

        /* Seek to data block */
        SetFilePointer(FileHandle, AbsoluteOffset, NULL, FILE_BEGIN);
        if (GetLastError() != NO_ERROR) {
            DPRINT(MIN_TRACE, ("SetFilePointer() failed.\n"));
            return CAB_STATUS_INVALID_CAB;
        }

        if ((Status = ReadBlock(&Node->Data, sizeof(CFDATA),
            &BytesRead)) != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot read from file (%d).\n", (UINT)Status));
            return CAB_STATUS_INVALID_CAB;
        }

        DPRINT(MAX_TRACE, ("AbsOffset (0x%X)  UncompOffset (0x%X)  Checksum (0x%X)  CompSize (%d)  UncompSize (%d).\n",
            (UINT)AbsoluteOffset,
            (UINT)UncompOffset,
            (UINT)Node->Data.Checksum,
            (UINT)Node->Data.CompSize,
            (UINT)Node->Data.UncompSize));

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

    Node = (PCFFOLDER_NODE)HeapAlloc(GetProcessHeap(),
        0, sizeof(CFFOLDER_NODE));
    if (!Node)
        return NULL;

    ZeroMemory(Node, sizeof(CFFOLDER_NODE));

    Node->Folder.CompressionType = CAB_COMP_NONE;

    Node->Prev = FolderListTail;

    if (FolderListTail != NULL) {
        FolderListTail->Next = Node;
    } else {
        FolderListHead = Node;
    }
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

    Node = (PCFFILE_NODE)HeapAlloc(GetProcessHeap(),
        0, sizeof(CFFILE_NODE));
    if (!Node)
        return NULL;

    ZeroMemory(Node, sizeof(CFFILE_NODE));

    Node->Prev = FileListTail;

    if (FileListTail != NULL) {
        FileListTail->Next = Node;
    } else {
        FileListHead = Node;
    }
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

    Node = (PCFDATA_NODE)HeapAlloc(GetProcessHeap(),
        0, sizeof(CFDATA_NODE));
    if (!Node)
        return NULL;

    ZeroMemory(Node, sizeof(CFDATA_NODE));

    Node->Prev = FolderNode->DataListTail;

    if (FolderNode->DataListTail != NULL) {
        FolderNode->DataListTail->Next = Node;
    } else {
        FolderNode->DataListHead = Node;
    }
    FolderNode->DataListTail = Node;

    return Node;
}


VOID CCabinet::DestroyDataNodes(PCFFOLDER_NODE FolderNode)
/*
 * FUNCTION: Destroys data block nodes bound to a folder node
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
    PCFDATA_NODE PrevNode;
    PCFDATA_NODE NextNode;

    NextNode = FolderNode->DataListHead;
    while (NextNode != NULL) {
        PrevNode = NextNode->Next;
        HeapFree(GetProcessHeap(), 0, NextNode);
        NextNode = PrevNode;
    }
    FolderNode->DataListHead = NULL;
    FolderNode->DataListTail = NULL;
}


VOID CCabinet::DestroyFileNodes()
/*
 * FUNCTION: Destroys file nodes
 * ARGUMENTS:
 *     FolderNode = Pointer to folder node
 */
{
    PCFFILE_NODE PrevNode;
    PCFFILE_NODE NextNode;

    NextNode = FileListHead;
    while (NextNode != NULL) {
        PrevNode = NextNode->Next;
        if (NextNode->FileName)
            HeapFree(GetProcessHeap(), 0, NextNode->FileName);
        HeapFree(GetProcessHeap(), 0, NextNode);
        NextNode = PrevNode;
    }
    FileListHead = NULL;
    FileListTail = NULL;
}


VOID CCabinet::DestroyDeletedFileNodes()
/*
 * FUNCTION: Destroys file nodes that are marked for deletion
 */
{
    PCFFILE_NODE CurNode;
    PCFFILE_NODE NextNode;

    CurNode = FileListHead;
    while (CurNode != NULL) {
        NextNode = CurNode->Next;

        if (CurNode->Delete) {
            if (CurNode->Prev != NULL) {
                CurNode->Prev->Next = CurNode->Next;
            } else {
                FileListHead = CurNode->Next;
                if (FileListHead)
                    FileListHead->Prev = NULL;
            }

            if (CurNode->Next != NULL) {
                CurNode->Next->Prev = CurNode->Prev;
            } else {
                FileListTail = CurNode->Prev;
                if (FileListTail)
                    FileListTail->Next = NULL;
            }

            DPRINT(MAX_TRACE, ("Deleting file: '%s'\n", CurNode->FileName));

            TotalFileSize -= (sizeof(CFFILE) + lstrlen(GetFileName(CurNode->FileName)) + 1);

            if (CurNode->FileName)
                HeapFree(GetProcessHeap(), 0, CurNode->FileName);
            HeapFree(GetProcessHeap(), 0, CurNode);
        }
        CurNode = NextNode;
    }
}


VOID CCabinet::DestroyFolderNodes()
/*
 * FUNCTION: Destroys folder nodes
 */
{
    PCFFOLDER_NODE PrevNode;
    PCFFOLDER_NODE NextNode;

    NextNode = FolderListHead;
    while (NextNode != NULL) {
        PrevNode = NextNode->Next;
        DestroyDataNodes(NextNode);
        HeapFree(GetProcessHeap(), 0, NextNode);
        NextNode = PrevNode;
    }
    FolderListHead = NULL;
    FolderListTail = NULL;
}


VOID CCabinet::DestroyDeletedFolderNodes()
/*
 * FUNCTION: Destroys folder nodes that are marked for deletion
 */
{
    PCFFOLDER_NODE CurNode;
    PCFFOLDER_NODE NextNode;

    CurNode = FolderListHead;
    while (CurNode != NULL) {
        NextNode = CurNode->Next;

        if (CurNode->Delete) {
            if (CurNode->Prev != NULL) {
                CurNode->Prev->Next = CurNode->Next;
            } else {
                FolderListHead = CurNode->Next;
                if (FolderListHead)
                    FolderListHead->Prev = NULL;
            }

            if (CurNode->Next != NULL) {
                CurNode->Next->Prev = CurNode->Prev;
            } else {
                FolderListTail = CurNode->Prev;
                if (FolderListTail)
                    FolderListTail->Next = NULL;
            }

            DestroyDataNodes(CurNode);
            HeapFree(GetProcessHeap(), 0, CurNode);

            TotalFolderSize -= sizeof(CFFOLDER);
        }
        CurNode = NextNode;
    }
}


ULONG CCabinet::ComputeChecksum(PVOID Buffer,
                                UINT Size,
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
    INT UlongCount; // Number of ULONGs in block
    ULONG Checksum; // Checksum accumulator
    PBYTE pb;
    ULONG ul;

    /* FIXME: Doesn't seem to be correct. EXTRACT.EXE
       won't accept checksums computed by this routine */

    DPRINT(MIN_TRACE, ("Checksumming buffer (0x%X)  Size (%d)\n", (UINT)Buffer, Size));

    UlongCount = Size / 4;              // Number of ULONGs
    Checksum   = Seed;                  // Init checksum
    pb         = (PBYTE)Buffer;         // Start at front of data block

    /* Checksum integral multiple of ULONGs */
    while (UlongCount-- > 0) {
        /* NOTE: Build ULONG in big/little-endian independent manner */
        ul = *pb++;                     // Get low-order byte
        ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
        ul |= (((ULONG)(*pb++)) << 16); // Add 3nd byte
        ul |= (((ULONG)(*pb++)) << 24); // Add 4th byte

        Checksum ^= ul;                 // Update checksum
    }

    /* Checksum remainder bytes */
    ul = 0;
    switch (Size % 4) {
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


ULONG CCabinet::ReadBlock(PVOID Buffer,
                          DWORD Size,
                          PDWORD BytesRead)
/*
 * FUNCTION: Read a block of data from file
 * ARGUMENTS:
 *     Buffer    = Pointer to data buffer
 *     Size      = Length of data buffer
 *     BytesRead = Pointer to DWORD that on return will contain
 *                 number of bytes read
 * RETURNS:
 *     Status of operation
 */
{
    if (!ReadFile(FileHandle, Buffer, Size, BytesRead, NULL))
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
    PCFFOLDER_NODE FolderNode;
    PCFFILE_NODE FileNode;
    DWORD TotalSize;
    DWORD Size;

    CABHeader.FileTableOffset = 0;    // Not known yet
    CABHeader.FolderCount     = 0;    // Not known yet
    CABHeader.FileCount       = 0;    // Not known yet
    CABHeader.Flags           = 0;    // Not known yet

    CABHeader.CabinetNumber = CurrentDiskNumber;
    
    if ((CurrentDiskNumber > 0) && (OnCabinetName(PrevCabinetNumber, CabinetPrev))) {
        CABHeader.Flags |= CAB_FLAG_HASPREV;
        if (!OnDiskLabel(PrevCabinetNumber, DiskPrev))
            lstrcpy(CabinetPrev, "");
    }

    if (OnCabinetName(CurrentDiskNumber + 1, CabinetNext)) {
        CABHeader.Flags |= CAB_FLAG_HASNEXT;
        if (!OnDiskLabel(CurrentDiskNumber + 1, DiskNext))
            lstrcpy(DiskNext, "");
    }

    TotalSize = 0;

    if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0) {

        DPRINT(MAX_TRACE, ("CabinetPrev '%s'.\n", CabinetPrev));

        /* Calculate size of name of previous cabinet */
        TotalSize += lstrlen(CabinetPrev) + 1;

        /* Calculate size of label of previous disk */
        TotalSize += lstrlen(DiskPrev) + 1;
    }

    if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0) {

        DPRINT(MAX_TRACE, ("CabinetNext '%s'.\n", CabinetNext));

        /* Calculate size of name of next cabinet */
        Size = lstrlen(CabinetNext) + 1;
        TotalSize += Size;
        NextFieldsSize = Size;

        /* Calculate size of label of next disk */
        Size = lstrlen(DiskNext) + 1;
        TotalSize += Size;
        NextFieldsSize += Size;
    } else
        NextFieldsSize = 0;

    DiskSize += TotalSize;

    /* FIXME: Add cabinet reserved area size */
    TotalHeaderSize = sizeof(CFHEADER) + TotalSize;

    return CAB_STATUS_SUCCESS;
}


ULONG CCabinet::WriteCabinetHeader(BOOL MoreDisks)
/*
 * FUNCTION: Writes the cabinet header and optional fields
 * ARGUMENTS:
 *     MoreDisks = TRUE if next cabinet name should be included
 * RETURNS:
 *     Status of operation
 */
{
    PCFFOLDER_NODE FolderNode;
    PCFFILE_NODE FileNode;
    DWORD BytesWritten;
    DWORD Size;

    if (MoreDisks) {
        CABHeader.Flags |= CAB_FLAG_HASNEXT;
        Size = TotalHeaderSize;
    } else {
        CABHeader.Flags &= ~CAB_FLAG_HASNEXT;
        DiskSize -= NextFieldsSize;
        Size = TotalHeaderSize - NextFieldsSize;
    }

    /* Set absolute folder offsets */
    BytesWritten = Size + TotalFolderSize + TotalFileSize;
    CABHeader.FolderCount = 0;
    FolderNode = FolderListHead;
    while (FolderNode != NULL) {
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
    while (FileNode != NULL) {
        if (FileNode->Commit)
            CABHeader.FileCount++;
        FileNode = FileNode->Next;
    }

    CABHeader.CabinetSize = DiskSize;

    /* Write header */
    if (!WriteFile(FileHandle, &CABHeader, sizeof(CFHEADER), &BytesWritten, NULL)) {
        DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
        return CAB_STATUS_CANNOT_WRITE;
    }

    if ((CABHeader.Flags & CAB_FLAG_HASPREV) > 0) {

        DPRINT(MAX_TRACE, ("CabinetPrev '%s'.\n", CabinetPrev));

        /* Write name of previous cabinet */
        Size = lstrlen(CabinetPrev) + 1;
        if (!WriteFile(FileHandle, CabinetPrev, Size, &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }

        DPRINT(MAX_TRACE, ("DiskPrev '%s'.\n", DiskPrev));

        /* Write label of previous disk */
        Size = lstrlen(DiskPrev) + 1;
        if (!WriteFile(FileHandle, DiskPrev, Size, &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }
    }

    if ((CABHeader.Flags & CAB_FLAG_HASNEXT) > 0) {

        DPRINT(MAX_TRACE, ("CabinetNext '%s'.\n", CabinetNext));

        /* Write name of next cabinet */
        Size = lstrlen(CabinetNext) + 1;
        if (!WriteFile(FileHandle, CabinetNext, Size, &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }

        DPRINT(MAX_TRACE, ("DiskNext '%s'.\n", DiskNext));

        /* Write label of next disk */
        Size = lstrlen(DiskNext) + 1;
        if (!WriteFile(FileHandle, DiskNext, Size, &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }
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
    DWORD BytesWritten;

    DPRINT(MAX_TRACE, ("Writing folder table.\n"));

    FolderNode = FolderListHead;
    while (FolderNode != NULL) {
        if (FolderNode->Commit) {

            DPRINT(MAX_TRACE, ("Writing folder entry. CompressionType (0x%X)  DataBlockCount (%d)  DataOffset (0x%X).\n",
                FolderNode->Folder.CompressionType, FolderNode->Folder.DataBlockCount, FolderNode->Folder.DataOffset));

            if (!WriteFile(FileHandle,
                           &FolderNode->Folder,
                           sizeof(CFFOLDER),
                           &BytesWritten,
                           NULL)) {
                DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
                return CAB_STATUS_CANNOT_WRITE;
            }
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
    DWORD BytesWritten;
    BOOL SetCont;

    DPRINT(MAX_TRACE, ("Writing file table.\n"));

    File = FileListHead;
    while (File != NULL) {
        if (File->Commit) {
            /* Remove any continued files that ends in this disk */
            if (File->File.FileControlID == CAB_FILE_CONTINUED)
                File->Delete = TRUE;

            /* The file could end in the last (split) block and should therefore
               appear in the next disk too */
            
            if ((File->File.FileOffset + File->File.FileSize >= LastBlockStart) &&
                (File->File.FileControlID <= CAB_FILE_MAX_FOLDER) && (BlockIsSplit)) {
                File->File.FileControlID = CAB_FILE_SPLIT;
                File->Delete = FALSE;
                SetCont = TRUE;
            }

            DPRINT(MAX_TRACE, ("Writing file entry. FileControlID (0x%X)  FileOffset (0x%X)  FileSize (%d)  FileName (%s).\n",
                File->File.FileControlID, File->File.FileOffset, File->File.FileSize, File->FileName));

            if (!WriteFile(FileHandle,
                &File->File,
                sizeof(CFFILE),
                &BytesWritten,
                NULL))
                return CAB_STATUS_CANNOT_WRITE;

            if (!WriteFile(FileHandle,
                GetFileName(File->FileName),
                lstrlen(GetFileName(File->FileName)) + 1, &BytesWritten, NULL))
                return CAB_STATUS_CANNOT_WRITE;

            if (SetCont) {
                File->File.FileControlID = CAB_FILE_CONTINUED;
                SetCont = FALSE;
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
    DWORD BytesWritten;
    DWORD BytesRead;
    ULONG Status;

    DataNode = FolderNode->DataListHead;
    if (DataNode != NULL)
        Status = ScratchFile->Seek(DataNode->ScratchFilePosition);
    
    while (DataNode != NULL) {
        DPRINT(MAX_TRACE, ("Reading block at (0x%X)  CompSize (%d)  UncompSize (%d).\n",
            DataNode->ScratchFilePosition,
            DataNode->Data.CompSize,
            DataNode->Data.UncompSize));

        /* InputBuffer is free for us to use here, so we use it and avoid a
           memory allocation. OutputBuffer can't be used here because it may
           still contain valid data (if a data block spans two or more disks) */
        Status = ScratchFile->ReadBlock(&DataNode->Data, InputBuffer, &BytesRead);
        if (Status != CAB_STATUS_SUCCESS) {
            DPRINT(MIN_TRACE, ("Cannot read from scratch file (%d).\n", (UINT)Status));
            return Status;
        }

        if (!WriteFile(FileHandle, &DataNode->Data,
            sizeof(CFDATA), &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }

        if (!WriteFile(FileHandle, InputBuffer,
            DataNode->Data.CompSize, &BytesWritten, NULL)) {
            DPRINT(MIN_TRACE, ("Cannot write to file.\n"));
            return CAB_STATUS_CANNOT_WRITE;
        }

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
    DWORD BytesWritten;
    PCFDATA_NODE DataNode;

    if (!BlockIsSplit) {
        Status = Codec->Compress(OutputBuffer,
            InputBuffer,
            CurrentIBufferSize,
            &TotalCompSize);

        DPRINT(MAX_TRACE, ("Block compressed. CurrentIBufferSize (%d)  TotalCompSize(%d).\n",
            CurrentIBufferSize, TotalCompSize));

        CurrentOBuffer     = OutputBuffer;
        CurrentOBufferSize = TotalCompSize;
    }

    DataNode = NewDataNode(CurrentFolderNode);
    if (!DataNode) {
        DPRINT(MIN_TRACE, ("Insufficient memory.\n"));
        return CAB_STATUS_NOMEMORY;
    }

    DiskSize += sizeof(CFDATA);

    if (MaxDiskSize > 0)
        /* Disk size is limited */
        BlockIsSplit = (DiskSize + CurrentOBufferSize > MaxDiskSize);
    else
        BlockIsSplit = FALSE;

    if (BlockIsSplit) {
        DataNode->Data.CompSize   = (WORD)(MaxDiskSize - DiskSize);
        DataNode->Data.UncompSize = 0;
        CreateNewDisk = TRUE;
    } else {
        DataNode->Data.CompSize   = (WORD)CurrentOBufferSize;
        DataNode->Data.UncompSize = (WORD)CurrentIBufferSize;
    }

    DataNode->Data.Checksum = 0;
    DataNode->ScratchFilePosition = ScratchFile->Position();

    // FIXME: MAKECAB.EXE does not like this checksum algorithm
    //DataNode->Data.Checksum = ComputeChecksum(CurrentOBuffer, DataNode->Data.CompSize, 0);

    DPRINT(MAX_TRACE, ("Writing block. Checksum (0x%X)  CompSize (%d)  UncompSize (%d).\n",
        (UINT)DataNode->Data.Checksum,
        (UINT)DataNode->Data.CompSize,
        (UINT)DataNode->Data.UncompSize));

    Status = ScratchFile->WriteBlock(&DataNode->Data,
        CurrentOBuffer, &BytesWritten);
    if (Status != CAB_STATUS_SUCCESS)
        return Status;

    DiskSize += BytesWritten;

    CurrentFolderNode->TotalFolderSize += (BytesWritten + sizeof(CFDATA));
    CurrentFolderNode->Folder.DataBlockCount++;

    (PUCHAR)CurrentOBuffer += DataNode->Data.CompSize;
    CurrentOBufferSize     -= DataNode->Data.CompSize;

    LastBlockStart += DataNode->Data.UncompSize;

    if (!BlockIsSplit) {
        CurrentIBufferSize = 0;
        CurrentIBuffer     = InputBuffer;
    }

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
    DWORD Attributes;

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
    DWORD Attributes = 0;

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
}

#endif /* CAB_READ_ONLY */

/* EOF */
