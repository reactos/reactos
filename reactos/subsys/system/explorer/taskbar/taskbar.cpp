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
 // taskbar.cpp
 //
 // Martin Fuchs, 16.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "taskbar.h"
#include "traynotify.h"	// for NOTIFYAREA_WIDTH_DEF


TaskBarEntry::TaskBarEntry()
{
	_id = 0;
	_hbmp = 0;
	_bmp_idx = 0;
	_used = 0;
	_btn_idx = 0;
	_fsState = 0;
}

TaskBarMap::~TaskBarMap()
{
	while(!empty()) {
		iterator it = begin();
		DeleteBitmap(it->second._hbmp);
		erase(it);
	}
}


TaskBar::TaskBar(HWND hwnd)
 :	super(hwnd)
{
}

TaskBar::~TaskBar()
{
	KillTimer(_hwnd, 0);

	//DeinstallShellHook();
}

HWND TaskBar::Create(HWND hwndParent)
{
	ClientRect clnt(hwndParent);

	return Window::Create(WINDOW_CREATOR(TaskBar), 0,
							BtnWindowClass(CLASSNAME_TASKBAR), TITLE_TASKBAR, WS_CHILD|WS_VISIBLE,
							TASKBAR_LEFT, 0, clnt.right-TASKBAR_LEFT-(NOTIFYAREA_WIDTH_DEF+1), clnt.bottom, hwndParent);
}

LRESULT TaskBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	_htoolbar = CreateToolbarEx(_hwnd,
								WS_CHILD|WS_VISIBLE|CCS_NODIVIDER|CCS_TOP|
								TBSTYLE_LIST|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE,
								IDW_TASKTOOLBAR, 0, 0, 0, NULL, 0, 0, 0, 16, 16, sizeof(TBBUTTON));

	SendMessage(_htoolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(80,160));
	//SendMessage(_htoolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	//SendMessage(_htoolbar, TB_SETDRAWTEXTFLAGS, DT_CENTER|DT_VCENTER, DT_CENTER|DT_VCENTER);
	//SetWindowFont(_htoolbar, GetStockFont(ANSI_VAR_FONT), FALSE);

	_next_id = IDC_FIRST_APP;

	//InstallShellHook(_hwnd, PM_SHELLHOOK_NOTIFY);

	Refresh();

	SetTimer(_hwnd, 0, 200, NULL);

	return 0;
}

LRESULT TaskBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_SIZE:
		SendMessage(_htoolbar, WM_SIZE, 0, 0);
		break;

	  case WM_TIMER:
		Refresh();
		return 0;
/*
//#define PM_SHELLHOOK_NOTIFY		(WM_APP+0x10)

	  case PM_SHELLHOOK_NOTIFY: {
		int code = lparam;

		switch(code) {
		  case HSHELL_WINDOWCREATED:
		  case HSHELL_WINDOWDESTROYED:
		  case HSHELL_WINDOWACTIVATED:
		  case HSHELL_WINDOWREPLACED:
			Refresh();
			break;
		}
		Refresh();
		break;}
*/
	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int TaskBar::Command(int id, int code)
{
	TaskBarMap::iterator found = _map.find_id(id);

	if (found != _map.end()) {
		ActivateApp(found);
		return 0;
	}

	return super::Command(id, code);
}

int TaskBar::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->hwndFrom == _htoolbar)
		switch(pnmh->code) {
		  case NM_RCLICK: {
			TBBUTTONINFO btninfo;
			TaskBarMap::iterator it;
			Point pt(GetMessagePos());
			ScreenToClient(_htoolbar, &pt);

			btninfo.cbSize = sizeof(TBBUTTONINFO);
			btninfo.dwMask = TBIF_BYINDEX|TBIF_COMMAND;

			int idx = SendMessage(_htoolbar, TB_HITTEST, 0, (LPARAM)&pt);

			if (idx>=0 &&
				SendMessage(_htoolbar, TB_GETBUTTONINFO, idx, (LPARAM)&btninfo)!=-1 &&
				(it=_map.find_id(btninfo.idCommand))!=_map.end()) {
				//TaskBarEntry& entry = it->second;

				ActivateApp(it, false);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
				static DynamicFct<DWORD(STDAPICALLTYPE*)(RESTRICTIONS)> pSHRestricted(TEXT("SHELL32"), "SHRestricted");

				if (pSHRestricted && !(*pSHRestricted)(REST_NOTRAYCONTEXTMENU))
#endif
					ShowAppSystemMenu(it);
			}
			break;}

		  default:
			return super::Notify(id, pnmh);
		}

	return 0;
}


