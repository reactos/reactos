/*
   rdesktop: A Remote Desktop Protocol client.
   Disk Redirection definitions
   Copyright (C) Jeroen Meijer 2003
   Copyright (C) Peter Astrand 2004
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define FILE_ATTRIBUTE_READONLY			0x00000001
#define FILE_ATTRIBUTE_HIDDEN			0x00000002
#define FILE_ATTRIBUTE_SYSTEM			0x00000004
#define FILE_ATTRIBUTE_DIRECTORY		0x00000010
#define FILE_ATTRIBUTE_ARCHIVE			0x00000020
#define FILE_ATTRIBUTE_DEVICE			0x00000040
#define FILE_ATTRIBUTE_UNKNOWNXXX0		0x00000060	/* ??? ACTION i.e. 0x860 == compress this file ? */
#define FILE_ATTRIBUTE_NORMAL			0x00000080
#define FILE_ATTRIBUTE_TEMPORARY		0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE		0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT		0x00000400
#define FILE_ATTRIBUTE_COMPRESSED		0x00000800
#define FILE_ATTRIBUTE_OFFLINE			0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED	0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED		0x00004000

#define FILE_FLAG_OPEN_NO_RECALL		0x00100000
#define FILE_FLAG_OPEN_REPARSE_POINT		0x00200000
#define FILE_FLAG_POSIX_SEMANTICS		0x01000000
#define FILE_FLAG_BACKUP_SEMANTICS		0x02000000	/* sometimes used to create a directory */
#define FILE_FLAG_DELETE_ON_CLOSE		0x04000000
#define FILE_FLAG_SEQUENTIAL_SCAN		0x08000000
#define FILE_FLAG_RANDOM_ACCESS			0x10000000
#define FILE_FLAG_NO_BUFFERING			0x20000000
#define FILE_FLAG_OVERLAPPED			0x40000000
#define FILE_FLAG_WRITE_THROUGH			0x80000000

#define FILE_SHARE_READ				0x01
#define FILE_SHARE_WRITE			0x02
#define FILE_SHARE_DELETE			0x04

#define FILE_BASIC_INFORMATION			0x04
#define FILE_STANDARD_INFORMATION		0x05

#define FS_CASE_SENSITIVE			0x00000001
#define FS_CASE_IS_PRESERVED			0x00000002
#define FS_UNICODE_STORED_ON_DISK		0x00000004
#define FS_PERSISTENT_ACLS			0x00000008
#define FS_FILE_COMPRESSION			0x00000010
#define FS_VOLUME_QUOTAS			0x00000020
#define FS_SUPPORTS_SPARSE_FILES		0x00000040
#define FS_SUPPORTS_REPARSE_POINTS		0x00000080
#define FS_SUPPORTS_REMOTE_STORAGE		0X00000100
#define FS_VOL_IS_COMPRESSED			0x00008000
#define FILE_READ_ONLY_VOLUME			0x00080000

#define OPEN_EXISTING				1
#define CREATE_NEW				2
#define OPEN_ALWAYS				3
#define TRUNCATE_EXISTING			4
#define CREATE_ALWAYS				5

#define GENERIC_READ				0x80000000
#define GENERIC_WRITE				0x40000000
#define GENERIC_EXECUTE				0x20000000
#define GENERIC_ALL				0x10000000

#define ERROR_FILE_NOT_FOUND			2L
#define ERROR_ALREADY_EXISTS			183L

typedef enum _FILE_INFORMATION_CLASS
{
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileCopyOnWriteInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileOleClassIdInformation,
	FileOleStateBitsInformation,
	FileNetworkOpenInformation,
	FileObjectIdInformation,
	FileOleAllInformation,
	FileOleDirectoryInformation,
	FileContentIndexInformation,
	FileInheritContentIndexInformation,
	FileOleInformation,
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef enum _FSINFOCLASS
{
	FileFsVolumeInformation = 1,
	FileFsLabelInformation,
	FileFsSizeInformation,
	FileFsDeviceInformation,
	FileFsAttributeInformation,
	FileFsControlInformation,
	FileFsFullSizeInformation,
	FileFsObjectIdInformation,
	FileFsDriverPathInformation,
	FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;
