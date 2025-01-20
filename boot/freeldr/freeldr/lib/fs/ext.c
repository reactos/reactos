/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2024-2025  Daniel Victor <ilauncherdeveloper@gmail.com>
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

#ifndef _M_ARM
#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(FILESYSTEM);

BOOLEAN    ExtOpenVolume(PEXT_VOLUME_INFO Volume);
PEXT_FILE_INFO    ExtOpenFile(PEXT_VOLUME_INFO Volume, PCSTR FileName);
BOOLEAN    ExtLookupFile(PEXT_VOLUME_INFO Volume, PCSTR FileName, PEXT_FILE_INFO ExtFileInfo);
BOOLEAN    ExtSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PEXT_DIR_ENTRY DirectoryEntry);
BOOLEAN    ExtReadVolumeSectors(PEXT_VOLUME_INFO Volume, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);

BOOLEAN    ExtReadFileBig(PEXT_FILE_INFO ExtFileInfo, ULONGLONG BytesToRead, ULONGLONG* BytesRead, PVOID Buffer);
BOOLEAN    ExtReadSuperBlock(PEXT_VOLUME_INFO Volume);
BOOLEAN    ExtReadGroupDescriptors(PEXT_VOLUME_INFO Volume);
BOOLEAN    ExtReadDirectory(PEXT_VOLUME_INFO Volume, ULONG Inode, PVOID* DirectoryBuffer, PEXT_INODE InodePointer);
BOOLEAN    ExtReadBlock(PEXT_VOLUME_INFO Volume, ULONG BlockNumber, PVOID Buffer);
BOOLEAN    ExtReadPartialBlock(PEXT_VOLUME_INFO Volume, ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOLEAN    ExtReadInode(PEXT_VOLUME_INFO Volume, ULONG Inode, PEXT_INODE InodeBuffer);
BOOLEAN    ExtReadGroupDescriptor(PEXT_VOLUME_INFO Volume, ULONG Group, PEXT_GROUP_DESC GroupBuffer);
ULONG*    ExtReadBlockPointerList(PEXT_VOLUME_INFO Volume, PEXT_INODE Inode);
ULONGLONG        ExtGetInodeFileSize(PEXT_INODE Inode);
BOOLEAN ExtCopyBlockPointersByExtents(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, PEXT4_EXTENT_HEADER ExtentHeader);
BOOLEAN    ExtCopyIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock);
BOOLEAN    ExtCopyDoubleIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock);
BOOLEAN    ExtCopyTripleIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock);

typedef struct _EXT_VOLUME_INFO
{
    ULONG BytesPerSector;  // Usually 512...

    PEXT_SUPER_BLOCK SuperBlock;       // Ext file system super block
    PEXT_GROUP_DESC  GroupDescriptors; // Ext file system group descriptors

    ULONG BlockSizeInBytes;         // Block size in bytes
    ULONG BlockSizeInSectors;       // Block size in sectors
    ULONG FragmentSizeInBytes;      // Fragment size in bytes
    ULONG FragmentSizeInSectors;    // Fragment size in sectors
    ULONG InodeSizeInBytes;         // Inode size in bytes
    ULONG GroupDescSizeInBytes;     // Group descriptor size in bytes
    ULONG GroupCount;               // Number of groups in this file system
    ULONG InodesPerBlock;           // Number of inodes in one block
    ULONG GroupDescPerBlock;        // Number of group descriptors in one block

    ULONG DeviceId; // Ext file system device ID

} EXT_VOLUME_INFO;

PEXT_VOLUME_INFO ExtVolumes[MAX_FDS];

#define TAG_EXT_BLOCK_LIST 'LtxE'
#define TAG_EXT_FILE 'FtxE'
#define TAG_EXT_BUFFER  'BtxE'
#define TAG_EXT_SUPER_BLOCK 'StxE'
#define TAG_EXT_GROUP_DESC 'GtxE'
#define TAG_EXT_VOLUME 'VtxE'

BOOLEAN ExtOpenVolume(PEXT_VOLUME_INFO Volume)
{
    TRACE("ExtOpenVolume() DeviceId = %d\n", Volume->DeviceId);

#if 0
    /* Initialize the disk cache for this drive */
    if (!CacheInitializeDrive(DriveNumber))
    {
        return FALSE;
    }
#endif
    Volume->BytesPerSector = SECTOR_SIZE;

    /* Read in the super block */
    if (!ExtReadSuperBlock(Volume))
        return FALSE;

    /* Read in the group descriptors */
    if (!ExtReadGroupDescriptors(Volume))
        return FALSE;

    return TRUE;
}

