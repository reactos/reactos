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
 // traynotify.h
 //
 // Martin Fuchs, 22.08.2003
 //


#define	CLASSNAME_TRAYNOTIFY	TEXT("TrayNotifyWnd")
#define	TITLE_TRAYNOTIFY		TEXT("")

#define	CLASSNAME_CLOCKWINDOW	TEXT("TrayClockWClass")

#define	NOTIFYAREA_WIDTH_DEF	100
#define	NOTIFYICON_DIST			20
#define	NOTIFYAREA_SPACE		10

#define	PM_GETMODULEPATH_CB		(WM_APP+0x21)

#define	ICON_AUTOHIDE_SECONDS	300


 /// NotifyIconIndex is used for maintaining the order of notification icons.
struct NotifyIconIndex
{
	NotifyIconIndex(NOTIFYICONDATA* pnid);

	 // sort operator
	friend bool operator<(const NotifyIconIndex& a, const NotifyIconIndex& b)
		{return a._hWnd<b._hWnd || (a._hWnd==b._hWnd && a._uID<b._uID);}

	HWND	_hWnd;
	UINT	_uID;

protected:
	NotifyIconIndex();
};


enum NOTIFYICONMODE {
	NIM_AUTO, NIM_SHOW, NIM_HIDE
};

 /// properties used to identify a notification icon
struct NotifyIconProps
{
	String	_tipText;
	String	_windowTitle;	// To look at the window title and at the window module path of the notify icon owner window
	String	_modulePath;	// to identify notification icons is an extension above XP's behaviour.
};							// (XP seems to store icon image data in the registry instead.)

 /// configuration for the display mode of a notification icon
struct NotifyIconConfig : public NotifyIconProps
{
	NotifyIconConfig() : _mode(NIM_AUTO) {}

	bool	match(const NotifyIconProps& props) const;

	NOTIFYICONMODE	_mode;
};

 /// list of NotifyIconConfig structures
typedef list<NotifyIconConfig> NotifyIconCfgList;


 /// structure for maintaining informations about one notification icon
struct NotifyInfo : public NotifyIconIndex
{
	NotifyInfo();

	 // sort operator
	friend bool operator<(const NotifyInfo& a, const NotifyInfo& b)
		{return a._idx < b._idx;}

	NotifyInfo& operator=(NOTIFYICONDATA* pnid);

	int		_idx;	// display index
	HICON	_hIcon;
	DWORD	_dwState;
	UINT	_uCallbackMessage;
	UINT	_version;
	String	_tipText;

	NOTIFYICONMODE _mode;
	DWORD	_lastChange;	// timer tick value of the last change
};

typedef map<NotifyIconIndex, NotifyInfo> NotifyIconMap;
typedef set<NotifyInfo> NotifyIconSet;


struct NotifyHook
{
	NotifyHook();
	~NotifyHook();

	void	GetModulePath(HWND hwnd, HWND hwndCallback);
	bool	ModulePathCopyData(LPARAM lparam, HWND* phwnd, String& path);

protected:
	const UINT WM_GETMODULEPATH;
};


 /// tray notification area aka "tray"
struct NotifyArea : public Window
{
	typedef Window super;

	NotifyArea(HWND hwnd);
	~NotifyArea();

	static HWND Create(HWND hwndParent);

	LRESULT	ProcessTrayNotification(int notify_code, NOTIFYICONDATA* pnid);

protected:
	WindowHandle _hwndClock;
	int		_clock_width;

	NotifyIconMap _icon_map;
	NotifyIconSet _sorted_icons;
	int		_next_idx;
	size_t	_last_icon_count;

	ToolTip	_tooltip;

	bool	_show_hidden;

	LRESULT Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Notify(int id, NMHDR* pnmh);

	void	Refresh();
	void	Paint();
	void	TimerTick();
	void	CancelModes();

	NotifyIconSet::iterator IconHitTest(const POINT& pos);
	bool	DetermineHideState(NotifyInfo& entry);

	NotifyIconCfgList _cfg;

	NotifyHook _hook;
	map<HWND, String> _window_modules;
};


 /// window for displaying the time in the tray notification area
struct ClockWindow : public Window
{
	typedef Window super;

	ClockWindow(HWND hwnd);

	static HWND Create(HWND hwndParent);

	void	TimerTick();

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Notify(int id, NMHDR* pnmh);

	bool	FormatTime();
	void	Paint();

	TCHAR	_time[16];
	ToolTip	_tooltip;
};
