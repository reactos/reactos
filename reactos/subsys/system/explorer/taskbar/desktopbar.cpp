/*
 * Copyright 2003, 2004 Martin Fuchs
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
 // desktopbar.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "desktopbar.h"
#include "taskbar.h"
#include "startmenu.h"
#include "traynotify.h"
#include "quicklaunch.h"

#include "../dialogs/settings.h"


DesktopBar::DesktopBar(HWND hwnd)
 :	super(hwnd),
	_trayIcon(hwnd, ID_TRAY_VOLUME)
{
	SystemParametersInfo(SPI_GETWORKAREA, 0, &_work_area_org, 0);
}

DesktopBar::~DesktopBar()
{
	 // restore work area to the previous size
	SystemParametersInfo(SPI_SETWORKAREA, 0, &_work_area_org, 0);
	PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETWORKAREA, 0);

	 // exit application after destroying desktop window
	PostQuitMessage(0);
}


HWND DesktopBar::Create()
{
	static BtnWindowClass wcDesktopBar(CLASSNAME_EXPLORERBAR);

	RECT rect;

	rect.left = -2;	// hide left border
#ifdef TASKBAR_AT_TOP
	rect.top = -2;	// hide top border
#else
	rect.top = GetSystemMetrics(SM_CYSCREEN) - DESKTOPBARBAR_HEIGHT;
#endif
	rect.right = GetSystemMetrics(SM_CXSCREEN) + 2;
	rect.bottom = rect.top + DESKTOPBARBAR_HEIGHT + 2;

	return Window::Create(WINDOW_CREATOR(DesktopBar), WS_EX_PALETTEWINDOW,
							wcDesktopBar, TITLE_EXPLORERBAR,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE,
							rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);
}


LRESULT DesktopBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	 // create start button
	ResString start_str(IDS_START);
	WindowCanvas canvas(_hwnd);
	RECT rect = {0, 0};
	DrawText(canvas, start_str, -1, &rect, DT_SINGLELINE|DT_CALCRECT);
	int start_btn_width = rect.right+16+8;

	_taskbar_pos = start_btn_width + 6;

	new PictureButton(Button(_hwnd, start_str, 2, 2, start_btn_width, DESKTOPBARBAR_HEIGHT-8, IDC_START, WS_VISIBLE|WS_CHILD|BS_OWNERDRAW),
						SmallIcon(IDI_STARTMENU)/*, GetStockBrush(WHITE_BRUSH)*/);

	 // create task bar
	_hwndTaskBar = TaskBar::Create(_hwnd);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOTRAYITEMSDISPLAY))
#endif
		 // create tray notification area
		_hwndNotify = NotifyArea::Create(_hwnd);

	_hwndQuickLaunch = QuickLaunchBar::Create(_hwnd);

	 // create rebar window to manage task and quick launch bar
#ifndef _NO_REBAR
	_hwndrebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
					WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
					RBS_VARHEIGHT|RBS_AUTOSIZE|RBS_DBLCLKTOGGLE|	//|RBS_REGISTERDROP
					CCS_NODIVIDER|CCS_NOPARENTALIGN,
					0, 0, 0, 0, _hwnd, 0, g_Globals._hInstance, 0);

	REBARBANDINFO rbBand;

	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask  = RBBIM_TEXT|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
#ifndef RBBS_HIDETITLE // missing in MinGW headers as of 25.02.2004
#define RBBS_HIDETITLE	0x400
#endif
	rbBand.fStyle = RBBS_CHILDEDGE|RBBS_GRIPPERALWAYS|RBBS_HIDETITLE;

	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = 0;
	rbBand.cyChild = 0;
	rbBand.cyMaxChild = 0;
	rbBand.cyIntegral = DESKTOPBARBAR_HEIGHT;	//@@ OK?

	rbBand.lpText = TEXT("Quicklaunch");
	rbBand.hwndChild = _hwndQuickLaunch;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = HIWORD(SendMessage(_hwndQuickLaunch, TB_GETBUTTONSIZE, 0, 0)) + 2;
	rbBand.cx = 250;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	rbBand.lpText = TEXT("Taskbar");
	rbBand.hwndChild = _hwndTaskBar;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = ClientRect(_hwndTaskBar).bottom + 2;
	rbBand.cx = 200;	//pcs->cx-_taskbar_pos-quicklaunch_width-(notifyarea_width+1);
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
#endif

	RegisterHotkeys();

	 // notify all top level windows about the successfully created desktop bar
	PostMessage(HWND_BROADCAST, WM_TASKBARCREATED, 0, 0);

	 // prepare Startmenu, but hide it for now
	_startMenuRoot = GET_WINDOW(StartMenuRoot, StartMenuRoot::Create(_hwnd));

	return 0;
}


