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
 // filechild.h
 //
 // Martin Fuchs, 23.07.2003
 //


 /// information structure for creation of FileChildWindow
struct FileChildWndInfo
{
	FileChildWndInfo(LPCTSTR path);

	ENTRY_TYPE	_etype;
	LPCTSTR		_path;

	WINDOWPLACEMENT _pos;
	int			_open_mode;	//OPEN_WINDOW_MODE
};

 /// information structure for creation of ShellBrowserChild
struct ShellChildWndInfo : public FileChildWndInfo
{
	ShellChildWndInfo(LPCTSTR path, const ShellPath& root_shell_path);

	ShellPath	_shell_path;
	ShellPath	_root_shell_path;
};

 /// information structure for creation of FileChildWindow for NT object namespace
struct NtObjChildWndInfo : public FileChildWndInfo
{
	NtObjChildWndInfo(LPCTSTR path);
};

 /// information structure for creation of FileChildWindow for the Registry
struct RegistryChildWndInfo : public FileChildWndInfo
{
	RegistryChildWndInfo(LPCTSTR path);
};

 /// information structure for creation of FileChildWindow for the Registry
struct FATChildWndInfo : public FileChildWndInfo
{
	FATChildWndInfo(LPCTSTR path);
};


 /// MDI child window displaying file lists
struct FileChildWindow : public ChildWindow
{
	typedef ChildWindow super;

	FileChildWindow(HWND hwnd, const FileChildWndInfo& info);
	~FileChildWindow();

	static FileChildWindow* create(HWND hmdiclient, const FileChildWndInfo& info);

protected:
	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int		Command(int id, int code);
	int		Notify(int id, NMHDR* pnmh);

	virtual void resize_children(int cx, int cy);

	void	scan_entry(Entry* entry, HWND hwnd);

	bool	expand_entry(Entry* dir);
	static void collapse_entry(Pane* pane, Entry* dir);

	void	set_curdir(Entry* entry, HWND hwnd);
	void	activate_entry(Pane* pane, HWND hwnd);

protected:
	Root	_root;
	Pane*	_left;
	Pane*	_right;
	SORT_ORDER _sortOrder;
	TCHAR	_path[MAX_PATH];
	bool	_header_wdths_ok;

public:
	const Root& get_root() const {return _root;}

	void	set_focus_pane(Pane* pane)
		{_focus_pane = pane==_right? 1: 0;}

	void	switch_focus_pane()
		{SetFocus(_focus_pane? *_left: *_right);}
};


 /// The "Execute..."-dialog lets the user enter a command line to launch.
struct ExecuteDialog {	///@todo use class Dialog
	TCHAR	cmd[MAX_PATH];
	int		cmdshow;

	static BOOL CALLBACK WndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
};
