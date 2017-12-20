/*
	vfdtypes.h

	Virtual Floppy Drive for Windows
	kernel mode / user mode common data types / constants

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef _VFDTYPES_H_
#define _VFDTYPES_H_

//
//	Supported disk type enumeration
//
enum _VFD_DISKTYPE
{
	VFD_DISKTYPE_FILE = 0,			//	file disk (direct file access)
	VFD_DISKTYPE_RAM				//	ram disk (on memory image)
};

//
//	Supported media type enumeration
//
enum _VFD_MEDIA
{
	VFD_MEDIA_NONE = 0,				//	no media / unknown
	VFD_MEDIA_F5_160,				//	5.25"	160KB
	VFD_MEDIA_F5_180,				//	5.25"	180KB
	VFD_MEDIA_F5_320,				//	5.25"	320KB
	VFD_MEDIA_F5_360,				//	5.25"	360KB
	VFD_MEDIA_F3_640,				//	3.5"	640KB
	VFD_MEDIA_F5_640,				//	5.25"	640KB
	VFD_MEDIA_F3_720,				//	3.5"	720KB
	VFD_MEDIA_F5_720,				//	5.25"	720KB
	VFD_MEDIA_F3_820,				//	3.5"	820KB
	VFD_MEDIA_F3_1P2,				//	3.5"	1.2MB
	VFD_MEDIA_F5_1P2,				//	5.25"	1.2MB
	VFD_MEDIA_F3_1P4,				//	3.5"	1.44MB
	VFD_MEDIA_F3_1P6,				//	3.5"	1.68MB DMF
	VFD_MEDIA_F3_1P7,				//	3.5"	1.72MB DMF
	VFD_MEDIA_F3_2P8,				//	3.5"	2.88MB
	VFD_MEDIA_MAX					//	max value placeholder
};

//
//	Supported file type enumeration
//
enum _VFD_FILETYPE
{
	VFD_FILETYPE_NONE = 0,			//	no file
	VFD_FILETYPE_RAW,				//	RAW image file
	VFD_FILETYPE_ZIP,				//	ZIP compressed image
	VFD_FILETYPE_MAX				//	max value place holder
};

//
//	Type definition
//
typedef UCHAR						VFD_DISKTYPE,	*PVFD_DISKTYPE;
typedef UCHAR						VFD_MEDIA,		*PVFD_MEDIA;
typedef UCHAR						VFD_FILETYPE,	*PVFD_FILETYPE;
typedef UCHAR						VFD_FLAGS,		*PVFD_FLAGS;

//
//	Image flag values
//
#define VFD_FLAG_WRITE_PROTECTED	(VFD_FLAGS)0x01
#define VFD_FLAG_DATA_MODIFIED		(VFD_FLAGS)0x02

//
//	Default and max number of virtual floppy devices
//
#define VFD_DEFAULT_DEVICES			2
#define VFD_MAXIMUM_DEVICES			2

#endif	//	_VFDTYPES_H_
