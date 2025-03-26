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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // entries.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include <precomp.h>

//#include "entries.h"


 // allocate and initialise a directory entry
Entry::Entry(ENTRY_TYPE etype)
 :	_etype(etype)
{
	_up = NULL;
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_bhfi_valid = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_display_name = _data.cFileName;
	_type_name = NULL;
	_content = NULL;
}

Entry::Entry(Entry* parent, ENTRY_TYPE etype)
 :	_up(parent),
	_etype(etype)
{
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_bhfi_valid = false;
	_level = 0;
	_icon_id = ICID_UNKNOWN;
	_shell_attribs = 0;
	_display_name = _data.cFileName;
	_type_name = NULL;
	_content = NULL;
}

Entry::Entry(const Entry& other)
{
	_next = NULL;
	_down = NULL;
	_up = NULL;

	assert(!other._next);
	assert(!other._down);
	assert(!other._up);

	_expanded = other._expanded;
	_scanned = other._scanned;
	_level = other._level;

	_data = other._data;

	_shell_attribs = other._shell_attribs;
	_display_name = other._display_name==other._data.cFileName? _data.cFileName: _tcsdup(other._display_name);
	_type_name = other._type_name? _tcsdup(other._type_name): NULL;
	_content = other._content? _tcsdup(other._content): NULL;

	_etype = other._etype;
	_icon_id = other._icon_id;

	_bhfi = other._bhfi;
	_bhfi_valid = other._bhfi_valid;
}

 // free a directory entry
Entry::~Entry()
{
	free_subentries();

	if (_icon_id > ICID_NONE)
		g_Globals._icon_cache.free_icon(_icon_id);

	if (_display_name != _data.cFileName)
		free(_display_name);

	if (_type_name)
		free(_type_name);

	if (_content)
		free(_content);

	if (_down)
		delete _down;
}


 // read directory tree and expand to the given location
Entry* Entry::read_tree(const void* path, SORT_ORDER sortOrder, int scan_flags)
{
	CONTEXT("Entry::read_tree()");

	WaitCursor wait;

	Entry* entry = this;

	for(const void*p=path; p && entry; ) {
		entry->smart_scan(sortOrder, scan_flags);

		if (entry->_down)
			entry->_expanded = true;

		Entry* found = entry->find_entry(p);
		p = entry->get_next_path_component(p);

		entry = found;
	}

	return entry;
}


void Entry::read_directory_base(SORT_ORDER sortOrder, int scan_flags)
{
	CONTEXT("Entry::read_directory_base()");

	 // call into subclass
	read_directory(scan_flags);

#ifndef ROSSHELL
	if (g_Globals._prescan_nodes) {	///@todo _prescan_nodes should not be used for reading the start menu.
		for(Entry*entry=_down; entry; entry=entry->_next)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				entry->read_directory(scan_flags);
				entry->sort_directory(sortOrder);
			}
	}
#endif

	sort_directory(sortOrder);
}


Root::Root()
{
	memset(this, 0, sizeof(Root));
}

Root::~Root()
{
	if (_entry) {
		_entry->free_subentries();
		delete _entry;
	}
}


 // sort order for different directory/file types
enum TYPE_ORDER {
	TO_DIR,
	TO_DOT,
	TO_DOTDOT,
	TO_OTHER_DIR,
	TO_VIRTUAL_FOLDER,
	TO_FILE
};

 // distinguish between ".", ".." and any other directory names
static TYPE_ORDER TypeOrderFromDirname(LPCTSTR name)
{
	if (name[0] == '.') {
		if (name[1] == '\0')
			return TO_DOT;		// "."

		if (name[1]=='.' && name[2]=='\0')
			return TO_DOTDOT;	// ".."
	}

	return TO_OTHER_DIR;		// any other directory
}

 // directories first...
