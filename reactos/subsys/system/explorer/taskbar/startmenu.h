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


#include <list>


#define	CLASSNAME_STARTMENU		_T("ReactosStartmenuClass")
#define	TITLE_STARTMENU			_T("Start Menu")


 // Startmenu button
struct StartMenuButton : public Button
{
	StartMenuButton(HWND parent, int y, LPCTSTR title,
					UINT id, HICON hIcon=0, DWORD style=WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW, DWORD exStyle=0)
	 :	Button(parent, title, 2, y, STARTMENU_WIDTH-4, STARTMENU_LINE_HEIGHT, id, style, exStyle)
	{
		*new StartmenuEntry(_hwnd, hIcon);

		SetWindowFont(_hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
	}
};


struct StartMenuSeparator : public Static
{
	StartMenuSeparator(HWND parent, int y, DWORD style=WS_VISIBLE|WS_CHILD|SS_ETCHEDHORZ, DWORD exStyle=0)
	 :	Static(parent, NULL, 2, y+STARTMENU_SEP_HEIGHT/2-1, STARTMENU_WIDTH-4, 2, -1, style, exStyle)
	{
	}
};


struct StartMenuDirectory
{
	StartMenuDirectory(const ShellDirectory& dir, bool subfolders=true)
	 :	_dir(dir), _subfolders(subfolders) {}

	ShellDirectory _dir;
	bool	_subfolders;
};

typedef list<StartMenuDirectory> StartMenuShellDirs;

struct StartMenuEntry
{
	StartMenuEntry() : _entry(NULL), _hIcon(0) {}

	String	_title;
	HICON	_hIcon;
	const ShellEntry* _entry;
};

typedef map<UINT, StartMenuEntry> ShellEntryMap;


typedef list<ShellPath> StartMenuFolders;

#define STARTMENU_CREATOR(WND_CLASS) WINDOW_CREATOR_INFO(WND_CLASS, StartMenuFolders)


 // Startmenu window
struct StartMenu : public OwnerDrawParent<Dialog>
{
	typedef OwnerDrawParent<Dialog> super;

	StartMenu(HWND hwnd);
	StartMenu(HWND hwnd, const StartMenuFolders& info);

	static HWND Create(int x, int y, HWND hwndParent=0);
	static HWND Create(int x, int y, const StartMenuFolders&, HWND hwndParent=0, CREATORFUNC creator=s_def_creator);
	static CREATORFUNC s_def_creator;

protected:
	int		_next_id;
	ShellEntryMap _entries;
	StartMenuShellDirs _dirs;

	static BtnWindowClass s_wcStartMenu;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	virtual void AddEntries();

	StartMenuEntry& AddEntry(LPCTSTR title, HICON hIcon=0, UINT id=(UINT)-1);
	StartMenuEntry& AddEntry(const ShellFolder folder, const ShellEntry* entry);

	void	AddShellEntries(const ShellDirectory& dir, int max=-1, bool subfolders=true);

	void	AddButton(LPCTSTR title, HICON hIcon=0, UINT id=(UINT)-1);
	void	AddSeparator();

	void	CreateSubmenu(int id, const StartMenuFolders& new_folders, CREATORFUNC creator=s_def_creator);
	void	CreateSubmenu(int id, int folder1, int folder2, CREATORFUNC creator=s_def_creator);
	void	CreateSubmenu(int id, int folder, CREATORFUNC creator=s_def_creator);
	void	ActivateEntry(int id, ShellEntry* entry);
};


 // Startmenu root window
struct StartMenuRoot : public StartMenu
{
	typedef StartMenu super;

	StartMenuRoot(HWND hwnd);

	static HWND Create(int x, int y, HWND hwndParent=0);

protected:
	LRESULT	Init(LPCREATESTRUCT pcs);
	int		Command(int id, int code);
};


struct RecentStartMenu : public StartMenu
{
	typedef StartMenu super;

	RecentStartMenu(HWND hwnd, const StartMenuFolders& info);

protected:
	virtual void AddEntries();
};
