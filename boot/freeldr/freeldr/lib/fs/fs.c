/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2008-2009  Hervé Poussineau  <hpoussin@reactos.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(FILESYSTEM);

/* GLOBALS ********************************************************************/

#define TAG_DEVICE_NAME 'NDsF'
#define TAG_DEVICE 'vDsF'

typedef struct tagDEVICE
{
    LIST_ENTRY ListEntry;
    const DEVVTBL* FuncTable;     ///< Driver function table.
    const DEVVTBL* FileFuncTable; ///< Used only by mounted storage devices.
    PSTR DeviceName;
    ULONG DeviceId; ///< Entry ID in FileData when the device gets referenced.
    ULONG ReferenceCount;
} DEVICE;

typedef struct tagFILEDATA
{
    ULONG DeviceId; ///< Parent device's ID.
    ULONG ReferenceCount;
    const DEVVTBL* FuncTable;
    PVOID Specific;
} FILEDATA;

static FILEDATA FileData[MAX_FDS];
static LIST_ENTRY DeviceListHead;

#define IS_VALID_FILEID(FileId) \
    ((ULONG)(FileId) < _countof(FileData) && FileData[(ULONG)(FileId)].FuncTable)

typedef const DEVVTBL* (*PFS_MOUNT)(ULONG DeviceId);

PFS_MOUNT FileSystems[] =
{
#ifndef _M_ARM
    IsoMount,
#endif
    FatMount,
    BtrFsMount,
#ifndef _M_ARM
    NtfsMount,
    Ext2Mount,
#endif
#if defined(_M_IX86) || defined(_M_AMD64)
#ifndef UEFIBOOT
    PxeMount,
#endif
#endif
};


/* DEBUGGING HELPERS **********************************************************/

#if DBG && 0
static VOID
DumpDeviceList(VOID)
{
    PLIST_ENTRY pEntry;

    DbgPrint("\n== Dumping ARC devices ==\n");

    for (pEntry = DeviceListHead.Flink;
         pEntry != &DeviceListHead;
         pEntry = pEntry->Flink)
    {
        DEVICE* pDevice = CONTAINING_RECORD(pEntry, DEVICE, ListEntry);
        ULONG DeviceId = pDevice->DeviceId;
        const DEVVTBL* FuncTable;

        DbgPrint("\n"
                 "DeviceId %lu\n"
                 "  RefCount: %lu\n"
                 "  DeviceName  : '%s'\n",
                 DeviceId, pDevice->ReferenceCount, pDevice->DeviceName);

        FuncTable = pDevice->FuncTable;
        DbgPrint("  DevFuncTable: 0x%p (In array: 0x%p) ; Name: '%S'\n",
                 FuncTable,
                 (DeviceId != INVALID_FILE_ID ? FileData[DeviceId].FuncTable : NULL),
                 (FuncTable && FuncTable->ServiceName)
                    ? FuncTable->ServiceName : L"n/a");

        FuncTable = pDevice->FileFuncTable;
        DbgPrint("  FileFuncTable: 0x%p ; Name: '%S'\n",
                 FuncTable,
                 (FuncTable && FuncTable->ServiceName)
                    ? FuncTable->ServiceName : L"n/a");
    }

    DbgPrint("\n== END Dumping ARC devices ==\n\n");
}

static VOID
DumpFileTable(VOID)
{
    ULONG i;

    DbgPrint("\n== Dumping ARC files table ==\n");

    for (i = 0; i < _countof(FileData); ++i)
    {
        FILEDATA* pFileData = &FileData[i];
        const DEVVTBL* FuncTable;

        /* Show only occupied slots */
        if (!pFileData->FuncTable)
            continue;

        DbgPrint("\n"
                 "FileId %lu\n"
                 "  RefCount: %lu\n"
                 "  ParentDeviceId: %lu\n"
                 /*"  FileName  : '%s'\n"*/
                 "  Specific: 0x%p\n",
                 i, pFileData->ReferenceCount,
                 pFileData->DeviceId, pFileData->Specific);

        FuncTable = pFileData->FuncTable;
        DbgPrint("  FuncTable: 0x%p ; Name: '%S'\n",
                 FuncTable,
                 (FuncTable && FuncTable->ServiceName)
                    ? FuncTable->ServiceName : L"n/a");
    }

    DbgPrint("\n== END Dumping ARC files table ==\n\n");
}
#endif // DBG


