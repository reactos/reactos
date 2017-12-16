/*
	vfdapi.h

	Virtual Floppy Drive for Windows
	Driver control library API header

	Copyright (C) 2003-2008 Ken Kato
*/

#ifndef _VFDAPI_H_
#define _VFDAPI_H_

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

//
//	custom SERVICE STATE value returned by VfdGetDriverState
//
#define VFD_NOT_INSTALLED		0xffffffff

//
//	VFD operation code for VFD notification message
//
typedef enum _VFD_OPERATION {
	VFD_OPERATION_NONE,			//	No operation
	VFD_OPERATION_INSTALL,		//	The driver was installed
	VFD_OPERATION_CONFIG,		//	The driver config was changed
	VFD_OPERATION_REMOVE,		//	The driver was removed
	VFD_OPERATION_START,		//	The driver was started
	VFD_OPERATION_STOP,			//	The driver was stopped
	VFD_OPERATION_OPEN,			//	An image was opened
	VFD_OPERATION_SAVE,			//	An image was saved
	VFD_OPERATION_CLOSE,		//	An image was closed
	VFD_OPERATION_SETLINK,		//	A drive letter was created
	VFD_OPERATION_DELLINK,		//	A drive letter was removed
	VFD_OPERATION_PROTECT,		//	Write protect state was changed
	VFD_OPERATION_SHELL,		//	Shell extension was installed/removed
	VFD_OPERATION_MAX			//	Maximum value place holder
} VFD_OPERATION, *PVFD_OPERATION;

//==============================
//	Driver management functions
//==============================

//	Install the driver

DWORD WINAPI VfdInstallDriver(
	PCSTR			sFileName,
	DWORD			nStart);

//	Uninstall the driver

DWORD WINAPI VfdRemoveDriver();

//	Configure the driver

DWORD WINAPI VfdConfigDriver(
	DWORD			nStart);

//	Start the driver

DWORD WINAPI VfdStartDriver(
	PDWORD			pState);

//	Stop the driver

DWORD WINAPI VfdStopDriver(
	PDWORD			pState);

//	Get current driver config information

DWORD WINAPI VfdGetDriverConfig(
	PSTR			sFileName,
	PDWORD			pStart);

//	Get current driver state

DWORD WINAPI VfdGetDriverState(
	PDWORD			pState);

//==============================
//	Device control functions
//==============================

//	Open a VFD device

HANDLE WINAPI VfdOpenDevice(
	ULONG			nTarget);

//	Get the device number

DWORD WINAPI VfdGetDeviceNumber(
	HANDLE			hDevice,
	PULONG			pNumber);

//	Get the device name

DWORD WINAPI VfdGetDeviceName(
	HANDLE			hDevice,
	PCHAR			pName,
	ULONG			nLength);

//	Get the driver version

DWORD WINAPI VfdGetDriverVersion(
	HANDLE			hDevice,
	PULONG			pVersion);

//==============================
//	image functions
//==============================

//	Open a virtual floppy image

DWORD WINAPI VfdOpenImage(
	HANDLE			hDevice,
	PCSTR			sFileName,
	VFD_DISKTYPE	nDiskType,
	VFD_MEDIA		nMediaType,
	VFD_FLAGS		nMediaFlags);

//	Close the current virtual floppy image

DWORD WINAPI VfdCloseImage(
	HANDLE			hDevice,
	BOOL			bForce);

//	Get the current image information

DWORD WINAPI VfdGetImageInfo(
	HANDLE			hDevice,
	PSTR			sFileName,
	PVFD_DISKTYPE	pDiskType,
	PVFD_MEDIA		pMediaType,
	PVFD_FLAGS		pMediaFlags,
	PVFD_FILETYPE	pFileType,
	PULONG			pImageSize);

//	Save the current image into a file

DWORD WINAPI VfdSaveImage(
	HANDLE			hDevice,
	PCSTR			sFileName,
	BOOL			bOverWrite,
	BOOL			bTruncate);

//	Format the current virtual media

DWORD WINAPI VfdFormatMedia(
	HANDLE			hDevice);

//	Get the current media state (opened / write protected)

