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
	StartMenuButton(HWND parent, int y, LPCTSTR text,
					UINT id, HICON hIcon, DWORD style=WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW, DWORD exStyle=0)
	 :	Button(parent, text, 2, y, STARTMENU_WIDTH-4, STARTMENU_LINE_HEIGHT, id, style, exStyle)
	{
		*new StartmenuEntry(_hwnd, hIcon);

		SetWindowFont(_hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);
	}
};


typedef list<ShellPath> StartMenuFolders;
typedef list<ShellDirectory> StartMenuShellDirs;
typedef map<UINT, const ShellEntry*> ShellEntryMap;


 // Startmenu window
struct StartMenu : public OwnerDrawParent<Dialog>
{
	typedef OwnerDrawParent<Dialog> super;

	StartMenu(HWND hwnd);
	StartMenu(HWND hwnd, const StartMenuFolders& info);

	static HWND Create(int x, int y, HWND hwndParent=0);
	static HWND Create(int x, int y, const StartMenuFolders&, HWND hwndParent=0);

protected:
	int		_next_id;
	StartMenuShellDirs _dirs;
	ShellEntryMap _entry_map;

	static BtnWindowClass s_wcStartMenu;

	LRESULT	Init(LPCREATESTRUCT pcs);
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);

	UINT	AddButton(LPCTSTR text, HICON hIcon=0, UINT id=(UINT)-1);
	UINT	AddButton(const ShellFolder folder, const ShellEntry* entry);

	void	AddShellEntries(const ShellDirectory& dir, bool subfolders=true);

	void	ActivateEntry(ShellEntry* entry);
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
