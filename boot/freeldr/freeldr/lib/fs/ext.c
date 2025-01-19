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

BOOLEAN    Ext2OpenVolume(PEXT2_VOLUME_INFO Volume);
PEXT2_FILE_INFO    Ext2OpenFile(PEXT2_VOLUME_INFO Volume, PCSTR FileName);
BOOLEAN    Ext2LookupFile(PEXT2_VOLUME_INFO Volume, PCSTR FileName, PEXT2_FILE_INFO Ext2FileInfo);
BOOLEAN    Ext2SearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PEXT2_DIR_ENTRY DirectoryEntry);
BOOLEAN    Ext2ReadVolumeSectors(PEXT2_VOLUME_INFO Volume, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);

BOOLEAN    Ext2ReadFileBig(PEXT2_FILE_INFO Ext2FileInfo, ULONGLONG BytesToRead, ULONGLONG* BytesRead, PVOID Buffer);
BOOLEAN    Ext2ReadSuperBlock(PEXT2_VOLUME_INFO Volume);
BOOLEAN    Ext2ReadGroupDescriptors(PEXT2_VOLUME_INFO Volume);
BOOLEAN    Ext2ReadDirectory(PEXT2_VOLUME_INFO Volume, ULONG Inode, PVOID* DirectoryBuffer, PEXT2_INODE InodePointer);
BOOLEAN    Ext2ReadBlock(PEXT2_VOLUME_INFO Volume, ULONG BlockNumber, PVOID Buffer);
BOOLEAN    Ext2ReadPartialBlock(PEXT2_VOLUME_INFO Volume, ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOLEAN    Ext2ReadInode(PEXT2_VOLUME_INFO Volume, ULONG Inode, PEXT2_INODE InodeBuffer);
BOOLEAN    Ext2ReadGroupDescriptor(PEXT2_VOLUME_INFO Volume, ULONG Group, PEXT2_GROUP_DESC GroupBuffer);
ULONG*    Ext2ReadBlockPointerList(PEXT2_VOLUME_INFO Volume, PEXT2_INODE Inode);
ULONGLONG        Ext2GetInodeFileSize(PEXT2_INODE Inode);
BOOLEAN    Ext2CopyIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock);
BOOLEAN    Ext2CopyDoubleIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock);
BOOLEAN    Ext2CopyTripleIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock);

typedef struct _EXT2_VOLUME_INFO
{
    ULONG BytesPerSector;  // Usually 512...

    PEXT2_SUPER_BLOCK SuperBlock;       // Ext2 file system super block
    PEXT2_GROUP_DESC  GroupDescriptors; // Ext2 file system group descriptors

    ULONG BlockSizeInBytes;         // Block size in bytes
    ULONG BlockSizeInSectors;       // Block size in sectors
    ULONG FragmentSizeInBytes;      // Fragment size in bytes
    ULONG FragmentSizeInSectors;    // Fragment size in sectors
    ULONG GroupCount;               // Number of groups in this file system
    ULONG InodesPerBlock;           // Number of inodes in one block
    ULONG GroupDescPerBlock;        // Number of group descriptors in one block

    ULONG DeviceId; // Ext2 file system device ID

} EXT2_VOLUME_INFO;

PEXT2_VOLUME_INFO Ext2Volumes[MAX_FDS];

#define TAG_EXT_BLOCK_LIST 'LtxE'
#define TAG_EXT_FILE 'FtxE'
#define TAG_EXT_BUFFER  'BtxE'
#define TAG_EXT_SUPER_BLOCK 'StxE'
#define TAG_EXT_GROUP_DESC 'GtxE'
#define TAG_EXT_VOLUME 'VtxE'

BOOLEAN Ext2OpenVolume(PEXT2_VOLUME_INFO Volume)
{
    TRACE("Ext2OpenVolume() DeviceId = %d\n", Volume->DeviceId);

#if 0
    /* Initialize the disk cache for this drive */
    if (!CacheInitializeDrive(DriveNumber))
    {
        return FALSE;
    }
#endif
    Volume->BytesPerSector = SECTOR_SIZE;

    /* Read in the super block */
    if (!Ext2ReadSuperBlock(Volume))
        return FALSE;

    /* Read in the group descriptors */
    if (!Ext2ReadGroupDescriptors(Volume))
        return FALSE;

    return TRUE;
}

