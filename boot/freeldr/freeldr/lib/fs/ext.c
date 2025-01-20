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
    CHAR            SymLinkPath[EXT_NAME_LEN];
    CHAR            FullPath[EXT_NAME_LEN * 2];
    ULONG_PTR        Index;

    TRACE("ExtOpenFile() FileName = %s\n", FileName);

    RtlZeroMemory(SymLinkPath, sizeof(SymLinkPath));

    // Lookup the file in the file system
    if (!ExtLookupFile(Volume, FileName, &TempExtFileInfo))
    {
        return NULL;
    }

    // If we got a symbolic link then fix up the path
    // and re-call this function
    if ((TempExtFileInfo.Inode.mode & EXT_S_IFMT) == EXT_S_IFLNK)
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
    ULONG        DirectoryInode = EXT_ROOT_INO;
    EXT_INODE    InodeData;
    EXT_DIR_ENTRY    DirectoryEntry;

    TRACE("ExtLookupFile() FileName = %s\n", FileName);

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

        DirectoryInode = DirectoryEntry.inode;
    }

    if (!ExtReadInode(Volume, DirectoryInode, &InodeData))
    {
        return FALSE;
    }

    if (((InodeData.mode & EXT_S_IFMT) != EXT_S_IFREG) &&
        ((InodeData.mode & EXT_S_IFMT) != EXT_S_IFLNK))
    {
        FileSystemError("Inode is not a regular file or symbolic link.");
        return FALSE;
    }

    // Set the associated volume
    ExtFileInfo->Volume = Volume;

    // If it's a regular file or a regular symbolic link
    // then get the block pointer list otherwise it must
    // be a fast symbolic link which doesn't have a block list
    if (((InodeData.mode & EXT_S_IFMT) == EXT_S_IFREG) ||
        ((InodeData.mode & EXT_S_IFMT) == EXT_S_IFLNK && InodeData.size > FAST_SYMLINK_MAX_NAME_SIZE))
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
    ULONG        CurrentOffset;
    PEXT_DIR_ENTRY    CurrentDirectoryEntry;

    TRACE("ExtSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x DirectorySize = %d FileName = %s\n", DirectoryBuffer, DirectorySize, FileName);

    for (CurrentOffset=0; CurrentOffset<DirectorySize; )
    {
        CurrentDirectoryEntry = (PEXT_DIR_ENTRY)((ULONG_PTR)DirectoryBuffer + CurrentOffset);

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
            RtlCopyMemory(DirectoryEntry, CurrentDirectoryEntry, sizeof(EXT_DIR_ENTRY));

            TRACE("EXT Directory Entry:\n");
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
        if (((ExtFileInfo->Inode.mode & EXT_S_IFMT) != EXT_S_IFLNK) ||
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
    if (((ExtFileInfo->Inode.mode & EXT_S_IFMT) == EXT_S_IFLNK) &&
        (ExtFileInfo->FileSize <= FAST_SYMLINK_MAX_NAME_SIZE))
    {
        TRACE("Reading fast symbolic link data\n");

        // Copy the data from the link
        RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)ExtFileInfo->FilePointer + ExtFileInfo->Inode.symlink), (ULONG)BytesToRead);

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
    if (SuperBlock->magic != EXT_MAGIC)
    {
        FileSystemError("Invalid super block magic (0xef53)");
        return FALSE;
    }

    //
    // Check the revision level
    //
    if (SuperBlock->revision_level > EXT_DYNAMIC_REVISION)
    {
        FileSystemError("FreeLoader does not understand the revision of this EXT/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    //
    // Check the feature set
    // Don't need to check the compatible or read-only compatible features
    // because we only mount the filesystem as read-only
    //
    if ((SuperBlock->revision_level >= EXT_DYNAMIC_REVISION) &&
        (/*((SuperBlock->s_feature_compat & ~EXT3_FEATURE_COMPAT_SUPP) != 0) ||*/
         /*((SuperBlock->s_feature_ro_compat & ~EXT3_FEATURE_RO_COMPAT_SUPP) != 0) ||*/
         ((SuperBlock->feature_incompat & ~EXT3_FEATURE_INCOMPAT_SUPP) != 0)))
    {
        FileSystemError("FreeLoader does not understand features of this EXT/EXT3 filesystem.\nPlease update FreeLoader.");
        return FALSE;
    }

    // Calculate the group count
    Volume->GroupCount = (SuperBlock->total_blocks - SuperBlock->first_data_block + SuperBlock->blocks_per_group - 1) / SuperBlock->blocks_per_group;
    TRACE("ExtGroupCount: %d\n", Volume->GroupCount);

    // Calculate the block size
    Volume->BlockSizeInBytes = 1024 << SuperBlock->log2_block_size;
    Volume->BlockSizeInSectors = Volume->BlockSizeInBytes / Volume->BytesPerSector;
    TRACE("ExtBlockSizeInBytes: %d\n", Volume->BlockSizeInBytes);
    TRACE("ExtBlockSizeInSectors: %d\n", Volume->BlockSizeInSectors);

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
    TRACE("ExtFragmentSizeInBytes: %d\n", Volume->FragmentSizeInBytes);
    TRACE("ExtFragmentSizeInSectors: %d\n", Volume->FragmentSizeInSectors);

    // Verify that the fragment size and the block size are equal
    if (Volume->BlockSizeInBytes != Volume->FragmentSizeInBytes)
    {
        FileSystemError("The fragment size must be equal to the block size.");
        return FALSE;
    }

    // Calculate the number of inodes in one block
    Volume->InodesPerBlock = Volume->BlockSizeInBytes / EXT_INODE_SIZE(SuperBlock);
    TRACE("ExtInodesPerBlock: %d\n", Volume->InodesPerBlock);

    // Calculate the number of group descriptors in one block
    Volume->GroupDescPerBlock = EXT_DESC_PER_BLOCK(SuperBlock);
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
    BlockNumber = Volume->SuperBlock->first_data_block + 1;

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
    if ((InodePointer->mode & EXT_S_IFMT) != EXT_S_IFDIR)
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

#if 0
ULONG ExtGetGroupDescBlockNumber(PEXT_VOLUME_INFO Volume, ULONG Group)
{
    return (((Group * sizeof(EXT_GROUP_DESC)) / Volume->GroupDescPerBlock) + Volume->SuperBlock->first_data_block + 1);
}

ULONG ExtGetGroupDescOffsetInBlock(PEXT_VOLUME_INFO Volume, ULONG Group)
{
    return ((Group * sizeof(EXT_GROUP_DESC)) % Volume->GroupDescPerBlock);
}
#endif

ULONG ExtGetInodeGroupNumber(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return ((Inode - 1) / Volume->SuperBlock->inodes_per_group);
}

ULONG ExtGetInodeBlockNumber(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->inodes_per_group) / Volume->InodesPerBlock);
}

