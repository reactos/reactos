/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include <fs.h>
#include "ext2.h"
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
#include <mm.h>
#include <debug.h>
#include <cache.h>


GEOMETRY			Ext2DiskGeometry;				// Ext2 file system disk geometry

PEXT2_SUPER_BLOCK	Ext2SuperBlock = NULL;			// Ext2 file system super block
PEXT2_GROUP_DESC	Ext2GroupDescriptors = NULL;	// Ext2 file system group descriptors

U8					Ext2DriveNumber = 0;			// Ext2 file system drive number
U64					Ext2VolumeStartSector = 0;		// Ext2 file system starting sector
U32					Ext2BlockSizeInBytes = 0;		// Block size in bytes
U32					Ext2BlockSizeInSectors = 0;		// Block size in sectors
U32					Ext2FragmentSizeInBytes = 0;	// Fragment size in bytes
U32					Ext2FragmentSizeInSectors = 0;	// Fragment size in sectors
U32					Ext2GroupCount = 0;				// Number of groups in this file system
U32					Ext2InodesPerBlock = 0;			// Number of inodes in one block
U32					Ext2GroupDescPerBlock = 0;		// Number of group descriptors in one block

BOOL Ext2OpenVolume(U8 DriveNumber, U64 VolumeStartSector)
{

	DbgPrint((DPRINT_FILESYSTEM, "Ext2OpenVolume() DriveNumber = 0x%x VolumeStartSector = %d\n", DriveNumber, VolumeStartSector));

	// Store the drive number and start sector
	Ext2DriveNumber = DriveNumber;
	Ext2VolumeStartSector = VolumeStartSector;

	if (!DiskGetDriveGeometry(DriveNumber, &Ext2DiskGeometry))
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
FILE* Ext2OpenFile(PUCHAR FileName)
{
	EXT2_FILE_INFO		TempExt2FileInfo;
	PEXT2_FILE_INFO		FileHandle;
	UCHAR				SymLinkPath[EXT3_NAME_LEN];
	UCHAR				FullPath[EXT3_NAME_LEN * 2];
	U32					Index;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2OpenFile() FileName = %s\n", FileName));

	RtlZeroMemory(SymLinkPath, EXT3_NAME_LEN);

	// Lookup the file in the file system
	if (!Ext2LookupFile(FileName, &TempExt2FileInfo))
	{
		return NULL;
	}

	// If we got a symbolic link then fix up the path
	// and re-call this function
	if ((TempExt2FileInfo.Inode.i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK)
	{
		DbgPrint((DPRINT_FILESYSTEM, "File is a symbolic link\n"));

		// Now read in the symbolic link path
		if (!Ext2ReadFile(&TempExt2FileInfo, TempExt2FileInfo.FileSize, NULL, SymLinkPath))
		{
			if (TempExt2FileInfo.FileBlockList != NULL)
			{
				MmFreeMemory(TempExt2FileInfo.FileBlockList);
			}

			return NULL;
		}

		DbgPrint((DPRINT_FILESYSTEM, "Symbolic link path = %s\n", SymLinkPath));

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

		DbgPrint((DPRINT_FILESYSTEM, "Full file path = %s\n", FullPath));

		if (TempExt2FileInfo.FileBlockList != NULL)
		{
			MmFreeMemory(TempExt2FileInfo.FileBlockList);
		}

		return Ext2OpenFile(FullPath);
	}
	else
	{
		FileHandle = MmAllocateMemory(sizeof(EXT2_FILE_INFO));

		if (FileHandle == NULL)
		{
			if (TempExt2FileInfo.FileBlockList != NULL)
			{
				MmFreeMemory(TempExt2FileInfo.FileBlockList);
			}

			return NULL;
		}

		RtlCopyMemory(FileHandle, &TempExt2FileInfo, sizeof(EXT2_FILE_INFO));

		return (FILE*)FileHandle;
	}
}

/*
 * Ext2LookupFile()
 * This function searches the file system for the
 * specified filename and fills in a EXT2_FILE_INFO structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
BOOL Ext2LookupFile(PUCHAR FileName, PEXT2_FILE_INFO Ext2FileInfoPointer)
{
	int				i;
	U32				NumberOfPathParts;
	UCHAR			PathPart[261];
	PVOID			DirectoryBuffer;
	U32				DirectoryInode = EXT3_ROOT_INO;
	EXT2_INODE		InodeData;
	EXT2_DIR_ENTRY	DirectoryEntry;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2LookupFile() FileName = %s\n", FileName));

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
		if (!Ext2SearchDirectoryBufferForFile(DirectoryBuffer, (U32)Ext2GetInodeFileSize(&InodeData), PathPart, &DirectoryEntry))
		{
			MmFreeMemory(DirectoryBuffer);
			return FALSE;
		}

		MmFreeMemory(DirectoryBuffer);

		DirectoryInode = DirectoryEntry.inode;
	}

	if (!Ext2ReadInode(DirectoryInode, &InodeData))
	{
		return FALSE;
	}

	if (((InodeData.i_mode & EXT2_S_IFMT) != EXT2_S_IFREG) &&
		((InodeData.i_mode & EXT2_S_IFMT) != EXT2_S_IFLNK))
	{
		FileSystemError("Inode is not a regular file or symbolic link.");
		return FALSE;
	}

	// Set the drive number
	Ext2FileInfoPointer->DriveNumber = Ext2DriveNumber;

	// If it's a regular file or a regular symbolic link
	// then get the block pointer list otherwise it must
	// be a fast symbolic link which doesn't have a block list
	if (((InodeData.i_mode & EXT2_S_IFMT) == EXT2_S_IFREG) ||
		((InodeData.i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK && InodeData.i_size > FAST_SYMLINK_MAX_NAME_SIZE))
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

BOOL Ext2SearchDirectoryBufferForFile(PVOID DirectoryBuffer, U32 DirectorySize, PUCHAR FileName, PEXT2_DIR_ENTRY DirectoryEntry)
{
	U32				CurrentOffset;
	PEXT2_DIR_ENTRY	CurrentDirectoryEntry;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2SearchDirectoryBufferForFile() DirectoryBuffer = 0x%x DirectorySize = %d FileName = %s\n", DirectoryBuffer, DirectorySize, FileName));

	for (CurrentOffset=0; CurrentOffset<DirectorySize; )
	{
		CurrentDirectoryEntry = (PEXT2_DIR_ENTRY)(DirectoryBuffer + CurrentOffset);

		if (CurrentDirectoryEntry->rec_len == 0)
		{
			break;
		}

		if ((CurrentDirectoryEntry->rec_len + CurrentOffset) > DirectorySize)
		{
			FileSystemError("Directory entry extends past end of directory file.");
			return FALSE;
		}

		DbgPrint((DPRINT_FILESYSTEM, "Dumping directory entry at offset %d:\n", CurrentOffset));
		DbgDumpBuffer(DPRINT_FILESYSTEM, CurrentDirectoryEntry, CurrentDirectoryEntry->rec_len);

		if ((strnicmp(FileName, CurrentDirectoryEntry->name, CurrentDirectoryEntry->name_len) == 0) &&
			(strlen(FileName) == CurrentDirectoryEntry->name_len))
		{
			RtlCopyMemory(DirectoryEntry, CurrentDirectoryEntry, sizeof(EXT2_DIR_ENTRY));

			DbgPrint((DPRINT_FILESYSTEM, "EXT2 Directory Entry:\n"));
			DbgPrint((DPRINT_FILESYSTEM, "inode = %d\n", DirectoryEntry->inode));
			DbgPrint((DPRINT_FILESYSTEM, "rec_len = %d\n", DirectoryEntry->rec_len));
			DbgPrint((DPRINT_FILESYSTEM, "name_len = %d\n", DirectoryEntry->name_len));
			DbgPrint((DPRINT_FILESYSTEM, "file_type = %d\n", DirectoryEntry->file_type));
			DbgPrint((DPRINT_FILESYSTEM, "name = "));
			for (CurrentOffset=0; CurrentOffset<DirectoryEntry->name_len; CurrentOffset++)
			{
				DbgPrint((DPRINT_FILESYSTEM, "%c", DirectoryEntry->name[CurrentOffset]));
			}
			DbgPrint((DPRINT_FILESYSTEM, "\n"));

			return TRUE;
		}

		CurrentOffset += CurrentDirectoryEntry->rec_len;
	}

	return FALSE;
}

/*
 * Ext2ReadFile()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOL Ext2ReadFile(FILE *FileHandle, U64 BytesToRead, U64* BytesRead, PVOID Buffer)
{
	PEXT2_FILE_INFO	Ext2FileInfo = (PEXT2_FILE_INFO)FileHandle;
	U32				BlockNumber;
	U32				BlockNumberIndex;
	U32				OffsetInBlock;
	U32				LengthInBlock;
	U32				NumberOfBlocks;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadFile() BytesToRead = %d Buffer = 0x%x\n", (U32)BytesToRead, Buffer));

	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	// Make sure we have the block pointer list if we need it
	if (Ext2FileInfo->FileBlockList == NULL)
	{
		// Block pointer list is NULL
		// so this better be a fast symbolic link or else
		if (((Ext2FileInfo->Inode.i_mode & EXT2_S_IFMT) != EXT2_S_IFLNK) ||
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
	if (((Ext2FileInfo->Inode.i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK) &&
		(Ext2FileInfo->FileSize <= FAST_SYMLINK_MAX_NAME_SIZE))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Reading fast symbolic link data\n"));

		// Copy the data from the link
		RtlCopyMemory(Buffer, (PVOID)(Ext2FileInfo->Inode.i_block) + Ext2FileInfo->FilePointer, BytesToRead);

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
		BlockNumberIndex = (Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
		BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];
		OffsetInBlock = (Ext2FileInfo->FilePointer % Ext2BlockSizeInBytes);
		LengthInBlock = (BytesToRead > (Ext2BlockSizeInBytes - OffsetInBlock)) ? (Ext2BlockSizeInBytes - OffsetInBlock) : BytesToRead;

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
		Buffer += LengthInBlock;
	}

	//
	// Do the math for our second read (if any data left)
	//
	if (BytesToRead > 0)
	{
		//
		// Determine how many full clusters we need to read
		//
		NumberOfBlocks = (BytesToRead / Ext2BlockSizeInBytes);

		while (NumberOfBlocks > 0)
		{
			BlockNumberIndex = (Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
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
			Buffer += Ext2BlockSizeInBytes;
			NumberOfBlocks--;
		}
	}

	//
	// Do the math for our third read (if any data left)
	//
	if (BytesToRead > 0)
	{
		BlockNumberIndex = (Ext2FileInfo->FilePointer / Ext2BlockSizeInBytes);
		BlockNumber = Ext2FileInfo->FileBlockList[BlockNumberIndex];

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!Ext2ReadPartialBlock(BlockNumber, 0, BytesToRead, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += BytesToRead;
		}
		Ext2FileInfo->FilePointer += BytesToRead;
		BytesToRead -= BytesToRead;
		Buffer += BytesToRead;
	}

	return TRUE;
}

U64 Ext2GetFileSize(FILE *FileHandle)
{
	PEXT2_FILE_INFO	Ext2FileHandle = (PEXT2_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2GetFileSize() FileSize = %d\n", Ext2FileHandle->FileSize));

	return Ext2FileHandle->FileSize;
}

VOID Ext2SetFilePointer(FILE *FileHandle, U64 NewFilePointer)
{
	PEXT2_FILE_INFO	Ext2FileHandle = (PEXT2_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2SetFilePointer() NewFilePointer = %d\n", NewFilePointer));

	Ext2FileHandle->FilePointer = NewFilePointer;
}

U64 Ext2GetFilePointer(FILE *FileHandle)
{
	PEXT2_FILE_INFO	Ext2FileHandle = (PEXT2_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2GetFilePointer() FilePointer = %d\n", Ext2FileHandle->FilePointer));

	return Ext2FileHandle->FilePointer;
}

BOOL Ext2ReadVolumeSectors(U8 DriveNumber, U64 SectorNumber, U64 SectorCount, PVOID Buffer)
{
	//GEOMETRY	DiskGeometry;
	//BOOL		ReturnValue;
	//if (!DiskGetDriveGeometry(DriveNumber, &DiskGeometry))
	//{
	//	return FALSE;
	//}
	//ReturnValue = DiskReadLogicalSectors(DriveNumber, SectorNumber + Ext2VolumeStartSector, SectorCount, (PVOID)DISKREADBUFFER);
	//RtlCopyMemory(Buffer, (PVOID)DISKREADBUFFER, SectorCount * DiskGeometry.BytesPerSector);
	//return ReturnValue;

	return CacheReadDiskSectors(DriveNumber, SectorNumber + Ext2VolumeStartSector, SectorCount, Buffer);
}

BOOL Ext2ReadSuperBlock(VOID)
{

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadSuperBlock()\n"));

	//
	// Free any memory previously allocated
	//
	if (Ext2SuperBlock != NULL)
	{
		MmFreeMemory(Ext2SuperBlock);

		Ext2SuperBlock = NULL;
	}

	//
	// Now allocate the memory to hold the super block
	//
	Ext2SuperBlock = (PEXT2_SUPER_BLOCK)MmAllocateMemory(1024);

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
	if (!DiskReadLogicalSectors(Ext2DriveNumber, Ext2VolumeStartSector, 8, (PVOID)DISKREADBUFFER))
	{
		return FALSE;
	}
	RtlCopyMemory(Ext2SuperBlock, (PVOID)(DISKREADBUFFER + 1024), 1024);

	DbgPrint((DPRINT_FILESYSTEM, "Dumping super block:\n"));

	DbgPrint((DPRINT_FILESYSTEM, "s_inodes_count: %d\n", Ext2SuperBlock->s_inodes_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_blocks_count: %d\n", Ext2SuperBlock->s_blocks_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_r_blocks_count: %d\n", Ext2SuperBlock->s_r_blocks_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_free_blocks_count: %d\n", Ext2SuperBlock->s_free_blocks_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_free_inodes_count: %d\n", Ext2SuperBlock->s_free_inodes_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_first_data_block: %d\n", Ext2SuperBlock->s_first_data_block));
	DbgPrint((DPRINT_FILESYSTEM, "s_log_block_size: %d\n", Ext2SuperBlock->s_log_block_size));
	DbgPrint((DPRINT_FILESYSTEM, "s_log_frag_size: %d\n", Ext2SuperBlock->s_log_frag_size));
	DbgPrint((DPRINT_FILESYSTEM, "s_blocks_per_group: %d\n", Ext2SuperBlock->s_blocks_per_group));
	DbgPrint((DPRINT_FILESYSTEM, "s_frags_per_group: %d\n", Ext2SuperBlock->s_frags_per_group));
	DbgPrint((DPRINT_FILESYSTEM, "s_inodes_per_group: %d\n", Ext2SuperBlock->s_inodes_per_group));
	DbgPrint((DPRINT_FILESYSTEM, "s_mtime: %d\n", Ext2SuperBlock->s_mtime));
	DbgPrint((DPRINT_FILESYSTEM, "s_wtime: %d\n", Ext2SuperBlock->s_wtime));
	DbgPrint((DPRINT_FILESYSTEM, "s_mnt_count: %d\n", Ext2SuperBlock->s_mnt_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_max_mnt_count: %d\n", Ext2SuperBlock->s_max_mnt_count));
	DbgPrint((DPRINT_FILESYSTEM, "s_magic: 0x%x\n", Ext2SuperBlock->s_magic));
	DbgPrint((DPRINT_FILESYSTEM, "s_state: %d\n", Ext2SuperBlock->s_state));
	DbgPrint((DPRINT_FILESYSTEM, "s_errors: %d\n", Ext2SuperBlock->s_errors));
	DbgPrint((DPRINT_FILESYSTEM, "s_minor_rev_level: %d\n", Ext2SuperBlock->s_minor_rev_level));
	DbgPrint((DPRINT_FILESYSTEM, "s_lastcheck: %d\n", Ext2SuperBlock->s_lastcheck));
	DbgPrint((DPRINT_FILESYSTEM, "s_checkinterval: %d\n", Ext2SuperBlock->s_checkinterval));
	DbgPrint((DPRINT_FILESYSTEM, "s_creator_os: %d\n", Ext2SuperBlock->s_creator_os));
	DbgPrint((DPRINT_FILESYSTEM, "s_rev_level: %d\n", Ext2SuperBlock->s_rev_level));
	DbgPrint((DPRINT_FILESYSTEM, "s_def_resuid: %d\n", Ext2SuperBlock->s_def_resuid));
	DbgPrint((DPRINT_FILESYSTEM, "s_def_resgid: %d\n", Ext2SuperBlock->s_def_resgid));
	DbgPrint((DPRINT_FILESYSTEM, "s_first_ino: %d\n", Ext2SuperBlock->s_first_ino));
	DbgPrint((DPRINT_FILESYSTEM, "s_inode_size: %d\n", Ext2SuperBlock->s_inode_size));
	DbgPrint((DPRINT_FILESYSTEM, "s_block_group_nr: %d\n", Ext2SuperBlock->s_block_group_nr));
	DbgPrint((DPRINT_FILESYSTEM, "s_feature_compat: 0x%x\n", Ext2SuperBlock->s_feature_compat));
	DbgPrint((DPRINT_FILESYSTEM, "s_feature_incompat: 0x%x\n", Ext2SuperBlock->s_feature_incompat));
	DbgPrint((DPRINT_FILESYSTEM, "s_feature_ro_compat: 0x%x\n", Ext2SuperBlock->s_feature_ro_compat));
	DbgPrint((DPRINT_FILESYSTEM, "s_uuid[16] = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", Ext2SuperBlock->s_uuid[0], Ext2SuperBlock->s_uuid[1], Ext2SuperBlock->s_uuid[2], Ext2SuperBlock->s_uuid[3], Ext2SuperBlock->s_uuid[4], Ext2SuperBlock->s_uuid[5], Ext2SuperBlock->s_uuid[6], Ext2SuperBlock->s_uuid[7], Ext2SuperBlock->s_uuid[8], Ext2SuperBlock->s_uuid[9], Ext2SuperBlock->s_uuid[10], Ext2SuperBlock->s_uuid[11], Ext2SuperBlock->s_uuid[12], Ext2SuperBlock->s_uuid[13], Ext2SuperBlock->s_uuid[14], Ext2SuperBlock->s_uuid[15]));
	DbgPrint((DPRINT_FILESYSTEM, "s_volume_name[16] = '%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c'\n", Ext2SuperBlock->s_volume_name[0], Ext2SuperBlock->s_volume_name[1], Ext2SuperBlock->s_volume_name[2], Ext2SuperBlock->s_volume_name[3], Ext2SuperBlock->s_volume_name[4], Ext2SuperBlock->s_volume_name[5], Ext2SuperBlock->s_volume_name[6], Ext2SuperBlock->s_volume_name[7], Ext2SuperBlock->s_volume_name[8], Ext2SuperBlock->s_volume_name[9], Ext2SuperBlock->s_volume_name[10], Ext2SuperBlock->s_volume_name[11], Ext2SuperBlock->s_volume_name[12], Ext2SuperBlock->s_volume_name[13], Ext2SuperBlock->s_volume_name[14], Ext2SuperBlock->s_volume_name[15]));
	DbgPrint((DPRINT_FILESYSTEM, "s_last_mounted[64]='%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c'\n", Ext2SuperBlock->s_last_mounted[0], Ext2SuperBlock->s_last_mounted[1], Ext2SuperBlock->s_last_mounted[2], Ext2SuperBlock->s_last_mounted[3], Ext2SuperBlock->s_last_mounted[4], Ext2SuperBlock->s_last_mounted[5], Ext2SuperBlock->s_last_mounted[6], Ext2SuperBlock->s_last_mounted[7], Ext2SuperBlock->s_last_mounted[8], Ext2SuperBlock->s_last_mounted[9],
		Ext2SuperBlock->s_last_mounted[10], Ext2SuperBlock->s_last_mounted[11], Ext2SuperBlock->s_last_mounted[12], Ext2SuperBlock->s_last_mounted[13], Ext2SuperBlock->s_last_mounted[14], Ext2SuperBlock->s_last_mounted[15], Ext2SuperBlock->s_last_mounted[16], Ext2SuperBlock->s_last_mounted[17], Ext2SuperBlock->s_last_mounted[18], Ext2SuperBlock->s_last_mounted[19],
		Ext2SuperBlock->s_last_mounted[20], Ext2SuperBlock->s_last_mounted[21], Ext2SuperBlock->s_last_mounted[22], Ext2SuperBlock->s_last_mounted[23], Ext2SuperBlock->s_last_mounted[24], Ext2SuperBlock->s_last_mounted[25], Ext2SuperBlock->s_last_mounted[26], Ext2SuperBlock->s_last_mounted[27], Ext2SuperBlock->s_last_mounted[28], Ext2SuperBlock->s_last_mounted[29],
		Ext2SuperBlock->s_last_mounted[30], Ext2SuperBlock->s_last_mounted[31], Ext2SuperBlock->s_last_mounted[32], Ext2SuperBlock->s_last_mounted[33], Ext2SuperBlock->s_last_mounted[34], Ext2SuperBlock->s_last_mounted[35], Ext2SuperBlock->s_last_mounted[36], Ext2SuperBlock->s_last_mounted[37], Ext2SuperBlock->s_last_mounted[38], Ext2SuperBlock->s_last_mounted[39],
		Ext2SuperBlock->s_last_mounted[40], Ext2SuperBlock->s_last_mounted[41], Ext2SuperBlock->s_last_mounted[42], Ext2SuperBlock->s_last_mounted[43], Ext2SuperBlock->s_last_mounted[44], Ext2SuperBlock->s_last_mounted[45], Ext2SuperBlock->s_last_mounted[46], Ext2SuperBlock->s_last_mounted[47], Ext2SuperBlock->s_last_mounted[48], Ext2SuperBlock->s_last_mounted[49],
		Ext2SuperBlock->s_last_mounted[50], Ext2SuperBlock->s_last_mounted[51], Ext2SuperBlock->s_last_mounted[52], Ext2SuperBlock->s_last_mounted[53], Ext2SuperBlock->s_last_mounted[54], Ext2SuperBlock->s_last_mounted[55], Ext2SuperBlock->s_last_mounted[56], Ext2SuperBlock->s_last_mounted[57], Ext2SuperBlock->s_last_mounted[58], Ext2SuperBlock->s_last_mounted[59],
		Ext2SuperBlock->s_last_mounted[60], Ext2SuperBlock->s_last_mounted[61], Ext2SuperBlock->s_last_mounted[62], Ext2SuperBlock->s_last_mounted[63]));
	DbgPrint((DPRINT_FILESYSTEM, "s_algorithm_usage_bitmap = 0x%x\n", Ext2SuperBlock->s_algorithm_usage_bitmap));
	DbgPrint((DPRINT_FILESYSTEM, "s_prealloc_blocks = %d\n", Ext2SuperBlock->s_prealloc_blocks));
	DbgPrint((DPRINT_FILESYSTEM, "s_prealloc_dir_blocks = %d\n", Ext2SuperBlock->s_prealloc_dir_blocks));
	DbgPrint((DPRINT_FILESYSTEM, "s_padding1 = %d\n", Ext2SuperBlock->s_padding1));
	DbgPrint((DPRINT_FILESYSTEM, "s_journal_uuid[16] = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", Ext2SuperBlock->s_journal_uuid[0], Ext2SuperBlock->s_journal_uuid[1], Ext2SuperBlock->s_journal_uuid[2], Ext2SuperBlock->s_journal_uuid[3], Ext2SuperBlock->s_journal_uuid[4], Ext2SuperBlock->s_journal_uuid[5], Ext2SuperBlock->s_journal_uuid[6], Ext2SuperBlock->s_journal_uuid[7], Ext2SuperBlock->s_journal_uuid[8], Ext2SuperBlock->s_journal_uuid[9], Ext2SuperBlock->s_journal_uuid[10], Ext2SuperBlock->s_journal_uuid[11], Ext2SuperBlock->s_journal_uuid[12], Ext2SuperBlock->s_journal_uuid[13], Ext2SuperBlock->s_journal_uuid[14], Ext2SuperBlock->s_journal_uuid[15]));
	DbgPrint((DPRINT_FILESYSTEM, "s_journal_inum = %d\n", Ext2SuperBlock->s_journal_inum));
	DbgPrint((DPRINT_FILESYSTEM, "s_journal_dev = %d\n", Ext2SuperBlock->s_journal_dev));
	DbgPrint((DPRINT_FILESYSTEM, "s_last_orphan = %d\n", Ext2SuperBlock->s_last_orphan));

	//
	// Check the super block magic
	//
	if (Ext2SuperBlock->s_magic != EXT3_SUPER_MAGIC)
	{
		FileSystemError("Invalid super block magic (0xef53)");
		return FALSE;
	}

	//
	// Check the revision level
	//
	if (Ext2SuperBlock->s_rev_level > EXT3_DYNAMIC_REV)
	{
		FileSystemError("FreeLoader does not understand the revision of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
		return FALSE;
	}

	//
	// Check the feature set
	// Don't need to check the compatible or read-only compatible features
	// because we only mount the filesystem as read-only
	//
	if ((Ext2SuperBlock->s_rev_level >= EXT3_DYNAMIC_REV) &&
		(/*((Ext2SuperBlock->s_feature_compat & ~EXT3_FEATURE_COMPAT_SUPP) != 0) ||*/
		 /*((Ext2SuperBlock->s_feature_ro_compat & ~EXT3_FEATURE_RO_COMPAT_SUPP) != 0) ||*/
		 ((Ext2SuperBlock->s_feature_incompat & ~EXT3_FEATURE_INCOMPAT_SUPP) != 0)))
	{
		FileSystemError("FreeLoader does not understand features of this EXT2/EXT3 filesystem.\nPlease update FreeLoader.");
		return FALSE;
	}

	// Calculate the group count
	Ext2GroupCount = (Ext2SuperBlock->s_blocks_count - Ext2SuperBlock->s_first_data_block + Ext2SuperBlock->s_blocks_per_group - 1) / Ext2SuperBlock->s_blocks_per_group;
	DbgPrint((DPRINT_FILESYSTEM, "Ext2GroupCount: %d\n", Ext2GroupCount));

	// Calculate the block size
	Ext2BlockSizeInBytes = 1024 << Ext2SuperBlock->s_log_block_size;
	Ext2BlockSizeInSectors = Ext2BlockSizeInBytes / Ext2DiskGeometry.BytesPerSector;
	DbgPrint((DPRINT_FILESYSTEM, "Ext2BlockSizeInBytes: %d\n", Ext2BlockSizeInBytes));
	DbgPrint((DPRINT_FILESYSTEM, "Ext2BlockSizeInSectors: %d\n", Ext2BlockSizeInSectors));

	// Calculate the fragment size
	if (Ext2SuperBlock->s_log_frag_size >= 0)
	{
		Ext2FragmentSizeInBytes = 1024 << Ext2SuperBlock->s_log_frag_size;
	}
	else
	{
		Ext2FragmentSizeInBytes = 1024 >> -(Ext2SuperBlock->s_log_frag_size);
	}
	Ext2FragmentSizeInSectors = Ext2FragmentSizeInBytes / Ext2DiskGeometry.BytesPerSector;
	DbgPrint((DPRINT_FILESYSTEM, "Ext2FragmentSizeInBytes: %d\n", Ext2FragmentSizeInBytes));
	DbgPrint((DPRINT_FILESYSTEM, "Ext2FragmentSizeInSectors: %d\n", Ext2FragmentSizeInSectors));

	// Verify that the fragment size and the block size are equal
	if (Ext2BlockSizeInBytes != Ext2FragmentSizeInBytes)
	{
		FileSystemError("The fragment size must be equal to the block size.");
		return FALSE;
	}

	// Calculate the number of inodes in one block
	Ext2InodesPerBlock = Ext2BlockSizeInBytes / EXT3_INODE_SIZE(Ext2SuperBlock);
	DbgPrint((DPRINT_FILESYSTEM, "Ext2InodesPerBlock: %d\n", Ext2InodesPerBlock));

	// Calculate the number of group descriptors in one block
	Ext2GroupDescPerBlock = EXT3_DESC_PER_BLOCK(Ext2SuperBlock);
	DbgPrint((DPRINT_FILESYSTEM, "Ext2GroupDescPerBlock: %d\n", Ext2GroupDescPerBlock));

	return TRUE;
}

