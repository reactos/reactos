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
 // entries.h
 //
 // Martin Fuchs, 23.07.2003
 //


enum ENTRY_TYPE {
	ET_WINDOWS,
#ifdef __WINE__
	ET_UNIX,
#endif
	ET_SHELL,
	ET_NTOBJS,
	ET_REGISTRY
};

enum SORT_ORDER {
	SORT_NONE,
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
};

enum SCAN_FLAGS {
	SCAN_EXTRACT_ICONS	= 1,
	SCAN_DO_ACCESS		= 2,

	SCAN_ALL			= 3,

	SCAN_FILESYSTEM		= 4
};

#ifndef ATTRIBUTE_SYMBOLIC_LINK
#define ATTRIBUTE_SYMBOLIC_LINK		0x40000000
#define	ATTRIBUTE_EXECUTABLE		0x80000000
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

	ENTRY_TYPE	_etype;
	int /*ICON_ID*/ _icon_id;

	BY_HANDLE_FILE_INFORMATION _bhfi;
	bool		_bhfi_valid;

	void	free_subentries();

	void	read_directory(SORT_ORDER sortOrder, int scan_flags=SCAN_ALL);
	Entry*	read_tree(const void* path, SORT_ORDER sortOrder);
	void	sort_directory(SORT_ORDER sortOrder);
	void	smart_scan(int scan_flags=SCAN_ALL);
	void	extract_icon();

	virtual void read_directory(int scan_flags=SCAN_ALL) {}
	virtual const void* get_next_path_component(const void*) {return NULL;}
	virtual Entry* find_entry(const void*) {return NULL;}
	virtual bool get_path(PTSTR path) const = 0;
	virtual ShellPath create_absolute_pidl() const;
	virtual HRESULT GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut);
	virtual BOOL launch_entry(HWND hwnd, UINT nCmdShow=SW_SHOWNORMAL);
};


 /// base for all directory entries
struct Directory {
protected:
	Directory() : _path(NULL) {}
	virtual ~Directory() {}

	 // default implementation like that of Windows file systems
	virtual const void* get_next_path_component(const void*);

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
};
