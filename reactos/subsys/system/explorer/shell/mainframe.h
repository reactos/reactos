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
 // Explorer clone
 //
 // mainframe.h
 //
 // Martin Fuchs, 23.07.2003
 //


struct MainFrame : public PreTranslateWindow
{
	typedef PreTranslateWindow super;

	MainFrame(HWND hwnd);
	~MainFrame();

	static HWND Create();

protected:
	FullScreenParameters _fullscreen;

#ifndef _NO_MDI
	HWND	_hmdiclient;
#endif

	HWND	_hstatusbar;
	HWND	_htoolbar;
	HWND	_hdrivebar;

	HMENU	_hMenuFrame;
	HMENU	_hMenuWindow;

	MenuInfo _menu_info;

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	void	toggle_child(HWND hwnd, UINT cmd, HWND hchild);
	bool	activate_drive_window(LPCTSTR path);
	bool	activate_fs_window(LPCTSTR filesys);

	void	resize_frame_rect(PRECT prect);
	void	resize_frame(int cx, int cy);
	void	resize_frame_client();
	void	frame_get_clientspace(PRECT prect);
	BOOL	toggle_fullscreen();
	void	fullscreen_move();

	HACCEL	_hAccel;
	TCHAR	_drives[BUFFER_LEN];
};

