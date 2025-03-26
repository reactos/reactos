/*
	vfdguisave.c

	Virtual Floppy Drive for Windows
	Driver control library
	Save image GUI utility function

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
//	local functions
//
#ifndef __REACTOS__
static INT CALLBACK SaveDialogProc(
#else
static INT_PTR CALLBACK SaveDialogProc(
#endif
	HWND			hDlg,
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam);

static void OnInit(HWND hDlg, PCSAVE_PARAM pParam);
static void OnTarget(HWND hDlg, HWND hEdit);
static void OnBrowse(HWND hDlg);
static void OnOverwrite(HWND hDlg, HWND hCheck);
static void OnTruncate(HWND hDlg, HWND hCheck);
static DWORD OnOK(HWND hDlg);

//
//	Show Save Image dialog box
//
DWORD WINAPI VfdGuiSave(
	HWND			hParent,
	ULONG			nDevice)
{
	SAVE_PARAM		param;
	CHAR			path[MAX_PATH];
	DWORD			ret;

	//	open the source device

	param.hDevice = VfdOpenDevice(nDevice);

	if (param.hDevice == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	//	get current image information

	param.ImageName = path;

	ret = VfdGetImageInfo(
		param.hDevice,
		param.ImageName,
		&param.DiskType,
		&param.MediaType,
		&param.MediaFlags,
		&param.FileType,
		&param.ImageSize);

	if (ret == ERROR_SUCCESS) {

		//	show dialog box

		ret = GuiSaveParam(hParent, &param);
	}

	//	close the source device

	CloseHandle(param.hDevice);

	return ret;
}

DWORD GuiSaveParam(
	HWND			hParent,
	PCSAVE_PARAM	pParam)
{
	switch (DialogBoxParam(
		g_hDllModule,
		MAKEINTRESOURCE(IDD_SAVEDIALOG),
		hParent,
		SaveDialogProc,
		(LPARAM)pParam))
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
// The dialog procedure
//
#ifndef __REACTOS__
INT CALLBACK SaveDialogProc(
#else
INT_PTR CALLBACK SaveDialogProc(
#endif
	HWND			hDlg,
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		OnInit(hDlg, (PCSAVE_PARAM)lParam);
		return TRUE;

	case WM_COMMAND:
		switch (wParam) {
		case MAKELONG(IDC_TARGETFILE, EN_CHANGE):
			OnTarget(hDlg, (HWND)lParam);
			return TRUE;

		case IDC_BROWSE:
			OnBrowse(hDlg);
			return TRUE;

		case IDC_OVERWRITE:
			OnOverwrite(hDlg, (HWND)lParam);
			return TRUE;

		case IDC_TRUNCATE:
			OnTruncate(hDlg, (HWND)lParam);
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
//	Initialize the dialog
//
void OnInit(
	HWND			hDlg,
	PCSAVE_PARAM	pParam)
{
	//	Store parameters

#ifndef __REACTOS__
	SetWindowLong(hDlg, GWL_USERDATA, (ULONG)pParam);
#else
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (ULONG_PTR)pParam);
#endif

	//	clear the target existence flag

	SetWindowLong(hDlg, DWL_USER, 0);

	// Set dialog window title

	SetControlText(hDlg, 0, MSG_SAVE_TITLE);

	// Set control captions

	SetControlText(hDlg, IDC_IMAGEFILE_LABEL,	MSG_IMAGEFILE_LABEL);
	SetControlText(hDlg, IDC_DISKTYPE_LABEL,	MSG_DISKTYPE_LABEL);
	SetControlText(hDlg, IDC_MEDIATYPE_LABEL,	MSG_MEDIATYPE_LABEL);
	SetControlText(hDlg, IDC_TARGETFILE_LABEL,	MSG_TARGETFILE_LABEL);
	SetControlText(hDlg, IDC_IMAGEDESC_LABEL,	MSG_DESCRIPTION_LABEL);
	SetControlText(hDlg, IDC_BROWSE,			MSG_BROWSE_BUTTON);
	SetControlText(hDlg, IDC_OVERWRITE,			MSG_OVERWRITE_CHECK);
	SetControlText(hDlg, IDC_TRUNCATE,			MSG_TRUNCATE_CHECK);
	SetControlText(hDlg, IDOK,					MSG_SAVE_BUTTON);
	SetControlText(hDlg, IDCANCEL,				MSG_CANCEL_BUTTON);

	//	set disk type

	if (pParam->DiskType == VFD_DISKTYPE_FILE) {
		SetDlgItemText(hDlg, IDC_DISKTYPE, "FILE");
	}
	else {
		SetDlgItemText(hDlg, IDC_DISKTYPE, "RAM");
	}

	//	display media type

	SetDlgItemText(hDlg, IDC_MEDIATYPE,
		VfdMediaTypeName(pParam->MediaType));

	//	set current image and initial target

	if (pParam->ImageName[0]) {
		SetDlgItemText(hDlg, IDC_IMAGEFILE, pParam->ImageName);
		SetDlgItemText(hDlg, IDC_TARGETFILE, pParam->ImageName);
	}
	else if (pParam->DiskType != VFD_DISKTYPE_FILE) {
		SetDlgItemText(hDlg, IDC_IMAGEFILE, "<RAM>");
		OnTarget(hDlg, GetDlgItem(hDlg, IDC_TARGETFILE));
	}
}

//
//	Path is changed -- check specified target file
//
void OnTarget(
	HWND			hDlg,
	HWND			hEdit)
{
	PCSAVE_PARAM	param;
	CHAR			buf[MAX_PATH];
	ULONG			file_size;
	VFD_FILETYPE	file_type;
	DWORD			file_attr;
	DWORD			ret;

	//	clear the target existence flag

	SetWindowLong(hDlg, DWL_USER, 0);

	//	clear the description and hint text

	SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, NULL);
	SetDlgItemText(hDlg, IDC_IMAGEFILE_HINT, NULL);

	//	get the target file name

	if (GetWindowText(hEdit, buf, sizeof(buf)) == 0) {

		//	target file is blank

		EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

		return;
	}
	else {
		CHAR full[MAX_PATH];
		PSTR file;

		//	convert into a full path

		if (GetFullPathName(buf, sizeof(full), full, &file)) {
			strcpy(buf, full);
		}
	}

	//
	//	get the current image info
	//
#ifndef __REACTOS__
	param = (PCSAVE_PARAM)GetWindowLong(hDlg, GWL_USERDATA);
#else
	param = (PCSAVE_PARAM)GetWindowLongPtr(hDlg, GWLP_USERDATA);
#endif

	if (_stricmp(param->ImageName, buf) == 0) {

		//	target is the current file

		char desc[MAX_PATH];

		VfdMakeFileDesc(desc, sizeof(desc),
			param->FileType, param->ImageSize,
			GetFileAttributes(param->ImageName));

		SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, desc);
		SetControlText(hDlg, IDC_IMAGEFILE_HINT, MSG_CURRENT_FILE);

		if (param->DiskType == VFD_DISKTYPE_FILE) {

			//	cannot overwrite the current FILE disk image

			EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			return;
		}
	}

	//
	//	check target image file
	//
	ret = VfdCheckImageFile(
		buf, &file_attr, &file_type, &file_size);

	if (ret == ERROR_FILE_NOT_FOUND) {
		//	file does not exist
		SetControlText(hDlg, IDC_IMAGEFILE_DESC, MSG_DESC_NEW_FILE);
		EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
		return;
	}
	else if (ret != ERROR_SUCCESS) {
		SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, SystemMessage(ret));
		EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		return;
	}

	//	set target file description

	VfdMakeFileDesc(buf, sizeof(buf),
		file_type, file_size, file_attr);

	SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, buf);

	//	check target file type

	if (file_type == VFD_FILETYPE_ZIP) {
		SetControlText(hDlg, IDC_IMAGEFILE_HINT, MSG_TARGET_IS_ZIP);
		EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		return;
	}

	//	the target is an existing raw file

	EnableWindow(GetDlgItem(hDlg, IDC_OVERWRITE), TRUE);

	//	set truncate box

	if (file_size > VfdGetMediaSize(param->MediaType)) {
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), TRUE);
		SetControlText(hDlg, IDC_IMAGEFILE_HINT, MSG_SIZE_MISMATCH);
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
	}

	//	check overwrite setting

	if (IsDlgButtonChecked(hDlg, IDC_OVERWRITE) != BST_CHECKED) {
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
	}

	//	target file exists and overwritable

	SetWindowLong(hDlg, DWL_USER, 1);
}


//
//	Show save file dialog box
//
void OnBrowse(
	HWND			hDlg)
{
	OPENFILENAME	ofn;
	PSTR			title;
	CHAR			file[MAX_PATH];
	CHAR			dir[MAX_PATH];
	DWORD			len;

	title = ModuleMessage(MSG_SAVE_TITLE);

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(file, sizeof(file));
	ZeroMemory(dir, sizeof(dir));

	len = GetDlgItemText(hDlg, IDC_TARGETFILE, file, sizeof(file));

	if (len && file[len - 1] == '\\') {
		strcpy(dir, file);
		ZeroMemory(file, sizeof(file));
	}

	// Different structure sizes must be used for NT and 2K/XP
	ofn.lStructSize = IS_WINDOWS_NT() ?
		OPENFILENAME_SIZE_VERSION_400 : sizeof(ofn);

	ofn.hwndOwner	= hDlg;
	ofn.lpstrFile	= file;
	ofn.nMaxFile	= sizeof(file);
	ofn.lpstrInitialDir = dir;
	ofn.lpstrTitle	= title ? title : "Save Image";
	ofn.lpstrFilter	= "*.*\0*.*\0";
#ifndef __REACTOS__
	ofn.Flags		= OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
#else
	ofn.Flags		= OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
#endif

	if (GetSaveFileName(&ofn)) {
		SetDlgItemText(hDlg, IDC_TARGETFILE, file);
		SetFocus(GetDlgItem(hDlg, IDC_TARGETFILE));
	}

	if (title) {
		LocalFree(title);
	}
}

void OnOverwrite(
	HWND			hDlg,
	HWND			hCheck)
{
	if (GetWindowLong(hDlg, DWL_USER)) {
		//	the target file exists and overwritable
		if (SendMessage(hCheck, BM_GETCHECK, 0, 0) != BST_CHECKED) {
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		}
		else {
			EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
		}
	}
}

void OnTruncate(
	HWND			hDlg,
	HWND			hCheck)
{
	UNREFERENCED_PARAMETER(hDlg);
	UNREFERENCED_PARAMETER(hCheck);
}

//
//	Save image
//
DWORD OnOK(
	HWND			hDlg)
{
	PCSAVE_PARAM	param;
	CHAR			path[MAX_PATH];
	BOOL			overwrite;
	BOOL			truncate;
	DWORD			ret;

#ifndef __REACTOS__
	param = (PCSAVE_PARAM)GetWindowLong(hDlg, GWL_USERDATA);
#else
	param = (PCSAVE_PARAM)GetWindowLongPtr(hDlg, GWLP_USERDATA);
#endif

	if (!param) {
		return ERROR_INVALID_FUNCTION;
	}

	//	get the target image name

	if (GetDlgItemText(hDlg, IDC_TARGETFILE, path, sizeof(path)) == 0) {
		return ERROR_INVALID_FUNCTION;
	}

	if (GetWindowLong(hDlg, DWL_USER)) {
		//	the target file exists and overwritable
		overwrite = (IsDlgButtonChecked(hDlg, IDC_OVERWRITE) == BST_CHECKED);
		truncate = (IsDlgButtonChecked(hDlg, IDC_TRUNCATE) == BST_CHECKED);
	}
	else {
		overwrite = FALSE;
		truncate = TRUE;
	}

retry:
	ret = VfdDismountVolume(param->hDevice, FALSE);

	if (ret == ERROR_ACCESS_DENIED) {
		PSTR msg = ModuleMessage(MSG_UNMOUNT_CONFIRM);

		int reply = MessageBox(
			hDlg, msg ? msg : "retry", VFD_MSGBOX_TITLE,
			MB_ICONEXCLAMATION | MB_CANCELTRYCONTINUE);

		if (msg) {
			LocalFree(msg);
		}

		if (reply == IDRETRY) {
			goto retry;
		}
		else if (reply == IDCANCEL) {
			return ERROR_CANCELLED;
		}
		else {
			VfdDismountVolume(param->hDevice, TRUE);
		}
	}
	else if (ret != ERROR_SUCCESS) {

		MessageBox(hDlg, SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);

		return ret;
	}

	ret = VfdSaveImage(param->hDevice, path, overwrite, truncate);

	if (ret != ERROR_SUCCESS) {

		//	show error message

		MessageBox(hDlg, SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
	}

	return ret;
}
