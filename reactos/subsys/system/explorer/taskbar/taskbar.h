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
 // Explorer and Desktop clone
 //
 // taskbar.h
 //
 // Martin Fuchs, 16.08.2003
 //


//#include "shellhook.h"


#define	TASKBAR_HEIGHT			30
#define	STARTBUTTON_WIDTH		60
#define	TASKBAR_LEFT			70
//#define TASKBAR_AT_TOP


#define	CLASSNAME_EXPLORERBAR	_T("Shell_TrayWnd")
#define	TITLE_EXPLORERBAR		_T("DesktopBar")

#define	CLASSNAME_TASKBAR		_T("MSTaskSwWClass")
#define	TITLE_TASKBAR			_T("Running Applications")


#define	WM_SHELLHOOK_NOTIFY		(WM_APP+0x10)


#define	IDC_START		0x1000
#define	IDC_LOGOFF		0x1001
#define	IDC_SHUTDOWN	0x1002
#define	IDC_LAUNCH		0x1003
#define	IDC_START_HELP	0x1004
#define	IDC_SEARCH		0x1005
#define	IDC_SETTINGS	0x1006
#define	IDC_ADMIN		0x1007
#define	IDC_DOCUMENTS	0x1008
#define	IDC_RECENT		0x1009
#define	IDC_FAVORITES	0x100A
#define	IDC_PROGRAMS	0x100B
#define	IDC_EXPLORE		0x100C
#define	IDC_NETWORK		0x100D
#define	IDC_CONNECTIONS	0x100E

#define	IDC_FIRST_APP	0x2000
#define	IDC_FIRST_MENU	0x3000


struct DesktopBar : public OwnerDrawParent<Window>
{
	typedef OwnerDrawParent<Window> super;

	DesktopBar(HWND hwnd);
	~DesktopBar();

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	RegisterHotkeys();
	void	ProcessHotKey(int id_hotkey);
	void	ToggleStartmenu();

	WindowHandle _hwndTaskBar;
	WindowHandle _startMenuRoot;
};


#define	IDW_TASKTOOLBAR	100

 // internal task bar button management entry
struct TaskBarEntry
{	
	TaskBarEntry();

	int		_id;	// ID for WM_COMMAND
	HBITMAP	_hbmp;
	int		_bmp_idx;
	int		_used;
	int		_btn_idx;
	String	_title;
	BYTE	_fsState;
};

 // map for managing the task bar buttons
struct TaskBarMap : public map<HWND, TaskBarEntry>
{
	~TaskBarMap();

	iterator find_id(int id);
};

 // Taskbar window
struct TaskBar : public Window
{
	typedef Window super;

	TaskBar(HWND hwnd);
	~TaskBar();

	DesktopBar*	_desktop_bar;

protected:
	WindowHandle _htoolbar;
	TaskBarMap	_map;
	int			_next_id;
	WindowHandle _last_foreground_wnd;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam);

	void	Refresh();
};
