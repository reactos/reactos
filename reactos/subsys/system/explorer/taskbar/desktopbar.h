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
 // desktopbar.h
 //
 // Martin Fuchs, 22.08.2003
 //


#define	CLASSNAME_EXPLORERBAR	TEXT("Shell_TrayWnd")
#define	TITLE_EXPLORERBAR		TEXT("")	// use an empty window title, so windows taskmanager does not show the window in its application list


#define	WINMSG_TASKBARCREATED	TEXT("TaskbarCreated")


#define	DESKTOPBARBAR_HEIGHT	29


#define	IDC_START				0x1000
#define	IDC_LOGOFF				0x1001
#define	IDC_SHUTDOWN			0x1002
#define	IDC_LAUNCH				0x1003
#define	IDC_START_HELP			0x1004
#define	IDC_SEARCH_FILES		0x1005
#define	IDC_SEARCH_COMPUTER		0x1006
#define	IDC_SETTINGS			0x1007
#define	IDC_ADMIN				0x1008
#define	IDC_DOCUMENTS			0x1009
#define	IDC_RECENT				0x100A
#define	IDC_FAVORITES			0x100B
#define	IDC_PROGRAMS			0x100C
#define	IDC_EXPLORE				0x100D
#define	IDC_NETWORK				0x100E
#define	IDC_CONNECTIONS			0x100F
#define	IDC_DRIVES				0x1010
#define	IDC_SETTINGS_MENU		0x1011
#define	IDC_CONTROL_PANEL		0x1012
#define	IDC_PRINTERS			0x1013
#define	IDC_BROWSE				0x1014
#define	IDC_SEARCH_PROGRAM		0x1015
#define	IDC_SEARCH				0x1016

#define	IDC_FIRST_MENU			0x3000


 /// desktop bar window, also known as "system tray"
struct DesktopBar : public OwnerDrawParent<Window>
{
	typedef OwnerDrawParent<Window> super;

	DesktopBar(HWND hwnd);
	~DesktopBar();

	static HWND Create();

protected:
	CommonControlInit _usingCmnCtrl;

	int		WM_TASKBARCREATED;
	RECT	_work_area_org;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	Resize(int cx, int cy);
	void	RegisterHotkeys();
	void	ProcessHotKey(int id_hotkey);
	void	ShowStartMenu();
	LRESULT	ProcessCopyData(COPYDATASTRUCT* pcd);

	WindowHandle _hwndTaskBar;
	WindowHandle _hwndNotify;
	WindowHandle _hwndQuickLaunch;

	struct StartMenuRoot* _startMenuRoot;

	void	DoPropertySheet();
};


 /// "Desktopbar Settings" Property Sheet Dialog
struct DesktopSettingsDlg : public PropSheetPageDlg
{
	typedef PropSheetPageDlg super;

	DesktopSettingsDlg(HWND hwnd);

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void	Paint();
};


 /// "Desktopbar Settings" Property Sheet Dialog
struct TaskbarSettingsDlg : public PropSheetPageDlg
{
	typedef PropSheetPageDlg super;

	TaskbarSettingsDlg(HWND hwnd);

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


 /// "Startmenu Settings" Property Sheet Dialog
struct StartmenuSettingsDlg : public PropSheetPageDlg
{
	typedef PropSheetPageDlg super;

	StartmenuSettingsDlg(HWND hwnd);

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};
