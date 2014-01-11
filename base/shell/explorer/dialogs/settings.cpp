/*
 * Copyright 2004 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // dialogs/settings.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 18.01.2004
 //


#include <precomp.h>

#include "../taskbar/traynotify.h"
#include "settings.h"


void ExplorerPropertySheet(HWND hparent)
{
	PropertySheetDialog ps(hparent);

	ps.dwFlags |= PSH_USEICONID | PSH_PROPTITLE;
	ps.pszIcon = MAKEINTRESOURCE(IDI_REACTOS);
	ps.pszCaption = TEXT("Explorer");

	PropSheetPage psp1(IDD_DESKBAR_DESKTOP, WINDOW_CREATOR(DesktopSettingsDlg));
	psp1.dwFlags |= PSP_USETITLE;
	psp1.pszTitle = MAKEINTRESOURCE(IDS_DESKTOP);
	ps.add(psp1);

	PropSheetPage psp2(IDD_DESKBAR_TASKBAR, WINDOW_CREATOR(TaskbarSettingsDlg));
	psp2.dwFlags |= PSP_USETITLE;
	psp2.pszTitle = MAKEINTRESOURCE(IDS_TASKBAR);
	ps.add(psp2);

	PropSheetPage psp3(IDD_DESKBAR_STARTMENU, WINDOW_CREATOR(StartmenuSettingsDlg));
	psp3.dwFlags |= PSP_USETITLE;
	psp3.pszTitle = MAKEINTRESOURCE(IDS_STARTMENU);
	ps.add(psp3);

	ps.DoModal();
}


DesktopSettingsDlg::DesktopSettingsDlg(HWND hwnd)
 :	super(hwnd),
	_bmp0(IDB_ICON_ALIGN_0),
	_bmp1(IDB_ICON_ALIGN_1),
	_bmp2(IDB_ICON_ALIGN_2),
	_bmp3(IDB_ICON_ALIGN_3),
	_bmp4(IDB_ICON_ALIGN_4),
	_bmp5(IDB_ICON_ALIGN_5),
	_bmp6(IDB_ICON_ALIGN_6),
	_bmp7(IDB_ICON_ALIGN_7),
	_bmp8(IDB_ICON_ALIGN_8),
	_bmp9(IDB_ICON_ALIGN_9),
	_bmp10(IDB_ICON_ALIGN_10)
{
	new PictureButton(_hwnd, IDC_ICON_ALIGN_0, _bmp0);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_1, _bmp1);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_2, _bmp2);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_3, _bmp3);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_4, _bmp4);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_5, _bmp5);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_6, _bmp6);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_7, _bmp7);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_8, _bmp8);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_9, _bmp9);
	new PictureButton(_hwnd, IDC_ICON_ALIGN_10, _bmp10);

	_alignment_cur = SendMessage(g_Globals._hwndShellView, PM_GET_ICON_ALGORITHM, 0, 0);
	_alignment_tmp = _alignment_cur;

	_display_version_org = SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, 0, MAKELONG(0,0));
	CheckDlgButton(hwnd, ID_DESKTOP_VERSION, _display_version_org? BST_CHECKED: BST_UNCHECKED);
}

#ifndef PSN_QUERYINITIALFOCUS	// currently (as of 18.01.2004) missing in MinGW headers
#define PSN_QUERYINITIALFOCUS (-213)
#endif

int DesktopSettingsDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case PSN_QUERYINITIALFOCUS:
		SetWindowLongPtr(_hwnd, DWL_MSGRESULT, (LPARAM)GetDlgItem(_hwnd, IDC_ICON_ALIGN_0+_alignment_cur));
		break;

	  case PSN_APPLY:
		_alignment_cur = _alignment_tmp;
		_display_version_org = SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, 0, MAKELONG(0,0));
		break;

	  case PSN_RESET:
		if (_alignment_tmp != _alignment_cur)
			SendMessage(g_Globals._hwndShellView, PM_SET_ICON_ALGORITHM, _alignment_cur, 0);
		SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, _display_version_org, MAKELONG(1,0));
		break;

	  default:
		return super::Notify(id, pnmh);
	}

	return 0;
}

int	DesktopSettingsDlg::Command(int id, int code)
{
	if (id>=IDC_ICON_ALIGN_0 && id<=IDC_ICON_ALIGN_10) {
		int alignment = id - IDC_ICON_ALIGN_0;

		if (alignment != _alignment_tmp) {
			_alignment_tmp = alignment;

			PropSheet_Changed(GetParent(_hwnd), _hwnd);

			SendMessage(g_Globals._hwndShellView, PM_SET_ICON_ALGORITHM, alignment, 0);
		}

		return 0;
	}

	switch(id) {
	  case ID_DESKTOP_VERSION:
		SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, 0, MAKELONG(0,1));	// toggle version display flag
		PropSheet_Changed(GetParent(_hwnd), _hwnd);
		break;

	  default:
		return 1;
	}

	return 0;
}


TaskbarSettingsDlg::TaskbarSettingsDlg(HWND hwnd)
 :	super(hwnd),
	_cfg_org(g_Globals._cfg)
{
	XMLPos options = g_Globals.get_cfg("desktopbar/options");

 	CheckDlgButton(hwnd, ID_SHOW_CLOCK, XMLBool(options, "show-clock", true)? BST_CHECKED: BST_UNCHECKED);
	CheckDlgButton(hwnd, ID_HIDE_INACTIVE_ICONS, XMLBool(options, "hide-inactive", true)? BST_CHECKED: BST_UNCHECKED);
}

int TaskbarSettingsDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case PSN_APPLY:
		_cfg_org = g_Globals._cfg;
		break;

	  case PSN_RESET:
		g_Globals._cfg = _cfg_org;
		SendMessage(g_Globals._hwndDesktopBar, PM_REFRESH_CONFIG, 0, 0);
		break;

	  default:
		return super::Notify(id, pnmh);
	}

	return 0;
}

int	TaskbarSettingsDlg::Command(int id, int code)
{
	switch(id) {
	  case ID_CONFIG_NOTIFYAREA:
		Dialog::DoModal(IDD_NOTIFYAREA, WINDOW_CREATOR(TrayNotifyDlg), _hwnd);
		break;

	  case ID_SHOW_CLOCK: {
		XMLBoolRef boolRef1(XMLPos(g_Globals.get_cfg("desktopbar/options")), "show-clock", true);
		boolRef1.toggle();
		SendMessage(g_Globals._hwndDesktopBar, PM_REFRESH_CONFIG, 0, 0);
		PropSheet_Changed(GetParent(_hwnd), _hwnd);
		break;}

	  case ID_HIDE_INACTIVE_ICONS: {
		XMLBoolRef boolRef2(XMLPos(g_Globals.get_cfg("notify-icons/options")), "hide-inactive", true);
        boolRef2.toggle();
		SendMessage(g_Globals._hwndDesktopBar, PM_REFRESH_CONFIG, 0, 0);
		PropSheet_Changed(GetParent(_hwnd), _hwnd);
		break;}

	  default:
		return 1;
	}

	return 0;
}


StartmenuSettingsDlg::StartmenuSettingsDlg(HWND hwnd)
 :	super(hwnd)
{
}

int	StartmenuSettingsDlg::Command(int id, int code)
{
/*
	switch(id) {
	  case ID_CONFIG_NOTIFYAREA:
		Dialog::DoModal(IDD_NOTIFYAREA, WINDOW_CREATOR(TrayNotifyDlg), _hwnd);
		return 0;
	}
*/
	return 1;
}


