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
 // window.h
 //
 // Martin Fuchs, 23.07.2003
 //


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


 /// information structure for creation of a MDI child window
struct ChildWndInfo
{
	ChildWndInfo(HWND hmdiclient)
	 :	_hmdiclient(hmdiclient) {}

	HWND	_hmdiclient;
};


 /**
	Class Window is the base class for several C++ window wrapper classes.
	Window objects are allocated from the heap. They are automatically freed
	when the window gets destroyed.
 */
struct Window : public WindowHandle
{
	Window(HWND hwnd);
	virtual ~Window();


	typedef map<HWND,Window*> WindowMap;

	typedef Window* (*CREATORFUNC)(HWND);
	typedef Window* (*CREATORFUNC_INFO)(HWND, const void*);

	static HWND Create(CREATORFUNC creator, DWORD dwExStyle,
				LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0/*, LPVOID lpParam=0*/);

	static HWND Create(CREATORFUNC_INFO creator, const void* info,
				DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
				DWORD dwStyle, int x, int y, int w, int h,
				HWND hwndParent=0, HMENU hMenu=0/*, LPVOID lpParam=0*/);

	static Window* create_mdi_child(const ChildWndInfo& info, const MDICREATESTRUCT& mcs, CREATORFUNC_INFO creator);
//	static Window* create_property_sheet(struct PropertySheetDialog* ppsd, CREATORFUNC creator, const void* info);

	static LRESULT CALLBACK WindowWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);

	static Window* get_window(HWND hwnd);
#ifndef _MSC_VER
	template<typename CLASS> static CLASS* get_window(HWND hwnd) {return static_cast<CLASS*>(get_window(hwnd));}
#define	GET_WINDOW(CLASS, hwnd) Window::get_window<CLASS>(hwnd)
#endif

	static void register_pretranslate(HWND hwnd);
	static void unregister_pretranslate(HWND hwnd);
	static BOOL	pretranslate_msg(LPMSG pmsg);

	static void	register_dialog(HWND hwnd);
	static void	unregister_dialog(HWND hwnd);
	static BOOL	dispatch_dialog_msg(LPMSG pmsg);

	static int	MessageLoop();


	LRESULT	SendParent(UINT nmsg, WPARAM wparam=0, LPARAM lparam=0);
	LRESULT	PostParent(UINT nmsg, WPARAM wparam=0, LPARAM lparam=0);

	static void CancelModes(HWND hwnd=0);


protected:
	virtual LRESULT	Init(LPCREATESTRUCT pcs);							// WM_CREATE processing
	virtual LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual int		Command(int id, int code);							// WM_COMMAND processing
	virtual int		Notify(int id, NMHDR* pnmh);						// WM_NOTIFY processing

	static Window* create_controller(HWND hwnd);


	static WindowMap	s_wnd_map;

	static const void*	s_new_info;
	static CREATORFUNC	s_window_creator;


	 /// structure for managing critical sections as static class information in struct Window
	struct StaticWindowData {
		CritSect	_map_crit_sect;
		CritSect	_create_crit_sect;
	};

	static StaticWindowData& GetStaticWindowData();


	 // MDI child creation
	static HHOOK s_hcbtHook;
	static LRESULT CALLBACK MDICBTHookProc(int code, WPARAM wparam, LPARAM lparam);
	static LRESULT CALLBACK PropSheetCBTHookProc(int code, WPARAM wparam, LPARAM lparam);

	static WindowSet s_pretranslate_windows;
	static WindowSet s_dialogs;
};

#ifdef UNICODE
#define	NFR_CURRENT	NFR_UNICODE
#else
#define	NFR_CURRENT	NFR_ANSI
#endif


#ifdef _MSC_VER
template<typename CLASS> struct GetWindowHelper
{
	static CLASS* get_window(HWND hwnd) {
		return static_cast<CLASS*>(Window::get_window(hwnd));
	}
};
#define	GET_WINDOW(CLASS, hwnd) GetWindowHelper<CLASS>::get_window(hwnd)
#endif


 /// dynamic casting of Window pointers
template<typename CLASS> struct TypeCheck
{
	static CLASS* dyn_cast(Window* wnd)
		{return dynamic_cast<CLASS*>(wnd);}
};

