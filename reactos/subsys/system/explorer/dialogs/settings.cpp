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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone, lean version
 //
 // dialogs/settings.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 18.01.2004
 //


#include "precomp.h"

#include "../explorer_intres.h"

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
 :	super(hwnd)
{
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
		break;

	  case PSN_APPLY:
		_display_version_org = SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, 0, MAKELONG(0,0));
		break;

	  case PSN_RESET:
		SendMessage(g_Globals._hwndShellView, PM_DISPLAY_VERSION, _display_version_org, MAKELONG(1,0));
		break;

	  default:
		return super::Notify(id, pnmh);
	}

	return 0;
}

int	DesktopSettingsDlg::Command(int id, int code)
{
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
 :	super(hwnd)
{
}

LRESULT TaskbarSettingsDlg::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		FillRect(canvas, &canvas.rcPaint, GetStockBrush(GRAY_BRUSH));
		break;}

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


StartmenuSettingsDlg::StartmenuSettingsDlg(HWND hwnd)
 :	super(hwnd)
{
}

LRESULT StartmenuSettingsDlg::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		FillRect(canvas, &canvas.rcPaint, GetStockBrush(DKGRAY_BRUSH));
		break;}

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}