/* ARC FUNCTIONS **************************************************************/

/**
 * @brief
 * Replace "()" by "(0)" in the given ARC device name, if necessary.
 *
 * @param[in]   DeviceName
 * The original ARC device name string to normalize.
 *
 * @param[in,out]   Length
 * Points to a SIZE_T variable that:
 * - on input, specifies the length of the ARC device name string pointed
 *   by DeviceName;
 * - on output, receives the length of the normalized ARC device name,
 *   returned by value by this function.
 *
 * @return
 * - On success, returns a string pointer to the normalized ARC device name.
 *   If the device name string had to be normalized, this returned string is
 *   allocated from the temporary heap, with TAG_DEVICE_NAME tag.
 *   Otherwise, the function returns the original device name string pointer.
 * - On failure, the function returns NULL.
 **/
PCHAR
NormalizeArcDeviceName(
    _In_ PCCH DeviceName,
    _Inout_ PSIZE_T Length)
{
    ULONG Count;
    SIZE_T NameLength;
    PCCH p, End;
    PCHAR q, NormName;

    NameLength = *Length;
    End = DeviceName + NameLength;

    /* Count the number of "()", which needs to be replaced by "(0)" */
    Count = 0;
    for (p = DeviceName; p < End; ++p)
    {
        if ((p + 1) < End && *p == '(' && *(p + 1) == ')')
            ++Count; //, ++p;
    }

    if (Count == 0) /* No need to duplicate the device name */
        return (PCHAR)DeviceName;

    /* Return the updated length */
    *Length = NameLength + Count;

    /* Duplicate the device name and replace "()" by "(0)" */
    NormName = FrLdrTempAlloc(*Length, TAG_DEVICE_NAME);
    if (!NormName)
        return NULL;
    for (p = DeviceName, q = NormName; p < End; ++p)
    {
        *q++ = *p;
        if ((p + 1) < End && *p == '(' && *(p + 1) == ')')
            *q++ = '0'; //, *q++ = *++p;
    }
    return NormName;
}