#define	WINDOW_DYNAMIC_CAST(CLASS, hwnd) \
	TypeCheck<CLASS>::dyn_cast(Window::get_window(hwnd))


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

	static LRESULT CALLBACK SubclassedWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);

	virtual LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual int		Command(int id, int code);
	virtual int		Notify(int id, NMHDR* pnmh);
};


 /// template class used in macro WINDOW_CREATOR to define the creater functions for Window objects
template<typename WND_CLASS> struct WindowCreator
{
	static WND_CLASS* window_creator(HWND hwnd)
	{
		return new WND_CLASS(hwnd);
	}
};

#define WINDOW_CREATOR(WND_CLASS) \
	((Window::CREATORFUNC) WindowCreator<WND_CLASS>::window_creator)


 /// template class used in macro WINDOW_CREATOR_INFO to the define creater functions for Window objects with additional creation information
template<typename WND_CLASS, typename INFO_CLASS> struct WindowCreatorInfo
{
	static WND_CLASS* window_creator(HWND hwnd, const void* info)
	{
		return new WND_CLASS(hwnd, *static_cast<const INFO_CLASS*>(info));
	}
};

#define WINDOW_CREATOR_INFO(WND_CLASS, INFO_CLASS) \
	((Window::CREATORFUNC_INFO) WindowCreatorInfo<WND_CLASS, INFO_CLASS>::window_creator)


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


 // private message constants
#define	PM_DISPATCH_COMMAND		(WM_APP+0x00)
#define	PM_TRANSLATE_MSG		(WM_APP+0x01)


#define SPLIT_WIDTH 		5
#define DEFAULT_SPLIT_POS	300
#define	COLOR_SPLITBAR		LTGRAY_BRUSH


 /// menu info structure
struct MenuInfo
{
	HMENU	_hMenuView;
};

#define	PM_FRM_GET_MENUINFO		(WM_APP+0x02)

#define	Frame_GetMenuInfo(hwnd) ((MenuInfo*)SNDMSG(hwnd, PM_FRM_GET_MENUINFO, 0, 0))


 /**
	Class ChildWindow represents MDI child windows.
	It is used with class MainFrame.
 */
struct ChildWindow : public Window
{
	typedef Window super;

	ChildWindow(HWND hwnd, const ChildWndInfo& info);

	static ChildWindow* create(const ChildWndInfo& info, const RECT& rect, CREATORFUNC_INFO creator,
								LPCTSTR classname, LPCTSTR title=NULL, DWORD style=0);

	bool	go_to(LPCTSTR url);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	virtual void resize_children(int cx, int cy);
	virtual String jump_to_int(LPCTSTR url) = 0;

protected:
	MenuInfo*_menu_info;

	WindowHandle _left_hwnd;
	WindowHandle _right_hwnd;
	int 	_focus_pane;		// 0: left	1: right

	int 	_split_pos;
	int		_last_split;

	HWND	_hwndFrame;
	String	_statusText;
	String	_url;

	stack<String> _url_history;

	void	set_url(LPCTSTR url);
};

#define	PM_SETSTATUSTEXT		(WM_APP+0x1E)


 /**
	PreTranslateWindow is used to register windows to be called by Window::pretranslate_msg().
	This way you get PM_TRANSLATE_MSG messages before the message loop dispatches messages.
	You can then for example use TranslateAccelerator() to implement key shortcuts.
 */
struct PreTranslateWindow : public Window
{
	typedef Window super;

	PreTranslateWindow(HWND);
	~PreTranslateWindow();
};


 /**
	The class DialogWindow implements modeless dialogs, which are managed by
	Window::dispatch_dialog_msg() in Window::MessageLoop().
	A DialogWindow object should be constructed by calling Window::Create()
	and specifying the class using the WINDOW_CREATOR() macro.
 */
struct DialogWindow : public Window
{
	typedef Window super;

	DialogWindow(HWND hwnd)
	 :	super(hwnd)
	{
		register_dialog(hwnd);
	}

	~DialogWindow()
	{
		unregister_dialog(_hwnd);
	}
};


 /**
	The class Dialog implements modal dialogs.
	A Dialog object should be constructed by calling Dialog::DoModal()
	and specifying the class using the WINDOW_CREATOR() macro.
 */
