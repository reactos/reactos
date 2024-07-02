/*
	vfdio.h

	Virtual Floppy Drive for Windows
	Kernel mode driver / user mode program interface header

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef _VFDIO_H_
#define _VFDIO_H_

#ifndef __T
#ifdef _NTDDK_
#define __T(x)	L ## x
#else
#define __T(x)	x
#endif
#endif

#ifndef _T
#define _T(x)	__T(x)
#endif

//
//	Device/driver setting registry value names
//
#define VFD_REG_DEVICE_NUMBER		_T("NumberOfDisks")
#define VFD_REG_TRACE_FLAGS			_T("TraceFlags")
#define VFD_REG_DRIVE_LETTER		_T("DriveLetter")

//
//	Device object interface base name
//
#define VFD_DEVICE_BASENAME			_T("VirtualFD")

//
//	sector size constants and macros
//
#define VFD_BYTES_PER_SECTOR		512
#define VFD_SECTOR_ALIGN_MASK		(VFD_BYTES_PER_SECTOR - 1)
#define VFD_BYTE_SHIFT_COUNT		9

#define VFD_BYTE_TO_SECTOR(b)		((b) >> VFD_BYTE_SHIFT_COUNT)
#define VFD_SECTOR_TO_BYTE(s)		((s) << VFD_BYTE_SHIFT_COUNT)
#define VFD_SECTOR_ALIGNED(b)		(((b) & VFD_SECTOR_ALIGN_MASK) == 0)

//
//	Fill character for formatting media
//
#define VFD_FORMAT_FILL_DATA		(UCHAR)0xf6

//
//	Image information structure
//	Used for IOCTL_VFD_OPEN_IMAGE and IOCTL_VFD_QUERY_IMAGE
//
#pragma pack	(push,2)
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable: 4200)		//	Zero sized struct member warning
#endif

typedef struct _VFD_IMAGE_INFO {
	VFD_DISKTYPE	DiskType;		//	VFD_DISKTYPE_xxx value in vfdtypes.h
	VFD_MEDIA		MediaType;		//	VFD_MEDIA_xxx value in vfdtypes.h
	VFD_FLAGS		MediaFlags;		//	VFD_FLAG_xxx value in vfdtypes.h
	VFD_FILETYPE	FileType;		//	VFD_FILETYE_xxx value in vfdtypes.h
	ULONG			ImageSize;		//	actual image size in bytes
	USHORT			NameLength;		//	length in bytes of the file name
	CHAR			FileName[0];	//	variable length file name string
} VFD_IMAGE_INFO, *PVFD_IMAGE_INFO;

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning (pop)
#endif
#pragma pack	(pop)

//
//	Device IO control codes
//

/*
	IOCTL_VFD_OPEN_IMAGE

	Open an existing floppy image file or create an empty RAM disk

	Input:
		buffer containing a VFD_IMAGE_INFO structure followed by
		an image file name

	InputLength:
		sizeof(VFD_IMAGE_INFO) plus length of the image file name

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_INVALID_PARAMETER	input buffer size < sizeof(VFD_IMAGE_INFO)
									or any other parameter errors
		STATUS_DEVICE_BUSY			an image is already opened
		STATUS_ACCESS_DENIED		file access error. returned also when the
									file is compressed / encrypted
*/
#define IOCTL_VFD_OPEN_IMAGE		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x800,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_CLOSE_IMAGE

	Close the current virtual floppy image

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_NO_MEDIA_IN_DEVICE	image is not opened
*/
#define IOCTL_VFD_CLOSE_IMAGE		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x801,				\
										METHOD_NEITHER,		\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_QUERY_IMAGE

	Get the current image information

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		Buffer to receive a VFD_IMAGE_INFO data structure

	Output Length:
		must be long enough to hold a VFD_IMAGE_INFO with the image file name

	Return:
		STATUS_BUFFER_TOO_SMALL		buffer length < sizeof(VFD_IMAGE_INFO)
		STATUS_BUFFER_OVERFLOW		buffer cannot hold the image file name.
									NameLength member contains the file name
									length (number of bytes). See this value
									to decide necessary buffer length.
