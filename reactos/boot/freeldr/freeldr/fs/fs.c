/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////////////////////

const FS_VTBL* pFSVtbl = NULL; // Type of filesystem on boot device, set by FsOpenVolume()
PVOID FsStaticBufferDisk = 0, FsStaticBufferData = 0;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID FileSystemError(PCSTR ErrorString)
{
	DPRINTM(DPRINT_FILESYSTEM, "%s\n", ErrorString);

	UiMessageBox(ErrorString);
}

/*
 *
 * BOOLEAN FsOpenVolume(ULONG DriveNumber, ULONGLONG StartSector, ULONGLONG SectorCount, int Type);
 *
 * This function is called to open a disk volume for file access.
 * It must be called before any of the file functions will work.
 *
 */
static BOOLEAN FsOpenVolume(ULONG DriveNumber, ULONGLONG StartSector, ULONGLONG SectorCount, int Type)
{
	CHAR ErrorText[80];

	if( !FsStaticBufferDisk )
		FsStaticBufferDisk = MmAllocateMemory( 0x20000 );
	if( !FsStaticBufferDisk )
	{
		FileSystemError("could not allocate filesystem static buffer");
		return FALSE;
	}
	FsStaticBufferData = ((PCHAR)FsStaticBufferDisk) + 0x10000;

	switch (Type)
	{
	case FS_FAT:
		pFSVtbl = &FatVtbl;
		break;
	case FS_NTFS:
		pFSVtbl = &NtfsVtbl;
		break;
	case FS_EXT2:
		pFSVtbl = &Ext2Vtbl;
		break;
	case FS_ISO9660:
		pFSVtbl = &Iso9660Vtbl;
		break;
	default:
		pFSVtbl = NULL;
		break;
	}

	if (pFSVtbl && pFSVtbl->OpenVolume)
	{
		return (*pFSVtbl->OpenVolume)(DriveNumber, StartSector, SectorCount);
	}
	else
	{
		sprintf(ErrorText, "Unsupported file system. Type: 0x%x", Type);
		FileSystemError(ErrorText);
	}

	return FALSE;
}
/*
 *
 * BOOLEAN FsOpenBootVolume()
 *
 * This function is called to open the boot disk volume for file access.
 * It must be called before any of the file functions will work.
 */
BOOLEAN FsOpenBootVolume()
{
	ULONG DriveNumber;
	ULONGLONG StartSector;
	ULONGLONG SectorCount;
	int Type;

	if (! MachDiskGetBootVolume(&DriveNumber, &StartSector, &SectorCount, &Type))
	{
		FileSystemError("Unable to locate boot partition\n");
		return FALSE;
	}

	return FsOpenVolume(DriveNumber, StartSector, SectorCount, Type);
}

BOOLEAN FsOpenSystemVolume(char *SystemPath, char *RemainingPath, PULONG Device)
{
	ULONG DriveNumber;
	ULONGLONG StartSector;
	ULONGLONG SectorCount;
	int Type;

	if (! MachDiskGetSystemVolume(SystemPath, RemainingPath, Device,
	                              &DriveNumber, &StartSector, &SectorCount,
	                              &Type))
	{
		FileSystemError("Unable to locate system partition\n");
		return FALSE;
	}

	return FsOpenVolume(DriveNumber, StartSector, SectorCount, Type);
}


PFILE FsOpenFile(PCSTR FileName)
{
	PFILE	FileHandle = NULL;

	//
	// Print status message
	//
	DPRINTM(DPRINT_FILESYSTEM, "Opening file '%s'...\n", FileName);

	//
	// Check and see if the first character is '\' or '/' and remove it if so
	//
	while ((*FileName == '\\') || (*FileName == '/'))
	{
		FileName++;
	}

	//
	// Check file system type and pass off to appropriate handler
	//
	if (pFSVtbl && pFSVtbl->OpenFile)
	{
		FileHandle = pFSVtbl->OpenFile(FileName);
	}
	else
	{
		FileSystemError("Error: Unknown filesystem.");
	}

	//
	// Check return value
	//
	if (FileHandle != NULL)
	{
		DPRINTM(DPRINT_FILESYSTEM, "FsOpenFile() succeeded. FileHandle: 0x%x\n", FileHandle);
	}
	else
	{
		DPRINTM(DPRINT_FILESYSTEM, "FsOpenFile() failed.\n");
	}

	return FileHandle;
}