/*
 * ExtOpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
PEXT_FILE_INFO ExtOpenFile(PEXT_VOLUME_INFO Volume, PCSTR FileName)
{
    EXT_FILE_INFO        TempExtFileInfo;
    PEXT_FILE_INFO        FileHandle;
    CHAR            SymLinkPath[EXT_DIR_ENTRY_MAX_NAME_LENGTH];
    CHAR            FullPath[EXT_DIR_ENTRY_MAX_NAME_LENGTH * 2];
    ULONG_PTR        Index;

    TRACE("ExtOpenFile() FileName = \"%s\"\n", FileName);

    RtlZeroMemory(SymLinkPath, sizeof(SymLinkPath));

    // Lookup the file in the file system
    if (!ExtLookupFile(Volume, FileName, &TempExtFileInfo))
    {
        return NULL;
    }

    // If we got a symbolic link then fix up the path
    // and re-call this function
    if ((TempExtFileInfo.Inode.Mode & EXT_S_IFMT) == EXT_S_IFLNK)
    {
        TRACE("File is a symbolic link\n");

        // Now read in the symbolic link path
        if (!ExtReadFileBig(&TempExtFileInfo, TempExtFileInfo.FileSize, NULL, SymLinkPath))
        {
            if (TempExtFileInfo.FileBlockList != NULL)
            {
                FrLdrTempFree(TempExtFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
            }

            return NULL;
        }

        TRACE("Symbolic link path = \"%s\"\n", SymLinkPath);

        // Get the full path
        if (SymLinkPath[0] == '/' || SymLinkPath[0] == '\\')
        {
            // Symbolic link is an absolute path
            // So copy it to FullPath, but skip over
            // the '/' character at the beginning
            strcpy(FullPath, &SymLinkPath[1]);
        }
        else
        {
            // Symbolic link is a relative path
            // Copy the first part of the path
            strcpy(FullPath, FileName);

            // Remove the last part of the path
            for (Index=strlen(FullPath); Index>0; )
            {
                Index--;
                if (FullPath[Index] == '/' || FullPath[Index] == '\\')
                {
                    break;
                }
            }
            FullPath[Index] = '\0';

            // Concatenate the symbolic link
            strcat(FullPath, Index == 0 ? "" : "/");
            strcat(FullPath, SymLinkPath);
        }

        TRACE("Full file path = \"%s\"\n", FullPath);

        if (TempExtFileInfo.FileBlockList != NULL)
        {
            FrLdrTempFree(TempExtFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
        }

        return ExtOpenFile(Volume, FullPath);
    }
    else
    {
        FileHandle = FrLdrTempAlloc(sizeof(EXT_FILE_INFO), TAG_EXT_FILE);
        if (FileHandle == NULL)
        {
            if (TempExtFileInfo.FileBlockList != NULL)
            {
                FrLdrTempFree(TempExtFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
            }

            return NULL;
        }

        RtlCopyMemory(FileHandle, &TempExtFileInfo, sizeof(EXT_FILE_INFO));

        return FileHandle;
    }
}

/*
 * ExtLookupFile()
 * This function searches the file system for the
 * specified filename and fills in a EXT_FILE_INFO structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
BOOLEAN ExtLookupFile(PEXT_VOLUME_INFO Volume, PCSTR FileName, PEXT_FILE_INFO ExtFileInfo)
{
    UINT32        i;
    ULONG        NumberOfPathParts;
    CHAR        PathPart[261];
    PVOID        DirectoryBuffer;
    ULONG        DirectoryInode = EXT_ROOT_INODE;
    EXT_INODE    InodeData;
    EXT_DIR_ENTRY    DirectoryEntry;

    TRACE("ExtLookupFile() FileName = \"%s\"\n", FileName);

    RtlZeroMemory(ExtFileInfo, sizeof(EXT_FILE_INFO));

    /* Skip leading path separator, if any */
    if (*FileName == '\\' || *FileName == '/')
        ++FileName;
    //
    // Figure out how many sub-directories we are nested in
    //
    NumberOfPathParts = FsGetNumPathParts(FileName);

    //
    // Loop once for each part
    //
    for (i=0; i<NumberOfPathParts; i++)
    {
        //
        // Get first path part
        //
        FsGetFirstNameFromPath(PathPart, FileName);

        //
        // Advance to the next part of the path
        //
        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
        {
        }
        FileName++;

        //
        // Buffer the directory contents
        //
        if (!ExtReadDirectory(Volume, DirectoryInode, &DirectoryBuffer, &InodeData))
        {
            return FALSE;
        }

        //
        // Search for file name in directory
        //
        if (!ExtSearchDirectoryBufferForFile(DirectoryBuffer, (ULONG)ExtGetInodeFileSize(&InodeData), PathPart, &DirectoryEntry))
        {
            FrLdrTempFree(DirectoryBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }

        FrLdrTempFree(DirectoryBuffer, TAG_EXT_BUFFER);

        DirectoryInode = DirectoryEntry.Inode;
    }

    if (!ExtReadInode(Volume, DirectoryInode, &InodeData))
    {
        return FALSE;
    }

    if (((InodeData.Mode & EXT_S_IFMT) != EXT_S_IFREG) &&
        ((InodeData.Mode & EXT_S_IFMT) != EXT_S_IFLNK))
    {
        FileSystemError("Inode is not a regular file or symbolic link.");
        return FALSE;
    }

    // Set the associated volume
    ExtFileInfo->Volume = Volume;

    // If it's a regular file or a regular symbolic link
    // then get the block pointer list otherwise it must
    // be a fast symbolic link which doesn't have a block list
    if (((InodeData.Mode & EXT_S_IFMT) == EXT_S_IFREG) ||
        ((InodeData.Mode & EXT_S_IFMT) == EXT_S_IFLNK && InodeData.Size > FAST_SYMLINK_MAX_NAME_SIZE))
    {
        ExtFileInfo->FileBlockList = ExtReadBlockPointerList(Volume, &InodeData);
        if (ExtFileInfo->FileBlockList == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        ExtFileInfo->FileBlockList = NULL;
    }

    ExtFileInfo->FilePointer = 0;
    ExtFileInfo->FileSize = ExtGetInodeFileSize(&InodeData);
    RtlCopyMemory(&ExtFileInfo->Inode, &InodeData, sizeof(EXT_INODE));

    return TRUE;
}

BOOLEAN ExtSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PEXT_DIR_ENTRY DirectoryEntry)
{
    ULONG           CurrentOffset = 0;
    PEXT_DIR_ENTRY  CurrentDirectoryEntry;

    TRACE("ExtSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x DirectorySize = %d FileName = \"%s\"\n", DirectoryBuffer, DirectorySize, FileName);

    while (CurrentOffset < DirectorySize)
    {
        CurrentDirectoryEntry = (PEXT_DIR_ENTRY)((ULONG_PTR)DirectoryBuffer + CurrentOffset);

        if (!CurrentDirectoryEntry->EntryLen)
            break;

        if ((CurrentDirectoryEntry->EntryLen + CurrentOffset) > DirectorySize)
        {
            FileSystemError("Directory entry extends past end of directory file.");
            return FALSE;
        }

        if (!CurrentDirectoryEntry->Inode)
            goto NextDirectoryEntry;

        TRACE("EXT Directory Entry:\n");
        TRACE("Inode = %d\n", CurrentDirectoryEntry->Inode);
        TRACE("EntryLen = %d\n", CurrentDirectoryEntry->EntryLen);
        TRACE("NameLen = %d\n", CurrentDirectoryEntry->NameLen);
        TRACE("FileType = %d\n", CurrentDirectoryEntry->FileType);
        TRACE("Name = \"");
        for (ULONG NameOffset = 0; NameOffset < CurrentDirectoryEntry->NameLen; NameOffset++)
        {
            TRACE("%c", CurrentDirectoryEntry->Name[NameOffset]);
        }
        TRACE("\"\n\n");

        if (strlen(FileName) == CurrentDirectoryEntry->NameLen &&
            !_strnicmp(FileName, CurrentDirectoryEntry->Name, CurrentDirectoryEntry->NameLen))
        {
            RtlCopyMemory(DirectoryEntry, CurrentDirectoryEntry, sizeof(EXT_DIR_ENTRY));

            return TRUE;
        }

NextDirectoryEntry:
        CurrentOffset += CurrentDirectoryEntry->EntryLen;
    }

    return FALSE;
}