struct Dialog : public Window
{
	typedef Window super;

	Dialog(HWND);
	~Dialog();

	static int DoModal(UINT nid, CREATORFUNC creator, HWND hwndParent=0);
	static int DoModal(UINT nid, CREATORFUNC_INFO creator, const void* info, HWND hwndParent=0);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);
};


#define	PM_FRM_CALC_CLIENT		(WM_APP+0x03)
#define	Frame_CalcFrameClient(hwnd, prt) ((BOOL)SNDMSG(hwnd, PM_FRM_CALC_CLIENT, 0, (LPARAM)(PRECT)prt))

#define	PM_JUMP_TO_URL			(WM_APP+0x25)
#define	PM_URL_CHANGED			(WM_APP+0x26)


struct PropSheetPage : public PROPSHEETPAGE
{
	PropSheetPage(UINT nid, Window::CREATORFUNC dlg_creator);

	void	init(struct PropertySheetDialog*);

protected:
	friend struct PropSheetPageDlg;

	Window::CREATORFUNC	_dlg_creator;
};


 /// Property Sheet dialog
struct PropertySheetDialog : public PROPSHEETHEADER
{
	PropertySheetDialog(HWND owner);

	void	add(PropSheetPage& psp);
	int		DoModal(int start_page=0);

	HWND	GetCurrentPage();

protected:
	typedef vector<PROPSHEETPAGE> Vector;
	Vector	_pages;
	HWND	_hwnd;
};


 /// Property Sheet Page (inner dialog)
struct PropSheetPageDlg : public Dialog
{
	typedef Dialog super;

	PropSheetPageDlg(HWND);

protected:
	friend struct PropertySheetDialog;
	friend struct PropSheetPage;

	static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);

	int	Command(int id, int code);
};


/*
 /// Property Sheet Dialog (outer dialog)
struct PropertySheetDlg : public SubclassedWindow
{
	typedef SubclassedWindow super;

	PropertySheetDlg(HWND hwnd) : super(hwnd) {}
};
*/


 // Layouting of resizable windows

 /// Flags to specify how to move and resize controls when resizing their parent window
enum RESIZE_FLAGS {
	MOVE_LEFT	= 0x1,
	MOVE_RIGHT	= 0x2,
	MOVE_TOP	= 0x4,
	MOVE_BOTTOM	= 0x8,

	MOVE_X	=  MOVE_LEFT | MOVE_RIGHT,
	MOVE_Y	=  MOVE_TOP | MOVE_BOTTOM,
	RESIZE_X=  MOVE_RIGHT,
	RESIZE_Y=  MOVE_BOTTOM,

	MOVE	= MOVE_X   | MOVE_Y,
	RESIZE	= RESIZE_X | RESIZE_Y
};

 /// structure to assign RESIZE_FLAGS to dialogs control
struct ResizeEntry
{
	ResizeEntry(UINT id, int flags)
	 : _id(id), _flags(flags) {}

	ResizeEntry(HWND hwnd, int flags)
	 : _id(GetDlgCtrlID(hwnd)), _flags(flags) {}

	UINT	_id;
	int		_flags;
};


 /// Management of controls in resizable dialogs
struct ResizeManager : public std::list<ResizeEntry>
{
	typedef std::list<ResizeEntry> super;

	ResizeManager(HWND hwnd);

	void Add(UINT id, int flags)
		{push_back(ResizeEntry(id, flags));}

	void Add(HWND hwnd, int flags)
		{push_back(ResizeEntry(hwnd, flags));}

	void HandleSize(int cx, int cy);
	void Resize(int dx, int dy);

	void SetMinMaxInfo(LPMINMAXINFO lpmmi)
	{
		lpmmi->ptMinTrackSize.x = _min_wnd_size.cx;
		lpmmi->ptMinTrackSize.y = _min_wnd_size.cy;
	}

	SIZE	_min_wnd_size;

protected:
	HWND	_hwnd;
	SIZE	_last_size;
};


 /// Controller base template class for resizable dialogs
