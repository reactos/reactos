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
 // shellfs.h
 //
 // Martin Fuchs, 23.07.2003
 //


 /// shell file/directory entry
struct ShellEntry : public Entry
{
	ShellEntry(Entry* parent, LPITEMIDLIST shell_path) : Entry(parent, ET_SHELL), _pidl(shell_path) {}
	ShellEntry(Entry* parent, const ShellPath& shell_path) : Entry(parent, ET_SHELL), _pidl(shell_path) {}

	virtual bool get_path(PTSTR path) const;
	virtual ShellPath create_absolute_pidl() const;
	virtual BOOL launch_entry(HWND hwnd, UINT nCmdShow=SW_SHOWNORMAL);
	virtual HRESULT GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut);

	IShellFolder* get_parent_folder() const;

	ShellPath	_pidl;	// parent relative PIDL

protected:
	ShellEntry(LPITEMIDLIST shell_path) : Entry(ET_SHELL), _pidl(shell_path) {}
	ShellEntry(const ShellPath& shell_path) : Entry(ET_SHELL), _pidl(shell_path) {}
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

	virtual void read_directory(int scan_flags=SCAN_ALL);
	virtual const void* get_next_path_component(const void*) const;
	virtual Entry* find_entry(const void* p);

	virtual bool get_path(PTSTR path) const;

	int	extract_icons();

	ShellFolder _folder;
	HWND	_hwnd;

protected:
	bool	fill_w32fdata_shell(LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA*, BY_HANDLE_FILE_INFORMATION*, bool do_access=true);
};


inline IShellFolder* ShellEntry::get_parent_folder() const
{
	if (_up)
		return static_cast<ShellDirectory*>(_up)->_folder;
	else
		return GetDesktopFolder();
}
