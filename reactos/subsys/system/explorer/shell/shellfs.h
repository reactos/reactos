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
 // shellfs.h
 //
 // Martin Fuchs, 23.07.2003
 //


struct ShellEntry : public Entry {
	ShellEntry(Entry* parent, LPITEMIDLIST shell_path) : Entry(parent), _pidl(shell_path) {}
	ShellEntry(Entry* parent, const ShellPath& shell_path) : Entry(parent), _pidl(shell_path) {}

	virtual void get_path(PTSTR path);
	virtual BOOL launch_entry(HWND hwnd, UINT nCmdShow);

	LPITEMIDLIST create_absolute_pidl(HWND hwnd);

	ShellPath	_pidl;

protected:
	ShellEntry(LPITEMIDLIST shell_path) : Entry(ET_SHELL), _pidl(shell_path) {}
	ShellEntry(const ShellPath& shell_path) : Entry(ET_SHELL), _pidl(shell_path) {}
};

struct ShellDirectory : public ShellEntry, public Directory {
	ShellDirectory(IShellFolder* shell_root, const ShellPath& shell_path, HWND hwnd)
	 :	ShellEntry(shell_path),
		Directory(shell_root),
		_hwnd(hwnd)
	{
	}

	ShellDirectory(ShellDirectory* parent, IShellFolder* shell_root, LPITEMIDLIST shell_path, HWND hwnd)
	 :	ShellEntry(parent, shell_path),
		Directory(shell_root),
		_folder(shell_root),
		_hwnd(hwnd)
	{
	}

	virtual void read_directory();
	virtual const void* get_next_path_component(const void*);
	virtual Entry* find_entry(const void* p);

	virtual void get_path(PTSTR path);

	ShellFolder _folder;
	HWND	_hwnd;

protected:
	bool	fill_w32fdata_shell(LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA*, BY_HANDLE_FILE_INFORMATION*);
};