/*
 * ExtReadFileBig()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOLEAN ExtReadFileBig(PEXT_FILE_INFO ExtFileInfo, ULONGLONG BytesToRead, ULONGLONG* BytesRead, PVOID Buffer)
{
    PEXT_VOLUME_INFO Volume = ExtFileInfo->Volume;
    ULONG                BlockNumber;
    ULONG                BlockNumberIndex;
    ULONG                OffsetInBlock;
    ULONG                LengthInBlock;
    ULONG                NumberOfBlocks;

    TRACE("ExtReadFileBig() BytesToRead = %d Buffer = 0x%x\n", (ULONG)BytesToRead, Buffer);

    if (BytesRead != NULL)
    {
        *BytesRead = 0;
    }

    // Make sure we have the block pointer list if we need it
    if (ExtFileInfo->FileBlockList == NULL)
    {
        // Block pointer list is NULL
        // so this better be a fast symbolic link or else
        if (((ExtFileInfo->Inode.Mode & EXT_S_IFMT) != EXT_S_IFLNK) ||
            (ExtFileInfo->FileSize > FAST_SYMLINK_MAX_NAME_SIZE))
        {
            FileSystemError("Block pointer list is NULL and file is not a fast symbolic link.");
            return FALSE;
        }
    }

    //
    // If the user is trying to read past the end of
    // the file then return success with BytesRead == 0.
    //
    if (ExtFileInfo->FilePointer >= ExtFileInfo->FileSize)
    {
        return TRUE;
    }

    //
    // If the user is trying to read more than there is to read
    // then adjust the amount to read.
    //
    if ((ExtFileInfo->FilePointer + BytesToRead) > ExtFileInfo->FileSize)
    {
        BytesToRead = (ExtFileInfo->FileSize - ExtFileInfo->FilePointer);
    }

    // Check if this is a fast symbolic link
    // if so then the read is easy
    if (((ExtFileInfo->Inode.Mode & EXT_S_IFMT) == EXT_S_IFLNK) &&
        (ExtFileInfo->FileSize <= FAST_SYMLINK_MAX_NAME_SIZE))
    {
        TRACE("Reading fast symbolic link data\n");

        // Copy the data from the link
        RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)ExtFileInfo->FilePointer + ExtFileInfo->Inode.SymLink), (ULONG)BytesToRead);

        if (BytesRead != NULL)
        {
            *BytesRead = BytesToRead;
        }
        // ExtFileInfo->FilePointer += BytesToRead;

        return TRUE;
    }

    //
    // Ok, now we have to perform at most 3 calculations
    // I'll draw you a picture (using nifty ASCII art):
    //
    // CurrentFilePointer -+
    //                     |
    //    +----------------+
    //    |
    // +-----------+-----------+-----------+-----------+
    // | Block 1   | Block 2   | Block 3   | Block 4   |
    // +-----------+-----------+-----------+-----------+
    //    |                                    |
    //    +---------------+--------------------+
    //                    |
    // BytesToRead -------+
    //
    // 1 - The first calculation (and read) will align
    //     the file pointer with the next block.
    //     boundary (if we are supposed to read that much)
    // 2 - The next calculation (and read) will read
    //     in all the full blocks that the requested
    //     amount of data would cover (in this case
    //     blocks 2 & 3).
    // 3 - The last calculation (and read) would read
    //     in the remainder of the data requested out of
    //     the last block.
    //

    //
    // Only do the first read if we
    // aren't aligned on a block boundary
    //
    if (ExtFileInfo->FilePointer % Volume->BlockSizeInBytes)
    {
        //
        // Do the math for our first read
        //
        BlockNumberIndex = (ULONG)(ExtFileInfo->FilePointer / Volume->BlockSizeInBytes);
        BlockNumber = ExtFileInfo->FileBlockList[BlockNumberIndex];
        OffsetInBlock = (ExtFileInfo->FilePointer % Volume->BlockSizeInBytes);
        LengthInBlock = (ULONG)((BytesToRead > (Volume->BlockSizeInBytes - OffsetInBlock)) ? (Volume->BlockSizeInBytes - OffsetInBlock) : BytesToRead);

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!ExtReadPartialBlock(Volume, BlockNumber, OffsetInBlock, LengthInBlock, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += LengthInBlock;
        }
        BytesToRead -= LengthInBlock;
        ExtFileInfo->FilePointer += LengthInBlock;
        Buffer = (PVOID)((ULONG_PTR)Buffer + LengthInBlock);
    }

    //
    // Do the math for our second read (if any data left)
    //
    if (BytesToRead > 0)
    {
        //
        // Determine how many full clusters we need to read
        //
        NumberOfBlocks = (ULONG)(BytesToRead / Volume->BlockSizeInBytes);

        while (NumberOfBlocks > 0)
        {
            BlockNumberIndex = (ULONG)(ExtFileInfo->FilePointer / Volume->BlockSizeInBytes);
            BlockNumber = ExtFileInfo->FileBlockList[BlockNumberIndex];

            //
            // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
            //
            if (!ExtReadBlock(Volume, BlockNumber, Buffer))
            {
                return FALSE;
            }
            if (BytesRead != NULL)
            {
                *BytesRead += Volume->BlockSizeInBytes;
            }
            BytesToRead -= Volume->BlockSizeInBytes;
            ExtFileInfo->FilePointer += Volume->BlockSizeInBytes;
            Buffer = (PVOID)((ULONG_PTR)Buffer + Volume->BlockSizeInBytes);
            NumberOfBlocks--;
        }
    }

    //
    // Do the math for our third read (if any data left)
    //
    if (BytesToRead > 0)
    {
        BlockNumberIndex = (ULONG)(ExtFileInfo->FilePointer / Volume->BlockSizeInBytes);
        BlockNumber = ExtFileInfo->FileBlockList[BlockNumberIndex];

        //
        // Now do the read and update BytesRead & FilePointer
        //
        if (!ExtReadPartialBlock(Volume, BlockNumber, 0, (ULONG)BytesToRead, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += BytesToRead;
        }
        ExtFileInfo->FilePointer += BytesToRead;
    }

    return TRUE;
}

BOOLEAN ExtReadVolumeSectors(PEXT_VOLUME_INFO Volume, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
#if 0
    return CacheReadDiskSectors(DriveNumber, SectorNumber + ExtVolumeStartSector, SectorCount, Buffer);
#endif

    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    /* Seek to right position */
    Position.QuadPart = (ULONGLONG)SectorNumber * 512;
    Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        TRACE("ExtReadVolumeSectors() Failed to seek\n");
        return FALSE;
    }

    /* Read data */
    Status = ArcRead(Volume->DeviceId, Buffer, SectorCount * 512, &Count);
    if (Status != ESUCCESS || Count != SectorCount * 512)
    {
        TRACE("ExtReadVolumeSectors() Failed to read\n");
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

BOOLEAN ExtReadSuperBlock(PEXT_VOLUME_INFO Volume)
{
    PEXT_SUPER_BLOCK SuperBlock = Volume->SuperBlock;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("ExtReadSuperBlock()\n");

#if 0
    /* Free any memory previously allocated */
    if (SuperBlock != NULL)
    {
        FrLdrTempFree(SuperBlock, TAG_EXT_SUPER_BLOCK);
        SuperBlock = NULL;
    }
#endif

    /* Allocate the memory to hold the super block if needed */
    if (SuperBlock == NULL)
    {
        SuperBlock = (PEXT_SUPER_BLOCK)FrLdrTempAlloc(1024, TAG_EXT_SUPER_BLOCK);
        if (SuperBlock == NULL)
        {
            FileSystemError("Out of memory.");
            return FALSE;
        }
    }
    Volume->SuperBlock = SuperBlock;

    /* Reset its contents */
    RtlZeroMemory(SuperBlock, 1024);

    /* Read the SuperBlock */
    Position.QuadPart = 2 * 512;
    Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
        return FALSE;
    Status = ArcRead(Volume->DeviceId, SuperBlock, 2 * 512, &Count);
    if (Status != ESUCCESS || Count != 2 * 512)
        return FALSE;

    TRACE("Dumping super block:\n");
    TRACE("InodesCount: %d\n", SuperBlock->InodesCount);
    TRACE("BlocksCountLo: %d\n", SuperBlock->BlocksCountLo);
    TRACE("RBlocksCountLo: %d\n", SuperBlock->RBlocksCountLo);
    TRACE("FreeBlocksCountLo: %d\n", SuperBlock->FreeBlocksCountLo);
    TRACE("FreeInodesCount: %d\n", SuperBlock->FreeInodesCount);
    TRACE("FirstDataBlock: %d\n", SuperBlock->FirstDataBlock);
    TRACE("LogBlockSize: %d\n", SuperBlock->LogBlockSize);
    TRACE("LogFragSize: %d\n", SuperBlock->LogFragSize);
    TRACE("BlocksPerGroup: %d\n", SuperBlock->BlocksPerGroup);
    TRACE("FragsPerGroup: %d\n", SuperBlock->FragsPerGroup);
    TRACE("InodesPerGroup: %d\n", SuperBlock->InodesPerGroup);
    TRACE("MTime: %d\n", SuperBlock->MTime);
    TRACE("WTime: %d\n", SuperBlock->WTime);
    TRACE("MntCount: %d\n", SuperBlock->MntCount);
    TRACE("MaxMntCount: %d\n", SuperBlock->MaxMntCount);
    TRACE("Magic: 0x%x\n", SuperBlock->Magic);
    TRACE("State: 0x%x\n", SuperBlock->State);
    TRACE("Errors: 0x%x\n", SuperBlock->Errors);
    TRACE("MinorRevisionLevel: %d\n", SuperBlock->MinorRevisionLevel);
    TRACE("LastCheck: %d\n", SuperBlock->LastCheck);
    TRACE("CheckInterval: %d\n", SuperBlock->CheckInterval);
    TRACE("CreatorOS: %d\n", SuperBlock->CreatorOS);
    TRACE("RevisionLevel: %d\n", SuperBlock->RevisionLevel);
    TRACE("DefResUID: %d\n", SuperBlock->DefResUID);
    TRACE("DefResGID: %d\n", SuperBlock->DefResGID);
    TRACE("FirstInode: %d\n", SuperBlock->FirstInode);
    TRACE("InodeSize: %d\n", SuperBlock->InodeSize);
    TRACE("BlockGroupNr: %d\n", SuperBlock->BlockGroupNr);
    TRACE("FeatureCompat: 0x%x\n", SuperBlock->FeatureCompat);
    TRACE("FeatureIncompat: 0x%x\n", SuperBlock->FeatureIncompat);
    TRACE("FeatureROCompat: 0x%x\n", SuperBlock->FeatureROCompat);
    TRACE("UUID: { ");
    for (ULONG i = 0; i < sizeof(SuperBlock->UUID); i++)
    {
        TRACE("0x%02x", SuperBlock->UUID[i]);
        if (i < sizeof(SuperBlock->UUID) - 1)
            TRACE(", ");
    }
    TRACE(" }\n");
    TRACE("VolumeName: \"%s\"\n", SuperBlock->VolumeName);
    TRACE("LastMounted: \"%s\"\n", SuperBlock->LastMounted);
    TRACE("AlgorithmUsageBitmap: 0x%x\n", SuperBlock->AlgorithmUsageBitmap);
    TRACE("PreallocBlocks: %d\n", SuperBlock->PreallocBlocks);
    TRACE("PreallocDirBlocks: %d\n", SuperBlock->PreallocDirBlocks);
    TRACE("ReservedGdtBlocks: %d\n", SuperBlock->ReservedGdtBlocks);
    TRACE("JournalUUID: { ");
    for (ULONG i = 0; i < sizeof(SuperBlock->JournalUUID); i++)
    {
        TRACE("0x%02x", SuperBlock->JournalUUID[i]);
        if (i < sizeof(SuperBlock->JournalUUID) - 1)
            TRACE(", ");
    }
    TRACE(" }\n");
    TRACE("JournalInum: %d\n", SuperBlock->JournalInum);
    TRACE("JournalDev: %d\n", SuperBlock->JournalDev);
    TRACE("LastOrphan: %d\n", SuperBlock->LastOrphan);
    TRACE("HashSeed: { 0x%02x, 0x%02x, 0x%02x, 0x%02x }\n",
        SuperBlock->HashSeed[0], SuperBlock->HashSeed[1],
        SuperBlock->HashSeed[2], SuperBlock->HashSeed[3]);
    TRACE("DefHashVersion: %d\n", SuperBlock->DefHashVersion);
    TRACE("JournalBackupType: %d\n", SuperBlock->JournalBackupType);
    TRACE("GroupDescSize: %d\n", SuperBlock->GroupDescSize);

    //
    // Check the super block magic
    //
    if (SuperBlock->Magic != EXT_SUPERBLOCK_MAGIC)
    {
        FileSystemError("Invalid super block magic (0xef53)");
        return FALSE;
    }

    // Calculate the group count
    Volume->GroupCount = (SuperBlock->BlocksCountLo - SuperBlock->FirstDataBlock + SuperBlock->BlocksPerGroup - 1) / SuperBlock->BlocksPerGroup;
    TRACE("ExtGroupCount: %d\n", Volume->GroupCount);

    // Calculate the block size
    Volume->BlockSizeInBytes = 1024 << SuperBlock->LogBlockSize;
    Volume->BlockSizeInSectors = Volume->BlockSizeInBytes / Volume->BytesPerSector;
    TRACE("ExtBlockSizeInBytes: %d\n", Volume->BlockSizeInBytes);
    TRACE("ExtBlockSizeInSectors: %d\n", Volume->BlockSizeInSectors);

    // Calculate the fragment size
    if (SuperBlock->LogFragSize >= 0)
    {
        Volume->FragmentSizeInBytes = 1024 << SuperBlock->LogFragSize;
    }
    else
    {
        Volume->FragmentSizeInBytes = 1024 >> -(SuperBlock->LogFragSize);
    }
    Volume->FragmentSizeInSectors = Volume->FragmentSizeInBytes / Volume->BytesPerSector;
    TRACE("ExtFragmentSizeInBytes: %d\n", Volume->FragmentSizeInBytes);
    TRACE("ExtFragmentSizeInSectors: %d\n", Volume->FragmentSizeInSectors);

    // Verify that the fragment size and the block size are equal
    if (Volume->BlockSizeInBytes != Volume->FragmentSizeInBytes)
    {
        FileSystemError("The fragment size must be equal to the block size.");
        return FALSE;
    }

    // Set the volume inode size in bytes
    Volume->InodeSizeInBytes = EXT_INODE_SIZE(SuperBlock);
    TRACE("InodeSizeInBytes: %d\n", Volume->InodeSizeInBytes);

    // Set the volume group descriptor size in bytes
    Volume->GroupDescSizeInBytes = EXT_GROUP_DESC_SIZE(SuperBlock);
    TRACE("GroupDescSizeInBytes: %d\n", Volume->GroupDescSizeInBytes);

    // Calculate the number of inodes in one block
    Volume->InodesPerBlock = Volume->BlockSizeInBytes / Volume->InodeSizeInBytes;
    TRACE("ExtInodesPerBlock: %d\n", Volume->InodesPerBlock);

    // Calculate the number of group descriptors in one block
    Volume->GroupDescPerBlock = Volume->BlockSizeInBytes / Volume->GroupDescSizeInBytes;
    TRACE("ExtGroupDescPerBlock: %d\n", Volume->GroupDescPerBlock);

    return TRUE;
}

BOOLEAN ExtReadGroupDescriptors(PEXT_VOLUME_INFO Volume)
{
    ULONG GroupDescBlockCount;
    ULONG BlockNumber;
    PUCHAR CurrentGroupDescBlock;

    TRACE("ExtReadGroupDescriptors()\n");

    /* Free any memory previously allocated */
    if (Volume->GroupDescriptors != NULL)
    {
        FrLdrTempFree(Volume->GroupDescriptors, TAG_EXT_GROUP_DESC);
        Volume->GroupDescriptors = NULL;
    }

    /* Now allocate the memory to hold the group descriptors */
    GroupDescBlockCount = ROUND_UP(Volume->GroupCount, Volume->GroupDescPerBlock) / Volume->GroupDescPerBlock;
    Volume->GroupDescriptors = (PEXT_GROUP_DESC)FrLdrTempAlloc(GroupDescBlockCount * Volume->BlockSizeInBytes, TAG_EXT_GROUP_DESC);
    if (Volume->GroupDescriptors == NULL)
    {
        FileSystemError("Out of memory.");
        return FALSE;
    }

    // Now read the group descriptors
    CurrentGroupDescBlock = (PUCHAR)Volume->GroupDescriptors;
    BlockNumber = Volume->SuperBlock->FirstDataBlock + 1;

    while (GroupDescBlockCount--)
    {
        if (!ExtReadBlock(Volume, BlockNumber, CurrentGroupDescBlock))
        {
            return FALSE;
        }

        BlockNumber++;
        CurrentGroupDescBlock += Volume->BlockSizeInBytes;
    }

    return TRUE;
}

BOOLEAN ExtReadDirectory(PEXT_VOLUME_INFO Volume, ULONG Inode, PVOID* DirectoryBuffer, PEXT_INODE InodePointer)
{
    EXT_FILE_INFO DirectoryFileInfo;

    TRACE("ExtReadDirectory() Inode = %d\n", Inode);

    // Read the directory inode
    if (!ExtReadInode(Volume, Inode, InodePointer))
    {
        return FALSE;
    }

    // Make sure it is a directory inode
    if ((InodePointer->Mode & EXT_S_IFMT) != EXT_S_IFDIR)
    {
        FileSystemError("Inode is not a directory.");
        return FALSE;
    }

    // Fill in file info struct so we can call ExtReadFileBig()
    RtlZeroMemory(&DirectoryFileInfo, sizeof(EXT_FILE_INFO));
    DirectoryFileInfo.Volume = Volume;
    DirectoryFileInfo.FileBlockList = ExtReadBlockPointerList(Volume, InodePointer);
    DirectoryFileInfo.FilePointer = 0;
    DirectoryFileInfo.FileSize = ExtGetInodeFileSize(InodePointer);

    if (DirectoryFileInfo.FileBlockList == NULL)
    {
        return FALSE;
    }

    //
    // Now allocate the memory to hold the group descriptors
    //
    ASSERT(DirectoryFileInfo.FileSize <= 0xFFFFFFFF);
    *DirectoryBuffer = (PEXT_DIR_ENTRY)FrLdrTempAlloc((ULONG)DirectoryFileInfo.FileSize, TAG_EXT_BUFFER);

    //
    // Make sure we got the memory
    //
    if (*DirectoryBuffer == NULL)
    {
        FrLdrTempFree(DirectoryFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
        FileSystemError("Out of memory.");
        return FALSE;
    }

    // Now read the root directory data
    if (!ExtReadFileBig(&DirectoryFileInfo, DirectoryFileInfo.FileSize, NULL, *DirectoryBuffer))
    {
        FrLdrTempFree(*DirectoryBuffer, TAG_EXT_BUFFER);
        *DirectoryBuffer = NULL;
        FrLdrTempFree(DirectoryFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
        return FALSE;
    }

    FrLdrTempFree(DirectoryFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
    return TRUE;
}

BOOLEAN ExtReadBlock(PEXT_VOLUME_INFO Volume, ULONG BlockNumber, PVOID Buffer)
{
    CHAR    ErrorString[80];

    TRACE("ExtReadBlock() BlockNumber = %d Buffer = 0x%x\n", BlockNumber, Buffer);

    // Make sure its a valid block
    if (BlockNumber > Volume->SuperBlock->BlocksCountLo)
    {
        sprintf(ErrorString, "Error reading block %d - block out of range.", (int) BlockNumber);
        FileSystemError(ErrorString);
        return FALSE;
    }

    // Check to see if this is a sparse block
    if (BlockNumber == 0)
    {
        TRACE("Block is part of a sparse file. Zeroing input buffer.\n");

        RtlZeroMemory(Buffer, Volume->BlockSizeInBytes);

        return TRUE;
    }

    return ExtReadVolumeSectors(Volume, (ULONGLONG)BlockNumber * Volume->BlockSizeInSectors, Volume->BlockSizeInSectors, Buffer);
}

/*
 * ExtReadPartialBlock()
 * Reads part of a block into memory
 */
BOOLEAN ExtReadPartialBlock(PEXT_VOLUME_INFO Volume, ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
    PVOID TempBuffer;

    TRACE("ExtReadPartialBlock() BlockNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", BlockNumber, StartingOffset, Length, Buffer);

    TempBuffer = FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);

    if (!ExtReadBlock(Volume, BlockNumber, TempBuffer))
    {
        FrLdrTempFree(TempBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    RtlCopyMemory(Buffer, ((PUCHAR)TempBuffer + StartingOffset), Length);

    FrLdrTempFree(TempBuffer, TAG_EXT_BUFFER);

    return TRUE;
}

ULONG ExtGetInodeGroupNumber(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return ((Inode - 1) / Volume->SuperBlock->InodesPerGroup);
}

ULONG ExtGetInodeBlockNumber(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->InodesPerGroup) / Volume->InodesPerBlock);
}

ULONG ExtGetInodeOffsetInBlock(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->InodesPerGroup) % Volume->InodesPerBlock);
}

BOOLEAN ExtReadInode(PEXT_VOLUME_INFO Volume, ULONG Inode, PEXT_INODE InodeBuffer)
{
    ULONG        InodeGroupNumber;
    ULONG        InodeBlockNumber;
    ULONG        InodeOffsetInBlock;
    CHAR        ErrorString[80];
    EXT_GROUP_DESC    GroupDescriptor;

    TRACE("ExtReadInode() Inode = %d\n", Inode);

    // Make sure its a valid inode
    if ((Inode < 1) || (Inode > Volume->SuperBlock->InodesCount))
    {
        sprintf(ErrorString, "Error reading inode %ld - inode out of range.", Inode);
        FileSystemError(ErrorString);
        return FALSE;
    }

    // Get inode group & block number and offset in block
    InodeGroupNumber = ExtGetInodeGroupNumber(Volume, Inode);
    InodeBlockNumber = ExtGetInodeBlockNumber(Volume, Inode);
    InodeOffsetInBlock = ExtGetInodeOffsetInBlock(Volume, Inode);
    TRACE("InodeGroupNumber = %d\n", InodeGroupNumber);
    TRACE("InodeBlockNumber = %d\n", InodeBlockNumber);
    TRACE("InodeOffsetInBlock = %d\n", InodeOffsetInBlock);

    // Read the group descriptor
    if (!ExtReadGroupDescriptor(Volume, InodeGroupNumber, &GroupDescriptor))
    {
        return FALSE;
    }

    // Add the start block of the inode table to the inode block number
    InodeBlockNumber += GroupDescriptor.InodeTable;
    TRACE("InodeBlockNumber (after group desc correction) = %d\n", InodeBlockNumber);

    // Read the block
    if (!ExtReadPartialBlock(Volume,
                              InodeBlockNumber,
                              (InodeOffsetInBlock * Volume->InodeSizeInBytes),
                              sizeof(EXT_INODE),
                              InodeBuffer))
    {
        return FALSE;
    }

    TRACE("Dumping inode information:\n");
    TRACE("Mode = 0x%x\n", InodeBuffer->Mode);
    TRACE("UID = %d\n", InodeBuffer->UID);
    TRACE("Size = %d\n", InodeBuffer->Size);
    TRACE("Atime = %d\n", InodeBuffer->Atime);
    TRACE("Ctime = %d\n", InodeBuffer->Ctime);
    TRACE("Mtime = %d\n", InodeBuffer->Mtime);
    TRACE("Dtime = %d\n", InodeBuffer->Dtime);
    TRACE("GID = %d\n", InodeBuffer->GID);
    TRACE("LinksCount = %d\n", InodeBuffer->LinksCount);
    TRACE("Blocks = %d\n", InodeBuffer->Blocks);
    TRACE("Flags = 0x%x\n", InodeBuffer->Flags);
    TRACE("OSD1 = 0x%x\n", InodeBuffer->OSD1);
    TRACE("DirectBlocks = { %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u }\n",
        InodeBuffer->Blocks.DirectBlocks[0], InodeBuffer->Blocks.DirectBlocks[1], InodeBuffer->Blocks.DirectBlocks[2], InodeBuffer->Blocks.DirectBlocks[3],
        InodeBuffer->Blocks.DirectBlocks[4], InodeBuffer->Blocks.DirectBlocks[5], InodeBuffer->Blocks.DirectBlocks[6], InodeBuffer->Blocks.DirectBlocks[7],
        InodeBuffer->Blocks.DirectBlocks[8], InodeBuffer->Blocks.DirectBlocks[9], InodeBuffer->Blocks.DirectBlocks[10], InodeBuffer->Blocks.DirectBlocks[11]);
    TRACE("IndirectBlock = %u\n", InodeBuffer->Blocks.IndirectBlock);
    TRACE("DoubleIndirectBlock = %u\n", InodeBuffer->Blocks.DoubleIndirectBlock);
    TRACE("TripleIndirectBlock = %u\n", InodeBuffer->Blocks.TripleIndirectBlock);
    TRACE("Generation = %d\n", InodeBuffer->Generation);
    TRACE("FileACL = %d\n", InodeBuffer->FileACL);
    TRACE("DirACL = %d\n", InodeBuffer->DirACL);
    TRACE("FragAddress = %d\n", InodeBuffer->FragAddress);
    TRACE("OSD2 = { %d, %d, %d }\n",
        InodeBuffer->OSD2[0], InodeBuffer->OSD2[1], InodeBuffer->OSD2[2]);

    return TRUE;
}

BOOLEAN ExtReadGroupDescriptor(PEXT_VOLUME_INFO Volume, ULONG Group, PEXT_GROUP_DESC GroupBuffer)
{
    TRACE("ExtReadGroupDescriptor()\n");

#if 0
    if (!ExtReadBlock(Volume, ExtGetGroupDescBlockNumber(Volume, Group), (PVOID)FILESYSBUFFER))
    {
        return FALSE;
    }
    RtlCopyMemory(GroupBuffer, (PVOID)(FILESYSBUFFER + ExtGetGroupDescOffsetInBlock(Volume, Group)), sizeof(EXT_GROUP_DESC));
#endif

    RtlCopyMemory(GroupBuffer, &((PUCHAR)Volume->GroupDescriptors)[Volume->GroupDescSizeInBytes * Group], sizeof(EXT_GROUP_DESC));

    TRACE("Dumping group descriptor:\n");
    TRACE("BlockBitmap = %d\n", GroupBuffer->BlockBitmap);
    TRACE("InodeBitmap = %d\n", GroupBuffer->InodeBitmap);
    TRACE("InodeTable = %d\n", GroupBuffer->InodeTable);
    TRACE("FreeBlocksCount = %d\n", GroupBuffer->FreeBlocksCount);
    TRACE("FreeInodesCount = %d\n", GroupBuffer->FreeInodesCount);
    TRACE("UsedDirsCount = %d\n", GroupBuffer->UsedDirsCount);

    return TRUE;
}

ULONG* ExtReadBlockPointerList(PEXT_VOLUME_INFO Volume, PEXT_INODE Inode)
{
    ULONGLONG        FileSize;
    ULONG        BlockCount;
    ULONG*    BlockList;
    ULONG        CurrentBlockInList;
    ULONG        CurrentBlock;

    TRACE("ExtReadBlockPointerList()\n");

    // Get the number of blocks this file occupies
    // I would just use Inode->i_blocks but it
    // doesn't seem to be the number of blocks
    // the file size corresponds to, but instead
    // it is much bigger.
    //BlockCount = Inode->i_blocks;
    FileSize = ExtGetInodeFileSize(Inode);
    FileSize = ROUND_UP(FileSize, Volume->BlockSizeInBytes);
    BlockCount = (ULONG)(FileSize / Volume->BlockSizeInBytes);

    // Allocate the memory for the block list
    BlockList = FrLdrTempAlloc(BlockCount * sizeof(ULONG), TAG_EXT_BLOCK_LIST);
    if (BlockList == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(BlockList, BlockCount * sizeof(ULONG));

    // If the file is stored in extents, copy the block pointers by reading the
    // extent entries.
    if (Inode->Flags & EXT4_INODE_FLAG_EXTENTS)
    {
        CurrentBlockInList = 0;

        if (!ExtCopyBlockPointersByExtents(Volume, BlockList, &CurrentBlockInList, BlockCount, &Inode->ExtentHeader))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }

        return BlockList;
    }

    // Copy the direct block pointers
    for (CurrentBlockInList = CurrentBlock = 0;
         CurrentBlockInList < BlockCount && CurrentBlock < sizeof(Inode->Blocks.DirectBlocks) / sizeof(*Inode->Blocks.DirectBlocks);
         CurrentBlock++, CurrentBlockInList++)
    {
        BlockList[CurrentBlockInList] = Inode->Blocks.DirectBlocks[CurrentBlock];
    }

    // Copy the indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!ExtCopyIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->Blocks.IndirectBlock))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the double indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!ExtCopyDoubleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->Blocks.DoubleIndirectBlock))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the triple indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!ExtCopyTripleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->Blocks.TripleIndirectBlock))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    return BlockList;
}

ULONGLONG ExtGetInodeFileSize(PEXT_INODE Inode)
{
    if ((Inode->Mode & EXT_S_IFMT) == EXT_S_IFDIR)
    {
        return (ULONGLONG)(Inode->Size);
    }
    else
    {
        return ((ULONGLONG)(Inode->Size) | ((ULONGLONG)(Inode->DirACL) << 32));
    }
}

BOOLEAN ExtCopyBlockPointersByExtents(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, PEXT4_EXTENT_HEADER ExtentHeader)
{
    TRACE("ExtCopyBlockPointersByExtents() BlockCount = 0x%p\n", BlockCount);

    if (ExtentHeader->Magic != EXT4_EXTENT_HEADER_MAGIC ||
        ExtentHeader->Depth > EXT4_EXTENT_MAX_LEVEL)
        return FALSE;

    ULONG Level = ExtentHeader->Depth;
    ULONG Entries = ExtentHeader->Entries;

    TRACE("Level: %d\n", Level);
    TRACE("Entries: %d\n", Entries);

    // If the level is 0, we have a direct extent block mapping
    if (!Level)
    {
        PEXT4_EXTENT Extent = (PVOID)&ExtentHeader[1];

        while ((*CurrentBlockInList) < BlockCount && Entries--)
        {
            BOOLEAN SparseExtent = (Extent->Length > EXT4_EXTENT_MAX_LENGTH);
            ULONG Length = SparseExtent ? (Extent->Length - EXT4_EXTENT_MAX_LENGTH) : Extent->Length; 
            ULONG CurrentBlock = SparseExtent ? 0 : Extent->Start;

            // Copy the pointers to the block list
            while ((*CurrentBlockInList) < BlockCount && Length--)
            {
                BlockList[(*CurrentBlockInList)++] = CurrentBlock;

                if (!SparseExtent)
                    CurrentBlock++;
            }

            Extent++;
        }
    }
    else
    {
        PEXT4_EXTENT_IDX Extent = (PVOID)&ExtentHeader[1];

        PEXT4_EXTENT_HEADER BlockBuffer = FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
        if (!BlockBuffer)
        {
            return FALSE;
        }

        // Recursively copy the pointers to the block list
        while ((*CurrentBlockInList) < BlockCount && Entries--)
        {
            if (!(ExtReadBlock(Volume, Extent->Leaf, BlockBuffer) &&
                  ExtCopyBlockPointersByExtents(Volume, BlockList, CurrentBlockInList, BlockCount, BlockBuffer)))
            {
                FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
                return FALSE;
            }

            Extent++;
        }

        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    }

    return TRUE;
}

BOOLEAN ExtCopyIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("ExtCopyIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (!BlockBuffer)
    {
        return FALSE;
    }

    if (!ExtReadBlock(Volume, IndirectBlock, BlockBuffer))
    {
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        BlockList[(*CurrentBlockInList)] = BlockBuffer[CurrentBlock];
        (*CurrentBlockInList)++;
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);

    return TRUE;
}

BOOLEAN ExtCopyDoubleIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("ExtCopyDoubleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!ExtReadBlock(Volume, DoubleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!ExtCopyIndirectBlockPointers(Volume, BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
        {
            FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    return TRUE;
}

BOOLEAN ExtCopyTripleIndirectBlockPointers(PEXT_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("ExtCopyTripleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!ExtReadBlock(Volume, TripleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!ExtCopyDoubleIndirectBlockPointers(Volume, BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
        {
            FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    return TRUE;
}

ARC_STATUS ExtClose(ULONG FileId)
{
    PEXT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
    FrLdrTempFree(FileHandle, TAG_EXT_FILE);
    return ESUCCESS;
}

ARC_STATUS ExtGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PEXT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = FileHandle->FileSize;
    Information->CurrentAddress.QuadPart = FileHandle->FilePointer;

    TRACE("ExtGetFileInformation(%lu) -> FileSize = %llu, FilePointer = 0x%llx\n",
          FileId, Information->EndingAddress.QuadPart, Information->CurrentAddress.QuadPart);

    return ESUCCESS;
}

ARC_STATUS ExtOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PEXT_VOLUME_INFO Volume;
    PEXT_FILE_INFO FileHandle;
    ULONG DeviceId;

    /* Check parameters */
    if (OpenMode != OpenReadOnly)
        return EACCES;

    /* Get underlying device */
    DeviceId = FsGetDeviceId(*FileId);
    Volume = ExtVolumes[DeviceId];

    TRACE("ExtOpen() FileName = \"%s\"\n", Path);

    /* Call the internal open method */
    // Status = ExtOpenFile(Volume, Path, &FileHandle);
    FileHandle = ExtOpenFile(Volume, Path);
    if (!FileHandle)
        return ENOENT;

    /* Success, remember the handle */
    FsSetDeviceSpecific(*FileId, FileHandle);
    return ESUCCESS;
}

ARC_STATUS ExtRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PEXT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
    ULONGLONG BytesReadBig;
    BOOLEAN Success;

    //
    // Read data
    //
    Success = ExtReadFileBig(FileHandle, N, &BytesReadBig, Buffer);
    *Count = (ULONG)BytesReadBig;

    //
    // Check for success
    //
    if (Success)
        return ESUCCESS;
    else
        return EIO;
}

ARC_STATUS ExtSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PEXT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += FileHandle->FilePointer;
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart >= FileHandle->FileSize)
        return EINVAL;

    FileHandle->FilePointer = NewPosition.QuadPart;
    return ESUCCESS;
}