void DesktopBar::RegisterHotkeys()
{
	 // register hotkey WIN+E opening explorer
	RegisterHotKey(_hwnd, 0, MOD_WIN, 'E');

		///@todo register all common hotkeys
}

void DesktopBar::ProcessHotKey(int id_hotkey)
{
	switch(id_hotkey) {
	  case 0:	explorer_show_frame(SW_SHOWNORMAL);	break;
		///@todo implement all common hotkeys
	}
}


LRESULT DesktopBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);

		if (res>=HTSIZEFIRST && res<=HTSIZELAST) {
#ifdef TASKBAR_AT_TOP
			if (res == HTBOTTOM)	// enable vertical resizing at the lower border
#else
			if (res == HTTOP)		// enable vertical resizing at the upper border
#endif
				return res;
			else
				return HTCLIENT;	// disable any other resizing
		}
		return res;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE) {
#ifdef TASKBAR_AT_TOP
			if (wparam == SC_SIZE+6)// enable vertical resizing at the lower border
#else
			if (wparam == SC_SIZE+3)// enable vertical resizing at the upper border
#endif
				goto def;
			else
				return 0;			// disable any other resizing
		} else if (wparam == SC_TASKLIST)
			ShowStartMenu();
		goto def;

	  case WM_SIZE:
		Resize(LOWORD(lparam), HIWORD(lparam));
		break;

	  case PM_RESIZE_CHILDREN: {
		ClientRect size(_hwnd);
		Resize(size.right, size.bottom);
		break;}

	  case WM_CLOSE:
		ShowExitWindowsDialog(_hwnd);
		break;

	  case WM_HOTKEY:
		ProcessHotKey(wparam);
		break;

	  case WM_COPYDATA:
		return ProcessCopyData((COPYDATASTRUCT*)lparam);

	  case WM_CONTEXTMENU: {
		PopupMenu menu(IDM_DESKTOPBAR);
		SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
		menu.TrackPopupMenu(_hwnd, MAKEPOINTS(lparam));
		break;}

	  case PM_GET_LAST_ACTIVE:
		if (_hwndTaskBar)
			return SendMessage(_hwndTaskBar, nmsg, wparam, lparam);
		break;

	  case WM_TIMER:
		if (wparam == ID_TRAY_VOLUME) {
			KillTimer(_hwnd, wparam);
			launch_file(_hwnd, TEXT("sndvol32.exe"), SW_SHOWNORMAL, TEXT("-t"));	// launch volume control in small mode
		}
		break;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


void DesktopBar::Resize(int cx, int cy)
{
	///@todo general children resizing algorithm
	int quicklaunch_width = SendMessage(_hwndQuickLaunch, PM_GET_WIDTH, 0, 0);
	int notifyarea_width = SendMessage(_hwndNotify, PM_GET_WIDTH, 0, 0);

	HDWP hdwp = BeginDeferWindowPos(3);

	if (_hwndrebar)
		DeferWindowPos(hdwp, _hwndrebar, 0, _taskbar_pos, 0, cx-_taskbar_pos-(notifyarea_width+1), cy, SWP_NOZORDER|SWP_NOACTIVATE);
	else {
		if (_hwndQuickLaunch)
			DeferWindowPos(hdwp, _hwndQuickLaunch, 0, _taskbar_pos, 1, quicklaunch_width, cy-2, SWP_NOZORDER|SWP_NOACTIVATE);

		if (_hwndTaskBar)
			DeferWindowPos(hdwp, _hwndTaskBar, 0, _taskbar_pos+quicklaunch_width, 0, cx-_taskbar_pos-quicklaunch_width-(notifyarea_width+1), cy, SWP_NOZORDER|SWP_NOACTIVATE);
	}

	if (_hwndNotify)
		DeferWindowPos(hdwp, _hwndNotify, 0, cx-(notifyarea_width+1), 1, notifyarea_width, cy-2, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);

	WindowRect rect(_hwnd);
	RECT work_area = {0, 0, GetSystemMetrics(SM_CXSCREEN), rect.top};
	SystemParametersInfo(SPI_SETWORKAREA, 0, &work_area, 0);	// don't use SPIF_SENDCHANGE because then we have to wait for any message being delivered
	PostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETWORKAREA, 0);
}