VOID FsCloseFile(PFILE FileHandle)
{
	if (pFSVtbl)
	{
		if (pFSVtbl->CloseFile)
			(*pFSVtbl->CloseFile)(FileHandle);
	}
	else
	{
		FileSystemError("Error: Unknown filesystem.");
	}
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOLEAN FsReadFile(PFILE FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
	//
	// Set the number of bytes read equal to zero
	//
	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	if (pFSVtbl && pFSVtbl->ReadFile)
	{
		return (*pFSVtbl->ReadFile)(FileHandle, BytesToRead, BytesRead, Buffer);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return FALSE;
	}
}

ULONG FsGetFileSize(PFILE FileHandle)
{
	if (pFSVtbl && pFSVtbl->GetFileSize)
	{
		return (*pFSVtbl->GetFileSize)(FileHandle);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return 0;
	}
}

VOID FsSetFilePointer(PFILE FileHandle, ULONG NewFilePointer)
{
	if (pFSVtbl && pFSVtbl->SetFilePointer)
	{
		(*pFSVtbl->SetFilePointer)(FileHandle, NewFilePointer);
	}
	else
	{
		FileSystemError("Unknown file system.");
	}
}

ULONG FsGetFilePointer(PFILE FileHandle)
{
	if (pFSVtbl && pFSVtbl->SetFilePointer)
	{
		return (*pFSVtbl->GetFilePointer)(FileHandle);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return 0;
	}
}

BOOLEAN FsIsEndOfFile(PFILE FileHandle)
{
	if (FsGetFilePointer(FileHandle) >= FsGetFileSize(FileHandle))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
 * FsGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext)
 */
ULONG FsGetNumPathParts(PCSTR Path)
{
	size_t		i;
	ULONG		num;

	for (i=0,num=0; i<strlen(Path); i++)
	{
		if ((Path[i] == '\\') || (Path[i] == '/'))
		{
			num++;
		}
	}
	num++;

	DPRINTM(DPRINT_FILESYSTEM, "FatGetNumPathParts() Path = %s NumPathParts = %d\n", Path, num);

	return num;
}

/*
 * FsGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure
 */
VOID FsGetFirstNameFromPath(PCHAR Buffer, PCSTR Path)
{
	size_t		i;

	// Copy all the characters up to the end of the
	// string or until we hit a '\' character
	// and put them in Buffer
	for (i=0; i<strlen(Path); i++)
	{
		if ((Path[i] == '\\') || (Path[i] == '/'))
		{
			break;
		}
		else
		{
			Buffer[i] = Path[i];
		}
	}

	Buffer[i] = 0;

	DPRINTM(DPRINT_FILESYSTEM, "FatGetFirstNameFromPath() Path = %s FirstName = %s\n", Path, Buffer);
}

LONG CompatFsClose(ULONG FileId)
{
    PFILE FileHandle = FsGetDeviceSpecific(FileId);

    FsCloseFile(FileHandle);
    return ESUCCESS;
}

LONG CompatFsGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PFILE FileHandle = FsGetDeviceSpecific(FileId);

    memset(Information, 0, sizeof(FILEINFORMATION));
    Information->EndingAddress.LowPart = FsGetFileSize(FileHandle);
    Information->CurrentAddress.LowPart = FsGetFilePointer(FileHandle);
    return ESUCCESS;
}

LONG CompatFsOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PFILE FileHandle;
    static BOOLEAN bVolumeOpened = FALSE;

    if (!bVolumeOpened)
    {
        bVolumeOpened = FsOpenBootVolume();
        if (!bVolumeOpened)
            return EIO;
    }

    FileHandle = FsOpenFile(Path);
    if (!FileHandle)
        return EIO;
    FsSetDeviceSpecific(*FileId, FileHandle);
    return ESUCCESS;
}

LONG CompatFsRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PFILE FileHandle = FsGetDeviceSpecific(FileId);
    BOOLEAN ret;

    ret = FsReadFile(FileHandle, N, Count, Buffer);
    return (ret ? ESUCCESS : EFAULT);
}

LONG CompatFsSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PFILE FileHandle = FsGetDeviceSpecific(FileId);

    if (SeekMode != SeekAbsolute)
        return EINVAL;

    FsSetFilePointer(FileHandle, Position->LowPart);
    return ESUCCESS;
}

const DEVVTBL CompatFsFuncTable = {
    CompatFsClose,
    CompatFsGetFileInformation,
    CompatFsOpen,
    CompatFsRead,
    CompatFsSeek,
};

#define MAX_FDS 20
typedef struct tagFILEDATA
{
    ULONG DeviceId;
    ULONG ReferenceCount;
    const DEVVTBL* FuncTable;
    const DEVVTBL* FileFuncTable;
    VOID* Specific;
} FILEDATA;

typedef struct tagDEVICE
{
    LIST_ENTRY ListEntry;
    const DEVVTBL* FuncTable;
    CHAR* Prefix;
    ULONG DeviceId;
    ULONG ReferenceCount;
} DEVICE;

static FILEDATA FileData[MAX_FDS];
static LIST_ENTRY DeviceListHead;

LONG ArcClose(ULONG FileId)
{
    LONG ret;

    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;

    ret = FileData[FileId].FuncTable->Close(FileId);

    if (ret == ESUCCESS)
    {
        FileData[FileId].FuncTable = NULL;
        FileData[FileId].Specific = NULL;
    }
    return ret;
}

LONG ArcGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->GetFileInformation(FileId, Information);
}

