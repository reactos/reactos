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


#include <map>
#include <set>


typedef set<HWND> WindowSet;


 /*
	Classes are declared using "struct", not "class" because the default
	access mode is "public". This way we can list the member functions in a
	natural order without explicitly specifying any access mode at the begin
	of the definition.
	First are public constructors and destructor, then public member functions.
	After that we list protected member varibables and functions. If needed,
	private implemenation varibales and functions are positioned at the end.
 */


 /**
	Class Window is the base class for several C++ window wrapper classes.
	Window objects are allocated from the heap. They are automatically freed
	when the window gets destroyed.
 */
struct Window
{
	Window(HWND hwnd)
	 :	_hwnd(hwnd)
	{
		 // store "this" pointer as user data
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)this);
	}

	virtual ~Window()
	{
		 // empty user data field
		SetWindowLong(_hwnd, GWL_USERDATA, 0);
	}


	operator HWND() const {return _hwnd;}


	typedef Window* (*CREATORFUNC)(HWND, const void*);

	static HWND Create(CREATORFUNC creator, DWORD dwExStyle,
				LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0, LPVOID lpParam=0);

	static HWND Create(CREATORFUNC creator, const void* info,
				DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0, LPVOID lpParam=0);

	static Window* create_mdi_child(HWND hmdiclient, const MDICREATESTRUCT& mcs, CREATORFUNC creator, const void* info=NULL);

	static LRESULT CALLBACK WindowWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
	static Window* get_window(HWND hwnd);


	static void register_pretranslate(HWND hwnd);
	static void unregister_pretranslate(HWND hwnd);
	static BOOL	pretranslate_msg(LPMSG pmsg);

	static void	register_dialog(HWND hwnd);
	static void	unregister_dialog(HWND hwnd);
	static BOOL	dispatch_dialog_msg(LPMSG pmsg);

	static int	MessageLoop();


protected:
	HWND	_hwnd;


	virtual LRESULT	Init(LPCREATESTRUCT pcs);							// WM_CREATE processing
	virtual LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual int		Command(int id, int code);							// WM_COMMAND processing
	virtual int		Notify(int id, NMHDR* pnmh);						// WM_NOTIFY processing


	static const void* s_new_info;	//TODO: protect for multithreaded access
	static CREATORFUNC s_window_creator;	//TODO: protect for multithreaded access

	 // MDI child creation
	static HHOOK s_hcbtHook;
	static LRESULT CALLBACK CBTHookProc(int code, WPARAM wparam, LPARAM lparam);

	static WindowSet s_pretranslate_windows;
	static WindowSet s_dialogs;
};


 /**
	SubclassedWindow is used to wrap already existing window handles
	into C++ Window objects. To construct a object, use the "new" operator
	to put it in the heap. It is automatically freed, when the window
	gets destroyed.
 */
struct SubclassedWindow : public Window
{
	typedef Window super;

	SubclassedWindow(HWND);

protected:
	WNDPROC	_orgWndProc;

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
};


template<typename WND_CLASS> struct WindowCreator
{
	static WND_CLASS* window_creator(HWND hwnd)
	{
		return new WND_CLASS(hwnd);
	}
};

#define WINDOW_CREATOR(WND_CLASS) \
	(Window::CREATORFUNC) WindowCreator<WND_CLASS>::window_creator


template<typename WND_CLASS, typename INFO_CLASS> struct WindowCreatorInfo
{
	static WND_CLASS* window_creator(HWND hwnd, const void* info)
	{
		return new WND_CLASS(hwnd, *static_cast<const INFO_CLASS*>(info));
	}
};

#define WINDOW_CREATOR_INFO(WND_CLASS, INFO_CLASS) \
	(Window::CREATORFUNC) WindowCreatorInfo<WND_CLASS, INFO_CLASS>::window_creator


 /**
	WindowClass is a neat wrapper for RegisterClassEx().
	Just construct a WindowClass object, override the attributes you want
	to change, then call Register() or simply request the ATOM value to
	register the window class. You don't have to worry calling Register()
	more than once. It checks if, the class has already been registered.
 */
struct WindowClass : public WNDCLASSEX
{
	WindowClass(LPCTSTR classname, UINT style=0, WNDPROC wndproc=Window::WindowWndProc);

	ATOM Register()
	{
		if (!_atomClass)
			_atomClass = RegisterClassEx(this);

		return _atomClass;
	}

	operator ATOM() {return Register();}

	 // return LPCTSTR for the CreateWindowEx() parameter
	operator LPCTSTR() {return (LPCTSTR)(int)Register();}

protected:
	ATOM	_atomClass;
};

 /// window class with gray background color
struct BtnWindowClass : public WindowClass
{
	BtnWindowClass(LPCTSTR classname, UINT style=0, WNDPROC wndproc=Window::WindowWndProc)
	 :	WindowClass(classname, style, wndproc)
	{
		hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	}
};

 /// window class with specified icon from resources
struct IconWindowClass : public WindowClass
{
	IconWindowClass(LPCTSTR classname, UINT nid, UINT style=0, WNDPROC wndproc=Window::WindowWndProc);
};


#define	WM_DISPATCH_COMMAND		(WM_APP+0x00)
#define	WM_TRANSLATE_MSG		(WM_APP+0x01)


#define SPLIT_WIDTH 		5
#define DEFAULT_SPLIT_POS	300
#define	COLOR_SPLITBAR		LTGRAY_BRUSH


 /// menu info structure for MDI child windows
