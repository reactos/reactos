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


#define	CLASSNAME_TASKBAR		_T("MSTaskSwWClass")
#define	TITLE_TASKBAR			_T("Running Applications")

#define	IDC_FIRST_APP	0x2000

#define	STARTBUTTON_WIDTH		60
#define	TASKBAR_LEFT			70
//#define TASKBAR_AT_TOP


#define	xxx CSIDL_APPDATA _T("Microsoft\\Internet Explorer\\Quick Launch")


#define	IDW_TASKTOOLBAR	100

 /// internal task bar button management entry
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

 /// map for managing the task bar buttons
struct TaskBarMap : public map<HWND, TaskBarEntry>
{
	~TaskBarMap();

	iterator find_id(int id);
};

struct DesktopBar;

 /// Taskbar window
struct TaskBar : public Window
{
	typedef Window super;

	TaskBar(HWND hwnd);
	~TaskBar();

	static HWND Create(HWND hwndParent);

	DesktopBar*	_desktop_bar;	// may be not necessary

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