BOOL Ext2ReadGroupDescriptors(VOID)
{
	U32		GroupDescBlockCount;
	U32		CurrentGroupDescBlock;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadGroupDescriptors()\n"));

	//
	// Free any memory previously allocated
	//
	if (Ext2GroupDescriptors != NULL)
	{
		MmFreeMemory(Ext2GroupDescriptors);

		Ext2GroupDescriptors = NULL;
	}

	//
	// Now allocate the memory to hold the group descriptors
	//
	GroupDescBlockCount = ROUND_UP(Ext2GroupCount, Ext2GroupDescPerBlock) / Ext2GroupDescPerBlock;
	Ext2GroupDescriptors = (PEXT2_GROUP_DESC)MmAllocateMemory(GroupDescBlockCount * Ext2BlockSizeInBytes);

	//
	// Make sure we got the memory
	//
	if (Ext2GroupDescriptors == NULL)
	{
		FileSystemError("Out of memory.");
		return FALSE;
	}

	// Now read the group descriptors
	for (CurrentGroupDescBlock=0; CurrentGroupDescBlock<GroupDescBlockCount; CurrentGroupDescBlock++)
	{
		if (!Ext2ReadBlock(Ext2SuperBlock->s_first_data_block + 1 + CurrentGroupDescBlock, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		RtlCopyMemory((Ext2GroupDescriptors + (CurrentGroupDescBlock * Ext2BlockSizeInBytes)), (PVOID)FILESYSBUFFER, Ext2BlockSizeInBytes);
	}

	return TRUE;
}

BOOL Ext2ReadDirectory(U32 Inode, PVOID* DirectoryBuffer, PEXT2_INODE InodePointer)
{
	EXT2_FILE_INFO	DirectoryFileInfo;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadDirectory() Inode = %d\n", Inode));

	// Read the directory inode
	if (!Ext2ReadInode(Inode, InodePointer))
	{
		return FALSE;
	}

	// Make sure it is a directory inode
	if ((InodePointer->i_mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
	{
		FileSystemError("Inode is not a directory.");
		return FALSE;
	}

	// Fill in file info struct so we can call Ext2ReadFile()
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
	*DirectoryBuffer = (PEXT2_DIR_ENTRY)MmAllocateMemory(DirectoryFileInfo.FileSize);

	//
	// Make sure we got the memory
	//
	if (*DirectoryBuffer == NULL)
	{
		MmFreeMemory(DirectoryFileInfo.FileBlockList);
		FileSystemError("Out of memory.");
		return FALSE;
	}

	// Now read the root directory data
	if (!Ext2ReadFile(&DirectoryFileInfo, DirectoryFileInfo.FileSize, NULL, *DirectoryBuffer))
	{
		MmFreeMemory(*DirectoryBuffer);
		*DirectoryBuffer = NULL;
		MmFreeMemory(DirectoryFileInfo.FileBlockList);
		return FALSE;
	}

	MmFreeMemory(DirectoryFileInfo.FileBlockList);
	return TRUE;
}

BOOL Ext2ReadBlock(U32 BlockNumber, PVOID Buffer)
{
	UCHAR			ErrorString[80];

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadBlock() BlockNumber = %d Buffer = 0x%x\n", BlockNumber, Buffer));

	// Make sure its a valid block
	if (BlockNumber > Ext2SuperBlock->s_blocks_count)
	{
		sprintf(ErrorString, "Error reading block %d - block out of range.", BlockNumber);
		FileSystemError(ErrorString);
		return FALSE;
	}

	// Check to see if this is a sparse block
	if (BlockNumber == 0)
	{
		DbgPrint((DPRINT_FILESYSTEM, "Block is part of a sparse file. Zeroing input buffer.\n"));

		RtlZeroMemory(Buffer, Ext2BlockSizeInBytes);

		return TRUE;
	}

	return Ext2ReadVolumeSectors(Ext2DriveNumber, (U64)BlockNumber * Ext2BlockSizeInSectors, Ext2BlockSizeInSectors, Buffer);
}

/*
 * Ext2ReadPartialBlock()
 * Reads part of a block into memory
 */
BOOL Ext2ReadPartialBlock(U32 BlockNumber, U32 StartingOffset, U32 Length, PVOID Buffer)
{

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadPartialBlock() BlockNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", BlockNumber, StartingOffset, Length, Buffer));

	if (!Ext2ReadBlock(BlockNumber, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	memcpy(Buffer, ((PVOID)FILESYSBUFFER + StartingOffset), Length);

	return TRUE;
}

U32 Ext2GetGroupDescBlockNumber(U32 Group)
{
	return (((Group * sizeof(EXT2_GROUP_DESC)) / Ext2GroupDescPerBlock) + Ext2SuperBlock->s_first_data_block + 1);
}

U32 Ext2GetGroupDescOffsetInBlock(U32 Group)
{
	return ((Group * sizeof(EXT2_GROUP_DESC)) % Ext2GroupDescPerBlock);
}

U32 Ext2GetInodeGroupNumber(U32 Inode)
{
	return ((Inode - 1) / Ext2SuperBlock->s_inodes_per_group);
}

U32 Ext2GetInodeBlockNumber(U32 Inode)
{
	return (((Inode - 1) % Ext2SuperBlock->s_inodes_per_group) / Ext2InodesPerBlock);
}

U32 Ext2GetInodeOffsetInBlock(U32 Inode)
{
	return (((Inode - 1) % Ext2SuperBlock->s_inodes_per_group) % Ext2InodesPerBlock);
}

BOOL Ext2ReadInode(U32 Inode, PEXT2_INODE InodeBuffer)
{
	U32				InodeGroupNumber;
	U32				InodeBlockNumber;
	U32				InodeOffsetInBlock;
	UCHAR			ErrorString[80];
	EXT2_GROUP_DESC	GroupDescriptor;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadInode() Inode = %d\n", Inode));

	// Make sure its a valid inode
	if ((Inode < 1) || (Inode > Ext2SuperBlock->s_inodes_count))
	{
		sprintf(ErrorString, "Error reading inode %ld - inode out of range.", Inode);
		FileSystemError(ErrorString);
		return FALSE;
	}

	// Get inode group & block number and offset in block
	InodeGroupNumber = Ext2GetInodeGroupNumber(Inode);
	InodeBlockNumber = Ext2GetInodeBlockNumber(Inode);
	InodeOffsetInBlock = Ext2GetInodeOffsetInBlock(Inode);
	DbgPrint((DPRINT_FILESYSTEM, "InodeGroupNumber = %d\n", InodeGroupNumber));
	DbgPrint((DPRINT_FILESYSTEM, "InodeBlockNumber = %d\n", InodeBlockNumber));
	DbgPrint((DPRINT_FILESYSTEM, "InodeOffsetInBlock = %d\n", InodeOffsetInBlock));

	// Read the group descriptor
	if (!Ext2ReadGroupDescriptor(InodeGroupNumber, &GroupDescriptor))
	{
		return FALSE;
	}

	// Add the start block of the inode table to the inode block number
	InodeBlockNumber += GroupDescriptor.bg_inode_table;
	DbgPrint((DPRINT_FILESYSTEM, "InodeBlockNumber (after group desc correction) = %d\n", InodeBlockNumber));

	// Read the block
	if (!Ext2ReadBlock(InodeBlockNumber, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}
	
	// Copy the data to their buffer
	RtlCopyMemory(InodeBuffer, (PVOID)(FILESYSBUFFER + (InodeOffsetInBlock * EXT3_INODE_SIZE(Ext2SuperBlock))), sizeof(EXT2_INODE));

	DbgPrint((DPRINT_FILESYSTEM, "Dumping inode information:\n"));
	DbgPrint((DPRINT_FILESYSTEM, "i_mode = 0x%x\n", InodeBuffer->i_mode));
	DbgPrint((DPRINT_FILESYSTEM, "i_uid = %d\n", InodeBuffer->i_uid));
	DbgPrint((DPRINT_FILESYSTEM, "i_size = %d\n", InodeBuffer->i_size));
	DbgPrint((DPRINT_FILESYSTEM, "i_atime = %d\n", InodeBuffer->i_atime));
	DbgPrint((DPRINT_FILESYSTEM, "i_ctime = %d\n", InodeBuffer->i_ctime));
	DbgPrint((DPRINT_FILESYSTEM, "i_mtime = %d\n", InodeBuffer->i_mtime));
	DbgPrint((DPRINT_FILESYSTEM, "i_dtime = %d\n", InodeBuffer->i_dtime));
	DbgPrint((DPRINT_FILESYSTEM, "i_gid = %d\n", InodeBuffer->i_gid));
	DbgPrint((DPRINT_FILESYSTEM, "i_links_count = %d\n", InodeBuffer->i_links_count));
	DbgPrint((DPRINT_FILESYSTEM, "i_blocks = %d\n", InodeBuffer->i_blocks));
	DbgPrint((DPRINT_FILESYSTEM, "i_flags = 0x%x\n", InodeBuffer->i_flags));
	DbgPrint((DPRINT_FILESYSTEM, "i_block[EXT3_N_BLOCKS (%d)] =\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d,\n%d\n", EXT3_N_BLOCKS, InodeBuffer->i_block[0], InodeBuffer->i_block[1], InodeBuffer->i_block[2], InodeBuffer->i_block[3], InodeBuffer->i_block[4], InodeBuffer->i_block[5], InodeBuffer->i_block[6], InodeBuffer->i_block[7], InodeBuffer->i_block[8], InodeBuffer->i_block[9], InodeBuffer->i_block[10], InodeBuffer->i_block[11], InodeBuffer->i_block[12], InodeBuffer->i_block[13], InodeBuffer->i_block[14]));
	DbgPrint((DPRINT_FILESYSTEM, "i_generation = %d\n", InodeBuffer->i_generation));
	DbgPrint((DPRINT_FILESYSTEM, "i_file_acl = %d\n", InodeBuffer->i_file_acl));
	DbgPrint((DPRINT_FILESYSTEM, "i_dir_acl = %d\n", InodeBuffer->i_dir_acl));
	DbgPrint((DPRINT_FILESYSTEM, "i_faddr = %d\n", InodeBuffer->i_faddr));
	DbgPrint((DPRINT_FILESYSTEM, "l_i_frag = %d\n", InodeBuffer->osd2.linux2.l_i_frag));
	DbgPrint((DPRINT_FILESYSTEM, "l_i_fsize = %d\n", InodeBuffer->osd2.linux2.l_i_fsize));
	DbgPrint((DPRINT_FILESYSTEM, "l_i_uid_high = %d\n", InodeBuffer->osd2.linux2.l_i_uid_high));
	DbgPrint((DPRINT_FILESYSTEM, "l_i_gid_high = %d\n", InodeBuffer->osd2.linux2.l_i_gid_high));

	return TRUE;
}

BOOL Ext2ReadGroupDescriptor(U32 Group, PEXT2_GROUP_DESC GroupBuffer)
{
	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadGroupDescriptor()\n"));

	/*if (!Ext2ReadBlock(Ext2GetGroupDescBlockNumber(Group), (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	RtlCopyMemory(GroupBuffer, (PVOID)(FILESYSBUFFER + Ext2GetGroupDescOffsetInBlock(Group)), sizeof(EXT2_GROUP_DESC));*/

	RtlCopyMemory(GroupBuffer, &Ext2GroupDescriptors[Group], sizeof(EXT2_GROUP_DESC));

	DbgPrint((DPRINT_FILESYSTEM, "Dumping group descriptor:\n"));
	DbgPrint((DPRINT_FILESYSTEM, "bg_block_bitmap = %d\n", GroupBuffer->bg_block_bitmap));
	DbgPrint((DPRINT_FILESYSTEM, "bg_inode_bitmap = %d\n", GroupBuffer->bg_inode_bitmap));
	DbgPrint((DPRINT_FILESYSTEM, "bg_inode_table = %d\n", GroupBuffer->bg_inode_table));
	DbgPrint((DPRINT_FILESYSTEM, "bg_free_blocks_count = %d\n", GroupBuffer->bg_free_blocks_count));
	DbgPrint((DPRINT_FILESYSTEM, "bg_free_inodes_count = %d\n", GroupBuffer->bg_free_inodes_count));
	DbgPrint((DPRINT_FILESYSTEM, "bg_used_dirs_count = %d\n", GroupBuffer->bg_used_dirs_count));

	return TRUE;
}

U32* Ext2ReadBlockPointerList(PEXT2_INODE Inode)
{
	U64		FileSize;
	U32		BlockCount;
	U32*	BlockList;
	U32		CurrentBlockInList;
	U32		CurrentBlock;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2ReadBlockPointerList()\n"));

	// Get the number of blocks this file occupies
	// I would just use Inode->i_blocks but it
	// doesn't seem to be the number of blocks
	// the file size corresponds to, but instead
	// it is much bigger.
	//BlockCount = Inode->i_blocks;
	FileSize = Ext2GetInodeFileSize(Inode);
	FileSize = ROUND_UP(FileSize, Ext2BlockSizeInBytes);
	BlockCount = (FileSize / Ext2BlockSizeInBytes);

	// Allocate the memory for the block list
	BlockList = MmAllocateMemory(BlockCount * sizeof(U32));
	if (BlockList == NULL)
	{
		return NULL;
	}

	RtlZeroMemory(BlockList, BlockCount * sizeof(U32));
	CurrentBlockInList = 0;

	// Copy the direct block pointers
	for (CurrentBlock=0; CurrentBlockInList<BlockCount && CurrentBlock<EXT3_NDIR_BLOCKS; CurrentBlock++)
	{
		BlockList[CurrentBlockInList] = Inode->i_block[CurrentBlock];
		CurrentBlockInList++;
	}

	// Copy the indirect block pointers
	if (CurrentBlockInList < BlockCount)
	{
		if (!Ext2CopyIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->i_block[EXT3_IND_BLOCK]))
		{
			MmFreeMemory(BlockList);
			return FALSE;
		}
	}

	// Copy the double indirect block pointers
	if (CurrentBlockInList < BlockCount)
	{
		if (!Ext2CopyDoubleIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->i_block[EXT3_DIND_BLOCK]))
		{
			MmFreeMemory(BlockList);
			return FALSE;
		}
	}

	// Copy the triple indirect block pointers
	if (CurrentBlockInList < BlockCount)
	{
		if (!Ext2CopyTripleIndirectBlockPointers(BlockList, &CurrentBlockInList, BlockCount, Inode->i_block[EXT3_TIND_BLOCK]))
		{
			MmFreeMemory(BlockList);
			return FALSE;
		}
	}

	return BlockList;
}

U64 Ext2GetInodeFileSize(PEXT2_INODE Inode)
{
	if ((Inode->i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR)
	{
		return (U64)(Inode->i_size);
	}
	else
	{
		return ((U64)(Inode->i_size) | ((U64)(Inode->i_dir_acl) << 32));
	}
}

BOOL Ext2CopyIndirectBlockPointers(U32* BlockList, U32* CurrentBlockInList, U32 BlockCount, U32 IndirectBlock)
{
	U32*	BlockBuffer = (U32*)FILESYSBUFFER;
	U32		CurrentBlock;
	U32		BlockPointersPerBlock;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2CopyIndirectBlockPointers() BlockCount = %d\n", BlockCount));

	BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(U32);

	if (!Ext2ReadBlock(IndirectBlock, BlockBuffer))
	{
		return FALSE;
	}

	for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
	{
		BlockList[(*CurrentBlockInList)] = BlockBuffer[CurrentBlock];
		(*CurrentBlockInList)++;
	}

	return TRUE;
}

BOOL Ext2CopyDoubleIndirectBlockPointers(U32* BlockList, U32* CurrentBlockInList, U32 BlockCount, U32 DoubleIndirectBlock)
{
	U32*	BlockBuffer;
	U32		CurrentBlock;
	U32		BlockPointersPerBlock;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2CopyDoubleIndirectBlockPointers() BlockCount = %d\n", BlockCount));

	BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(U32);

	BlockBuffer = (U32*)MmAllocateMemory(Ext2BlockSizeInBytes);
	if (BlockBuffer == NULL)
	{
		return FALSE;
	}

	if (!Ext2ReadBlock(DoubleIndirectBlock, BlockBuffer))
	{
		MmFreeMemory(BlockBuffer);
		return FALSE;
	}

	for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
	{
		if (!Ext2CopyIndirectBlockPointers(BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
		{
			MmFreeMemory(BlockBuffer);
			return FALSE;
		}
	}

	MmFreeMemory(BlockBuffer);
	return TRUE;
}

BOOL Ext2CopyTripleIndirectBlockPointers(U32* BlockList, U32* CurrentBlockInList, U32 BlockCount, U32 TripleIndirectBlock)
{
	U32*	BlockBuffer;
	U32		CurrentBlock;
	U32		BlockPointersPerBlock;

	DbgPrint((DPRINT_FILESYSTEM, "Ext2CopyTripleIndirectBlockPointers() BlockCount = %d\n", BlockCount));

	BlockPointersPerBlock = Ext2BlockSizeInBytes / sizeof(U32);

	BlockBuffer = (U32*)MmAllocateMemory(Ext2BlockSizeInBytes);
	if (BlockBuffer == NULL)
	{
		return FALSE;
	}

	if (!Ext2ReadBlock(TripleIndirectBlock, BlockBuffer))
	{
		MmFreeMemory(BlockBuffer);
		return FALSE;
	}

	for (CurrentBlock=0; (*CurrentBlockInList)<BlockCount && CurrentBlock<BlockPointersPerBlock; CurrentBlock++)
	{
		if (!Ext2CopyDoubleIndirectBlockPointers(BlockList, CurrentBlockInList, BlockCount, BlockBuffer[CurrentBlock]))
		{
			MmFreeMemory(BlockBuffer);
			return FALSE;
		}
	}

	MmFreeMemory(BlockBuffer);
	return TRUE;
}