template<typename BASE> struct ResizeController : public BASE
{
	typedef BASE super;

	ResizeController(HWND hwnd)
	 :	super(hwnd),
		_resize_mgr(hwnd)
	{
	}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case PM_FRM_CALC_CLIENT:
			GetClientSpace((PRECT)lparam);
			return TRUE;

		  case WM_SIZE:
			if (wparam != SIZE_MINIMIZED)
				_resize_mgr.HandleSize(LOWORD(lparam), HIWORD(lparam));
			goto def;

		  case WM_GETMINMAXINFO:
			_resize_mgr.SetMinMaxInfo((LPMINMAXINFO)lparam);
			goto def;

		  default: def:
			return super::WndProc(nmsg, wparam, lparam);
		}
	}

	virtual void GetClientSpace(PRECT prect)
	{
		 if (!IsIconic(this->_hwnd)) {
			GetClientRect(this->_hwnd, prect);
		 } else {
			WINDOWPLACEMENT wp;
			GetWindowPlacement(this->_hwnd, &wp);
			prect->left = prect->top = 0;
			prect->right = wp.rcNormalPosition.right-wp.rcNormalPosition.left-
				2*(GetSystemMetrics(SM_CXSIZEFRAME)+GetSystemMetrics(SM_CXEDGE));
			prect->bottom = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-
				2*(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYEDGE))-
				GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENUSIZE);
		}   
	}

protected:
	ResizeManager _resize_mgr;
};


 /**
	This class constructs button controls.
	The button will remain existent when the C++ Button object is destroyed.
	There is no conjunction between C++ object and windows control life time.
 */
struct Button : public WindowHandle
{
	Button(HWND parent, LPCTSTR text, int left, int top, int width, int height,
			int id, DWORD flags=WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, DWORD exStyle=0);
};


 /**
	This class constructs static controls.
	The control will remain existent when the C++ object is destroyed.
	There is no conjunction between C++ object and windows control life time.
 */
struct Static : public WindowHandle
{
	Static(HWND parent, LPCTSTR text, int left, int top, int width, int height,
			int id, DWORD flags=WS_VISIBLE|WS_CHILD|SS_SIMPLE, DWORD ex_flags=0);
};


 // control color message routing for ColorStatic and HyperlinkCtrl

#define	PM_DISPATCH_CTLCOLOR	(WM_APP+0x08)

template<typename BASE> struct CtlColorParent : public BASE
{
	typedef BASE super;

	CtlColorParent(HWND hwnd)
	 : super(hwnd) {}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case WM_CTLCOLOR:
		  case WM_CTLCOLORBTN:
		  case WM_CTLCOLORDLG:
		  case WM_CTLCOLORSCROLLBAR:
		  case WM_CTLCOLORSTATIC: {
			HWND hctl = (HWND) lparam;
			return SendMessage(hctl, PM_DISPATCH_CTLCOLOR, wparam, nmsg);
		  }

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}
	}
};


#define	PM_DISPATCH_DRAWITEM	(WM_APP+0x09)

 /// draw message routing for ColorButton and PictureButton 
template<typename BASE> struct OwnerDrawParent : public BASE
{
	typedef BASE super;

	OwnerDrawParent(HWND hwnd)
	 : super(hwnd) {}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case WM_DRAWITEM:
			if (wparam) {	// should there be drawn a control?
				HWND hctl = GetDlgItem(this->_hwnd, wparam);

				if (hctl)
					return SendMessage(hctl, PM_DISPATCH_DRAWITEM, wparam, lparam);
			} /*else		// or is it a menu entry?
				; */

			return 0;

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}
	}
};


 /**
	Subclass button controls to draw them by using PM_DISPATCH_DRAWITEM
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
struct OwnerdrawnButton : public SubclassedWindow
{
	typedef SubclassedWindow super;

	OwnerdrawnButton(HWND hwnd)
	 :	super(hwnd) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	virtual void DrawItem(LPDRAWITEMSTRUCT dis) = 0;
};

extern void DrawGrayText(HDC hdc, LPRECT pRect, LPCTSTR text, int dt_flags);


 /**
	Subclass button controls to paint colored text labels.
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
/* not yet used
struct ColorButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	ColorButton(HWND hwnd, COLORREF textColor)
	 :	super(hwnd), _textColor(textColor) {}

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	COLORREF _textColor;
};
*/


