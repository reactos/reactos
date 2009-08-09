/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 // mainframe.h
 //
 // Martin Fuchs, 23.07.2003
 //


#define	PM_OPEN_WINDOW			(WM_APP+0x07)

enum OPEN_WINDOW_MODE {
	OWM_EXPLORE=1,	/// window in explore mode
	OWM_ROOTED=2,	/// "rooted" window with special shell namespace root
	OWM_DETAILS=4,	/// view files in detail mode
	OWM_PIDL=8,		/// path is given as PIDL, otherwise as LPCTSTR
	OWM_SEPARATE=16	/// open separate subfolder windows
};


 /// Explorer frame window base class
struct MainFrameBase : public PreTranslateWindow
{
	typedef PreTranslateWindow super;

	MainFrameBase(HWND hwnd);
	~MainFrameBase();

	static HWND Create(const ExplorerCmd& cmd);
	static int OpenShellFolders(LPIDA pida, HWND hFrameWnd);

	WindowHandle _hwndrebar;

	WindowHandle _htoolbar;
	WindowHandle _haddrcombo;
	WindowHandle _hstatusbar;

	WindowHandle _hsidebar;
	HIMAGELIST	_himl;

	HMENU		_hMenuFrame;
	HMENU		_hMenuWindow;

	MenuInfo	_menu_info;

protected:
	FullScreenParameters _fullscreen;

	HACCEL		_hAccel;
	HIMAGELIST	_himl_old;

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	bool	ProcessMessage(UINT nmsg, WPARAM wparam, LPARAM lparam, LRESULT* pres);
	int		Command(int id, int code);
	int		Notify(int id, NMHDR* pnmh);

	virtual BOOL TranslateMsg(MSG* pmsg);

	void	toggle_child(HWND hwnd, UINT cmd, HWND hchild, int band_idx=-1);

	void	resize_frame_client();
	virtual void resize_frame(int cx, int cy);
	virtual void frame_get_clientspace(PRECT prect);

	BOOL	toggle_fullscreen();
	void	fullscreen_move();

	void	FillBookmarks();
	virtual bool go_to(LPCTSTR url, bool new_window);
};


#ifndef _NO_MDI

struct MDIMainFrame : public MainFrameBase
{
	typedef MainFrameBase super;

	MDIMainFrame(HWND hwnd);

	static HWND Create();
	static HWND Create(LPCTSTR path, int mode=OWM_EXPLORE|OWM_DETAILS);
	static HWND Create(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);

	ChildWindow* CreateChild(LPCTSTR path=NULL, int mode=OWM_EXPLORE|OWM_DETAILS);
	ChildWindow* CreateChild(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);

protected:
	HWND	_hmdiclient;

	WindowHandle _hextrabar;
#ifndef _NO_WIN_FS
	WindowHandle _hdrivebar;
#endif

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	virtual BOOL TranslateMsg(MSG* pmsg);

	bool	activate_drive_window(LPCTSTR path);
	bool	activate_child_window(LPCTSTR filesys);

	virtual void resize_frame(int cx, int cy);
	virtual void frame_get_clientspace(PRECT prect);

	virtual bool go_to(LPCTSTR url, bool new_window);

#ifndef _NO_WIN_FS
	TCHAR	_drives[BUFFER_LEN];
#endif
};

#endif


struct SDIMainFrame : public ExtContextMenuHandlerT<
				ShellBrowserChildT<MainFrameBase>
			>
{
	typedef ExtContextMenuHandlerT<
				ShellBrowserChildT<MainFrameBase>
			> super;

	SDIMainFrame(HWND hwnd);

	static HWND Create();
	static HWND Create(LPCTSTR path, int mode=OWM_EXPLORE|OWM_DETAILS);
	static HWND Create(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);

protected:
	ShellPathInfo _shellpath_info;

	WindowHandle _left_hwnd;
	WindowHandle _right_hwnd;

/**@todo focus handling for TAB switching
	int 	_focus_pane;		// 0: left	1: right
*/

	int 	_split_pos;
	int		_last_split;
	RECT	_clnt_rect;

	String	_url;

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	resize_frame(int cx, int cy);
	void	resize_children();
	void	update_clnt_rect();

	void	update_shell_browser();
	void	jump_to(LPCTSTR path, int mode);
	void	jump_to(LPCITEMIDLIST path, int mode);

	 // interface BrowserCallback
	virtual void	entry_selected(Entry* entry);

	void	set_url(LPCTSTR url);
};