void TaskBar::ActivateApp(TaskBarMap::iterator it, bool can_minimize)
{
	HWND hwnd = it->first;

	if (can_minimize && (hwnd==GetForegroundWindow() || hwnd==_last_foreground_wnd)) {
		PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		_last_foreground_wnd = 0;
	} else {
		 // switch to selected application window
		if (IsIconic(hwnd))
			PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		SetForegroundWindow(hwnd);

		_last_foreground_wnd = hwnd;
	}

	Refresh();
}

void TaskBar::ShowAppSystemMenu(TaskBarMap::iterator it)
{
	HMENU hmenu = GetSystemMenu(it->first, FALSE);

	if (hmenu) {
		POINT pt;

		GetCursorPos(&pt);
		int cmd = TrackPopupMenu(hmenu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON|TPM_RETURNCMD, pt.x, pt.y, 0, _hwnd, NULL);

		if (cmd)
			PostMessage(it->first, WM_SYSCOMMAND, cmd, 0);
	}
}


static HICON get_window_icon(HWND hwnd)
{
	HICON hIcon = 0;

	SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		hIcon = (HICON)GetClassLong(hwnd, GCL_HICONSM);

	if (!hIcon)
		hIcon = (HICON)GetClassLong(hwnd, GCL_HICON);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		hIcon = LoadIcon(0, IDI_APPLICATION);

	return hIcon;
}

 // fill task bar with buttons for enumerated top level windows