*/
#define IOCTL_VFD_QUERY_IMAGE		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x802,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS)

/*
	IOCTL_VFD_SET_LINK

	Create or delete a persistent drive letter
	On Windows NT, this command simply creates a symbolic link.
	On Windows 2000/XP, the driver calls the Mount Manager to manipulate
	a drive letter.

	Input:
		buffer containing a drive letter 'A' - 'Z' to create a drive letter,
		or 0 to delete the current drive letter.

	Input Length:
		sizeof(CHAR) or larger

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_INVALID_PARAMETER	input length == 0 or
									any other parameter errors
*/
#define IOCTL_VFD_SET_LINK			CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x803,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_QUERY_LINK

	Get the current persistent drive letter

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		buffer to receive the current drive letter.
		0 is returned if there is none.

	Output Length:
		sizeof(CHAR) or larger

	Return:
		STATUS_BUFFER_TOO_SMALL		buffer length < sizeof(CHAR)
*/
#define IOCTL_VFD_QUERY_LINK		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x804,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS)

/*
	IOCTL_VFD_SET_PROTECT

	Enable the virtual media write protection

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_NO_MEDIA_IN_DEVICE	image is not opened
*/
#define IOCTL_VFD_SET_PROTECT		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x805,				\
										METHOD_NEITHER,		\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_CLEAR_PROTECT

	Disable the virtual media write protection

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_NO_MEDIA_IN_DEVICE	image is not opened
*/
#define IOCTL_VFD_CLEAR_PROTECT		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x806,				\
										METHOD_NEITHER,		\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_RESET_MODIFY

	Reset the data modify flag

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		Not used with this operation; set to NULL.

	Output Length:
		Not used with this operation; set to zero.

	Return:
		STATUS_NO_MEDIA_IN_DEVICE	image is not opened
*/
#define IOCTL_VFD_RESET_MODIFY		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x807,				\
										METHOD_NEITHER,		\
										FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/*
	IOCTL_VFD_QUERY_NUMBER

	Get the current device's VFD device number (<n> in "\??\VirtualFD<n>")

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		buffer to receive the VFD device number

	Output Length:
		sizeof(ULONG) or larger

	Return:
		STATUS_BUFFER_TOO_SMALL		buffer length < sizeof(ULONG)
*/
#define IOCTL_VFD_QUERY_NUMBER		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x80d,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS)

/*
	IOCTL_VFD_QUERY_NAME

	Get the current device's name (\Device\Floppy<n>)
	The name is returned in a counted UNICODE string (not NULL terminated)

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		buffer to receive the length (USHORT value, number of bytes) followed
		by the UNICODE device name.

	Output Length:
		enough to receive the length and the name

	Return:
		STATUS_BUFFER_TOO_SMALL		buffer length < sizeof(USHORT)
		STATUS_BUFFER_OVERFLOW		buffer cannot hold the device name.
									The first sizeof(USHORT) bytes of the
									buffer contains the device name length.
									See this value to decide the necessary
									buffer length.
*/
#define IOCTL_VFD_QUERY_NAME		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x80e,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS)

/*
	IOCTL_VFD_QUERY_VERSION

	Get the running VFD driver version

	Input:
		Not used with this operation; set to NULL.

	Input Length:
		Not used with this operation; set to zero.

	Output:
		buffer to receive the VFD version (ULONG value)
		High word:	major version
		Low word:	minor version
		MSB:		debug version flag (1:debug 0:release)

	Output Length:
		sizeof(ULONG) or larger

	Return:
		STATUS_BUFFER_TOO_SMALL		buffer length < sizeof(ULONG)
*/
#define IOCTL_VFD_QUERY_VERSION		CTL_CODE(				\
										IOCTL_DISK_BASE,	\
										0x80f,				\
										METHOD_BUFFERED,	\
										FILE_READ_ACCESS)

#endif	//	_VFDIO_H_
