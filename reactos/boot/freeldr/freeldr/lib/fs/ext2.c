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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _M_ARM
#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(FILESYSTEM);

BOOLEAN    Ext2OpenVolume(UCHAR DriveNumber, ULONGLONG VolumeStartSector, ULONGLONG PartitionSectorCount);
PEXT2_FILE_INFO    Ext2OpenFile(PCSTR FileName);
BOOLEAN    Ext2LookupFile(PCSTR FileName, PEXT2_FILE_INFO Ext2FileInfoPointer);
BOOLEAN    Ext2SearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PEXT2_DIR_ENTRY DirectoryEntry);
BOOLEAN    Ext2ReadVolumeSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);

BOOLEAN    Ext2ReadFileBig(PEXT2_FILE_INFO Ext2FileInfo, ULONGLONG BytesToRead, ULONGLONG* BytesRead, PVOID Buffer);
BOOLEAN    Ext2ReadSuperBlock(VOID);
BOOLEAN    Ext2ReadGroupDescriptors(VOID);
BOOLEAN    Ext2ReadDirectory(ULONG Inode, PVOID* DirectoryBuffer, PEXT2_INODE InodePointer);
BOOLEAN    Ext2ReadBlock(ULONG BlockNumber, PVOID Buffer);
BOOLEAN    Ext2ReadPartialBlock(ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
ULONG        Ext2GetGroupDescBlockNumber(ULONG Group);
ULONG        Ext2GetGroupDescOffsetInBlock(ULONG Group);
ULONG        Ext2GetInodeGroupNumber(ULONG Inode);
ULONG        Ext2GetInodeBlockNumber(ULONG Inode);
ULONG        Ext2GetInodeOffsetInBlock(ULONG Inode);
BOOLEAN    Ext2ReadInode(ULONG Inode, PEXT2_INODE InodeBuffer);
BOOLEAN    Ext2ReadGroupDescriptor(ULONG Group, PEXT2_GROUP_DESC GroupBuffer);
ULONG*    Ext2ReadBlockPointerList(PEXT2_INODE Inode);
ULONGLONG        Ext2GetInodeFileSize(PEXT2_INODE Inode);
BOOLEAN    Ext2CopyIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock);
BOOLEAN    Ext2CopyDoubleIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock);
BOOLEAN    Ext2CopyTripleIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock);

GEOMETRY        Ext2DiskGeometry;                // Ext2 file system disk geometry

PEXT2_SUPER_BLOCK    Ext2SuperBlock = NULL;            // Ext2 file system super block
PEXT2_GROUP_DESC    Ext2GroupDescriptors = NULL;    // Ext2 file system group descriptors

UCHAR                    Ext2DriveNumber = 0;            // Ext2 file system drive number
ULONGLONG                Ext2VolumeStartSector = 0;        // Ext2 file system starting sector
ULONG                    Ext2BlockSizeInBytes = 0;        // Block size in bytes
ULONG                    Ext2BlockSizeInSectors = 0;        // Block size in sectors
ULONG                    Ext2FragmentSizeInBytes = 0;    // Fragment size in bytes
ULONG                    Ext2FragmentSizeInSectors = 0;    // Fragment size in sectors
ULONG                    Ext2GroupCount = 0;                // Number of groups in this file system
ULONG                    Ext2InodesPerBlock = 0;            // Number of inodes in one block
ULONG                    Ext2GroupDescPerBlock = 0;        // Number of group descriptors in one block

#define TAG_EXT_BLOCK_LIST 'LtxE'
#define TAG_EXT_FILE 'FtxE'
#define TAG_EXT_BUFFER  'BtxE'
#define TAG_EXT_SUPER_BLOCK 'StxE'
#define TAG_EXT_GROUP_DESC 'GtxE'

BOOLEAN DiskGetBootVolume(PUCHAR DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType)
{
    *DriveNumber = 0;
    *StartSector = 0;
    *SectorCount = 0;
    *FsType = 0;
    return FALSE;
}

