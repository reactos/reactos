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
 // startmenu.h
 //
 // Martin Fuchs, 16.08.2003
 //


#define _LIGHT_STARTMENU
#define _LAZY_ICONEXTRACT
#define _SINGLE_ICONEXTRACT


#define	CLASSNAME_STARTMENU		TEXT("ReactosStartmenuClass")
#define	TITLE_STARTMENU			TEXT("Start Menu")


#define	STARTMENU_WIDTH_MIN		120
#define	STARTMENU_LINE_HEIGHT	22
#define	STARTMENU_SEP_HEIGHT	(STARTMENU_LINE_HEIGHT/2)
#define	STARTMENU_TOP_BTN_SPACE	8


 // private message constants
#define	PM_STARTMENU_CLOSED		(WM_APP+0x11)
#define	PM_STARTENTRY_LAUNCHED	(WM_APP+0x12)

#ifndef _LIGHT_STARTMENU
#define	PM_STARTENTRY_FOCUSED	(WM_APP+0x13)
#endif

#define	PM_UPDATE_ICONS			(WM_APP+0x14)
#define	PM_SELECT_ENTRY			(WM_APP+0x15)


 /// StartMenuDirectory is used to store the base directory of start menus.
struct StartMenuDirectory
{
	StartMenuDirectory(const ShellDirectory& dir, bool subfolders=true)
	 :	_dir(dir), _subfolders(subfolders)
	{
	}

	~StartMenuDirectory()
	{
		_dir.free_subentries();
	}

	ShellDirectory _dir;
	bool	_subfolders;
};

typedef list<StartMenuDirectory> StartMenuShellDirs;
typedef set<Entry*> ShellEntrySet;

 /// structure holding information about one start menu entry
struct StartMenuEntry
{
	StartMenuEntry() : _icon_id(ICID_UNKNOWN) {}

	String	_title;
	ICON_ID	_icon_id;
	ShellEntrySet _entries;
};


extern int GetStartMenuBtnTextWidth(HDC hdc, LPCTSTR title, HWND hwnd);


#ifndef _LIGHT_STARTMENU

 /**
	StartMenuButton draws the face of a StartMenuCtrl button control.
 */
struct StartMenuButton : public OwnerdrawnButton
{
	typedef OwnerdrawnButton super;

	StartMenuButton(HWND hwnd, ICON_ID icon_id, bool hasSubmenu)
	 :	super(hwnd), _hIcon(hIcon), _hasSubmenu(hasSubmenu) {}

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	virtual void DrawItem(LPDRAWITEMSTRUCT dis);

	ICON_ID	_icon_id;
	bool	_hasSubmenu;
};


 /**
	To create a Startmenu button control, construct a StartMenuCtrl object.
 */
struct StartMenuCtrl : public Button
{
	StartMenuCtrl(HWND parent, int x, int y, int w, LPCTSTR title,
					UINT id, HICON hIcon=0, bool hasSubmenu=false, DWORD style=WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, DWORD exStyle=0)
	 :	Button(parent, title, x, y, w, STARTMENU_LINE_HEIGHT, id, style, exStyle)
	{
		*new StartMenuButton(_hwnd, hIcon, hasSubmenu);

		SetWindowFont(_hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
	}
};


 /// separator between start menu entries
struct StartMenuSeparator : public Static
{
	StartMenuSeparator(HWND parent, int x, int y, int w, DWORD style=WS_VISIBLE|WS_CHILD|WS_DISABLED|SS_ETCHEDHORZ, DWORD exStyle=0)
	 :	Static(parent, NULL, x, y+STARTMENU_SEP_HEIGHT/2-1, w, 2, -1, style, exStyle)
	{
	}
};

#endif


typedef list<ShellPath> StartMenuFolders;

 /// structor containing information for creating start menus
struct StartMenuCreateInfo
{
	StartMenuCreateInfo() : _border_top(0) {}

	StartMenuFolders _folders;
	int		_border_top;
	String	_title;
	Window::CREATORFUNC_INFO _creator;
};

#define STARTMENU_CREATOR(WND_CLASS) WINDOW_CREATOR_INFO(WND_CLASS, StartMenuCreateInfo)

typedef map<int, StartMenuEntry> ShellEntryMap;


#ifdef _LIGHT_STARTMENU

struct SMBtnInfo
{
	SMBtnInfo(const StartMenuEntry& entry, int id, bool hasSubmenu=false, bool enabled=true)
	 :	_title(entry._title),
		_icon_id(entry._icon_id),
		_id(id),
		_hasSubmenu(hasSubmenu),
		_enabled(enabled)
	{
	}