struct FlatButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	FlatButton(HWND hwnd)
	 : super(hwnd), _active(false) {}

	FlatButton(HWND owner, int id)
	 : super(GetDlgItem(owner, IDOK)), _active(false) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	COLORREF _textColor;
	COLORREF _activeColor;
	bool	_active;
};


 /**
	Subclass button controls to paint pictures left to the labels.
	The buttons should have set the style bit BS_OWNERDRAW.
	The owning window should use the OwnerDrawParent template to route owner draw messages to the buttons.
 */
struct PictureButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	PictureButton(HWND hwnd, HICON hIcon, HBRUSH hbrush=GetSysColorBrush(COLOR_BTNFACE), bool flat=false)
	 :	super(hwnd), _hIcon(hIcon), _hBmp(0), _hBrush(hbrush), _flat(flat)
	{
		_cx = 16;
		_cy = 16;
	}

	PictureButton(HWND hparent, int id, HICON hIcon, HBRUSH hbrush=GetSysColorBrush(COLOR_BTNFACE), bool flat=false)
	 :	super(GetDlgItem(hparent, id)), _hIcon(hIcon), _hBmp(0), _hBrush(hbrush), _flat(flat)
	{
		_cx = 16;
		_cy = 16;
	}

	PictureButton(HWND hwnd, HBITMAP hBmp, HBRUSH hbrush=GetSysColorBrush(COLOR_BTNFACE), bool flat=false)
	 :	super(hwnd), _hIcon(0), _hBmp(hBmp), _hBrush(hbrush), _flat(flat)
	{
		BITMAP bmp;
		GetObject(hBmp, sizeof(bmp), &bmp);
		_cx = bmp.bmWidth;
		_cy = bmp.bmHeight;
	}

	PictureButton(HWND hparent, int id, HBITMAP hBmp, HBRUSH hbrush=GetSysColorBrush(COLOR_BTNFACE), bool flat=false)
	 :	super(GetDlgItem(hparent, id)), _hIcon(0), _hBmp(hBmp), _hBrush(hbrush), _flat(flat)
	{
		BITMAP bmp;
		GetObject(hBmp, sizeof(bmp), &bmp);
		_cx = bmp.bmWidth;
		_cy = bmp.bmHeight;
	}

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	HICON	_hIcon;
	HBITMAP	_hBmp;
	HBRUSH	_hBrush;

	int		_cx;
	int		_cy;

	bool	_flat;
};


struct ColorStatic : public SubclassedWindow
{
	typedef SubclassedWindow super;

	ColorStatic(HWND hwnd, COLORREF textColor=RGB(255,0,0), HBRUSH hbrush_bkgnd=0, HFONT hfont=0)
	 :	super(hwnd),
		_textColor(textColor),
		_hbrush_bkgnd(hbrush_bkgnd),
		_hfont(hfont)
	{
	}

	ColorStatic(HWND owner, int id, COLORREF textColor=RGB(255,0,0), HBRUSH hbrush_bkgnd=0, HFONT hfont=0)
	 :	super(GetDlgItem(owner, id)),
		_textColor(textColor),
		_hbrush_bkgnd(hbrush_bkgnd),
		_hfont(hfont)
	{
	}

protected:
	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		if (nmsg == PM_DISPATCH_CTLCOLOR) {
			HDC hdc = (HDC) wparam;

			SetTextColor(hdc, _textColor);

			if (_hfont)
				SelectFont(hdc, _hfont);

			if (_hbrush_bkgnd)
				return (LRESULT)_hbrush_bkgnd;
			else {
				SetBkMode(hdc, TRANSPARENT);
				return (LRESULT)GetStockBrush(HOLLOW_BRUSH);
			}
		} else
			return super::WndProc(nmsg, wparam, lparam);
	}

	COLORREF	_textColor;
	HBRUSH		_hbrush_bkgnd;
	HFONT		_hfont;
};


  /// Hyperlink Controls

struct HyperlinkCtrl : public SubclassedWindow
{
	typedef SubclassedWindow super;

	HyperlinkCtrl(HWND hwnd, COLORREF colorLink=RGB(0,0,255), COLORREF colorVisited=RGB(128,0,128));
	HyperlinkCtrl(HWND owner, int id, COLORREF colorLink=RGB(0,0,255), COLORREF colorVisited=RGB(128,0,128));