BOOLEAN Ext2OpenVolume(UCHAR DriveNumber, ULONGLONG VolumeStartSector, ULONGLONG PartitionSectorCount)
{

    TRACE("Ext2OpenVolume() DriveNumber = 0x%x VolumeStartSector = %d\n", DriveNumber, VolumeStartSector);

    // Store the drive number and start sector
    Ext2DriveNumber = DriveNumber;
    Ext2VolumeStartSector = VolumeStartSector;

    if (!MachDiskGetDriveGeometry(DriveNumber, &Ext2DiskGeometry))
    {
        return FALSE;
    }

    //
    // Initialize the disk cache for this drive
    //
    if (!CacheInitializeDrive(DriveNumber))
    {
        return FALSE;
    }

    // Read in the super block
    if (!Ext2ReadSuperBlock())
    {
        return FALSE;
    }

    // Read in the group descriptors
    if (!Ext2ReadGroupDescriptors())
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * Ext2OpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
PEXT2_FILE_INFO Ext2OpenFile(PCSTR FileName)
{
    EXT2_FILE_INFO        TempExt2FileInfo;
    PEXT2_FILE_INFO        FileHandle;
    CHAR            SymLinkPath[EXT2_NAME_LEN];
    CHAR            FullPath[EXT2_NAME_LEN * 2];
    ULONG_PTR        Index;

    TRACE("Ext2OpenFile() FileName = %s\n", FileName);

    RtlZeroMemory(SymLinkPath, sizeof(SymLinkPath));

    // Lookup the file in the file system
    if (!Ext2LookupFile(FileName, &TempExt2FileInfo))
    {
        return NULL;
    }

    // If we got a symbolic link then fix up the path
    // and re-call this function
    if ((TempExt2FileInfo.Inode.mode & EXT2_S_IFMT) == EXT2_S_IFLNK)
    {
        TRACE("File is a symbolic link\n");

        // Now read in the symbolic link path
        if (!Ext2ReadFileBig(&TempExt2FileInfo, TempExt2FileInfo.FileSize, NULL, SymLinkPath))
        {
            if (TempExt2FileInfo.FileBlockList != NULL)
            {
                FrLdrTempFree(TempExt2FileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
            }

            return NULL;
        }

        TRACE("Symbolic link path = %s\n", SymLinkPath);

        // Get the full path
        if (SymLinkPath[0] == '/' || SymLinkPath[0] == '\\')
        {
            // Symbolic link is an absolute path
            // So copy it to FullPath, but skip over
            // the '/' char at the beginning
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

        TRACE("Full file path = %s\n", FullPath);

        if (TempExt2FileInfo.FileBlockList != NULL)
        {
            FrLdrTempFree(TempExt2FileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
        }

        return Ext2OpenFile(FullPath);
    }
    else
    {
        FileHandle = FrLdrTempAlloc(sizeof(EXT2_FILE_INFO), TAG_EXT_FILE);

        if (FileHandle == NULL)
        {
            if (TempExt2FileInfo.FileBlockList != NULL)
            {
                FrLdrTempFree(TempExt2FileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
            }

            return NULL;
        }

        RtlCopyMemory(FileHandle, &TempExt2FileInfo, sizeof(EXT2_FILE_INFO));

        return FileHandle;
    }
}

/*
 * Ext2LookupFile()
 * This function searches the file system for the
 * specified filename and fills in a EXT2_FILE_INFO structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
BOOLEAN Ext2LookupFile(PCSTR FileName, PEXT2_FILE_INFO Ext2FileInfoPointer)
{
    UINT32        i;
    ULONG        NumberOfPathParts;
    CHAR        PathPart[261];
    PVOID        DirectoryBuffer;
    ULONG        DirectoryInode = EXT2_ROOT_INO;
    EXT2_INODE    InodeData;
    EXT2_DIR_ENTRY    DirectoryEntry;

    TRACE("Ext2LookupFile() FileName = %s\n", FileName);

    RtlZeroMemory(Ext2FileInfoPointer, sizeof(EXT2_FILE_INFO));

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
        if (!Ext2ReadDirectory(DirectoryInode, &DirectoryBuffer, &InodeData))
        {
            return FALSE;
        }

        //
        // Search for file name in directory
        //
        if (!Ext2SearchDirectoryBufferForFile(DirectoryBuffer, (ULONG)Ext2GetInodeFileSize(&InodeData), PathPart, &DirectoryEntry))
        {
            FrLdrTempFree(DirectoryBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }

        FrLdrTempFree(DirectoryBuffer, TAG_EXT_BUFFER);

        DirectoryInode = DirectoryEntry.inode;
    }

    if (!Ext2ReadInode(DirectoryInode, &InodeData))
    {
        return FALSE;
    }

    if (((InodeData.mode & EXT2_S_IFMT) != EXT2_S_IFREG) &&
        ((InodeData.mode & EXT2_S_IFMT) != EXT2_S_IFLNK))
    {
        FileSystemError("Inode is not a regular file or symbolic link.");
        return FALSE;
    }

    // Set the drive number
    Ext2FileInfoPointer->DriveNumber = Ext2DriveNumber;

    // If it's a regular file or a regular symbolic link
    // then get the block pointer list otherwise it must
    // be a fast symbolic link which doesn't have a block list
    if (((InodeData.mode & EXT2_S_IFMT) == EXT2_S_IFREG) ||
        ((InodeData.mode & EXT2_S_IFMT) == EXT2_S_IFLNK && InodeData.size > FAST_SYMLINK_MAX_NAME_SIZE))
    {
        Ext2FileInfoPointer->FileBlockList = Ext2ReadBlockPointerList(&InodeData);

        if (Ext2FileInfoPointer->FileBlockList == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        Ext2FileInfoPointer->FileBlockList = NULL;
    }

    Ext2FileInfoPointer->FilePointer = 0;
    Ext2FileInfoPointer->FileSize = Ext2GetInodeFileSize(&InodeData);
    RtlCopyMemory(&Ext2FileInfoPointer->Inode, &InodeData, sizeof(EXT2_INODE));

    return TRUE;
}

BOOLEAN Ext2SearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PEXT2_DIR_ENTRY DirectoryEntry)
{
    ULONG        CurrentOffset;
    PEXT2_DIR_ENTRY    CurrentDirectoryEntry;

    TRACE("Ext2SearchDirectoryBufferForFile() DirectoryBuffer = 0x%x DirectorySize = %d FileName = %s\n", DirectoryBuffer, DirectorySize, FileName);

    for (CurrentOffset=0; CurrentOffset<DirectorySize; )
    {
        CurrentDirectoryEntry = (PEXT2_DIR_ENTRY)((ULONG_PTR)DirectoryBuffer + CurrentOffset);

        if (CurrentDirectoryEntry->direntlen == 0)
        {
            break;
        }

        if ((CurrentDirectoryEntry->direntlen + CurrentOffset) > DirectorySize)
        {
            FileSystemError("Directory entry extends past end of directory file.");
            return FALSE;
        }

        TRACE("Dumping directory entry at offset %d:\n", CurrentOffset);
        DbgDumpBuffer(DPRINT_FILESYSTEM, CurrentDirectoryEntry, CurrentDirectoryEntry->direntlen);

        if ((_strnicmp(FileName, CurrentDirectoryEntry->name, CurrentDirectoryEntry->namelen) == 0) &&
            (strlen(FileName) == CurrentDirectoryEntry->namelen))
        {
            RtlCopyMemory(DirectoryEntry, CurrentDirectoryEntry, sizeof(EXT2_DIR_ENTRY));

            TRACE("EXT2 Directory Entry:\n");
            TRACE("inode = %d\n", DirectoryEntry->inode);
            TRACE("direntlen = %d\n", DirectoryEntry->direntlen);
            TRACE("namelen = %d\n", DirectoryEntry->namelen);
            TRACE("filetype = %d\n", DirectoryEntry->filetype);
            TRACE("name = ");
            for (CurrentOffset=0; CurrentOffset<DirectoryEntry->namelen; CurrentOffset++)
            {
                TRACE("%c", DirectoryEntry->name[CurrentOffset]);
            }
            TRACE("\n");

            return TRUE;
        }

        CurrentOffset += CurrentDirectoryEntry->direntlen;
    }

    return FALSE;
}

/*
 * Ext2ReadFileBig()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOLEAN Ext2ReadFileBig(PEXT2_FILE_INFO Ext2FileInfo, ULONGLONG BytesToRead, ULONGLONG* BytesRead, PVOID Buffer)
{
    ULONG                BlockNumber;
    ULONG                BlockNumberIndex;
    ULONG                OffsetInBlock;
    ULONG                LengthInBlock;
    ULONG                NumberOfBlocks;

    TRACE("Ext2ReadFileBig() BytesToRead = %d Buffer = 0x%x\n", (ULONG)BytesToRead, Buffer);

    if (BytesRead != NULL)
    {
        *BytesRead = 0;
    }

    // Make sure we have the block pointer list if we need it
    if (Ext2FileInfo->FileBlockList == NULL)
    {
        // Block pointer list is NULL
        // so this better be a fast symbolic link or else
        if (((Ext2FileInfo->Inode.mode & EXT2_S_IFMT) != EXT2_S_IFLNK) ||
            (Ext2FileInfo->FileSize > FAST_SYMLINK_MAX_NAME_SIZE))
        {
            FileSystemError("Block pointer list is NULL and file is not a fast symbolic link.");
            return FALSE;
        }
    }

    //
    // If they are trying to read past the
    // end of the file then return success
    // with BytesRead == 0
    //
    if (Ext2FileInfo->FilePointer >= Ext2FileInfo->FileSize)
    {
        return TRUE;
    }

    //
    // If they are trying to read more than there is to read
    // then adjust the amount to read
    //
    if ((Ext2FileInfo->FilePointer + BytesToRead) > Ext2FileInfo->FileSize)
    {
        BytesToRead = (Ext2FileInfo->FileSize - Ext2FileInfo->FilePointer);
    }

    // Check if this is a fast symbolic link
    // if so then the read is easy
    if (((Ext2FileInfo->Inode.mode & EXT2_S_IFMT) == EXT2_S_IFLNK) &&
        (Ext2FileInfo->FileSize <= FAST_SYMLINK_MAX_NAME_SIZE))
    {
        TRACE("Reading fast symbolic link data\n");

        // Copy the data from the link
        RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)Ext2FileInfo->FilePointer + Ext2FileInfo->Inode.symlink), (ULONG)BytesToRead);

        if (BytesRead != NULL)
        {
            *BytesRead = BytesToRead;
        }

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
    if (Ext2FileInfo->FilePointer % Ext2BlockSizeInBytes)
    {
        //
        // Do the math for our first read
        //
        BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
        BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];
        OffsetInBlock = (Ext2FileInfo->FilePointer % Ext2BlockSizeInBytes);
        LengthInBlock = (ULONG)((BytesToRead > (Ext2BlockSizeInBytes - OffsetInBlock)) ? (Ext2BlockSizeInBytes - OffsetInBlock) : BytesToRead);

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!Ext2ReadPartialBlock(BlockNumber, OffsetInBlock, LengthInBlock, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += LengthInBlock;
        }
        BytesToRead -= LengthInBlock;
        Ext2FileInfo->FilePointer += LengthInBlock;
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
        NumberOfBlocks = (ULONG)(BytesToRead / Ext2BlockSizeInBytes);

        while (NumberOfBlocks > 0)
        {
            BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
            BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];

            //
            // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
            //
            if (!Ext2ReadBlock(BlockNumber, Buffer))
            {
                return FALSE;
            }
            if (BytesRead != NULL)
            {
                *BytesRead += Ext2BlockSizeInBytes;
            }
            BytesToRead -= Ext2BlockSizeInBytes;
            Ext2FileInfo->FilePointer += Ext2BlockSizeInBytes;
            Buffer = (PVOID)((ULONG_PTR)Buffer + Ext2BlockSizeInBytes);
            NumberOfBlocks--;
        }
    }

    //
    // Do the math for our third read (if any data left)
    //
    if (BytesToRead > 0)
    {
        BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
        BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!Ext2ReadPartialBlock(BlockNumber, 0, (ULONG)BytesToRead, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += BytesToRead;
        }
        Ext2FileInfo->FilePointer += BytesToRead;
        BytesToRead -= BytesToRead;
        Buffer = (PVOID)((ULONG_PTR)Buffer + (ULONG_PTR)BytesToRead);
    }

    return TRUE;
}

BOOLEAN Ext2ReadVolumeSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    //GEOMETRY    DiskGeometry;
    //BOOLEAN        ReturnValue;
    //if (!DiskGetDriveGeometry(DriveNumber, &DiskGeometry))
    //{
    //    return FALSE;
    //}
    //ReturnValue = MachDiskReadLogicalSectors(DriveNumber, SectorNumber + Ext2VolumeStartSector, SectorCount, DiskReadBuffer);
    //RtlCopyMemory(Buffer, DiskReadBuffer, SectorCount * DiskGeometry.BytesPerSector);
    //return ReturnValue;

    return CacheReadDiskSectors(DriveNumber, SectorNumber + Ext2VolumeStartSector, SectorCount, Buffer);
}

BOOLEAN Ext2ReadSuperBlock(VOID)
{

    TRACE("Ext2ReadSuperBlock()\n");

    //
    // Free any memory previously allocated
    //
    if (Ext2SuperBlock != NULL)
    {
        FrLdrTempFree(Ext2SuperBlock, TAG_EXT_SUPER_BLOCK);

        Ext2SuperBlock = NULL;
    }

    //
    // Now allocate the memory to hold the super block
    //
    Ext2SuperBlock = (PEXT2_SUPER_BLOCK)FrLdrTempAlloc(1024, TAG_EXT_SUPER_BLOCK);

    //
    // Make sure we got the memory
    //
    if (Ext2SuperBlock == NULL)
    {
        FileSystemError("Out of memory.");
        return FALSE;
    }

    // Now try to read the super block
    // If this fails then abort
    if (!MachDiskReadLogicalSectors(Ext2DriveNumber, Ext2VolumeStartSector, 8, DiskReadBuffer))
    {
        return FALSE;
    }
    RtlCopyMemory(Ext2SuperBlock, ((PUCHAR)DiskReadBuffer + 1024), 1024);

    TRACE("Dumping super block:\n");
    TRACE("total_inodes: %d\n", Ext2SuperBlock->total_inodes);
    TRACE("total_blocks: %d\n", Ext2SuperBlock->total_blocks);
    TRACE("reserved_blocks: %d\n", Ext2SuperBlock->reserved_blocks);
    TRACE("free_blocks: %d\n", Ext2SuperBlock->free_blocks);
    TRACE("free_inodes: %d\n", Ext2SuperBlock->free_inodes);
    TRACE("first_data_block: %d\n", Ext2SuperBlock->first_data_block);
    TRACE("log2_block_size: %d\n", Ext2SuperBlock->log2_block_size);
    TRACE("log2_fragment_size: %d\n", Ext2SuperBlock->log2_fragment_size);
    TRACE("blocks_per_group: %d\n", Ext2SuperBlock->blocks_per_group);
    TRACE("fragments_per_group: %d\n", Ext2SuperBlock->fragments_per_group);
    TRACE("inodes_per_group: %d\n", Ext2SuperBlock->inodes_per_group);
    TRACE("mtime: %d\n", Ext2SuperBlock->mtime);
    TRACE("utime: %d\n", Ext2SuperBlock->utime);
    TRACE("mnt_count: %d\n", Ext2SuperBlock->mnt_count);
    TRACE("max_mnt_count: %d\n", Ext2SuperBlock->max_mnt_count);
    TRACE("magic: 0x%x\n", Ext2SuperBlock->magic);
    TRACE("fs_state: %d\n", Ext2SuperBlock->fs_state);
    TRACE("error_handling: %d\n", Ext2SuperBlock->error_handling);
    TRACE("minor_revision_level: %d\n", Ext2SuperBlock->minor_revision_level);
    TRACE("lastcheck: %d\n", Ext2SuperBlock->lastcheck);
    TRACE("checkinterval: %d\n", Ext2SuperBlock->checkinterval);
    TRACE("creator_os: %d\n", Ext2SuperBlock->creator_os);
    TRACE("revision_level: %d\n", Ext2SuperBlock->revision_level);
    TRACE("uid_reserved: %d\n", Ext2SuperBlock->uid_reserved);
    TRACE("gid_reserved: %d\n", Ext2SuperBlock->gid_reserved);
    TRACE("first_inode: %d\n", Ext2SuperBlock->first_inode);
    TRACE("inode_size: %d\n", Ext2SuperBlock->inode_size);
    TRACE("block_group_number: %d\n", Ext2SuperBlock->block_group_number);
    TRACE("feature_compatibility: 0x%x\n", Ext2SuperBlock->feature_compatibility);
    TRACE("feature_incompat: 0x%x\n", Ext2SuperBlock->feature_incompat);
    TRACE("feature_ro_compat: 0x%x\n", Ext2SuperBlock->feature_ro_compat);
    TRACE("unique_id = { 0x%x, 0x%x, 0x%x, 0x%x }\n",
        Ext2SuperBlock->unique_id[0], Ext2SuperBlock->unique_id[1], Ext2SuperBlock->unique_id[2], Ext2SuperBlock->unique_id[3]);
    TRACE("volume_name = '%.16s'\n", Ext2SuperBlock->volume_name);
    TRACE("last_mounted_on = '%.64s'\n", Ext2SuperBlock->last_mounted_on);
    TRACE("compression_info = 0x%x\n", Ext2SuperBlock->compression_info);

    //
    // Check the super block magic
    //
    if (Ext2SuperBlock->magic != EXT2_MAGIC)
    {
        FileSystemError("Invalid super block magic (0xef53)");
        return FALSE;
    }

    //
    // Check the revision level
    //
    if (Ext2SuperBlock->revision_level > EXT2_DYNAMIC_REVISION)
    {
        FileSystemError("FreeLoader does not understand the revision of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    //
    // Check the feature set
    // Don't need to check the compatible or read-only compatible features
    // because we only mount the filesystem as read-only
    //
    if ((Ext2SuperBlock->revision_level >= EXT2_DYNAMIC_REVISION) &&
        (/*((Ext2SuperBlock->s_feature_compat & ~EXT3_FEATURE_COMPAT_SUPP) != 0) ||*/
         /*((Ext2SuperBlock->s_feature_ro_compat & ~EXT3_FEATURE_RO_COMPAT_SUPP) != 0) ||*/
         ((Ext2SuperBlock->feature_incompat & ~EXT3_FEATURE_INCOMPAT_SUPP) != 0)))
    {
        FileSystemError("FreeLoader does not understand features of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    // Calculate the group count
    Ext2GroupCount = (Ext2SuperBlock->total_blocks - Ext2SuperBlock->first_data_block + Ext2SuperBlock->blocks_per_group - 1) / Ext2SuperBlock->blocks_per_group;
    TRACE("Ext2GroupCount: %d\n", Ext2GroupCount);

    // Calculate the block size
    Ext2BlockSizeInBytes = 1024 << Ext2SuperBlock->log2_block_size;
    Ext2BlockSizeInSectors = Ext2BlockSizeInBytes / Ext2DiskGeometry.BytesPerSector;
    TRACE("Ext2BlockSizeInBytes: %d\n", Ext2BlockSizeInBytes);
    TRACE("Ext2BlockSizeInSectors: %d\n", Ext2BlockSizeInSectors);

    // Calculate the fragment size
    if (Ext2SuperBlock->log2_fragment_size >= 0)
    {
        Ext2FragmentSizeInBytes = 1024 << Ext2SuperBlock->log2_fragment_size;
    }
    else
    {
        Ext2FragmentSizeInBytes = 1024 >> -(Ext2SuperBlock->log2_fragment_size);
    }
    Ext2FragmentSizeInSectors = Ext2FragmentSizeInBytes / Ext2DiskGeometry.BytesPerSector;
    TRACE("Ext2FragmentSizeInBytes: %d\n", Ext2FragmentSizeInBytes);
    TRACE("Ext2FragmentSizeInSectors: %d\n", Ext2FragmentSizeInSectors);

    // Verify that the fragment size and the block size are equal
    if (Ext2BlockSizeInBytes != Ext2FragmentSizeInBytes)
    {
        FileSystemError("The fragment size must be equal to the block size.");
        return FALSE;
    }

    // Calculate the number of inodes in one block
    Ext2InodesPerBlock = Ext2BlockSizeInBytes / EXT2_INODE_SIZE(Ext2SuperBlock);
    TRACE("Ext2InodesPerBlock: %d\n", Ext2InodesPerBlock);

    // Calculate the number of group descriptors in one block
    Ext2GroupDescPerBlock = EXT2_DESC_PER_BLOCK(Ext2SuperBlock);
    TRACE("Ext2GroupDescPerBlock: %d\n", Ext2GroupDescPerBlock);

    return TRUE;
}

BOOLEAN Ext2ReadGroupDescriptors(VOID)
{
    ULONG GroupDescBlockCount;
    ULONG BlockNumber;
    PUCHAR CurrentGroupDescBlock;

    TRACE("Ext2ReadGroupDescriptors()\n");

    //
    // Free any memory previously allocated
    //
    if (Ext2GroupDescriptors != NULL)
    {
        FrLdrTempFree(Ext2GroupDescriptors, TAG_EXT_GROUP_DESC);

        Ext2GroupDescriptors = NULL;
    }

    //
    // Now allocate the memory to hold the group descriptors
    //
    GroupDescBlockCount = ROUND_UP(Ext2GroupCount, Ext2GroupDescPerBlock) / Ext2GroupDescPerBlock;
    Ext2GroupDescriptors = (PEXT2_GROUP_DESC)FrLdrTempAlloc(GroupDescBlockCount * Ext2BlockSizeInBytes, TAG_EXT_GROUP_DESC);

    //
    // Make sure we got the memory
    //
    if (Ext2GroupDescriptors == NULL)
    {
        FileSystemError("Out of memory.");
        return FALSE;
    }

    // Now read the group descriptors
    CurrentGroupDescBlock = (PUCHAR)Ext2GroupDescriptors;
    BlockNumber = Ext2SuperBlock->first_data_block + 1;

    while (GroupDescBlockCount--)
    {
        if (!Ext2ReadBlock(BlockNumber, CurrentGroupDescBlock))
        {
            return FALSE;
        }

        BlockNumber++;
        CurrentGroupDescBlock += Ext2BlockSizeInBytes;
    }

    return TRUE;
}

BOOLEAN Ext2ReadDirectory(ULONG Inode, PVOID* DirectoryBuffer, PEXT2_INODE InodePointer)
{
    EXT2_FILE_INFO    DirectoryFileInfo;

    TRACE("Ext2ReadDirectory() Inode = %d\n", Inode);

    // Read the directory inode
    if (!Ext2ReadInode(Inode, InodePointer))
    {
        return FALSE;
    }

    // Make sure it is a directory inode
    if ((InodePointer->mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
    {
        FileSystemError("Inode is not a directory.");
        return FALSE;
    }

    // Fill in file info struct so we can call Ext2ReadFileBig()
    RtlZeroMemory(&DirectoryFileInfo, sizeof(EXT2_FILE_INFO));
    DirectoryFileInfo.DriveNumber = Ext2DriveNumber;
    DirectoryFileInfo.FileBlockList = Ext2ReadBlockPointerList(InodePointer);
    DirectoryFileInfo.FilePointer = 0;
    DirectoryFileInfo.FileSize = Ext2GetInodeFileSize(InodePointer);

    if (DirectoryFileInfo.FileBlockList == NULL)
    {
        return FALSE;
    }

    //
    // Now allocate the memory to hold the group descriptors
    //
    ASSERT(DirectoryFileInfo.FileSize <= 0xFFFFFFFF);
    *DirectoryBuffer = (PEXT2_DIR_ENTRY)FrLdrTempAlloc((ULONG)DirectoryFileInfo.FileSize, TAG_EXT_BUFFER);

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
    if (!Ext2ReadFileBig(&DirectoryFileInfo, DirectoryFileInfo.FileSize, NULL, *DirectoryBuffer))
    {
        FrLdrTempFree(*DirectoryBuffer, TAG_EXT_BUFFER);
        *DirectoryBuffer = NULL;
        FrLdrTempFree(DirectoryFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
        return FALSE;
    }

    FrLdrTempFree(DirectoryFileInfo.FileBlockList, TAG_EXT_BLOCK_LIST);
    return TRUE;
}

BOOLEAN Ext2ReadBlock(ULONG BlockNumber, PVOID Buffer)
{
    CHAR    ErrorString[80];

    TRACE("Ext2ReadBlock() BlockNumber = %d Buffer = 0x%x\n", BlockNumber, Buffer);

    // Make sure its a valid block
    if (BlockNumber > Ext2SuperBlock->total_blocks)
    {
        sprintf(ErrorString, "Error reading block %d - block out of range.", (int) BlockNumber);
        FileSystemError(ErrorString);
        return FALSE;
    }

    // Check to see if this is a sparse block
    if (BlockNumber == 0)
    {
        TRACE("Block is part of a sparse file. Zeroing input buffer.\n");

        RtlZeroMemory(Buffer, Ext2BlockSizeInBytes);

        return TRUE;
    }

    return Ext2ReadVolumeSectors(Ext2DriveNumber, (ULONGLONG)BlockNumber * Ext2BlockSizeInSectors, Ext2BlockSizeInSectors, Buffer);
}

/*
 * Ext2ReadPartialBlock()
 * Reads part of a block into memory
 */
BOOLEAN Ext2ReadPartialBlock(ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
    PVOID TempBuffer;

    TRACE("Ext2ReadPartialBlock() BlockNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", BlockNumber, StartingOffset, Length, Buffer);

    TempBuffer = FrLdrTempAlloc(Ext2BlockSizeInBytes, TAG_EXT_BUFFER);

    if (!Ext2ReadBlock(BlockNumber, TempBuffer))
    {
        return FALSE;
    }

    memcpy(Buffer, ((PUCHAR)TempBuffer + StartingOffset), Length);

    FrLdrTempFree(TempBuffer, TAG_EXT_BUFFER);

    return TRUE;
}

ULONG Ext2GetGroupDescBlockNumber(ULONG Group)
{
    return (((Group * sizeof(EXT2_GROUP_DESC)) / Ext2GroupDescPerBlock) + Ext2SuperBlock->first_data_block + 1);
}

ULONG Ext2GetGroupDescOffsetInBlock(ULONG Group)
{
    return ((Group * sizeof(EXT2_GROUP_DESC)) % Ext2GroupDescPerBlock);
}

ULONG Ext2GetInodeGroupNumber(ULONG Inode)
{
    return ((Inode - 1) / Ext2SuperBlock->inodes_per_group);
}

ULONG Ext2GetInodeBlockNumber(ULONG Inode)
{
    return (((Inode - 1) % Ext2SuperBlock->inodes_per_group) / Ext2InodesPerBlock);
}

ULONG Ext2GetInodeOffsetInBlock(ULONG Inode)
{
    return (((Inode - 1) % Ext2SuperBlock->inodes_per_group) % Ext2InodesPerBlock);
}

BOOLEAN Ext2ReadInode(ULONG Inode, PEXT2_INODE InodeBuffer)
{
    ULONG        InodeGroupNumber;
    ULONG        InodeBlockNumber;
    ULONG        InodeOffsetInBlock;
    CHAR        ErrorString[80];
    EXT2_GROUP_DESC    GroupDescriptor;

    TRACE("Ext2ReadInode() Inode = %d\n", Inode);

    // Make sure its a valid inode
    if ((Inode < 1) || (Inode > Ext2SuperBlock->total_inodes))
    {
        sprintf(ErrorString, "Error reading inode %ld - inode out of range.", Inode);
        FileSystemError(ErrorString);
        return FALSE;
    }

    // Get inode group & block number and offset in block
    InodeGroupNumber = Ext2GetInodeGroupNumber(Inode);
    InodeBlockNumber = Ext2GetInodeBlockNumber(Inode);
    InodeOffsetInBlock = Ext2GetInodeOffsetInBlock(Inode);
    TRACE("InodeGroupNumber = %d\n", InodeGroupNumber);
    TRACE("InodeBlockNumber = %d\n", InodeBlockNumber);
    TRACE("InodeOffsetInBlock = %d\n", InodeOffsetInBlock);

    // Read the group descriptor
    if (!Ext2ReadGroupDescriptor(InodeGroupNumber, &GroupDescriptor))
    {
        return FALSE;
    }

    // Add the start block of the inode table to the inode block number
    InodeBlockNumber += GroupDescriptor.inode_table_id;
    TRACE("InodeBlockNumber (after group desc correction) = %d\n", InodeBlockNumber);

    // Read the block
    if (!Ext2ReadPartialBlock(InodeBlockNumber,
                              (InodeOffsetInBlock * EXT2_INODE_SIZE(Ext2SuperBlock)),
                              sizeof(EXT2_INODE),
                              InodeBuffer))
    {
        return FALSE;
    }

    TRACE("Dumping inode information:\n");
    TRACE("mode = 0x%x\n", InodeBuffer->mode);
    TRACE("uid = %d\n", InodeBuffer->uid);
    TRACE("size = %d\n", InodeBuffer->size);
    TRACE("atime = %d\n", InodeBuffer->atime);
    TRACE("ctime = %d\n", InodeBuffer->ctime);
    TRACE("mtime = %d\n", InodeBuffer->mtime);
    TRACE("dtime = %d\n", InodeBuffer->dtime);
    TRACE("gid = %d\n", InodeBuffer->gid);
    TRACE("nlinks = %d\n", InodeBuffer->nlinks);
    TRACE("blockcnt = %d\n", InodeBuffer->blockcnt);
    TRACE("flags = 0x%x\n", InodeBuffer->flags);
    TRACE("osd1 = 0x%x\n", InodeBuffer->osd1);
    TRACE("dir_blocks = { %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u }\n",
        InodeBuffer->blocks.dir_blocks[0], InodeBuffer->blocks.dir_blocks[1], InodeBuffer->blocks.dir_blocks[ 2], InodeBuffer->blocks.dir_blocks[ 3],
        InodeBuffer->blocks.dir_blocks[4], InodeBuffer->blocks.dir_blocks[5], InodeBuffer->blocks.dir_blocks[ 6], InodeBuffer->blocks.dir_blocks[ 7],
        InodeBuffer->blocks.dir_blocks[8], InodeBuffer->blocks.dir_blocks[9], InodeBuffer->blocks.dir_blocks[10], InodeBuffer->blocks.dir_blocks[11]);
    TRACE("indir_block = %u\n", InodeBuffer->blocks.indir_block);
    TRACE("double_indir_block = %u\n", InodeBuffer->blocks.double_indir_block);
    TRACE("tripple_indir_block = %u\n", InodeBuffer->blocks.tripple_indir_block);
    TRACE("version = %d\n", InodeBuffer->version);
    TRACE("acl = %d\n", InodeBuffer->acl);
    TRACE("dir_acl = %d\n", InodeBuffer->dir_acl);
    TRACE("fragment_addr = %d\n", InodeBuffer->fragment_addr);
    TRACE("osd2 = { %d, %d, %d }\n",
        InodeBuffer->osd2[0], InodeBuffer->osd2[1], InodeBuffer->osd2[2]);

    return TRUE;
}

BOOLEAN Ext2ReadGroupDescriptor(ULONG Group, PEXT2_GROUP_DESC GroupBuffer)
{
    TRACE("Ext2ReadGroupDescriptor()\n");

    /*if (!Ext2ReadBlock(Ext2GetGroupDescBlockNumber(Group), (PVOID)FILESYSBUFFER))
    {
        return FALSE;
    }

    RtlCopyMemory(GroupBuffer, (PVOID)(FILESYSBUFFER + Ext2GetGroupDescOffsetInBlock(Group)), sizeof(EXT2_GROUP_DESC));*/

    RtlCopyMemory(GroupBuffer, &Ext2GroupDescriptors[Group], sizeof(EXT2_GROUP_DESC));

    TRACE("Dumping group descriptor:\n");
    TRACE("block_id = %d\n", GroupBuffer->block_id);
    TRACE("inode_id = %d\n", GroupBuffer->inode_id);
    TRACE("inode_table_id = %d\n", GroupBuffer->inode_table_id);
    TRACE("free_blocks = %d\n", GroupBuffer->free_blocks);
    TRACE("free_inodes = %d\n", GroupBuffer->free_inodes);
    TRACE("used_dirs = %d\n", GroupBuffer->used_dirs);

    return TRUE;
}

ULONG* Ext2ReadBlockPointerList(PEXT2_INODE Inode)
{
    ULONGLONG        FileSize;
    ULONG        BlockCount;
    ULONG*    BlockList;
    ULONG        CurrentBlockInList;
    ULONG        CurrentBlock;

    TRACE("Ext2ReadBlockPointerList()\n");

    // Get the number of blocks this file occupies
    // I would just use Inode->i_blocks but it
    // doesn't seem to be the number of blocks
    // the file size corresponds to, but instead
    // it is much bigger.
    //BlockCount = Inode->i_blocks;
    FileSize = Ext2GetInodeFileSize(Inode);
    FileSize = ROUND_UP(FileSize, Ext2BlockSizeInBytes);
    BlockCount = (ULONG)(FileSize / Ext2BlockSizeInBytes);

    // Allocate the memory for the block list
    BlockList = FrLdrTempAlloc(BlockCount * sizeof(ULONG), TAG_EXT_BLOCK_LIST);
    if (BlockList == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(BlockList, BlockCount * sizeof(ULONG));

    // Copy the direct block pointers
    for (CurrentBlockInList = CurrentBlock = 0;
         CurrentBlockInList < BlockCount && CurrentBlock < INDIRECT_BLOCKS;
         CurrentBlock++, CurrentBlockInList++)
    {
        BlockList[CurrentBlockInList] = Inode->blocks.dir_blocks[CurrentBlock];
    }

    // Copy the indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!Ext2CopyIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the double indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!Ext2CopyDoubleIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.double_indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the triple indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!Ext2CopyTripleIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.tripple_indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    return BlockList;
}

ULONGLONG Ext2GetInodeFileSize(PEXT2_INODE Inode)
{
    if ((Inode->mode & EXT2_S_IFMT) == EXT2_S_IFDIR)
    {
        return (ULONGLONG)(Inode->size);
    }
    else
    {
        return ((ULONGLONG)(Inode->size) | ((ULONGLONG)(Inode->dir_acl) << 32));
    }
}

BOOLEAN Ext2CopyIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = FrLdrTempAlloc(Ext2BlockSizeInBytes, TAG_EXT_BUFFER);
    if (!BlockBuffer)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(IndirectBlock, BlockBuffer))
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

BOOLEAN Ext2CopyDoubleIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyDoubleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Ext2BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(DoubleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!Ext2CopyIndirectBlockPointers(BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
        {
            FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    return TRUE;
}

BOOLEAN Ext2CopyTripleIndirectBlockPointers(ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyTripleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Ext2BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(TripleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!Ext2CopyDoubleIndirectBlockPointers(BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
        {
            FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    return TRUE;
}

ARC_STATUS Ext2Close(ULONG FileId)
{
    PEXT2_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    FrLdrTempFree(FileHandle, TAG_EXT_FILE);

    return ESUCCESS;
}

ARC_STATUS Ext2GetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PEXT2_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    Information->EndingAddress.QuadPart = FileHandle->FileSize;
    Information->CurrentAddress.QuadPart = FileHandle->FilePointer;

    TRACE("Ext2GetFileInformation() FileSize = %d\n",
        Information->EndingAddress.LowPart);
    TRACE("Ext2GetFileInformation() FilePointer = %d\n",
        Information->CurrentAddress.LowPart);

    return ESUCCESS;
}

ARC_STATUS Ext2Open(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PEXT2_FILE_INFO FileHandle;
    //ULONG DeviceId;

    if (OpenMode != OpenReadOnly)
        return EACCES;

    //DeviceId = FsGetDeviceId(*FileId);

    TRACE("Ext2Open() FileName = %s\n", Path);

    //
    // Call old open method
    //
    FileHandle = Ext2OpenFile(Path);

    //
    // Check for error
    //
    if (!FileHandle)
        return ENOENT;

    //
    // Success. Remember the handle
    //
    FsSetDeviceSpecific(*FileId, FileHandle);
    return ESUCCESS;
}

ARC_STATUS Ext2Read(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PEXT2_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
    ULONGLONG BytesReadBig;
    BOOLEAN Success;

    //
    // Read data
    //
    Success = Ext2ReadFileBig(FileHandle, N, &BytesReadBig, Buffer);
    *Count = (ULONG)BytesReadBig;

    //
    // Check for success
    //
    if (Success)
        return ESUCCESS;
    else
        return EIO;
}

ARC_STATUS Ext2Seek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PEXT2_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    TRACE("Ext2Seek() NewFilePointer = %lu\n", Position->LowPart);

    if (SeekMode != SeekAbsolute)
        return EINVAL;
    if (Position->HighPart != 0)
        return EINVAL;
    if (Position->LowPart >= FileHandle->FileSize)
        return EINVAL;

    FileHandle->FilePointer = Position->LowPart;
    return ESUCCESS;
}

const DEVVTBL Ext2FuncTable =
{
    Ext2Close,
    Ext2GetFileInformation,
    Ext2Open,
    Ext2Read,
    Ext2Seek,
    L"ext2",
};

const DEVVTBL* Ext2Mount(ULONG DeviceId)
{
    EXT2_SUPER_BLOCK SuperBlock;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    //
    // Read the SuperBlock
    //
    Position.HighPart = 0;
    Position.LowPart = 2 * 512;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
        return NULL;
    Status = ArcRead(DeviceId, &SuperBlock, sizeof(SuperBlock), &Count);
    if (Status != ESUCCESS || Count != sizeof(SuperBlock))
        return NULL;

    //
    // Check if SuperBlock is valid. If yes, return Ext2 function table
    //
    if (SuperBlock.magic == EXT2_MAGIC)
    {
        //
        // Compatibility hack as long as FS is not using underlying device DeviceId
        //
        UCHAR DriveNumber;
        ULONGLONG StartSector;
        ULONGLONG SectorCount;
        int Type;
        if (!DiskGetBootVolume(&DriveNumber, &StartSector, &SectorCount, &Type))
            return NULL;
        Ext2OpenVolume(DriveNumber, StartSector, SectorCount);
        return &Ext2FuncTable;
    }
    else
        return NULL;
}

#endif