	SMBtnInfo(LPCTSTR title, ICON_ID icon_id, int id, bool hasSubmenu=false, bool enabled=true)
	 :	_title(title),
		_icon_id(icon_id),
		_id(id),
		_hasSubmenu(hasSubmenu),
		_enabled(enabled)
	{
	}

	String	_title;
	ICON_ID	_icon_id;
	int		_id;
	bool	_hasSubmenu;
	bool	_enabled;
};

typedef vector<SMBtnInfo> SMBtnVector;

extern void DrawStartMenuButton(HDC hdc, const RECT& rect, LPCTSTR title, const SMBtnInfo& btn, bool has_focus, bool pushed);

#else

extern void DrawStartMenuButton(HDC hdc, const RECT& rect, LPCTSTR title, HICON hIcon,
								bool hasSubmenu, bool enabled, bool has_focus, bool pushed);

#endif


 /**
	Startmenu window.
	To create a start menu call its Create() function.
 */
struct StartMenu :
#ifdef _LIGHT_STARTMENU
	public OwnerDrawParent<Window>
#else
	public OwnerDrawParent<DialogWindow>
#endif
{
#ifdef _LIGHT_STARTMENU
	typedef OwnerDrawParent<Window> super;
#else
	typedef OwnerDrawParent<DialogWindow> super;
#endif

	StartMenu(HWND hwnd);
	StartMenu(HWND hwnd, const StartMenuCreateInfo& create_info);
	~StartMenu();

	static HWND Create(int x, int y, const StartMenuFolders&, HWND hwndParent, LPCTSTR title, CREATORFUNC_INFO creator=s_def_creator);
	static CREATORFUNC_INFO s_def_creator;

protected:
	 // overridden member functions
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	 // window class
	static BtnWindowClass& GetWndClasss();

	 // data members
	int		_next_id;
	ShellEntryMap _entries;
	StartMenuShellDirs _dirs;

	int		_submenu_id;
	WindowHandle _submenu;

	int		_border_left;	// left border in pixels
	int		_border_top;	// top border in pixels

	POINT	_last_pos;

	StartMenuCreateInfo _create_info;	// copy of the original create info

#ifdef _LIGHT_STARTMENU
	SMBtnVector _buttons;
	int		_selected_id;
	LPARAM	_last_mouse_pos;

	void	ResizeToButtons();
	int		ButtonHitTest(POINT pt);
	void	InvalidateSelection();
	const SMBtnInfo* GetButtonInfo(int id) const;
	bool	SelectButton(int id, bool open_sub=true);
	bool	SelectButtonIndex(int idx, bool open_sub=true);
	int		GetSelectionIndex();
	virtual void ProcessKey(int vk);
	bool	Navigate(int step);
	bool	OpenSubmenu(bool select_first=false);
	bool	JumpToNextShortcut(char c);
#endif

	 // member functions
	void	ResizeButtons(int cx);

	virtual void AddEntries();

	StartMenuEntry& AddEntry(const String& title, ICON_ID icon_id, Entry* entry);
	StartMenuEntry& AddEntry(const String& title, ICON_ID icon_id=ICID_NONE, int id=-1);
	StartMenuEntry& AddEntry(const ShellFolder folder, ShellEntry* entry);
	StartMenuEntry& AddEntry(const ShellFolder folder, Entry* entry);

	void	AddShellEntries(const ShellDirectory& dir, int max=-1, bool subfolders=true);

	void	AddButton(LPCTSTR title, ICON_ID icon_id=ICID_NONE, bool hasSubmenu=false, int id=-1, bool enabled=true);
	void	AddSeparator();

	bool	CloseSubmenus() {return CloseOtherSubmenus();}
	bool	CloseOtherSubmenus(int id=0);
	void	CreateSubmenu(int id, LPCTSTR title, CREATORFUNC_INFO creator=s_def_creator);
	bool	CreateSubmenu(int id, int folder, LPCTSTR title, CREATORFUNC_INFO creator=s_def_creator);
	bool	CreateSubmenu(int id, int folder1, int folder2, LPCTSTR title, CREATORFUNC_INFO creator=s_def_creator);
	void	CreateSubmenu(int id, const StartMenuFolders& new_folders, LPCTSTR title, CREATORFUNC_INFO creator=s_def_creator);
	void	ActivateEntry(int id, const ShellEntrySet& entries);
	virtual void CloseStartMenu(int id=0);

	bool	GetButtonRect(int id, PRECT prect) const;

	void	DrawFloatingButton(HDC hdc);
	void	GetFloatingButtonRect(LPRECT prect);

	void	Paint(PaintCanvas& canvas);
	void	UpdateIcons(/*int idx*/);
};