const DEVVTBL ExtFuncTable =
{
    ExtClose,
    ExtGetFileInformation,
    ExtOpen,
    ExtRead,
    ExtSeek,
    L"ext2fs",
};

const DEVVTBL* ExtMount(ULONG DeviceId)
{
    PEXT_VOLUME_INFO Volume;
    EXT_SUPER_BLOCK SuperBlock;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("Enter ExtMount(%lu)\n", DeviceId);

    /* Allocate data for volume information */
    Volume = FrLdrTempAlloc(sizeof(EXT_VOLUME_INFO), TAG_EXT_VOLUME);
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(EXT_VOLUME_INFO));

    /* Read the SuperBlock */
    Position.QuadPart = 2 * 512;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }
    Status = ArcRead(DeviceId, &SuperBlock, sizeof(SuperBlock), &Count);
    if (Status != ESUCCESS || Count != sizeof(SuperBlock))
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }

    /* Check if SuperBlock is valid. If yes, return Ext function table. */
    if (SuperBlock.Magic != EXT_SUPERBLOCK_MAGIC)
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }

    Volume->DeviceId = DeviceId;

    /* Really open the volume */
    if (!ExtOpenVolume(Volume))
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }

    /* Remember EXT volume information */
    ExtVolumes[DeviceId] = Volume;

    /* Return success */
    TRACE("ExtMount(%lu) success\n", DeviceId);
    return &ExtFuncTable;
}

#endif // _M_ARM