struct MenuInfo
{
	HMENU	_hMenuView;
	HMENU	_hMenuOptions;
};

#define	FRM_GET_MENUINFO		(WM_APP+0x02)

#define	Frame_GetMenuInfo(hwnd) ((MenuInfo*)SNDMSG(hwnd, FRM_GET_MENUINFO, 0, 0))


 /**
	Class ChildWindow represents MDI child windows.
	It is used with class MainFrame.
 */
struct ChildWindow : public Window
{
	typedef Window super;

	ChildWindow(HWND hwnd);

	static ChildWindow* create(HWND hmdiclient, const RECT& rect,
				CREATORFUNC creator, LPCTSTR classname, LPCTSTR title=NULL);

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


 /**
	PreTranslateWindow is used to register windows to be called by Window::pretranslate_msg().
	This way you get WM_TRANSLATE_MSG messages before the message loop dispatches messages.
	You can then for example use TranslateAccelerator() to implement key shortcuts.
 */
struct PreTranslateWindow : public Window
{
	typedef Window super;

	PreTranslateWindow(HWND);
	~PreTranslateWindow();
};


 /**
	The class Dialog implements modeless dialogs, which are managed by
	Window::dispatch_dialog_msg() in Window::MessageLoop().
	A Dialog object should be constructed by calling Window::Create()
	and specifying the class using the WINDOW_CREATOR() macro.
 */
struct Dialog : public Window
{
	typedef Window super;

	Dialog(HWND);
	~Dialog();
};


 /**
	This class constructs button controls.
	The button will remain existent when the C++ Button object is destroyed.
	There is no conjunction between C++ object and windows control life time.
 */
struct Button
{
	Button(HWND parent, LPCTSTR text, int left, int top, int width, int height,
			int id, DWORD flags=WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, DWORD exStyle=0);

	operator HWND() const {return _hwnd;}

protected:
	HWND	_hwnd;
};


 /**
	This class constructs static controls.
	The control will remain existent when the C++ object is destroyed.
	There is no conjunction between C++ object and windows control life time.
 */
struct Static
{
	Static(HWND parent, LPCTSTR text, int left, int top, int width, int height,
			int id, DWORD flags=WS_VISIBLE|WS_CHILD|SS_SIMPLE, DWORD ex_flags=0);

	operator HWND() const {return _hwnd;}

protected:
	HWND	_hwnd;
};


/*
 // control color message routing for ColorStatic and HyperlinkCtrl

#define	WM_DISPATCH_CTLCOLOR	(WM_APP+0x07)

template<typename BASE> struct CtlColorParent : public BASE
{
	typedef BASE super;

	CtlColorParent(HWND hwnd)
	 : super(hwnd) {}

	LRESULT WndProc(UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch(message) {
		  case WM_CTLCOLOR:
		  case WM_CTLCOLORBTN:
		  case WM_CTLCOLORDLG:
		  case WM_CTLCOLORSCROLLBAR:
		  case WM_CTLCOLORSTATIC: {
			HWND hctl = (HWND) lparam;
			return SendMessage(hctl, WM_DISPATCH_CTLCOLOR, wparam, message);
		  }

		  default:
			return super::WndProc(message, wparam, lparam);
		}
	}
};
*/


 // owner draw message routing for ColorButton and PictureButton 

#define	WM_DISPATCH_DRAWITEM	(WM_APP+0x08)

template<typename BASE> struct OwnerDrawParent : public BASE
{
	typedef BASE super;

	OwnerDrawParent(HWND hwnd)
	 : super(hwnd) {}

	LRESULT WndProc(UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch(message) {
		  case WM_DRAWITEM:
			if (wparam) {	// ein Control?
				HWND hctl = GetDlgItem(_hwnd, wparam);

				if (hctl)
					return SendMessage(hctl, WM_DISPATCH_DRAWITEM, wparam, lparam);
			} /*else	// oder ein Menüeintrag?
				; */

			return 0;

		  default:
			return super::WndProc(message, wparam, lparam);
		}
	}
};


 /**
	Subclass button controls to draw them by using WM_DISPATCH_DRAWITEM
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
struct OwnerdrawnButton : public SubclassedWindow
{
	typedef SubclassedWindow super;

	OwnerdrawnButton(HWND hwnd)
	 : super(hwnd) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void	DrawGrayText(LPDRAWITEMSTRUCT dis, LPRECT pRect, LPCTSTR text, int dt_flags);

	virtual void DrawItem(LPDRAWITEMSTRUCT dis) = 0;
};


 /**
	Subclass button controls to paint colored text labels.
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
/* not yet used
struct ColorButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	ColorButton(HWND hwnd, COLORREF textColor)
	 : super(hwnd), _textColor(textColor) {}

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	COLORREF _textColor;
};
*/


 /**
	Subclass button controls to paint pictures left to the labels.
	The buttons should have set the style bit BS_OWNERDRAW.
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
struct PictureButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	PictureButton(HWND hwnd, HICON hIcon, bool flat=false)
	 :	super(hwnd), _hIcon(hIcon), _flat(flat) {}

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	HICON	_hIcon;
	bool	_flat;
};


 /**
	implement start menu button as owner drawn buitton controls
 */
struct StartmenuEntry : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	StartmenuEntry(HWND hwnd, HICON hIcon, bool showArrow)
	 :	super(hwnd), _hIcon(hIcon), _showArrow(showArrow) {}

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	HICON	_hIcon;
	bool	_showArrow;
};