ARC_STATUS ArcOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    ARC_STATUS Status;
    ULONG i;
    PLIST_ENTRY pEntry;
    DEVICE* pDevice;
    PCHAR DeviceName;
    PSTR FileName;
    SIZE_T Length;
    OPENMODE DeviceOpenMode;
    ULONG DeviceId;

    /* Print status message */
    TRACE("Opening file '%s'...\n", Path);

    *FileId = INVALID_FILE_ID;

    /* Search last ')', which delimits device and path */
    FileName = strrchr(Path, ')');
    if (!FileName)
        return EINVAL;
    ++FileName;

    /* Normalize the device name, replacing "()" by "(0)" if necessary */
    Length = FileName - Path;
    DeviceName = NormalizeArcDeviceName(Path, &Length);
    if (!DeviceName)
        return ENOMEM;

    if (OpenMode == OpenReadOnly || OpenMode == OpenWriteOnly)
        DeviceOpenMode = OpenMode;
    else
        DeviceOpenMode = OpenReadWrite;

    /* Search for the registered device */
    pDevice = NULL;
    for (pEntry = DeviceListHead.Flink;
         pEntry != &DeviceListHead;
         pEntry = pEntry->Flink)
    {
        pDevice = CONTAINING_RECORD(pEntry, DEVICE, ListEntry);
        if (strncmp(pDevice->DeviceName, DeviceName, Length) == 0)
            break;
    }

    /* Cleanup */
    if (DeviceName != Path)
        FrLdrTempFree(DeviceName, TAG_DEVICE_NAME);
    DeviceName = NULL;

    if (pEntry == &DeviceListHead)
        return ENODEV;

    /* OK, device found. Is it already opened? */
    if (pDevice->ReferenceCount == 0)
    {
        /* Find some room for the device */
        for (DeviceId = 0; ; ++DeviceId)
        {
            if (DeviceId >= _countof(FileData))
                return EMFILE;
            if (!FileData[DeviceId].FuncTable)
                break;
        }

        /* Try to open the device */
        FileData[DeviceId].ReferenceCount = 0;
        FileData[DeviceId].FuncTable = pDevice->FuncTable;
        Status = pDevice->FuncTable->Open(pDevice->DeviceName, DeviceOpenMode, &DeviceId);
        if (Status != ESUCCESS)
        {
            FileData[DeviceId].FuncTable = NULL;
            return Status;
        }
        pDevice->DeviceId = DeviceId;
    }
    else
    {
        /* Reuse the existing entry */
        DeviceId = pDevice->DeviceId;
        ASSERT(FileData[DeviceId].FuncTable == pDevice->FuncTable);
    }

    /* Done, increase the device reference count */
    pDevice->ReferenceCount++;
    FileData[DeviceId].ReferenceCount++;

    if (!*FileName)
    {
        /* The caller wanted to open the raw device: return its ID */
        *FileId = DeviceId;
        return ESUCCESS;
    }


    /*
     * We are opening a file.
     */

    /* Find some room for the file */
    for (i = 0; ; ++i)
    {
        if (i >= _countof(FileData))
        {
            Status = EMFILE;
            goto Done;
        }
        if (!FileData[i].FuncTable)
            break;
    }

    /* The device is accessed using filesystem semantics.
     * Try to detect the file system if not already done. */
    if (!pDevice->FileFuncTable && (pDevice->ReferenceCount <= 1))
    {
        for (ULONG fs = 0; fs < _countof(FileSystems); ++fs)
        {
            pDevice->FileFuncTable = FileSystems[fs](DeviceId);
            if (pDevice->FileFuncTable)
                break;
        }
    }
    if (!pDevice->FileFuncTable)
    {
        /* Error, unable to detect the file system */
        Status = ENOENT; // ENXIO;
        goto Done;
    }

    /*
     * At this point, the device is found and opened. Its file ID is stored
     * in DeviceId, and pDevice->FileFuncTable contains what needs to be
     * called to manipulate the file.
     */

    /* Open the file */
    FileData[i].DeviceId = DeviceId;
    FileData[i].ReferenceCount = 0;
    FileData[i].FuncTable = pDevice->FileFuncTable;
    *FileId = i;
    Status = FileData[i].FuncTable->Open(FileName, OpenMode, FileId);
    if (Status != ESUCCESS)
    {
        FileData[i].DeviceId = INVALID_FILE_ID;
        FileData[i].FuncTable = NULL;
        FileData[i].Specific = NULL;
        *FileId = INVALID_FILE_ID;
    }
    else
    {
        /* Reference the file */
        FileData[i].ReferenceCount++;
    }

Done:
    /* If we failed somewhere, dereference the device as well */
    if (Status != ESUCCESS)
    {
        // ArcClose(DeviceId);
        if (--FileData[DeviceId].ReferenceCount == 0)
        {
            (void)FileData[DeviceId].FuncTable->Close(DeviceId);
            FileData[DeviceId].DeviceId = INVALID_FILE_ID;
            FileData[DeviceId].FuncTable = NULL;
            FileData[DeviceId].Specific = NULL;
        }
        if (--pDevice->ReferenceCount == 0)
            pDevice->DeviceId = INVALID_FILE_ID;
    }

    return Status;
}