ULONG ExtGetInodeOffsetInBlock(PEXT_VOLUME_INFO Volume, ULONG Inode)
{
    return (((Inode - 1) % Volume->SuperBlock->inodes_per_group) % Volume->InodesPerBlock);
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
    if ((Inode < 1) || (Inode > Volume->SuperBlock->total_inodes))
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
    InodeBlockNumber += GroupDescriptor.inode_table_id;
    TRACE("InodeBlockNumber (after group desc correction) = %d\n", InodeBlockNumber);

    // Read the block
    if (!ExtReadPartialBlock(Volume,
                              InodeBlockNumber,
                              (InodeOffsetInBlock * EXT_INODE_SIZE(Volume->SuperBlock)),
                              sizeof(EXT_INODE),
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

    RtlCopyMemory(GroupBuffer, &Volume->GroupDescriptors[Group], sizeof(EXT_GROUP_DESC));

    TRACE("Dumping group descriptor:\n");
    TRACE("block_id = %d\n", GroupBuffer->block_id);
    TRACE("inode_id = %d\n", GroupBuffer->inode_id);
    TRACE("inode_table_id = %d\n", GroupBuffer->inode_table_id);
    TRACE("free_blocks = %d\n", GroupBuffer->free_blocks);
    TRACE("free_inodes = %d\n", GroupBuffer->free_inodes);
    TRACE("used_dirs = %d\n", GroupBuffer->used_dirs);

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
        if (!ExtCopyIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the double indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!ExtCopyDoubleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.double_indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    // Copy the triple indirect block pointers
    if (CurrentBlockInList < BlockCount)
    {
        if (!ExtCopyTripleIndirectBlockPointers(Volume, BlockList, &CurrentBlockInList, BlockCount, Inode->blocks.tripple_indir_block))
        {
            FrLdrTempFree(BlockList, TAG_EXT_BLOCK_LIST);
            return NULL;
        }
    }

    return BlockList;
}

ULONGLONG ExtGetInodeFileSize(PEXT_INODE Inode)
{
    if ((Inode->mode & EXT_S_IFMT) == EXT_S_IFDIR)
    {
        return (ULONGLONG)(Inode->size);
    }
    else
    {
        return ((ULONGLONG)(Inode->size) | ((ULONGLONG)(Inode->dir_acl) << 32));
    }
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

    TRACE("ExtOpen() FileName = %s\n", Path);

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
    if (SuperBlock.magic != EXT_MAGIC)
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

#endif
