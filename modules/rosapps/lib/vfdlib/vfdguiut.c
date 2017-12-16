/*
	vfdguiut.c

	Virtual Floppy Drive for Windows
	Driver control library
	open / close / format GUI utility functions

	Copyright (c) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

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
//	message box constants added since Win2K
//

#ifndef MB_CANCELTRYCONTINUE
#define MB_CANCELTRYCONTINUE	0x00000006L
#endif

#ifndef IDTRYAGAIN
#define IDTRYAGAIN				10
#endif

#ifndef IDCONTINUE
#define IDCONTINUE				11
#endif

//
//	local funcitons
//
static PSTR FormatSizeBytes(ULONG size, PSTR buf)
{
	ULONG comma = 1;
	int len;

	while ((comma * 1000) < size) {
		comma *= 1000;
	}

	len = sprintf(buf, "%lu", size / comma);

	while (comma > 1) {
		size %= comma;
		comma /= 1000;
		len += sprintf(buf + len, ",%03lu", size / comma);
	}

	return buf;
}

static PSTR FormatSizeUnits(ULONG size, PSTR buf)
{
	static const char *name[3] = {
		" KB", " MB", " GB"
	};
	int unit;
	double dsize;

	if (size < 1000) {
#ifndef __REACTOS__
		sprintf(buf, "%u", size);
#else
		sprintf(buf, "%lu", size);
#endif
		return buf;
	}

	dsize = size;
	dsize /= 1024;
	unit = 0;

	while (unit < 2 && dsize >= 1000) {
		dsize /= 1000;
		unit++;
	}

	if (dsize < 10) {
		sprintf(buf, "%3.2f%s", dsize, name[unit]);
	}
	else if (dsize < 100) {
		sprintf(buf, "%3.1f%s", dsize, name[unit]);
	}
	else if (dsize < 1000) {
		sprintf(buf, "%3.0f%s", dsize, name[unit]);
	}
	else {
		FormatSizeBytes((ULONG)dsize, buf);
		strcat(buf, name[unit]);
	}

	return buf;
}

//
//	Close the current image
//
DWORD WINAPI VfdGuiClose(
	HWND			hParent,	//	parent window
	ULONG			nDevice)	//	device number
{
	HANDLE			hDevice;
	SAVE_PARAM		param;
	CHAR			path[MAX_PATH];
	HCURSOR			hourglass;
	DWORD			ret;
	int				reply;

	VFDTRACE(0, ("VfdGuiClose()\n"));

	hDevice = VfdOpenDevice(nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	//	get current image information

	ret = VfdGetImageInfo(
		hDevice,
		path,
		&param.DiskType,
		&param.MediaType,
		&param.MediaFlags,
		&param.FileType,
		&param.ImageSize);

	if (ret != ERROR_SUCCESS) {
		CloseHandle(hDevice);
		return ret;
	}

	param.hDevice = hDevice;
	param.ImageName = path;

	//	check if RAM image is modified

	if (param.MediaFlags & VFD_FLAG_DATA_MODIFIED) {
		PSTR msg = ModuleMessage(MSG_MEDIA_MODIFIED);

		for (;;) {
			reply = MessageBox(hParent, msg ? msg : "save?",
				VFD_MSGBOX_TITLE, MB_ICONQUESTION | MB_YESNOCANCEL);

			if (reply != IDYES) {
				break;
			}

			if (GuiSaveParam(hParent, &param) == ERROR_SUCCESS) {
				break;
			}
		}

		if (msg) {
			LocalFree(msg);
		}

		if (reply == IDCANCEL) {
			CloseHandle(hDevice);
			return ERROR_CANCELLED;
		}
	}

	//	close the image

	hourglass = LoadCursor(NULL, IDC_WAIT);

	for (;;) {

		//	show the hourglass cursor

		HCURSOR original = SetCursor(hourglass);

		//	close the current image

		ret = VfdCloseImage(hDevice, FALSE);

		//	restore the original cursor

		SetCursor(original);

		if (ret != ERROR_ACCESS_DENIED) {
			//	success or errors other than access denied
			break;
		}

		if (IS_WINDOWS_NT()) {

			//	Windows NT -- cannot force close
			//	show retry / cancel message box

			PSTR msg = ModuleMessage(MSG_UNMOUNT_FAILED);

			reply = MessageBox(
				hParent, msg ? msg : "retry", VFD_MSGBOX_TITLE,
				MB_ICONEXCLAMATION | MB_RETRYCANCEL);

			if (msg) {
				LocalFree(msg);
			}
		}
		else {

			//	Windows 2000 and later -- possible to force
			//	show cancel / retry / continue message box

			PSTR msg = ModuleMessage(MSG_UNMOUNT_CONFIRM);

			reply = MessageBox(
				hParent, msg ? msg : "retry", VFD_MSGBOX_TITLE,
				MB_ICONEXCLAMATION | MB_CANCELTRYCONTINUE);

			if (msg) {
				LocalFree(msg);
			}

			if (reply == IDCONTINUE) {

				//	try forced close

				ret = VfdCloseImage(hDevice, TRUE);
			}
		}

		if (reply == IDCANCEL) {
			ret = ERROR_CANCELLED;
			break;
		}
		else if (reply == IDCONTINUE) {

			//	try forced close

			ret = VfdCloseImage(hDevice, TRUE);
			break;
		}
	}

	CloseHandle(hDevice);

	return ret;
}

//
//	Format the current media
//
DWORD WINAPI VfdGuiFormat(
	HWND			hParent,	//	parent window
	ULONG			nDevice)	//	device number
{
	HANDLE			hDevice;
	ULONG			ret;
	PSTR			msg;

	msg = ModuleMessage(MSG_FORMAT_WARNING);

	ret = MessageBox(hParent,
		msg ? msg : "Format?",
		VFD_MSGBOX_TITLE,
		MB_ICONEXCLAMATION | MB_OKCANCEL);

	if (msg) {
		LocalFree(msg);
	}

	if (ret == IDCANCEL) {
		MessageBox(hParent,
			SystemMessage(ERROR_CANCELLED),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);

		return ERROR_CANCELLED;
	}

	hDevice = VfdOpenDevice(nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
	}
	else {
		HCURSOR original;

retry:
		original = SetCursor(LoadCursor(NULL, IDC_WAIT));

		ret = VfdDismountVolume(hDevice, FALSE);

		if (ret == ERROR_ACCESS_DENIED) {
			PSTR msg;
			int reply;

			SetCursor(original);

			msg = ModuleMessage(MSG_UNMOUNT_CONFIRM);

			reply = MessageBox(
				hParent, msg ? msg : "retry", VFD_MSGBOX_TITLE,
				MB_ICONEXCLAMATION | MB_CANCELTRYCONTINUE);

			if (msg) {
				LocalFree(msg);
			}

			if (reply == IDRETRY) {
				goto retry;
			}
			else if (reply == IDCANCEL) {
				ret = ERROR_CANCELLED;
			}
			else {
				VfdDismountVolume(hDevice, TRUE);
				ret = ERROR_SUCCESS;
			}
		}

		if (ret == ERROR_SUCCESS) {
			ret = VfdFormatMedia(hDevice);
		}

		SetCursor(original);

		CloseHandle(hDevice);
	}

	MessageBox(hParent,
		SystemMessage(ret),
		VFD_MSGBOX_TITLE,
		ret == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONSTOP);

	return ret;
}

//
//	Set a text to a dialog control
//
void SetControlText(
	HWND			hWnd,
	UINT			nCtrl,
	DWORD			nMsg)
{
	PSTR p = NULL;

	if (nMsg) {
		p = ModuleMessage(nMsg);
	}

	if (nCtrl) {
		SetDlgItemText(hWnd, nCtrl, p);
	}
	else {
		SetWindowText(hWnd, p);
	}

	if (p) {
		LocalFree(p);
	}
}

//
//	Make file description text
//
void WINAPI VfdMakeFileDesc(
	PSTR			pBuffer,
	ULONG			nBufSize,
	VFD_FILETYPE	nFileType,
	ULONG			nFileSize,
	DWORD			nFileAttr)
{
	PSTR			type_str;
	PSTR			size_str;
	PSTR			attr_ro;
	PSTR			attr_enc;
	PSTR			attr_cmp;

	ZeroMemory(pBuffer, nBufSize);

	switch (nFileType) {
	case VFD_FILETYPE_RAW:
		type_str = ModuleMessage(MSG_FILETYPE_RAW);
		break;

	case VFD_FILETYPE_ZIP:
		type_str = ModuleMessage(MSG_FILETYPE_ZIP);
		break;

	default:
		type_str = NULL;
	}

	if (nFileSize == INVALID_FILE_SIZE) {
		size_str = ModuleMessage(MSG_DESC_NEW_FILE);
	}
	else {
		CHAR buf[20], buf2[20];
		size_str = ModuleMessage(MSG_DESC_FILESIZE, 
			FormatSizeBytes(nFileSize, buf),
			FormatSizeUnits(nFileSize, buf2));
	}

	attr_ro = NULL;
	attr_cmp = NULL;
	attr_enc = NULL;

	if (nFileAttr != INVALID_FILE_ATTRIBUTES) {
		if (nFileAttr & FILE_ATTRIBUTE_READONLY) {
			attr_ro = ModuleMessage(MSG_ATTR_READONLY);
		}

		if (nFileAttr & FILE_ATTRIBUTE_COMPRESSED) {
			attr_cmp = ModuleMessage(MSG_ATTR_COMPRESSED);
		}

		if (nFileAttr & FILE_ATTRIBUTE_ENCRYPTED) {
			attr_enc = ModuleMessage(MSG_ATTR_ENCRYPTED);
		}
	}

	_snprintf(pBuffer, nBufSize - 1, "%s %s %s %s %s",
		type_str ? type_str : "",
		size_str ? size_str : "",
		attr_ro  ? attr_ro	: "",
		attr_cmp ? attr_cmp : "",
		attr_enc ? attr_enc : "");

	if (type_str) {
		LocalFree(type_str);
	}
	if (size_str) {
		LocalFree(size_str);
	}
	if (attr_ro) {
		LocalFree(attr_ro);
	}
	if (attr_cmp) {
		LocalFree(attr_cmp);
	}
	if (attr_enc) {
		LocalFree(attr_enc);
	}
}

void ShowContextMenu(
	HWND			hDlg,
	HWND			hCtl,
	LPARAM			lParam)
{
	POINT			pt;
	UINT			id;
	HMENU			hMenu;

	pt.x = ((int)(short)LOWORD(lParam));
	pt.y = ((int)(short)HIWORD(lParam));

	if (pt.x == -1 || pt.y == -1) {
		RECT rc;

		GetWindowRect(hCtl, &rc);
		pt.x = (rc.left + rc.right) / 2;
		pt.y = (rc.top + rc.bottom) / 2;

		id = GetDlgCtrlID(hCtl);
	}
	else {
		POINT pt2 = pt;

		ScreenToClient(hDlg, &pt2);

		id = GetDlgCtrlID(
			ChildWindowFromPoint(hDlg, pt2));
	}

	if (id < IDC_IMAGEFILE_LABEL ||
		id > IDC_TRUNCATE) {
		return;
	}

	hMenu = CreatePopupMenu();

	AppendMenu(hMenu, MF_STRING, 1, "&What's This");

	if (TrackPopupMenu(hMenu, TPM_RETURNCMD,
		pt.x, pt.y, 0, hDlg, NULL))
	{
		ShowHelpWindow(hDlg, id);
	}

	DestroyMenu(hMenu);
}

//
//	Show tool tip help
//
void ShowHelpWindow(
	HWND			hDlg,
	UINT			nCtl)
{
	UINT			msg;
	RECT			rc;
	PSTR			help;

	switch (nCtl) {
	case IDC_IMAGEFILE_LABEL:
	case IDC_IMAGEFILE:
		msg = MSG_HELP_IMAGEFILE;
		break;
	case IDC_IMAGEDESC_LABEL:
	case IDC_IMAGEFILE_DESC:
		msg = MSG_HELP_IMAGEDESC;
		break;
	case IDC_TARGETFILE_LABEL:
	case IDC_TARGETFILE:
		msg = MSG_HELP_TARGETFILE;
		break;
	case IDC_DISKTYPE_LABEL:
	case IDC_DISKTYPE:
	case IDC_DISKTYPE_FILE:
	case IDC_DISKTYPE_RAM:
		msg = MSG_HELP_DISKTYPE;
		break;
	case IDC_MEDIATYPE_LABEL:
	case IDC_MEDIATYPE:
		msg = MSG_HELP_MEDIATYPE;
		break;
	case IDC_WRITE_PROTECTED:
		msg = MSG_HELP_PROTECT_NOW;
		break;
	case IDC_OPEN_PROTECTED:
		msg = MSG_HELP_PROTECT_OPEN;
		break;
	case IDC_BROWSE:
		msg = MSG_HELP_BROWSE;
		break;
	case IDC_OPEN:
		msg = MSG_HELP_OPEN;
		break;
	case IDC_SAVE:
		msg = MSG_HELP_SAVE;
		break;
	case IDC_CLOSE:
		msg = MSG_HELP_CLOSE;
		break;
	case IDC_FORMAT:
		msg = MSG_HELP_FORMAT;
		break;
	case IDC_CONTROL:
		msg = MSG_HELP_CONTROL;
		break;
	case IDC_OVERWRITE:
		msg = MSG_HELP_OVERWRITE;
		break;
	case IDC_TRUNCATE:
		msg = MSG_HELP_TRUNCATE;
		break;
	default:
		return;
	}

	GetWindowRect(GetDlgItem(hDlg, nCtl), &rc);

	help = ModuleMessage(msg);

	if (help) {
		VfdToolTip(hDlg, help, rc.left, (rc.top + rc.bottom) / 2, TRUE);
		LocalFree(help);
	}
}

