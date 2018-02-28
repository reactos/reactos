/*
	vfdshprop.cpp

	Virtual Floppy Drive for Windows
	Driver control library
	COM shell extension class property sheet functions

	Copyright (c) 2003-2005 Ken Kato
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"
#include "vfdver.h"
#ifndef __REACTOS__
#include "vfdmsg.h"
#else
#include "vfdmsg_lib.h"
#endif
#include "vfdguirc.h"

//	class header
#include "vfdshext.h"

//	property sheet property ID

#define VFD_PROPERTY_ID	"VFD"

//
//	local functions
//
#ifndef __REACTOS__
static BOOL CALLBACK VfdPageDlgProc(
#else
static INT_PTR CALLBACK VfdPageDlgProc(
#endif
	HWND			hDlg,
	UINT			uMessage,
	WPARAM			wParam,
	LPARAM			lParam);

static UINT CALLBACK VfdPageCallback(
	HWND			hWnd,
	UINT			uMessage,
	LPPROPSHEETPAGE	ppsp);

static void OnPropInit(HWND hDlg);
static void OnControl(HWND hDlg);
static void UpdateImageInfo(HWND hDlg, ULONG nDevice);

//
//	property sheet callback function
//
UINT CALLBACK VfdPageCallback(
	HWND			hWnd,
	UINT			uMessage,
	LPPROPSHEETPAGE	ppsp)
{
	UNREFERENCED_PARAMETER(hWnd);

	switch(uMessage) {
	case PSPCB_CREATE:
		return TRUE;

	case PSPCB_RELEASE:
		if (ppsp->lParam) {
			((LPCVFDSHEXT)(ppsp->lParam))->Release();
		}
		return TRUE;
	}
	return TRUE;
}

//
//	property page dialog procedure
//
#ifndef __REACTOS__
BOOL CALLBACK VfdPageDlgProc(
#else
INT_PTR CALLBACK VfdPageDlgProc(
#endif
	HWND			hDlg,
	UINT			uMessage,
	WPARAM			wParam,
	LPARAM			lParam)
{
	LPPROPSHEETPAGE psp;
	LPCVFDSHEXT		lpcs;

	switch (uMessage) {
	case WM_INITDIALOG:
#ifndef __REACTOS__
		SetWindowLong(hDlg, DWL_USER, lParam);
#else
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
#endif

		if (lParam) {
			lpcs = (LPCVFDSHEXT)((LPPROPSHEETPAGE)lParam)->lParam;

			OnPropInit(hDlg);
			UpdateImageInfo(hDlg, lpcs->GetDevice());
		}
		return TRUE;

	case WM_COMMAND:
#ifndef __REACTOS__
		psp = (LPPROPSHEETPAGE)GetWindowLong(hDlg, DWL_USER);
#else
		psp = (LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
#endif

		if (!psp) {
			break;
		}

		lpcs = (LPCVFDSHEXT)psp->lParam;

		if (!lpcs) {
			break;
		}

		switch (wParam) {
		case IDC_OPEN:
			if (lpcs->DoVfdOpen(hDlg) == ERROR_SUCCESS) {
				SendMessage((HWND)lParam,
					BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
			}
			UpdateImageInfo(hDlg, lpcs->GetDevice());
			break;

		case IDC_SAVE:
			if (lpcs->DoVfdSave(hDlg) == ERROR_SUCCESS) {
				SendMessage((HWND)lParam,
					BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
			}
			UpdateImageInfo(hDlg, lpcs->GetDevice());
			break;

		case IDC_CLOSE:
			if (lpcs->DoVfdClose(hDlg) == ERROR_SUCCESS) {
				SendMessage((HWND)lParam,
					BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
			}
			UpdateImageInfo(hDlg, lpcs->GetDevice());
			break;

		case IDC_WRITE_PROTECTED:
			lpcs->DoVfdProtect(hDlg);
			break;

		case IDC_FORMAT:
			VfdGuiFormat(hDlg, lpcs->GetDevice());
			break;

		case IDC_CONTROL:
			OnControl(hDlg);
			break;
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

	default:
		if (uMessage == g_nNotifyMsg) {
#ifndef __REACTOS__
			psp = (LPPROPSHEETPAGE)GetWindowLong(hDlg, DWL_USER);
#else
			psp = (LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
#endif

			if (!psp) {
				break;
			}

			lpcs = (LPCVFDSHEXT)psp->lParam;

			if (!lpcs) {
				break;
			}

			UpdateImageInfo(hDlg, lpcs->GetDevice());
		}
		break;
	}

	return FALSE;
}

//
//	initialize the property page
//
void OnPropInit(
	HWND			hDlg)
{
	//	set up control text

	SetDlgItemText(hDlg, IDC_PROPERTY_TITLE,	VFD_PRODUCT_DESC);
	SetDlgItemText(hDlg, IDC_COPYRIGHT_STR,		VFD_COPYRIGHT_STR);

	SetControlText(hDlg, IDC_IMAGEFILE_LABEL,	MSG_IMAGEFILE_LABEL);
	SetControlText(hDlg, IDC_IMAGEDESC_LABEL,	MSG_DESCRIPTION_LABEL);
	SetControlText(hDlg, IDC_DISKTYPE_LABEL,	MSG_DISKTYPE_LABEL);
	SetControlText(hDlg, IDC_MEDIATYPE_LABEL,	MSG_MEDIATYPE_LABEL);
	SetControlText(hDlg, IDC_WRITE_PROTECTED,	MSG_MENU_PROTECT);
	SetControlText(hDlg, IDC_OPEN,				MSG_OPEN_BUTTON);
	SetControlText(hDlg, IDC_SAVE,				MSG_SAVE_BUTTON);
	SetControlText(hDlg, IDC_CLOSE,				MSG_CLOSE_BUTTON);
	SetControlText(hDlg, IDC_FORMAT,			MSG_FORMAT_BUTTON);
	SetControlText(hDlg, IDC_CONTROL,			MSG_CONTROL_BUTTON);
}

//
//	Control Panel button is clicked
//
void OnControl(
	HWND			hDlg)
{
	CHAR			module_path[MAX_PATH];
	CHAR			full_path[MAX_PATH];
	PSTR			file_name;
#ifndef __REACTOS__
	DWORD			ret;
#else
	DWORD_PTR		ret;
#endif

	ret = GetModuleFileName(
		g_hDllModule, module_path, sizeof(module_path));

	if (ret == 0 || ret >= sizeof(module_path)) {
		file_name = full_path;
	}
	else {
		ret = GetFullPathName(
			module_path, sizeof(full_path), full_path, &file_name);

		if (ret == 0 || ret >= sizeof(full_path)) {
			file_name = full_path;
		}
	}

	strcpy(file_name, "vfdwin.exe");

	VFDTRACE(0, ("Starting %s\n", full_path));

#ifndef __REACTOS__
	ret = (DWORD)ShellExecute(
		hDlg, NULL, full_path, NULL, NULL, SW_SHOW);
#else
	ret = (DWORD_PTR)ShellExecute(
		hDlg, NULL, full_path, NULL, NULL, SW_SHOW);
#endif

	if (ret > 32) {
		PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
	}
	else {
		MessageBox(hDlg, SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
	}
}

//
//	Update image information on the property page
//
void UpdateImageInfo(
	HWND			hDlg,
	ULONG			nDevice)
{
	HANDLE			hDevice;
	CHAR			buf[MAX_PATH];
	VFD_DISKTYPE	disk_type;
	VFD_MEDIA		media_type;
	VFD_FLAGS		media_flags;
	VFD_FILETYPE	file_type;
	ULONG			image_size;
	DWORD			attrib;
	ULONG			ret;

	hDevice = VfdOpenDevice(nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		MessageBox(hDlg,
			SystemMessage(GetLastError()),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
		return;
	}

	//	get current image information

	ret = VfdGetImageInfo(
		hDevice,
		buf,
		&disk_type,
		&media_type,
		&media_flags,
		&file_type,
		&image_size);

	CloseHandle(hDevice);

	if (ret != ERROR_SUCCESS) {
		MessageBox(hDlg,
			SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
		return;
	}

	if (media_type == VFD_MEDIA_NONE) {

		//	drive is empty

		SetDlgItemText(hDlg, IDC_IMAGEFILE,	NULL);
		SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, NULL);
		SetDlgItemText(hDlg, IDC_DISKTYPE, NULL);
		SetDlgItemText(hDlg, IDC_MEDIATYPE, NULL);

		EnableWindow(GetDlgItem(hDlg, IDC_WRITE_PROTECTED), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_OPEN),	TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_SAVE),	FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CLOSE),	FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_FORMAT),	FALSE);

		SendMessage(GetDlgItem(hDlg, IDC_OPEN),
			BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);

		SetFocus(GetDlgItem(hDlg, IDC_OPEN));

		return;
	}

	//	display image file name

	if (buf[0]) {
		attrib = GetFileAttributes(buf);

		if (attrib == INVALID_FILE_ATTRIBUTES) {
			attrib = 0;
		}
	}
	else {
		if (disk_type != VFD_DISKTYPE_FILE) {
			strcpy(buf, "<RAM>");
		}
		attrib = 0;
	}

	SetDlgItemText(hDlg, IDC_IMAGEFILE, buf);

	//	display image description

	VfdMakeFileDesc(buf, sizeof(buf),
		file_type, image_size, attrib);

	SetDlgItemText(hDlg, IDC_IMAGEFILE_DESC, buf);

	//	display disk type

	if (disk_type == VFD_DISKTYPE_FILE) {
		SetDlgItemText(hDlg, IDC_DISKTYPE, "FILE");
	}
	else {
		SetDlgItemText(hDlg, IDC_DISKTYPE, "RAM");
	}

	//	display media type

	SetDlgItemText(hDlg, IDC_MEDIATYPE,
		VfdMediaTypeName(media_type));

	//	set write protect check box

	if (media_flags & VFD_FLAG_WRITE_PROTECTED) {
		CheckDlgButton(hDlg, IDC_WRITE_PROTECTED, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_WRITE_PROTECTED, BST_UNCHECKED);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_WRITE_PROTECTED), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_OPEN),	FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_SAVE),	TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_CLOSE),	TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_FORMAT),	TRUE);

	SendMessage(GetDlgItem(hDlg, IDC_CLOSE),
		BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);

	SetFocus(GetDlgItem(hDlg, IDC_CLOSE));
}

//
//	CVfdShExt class members inherited from IShellPropSheetExt
//

//	Add property page
STDMETHODIMP CVfdShExt::AddPages(
	LPFNADDPROPSHEETPAGE	lpfnAddPage,
	LPARAM					lParam)
{
	PROPSHEETPAGE	psp;
	HPROPSHEETPAGE	hpage;

	if (!m_pDataObj || m_nDevice == (ULONG)-1) {
		// not a VFD drive
		VFDTRACE(0, ("PropPage: Not a VFD drive\n"));

		return NOERROR;
	}

	psp.dwSize		= sizeof(psp);	// no extra data.
	psp.dwFlags		= PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
	psp.hInstance	= g_hDllModule;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPDIALOG);
	psp.hIcon		= 0;
	psp.pszTitle	= "VFD";
	psp.pfnDlgProc	= VfdPageDlgProc;
	psp.pcRefParent = &g_cDllRefCnt;
	psp.pfnCallback = VfdPageCallback;
	psp.lParam		= (LPARAM)this;

	AddRef();
	hpage = CreatePropertySheetPage(&psp);

	if (hpage) {
		if (!lpfnAddPage(hpage, lParam)) {
			DestroyPropertySheetPage(hpage);
			Release();
		}
	}

	return NOERROR;
}

STDMETHODIMP CVfdShExt::ReplacePage(
	UINT					uPageID,
	LPFNADDPROPSHEETPAGE	lpfnReplaceWith,
	LPARAM					lParam)
{
	UNREFERENCED_PARAMETER(uPageID);
	UNREFERENCED_PARAMETER(lpfnReplaceWith);
	UNREFERENCED_PARAMETER(lParam);
	return E_FAIL;
}