DWORD WINAPI VfdGetMediaState(
	HANDLE			hDevice);

//	Set write protect state

DWORD WINAPI VfdWriteProtect(
	HANDLE			hDevice,
	BOOL			bProtect);

//	Dismount the volume (should be called before Save, Format)

DWORD WINAPI VfdDismountVolume(
	HANDLE			hDevice,
	BOOL			bForce);

//==============================
//	Drive letter functions
//==============================

//	Assign or remove a persistent drive letter

DWORD WINAPI VfdSetGlobalLink(
	HANDLE			hDevice,
	CHAR			cLetter);

//	Get the current persistent drive letter

DWORD WINAPI VfdGetGlobalLink(
	HANDLE			hDevice,
	PCHAR			pLetter);

//	Assign or remove an ephemeral drive letter

DWORD WINAPI VfdSetLocalLink(
	HANDLE			hDevice,
	CHAR			cLetter);

//	Get the first ephemeral drive letter

DWORD WINAPI VfdGetLocalLink(
	HANDLE			hDevice,
	PCHAR			pLetter);

//	Choose the first available drive letter

CHAR WINAPI VfdChooseLetter();

//==============================
//	utility functions
//==============================

//	Check running platform

BOOL WINAPI VfdIsValidPlatform();

//	Get VFD notification message value

UINT WINAPI VfdGetNotifyMessage();

//	Check if specified file is a valid VFD driver

DWORD WINAPI VfdCheckDriverFile(
	PCSTR			sFileName,
	PULONG			pFileVersion);

//	Check if specified path is a valid image file

DWORD WINAPI VfdCheckImageFile(
	PCSTR			sFileName,
	PDWORD			pAttributes,
	PVFD_FILETYPE	pFileType,
	PULONG			pImageSize);

//	Create a formatted new image file

DWORD WINAPI VfdCreateImageFile(
	PCSTR			sFileName,
	VFD_MEDIA		nMediaType,
	VFD_FILETYPE	nFileType,
	BOOL			bOverWrite);

//	Lookup the largest media to fit in a size

VFD_MEDIA WINAPI VfdLookupMedia(
	ULONG			nSize);

//	Get media size (in bytes) of a media type

ULONG WINAPI VfdGetMediaSize(
	VFD_MEDIA		nMediaType);

//	Get media type name

PCSTR WINAPI VfdMediaTypeName(
	VFD_MEDIA		nMediaType);

//	Make a file description text

void WINAPI VfdMakeFileDesc(
	PSTR			pBuffer,
	ULONG			nBufSize,
	VFD_FILETYPE	nFileType,
	ULONG			nFileSize,
	DWORD			nFileAttr);

//==============================
//	Shell Extension functions
//==============================

//	install the shell extension

DWORD WINAPI VfdRegisterHandlers();

//	uninstall the shell extension

DWORD WINAPI VfdUnregisterHandlers();

//	check if the shell extension is installed

DWORD WINAPI VfdCheckHandlers();

//==============================
//	GUI utility functions
//==============================

//	open an existing image file

DWORD WINAPI VfdGuiOpen(
	HWND			hParent,	//	parent window
	ULONG			nDevice);	//	device number

//	Save the current image

DWORD WINAPI VfdGuiSave(
	HWND			hParent,	//	parent window
	ULONG			nDevice);	//	device number

//	close the current image

DWORD WINAPI VfdGuiClose(
	HWND			hParent,	//	parent window
	ULONG			nDevice);	//	device number

//	format the current media

DWORD WINAPI VfdGuiFormat(
	HWND			hParent,	//	parent window
	ULONG			nDevice);	//	device number

//	display a tooltip window

void WINAPI VfdToolTip(
	HWND			hParent,	//	parent window
	PCSTR			sText,		//	tooltip text
	int				pos_x,		//	position x
	int				pos_y,		//	position y
	BOOL			stick);		//	stick (remain until losing the focus) or
							//	non-stick (remain until the mouse leaves)

//	Show image information tooltip

void WINAPI VfdImageTip(
	HWND			hParent,
	ULONG			nDevice);

#ifdef __cplusplus
}
#endif	//	__cplusplus

#endif	//	_VFDAPI_H_
