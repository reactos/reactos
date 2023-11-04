/*
	vfdlib.h

	Virtual Floppy Drive for Windows
	Driver control library local header

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef _VFDLIB_H_
#define _VFDLIB_H_

#ifdef __REACTOS__
#define DWL_USER DWLP_USER
#endif

#define VFD_LIBRARY_FILENAME	"vfd.dll"

#ifdef VFD_EMBED_DRIVER
#define VFD_DRIVER_NAME_ID		VFD_DRIVER
#define VFD_DRIVER_TYPE_ID		BINARY
#endif

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

//
//	DLL instance handle
//
extern HINSTANCE	g_hDllModule;

//
//	Reference count for the DLL
//
extern UINT			g_cDllRefCnt;

//
//	VFD notification message value
//
extern UINT			g_nNotifyMsg;

//
//	VFD notification message register string
//
#define VFD_NOTIFY_MESSAGE	"VfdNotifyMessage"

//
//	Message box title string
//
#define VFD_MSGBOX_TITLE	"Virtual Floppy Drive"

//
//	shell extention string constants
//
#define VFDEXT_DESCRIPTION	"VFD shell extension"
#define VFDEXT_MENU_REGKEY	"Drive\\shellex\\ContextMenuHandlers\\VFD"
#define VFDEXT_DND_REGKEY	"Drive\\shellex\\DragDropHandlers\\VFD"
#define VFDEXT_PROP_REGKEY	"Drive\\shellex\\PropertySheetHandlers\\VFD"
#define VFDEXT_INFO_REGKEY	"Drive\\shellex\\{00021500-0000-0000-C000-000000000046}"

//=====================================
//	Image handling functions
//=====================================

//	Format a buffer with FAT12

DWORD FormatBufferFat(
	PUCHAR			pBuffer,
	ULONG			nSectors);

//	Extract image information from a zip compressed file

DWORD ExtractZipInfo(
	HANDLE			hFile,
	ULONG			*pSize);

//	Extract original image from a zip compressed file

DWORD ExtractZipImage(
	HANDLE			hFile,
	PUCHAR			*pBuffer,
	PULONG			pLength);

//=====================================
//	GUI utility functions
//=====================================

typedef struct _SAVE_PARAM {
	HANDLE			hDevice;
	VFD_DISKTYPE	DiskType;
	VFD_MEDIA		MediaType;
	VFD_FLAGS		MediaFlags;
	VFD_FILETYPE	FileType;
	ULONG			ImageSize;
	PSTR			ImageName;
} SAVE_PARAM, PSAVE_PARAM;

typedef const SAVE_PARAM CSAVE_PARAM, *PCSAVE_PARAM;

DWORD GuiSaveParam(
	HWND			hParent,
	PCSAVE_PARAM	pParam);

void ShowContextMenu(
	HWND			hDlg,
	HWND			hCtl,
	LPARAM			lParam);

void ShowHelpWindow(
	HWND			hDlg,
	UINT			nCtl);

//
//	Set a message to a control window
//
void SetControlText(
	HWND			hWnd,
	UINT			nCtrl,
	DWORD			nMsg);

//==============================
//	Message extract functions
//==============================

//	Return a system error message

PCSTR SystemMessage(
	DWORD			nError);

//	Return a message from this DLL module

PSTR ModuleMessage(
	DWORD			nFormat, ...);

//==============================
//	utility macros
//==============================

#define IS_WINDOWS_NT()	((GetVersion() & 0xff) < 5)

//==============================
//	Debug functions
//==============================

#ifdef _DEBUG
extern ULONG TraceFlags;
#ifndef __REACTOS__
extern PCHAR TraceFile;
#else
extern CHAR const * TraceFile;
#endif
extern ULONG TraceLine;

#define VFDTRACE(LEVEL,STRING)					\
	if ((TraceFlags & (LEVEL)) == (LEVEL)) {	\
		TraceFile = __FILE__;					\
		TraceLine = __LINE__;					\
		DebugTrace STRING;						\
	}

void DebugTrace(PCSTR sFormat, ...);

#else	// _DEBUG
#define VFDTRACE(LEVEL,STRING)
#endif	// _DEBUG

//
//	supplement old system headers
//
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif	// INVALID_FILE_ATTRIBUTES

#if defined(_INC_COMMDLG) && !defined(OPENFILENAME_SIZE_VERSION_400)
// Pre Win2K system header is used
// OPENFILENAME is defined without extra fields.
#define OPENFILENAME_SIZE_VERSION_400 sizeof(OPENFILENAME)
#endif	// __INC_COMMDLG && !OPENFILENAME_SIZE_VERSION_400

#ifdef __cplusplus
}
#endif	//	__cplusplus

#endif	//	RC_INVOKED

#endif	//	_VFDLIB_H_
