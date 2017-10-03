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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // entries.h
 //
 // Martin Fuchs, 23.07.2003
 //


enum ENTRY_TYPE {
	ET_UNKNOWN,
#ifndef _NO_WIN_FS
	ET_WINDOWS,
#endif
#ifdef __WINE__
	ET_UNIX,
#endif
	ET_SHELL,
	ET_NTOBJS,
	ET_REGISTRY,
	ET_FAT,
	ET_WEB
};

enum SORT_ORDER {
	SORT_NONE,
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
};

enum SCAN_FLAGS {
	SCAN_DONT_EXTRACT_ICONS	= 1,
	SCAN_DONT_ACCESS		= 2,
	SCAN_NO_FILESYSTEM		= 4
};

#ifndef ATTRIBUTE_SYMBOLIC_LINK
#define	ATTRIBUTE_LONGNAME			0x08000000
#define	ATTRIBUTE_VOLNAME			0x10000000
#define	ATTRIBUTE_ERASED			0x20000000
#define ATTRIBUTE_SYMBOLIC_LINK		0x40000000
#define	ATTRIBUTE_EXECUTABLE		0x80000000
#endif

enum ICONCACHE_FLAGS {
	ICF_NORMAL	 =  0,
	ICF_MIDDLE	 =  1,
	ICF_LARGE	 =  2,
	ICF_OPEN	 =  4,
	ICF_OVERLAYS =  8,
	ICF_HICON	 = 16,
	ICF_SYSCACHE = 32
};

#ifndef SHGFI_ADDOVERLAYS // missing in MinGW (as of 28.12.2005)
#define SHGFI_ADDOVERLAYS 0x000000020
#endif


 /// base of all file and directory entries
struct Entry
{
protected:
	Entry(ENTRY_TYPE etype);
	Entry(Entry* parent, ENTRY_TYPE etype);
	Entry(const Entry&);

public:
	virtual ~Entry();

	Entry*		_next;
	Entry*		_down;
	Entry*		_up;

	bool		_expanded;
	bool		_scanned;
	int 		_level;

	WIN32_FIND_DATA _data;

	SFGAOF		_shell_attribs;
	LPTSTR		_display_name;
	LPTSTR		_type_name;
	LPTSTR		_content;

	ENTRY_TYPE	_etype;
	int /*ICON_ID*/ _icon_id;

	BY_HANDLE_FILE_INFORMATION _bhfi;
	bool		_bhfi_valid;

	void	free_subentries();

	void	read_directory_base(SORT_ORDER sortOrder=SORT_NAME, int scan_flags=0);
	Entry*	read_tree(const void* path, SORT_ORDER sortOrder=SORT_NAME, int scan_flags=0);
	void	sort_directory(SORT_ORDER sortOrder);
	void	smart_scan(SORT_ORDER sortOrder=SORT_NAME, int scan_flags=0);
	int		extract_icon(ICONCACHE_FLAGS flags=ICF_NORMAL);
	int		safe_extract_icon(ICONCACHE_FLAGS flags=ICF_NORMAL);

	virtual void		read_directory(int scan_flags=0) {}
	virtual const void*	get_next_path_component(const void*) const {return NULL;}
	virtual Entry*		find_entry(const void*) {return NULL;}
	virtual bool		get_path(PTSTR path, size_t path_count) const = 0;
	virtual ShellPath	create_absolute_pidl() const {return (LPCITEMIDLIST)NULL;}
	virtual HRESULT		GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut);
	virtual ShellFolder get_shell_folder() const;
	virtual BOOL		launch_entry(HWND hwnd, UINT nCmdShow=SW_SHOWNORMAL);
	virtual HRESULT		do_context_menu(HWND hwnd, const POINT& pos, CtxMenuInterfaces& cm_ifs);

protected:
	bool	get_path_base(PTSTR path, size_t path_count, ENTRY_TYPE etype) const;
};


 /// base for all directory entries
struct Directory {
protected:
	Directory() : _path(NULL) {}
	virtual ~Directory() {}

	void*	_path;
};


 /// root entry for file system trees
struct Root {
	Root();
	~Root();

	Entry*	_entry;
	TCHAR	_path[MAX_PATH];
	TCHAR	_volname[_MAX_FNAME];
	TCHAR	_fs[_MAX_DIR];
	DWORD	_drive_type;
	DWORD	_fs_flags;
	SORT_ORDER _sort_order;

	Entry*	read_tree(LPCTSTR path, int scan_flags=0);
	Entry*	read_tree(LPCITEMIDLIST pidl, int scan_flags=0);
};