BOOL CALLBACK TaskBar::EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TaskBar* pThis = (TaskBar*)lparam;

	DWORD style = GetWindowStyle(hwnd);
	DWORD ex_style = GetWindowExStyle(hwnd);

	if ((style&WS_VISIBLE) && !(ex_style&WS_EX_TOOLWINDOW) &&
		!GetParent(hwnd) && !GetWindow(hwnd,GW_OWNER)) {
		TCHAR title[BUFFER_LEN];

		if (!GetWindowText(hwnd, title, BUFFER_LEN))
			title[0] = '\0';

		TaskBarMap::iterator found = pThis->_map.find(hwnd);
		int last_id = 0;

		if (found != pThis->_map.end()) {
			last_id = found->second._id;

			if (!last_id)
				found->second._id = pThis->_next_id++;
		} else {
			HBITMAP hbmp;
			HICON hIcon = get_window_icon(hwnd);

			if (hIcon) {
				hbmp = create_bitmap_from_icon(hIcon, GetSysColorBrush(COLOR_BTNFACE), WindowCanvas(pThis->_htoolbar));
				DestroyIcon(hIcon); // some icons can be freed, some not - so ignore any error return of DestroyIcon()
			} else
				hbmp = 0;

			TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
			int bmp_idx = SendMessage(pThis->_htoolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);

			TaskBarEntry entry;

			entry._id = pThis->_next_id++;
			entry._hbmp = hbmp;
			entry._bmp_idx = bmp_idx;
			entry._title = title;

			pThis->_map[hwnd] = entry;
			found = pThis->_map.find(hwnd);
		}

		TBBUTTON btn = {-2/*I_IMAGENONE*/, 0, TBSTATE_ENABLED/*|TBSTATE_ELLIPSES*/, BTNS_BUTTON, {0, 0}, 0, 0};
		TaskBarEntry& entry = found->second;

		++entry._used;
		btn.idCommand = entry._id;

		HWND foreground = GetForegroundWindow();
		HWND foreground_owner = GetWindow(foreground, GW_OWNER);

		if (hwnd==foreground || hwnd==foreground_owner) {
			btn.fsState |= TBSTATE_PRESSED|TBSTATE_CHECKED;
			pThis->_last_foreground_wnd = hwnd;
		}

		if (!last_id) {
			 // create new toolbar buttons for new windows
			if (title[0])
				btn.iString = (INT_PTR)title;

			btn.iBitmap = entry._bmp_idx;
			entry._btn_idx = SendMessage(pThis->_htoolbar, TB_BUTTONCOUNT, 0, 0);

			SendMessage(pThis->_htoolbar, TB_INSERTBUTTON, entry._btn_idx, (LPARAM)&btn);
			SendMessage(pThis->_htoolbar, TB_AUTOSIZE, 0, 0);	///@todo useless?
		} else {
			 // refresh attributes of existing buttons
			if (btn.fsState != entry._fsState)
				SendMessage(pThis->_htoolbar, TB_SETSTATE, entry._id, MAKELONG(btn.fsState,0));

			if (entry._title != title) {
				TBBUTTONINFO info;

				info.cbSize = sizeof(TBBUTTONINFO);
				info.dwMask = TBIF_TEXT;
				info.pszText = title;

				SendMessage(pThis->_htoolbar, TB_SETBUTTONINFO, entry._id, (LPARAM)&info);

				entry._title = title;
			}
		}

		entry._fsState = btn.fsState;

		 // move minimized windows out of sight
		if (IsIconic(hwnd)) {
			RECT rect;

			GetWindowRect(hwnd, &rect);

			if (rect.bottom > 0)
				SetWindowPos(hwnd, 0, -32000, -32000, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}

	return TRUE;
}

void TaskBar::Refresh()
{
	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it)
		it->second._used = 0;

	EnumWindows(EnumWndProc, (LPARAM)this);
	//EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), EnumWndProc, (LPARAM)_htoolbar);

	set<int> btn_idx_to_delete;
	set<HBITMAP> hbmp_to_delete;

	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
		TaskBarEntry& entry = it->second;

		if (!entry._used && entry._id) {
			 // store button indexes to remove
			btn_idx_to_delete.insert(entry._btn_idx);
			hbmp_to_delete.insert(entry._hbmp);
			entry._id = 0;
		}
	}

	if (!btn_idx_to_delete.empty()) {
		 // remove buttons from right to left
		for(set<int>::reverse_iterator it=btn_idx_to_delete.rbegin(); it!=btn_idx_to_delete.rend(); ++it) {
			int idx = *it;

			SendMessage(_htoolbar, TB_DELETEBUTTON, idx, 0);

			for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
				TaskBarEntry& entry = it->second;

				 // adjust button indexes
				if (entry._btn_idx > idx) {
					--entry._btn_idx;
					--entry._bmp_idx;

					TBBUTTONINFO info;

					info.cbSize = sizeof(TBBUTTONINFO);
					info.dwMask = TBIF_IMAGE;
					info.iImage = entry._bmp_idx;

					SendMessage(_htoolbar, TB_SETBUTTONINFO, entry._id, (LPARAM)&info);
				}
			}
		}

		for(set<HBITMAP>::iterator it=hbmp_to_delete.begin(); it!=hbmp_to_delete.end(); ++it) {
			HBITMAP hbmp = *it;

			TBREPLACEBITMAP tbrepl = {0, (UINT_PTR)hbmp, 0, 0};
			SendMessage(_htoolbar, TB_REPLACEBITMAP, 0, (LPARAM)&tbrepl);

			DeleteObject(hbmp);

			for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it)
				if (it->second._hbmp == hbmp) {
					_map.erase(it);
					break;
				}
		}

		SendMessage(_htoolbar, TB_AUTOSIZE, 0, 0);	///@todo useless?
	}
}

TaskBarMap::iterator TaskBarMap::find_id(int id)
{
	for(iterator it=begin(); it!=end(); ++it)
		if (it->second._id == id)
			return it;

	return end();
}