 // declare shell32's "Run..." dialog export function
typedef	void (WINAPI* RUNFILEDLG)(HWND hwndOwner, HICON hIcon, LPCSTR lpstrDirectory, LPCSTR lpstrTitle, LPCSTR lpstrDescription, UINT uFlags);

 //
 // Flags for RunFileDlg
 //

#define	RFF_NOBROWSE		0x01	// Removes the browse button.
#define	RFF_NODEFAULT		0x02	// No default item selected.
#define	RFF_CALCDIRECTORY	0x04	// Calculates the working directory from the file name.
#define	RFF_NOLABEL			0x08	// Removes the edit box label.
#define	RFF_NOSEPARATEMEM	0x20	// Removes the Separate Memory Space check box (Windows NT only).


 // declare more undocumented shell32 functions
typedef	void (WINAPI* EXITWINDOWSDLG)(HWND hwndOwner);
typedef	int (WINAPI* RESTARTWINDOWSDLG)(HWND hwndOwner, LPCWSTR reason, UINT flags);
typedef	BOOL (WINAPI* SHFINDFILES)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch);
typedef	BOOL (WINAPI* SHFINDCOMPUTER)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch);


 /// Handling of standard start menu commands
struct StartMenuHandler : public StartMenu
{
	typedef StartMenu super;

	StartMenuHandler(HWND hwnd)
	 :	super(hwnd)
	{
	}

	StartMenuHandler(HWND hwnd, const StartMenuCreateInfo& create_info)
	 :	super(hwnd, create_info)
	{
	}

protected:
	int		Command(int id, int code);

	static void	ShowLaunchDialog(HWND hwndOwner);
	static void	ShowRestartDialog(HWND hwndOwner, UINT flags);
	static void	ShowSearchDialog();
	static void	ShowSearchComputer();
};


 /// Startmenu root window
struct StartMenuRoot : public StartMenuHandler
{
	typedef StartMenuHandler super;

	StartMenuRoot(HWND hwnd);

	static HWND Create(HWND hwndDesktopBar);
	void	TrackStartmenu();

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	SIZE	_logo_size;

	void	AddEntries();
	void	Paint(PaintCanvas& canvas);
	void	CloseStartMenu(int id=0);
	virtual void ProcessKey(int vk);
};


 /// Settings sub-startmenu
struct SettingsMenu : public StartMenuHandler
{
	typedef StartMenuHandler super;

	SettingsMenu(HWND hwnd, const StartMenuCreateInfo& create_info)
	 :	super(hwnd, create_info)
	{
	}

protected:
	void	AddEntries();
};


 /// "Browse Files..." sub-start menu
struct BrowseMenu : public StartMenuHandler
{
	typedef StartMenuHandler super;

	BrowseMenu(HWND hwnd, const StartMenuCreateInfo& create_info)
	 :	super(hwnd, create_info)
	{
	}

protected:
	void	AddEntries();
};


 /// Search sub-startmenu
struct SearchMenu : public StartMenuHandler
{
	typedef StartMenuHandler super;

	SearchMenu(HWND hwnd, const StartMenuCreateInfo& create_info)
	 :	super(hwnd, create_info)
	{
	}

protected:
	void	AddEntries();
};


#define	RECENT_DOCS_COUNT	20	///@todo read max. count of entries from registry

 /// "Recent Files" sub-start menu
struct RecentStartMenu : public StartMenu
{
	typedef StartMenu super;

	RecentStartMenu(HWND hwnd, const StartMenuCreateInfo& create_info)
	 :	super(hwnd, create_info)
	{
	}

protected:
	virtual void AddEntries();
};
