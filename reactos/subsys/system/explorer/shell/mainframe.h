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
 // mainframe.h
 //
 // Martin Fuchs, 23.07.2003
 //


#define	PM_OPEN_WINDOW			(WM_APP+0x07)
enum OPEN_WINDOW_MODE {OWM_EXPLORE=1, OWM_DETAILS=2, OWM_PIDL=4};


 /// Explorer frame window
struct MainFrame : public PreTranslateWindow
{
	typedef PreTranslateWindow super;

	MainFrame(HWND hwnd);
	~MainFrame();

	static HWND Create();
	static HWND Create(LPCTSTR path, int mode=OWM_EXPLORE|OWM_DETAILS);
	static HWND Create(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);
	static int OpenShellFolders(LPIDA pida, HWND hFrameWnd);

	ChildWindow* CreateChild(LPCTSTR path=NULL, int mode=OWM_EXPLORE|OWM_DETAILS);
	ChildWindow* CreateChild(LPCITEMIDLIST pidl, int mode=OWM_EXPLORE|OWM_DETAILS|OWM_PIDL);

protected:
	FullScreenParameters _fullscreen;

#ifndef _NO_MDI
	HWND	_hmdiclient;
#endif

	WindowHandle _hstatusbar;
	WindowHandle _hwndrebar;
	WindowHandle _htoolbar;
	WindowHandle _hextrabar;
	WindowHandle _hdrivebar;
	WindowHandle _haddressedit;
	WindowHandle _hcommandedit;
	WindowHandle _hsidebar;

	HIMAGELIST	_himl;

	HMENU	_hMenuFrame;
	HMENU	_hMenuWindow;

	MenuInfo _menu_info;

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);
	int		Notify(int id, NMHDR* pnmh);

	void	toggle_child(HWND hwnd, UINT cmd, HWND hchild);
	bool	activate_drive_window(LPCTSTR path);
	bool	activate_child_window(LPCTSTR filesys);

	void	resize_frame_rect(PRECT prect);
	void	resize_frame(int cx, int cy);
	void	resize_frame_client();
	void	frame_get_clientspace(PRECT prect);
	BOOL	toggle_fullscreen();
	void	fullscreen_move();

	void	FillBookmarks();

	HACCEL	_hAccel;
	TCHAR	_drives[BUFFER_LEN];
};
