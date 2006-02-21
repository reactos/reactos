/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 // Explorer clone, lean version
 //
 // entries.h
 //
 // Martin Fuchs, 23.07.2003
 //


enum SORT_ORDER {
	SORT_NONE,
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
};

enum SCAN_FLAGS {
	SCAN_DONT_EXTRACT_ICONS	= 1,
	SCAN_DONT_ACCESS		= 2
};

#ifndef ATTRIBUTE_SYMBOLIC_LINK
#define ATTRIBUTE_SYMBOLIC_LINK		0x40000000
#define	ATTRIBUTE_EXECUTABLE		0x80000000
#endif


struct ShellDirectory;


 /// base of all file entries
struct ShellEntry
{
	ShellEntry();
	ShellEntry(const ShellEntry& other);
	ShellEntry(ShellDirectory* parent, LPITEMIDLIST shell_path);
	ShellEntry(ShellDirectory* parent, const ShellPath& shell_path);

protected:
	ShellEntry(LPITEMIDLIST shell_path);
	ShellEntry(const ShellPath& shell_path);

public:
	virtual ~ShellEntry();

	ShellEntry*	_next;
	ShellEntry*	_down;
	ShellDirectory*	_up;

	bool		_expanded;
	bool		_scanned;
	int 		_level;

	WIN32_FIND_DATA _data;

	SFGAOF		_shell_attribs;
	LPTSTR		_display_name;

	int /*ICON_ID*/ _icon_id;

	ShellPath	_pidl;	// parent relative PIDL

	void	free_subentries();

	int		extract_icon();
	int		safe_extract_icon();

	virtual bool get_path(PTSTR path, size_t path_count) const;
	ShellPath create_absolute_pidl() const;
	BOOL launch_entry(HWND hwnd, UINT nCmdShow=SW_SHOWNORMAL);
	HRESULT GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut);

	IShellFolder* get_parent_folder() const;
};


 /// base for all directory entries
struct Directory {
protected:
	Directory() : _path(NULL) {}
	virtual ~Directory() {}

	void*	_path;
};


 /// shell folder entry
struct ShellDirectory : public ShellEntry, public Directory
{
	ShellDirectory(ShellFolder& root_folder, const ShellPath& shell_path, HWND hwnd)
	 :	ShellEntry(shell_path),
		_folder(root_folder, shell_path),
		_hwnd(hwnd)
	{
		CONTEXT("ShellDirectory::ShellDirectory()");

		lstrcpy(_data.cFileName, root_folder.get_name(shell_path, SHGDN_FORPARSING));
		_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		_shell_attribs = SFGAO_FOLDER;

		ShellFolder subfolder(root_folder, shell_path);
		IShellFolder* pFolder = subfolder;
		pFolder->AddRef();
		_path = pFolder;
	}

	explicit ShellDirectory(ShellDirectory* parent, LPITEMIDLIST shell_path, HWND hwnd)
	 :	ShellEntry(parent, shell_path),
		_folder(parent->_folder, shell_path),
		_hwnd(hwnd)
	{
		/* not neccessary - the caller will fill the info
		lstrcpy(_data.cFileName, _folder.get_name(shell_path));
		_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		_shell_attribs = SFGAO_FOLDER; */

		_folder->AddRef();
		_path = _folder;
	}

	ShellDirectory(const ShellDirectory& other)
	 :	ShellEntry(other),
		Directory(other),
		_folder(other._folder),
		_hwnd(other._hwnd)
	{
		IShellFolder* pFolder = (IShellFolder*)_path;
		pFolder->AddRef();
	}

	~ShellDirectory()
	{
		IShellFolder* pFolder = (IShellFolder*)_path;
		_path = NULL;
		pFolder->Release();
	}

	void	read_directory(SORT_ORDER sortOrder, int scan_flags=0);
	void	read_directory(int scan_flags=0);
	void	sort_directory(SORT_ORDER sortOrder);
	void	smart_scan(int scan_flags=0);

	virtual ShellEntry* find_entry(const void* p);
	virtual bool get_path(PTSTR path) const;

	int		extract_icons();

	ShellFolder _folder;
	HWND	_hwnd;

protected:
	void	fill_w32fdata_shell(LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA*, bool do_access=true);
};


inline IShellFolder* ShellEntry::get_parent_folder() const
{
	if (_up)
		return _up->_folder;
	else
		return GetDesktopFolder();
}


 /// root entry for file system trees
struct Root {
	Root();
	~Root();

	ShellDirectory*	_entry;
	TCHAR	_path[MAX_PATH];
	TCHAR	_volname[_MAX_FNAME];
	TCHAR	_fs[_MAX_DIR];
	DWORD	_drive_type;
	DWORD	_fs_flags;
};
