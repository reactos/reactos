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


#include <list>

//#include "shellhook.h"


#define	TASKBAR_HEIGHT			30
#define	STARTBUTTON_WIDTH		60
#define	TASKBAR_LEFT			70
//#define TASKBAR_AT_TOP

#define	STARTMENU_WIDTH			150
#define	STARTMENU_HEIGHT		4
#define	STARTMENU_LINE_HEIGHT	22

#define	WM_SHELLHOOK_NOTIFY		(WM_APP+0x10)


#define	CLASSNAME_EXPLORERBAR	_T("Shell_TrayWnd")
#define	TITLE_EXPLORERBAR		_T("DesktopBar")

#define	CLASSNAME_TASKBAR		_T("MSTaskSwWClass")
#define	TITLE_TASKBAR			_T("Running Applications")

#define	CLASSNAME_STARTMENU		_T("ReactosStartmenuClass")
#define	TITLE_STARTMENU			_T("Start Menu")


struct DesktopBar : public OwnerDrawParent<Window>
{
	typedef OwnerDrawParent<Window> super;

	DesktopBar(HWND hwnd);
	~DesktopBar();

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	create_startmenu();

	HWND	_hwndTaskBar;
	HWND	_hwndStartMenu;
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
	HWND	_htoolbar;
	TaskBarMap _map;
	int		_next_id;
	HWND	_last_foreground_wnd;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam);

	void	Refresh();
};


 // Startmenu button
struct StartMenuButton : public Button
{
	StartMenuButton(HWND parent, int y, LPCTSTR text,
					UINT id, HICON hIcon, DWORD style=WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW, DWORD exStyle=0)
	 :	Button(parent, text, 2, y, STARTMENU_WIDTH-4, STARTMENU_LINE_HEIGHT, id, style, exStyle)
	{
		*new StartmenuEntry(_hwnd, hIcon);

		SetWindowFont(_hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
	}
};


typedef list<ShellPath> StartMenuFolders;
typedef list<ShellDirectory> StartMenuShellDirs;

 // Startmenu window
struct StartMenu : public OwnerDrawParent<Dialog>
{
	typedef OwnerDrawParent<Dialog> super;

	StartMenu(HWND hwnd);
	StartMenu(HWND hwnd, const StartMenuFolders& info);

	static HWND Create(int x, int y, HWND hwndParent=0);
	static HWND Create(int x, int y, const StartMenuFolders&, HWND hwndParent=0);

protected:
	int		_next_id;
	StartMenuShellDirs _dirs;

	static BtnWindowClass s_wcStartMenu;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	AddButton(LPCTSTR text, HICON hIcon=0, UINT id=(UINT)-1);
	void	AddShellEntries(const ShellDirectory& dir, bool subfolders=true);
};


 // Startmenu root window
struct StartMenuRoot : public StartMenu
{
	typedef StartMenu super;

	StartMenuRoot(HWND hwnd);

	static HWND Create(int x, int y, HWND hwndParent=0);

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	int		Command(int id, int code);
};
