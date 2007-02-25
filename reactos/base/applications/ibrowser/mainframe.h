/*
 * Copyright 2005 Martin Fuchs
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
 // ROS Internet Web Browser
 //
 // mainframe.h
 //
 // Martin Fuchs, 25.01.2005
 //


#define	PM_OPEN_WINDOW			(WM_APP+0x07)


 /// Explorer frame window base class
struct MainFrameBase : public PreTranslateWindow
{
	typedef PreTranslateWindow super;

	MainFrameBase(HWND hwnd);
	~MainFrameBase();

	static HWND Create(LPCTSTR url, UINT cmdshow=SW_SHOWNORMAL);

	WindowHandle _hwndrebar;

	WindowHandle _htoolbar;
	WindowHandle _hstatusbar;

	WindowHandle _haddressedit;

	WindowHandle _hsidebar;
	HIMAGELIST	_himl;

	HMENU	_hMenuFrame;
	HMENU	_hMenuWindow;

	MenuInfo _menu_info;

protected:
	FullScreenParameters _fullscreen;

	HACCEL	_hAccel;

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
	virtual bool go_to(LPCTSTR url, bool new_window) {return false;}
};


struct MainFrame : public MainFrameBase
{
	typedef MainFrameBase super;

	MainFrame(HWND hwnd);

	static HWND Create();
	//@@static HWND Create(LPCTSTR url);

protected:
	WindowHandle _left_hwnd;
	WindowHandle _right_hwnd;

	int 	_split_pos;
	int		_last_split;
	RECT	_clnt_rect;

	String	_url;

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	resize_frame(int cx, int cy);
	void	resize_children();
	void	update_clnt_rect();

	void	set_url(LPCTSTR url);

	virtual bool go_to(LPCTSTR url, bool new_window);
};


struct WebChildWndInfo : public ChildWndInfo
{
	WebChildWndInfo(HWND hwndFrame, LPCTSTR url)
	 :	ChildWndInfo(hwndFrame),
		_url(url)
	{
	}

	String	_url;
};