static DEVICE*
FsGetDeviceById(ULONG DeviceId)
{
    PLIST_ENTRY pEntry;

    for (pEntry = DeviceListHead.Flink;
         pEntry != &DeviceListHead;
         pEntry = pEntry->Flink)
    {
        DEVICE* pDevice = CONTAINING_RECORD(pEntry, DEVICE, ListEntry);
        if (pDevice->DeviceId == DeviceId)
            return pDevice;
    }
    return NULL;
}

ARC_STATUS
ArcClose(
    _In_ ULONG FileId)
{
    ULONG DeviceId;
    DEVICE* pDevice;

    if (!IS_VALID_FILEID(FileId))
        return EBADF;

    /* Retrieve the parent device's ID if any, for later */
    DeviceId = FileData[FileId].DeviceId;

    /* Dereference the file and close it if needed */
    ASSERT(FileData[FileId].ReferenceCount > 0);
    if (--FileData[FileId].ReferenceCount == 0)
    {
        (void)FileData[FileId].FuncTable->Close(FileId);
        FileData[FileId].DeviceId = INVALID_FILE_ID;
        FileData[FileId].FuncTable = NULL;
        FileData[FileId].Specific = NULL;
    }

    /* Check whether this file actually references a device */
    pDevice = FsGetDeviceById(FileId);
    if (pDevice)
    {
        /* It does, dereference it */
        ASSERT(pDevice->ReferenceCount > 0);
        if (--pDevice->ReferenceCount == 0)
            pDevice->DeviceId = INVALID_FILE_ID;
    }

    /* And dereference the parent device too, if there is one */
    if (IS_VALID_FILEID(DeviceId))
        ArcClose(DeviceId);

    return ESUCCESS;
}

ARC_STATUS ArcRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    if (!IS_VALID_FILEID(FileId))
        return EBADF;
    return FileData[FileId].FuncTable->Read(FileId, Buffer, N, Count);
}

ARC_STATUS ArcSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    if (!IS_VALID_FILEID(FileId))
        return EBADF;
    return FileData[FileId].FuncTable->Seek(FileId, Position, SeekMode);
}

ARC_STATUS ArcGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    if (!IS_VALID_FILEID(FileId))
        return EBADF;
    return FileData[FileId].FuncTable->GetFileInformation(FileId, Information);
}

/* FUNCTIONS ******************************************************************/

VOID FileSystemError(PCSTR ErrorString)
{
    ERR("%s\n", ErrorString);
    UiMessageBox(ErrorString);
}

ARC_STATUS
FsOpenFile(
    IN PCSTR FileName,
    IN PCSTR DefaultPath OPTIONAL,
    IN OPENMODE OpenMode,
    OUT PULONG FileId)
{
    NTSTATUS Status;
    SIZE_T cchPathLen;
    CHAR FullPath[MAX_PATH] = "";

    /*
     * Check whether FileName is a full path and if not, create a full
     * file name using the user-provided default path (if present).
     *
     * See ArcOpen(): Search last ')', which delimits device and path.
     */
    if (strrchr(FileName, ')') == NULL)
    {
        /* This is not a full path: prepend the user-provided default path */
        if (DefaultPath)
        {
            Status = RtlStringCbCopyA(FullPath, sizeof(FullPath), DefaultPath);
            if (!NT_SUCCESS(Status))
                return ENAMETOOLONG;
        }

        /* Append a path separator if needed */

        cchPathLen = min(sizeof(FullPath)/sizeof(CHAR), strlen(FullPath));
        if (cchPathLen >= sizeof(FullPath)/sizeof(CHAR))
            return ENAMETOOLONG;

        if ((*FileName != '\\' && *FileName != '/') &&
            cchPathLen > 0 && (FullPath[cchPathLen-1] != '\\' && FullPath[cchPathLen-1] != '/'))
        {
            /* FileName does not start with '\' and FullPath does not end with '\' */
            Status = RtlStringCbCatA(FullPath, sizeof(FullPath), "\\");
            if (!NT_SUCCESS(Status))
                return ENAMETOOLONG;
        }
        else if ((*FileName == '\\' || *FileName == '/') &&
                 cchPathLen > 0 && (FullPath[cchPathLen-1] == '\\' || FullPath[cchPathLen-1] == '/'))
        {
            /* FileName starts with '\' and FullPath ends with '\' */
            while (*FileName == '\\' || *FileName == '/')
                ++FileName; // Skip any backslash
        }
    }
    /* Append (or just copy) the remaining file name */
    Status = RtlStringCbCatA(FullPath, sizeof(FullPath), FileName);
    if (!NT_SUCCESS(Status))
        return ENAMETOOLONG;

    /* Open the file */
    return ArcOpen(FullPath, OpenMode, FileId);
}