	~HyperlinkCtrl();

	String	_cmd;

protected:
	COLORREF _textColor;
	COLORREF _colorVisited;
	HFONT	 _hfont;
	HCURSOR	_crsr_link;

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void init();

	bool LaunchLink()
	{
		if (!_cmd.empty()) {
			HINSTANCE hinst = ShellExecute(GetParent(_hwnd), _T("open"), _cmd, 0, 0, SW_SHOWNORMAL);
			return (int)hinst > HINSTANCE_ERROR;
		}

		return true;
	}
};


 /// encapsulation of tool tip controls
struct ToolTip : public WindowHandle
{
	typedef WindowHandle super;

	ToolTip(HWND owner);

	void activate(BOOL active=TRUE)
	{
		SendMessage(_hwnd, TTM_ACTIVATE, active, 0);
	}

	void add(HWND hparent, HWND htool, LPCTSTR txt=LPSTR_TEXTCALLBACK, LPARAM lparam=0)
	{
		TOOLINFO ti = {
			sizeof(TOOLINFO), TTF_SUBCLASS|TTF_IDISHWND|TTF_TRANSPARENT, hparent, (UINT)htool,
			{0,0,0,0}, 0, (LPTSTR)txt, lparam
		};

#ifdef UNICODE	///@todo Why is it neccesary to try both TTM_ADDTOOLW and TTM_ADDTOOLW ?!
		if (!SendMessage(_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&ti))
			SendMessage(_hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
#else
		if (!SendMessage(_hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti))
			SendMessage(_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&ti);
#endif
	}

	void add(HWND hparent, UINT id, const RECT& rect, LPCTSTR txt=LPSTR_TEXTCALLBACK, LPARAM lparam=0)
	{
		TOOLINFO ti = {
			sizeof(TOOLINFO), TTF_SUBCLASS|TTF_TRANSPARENT, hparent, id,
			{rect.left,rect.top,rect.right,rect.bottom}, 0, (LPTSTR)txt, lparam
		};

#ifdef UNICODE
		if (!SendMessage(_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&ti))
			SendMessage(_hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
#else
		if (!SendMessage(_hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti))
			SendMessage(_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&ti);
#endif
	}

	void remove(HWND hparent, HWND htool)
	{
		TOOLINFO ti = {
			sizeof(TOOLINFO), TTF_IDISHWND, hparent, (UINT)htool,
			{0,0,0,0}, 0, 0, 0
		};

		SendMessage(_hwnd, TTM_DELTOOL, 0, (LPARAM)&ti);
	}

	void remove(HWND hparent, UINT id)
	{
		TOOLINFO ti = {
			sizeof(TOOLINFO), 0, hparent, id,
			{0,0,0,0}, 0, 0, 0
		};

		SendMessage(_hwnd, TTM_DELTOOL, 0, (LPARAM)&ti);
	}
};


inline int ListView_GetItemData(HWND list_ctrl, int idx)
{
	LV_ITEM item;

	item.mask = LVIF_PARAM;
	item.iItem = idx;

	if (!ListView_GetItem(list_ctrl, &item))
		return 0;

	return item.lParam;
}

inline int ListView_FindItemPara(HWND list_ctrl, LPARAM param)
{
	LVFINDINFO fi;

	fi.flags = LVFI_PARAM;
	fi.lParam = param;

	return ListView_FindItem(list_ctrl, (unsigned)-1, &fi);
}

inline int ListView_GetFocusedItem(HWND list_ctrl)
{
	int idx = ListView_GetItemCount(list_ctrl);

	while(--idx >= 0)
		if (ListView_GetItemState(list_ctrl, idx, LVIS_FOCUSED))
			break;

	return idx;
}


 /// sorting of list controls
struct ListSort : public WindowHandle
{
	ListSort(HWND hwndListview, PFNLVCOMPARE compare_fct);

	void	toggle_sort(int idx);
	void	sort();

	int		_sort_crit;
	bool	_direction;

protected:
	PFNLVCOMPARE _compare_fct;

	static int CALLBACK CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);
};


inline LPARAM TreeView_GetItemData(HWND hwndTreeView, HTREEITEM hItem)
{
	TVITEM tvItem;

	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = hItem;

	if (!TreeView_GetItem(hwndTreeView, &tvItem))
		return 0;

	return tvItem.lParam;
}


enum {TRAYBUTTON_LEFT=0, TRAYBUTTON_RIGHT, TRAYBUTTON_MIDDLE};

#define	PM_TRAYICON		(WM_APP+0x20)

#define	WINMSG_TASKBARCREATED	TEXT("TaskbarCreated")


struct TrayIcon
{
	TrayIcon(HWND hparent, UINT id)
	 :	_hparent(hparent), _id(id) {}