int DesktopBar::Command(int id, int code)
{
	switch(id) {
	  case IDC_START:	///@todo startmenu should popup for WM_LBUTTONDOWN, not for WM_COMMAND
		ShowStartMenu();
		break;

	  case ID_ABOUT_EXPLORER:
		explorer_about(_hwnd);
		break;

	  case ID_DESKTOPBAR_SETTINGS:
		ExplorerPropertySheet(_hwnd);
		break;

	  case ID_MINIMIZE_ALL:
		g_Globals._desktops.ToggleMinimize();
		break;

	  case ID_EXPLORE:
		explorer_show_frame(SW_SHOWNORMAL);
		break;

	  case ID_TASKMGR:
		launch_file(_hwnd, TEXT("taskmgr.exe"), SW_SHOWNORMAL);
		break;

	  case ID_SWITCH_DESKTOP_1:
	  case ID_SWITCH_DESKTOP_1+1:
	  case ID_SWITCH_DESKTOP_1+2:
	  case ID_SWITCH_DESKTOP_1+3: {
		int desktop_idx = id - ID_SWITCH_DESKTOP_1;

		g_Globals._desktops.SwitchToDesktop(desktop_idx);

 		if (_hwndQuickLaunch)
			PostMessage(_hwndQuickLaunch, PM_UPDATE_DESKTOP, desktop_idx, 0);
		break;}

	  case ID_TRAY_VOLUME:
		launch_file(_hwnd, TEXT("sndvol32.exe"), SW_SHOWNORMAL);	// launch volume control application
		break;

	  case ID_VOLUME_PROPERTIES:
		launch_cpanel(_hwnd, TEXT("mmsys.cpl"));
		break;

	  default:
		if (_hwndQuickLaunch)
			return SendMessage(_hwndQuickLaunch, WM_COMMAND, MAKEWPARAM(id,code), 0);
		else
			return 1;
	}

	return 0;
}


void DesktopBar::ShowStartMenu()
{
	if (_startMenuRoot)
		_startMenuRoot->TrackStartmenu();
}


 /// copy data structure for tray notifications
struct TrayNotifyCDS {
	DWORD	cookie;
	DWORD	notify_code;
	NOTIFYICONDATA nicon_data;
};

LRESULT DesktopBar::ProcessCopyData(COPYDATASTRUCT* pcd)
{
	 // Is this a tray notification message?
	if (pcd->dwData == 1) {
		TrayNotifyCDS* ptr = (TrayNotifyCDS*) pcd->lpData;

		NotifyArea* notify_area = GET_WINDOW(NotifyArea, _hwndNotify);

		if (notify_area)
			return notify_area->ProcessTrayNotification(ptr->notify_code, &ptr->nicon_data);
	}

	return FALSE;
}


void DesktopBar::AddTrayIcons()
{
	_trayIcon.Add(SmallIcon(IDI_SPEAKER), ResString(IDS_VOLUME));
}

void DesktopBar::TrayClick(UINT id, int btn)
{
	switch(id) {
	  case ID_TRAY_VOLUME:
		if (btn == TRAYBUTTON_LEFT)
			SetTimer(_hwnd, ID_TRAY_VOLUME, 500, NULL);	// wait a bit to correctly handle double clicks
		else {
			PopupMenu menu(IDM_VOLUME);
			SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
			menu.TrackPopupMenuAtPos(_hwnd, GetMessagePos());
		}
		break;
	}
}

void DesktopBar::TrayDblClick(UINT id, int btn)
{
	switch(id) {
	  case ID_TRAY_VOLUME:
		KillTimer(_hwnd, ID_TRAY_VOLUME);	// finish one-click timer
		launch_file(_hwnd, TEXT("sndvol32.exe"), SW_SHOWNORMAL);	// launch volume control application
		break;
	}
}