MdiSdiDlg::MdiSdiDlg(HWND hwnd)
 :	super(hwnd)
{
	CenterWindow(hwnd);

	XMLPos explorer_options = g_Globals.get_cfg("general/explorer");
	bool mdi = XMLBool(explorer_options, "mdi", true);
	bool separateFolders = XMLBool(explorer_options, "separate-folders", true);

	int id = mdi? IDC_MDI: IDC_SDI;
	CheckDlgButton(hwnd, id, BST_CHECKED);
	SetFocus(GetDlgItem(hwnd, id));

	CheckDlgButton(hwnd, IDC_SEPARATE_SUBFOLDERS, separateFolders?BST_CHECKED:BST_UNCHECKED);
}

int	MdiSdiDlg::Command(int id, int code)
{
	if (code == BN_CLICKED) {
		switch(id) {
		  case IDOK: {
			bool mdi = IsDlgButtonChecked(_hwnd, IDC_MDI)==BST_CHECKED;
			bool separateFolders = IsDlgButtonChecked(_hwnd, IDC_SEPARATE_SUBFOLDERS)==BST_CHECKED;

			XMLPos explorer_options = g_Globals.get_cfg("general/explorer");

			XMLBoolRef(explorer_options, "mdi") = mdi;
			XMLBoolRef(explorer_options, "separate-folders") = separateFolders;
		  } // fall through

		  case IDCANCEL:
			EndDialog(_hwnd, id);
			break;
		}

		return 0;
	}

	return 1;
}