	~TrayIcon()
		{Remove();}

	void	Add(HICON hIcon, LPCTSTR tooltip=NULL)
		{Set(NIM_ADD, _id, hIcon, tooltip);}

	void	Modify(HICON hIcon, LPCTSTR tooltip=NULL)
		{Set(NIM_MODIFY, _id, hIcon, tooltip);}

	void	Remove()
	{
		NOTIFYICONDATA nid = {
			sizeof(NOTIFYICONDATA),	// cbSize
			_hparent,				// hWnd
			_id,					// uID
		};

		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

protected:
	HWND	_hparent;
	UINT	_id;

	void	Set(DWORD dwMessage, UINT id, HICON hIcon, LPCTSTR tooltip=NULL)
	{
		NOTIFYICONDATA nid = {
			sizeof(NOTIFYICONDATA),	// cbSize
			_hparent,				// hWnd
			id,						// uID
			NIF_MESSAGE|NIF_ICON,	// uFlags
			PM_TRAYICON,			// uCallbackMessage
			hIcon					// hIcon
		};

		if (tooltip)
			lstrcpyn(nid.szTip, tooltip, COUNTOF(nid.szTip));

		if (nid.szTip[0])
			nid.uFlags |= NIF_TIP;

		Shell_NotifyIcon(dwMessage, &nid);
	}
};


template<typename BASE> struct TrayIconControllerTemplate : public BASE
{
	typedef BASE super;

	TrayIconControllerTemplate(HWND hwnd) : BASE(hwnd),
		WM_TASKBARCREATED(RegisterWindowMessage(WINMSG_TASKBARCREATED))
	{
	}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		if (nmsg == PM_TRAYICON) {
			switch(lparam) {
			  case WM_MOUSEMOVE:
				TrayMouseOver(wparam);
				break;

			  case WM_LBUTTONDOWN:
				TrayClick(wparam, TRAYBUTTON_LEFT);
				break;

			  case WM_LBUTTONDBLCLK:
				TrayDblClick(wparam, TRAYBUTTON_LEFT);
				break;

			  case WM_RBUTTONDOWN:
				TrayClick(wparam, TRAYBUTTON_RIGHT);
				break;

			  case WM_RBUTTONDBLCLK:
				TrayDblClick(wparam, TRAYBUTTON_RIGHT);
				break;

			  case WM_MBUTTONDOWN:
				TrayClick(wparam, TRAYBUTTON_MIDDLE);
				break;

			  case WM_MBUTTONDBLCLK:
				TrayDblClick(wparam, TRAYBUTTON_MIDDLE);
				break;
			}

			return 0;
		} else if (nmsg == WM_TASKBARCREATED) {
			AddTrayIcons();
			return 0;
		} else
			return super::WndProc(nmsg, wparam, lparam);
	}

	virtual void AddTrayIcons() = 0;
	virtual void TrayMouseOver(UINT id) {}
	virtual void TrayClick(UINT id, int btn) {}
	virtual void TrayDblClick(UINT id, int btn) {}

protected:
	const UINT WM_TASKBARCREATED;
};


struct EditController : public SubclassedWindow
{
	typedef SubclassedWindow super;

	EditController(HWND hwnd)
	 :	super(hwnd)
	{
	}

protected:
	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		if (nmsg==WM_KEYDOWN && wparam==VK_RETURN) {
			SendParent(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(_hwnd),1), (LPARAM)_hwnd);
			return 0;
		} else
			return super::WndProc(nmsg, wparam, lparam);
	}
};
