/*
	vfdlib.rc

	Virtual Floppy Drive for Windows
	Driver control library
	Resource Script

	The non-standard extension ".rs" is intentional, so that
	Microsoft Visual Studio won't try to open this file with
	the resource editor

	Copyright (c) 2003-2005 Ken Kato
*/

#ifndef APSTUDIO_INVOKED

//
//	version resource constants
//
#include <winver.h>

//
//	VFD common version constants
//
#include "vfdver.h"

//
//	Library specific version constants
//
#include "vfdlib.h"

#define VFD_FILEOS				VOS_NT_WINDOWS32
#define VFD_FILETYPE			VFT_DLL
#define VFD_FILESUBTYPE			VFT2_UNKNOWN

#define VFD_DESCRIPTION			"Virtual Floppy Drive Library"
#define VFD_INTERNALNAME		VFD_LIBRARY_FILENAME
#define VFD_FILE_MAJOR			2
#define VFD_FILE_MINOR			1

//
//	embedded VFD driver binary
//
#ifdef VFD_EMBED_DRIVER

#define VFD_SPECIAL_FLAG		VS_FF_SPECIALBUILD
#define VFD_SPECIAL_DESC		"Driver binary embedded version"
#define VFD_SPECIAL_DESC_ALT	"ドライババイナリ埋め込み版"

#ifdef _DEBUG
VFD_DRIVER_NAME_ID VFD_DRIVER_TYPE_ID "..\..\sys\objchk\i386\vfd.sys"
#else	// _DEBUG
VFD_DRIVER_NAME_ID VFD_DRIVER_TYPE_ID "..\..\sys\objfre\i386\vfd.sys"
#endif	// _DEBUG

#endif	// VFD_EMBED_DRIVER

//
//	Japanese version resource constants
//
#define VFD_VERSIONINFO_ALT		"041104B0"
#undef VFD_VERSIONINFO_TRANS
#define VFD_VERSIONINFO_TRANS	0x0409, 0x04B0, 0x0411, 0x04B0

#define VFD_DESCRIPTION_ALT		"Virtual Floppy Drive ライブラリ"
#define VFD_PRODUCT_NAME_ALT	VFD_PRODUCT_NAME

//
//	VFD common version resource
//
#include "vfdver.rc"

//
//	GUI resource
//
#include "vfdlib.rc"

//
//	Module message resource
//
#include "vfdmsg.rc"

#endif	//	!APSTUDIO_INVOKED
