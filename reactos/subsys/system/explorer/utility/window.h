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
 // window.h
 //
 // Martin Fuchs, 23.07.2003
 //


struct Window
{
	Window(HWND hwnd)
	 :	_hwnd(hwnd)
	{
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)this);
	}

	virtual ~Window()
	{
		SetWindowLong(_hwnd, GWL_USERDATA, 0);
	}


	operator HWND() const {return _hwnd;}


	typedef Window* (*WINDOWCREATORFUNC)(HWND, const void*);

	static HWND Create(WINDOWCREATORFUNC creator,
				DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0, LPVOID lpParam=0);

	static HWND Create(WINDOWCREATORFUNC creator, const void* info,
				DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0, LPVOID lpParam=0);

	static Window* create_mdi_child(HWND hmdiclient, const MDICREATESTRUCT& mcs, WINDOWCREATORFUNC creator, const void* info=NULL);

	static LRESULT CALLBACK WindowWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
	static Window* get_window(HWND hwnd);


protected:
	HWND	_hwnd;


	virtual LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual int		Command(int id, int code);
	virtual int 	Notify(int id, NMHDR* pnmh);


	static const void* s_new_info;	//TODO: protect for multithreaded access
	static WINDOWCREATORFUNC s_window_creator;	//TODO: protect for multithreaded access

	 // MDI child creation
	static HHOOK s_hcbthook;
	static LRESULT CALLBACK CBTHookProc(int code, WPARAM wparam, LPARAM lparam);
};


struct SubclassedWindow : public Window
{
	SubclassedWindow(HWND);

protected:
	WNDPROC	_orgWndProc;
};


template<typename WND_CLASS> struct WindowCreator
{
	static WND_CLASS* window_creator(HWND hwnd)
	{
		return new WND_CLASS(hwnd);
	}
};

#define WINDOW_CREATOR(WND_CLASS) \
	(Window::WINDOWCREATORFUNC) WindowCreator<WND_CLASS>::window_creator


template<typename WND_CLASS, typename INFO_CLASS> struct WindowCreatorInfo
{
	static WND_CLASS* window_creator(HWND hwnd, const void* info)
	{
		return new WND_CLASS(hwnd, *static_cast<const INFO_CLASS*>(info));
	}
};

#define WINDOW_CREATOR_INFO(WND_CLASS, INFO_CLASS) \
	(Window::WINDOWCREATORFUNC) WindowCreatorInfo<WND_CLASS, INFO_CLASS>::window_creator


struct WindowClass : public WNDCLASSEX
{
	WindowClass(LPCTSTR classname, WNDPROC wndproc=Window::WindowWndProc);

	ATOM Register()
	{
		return RegisterClassEx(this);
	}
};


#define	WM_DISPATCH_COMMAND		(WM_APP+0)


#define SPLIT_WIDTH 		5
#define DEFAULT_SPLIT_POS	300
#define	COLOR_SPLITBAR		LTGRAY_BRUSH


struct MenuInfo
{
	HMENU	_hMenuView;
	HMENU	_hMenuOptions;
};

#define	FRM_GET_MENUINFO		(WM_APP+1)

#define	Frame_GetMenuInfo(hwnd) ((MenuInfo*)SNDMSG(hwnd, FRM_GET_MENUINFO, 0, 0))


struct ChildWindow : public Window
{
	typedef Window super;

	ChildWindow(HWND hwnd);

	static ChildWindow* create(HWND hmdiclient, const RECT& rect,
				WINDOWCREATORFUNC creator, LPCTSTR classname, LPCTSTR title=NULL);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	virtual void resize_children(int cx, int cy);

protected:
	MenuInfo*_menu_info;

	HWND	_left_hwnd;
	HWND	_right_hwnd;
	int 	_focus_pane;		// 0: left	1: right

	int 	_split_pos;
	int		_last_split;
};