/*
 * Ext2OpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
PEXT2_FILE_INFO Ext2OpenFile(PEXT2_VOLUME_INFO Volume, PCSTR FileName)
{
    EXT2_FILE_INFO        TempExt2FileInfo;
    PEXT2_FILE_INFO        FileHandle;
    CHAR            SymLinkPath[EXT2_NAME_LEN];
    CHAR            FullPath[EXT2_NAME_LEN * 2];
    ULONG_PTR        Index;

    TRACE("Ext2OpenFile() FileName = %s\n", FileName);

    RtlZeroMemory(SymLinkPath, sizeof(SymLinkPath));

    // Lookup the file in the file system
    if (!Ext2LookupFile(Volume, FileName, &TempExt2FileInfo))
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

        return Ext2OpenFile(Volume, FullPath);
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
BOOLEAN Ext2LookupFile(PEXT2_VOLUME_INFO Volume, PCSTR FileName, PEXT2_FILE_INFO Ext2FileInfo)
{
    UINT32        i;
    ULONG        NumberOfPathParts;
    CHAR        PathPart[261];
    PVOID        DirectoryBuffer;
    ULONG        DirectoryInode = EXT2_ROOT_INO;
    EXT2_INODE    InodeData;
    EXT2_DIR_ENTRY    DirectoryEntry;

    TRACE("Ext2LookupFile() FileName = %s\n", FileName);

    RtlZeroMemory(Ext2FileInfo, sizeof(EXT2_FILE_INFO));

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
        if (!Ext2ReadDirectory(Volume, DirectoryInode, &DirectoryBuffer, &InodeData))
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

    if (!Ext2ReadInode(Volume, DirectoryInode, &InodeData))
    {
        return FALSE;
    }

    if (((InodeData.mode & EXT2_S_IFMT) != EXT2_S_IFREG) &&
        ((InodeData.mode & EXT2_S_IFMT) != EXT2_S_IFLNK))
    {
        FileSystemError("Inode is not a regular file or symbolic link.");
        return FALSE;
    }

    // Set the associated volume
    Ext2FileInfo->Volume = Volume;

    // If it's a regular file or a regular symbolic link
    // then get the block pointer list otherwise it must
    // be a fast symbolic link which doesn't have a block list
    if (((InodeData.mode & EXT2_S_IFMT) == EXT2_S_IFREG) ||
        ((InodeData.mode & EXT2_S_IFMT) == EXT2_S_IFLNK && InodeData.size > FAST_SYMLINK_MAX_NAME_SIZE))
    {
        Ext2FileInfo->FileBlockList = Ext2ReadBlockPointerList(Volume, &InodeData);
        if (Ext2FileInfo->FileBlockList == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        Ext2FileInfo->FileBlockList = NULL;
    }

    Ext2FileInfo->FilePointer = 0;
    Ext2FileInfo->FileSize = Ext2GetInodeFileSize(&InodeData);
    RtlCopyMemory(&Ext2FileInfo->Inode, &InodeData, sizeof(EXT2_INODE));

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
    PEXT2_VOLUME_INFO Volume = Ext2FileInfo->Volume;
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
    // If the user is trying to read past the end of
    // the file then return success with BytesRead == 0.
    //
    if (Ext2FileInfo->FilePointer >= Ext2FileInfo->FileSize)
    {
        return TRUE;
    }

    //
    // If the user is trying to read more than there is to read
    // then adjust the amount to read.
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
        // Ext2FileInfo->FilePointer += BytesToRead;

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
    if (Ext2FileInfo->FilePointer % Volume->BlockSizeInBytes)
    {
        //
        // Do the math for our first read
        //
        BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Volume->BlockSizeInBytes);
        BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];
        OffsetInBlock = (Ext2FileInfo->FilePointer % Volume->BlockSizeInBytes);
        LengthInBlock = (ULONG)((BytesToRead > (Volume->BlockSizeInBytes - OffsetInBlock)) ? (Volume->BlockSizeInBytes - OffsetInBlock) : BytesToRead);

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!Ext2ReadPartialBlock(Volume, BlockNumber, OffsetInBlock, LengthInBlock, Buffer))
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
        NumberOfBlocks = (ULONG)(BytesToRead / Volume->BlockSizeInBytes);

        while (NumberOfBlocks > 0)
        {
            BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Volume->BlockSizeInBytes);
            BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];

            //
            // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
            //
            if (!Ext2ReadBlock(Volume, BlockNumber, Buffer))
            {
                return FALSE;
            }
            if (BytesRead != NULL)
            {
                *BytesRead += Volume->BlockSizeInBytes;
            }
            BytesToRead -= Volume->BlockSizeInBytes;
            Ext2FileInfo->FilePointer += Volume->BlockSizeInBytes;
            Buffer = (PVOID)((ULONG_PTR)Buffer + Volume->BlockSizeInBytes);
            NumberOfBlocks--;
        }
    }

    //
    // Do the math for our third read (if any data left)
    //
    if (BytesToRead > 0)
    {
        BlockNumberIndex = (ULONG)(Ext2FileInfo->FilePointer / Volume->BlockSizeInBytes);
        BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];

        //
        // Now do the read and update BytesRead & FilePointer
        //
        if (!Ext2ReadPartialBlock(Volume, BlockNumber, 0, (ULONG)BytesToRead, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += BytesToRead;
        }
        Ext2FileInfo->FilePointer += BytesToRead;
    }

    return TRUE;
}

BOOLEAN Ext2ReadVolumeSectors(PEXT2_VOLUME_INFO Volume, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
#if 0
    return CacheReadDiskSectors(DriveNumber, SectorNumber + Ext2VolumeStartSector, SectorCount, Buffer);
#endif

    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    /* Seek to right position */
    Position.QuadPart = (ULONGLONG)SectorNumber * 512;
    Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        TRACE("Ext2ReadVolumeSectors() Failed to seek\n");
        return FALSE;
    }

    /* Read data */
    Status = ArcRead(Volume->DeviceId, Buffer, SectorCount * 512, &Count);
    if (Status != ESUCCESS || Count != SectorCount * 512)
    {
        TRACE("Ext2ReadVolumeSectors() Failed to read\n");
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

BOOLEAN Ext2ReadSuperBlock(PEXT2_VOLUME_INFO Volume)
{
    PEXT2_SUPER_BLOCK SuperBlock = Volume->SuperBlock;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("Ext2ReadSuperBlock()\n");

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
        SuperBlock = (PEXT2_SUPER_BLOCK)FrLdrTempAlloc(1024, TAG_EXT_SUPER_BLOCK);
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
    TRACE("total_inodes: %d\n", SuperBlock->total_inodes);
    TRACE("total_blocks: %d\n", SuperBlock->total_blocks);
    TRACE("reserved_blocks: %d\n", SuperBlock->reserved_blocks);
    TRACE("free_blocks: %d\n", SuperBlock->free_blocks);
    TRACE("free_inodes: %d\n", SuperBlock->free_inodes);
    TRACE("first_data_block: %d\n", SuperBlock->first_data_block);
    TRACE("log2_block_size: %d\n", SuperBlock->log2_block_size);
    TRACE("log2_fragment_size: %d\n", SuperBlock->log2_fragment_size);
    TRACE("blocks_per_group: %d\n", SuperBlock->blocks_per_group);
    TRACE("fragments_per_group: %d\n", SuperBlock->fragments_per_group);
    TRACE("inodes_per_group: %d\n", SuperBlock->inodes_per_group);
    TRACE("mtime: %d\n", SuperBlock->mtime);
    TRACE("utime: %d\n", SuperBlock->utime);
    TRACE("mnt_count: %d\n", SuperBlock->mnt_count);
    TRACE("max_mnt_count: %d\n", SuperBlock->max_mnt_count);
    TRACE("magic: 0x%x\n", SuperBlock->magic);
    TRACE("fs_state: %d\n", SuperBlock->fs_state);
    TRACE("error_handling: %d\n", SuperBlock->error_handling);
    TRACE("minor_revision_level: %d\n", SuperBlock->minor_revision_level);
    TRACE("lastcheck: %d\n", SuperBlock->lastcheck);
    TRACE("checkinterval: %d\n", SuperBlock->checkinterval);
    TRACE("creator_os: %d\n", SuperBlock->creator_os);
    TRACE("revision_level: %d\n", SuperBlock->revision_level);
    TRACE("uid_reserved: %d\n", SuperBlock->uid_reserved);
    TRACE("gid_reserved: %d\n", SuperBlock->gid_reserved);
    TRACE("first_inode: %d\n", SuperBlock->first_inode);
    TRACE("inode_size: %d\n", SuperBlock->inode_size);
    TRACE("block_group_number: %d\n", SuperBlock->block_group_number);
    TRACE("feature_compatibility: 0x%x\n", SuperBlock->feature_compatibility);
    TRACE("feature_incompat: 0x%x\n", SuperBlock->feature_incompat);
    TRACE("feature_ro_compat: 0x%x\n", SuperBlock->feature_ro_compat);
    TRACE("unique_id = { 0x%x, 0x%x, 0x%x, 0x%x }\n",
        SuperBlock->unique_id[0], SuperBlock->unique_id[1],
        SuperBlock->unique_id[2], SuperBlock->unique_id[3]);
    TRACE("volume_name = '%.16s'\n", SuperBlock->volume_name);
    TRACE("last_mounted_on = '%.64s'\n", SuperBlock->last_mounted_on);
    TRACE("compression_info = 0x%x\n", SuperBlock->compression_info);

    //
    // Check the super block magic
    //
    if (SuperBlock->magic != EXT2_MAGIC)
    {
        FileSystemError("Invalid super block magic (0xef53)");
        return FALSE;
    }

    //
    // Check the revision level
    //
    if (SuperBlock->revision_level > EXT2_DYNAMIC_REVISION)
    {
        FileSystemError("FreeLoader does not understand the revision of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    //
    // Check the feature set
    // Don't need to check the compatible or read-only compatible features
    // because we only mount the filesystem as read-only
    //
    if ((SuperBlock->revision_level >= EXT2_DYNAMIC_REVISION) &&
        (/*((SuperBlock->s_feature_compat & ~EXT3_FEATURE_COMPAT_SUPP) != 0) ||*/
         /*((SuperBlock->s_feature_ro_compat & ~EXT3_FEATURE_RO_COMPAT_SUPP) != 0) ||*/
         ((SuperBlock->feature_incompat & ~EXT3_FEATURE_INCOMPAT_SUPP) != 0)))
    {
        FileSystemError("FreeLoader does not understand features of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    // Calculate the group count
    Volume->GroupCount = (SuperBlock->total_blocks - SuperBlock->first_data_block + SuperBlock->blocks_per_group - 1) / SuperBlock->blocks_per_group;
    TRACE("Ext2GroupCount: %d\n", Volume->GroupCount);

    // Calculate the block size
    Volume->BlockSizeInBytes = 1024 << SuperBlock->log2_block_size;
    Volume->BlockSizeInSectors = Volume->BlockSizeInBytes / Volume->BytesPerSector;
    TRACE("Ext2BlockSizeInBytes: %d\n", Volume->BlockSizeInBytes);
    TRACE("Ext2BlockSizeInSectors: %d\n", Volume->BlockSizeInSectors);

    // Calculate the fragment size
    if (SuperBlock->log2_fragment_size >= 0)
    {
        Volume->FragmentSizeInBytes = 1024 << SuperBlock->log2_fragment_size;
    }
    else
    {
        Volume->FragmentSizeInBytes = 1024 >> -(SuperBlock->log2_fragment_size);
    }
    Volume->FragmentSizeInSectors = Volume->FragmentSizeInBytes / Volume->BytesPerSector;
    TRACE("Ext2FragmentSizeInBytes: %d\n", Volume->FragmentSizeInBytes);
    TRACE("Ext2FragmentSizeInSectors: %d\n", Volume->FragmentSizeInSectors);

    // Verify that the fragment size and the block size are equal
    if (Volume->BlockSizeInBytes != Volume->FragmentSizeInBytes)
    {
        FileSystemError("The fragment size must be equal to the block size.");
        return FALSE;
    }

    // Calculate the number of inodes in one block
    Volume->InodesPerBlock = Volume->BlockSizeInBytes / EXT2_INODE_SIZE(SuperBlock);
    TRACE("Ext2InodesPerBlock: %d\n", Volume->InodesPerBlock);

    // Calculate the number of group descriptors in one block
    Volume->GroupDescPerBlock = EXT2_DESC_PER_BLOCK(SuperBlock);
    TRACE("Ext2GroupDescPerBlock: %d\n", Volume->GroupDescPerBlock);

    return TRUE;
}

BOOLEAN Ext2ReadGroupDescriptors(PEXT2_VOLUME_INFO Volume)
{
    ULONG GroupDescBlockCount;
    ULONG BlockNumber;
    PUCHAR CurrentGroupDescBlock;

    TRACE("Ext2ReadGroupDescriptors()\n");

    /* Free any memory previously allocated */
    if (Volume->GroupDescriptors != NULL)
    {
        FrLdrTempFree(Volume->GroupDescriptors, TAG_EXT_GROUP_DESC);
        Volume->GroupDescriptors = NULL;
    }

    /* Now allocate the memory to hold the group descriptors */
    GroupDescBlockCount = ROUND_UP(Volume->GroupCount, Volume->GroupDescPerBlock) / Volume->GroupDescPerBlock;
    Volume->GroupDescriptors = (PEXT2_GROUP_DESC)FrLdrTempAlloc(GroupDescBlockCount * Volume->BlockSizeInBytes, TAG_EXT_GROUP_DESC);
    if (Volume->GroupDescriptors == NULL)
    {
        FileSystemError("Out of memory.");
        return FALSE;
    }

    // Now read the group descriptors
    CurrentGroupDescBlock = (PUCHAR)Volume->GroupDescriptors;
    BlockNumber = Volume->SuperBlock->first_data_block + 1;

    while (GroupDescBlockCount--)
    {
        if (!Ext2ReadBlock(Volume, BlockNumber, CurrentGroupDescBlock))
        {
            return FALSE;
        }

        BlockNumber++;
        CurrentGroupDescBlock += Volume->BlockSizeInBytes;
    }

    return TRUE;
}

BOOLEAN Ext2ReadDirectory(PEXT2_VOLUME_INFO Volume, ULONG Inode, PVOID* DirectoryBuffer, PEXT2_INODE InodePointer)
{
    EXT2_FILE_INFO DirectoryFileInfo;

    TRACE("Ext2ReadDirectory() Inode = %d\n", Inode);

    // Read the directory inode
    if (!Ext2ReadInode(Volume, Inode, InodePointer))
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
    DirectoryFileInfo.Volume = Volume;
    DirectoryFileInfo.FileBlockList = Ext2ReadBlockPointerList(Volume, InodePointer);
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

BOOLEAN Ext2ReadBlock(PEXT2_VOLUME_INFO Volume, ULONG BlockNumber, PVOID Buffer)
{
    CHAR    ErrorString[80];

    TRACE("Ext2ReadBlock() BlockNumber = %d Buffer = 0x%x\n", BlockNumber, Buffer);

    // Make sure its a valid block
    if (BlockNumber > Volume->SuperBlock->total_blocks)
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

    return Ext2ReadVolumeSectors(Volume, (ULONGLONG)BlockNumber * Volume->BlockSizeInSectors, Volume->BlockSizeInSectors, Buffer);
}

/*
 * Ext2ReadPartialBlock()
 * Reads part of a block into memory
 */
BOOLEAN Ext2ReadPartialBlock(PEXT2_VOLUME_INFO Volume, ULONG BlockNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
    PVOID TempBuffer;

    TRACE("Ext2ReadPartialBlock() BlockNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", BlockNumber, StartingOffset, Length, Buffer);

    TempBuffer = FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);

    if (!Ext2ReadBlock(Volume, BlockNumber, TempBuffer))
    {
        FrLdrTempFree(TempBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    RtlCopyMemory(Buffer, ((PUCHAR)TempBuffer + StartingOffset), Length);

    FrLdrTempFree(TempBuffer, TAG_EXT_BUFFER);

    return TRUE;
}

#if 0
ULONG Ext2GetGroupDescBlockNumber(PEXT2_VOLUME_INFO Volume, ULONG Group)
{
    return (((Group * sizeof(EXT2_GROUP_DESC)) / Volume->GroupDescPerBlock) + Volume->SuperBlock->first_data_block + 1);
}

ULONG Ext2GetGroupDescOffsetInBlock(PEXT2_VOLUME_INFO Volume, ULONG Group)
{
    return ((Group * sizeof(EXT2_GROUP_DESC)) % Volume->GroupDescPerBlock);
}
#endif

ULONG Ext2GetInodeGroupNumber(PEXT2_VOLUME_INFO Volume, ULONG Inode)
{
    return ((Inode - 1) / Volume->SuperBlock->inodes_per_group);
}

ULONG Ext2GetInodeBlockNumber(PEXT2_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->inodes_per_group) / Volume->InodesPerBlock);
}

ULONG Ext2GetInodeOffsetInBlock(PEXT2_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->inodes_per_group) % Volume->InodesPerBlock);
}

BOOLEAN Ext2ReadInode(PEXT2_VOLUME_INFO Volume, ULONG Inode, PEXT2_INODE InodeBuffer)
{
    ULONG        InodeGroupNumber;
    ULONG        InodeBlockNumber;
    ULONG        InodeOffsetInBlock;
    CHAR        ErrorString[80];
    EXT2_GROUP_DESC    GroupDescriptor;

    TRACE("Ext2ReadInode() Inode = %d\n", Inode);

    // Make sure its a valid inode
    if ((Inode < 1) || (Inode > Volume->SuperBlock->total_inodes))
    {
        sprintf(ErrorString, "Error reading inode %ld - inode out of range.", Inode);
        FileSystemError(ErrorString);
        return FALSE;
    }

    // Get inode group & block number and offset in block
    InodeGroupNumber = Ext2GetInodeGroupNumber(Volume, Inode);
    InodeBlockNumber = Ext2GetInodeBlockNumber(Volume, Inode);
    InodeOffsetInBlock = Ext2GetInodeOffsetInBlock(Volume, Inode);
    TRACE("InodeGroupNumber = %d\n", InodeGroupNumber);
    TRACE("InodeBlockNumber = %d\n", InodeBlockNumber);
    TRACE("InodeOffsetInBlock = %d\n", InodeOffsetInBlock);

    // Read the group descriptor
    if (!Ext2ReadGroupDescriptor(Volume, InodeGroupNumber, &GroupDescriptor))
    {
        return FALSE;
    }

    // Add the start block of the inode table to the inode block number
    InodeBlockNumber += GroupDescriptor.inode_table_id;
    TRACE("InodeBlockNumber (after group desc correction) = %d\n", InodeBlockNumber);

    // Read the block
    if (!Ext2ReadPartialBlock(Volume,
                              InodeBlockNumber,
                              (InodeOffsetInBlock * EXT2_INODE_SIZE(Volume->SuperBlock)),
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

BOOLEAN Ext2ReadGroupDescriptor(PEXT2_VOLUME_INFO Volume, ULONG Group, PEXT2_GROUP_DESC GroupBuffer)
{
    TRACE("Ext2ReadGroupDescriptor()\n");

#if 0
    if (!Ext2ReadBlock(Volume, Ext2GetGroupDescBlockNumber(Volume, Group), (PVOID)FILESYSBUFFER))
    {
        return FALSE;
    }
    RtlCopyMemory(GroupBuffer, (PVOID)(FILESYSBUFFER + Ext2GetGroupDescOffsetInBlock(Volume, Group)), sizeof(EXT2_GROUP_DESC));
#endif

    RtlCopyMemory(GroupBuffer, &Volume->GroupDescriptors[Group], sizeof(EXT2_GROUP_DESC));

    TRACE("Dumping group descriptor:\n");
    TRACE("block_id = %d\n", GroupBuffer->block_id);
    TRACE("inode_id = %d\n", GroupBuffer->inode_id);
    TRACE("inode_table_id = %d\n", GroupBuffer->inode_table_id);
    TRACE("free_blocks = %d\n", GroupBuffer->free_blocks);
    TRACE("free_inodes = %d\n", GroupBuffer->free_inodes);
    TRACE("used_dirs = %d\n", GroupBuffer->used_dirs);

    return TRUE;
}

ULONG* Ext2ReadBlockPointerList(PEXT2_VOLUME_INFO Volume, PEXT2_INODE Inode)
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
    FileSize = ROUND_UP(FileSize, Volume->BlockSizeInBytes);
    BlockCount = (ULONG)(FileSize / Volume->BlockSizeInBytes);

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
        if (!Ext2CopyIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the double indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!Ext2CopyDoubleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.double_indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the triple indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!Ext2CopyTripleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.tripple_indir_block))
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

BOOLEAN Ext2CopyIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG IndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (!BlockBuffer)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(Volume, IndirectBlock, BlockBuffer))
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

BOOLEAN Ext2CopyDoubleIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG DoubleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyDoubleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(Volume, DoubleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!Ext2CopyIndirectBlockPointers(Volume, BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
        {
            FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
            return FALSE;
        }
    }

    FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
    return TRUE;
}

BOOLEAN Ext2CopyTripleIndirectBlockPointers(PEXT2_VOLUME_INFO Volume, ULONG* BlockList, ULONG* CurrentBlockInList, ULONG BlockCount, ULONG TripleIndirectBlock)
{
    ULONG*    BlockBuffer;
    ULONG    CurrentBlock;
    ULONG    BlockPointersPerBlock;

    TRACE("Ext2CopyTripleIndirectBlockPointers() BlockCount = %d\n", BlockCount);

    BlockPointersPerBlock = Volume->BlockSizeInBytes / sizeof(ULONG);

    BlockBuffer = (ULONG*)FrLdrTempAlloc(Volume->BlockSizeInBytes, TAG_EXT_BUFFER);
    if (BlockBuffer == NULL)
    {
        return FALSE;
    }

    if (!Ext2ReadBlock(Volume, TripleIndirectBlock, BlockBuffer))
    {
        FrLdrTempFree(BlockBuffer, TAG_EXT_BUFFER);
        return FALSE;
    }

    for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
    {
        if (!Ext2CopyDoubleIndirectBlockPointers(Volume, BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
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

    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = FileHandle->FileSize;
    Information->CurrentAddress.QuadPart = FileHandle->FilePointer;

    TRACE("Ext2GetFileInformation(%lu) -> FileSize = %llu, FilePointer = 0x%llx\n",
          FileId, Information->EndingAddress.QuadPart, Information->CurrentAddress.QuadPart);

    return ESUCCESS;
}

ARC_STATUS Ext2Open(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PEXT2_VOLUME_INFO Volume;
    PEXT2_FILE_INFO FileHandle;
    ULONG DeviceId;

    /* Check parameters */
    if (OpenMode != OpenReadOnly)
        return EACCES;

    /* Get underlying device */
    DeviceId = FsGetDeviceId(*FileId);
    Volume = Ext2Volumes[DeviceId];

    TRACE("Ext2Open() FileName = %s\n", Path);

    /* Call the internal open method */
    // Status = Ext2OpenFile(Volume, Path, &FileHandle);
    FileHandle = Ext2OpenFile(Volume, Path);
    if (!FileHandle)
        return ENOENT;

    /* Success, remember the handle */
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

const DEVVTBL Ext2FuncTable =
{
    Ext2Close,
    Ext2GetFileInformation,
    Ext2Open,
    Ext2Read,
    Ext2Seek,
    L"ext2fs",
};

const DEVVTBL* Ext2Mount(ULONG DeviceId)
{
    PEXT2_VOLUME_INFO Volume;
    EXT2_SUPER_BLOCK SuperBlock;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("Enter Ext2Mount(%lu)\n", DeviceId);

    /* Allocate data for volume information */
    Volume = FrLdrTempAlloc(sizeof(EXT2_VOLUME_INFO), TAG_EXT_VOLUME);
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(EXT2_VOLUME_INFO));

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

    /* Check if SuperBlock is valid. If yes, return Ext2 function table. */
    if (SuperBlock.magic != EXT2_MAGIC)
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }

    Volume->DeviceId = DeviceId;

    /* Really open the volume */
    if (!Ext2OpenVolume(Volume))
    {
        FrLdrTempFree(Volume, TAG_EXT_VOLUME);
        return NULL;
    }

    /* Remember EXT2 volume information */
    Ext2Volumes[DeviceId] = Volume;

    /* Return success */
    TRACE("Ext2Mount(%lu) success\n", DeviceId);
    return &Ext2FuncTable;
}

#endif