static int compareType(const Entry* entry1, const Entry* entry2)
{
	const WIN32_FIND_DATA* fd1 = &entry1->_data;
	const WIN32_FIND_DATA* fd2 = &entry2->_data;

	TYPE_ORDER order1 = fd1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY? TO_DIR: TO_FILE;
	TYPE_ORDER order2 = fd2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY? TO_DIR: TO_FILE;

	 // Handle "." and ".." as special case and move them at the very first beginning.
	if (order1==TO_DIR && order2==TO_DIR) {
		order1 = TypeOrderFromDirname(fd1->cFileName);
		order2 = TypeOrderFromDirname(fd2->cFileName);

		 // Move virtual folders after physical folders
		if (!(entry1->_shell_attribs & SFGAO_FILESYSTEM))
			order1 = TO_VIRTUAL_FOLDER;

		if (!(entry2->_shell_attribs & SFGAO_FILESYSTEM))
			order2 = TO_VIRTUAL_FOLDER;
	}

	return order2==order1? 0: order1<order2? -1: 1;
}


static int compareNothing(const void* arg1, const void* arg2)
{
	return -1;
}

static int compareName(const void* arg1, const void* arg2)
{
	const Entry* entry1 = *(const Entry**)arg1;
	const Entry* entry2 = *(const Entry**)arg2;

	int cmp = compareType(entry1, entry2);
	if (cmp)
		return cmp;

	return lstrcmpi(entry1->_data.cFileName, entry2->_data.cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const Entry* entry1 = *(const Entry**)arg1;
	const Entry* entry2 = *(const Entry**)arg2;
	const TCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(entry1, entry2);
	if (cmp)
		return cmp;

	name1 = entry1->_data.cFileName;
	name2 = entry2->_data.cFileName;

	ext1 = _tcsrchr(name1, TEXT('.'));
	ext2 = _tcsrchr(name2, TEXT('.'));

	if (ext1)
		++ext1;
	else
		ext1 = TEXT("");

	if (ext2)
		++ext2;
	else
		ext2 = TEXT("");

	cmp = lstrcmpi(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpi(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	const Entry* entry1 = *(const Entry**)arg1;
	const Entry* entry2 = *(const Entry**)arg2;

	int cmp = compareType(entry1, entry2);
	if (cmp)
		return cmp;

	cmp = entry2->_data.nFileSizeHigh - entry1->_data.nFileSizeHigh;

	if (cmp < 0)
		return -1;
	else if (cmp > 0)
		return 1;

	cmp = entry2->_data.nFileSizeLow - entry1->_data.nFileSizeLow;

	return cmp<0? -1: cmp>0? 1: 0;
}

static int compareDate(const void* arg1, const void* arg2)
{
	const Entry* entry1 = *(const Entry**)arg1;
	const Entry* entry2 = *(const Entry**)arg2;

	int cmp = compareType(entry1, entry2);
	if (cmp)
		return cmp;

	return CompareFileTime(&entry2->_data.ftLastWriteTime, &entry1->_data.ftLastWriteTime);
}


static int (*sortFunctions[])(const void* arg1, const void* arg2) = {
	compareNothing,	// SORT_NONE
	compareName,	// SORT_NAME
	compareExt, 	// SORT_EXT
	compareSize,	// SORT_SIZE
	compareDate 	// SORT_DATE
};


void Entry::sort_directory(SORT_ORDER sortOrder)
{
	if (sortOrder != SORT_NONE) {
		Entry* entry = _down;
		Entry** array, **p;
		int len;

		len = 0;
		for(entry=_down; entry; entry=entry->_next)
			++len;

		if (len) {
			array = (Entry**) alloca(len*sizeof(Entry*));

			p = array;
			for(entry=_down; entry; entry=entry->_next)
				*p++ = entry;

			 // call qsort with the appropriate compare function
			qsort(array, len, sizeof(array[0]), sortFunctions[sortOrder]);

			_down = array[0];

			for(p=array; --len; p++)
				(*p)->_next = p[1];

			(*p)->_next = 0;
		}
	}
}


void Entry::smart_scan(SORT_ORDER sortOrder, int scan_flags)
{
	CONTEXT("Entry::smart_scan()");

	if (!_scanned) {
		free_subentries();
		read_directory_base(sortOrder, scan_flags);	///@todo We could use IShellFolder2::GetDefaultColumn to determine sort order.
	}
}



int Entry::extract_icon(ICONCACHE_FLAGS flags)
{
	TCHAR path[MAX_PATH];

	ICON_ID icon_id = ICID_NONE;

	if (_etype!=ET_SHELL && get_path(path, COUNTOF(path)))	// not for ET_SHELL to display the correct desktop icon
		if (!(flags & ICF_MIDDLE))	// not for ICF_MIDDLE to extract 24x24 icons because SHGetFileInfo() doesn't support this icon size
			icon_id = g_Globals._icon_cache.extract(path, flags);

	if (icon_id == ICID_NONE) {
		if (!(flags & ICF_OVERLAYS)) {
			IExtractIcon* pExtract;
			if (SUCCEEDED(GetUIObjectOf(0, IID_IExtractIcon, (LPVOID*)&pExtract))) {
				unsigned gil_flags = 0;
				int idx;

				if (flags & ICF_OPEN)
					gil_flags |= GIL_OPENICON;

				if (SUCCEEDED(pExtract->GetIconLocation(GIL_FORSHELL, path, COUNTOF(path), &idx, &gil_flags))) {
					if (gil_flags & GIL_NOTFILENAME)
						icon_id = g_Globals._icon_cache.extract(pExtract, path, idx, flags);
					else {
						if (idx == -1)
							idx = 0;	// special case for some control panel applications ("System")

						icon_id = g_Globals._icon_cache.extract(path, idx, flags);
					}

				/* using create_absolute_pidl() [see below] results in more correct icons for some control panel applets (NVidia display driver).
					if (icon_id == ICID_NONE) {
						SHFILEINFO sfi;

						if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON))
							icon_id = g_Globals._icon_cache.add(sfi.hIcon)._id;
					} */
				/*
					if (icon_id == ICID_NONE) {
						LPBYTE b = (LPBYTE) alloca(0x10000);
						SHFILEINFO sfi;

						FILE* file = fopen(path, "rb");
						if (file) {
							int l = fread(b, 1, 0x10000, file);
							fclose(file);

							if (l)
								icon_id = g_Globals._icon_cache.add(CreateIconFromResourceEx(b, l, TRUE, 0x00030000, 16, 16, LR_DEFAULTCOLOR));
						}
					} */
				}
			}
		}

		if (icon_id == ICID_NONE) {
			const ShellPath& pidl_abs = create_absolute_pidl();
			LPCITEMIDLIST pidl = pidl_abs;

			icon_id = g_Globals._icon_cache.extract(pidl, flags);
		}
	}

	return icon_id;
}

int Entry::safe_extract_icon(ICONCACHE_FLAGS flags)
{
	try {
		return extract_icon(flags);
	} catch(COMException&) {
		// ignore unexpected exceptions while extracting icons
	}

	return ICID_NONE;
}


BOOL Entry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	TCHAR cmd[MAX_PATH];

	if (!get_path(cmd, COUNTOF(cmd)))
		return FALSE;

	 // add path to the recent file list
	SHAddToRecentDocs(SHARD_PATH, cmd);

	  // start program, open document...
	return launch_file(hwnd, cmd, nCmdShow);
}


 // local replacement implementation for SHBindToParent()
 // (derived from https://web.archive.org/web/20021105062620/http://www.geocities.com/SiliconValley/2060/articles/shell-helpers.html)
static HRESULT my_SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, VOID** ppv, LPCITEMIDLIST* ppidlLast)
{
	HRESULT hr;

	if (!ppv)
		return E_POINTER;

	// There must be at least one item ID.
	if (!pidl || !pidl->mkid.cb)
		return E_INVALIDARG;

	 // Get the desktop folder as root.
	ShellFolder desktop;
/*	IShellFolderPtr desktop;
	hr = SHGetDesktopFolder(&desktop);
	if (FAILED(hr))
		return hr; */

	// Walk to the penultimate item ID.
	LPCITEMIDLIST marker = pidl;
	for (;;)
	{
		LPCITEMIDLIST next = reinterpret_cast<LPCITEMIDLIST>(
			marker->mkid.abID - sizeof(marker->mkid.cb) + marker->mkid.cb);
		if (!next->mkid.cb)
			break;
		marker = next;
	}

	if (marker == pidl)
	{
		// There was only a single item ID, so bind to the root folder.
		hr = desktop->QueryInterface(riid, ppv);
	}
	else
	{
		// Copy the ID list, truncating the last item.
		int length = marker->mkid.abID - pidl->mkid.abID;
		if (LPITEMIDLIST parent_id = reinterpret_cast<LPITEMIDLIST>(
			malloc(length + sizeof(pidl->mkid.cb))))
		{
			LPBYTE raw_data = reinterpret_cast<LPBYTE>(parent_id);
			memcpy(raw_data, pidl, length);
			memset(raw_data + length, 0, sizeof(pidl->mkid.cb));
			hr = desktop->BindToObject(parent_id, 0, riid, ppv);
			free(parent_id);
		}
		else
			return E_OUTOFMEMORY;
	}

	// Return a pointer to the last item ID.
	if (ppidlLast)
		*ppidlLast = marker;

	return hr;
}
#define USE_MY_SHBINDTOPARENT

HRESULT Entry::do_context_menu(HWND hwnd, const POINT& pos, CtxMenuInterfaces& cm_ifs)
{
	ShellPath shell_path = create_absolute_pidl();
	LPCITEMIDLIST pidl_abs = shell_path;

	if (!pidl_abs)
		return S_FALSE;	// no action for registry entries, etc.

#ifdef USE_MY_SHBINDTOPARENT
	IShellFolder* parentFolder;
	LPCITEMIDLIST pidlLast;

	 // get and use the parent folder to display correct context menu in all cases -> correct "Properties" dialog for directories, ...
	HRESULT hr = my_SHBindToParent(pidl_abs, IID_IShellFolder, (LPVOID*)&parentFolder, &pidlLast);

	if (SUCCEEDED(hr)) {
		hr = ShellFolderContextMenu(parentFolder, hwnd, 1, &pidlLast, pos.x, pos.y, cm_ifs);

		parentFolder->Release();
	}

	return hr;
#else
	static DynamicFct<HRESULT(WINAPI*)(LPCITEMIDLIST, REFIID, LPVOID*, LPCITEMIDLIST*)> SHBindToParent(TEXT("SHELL32"), "SHBindToParent");

	if (SHBindToParent) {
		IShellFolder* parentFolder;
		LPCITEMIDLIST pidlLast;

		 // get and use the parent folder to display correct context menu in all cases -> correct "Properties" dialog for directories, ...
		HRESULT hr = (*SHBindToParent)(pidl_abs, IID_IShellFolder, (LPVOID*)&parentFolder, &pidlLast);

		if (SUCCEEDED(hr)) {
			hr = ShellFolderContextMenu(parentFolder, hwnd, 1, &pidlLast, pos.x, pos.y, cm_ifs);

			parentFolder->Release();
		}

		return hr;
	} else {
		/**@todo use parent folder instead of desktop folder
		Entry* dir = _up;

		ShellPath parent_path;

		if (dir)
			parent_path = dir->create_absolute_pidl();
		else
			parent_path = DesktopFolderPath();

		ShellPath shell_path = create_relative_pidl(parent_path);
		LPCITEMIDLIST pidl = shell_path;

		ShellFolder parent_folder = parent_path;
		return ShellFolderContextMenu(parent_folder, hwnd, 1, &pidl, pos.x, pos.y);
		*/
		return ShellFolderContextMenu(GetDesktopFolder(), hwnd, 1, &pidl_abs, pos.x, pos.y, cm_ifs);
	}
#endif
}


HRESULT Entry::GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut)
{
	TCHAR path[MAX_PATH];
/*
	if (!get_path(path, COUNTOF(path)))
		return E_FAIL;

	ShellPath shell_path(path);

	IShellFolder* pFolder;
	LPCITEMIDLIST pidl_last = NULL;

	static DynamicFct<HRESULT(WINAPI*)(LPCITEMIDLIST, REFIID, LPVOID*, LPCITEMIDLIST*)> SHBindToParent(TEXT("SHELL32"), "SHBindToParent");

	if (!SHBindToParent)
		return E_NOTIMPL;

	HRESULT hr = (*SHBindToParent)(shell_path, IID_IShellFolder, (LPVOID*)&pFolder, &pidl_last);
	if (FAILED(hr))
		return hr;

	ShellFolder shell_folder(pFolder);

	shell_folder->Release();

	return shell_folder->GetUIObjectOf(hWnd, 1, &pidl_last, riid, NULL, ppvOut);
*/
	if (!_up)
		return E_INVALIDARG;

	if (!_up->get_path(path, COUNTOF(path)))
		return E_FAIL;

	ShellPath shell_path(path);
	ShellFolder shell_folder(shell_path);

#ifdef UNICODE
	LPWSTR wname = _data.cFileName;
#else
	WCHAR wname[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, _data.cFileName, -1, wname, COUNTOF(wname));
#endif

	LPITEMIDLIST pidl_last = NULL;
	HRESULT hr = shell_folder->ParseDisplayName(hWnd, NULL, wname, NULL, &pidl_last, NULL);

	if (FAILED(hr))
		return hr;

	hr = shell_folder->GetUIObjectOf(hWnd, 1, (LPCITEMIDLIST*)&pidl_last, riid, NULL, ppvOut);

	ShellMalloc()->Free((void*)pidl_last);

	return hr;
}


 // get full path of specified directory entry
