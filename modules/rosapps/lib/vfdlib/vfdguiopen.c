/*
	vfdguiopen.c

	Virtual Floppy Drive for Windows
	Driver control library
	Open image GUI utility function

	Copyright (c) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(push,3)
#endif
#include <commdlg.h>
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"
#ifndef __REACTOS__
#include "vfdmsg.h"
#else
#include "vfdmsg_lib.h"
#endif
#include "vfdguirc.h"

//
//	String constants
//

#define FALLBACK_IMAGE_FILTER	\
	"Common image files (bin,dat,fdd,flp,ima,img,vfd)\0" \
	"*.bin;*.dat;*.fdd;*.flp;*.ima;*.img;*.vfd\0" \
	"Zip compressed image (imz,zip)\0*.imz;*.zip\0" \
	"All files (*.*)\0*.*\0"

#define FALLBACK_IMAGE_TITLE	"Open Virtual Floppy Image"

//
//	local functions
//
#ifndef __REACTOS__
static INT CALLBACK OpenDialogProc(
#else
static INT_PTR CALLBACK OpenDialogProc(
#endif
	HWND			hDlg,
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam);

static void OnInit(HWND hDlg, ULONG nDevice);
static void OnImage(HWND hDlg, HWND hEdit);
static void OnBrowse(HWND hDlg);
static void OnDiskType(HWND hDlg, HWND hRadio);
static void OnMediaType(HWND hDlg, HWND hCombo);
static void OnProtect(HWND hDlg, HWND hCheck);
static DWORD OnOK(HWND hDlg);

//
//	Show Open Image dialog box
//
DWORD WINAPI VfdGuiOpen(
	HWND			hParent,
	ULONG			nDevice)
{
	switch (DialogBoxParam(
		g_hDllModule,
		MAKEINTRESOURCE(IDD_OPENDIALOG),
		hParent,
		OpenDialogProc,
		nDevice))
	{
	case IDOK:
		return ERROR_SUCCESS;

	case IDCANCEL:
		return ERROR_CANCELLED;

	default:
		return GetLastError();
	}
}

//
// Open image dialog procedure
//
#ifndef __REACTOS__
INT CALLBACK OpenDialogProc(
#else
INT_PTR CALLBACK OpenDialogProc(
#endif
	HWND			hDlg,
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		OnInit(hDlg, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (wParam) {
		case MAKELONG(IDC_IMAGEFILE, EN_CHANGE):
			OnImage(hDlg, (HWND)lParam);
			return TRUE;

		case IDC_BROWSE:
			OnBrowse(hDlg);
			return TRUE;

		case IDC_DISKTYPE_FILE:
		case IDC_DISKTYPE_RAM:
			OnDiskType(hDlg, (HWND)lParam);
			return TRUE;

		case MAKELONG(IDC_MEDIATYPE, CBN_SELCHANGE):
			OnMediaType(hDlg, (HWND)lParam);
			return TRUE;

		case IDC_OPEN_PROTECTED:
			OnProtect(hDlg, (HWND)lParam);
			return TRUE;

		case IDOK:
			if (OnOK(hDlg) == ERROR_SUCCESS) {
				EndDialog(hDlg, IDOK);
			}
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;

	case WM_CONTEXTMENU:
		ShowContextMenu(hDlg, (HWND)wParam, lParam);
		break;

	case WM_HELP:
		{
			LPHELPINFO info = (LPHELPINFO)lParam;

			if (info->iContextType == HELPINFO_WINDOW) {
				ShowHelpWindow(hDlg, info->iCtrlId);
			}
		}
		return TRUE;
	}

	return FALSE;
}

//
//	Initialize the Open Image dialog
//
void OnInit(
	HWND			hDlg,
	ULONG			nDevice)
{
	VFD_MEDIA i;

	//	Store the device number

	SetWindowLong(hDlg, GWL_USERDATA, nDevice);

	//	Store default file size

	SetWindowLong(hDlg, DWL_USER, INVALID_FILE_SIZE);

	// Set dialog window title

	SetControlText(hDlg, 0, MSG_OPEN_TITLE);

	// Set control captions

	SetControlText(hDlg, IDC_IMAGEFILE_LABEL,	MSG_IMAGEFILE_ACCEL);
	SetControlText(hDlg, IDC_IMAGEDESC_LABEL,	MSG_DESCRIPTION_LABEL);
	SetControlText(hDlg, IDC_BROWSE,			MSG_BROWSE_BUTTON);
	SetControlText(hDlg, IDC_DISKTYPE_LABEL,	MSG_DISKTYPE_LABEL);
	SetControlText(hDlg, IDC_MEDIATYPE_LABEL,	MSG_MEDIATYPE_ACCEL);
	SetControlText(hDlg, IDC_OPEN_PROTECTED,	MSG_MENU_PROTECT);
	SetControlText(hDlg, IDOK,					MSG_CREATE_BUTTON);
	SetControlText(hDlg, IDCANCEL,				MSG_CANCEL_BUTTON);

	//	select RAM disk as default

	CheckRadioButton(hDlg, IDC_DISKTYPE_FILE,
		IDC_DISKTYPE_RAM, IDC_DISKTYPE_RAM);

	//	setup media type combo list

	for (i = 1; i < VFD_MEDIA_MAX; i++) {
		SendDlgItemMessage(hDlg, IDC_MEDIATYPE,
			CB_ADDSTRING, 0, (LPARAM)VfdMediaTypeName(i));
	}

	//	select 1.44MB as the default

	SendDlgItemMessage(hDlg, IDC_MEDIATYPE, CB_SELECTSTRING,
		(WPARAM)-1, (LPARAM)VfdMediaTypeName(VFD_MEDIA_F3_1P4));

	//	set up other controls

	OnImage(hDlg, GetDlgItem(hDlg, IDC_IMAGEFILE));
}

//
//	Path is changed -- check if the file exists
//
void OnImage(
	HWND			hDlg,
	HWND			hEdit)
{
	CHAR			buf[MAX_PATH];
	DWORD			file_attr;
	ULONG			image_size;
	VFD_FILETYPE	file_type;
	VFD_MEDIA		media_type;

	DWORD			ret = ERROR_SUCCESS;

	//	Store default file size

	SetWindowLong(hDlg, DWL_USER, INVALID_FILE_SIZE);

	//	get currently selected media type

	media_type = (VFD_MEDIA)(SendDlgItemMessage(
		hDlg, IDC_MEDIATYPE, CB_GETCURSEL, 0, 0) + 1);

	//	clear hint and description text

	SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, NULL);
	SetDlgItemText(hDlg, IDC_IMAGEFILE_HINT, NULL);

	//	get file name and file information

	if (GetWindowText(hEdit, buf, sizeof(buf))) {

		ret = VfdCheckImageFile(
			buf, &file_attr, &file_type, &image_size);

		if (ret == ERROR_SUCCESS) {

			//	use media type from image size

			media_type = VfdLookupMedia(image_size);
		}
		else if (ret == ERROR_FILE_NOT_FOUND) {

			//	new file
			//	use the parent directory attributes

			PSTR p;

			if ((p = strrchr(buf, '\\')) != NULL) {
				*p = '\0';
			}

			file_attr	= GetFileAttributes(buf);

			if (file_attr == INVALID_FILE_ATTRIBUTES) {
				//	directory access error
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
				SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, SystemMessage(ret));
				return;
			}

			file_attr	&= ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);
			image_size	= INVALID_FILE_SIZE;
			file_type	= VFD_FILETYPE_RAW;
		}
		else {
			//	file access error
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, SystemMessage(ret));
			return;
		}

		//	make file description text

		VfdMakeFileDesc(buf, sizeof(buf),
			file_type, image_size, file_attr);

		//	set file description

		SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, buf);
	}
	else {

		//	filename is empty - RAM disk

		file_attr	= 0;
		image_size	= INVALID_FILE_SIZE;
		file_type	= VFD_FILETYPE_NONE;

		SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, "RAM");
	}

	//	store the image size

	SetWindowLong(hDlg, DWL_USER, image_size);

	//	setup disktype controls

	if (file_type != VFD_FILETYPE_RAW ||
		(file_attr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED))) {

		//	file cannot be opened directly -- RAM mode is forced

		CheckRadioButton(hDlg, IDC_DISKTYPE_FILE,
			IDC_DISKTYPE_RAM, IDC_DISKTYPE_RAM);

		EnableWindow(GetDlgItem(hDlg, IDC_DISKTYPE_FILE),	FALSE);
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_DISKTYPE_FILE),	TRUE);
	}

	//	set OK button text

	if (image_size == INVALID_FILE_SIZE) {
		//	file does not exist - OK button is "Create"

		SetControlText(hDlg, IDOK, MSG_CREATE_BUTTON);
	}
	else {
		//	file exists - OK button is "Open"

		SetControlText(hDlg, IDOK, MSG_OPEN_BUTTON);
	}

	//	select media type

	SendDlgItemMessage(hDlg, IDC_MEDIATYPE,
		CB_SETCURSEL, media_type - 1, 0);

	OnMediaType(hDlg, GetDlgItem(hDlg, IDC_MEDIATYPE));
}

//
//	Show open file dialog box
//
void OnBrowse(
	HWND			hDlg)
{
	OPENFILENAME	ofn;
	PSTR			title;
	PSTR			filter;
	CHAR			file[MAX_PATH];
	CHAR			dir[MAX_PATH];
	DWORD			len;

	//	prepare title and filter text

	title = ModuleMessage(MSG_OPEN_TITLE);

	filter = ModuleMessage(MSG_OPEN_FILTER);

	if (filter) {
		PSTR p = filter;

		do {
			if (*p == '|') {
				*p = '\0';
			}
		}
		while (*(++p));
	}

	//	get current file name from the control

	ZeroMemory(file, sizeof(file));
	ZeroMemory(dir, sizeof(dir));

	len = GetDlgItemText(hDlg, IDC_IMAGEFILE, file, sizeof(file));

	if (len && file[len - 1] == '\\') {
		strcpy(dir, file);
		ZeroMemory(file, sizeof(file));
	}

	// prepare OPENFILENAME structure

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = IS_WINDOWS_NT() ?
		OPENFILENAME_SIZE_VERSION_400 : sizeof(ofn);

	ofn.hwndOwner	= hDlg;
	ofn.lpstrFilter = filter ? filter : FALLBACK_IMAGE_FILTER;
	ofn.lpstrFile	= file;
	ofn.nMaxFile	= sizeof(file);
	ofn.lpstrInitialDir = dir;
	ofn.lpstrTitle	= title ? title : FALLBACK_IMAGE_TITLE;
#ifndef __REACTOS__
	ofn.Flags		= OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
#else
	ofn.Flags		= OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
#endif

	//	show the open file dialog box

	if (GetOpenFileName(&ofn)) {
		SetDlgItemText(hDlg, IDC_IMAGEFILE, file);
		SetFocus(GetDlgItem(hDlg, IDC_IMAGEFILE));
	}

	//	release text buffers

	if (filter) {
		LocalFree(filter);
	}

	if (title) {
		LocalFree(title);
	}
}

//
//	Disk type is changed
//
void OnDiskType(
	HWND			hDlg,
	HWND			hRadio)
{
	UNREFERENCED_PARAMETER(hDlg);
	UNREFERENCED_PARAMETER(hRadio);
}

//
//	Media type is changed
//
void OnMediaType(
	HWND			hDlg,
	HWND			hCombo)
{
	VFD_MEDIA		media_type;
	ULONG			media_size;
	ULONG			image_size;

	image_size = GetWindowLong(hDlg, DWL_USER);

	if (image_size == INVALID_FILE_SIZE) {
		return;
	}

	media_type = (VFD_MEDIA)(SendMessage(
		hCombo, CB_GETCURSEL, 0, 0) + 1);

	if (media_type == 0) {
		return;
	}

	media_size = VfdGetMediaSize(media_type);

	if (media_size > image_size) {
		//	selected media is too large
		SetControlText(hDlg, IDC_IMAGEFILE_HINT, MSG_FILE_TOO_SMALL);
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		return;
	}
	else if (media_size < image_size) {
		SetControlText(hDlg, IDC_IMAGEFILE_HINT, MSG_SIZE_MISMATCH);
	}
	else {
		SetDlgItemText(hDlg, IDC_IMAGEFILE_HINT, NULL);
	}

	EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
}

//
//	Write Protect check box is clicked
//
void OnProtect(
	HWND			hDlg,
	HWND			hCheck)
{
	UNREFERENCED_PARAMETER(hDlg);
	UNREFERENCED_PARAMETER(hCheck);
}

//
//	Create / open an image
//
DWORD OnOK(
	HWND			hDlg)
{
	CHAR			file_name[MAX_PATH];
	VFD_DISKTYPE	disk_type;
	VFD_MEDIA		media_type;
	VFD_FLAGS		image_flags;
	HANDLE			hDevice;
	DWORD			ret;

	//	get the disk type

	if (IsDlgButtonChecked(hDlg, IDC_DISKTYPE_FILE) == BST_CHECKED) {
		disk_type = VFD_DISKTYPE_FILE;
	}
	else {
		disk_type = VFD_DISKTYPE_RAM;
	}

	//	get the media type

	media_type = (VFD_MEDIA)(SendDlgItemMessage(
		hDlg, IDC_MEDIATYPE, CB_GETCURSEL, 0, 0) + 1);

	//	get the protect flag

	if (IsDlgButtonChecked(hDlg, IDC_OPEN_PROTECTED) == BST_CHECKED) {
		image_flags = VFD_FLAG_WRITE_PROTECTED;
	}
	else {
		image_flags = 0;
	}

	//	get the image name to create

	if (GetDlgItemText(hDlg, IDC_IMAGEFILE, file_name, sizeof(file_name))) {

		//	file is specified

		if (GetWindowLong(hDlg, DWL_USER) == INVALID_FILE_SIZE) {

			//	create a new image

			ret = VfdCreateImageFile(
				file_name, media_type, VFD_FILETYPE_RAW, FALSE);

			if (ret != ERROR_SUCCESS) {
				goto exit_func;
			}
		}
	}

	//	open the image

	hDevice = VfdOpenDevice(GetWindowLong(hDlg, GWL_USERDATA));

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		goto exit_func;
	}

	ret = VfdOpenImage(
		hDevice, file_name, disk_type, media_type, image_flags);

	CloseHandle(hDevice);

exit_func:
	if (ret != ERROR_SUCCESS) {

		//	show error message

		MessageBox(hDlg, SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
	}

	return ret;
}