LONG ArcOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    ULONG i, ret;
    PLIST_ENTRY pEntry;
    DEVICE* pDevice;
    CHAR* DeviceName;
    CHAR* FileName;
    CHAR* p;
    CHAR* q;
    ULONG dwCount, dwLength;
    OPENMODE DeviceOpenMode;
    ULONG DeviceId;

    *FileId = MAX_FDS;

    /* Search last ')', which delimits device and path */
    FileName = strrchr(Path, ')');
    if (!FileName)
        return EINVAL;
    FileName++;

    /* Count number of "()", which needs to be replaced by "(0)" */
    dwCount = 0;
    for (p = Path; p != FileName; p++)
        if (*p == '(' && *(p + 1) == ')')
            dwCount++;

    /* Duplicate device name, and replace "()" by "(0)" (if required) */
    dwLength = FileName - Path + dwCount;
    if (dwCount != 0)
    {
        DeviceName = MmHeapAlloc(FileName - Path + dwCount);
        if (!DeviceName)
            return ENOMEM;
        for (p = Path, q = DeviceName; p != FileName; p++)
        {
            *q++ = *p;
            if (*p == '(' && *(p + 1) == ')')
                *q++ = '0';
        }
    }
    else
        DeviceName = Path;

    /* Search for the device */
    pEntry = DeviceListHead.Flink;
    if (OpenMode == OpenReadOnly || OpenMode == OpenWriteOnly)
        DeviceOpenMode = OpenMode;
    else
        DeviceOpenMode = OpenReadWrite;
    while (pEntry != &DeviceListHead)
    {
        pDevice = CONTAINING_RECORD(pEntry, DEVICE, ListEntry);
        if (strncmp(pDevice->Prefix, DeviceName, dwLength) == 0)
        {
            /* OK, device found. It is already opened? */
            if (pDevice->ReferenceCount == 0)
            {
                /* Search some room for the device */
                for (DeviceId = 0; DeviceId < MAX_FDS; DeviceId++)
                    if (!FileData[DeviceId].FuncTable)
                        break;
                if (DeviceId == MAX_FDS)
                    return EMFILE;
                /* Try to open the device */
                FileData[DeviceId].FuncTable = pDevice->FuncTable;
                ret = pDevice->FuncTable->Open(pDevice->Prefix, DeviceOpenMode, &DeviceId);
                if (ret != ESUCCESS)
                {
                    FileData[DeviceId].FuncTable = NULL;
                    return ret;
                }

                /* Try to detect the file system */
                /* FIXME: we link there to old infrastructure... */
                FileData[DeviceId].FileFuncTable = &CompatFsFuncTable;

                pDevice->DeviceId = DeviceId;
            }
            else
            {
                DeviceId = pDevice->DeviceId;
            }
            pDevice->ReferenceCount++;
            break;
        }
        pEntry = pEntry->Flink;
    }
    if (pEntry == &DeviceListHead)
        return ENODEV;

    /* At this point, device is found and opened. Its file id is stored
     * in DeviceId, and FileData[DeviceId].FileFuncTable contains what
     * needs to be called to open the file */

    /* Search some room for the device */
    for (i = 0; i < MAX_FDS; i++)
        if (!FileData[i].FuncTable)
            break;
    if (i == MAX_FDS)
        return EMFILE;

    /* Open the file */
    FileData[i].FuncTable = FileData[DeviceId].FileFuncTable;
    *FileId = i;
    ret = FileData[i].FuncTable->Open(FileName, OpenMode, FileId);
    if (ret != ESUCCESS)
    {
        FileData[i].FuncTable = NULL;
        *FileId = MAX_FDS;
    }
    return ret;
}

LONG ArcRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->Read(FileId, Buffer, N, Count);
}

LONG ArcSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->Seek(FileId, Position, SeekMode);
}

VOID FsRegisterDevice(CHAR* Prefix, const DEVVTBL* FuncTable)
{
    DEVICE* pNewEntry;
    ULONG dwLength;

    dwLength = strlen(Prefix) + 1;
    pNewEntry = MmHeapAlloc(sizeof(DEVICE) + dwLength);
    if (!pNewEntry)
        return;
    pNewEntry->FuncTable = FuncTable;
    pNewEntry->ReferenceCount = 0;
    pNewEntry->Prefix = (CHAR*)(pNewEntry + 1);
    memcpy(pNewEntry->Prefix, Prefix, dwLength);

    InsertHeadList(&DeviceListHead, &pNewEntry->ListEntry);
}

VOID FsSetDeviceSpecific(ULONG FileId, VOID* Specific)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return;
    FileData[FileId].Specific = Specific;
}

VOID* FsGetDeviceSpecific(ULONG FileId)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return NULL;
    return FileData[FileId].Specific;
}

VOID FsInit(VOID)
{
    memset(FileData, 0, sizeof(FileData));
    InitializeListHead(&DeviceListHead);
}