/*
 * FsGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext).
 */
ULONG FsGetNumPathParts(PCSTR Path)
{
    size_t i;
    size_t len;
    ULONG  num;

    len = strlen(Path);

    for (i = 0, num = 0; i < len; i++)
    {
        if ((Path[i] == '\\') || (Path[i] == '/'))
        {
            num++;
        }
    }
    num++;

    TRACE("FsGetNumPathParts() Path = %s NumPathParts = %d\n", Path, num);

    return num;
}

/*
 * FsGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure.
 */
VOID FsGetFirstNameFromPath(PCHAR Buffer, PCSTR Path)
{
    size_t i;
    size_t len;

    len = strlen(Path);

    // Copy all the characters up to the end of the
    // string or until we hit a '\' character
    // and put them in Buffer
    for (i = 0; i < len; i++)
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

    TRACE("FsGetFirstNameFromPath() Path = %s FirstName = %s\n", Path, Buffer);
}

VOID
FsRegisterDevice(
    _In_ PCSTR DeviceName,
    _In_ const DEVVTBL* FuncTable)
{
    DEVICE* pNewEntry;
    SIZE_T Length;

    TRACE("FsRegisterDevice(%s)\n", DeviceName);

    Length = strlen(DeviceName) + 1;
    pNewEntry = FrLdrTempAlloc(sizeof(DEVICE) + Length, TAG_DEVICE);
    if (!pNewEntry)
        return;
    pNewEntry->FuncTable = FuncTable;
    pNewEntry->DeviceId = INVALID_FILE_ID;
    pNewEntry->ReferenceCount = 0;
    pNewEntry->DeviceName = (PSTR)(pNewEntry + 1);
    RtlCopyMemory(pNewEntry->DeviceName, DeviceName, Length);

    InsertHeadList(&DeviceListHead, &pNewEntry->ListEntry);
}

PCWSTR FsGetServiceName(ULONG FileId)
{
    if (!IS_VALID_FILEID(FileId))
        return NULL;
    return FileData[FileId].FuncTable->ServiceName;
}

VOID FsSetDeviceSpecific(ULONG FileId, PVOID Specific)
{
    if (!IS_VALID_FILEID(FileId))
        return;
    FileData[FileId].Specific = Specific;
}

PVOID FsGetDeviceSpecific(ULONG FileId)
{
    if (!IS_VALID_FILEID(FileId))
        return NULL;
    return FileData[FileId].Specific;
}

ULONG FsGetDeviceId(ULONG FileId)
{
    if (FileId >= _countof(FileData)) // !IS_VALID_FILEID(FileId)
        return INVALID_FILE_ID;
    return FileData[FileId].DeviceId;
}

VOID FsInit(VOID)
{
    ULONG i;

    RtlZeroMemory(FileData, sizeof(FileData));
    for (i = 0; i < _countof(FileData); ++i)
        FileData[i].DeviceId = INVALID_FILE_ID;

    InitializeListHead(&DeviceListHead);
}