bool Entry::get_path_base ( PTSTR path, size_t path_count, ENTRY_TYPE etype ) const
{
	int level = 0;
	size_t len = 0;
	size_t l = 0;
	LPCTSTR name = NULL;
	TCHAR buffer[MAX_PATH];

	if (!path || path_count==0)
		return false;

	const Entry* entry;
	if ( path_count > 1 )
	{
		for(entry=this; entry; level++) {
			l = 0;

			if (entry->_etype == etype) {
				name = entry->_data.cFileName;

				for(LPCTSTR s=name; *s && *s!=TEXT('/') && *s!=TEXT('\\'); s++)
					++l;

				if (!entry->_up)
					break;
			} else {
				if (entry->get_path(buffer, COUNTOF(buffer))) {
					l = _tcslen(buffer);
					name = buffer;

					/* special handling of drive names */
					if (l>0 && buffer[l-1]=='\\' && path[0]=='\\')
						--l;

					if ( len+l >= path_count )
					{
						if ( l + 1 > path_count )
							len = 0;
						else
							len = path_count - l - 1;
					}
					memmove(path+l, path, len*sizeof(TCHAR));
					if ( l+1 >= path_count )
						l = path_count - 1;
					memcpy(path, name, l*sizeof(TCHAR));
					len += l;
				}

				entry = NULL;
				break;
			}

			if (l > 0) {
				if ( len+l+1 >= path_count )
				{
					/* compare to 2 here because of terminator plus the '\\' we prepend */
					if ( l + 2 > path_count )
						len = 0;
					else
						len = path_count - l - 2;
				}
				memmove(path+l+1, path, len*sizeof(TCHAR));
				/* compare to 2 here because of terminator plus the '\\' we prepend */
				if ( l+2 >= path_count )
					l = path_count - 2;
				memcpy(path+1, name, l*sizeof(TCHAR));
				len += l+1;

#ifndef _NO_WIN_FS
				if (etype == ET_WINDOWS && entry->_up && !(entry->_up->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))	// a NTFS stream?
					path[0] = TEXT(':');
				else
#endif
					path[0] = TEXT('\\');
			}

			entry = entry->_up;
		}

		if (entry) {
			if ( len+l >= path_count )
			{
				if ( l + 1 > path_count )
					len = 0;
				else
					len = path_count - l - 1;
			}
			memmove(path+l, path, len*sizeof(TCHAR));
			if ( l+1 >= path_count )
				l = path_count - 1;
			memcpy(path, name, l*sizeof(TCHAR));
			len += l;
		}

		if ( !level && (len+1 < path_count) )
			path[len++] = TEXT('\\');
	}

	path[len] = TEXT('\0');

	return true;
}

 // recursively free all child entries
void Entry::free_subentries()
{
	Entry *entry, *next=_down;

	if (next) {
		_down = 0;

		do {
			entry = next;
			next = entry->_next;

			entry->free_subentries();
			delete entry;
		} while(next);
	}
}


Entry* Root::read_tree(LPCTSTR path, int scan_flags)
{
	Entry* entry;

	if (path && *path)
		entry = _entry->read_tree(path, _sort_order);
	else {
		entry = _entry->read_tree(NULL, _sort_order);

		_entry->smart_scan();

		if (_entry->_down)
			_entry->_expanded = true;
	}

	return entry;
}


Entry* Root::read_tree(LPCITEMIDLIST pidl, int scan_flags)
{
	return _entry->read_tree(pidl, _sort_order);
}
