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
 // Explorer clone, lean version
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

 /// Explorer frame window
struct MainFrame : public ExtContextMenuHandlerT<PreTranslateWindow>
{
	typedef ExtContextMenuHandlerT<PreTranslateWindow> super;

	MainFrame(HWND hwnd);
	~MainFrame();

	static HWND Create();
	static HWND Create(LPCTSTR path, int mode=OWM_EXPLORE|OWM_DETAILS);
	static HWND Create(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);
	static int OpenShellFolders(LPIDA pida, HWND hFrameWnd);

protected:
	WindowHandle _hstatusbar;
	WindowHandle _htoolbar;

	HMENU	_hMenuFrame;
	HMENU	_hMenuWindow;

	MenuInfo _menu_info;

	auto_ptr<ShellBrowserChild> _shellBrowser;

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int 	Notify(int id, NMHDR* pnmh);
	int		Command(int id, int code);

	void	toggle_child(HWND hwnd, UINT cmd, HWND hchild);

	void	resize_frame_rect(PRECT prect);
	void	resize_frame(int cx, int cy);
	void	resize_frame_client();
	void	frame_get_clientspace(PRECT prect);

	HACCEL	_hAccel;


	 // SDI integration

	ShellPathInfo _create_info;

	WindowHandle _left_hwnd;
	WindowHandle _right_hwnd;
	//@@int 	_focus_pane;		// 0: left	1: right

	void	update_explorer_view();
	void	jump_to(LPCTSTR path, int mode);
	void	jump_to(LPCITEMIDLIST path, int mode);
};


 /// The "Execute..."-dialog lets the user enter a command line to launch.
struct ExecuteDialog {	///@todo use class Dialog
	TCHAR	cmd[MAX_PATH];
	int		cmdshow;

	static BOOL CALLBACK WndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
};
