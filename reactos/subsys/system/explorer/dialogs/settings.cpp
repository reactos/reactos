/*
 * Copyright 2003 Martin Fuchs
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
 // Explorer clone
 //
 // dialogs/settings.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 18.01.2004
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"
#include "../desktop/desktop.h"

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
}

#ifndef PSN_QUERYINITIALFOCUS	// currently (as of 18.01.2004) missing in MinGW headers
#define PSN_QUERYINITIALFOCUS (-213)
#endif

int DesktopSettingsDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case PSN_QUERYINITIALFOCUS:
		SetWindowLong(_hwnd, DWL_MSGRESULT, (LPARAM)GetDlgItem(_hwnd, IDC_ICON_ALIGN_0+_alignment_cur));
		break;

	  case PSN_APPLY:
		_alignment_cur = _alignment_tmp;
		break;

	  case PSN_RESET:
		if (_alignment_tmp != _alignment_cur)
			SendMessage(g_Globals._hwndShellView, PM_SET_ICON_ALGORITHM, _alignment_cur, 0);
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
	}

	return 1;
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
