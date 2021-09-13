/*
 * Winefile
 *
 * Copyright 2000, 2003, 2004, 2005 Martin Fuchs
 * Copyright 2006 Jason Green
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef __WINE__
#include "config.h"
#include "wine/port.h"

/* for unix filesystem function calls */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#define COBJMACROS

#include "winefile.h"
#include "resource.h"
#include "wine/unicode.h"

#ifndef _MAX_PATH
#define _MAX_DRIVE          3
#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME
#define _MAX_PATH           260
#endif

#ifdef NONAMELESSUNION
#define	UNION_MEMBER(x) DUMMYUNIONNAME.x
#else
#define	UNION_MEMBER(x) x
#endif

#define	DEFAULT_SPLIT_POS	300

static const WCHAR registry_key[] = { 'S','o','f','t','w','a','r','e','\\',
                                      'W','i','n','e','\\',
                                      'W','i','n','e','F','i','l','e','\0'};
static const WCHAR reg_start_x[] = { 's','t','a','r','t','X','\0'};
static const WCHAR reg_start_y[] = { 's','t','a','r','t','Y','\0'};
static const WCHAR reg_width[] = { 'w','i','d','t','h','\0'};
static const WCHAR reg_height[] = { 'h','e','i','g','h','t','\0'};
static const WCHAR reg_logfont[] = { 'l','o','g','f','o','n','t','\0'};

enum ENTRY_TYPE {
	ET_WINDOWS,
	ET_UNIX,
	ET_SHELL
};

typedef struct _Entry {
	struct _Entry*	next;
	struct _Entry*	down;
	struct _Entry*	up;

	BOOL			expanded;
	BOOL			scanned;
	int				level;

	WIN32_FIND_DATAW	data;

	BY_HANDLE_FILE_INFORMATION bhfi;
	BOOL			bhfi_valid;
	enum ENTRY_TYPE	etype;
	LPITEMIDLIST	pidl;
	IShellFolder*	folder;
	HICON			hicon;
} Entry;

typedef struct {
	Entry	entry;
	WCHAR	path[MAX_PATH];
	WCHAR	volname[_MAX_FNAME];
	WCHAR	fs[_MAX_DIR];
	DWORD	drive_type;
	DWORD	fs_flags;
} Root;

enum COLUMN_FLAGS {
	COL_SIZE		= 0x01,
	COL_DATE		= 0x02,
	COL_TIME		= 0x04,
	COL_ATTRIBUTES	= 0x08,
	COL_DOSNAMES	= 0x10,
	COL_INDEX		= 0x20,
	COL_LINKS		= 0x40,
	COL_ALL = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES|COL_INDEX|COL_LINKS
};

typedef enum {
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
} SORT_ORDER;

typedef struct {
	HWND	hwnd;
	HWND	hwndHeader;

#define	COLUMNS	10
	int		widths[COLUMNS];
	int		positions[COLUMNS+1];

	BOOL	treePane;
	int		visible_cols;
	Entry*	root;
	Entry*	cur;
} Pane;

typedef struct {
	HWND	hwnd;
	Pane	left;
	Pane	right;
	int		focus_pane;		/* 0: left  1: right */
	WINDOWPLACEMENT pos;
	int		split_pos;
	BOOL	header_wdths_ok;

	WCHAR	path[MAX_PATH];
	WCHAR	filter_pattern[MAX_PATH];
	int		filter_flags;
	Root	root;

	SORT_ORDER sortOrder;
} ChildWnd;



static void read_directory(Entry* dir, LPCWSTR path, SORT_ORDER sortOrder, HWND hwnd);
static void set_curdir(ChildWnd* child, Entry* entry, int idx, HWND hwnd);
static void refresh_child(ChildWnd* child);
static void refresh_drives(void);
static void get_path(Entry* dir, PWSTR path);
static void format_date(const FILETIME* ft, WCHAR* buffer, int visible_cols);

static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK TreeWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);


/* globals */
WINEFILE_GLOBALS Globals;

static int last_split;

/* some common string constants */
static const WCHAR sEmpty[] = {'\0'};
static const WCHAR sSpace[] = {' ', '\0'};
static const WCHAR sNumFmt[] = {'%','d','\0'};
static const WCHAR sQMarks[] = {'?','?','?','\0'};

/* window class names */
static const WCHAR sWINEFILEFRAME[] = {'W','F','S','_','F','r','a','m','e','\0'};
static const WCHAR sWINEFILETREE[] = {'W','F','S','_','T','r','e','e','\0'};

static void format_longlong(LPWSTR ret, ULONGLONG val)
{
    WCHAR buffer[65], *p = &buffer[64];

    *p = 0;
    do {
        *(--p) = '0' + val % 10;
	val /= 10;
    } while (val);
    lstrcpyW( ret, p );
}


/* load resource string */
static LPWSTR load_string(LPWSTR buffer, DWORD size, UINT id)
{
	LoadStringW(Globals.hInstance, id, buffer, size);
	return buffer;
}

#define RS(b, i) load_string(b, sizeof(b)/sizeof(b[0]), i)


/* display error message for the specified WIN32 error code */
static void display_error(HWND hwnd, DWORD error)
{
	WCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];
	PWSTR msg;

	if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PWSTR)&msg, 0, NULL))
		MessageBoxW(hwnd, msg, RS(b2,IDS_WINEFILE), MB_OK);
	else
		MessageBoxW(hwnd, RS(b1,IDS_ERROR), RS(b2,IDS_WINEFILE), MB_OK);

	LocalFree(msg);
}


/* display network error message using WNetGetLastErrorW() */
static void display_network_error(HWND hwnd)
{
	WCHAR msg[BUFFER_LEN], provider[BUFFER_LEN], b2[BUFFER_LEN];
	DWORD error;

	if (WNetGetLastErrorW(&error, msg, BUFFER_LEN, provider, BUFFER_LEN) == NO_ERROR)
		MessageBoxW(hwnd, msg, RS(b2,IDS_WINEFILE), MB_OK);
}

static inline BOOL get_check(HWND hwnd, INT id)
{
	return BST_CHECKED&SendMessageW(GetDlgItem(hwnd, id), BM_GETSTATE, 0, 0);
}

static inline INT set_check(HWND hwnd, INT id, BOOL on)
{
	return SendMessageW(GetDlgItem(hwnd, id), BM_SETCHECK, on?BST_CHECKED:BST_UNCHECKED, 0);
}

static inline void choose_font(HWND hwnd)
{
        WCHAR dlg_name[BUFFER_LEN], dlg_info[BUFFER_LEN];
        CHOOSEFONTW chFont;
        LOGFONTW lFont;

        HDC hdc = GetDC(hwnd);

        GetObjectW(Globals.hfont, sizeof(LOGFONTW), &lFont);

        chFont.lStructSize = sizeof(CHOOSEFONTW);
        chFont.hwndOwner = hwnd;
        chFont.hDC = NULL;
        chFont.lpLogFont = &lFont;
        chFont.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_LIMITSIZE | CF_NOSCRIPTSEL | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;
        chFont.rgbColors = RGB(0,0,0);
        chFont.lCustData = 0;
        chFont.lpfnHook = NULL;
        chFont.lpTemplateName = NULL;
        chFont.hInstance = Globals.hInstance;
        chFont.lpszStyle = NULL;
        chFont.nFontType = SIMULATED_FONTTYPE;
        chFont.nSizeMin = 0;
        chFont.nSizeMax = 24;

        if (ChooseFontW(&chFont)) {
                HWND childWnd;
                HFONT hFontOld;

                DeleteObject(Globals.hfont);
                Globals.hfont = CreateFontIndirectW(&lFont);
                hFontOld = SelectObject(hdc, Globals.hfont);
                GetTextExtentPoint32W(hdc, sSpace, 1, &Globals.spaceSize);

                /* change font in all open child windows */
                for(childWnd=GetWindow(Globals.hmdiclient,GW_CHILD); childWnd; childWnd=GetNextWindow(childWnd,GW_HWNDNEXT)) {
                        ChildWnd* child = (ChildWnd*) GetWindowLongPtrW(childWnd, GWLP_USERDATA);
                        SendMessageW(child->left.hwnd, WM_SETFONT, (WPARAM)Globals.hfont, TRUE);
                        SendMessageW(child->right.hwnd, WM_SETFONT, (WPARAM)Globals.hfont, TRUE);
                        SendMessageW(child->left.hwnd, LB_SETITEMHEIGHT, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
                        SendMessageW(child->right.hwnd, LB_SETITEMHEIGHT, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
                        InvalidateRect(child->left.hwnd, NULL, TRUE);
                        InvalidateRect(child->right.hwnd, NULL, TRUE);
                }

                SelectObject(hdc, hFontOld);
        }
        else if (CommDlgExtendedError()) {
                LoadStringW(Globals.hInstance, IDS_FONT_SEL_DLG_NAME, dlg_name, BUFFER_LEN);
                LoadStringW(Globals.hInstance, IDS_FONT_SEL_ERROR, dlg_info, BUFFER_LEN);
                MessageBoxW(hwnd, dlg_info, dlg_name, MB_OK);
        }

        ReleaseDC(hwnd, hdc);
}


/* allocate and initialise a directory entry */
static Entry* alloc_entry(void)
{
	Entry* entry = HeapAlloc(GetProcessHeap(), 0, sizeof(Entry));

	entry->pidl = NULL;
	entry->folder = NULL;
	entry->hicon = 0;

	return entry;
}

/* free a directory entry */
static void free_entry(Entry* entry)
{
	if (entry->hicon && entry->hicon!=(HICON)-1)
		DestroyIcon(entry->hicon);

	if (entry->folder && entry->folder!=Globals.iDesktop)
		IShellFolder_Release(entry->folder);

	if (entry->pidl)
		IMalloc_Free(Globals.iMalloc, entry->pidl);

	HeapFree(GetProcessHeap(), 0, entry);
}

/* recursively free all child entries */
static void free_entries(Entry* dir)
{
	Entry *entry, *next=dir->down;

	if (next) {
		dir->down = 0;

		do {
			entry = next;
			next = entry->next;

			free_entries(entry);
			free_entry(entry);
		} while(next);
	}
}


static void read_directory_win(Entry* dir, LPCWSTR path)
{
	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;

	int level = dir->level + 1;
	WIN32_FIND_DATAW w32fd;
	HANDLE hFind;
	HANDLE hFile;

	WCHAR buffer[MAX_PATH], *p;
	for(p=buffer; *path; )
		*p++ = *path++;

	*p++ = '\\';
	p[0] = '*';
	p[1] = '\0';

	hFind = FindFirstFileW(buffer, &w32fd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			entry = alloc_entry();

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->next = entry;

			memcpy(&entry->data, &w32fd, sizeof(WIN32_FIND_DATAW));
			entry->down = NULL;
			entry->up = dir;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;
			entry->etype = ET_WINDOWS;
			entry->bhfi_valid = FALSE;

			lstrcpyW(p, entry->data.cFileName);

			hFile = CreateFileW(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

			if (hFile != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(hFile, &entry->bhfi))
					entry->bhfi_valid = TRUE;

				CloseHandle(hFile);
			}

			last = entry;
		} while(FindNextFileW(hFind, &w32fd));

		if (last)
			last->next = NULL;

		FindClose(hFind);
	}

	dir->down = first_entry;
	dir->scanned = TRUE;
}


static Entry* find_entry_win(Entry* dir, LPCWSTR name)
{
	Entry* entry;

	for(entry=dir->down; entry; entry=entry->next) {
		LPCWSTR p = name;
		LPCWSTR q = entry->data.cFileName;

		do {
			if (!*p || *p == '\\' || *p == '/')
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->data.cAlternateFileName;

		do {
			if (!*p || *p == '\\' || *p == '/')
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return 0;
}


static Entry* read_tree_win(Root* root, LPCWSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	WCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCWSTR s = path;
	PWSTR d = buffer;

	HCURSOR old_cursor = SetCursor(LoadCursorW(0, (LPCWSTR)IDC_WAIT));

	entry->etype = ET_WINDOWS;
	while(entry) {
		while(*s && *s != '\\' && *s != '/')
			*d++ = *s++;

		while(*s == '\\' || *s == '/')
			s++;

		*d++ = '\\';
		*d = '\0';

		read_directory(entry, buffer, sortOrder, hwnd);

		if (entry->down)
			entry->expanded = TRUE;

		if (!*s)
			break;

		entry = find_entry_win(entry, s);
	}

	SetCursor(old_cursor);

	return entry;
}


#ifdef __WINE__

static BOOL time_to_filetime(time_t t, FILETIME* ftime)
{
	struct tm* tm = gmtime(&t);
	SYSTEMTIME stime;

	if (!tm)
		return FALSE;

	stime.wYear = tm->tm_year+1900;
	stime.wMonth = tm->tm_mon+1;
	/*	stime.wDayOfWeek */
	stime.wDay = tm->tm_mday;
	stime.wHour = tm->tm_hour;
	stime.wMinute = tm->tm_min;
	stime.wSecond = tm->tm_sec;
	stime.wMilliseconds = 0;

	return SystemTimeToFileTime(&stime, ftime);
}

static void read_directory_unix(Entry* dir, LPCWSTR path)
{
	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;
	DIR* pdir;

	int level = dir->level + 1;
	char cpath[MAX_PATH];

	WideCharToMultiByte(CP_UNIXCP, 0, path, -1, cpath, MAX_PATH, NULL, NULL);
	pdir = opendir(cpath);

	if (pdir) {
		struct stat st;
		struct dirent* ent;
		char buffer[MAX_PATH], *p;
		const char* s;

		for(p=buffer,s=cpath; *s; )
			*p++ = *s++;

		if (p==buffer || p[-1]!='/')
			*p++ = '/';

		while((ent=readdir(pdir))) {
			entry = alloc_entry();

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->next = entry;

			entry->etype = ET_UNIX;

			strcpy(p, ent->d_name);
			MultiByteToWideChar(CP_UNIXCP, 0, p, -1, entry->data.cFileName, MAX_PATH);

			if (!stat(buffer, &st)) {
				entry->data.dwFileAttributes = p[0]=='.'? FILE_ATTRIBUTE_HIDDEN: 0;

				if (S_ISDIR(st.st_mode))
					entry->data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				entry->data.nFileSizeLow = st.st_size & 0xFFFFFFFF;
				entry->data.nFileSizeHigh = st.st_size >> 32;

				memset(&entry->data.ftCreationTime, 0, sizeof(FILETIME));
				time_to_filetime(st.st_atime, &entry->data.ftLastAccessTime);
				time_to_filetime(st.st_mtime, &entry->data.ftLastWriteTime);

				entry->bhfi.nFileIndexLow = ent->d_ino;
				entry->bhfi.nFileIndexHigh = 0;

				entry->bhfi.nNumberOfLinks = st.st_nlink;

				entry->bhfi_valid = TRUE;
			} else {
				entry->data.nFileSizeLow = 0;
				entry->data.nFileSizeHigh = 0;
				entry->bhfi_valid = FALSE;
			}

			entry->down = NULL;
			entry->up = dir;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;

			last = entry;
		}

		if (last)
			last->next = NULL;

		closedir(pdir);
	}

	dir->down = first_entry;
	dir->scanned = TRUE;
}

static Entry* find_entry_unix(Entry* dir, LPCWSTR name)
{
	Entry* entry;

	for(entry=dir->down; entry; entry=entry->next) {
		LPCWSTR p = name;
		LPCWSTR q = entry->data.cFileName;

		do {
			if (!*p || *p == '/')
				return entry;
		} while(*p++ == *q++);
	}

	return 0;
}

static Entry* read_tree_unix(Root* root, LPCWSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	WCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCWSTR s = path;
	PWSTR d = buffer;

	HCURSOR old_cursor = SetCursor(LoadCursorW(0, (LPCWSTR)IDC_WAIT));

	entry->etype = ET_UNIX;

	while(entry) {
		while(*s && *s != '/')
			*d++ = *s++;

		while(*s == '/')
			s++;

		*d++ = '/';
		*d = '\0';

		read_directory(entry, buffer, sortOrder, hwnd);

		if (entry->down)
			entry->expanded = TRUE;

		if (!*s)
			break;

		entry = find_entry_unix(entry, s);
	}

	SetCursor(old_cursor);

	return entry;
}

#endif /* __WINE__ */

static void free_strret(STRRET* str)
{
	if (str->uType == STRRET_WSTR)
		IMalloc_Free(Globals.iMalloc, str->UNION_MEMBER(pOleStr));
}

static LPWSTR wcscpyn(LPWSTR dest, LPCWSTR source, size_t count)
{
 LPCWSTR s;
 LPWSTR d = dest;

 for(s=source; count&&(*d++=*s++); )
  count--;

 return dest;
}

static void get_strretW(STRRET* str, const SHITEMID* shiid, LPWSTR buffer, int len)
{
 switch(str->uType) {
  case STRRET_WSTR:
	wcscpyn(buffer, str->UNION_MEMBER(pOleStr), len);
	break;

  case STRRET_OFFSET:
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)shiid+str->UNION_MEMBER(uOffset), -1, buffer, len);
	break;

  case STRRET_CSTR:
	MultiByteToWideChar(CP_ACP, 0, str->UNION_MEMBER(cStr), -1, buffer, len);
 }
}


static HRESULT name_from_pidl(IShellFolder* folder, LPITEMIDLIST pidl, LPWSTR buffer, int len, SHGDNF flags)
{
	STRRET str;

	HRESULT hr = IShellFolder_GetDisplayNameOf(folder, pidl, flags, &str);

	if (SUCCEEDED(hr)) {
		get_strretW(&str, &pidl->mkid, buffer, len);
		free_strret(&str);
	} else
		buffer[0] = '\0';

	return hr;
}


static HRESULT path_from_pidlW(IShellFolder* folder, LPITEMIDLIST pidl, LPWSTR buffer, int len)
{
	STRRET str;

	 /* SHGDN_FORPARSING: get full path of id list */
	HRESULT hr = IShellFolder_GetDisplayNameOf(folder, pidl, SHGDN_FORPARSING, &str);

	if (SUCCEEDED(hr)) {
		get_strretW(&str, &pidl->mkid, buffer, len);
		free_strret(&str);
	} else
		buffer[0] = '\0';

	return hr;
}


 /* create an item id list from a file system path */

static LPITEMIDLIST get_path_pidl(LPWSTR path, HWND hwnd)
{
	LPITEMIDLIST pidl;
	HRESULT hr;
	ULONG len;
	LPWSTR buffer = path;

	hr = IShellFolder_ParseDisplayName(Globals.iDesktop, hwnd, NULL, buffer, &len, &pidl, NULL);
	if (FAILED(hr))
		return NULL;

	return pidl;
}


 /* convert an item id list from relative to absolute (=relative to the desktop) format */

static LPITEMIDLIST get_to_absolute_pidl(Entry* entry, HWND hwnd)
{
	if (entry->up && entry->up->etype==ET_SHELL) {
		LPITEMIDLIST idl = NULL;

		while (entry->up) {
			idl = ILCombine(ILClone(entry->pidl), idl);
			entry = entry->up;
		}

		return idl;
	} else if (entry->etype == ET_WINDOWS) {
		WCHAR path[MAX_PATH];

		get_path(entry, path);

		return get_path_pidl(path, hwnd);
	} else if (entry->pidl)
		return ILClone(entry->pidl);

	return NULL;
}


static HICON extract_icon(IShellFolder* folder, LPCITEMIDLIST pidl)
{
	IExtractIconW* pExtract;

	if (SUCCEEDED(IShellFolder_GetUIObjectOf(folder, 0, 1, (LPCITEMIDLIST*)&pidl, &IID_IExtractIconW, 0, (LPVOID*)&pExtract))) {
		WCHAR path[_MAX_PATH];
		unsigned flags;
		HICON hicon;
		int idx;

		if (SUCCEEDED(IExtractIconW_GetIconLocation(pExtract, GIL_FORSHELL, path, _MAX_PATH, &idx, &flags))) {
			if (!(flags & GIL_NOTFILENAME)) {
				if (idx == -1)
					idx = 0;	/* special case for some control panel applications */

				if ((int)ExtractIconExW(path, idx, 0, &hicon, 1) > 0)
					flags &= ~GIL_DONTCACHE;
			} else {
				HICON hIconLarge = 0;

				HRESULT hr = IExtractIconW_Extract(pExtract, path, idx, &hIconLarge, &hicon, MAKELONG(0/*GetSystemMetrics(SM_CXICON)*/,GetSystemMetrics(SM_CXSMICON)));

				if (SUCCEEDED(hr))
					DestroyIcon(hIconLarge);
			}

			return hicon;
		}
	}

	return 0;
}


static Entry* find_entry_shell(Entry* dir, LPCITEMIDLIST pidl)
{
	Entry* entry;

	for(entry=dir->down; entry; entry=entry->next) {
		if (entry->pidl->mkid.cb == pidl->mkid.cb &&
			!memcmp(entry->pidl, pidl, entry->pidl->mkid.cb))
			return entry;
	}

	return 0;
}

static Entry* read_tree_shell(Root* root, LPITEMIDLIST pidl, SORT_ORDER sortOrder, HWND hwnd)
{
	Entry* entry = &root->entry;
	Entry* next;
	LPITEMIDLIST next_pidl = pidl;
	IShellFolder* folder;
	IShellFolder* child = NULL;
	HRESULT hr;

	HCURSOR old_cursor = SetCursor(LoadCursorW(0, (LPCWSTR)IDC_WAIT));

	entry->etype = ET_SHELL;
	folder = Globals.iDesktop;

	while(entry) {
		entry->pidl = next_pidl;
		entry->folder = folder;

		if (!pidl->mkid.cb)
			break;

		 /* copy first element of item idlist */
		next_pidl = IMalloc_Alloc(Globals.iMalloc, pidl->mkid.cb+sizeof(USHORT));
		memcpy(next_pidl, pidl, pidl->mkid.cb);
		((LPITEMIDLIST)((LPBYTE)next_pidl+pidl->mkid.cb))->mkid.cb = 0;

		hr = IShellFolder_BindToObject(folder, next_pidl, 0, &IID_IShellFolder, (void**)&child);
		if (FAILED(hr))
			break;

		read_directory(entry, NULL, sortOrder, hwnd);

		if (entry->down)
			entry->expanded = TRUE;

		next = find_entry_shell(entry, next_pidl);
		if (!next)
			break;

		folder = child;
		entry = next;

		 /* go to next element */
		pidl = (LPITEMIDLIST) ((LPBYTE)pidl+pidl->mkid.cb);
	}

	SetCursor(old_cursor);

	return entry;
}


static void fill_w32fdata_shell(IShellFolder* folder, LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATAW* w32fdata)
{
	if (!(attribs & SFGAO_FILESYSTEM) ||
			FAILED(SHGetDataFromIDListW(folder, pidl, SHGDFIL_FINDDATA, w32fdata, sizeof(WIN32_FIND_DATAW)))) {
		WIN32_FILE_ATTRIBUTE_DATA fad;
		IDataObject* pDataObj;

		STGMEDIUM medium = {0, {0}, 0};
		FORMATETC fmt = {Globals.cfStrFName, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		HRESULT hr = IShellFolder_GetUIObjectOf(folder, 0, 1, &pidl, &IID_IDataObject, 0, (LPVOID*)&pDataObj);

		if (SUCCEEDED(hr)) {
			hr = IDataObject_GetData(pDataObj, &fmt, &medium);

			IDataObject_Release(pDataObj);

			if (SUCCEEDED(hr)) {
				LPCWSTR path = GlobalLock(medium.UNION_MEMBER(hGlobal));
				UINT sem_org = SetErrorMode(SEM_FAILCRITICALERRORS);

				if (GetFileAttributesExW(path, GetFileExInfoStandard, &fad)) {
					w32fdata->dwFileAttributes = fad.dwFileAttributes;
					w32fdata->ftCreationTime = fad.ftCreationTime;
					w32fdata->ftLastAccessTime = fad.ftLastAccessTime;
					w32fdata->ftLastWriteTime = fad.ftLastWriteTime;

					if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						w32fdata->nFileSizeLow = fad.nFileSizeLow;
						w32fdata->nFileSizeHigh = fad.nFileSizeHigh;
					}
				}

				SetErrorMode(sem_org);

				GlobalUnlock(medium.UNION_MEMBER(hGlobal));
				GlobalFree(medium.UNION_MEMBER(hGlobal));
			}
		}
	}

	if (attribs & (SFGAO_FOLDER|SFGAO_HASSUBFOLDER))
		w32fdata->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (attribs & SFGAO_READONLY)
		w32fdata->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

	if (attribs & SFGAO_COMPRESSED)
		w32fdata->dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
}


static void read_directory_shell(Entry* dir, HWND hwnd)
{
	IShellFolder* folder = dir->folder;
	int level = dir->level + 1;
	HRESULT hr;

	IShellFolder* child;
	IEnumIDList* idlist;

	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;

	if (!folder)
		return;

	hr = IShellFolder_EnumObjects(folder, hwnd, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN|SHCONTF_SHAREABLE|SHCONTF_STORAGE, &idlist);

	if (SUCCEEDED(hr)) {
		for(;;) {
#define	FETCH_ITEM_COUNT	32
			LPITEMIDLIST pidls[FETCH_ITEM_COUNT];
			SFGAOF attribs;
			ULONG cnt = 0;
			ULONG n;

			memset(pidls, 0, sizeof(pidls));

			hr = IEnumIDList_Next(idlist, FETCH_ITEM_COUNT, pidls, &cnt);
			if (FAILED(hr))
				break;

			if (hr == S_FALSE)
				break;

			for(n=0; n<cnt; ++n) {
				entry = alloc_entry();

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->next = entry;

				memset(&entry->data, 0, sizeof(WIN32_FIND_DATAW));
				entry->bhfi_valid = FALSE;

				attribs = ~SFGAO_FILESYSTEM;	/*SFGAO_HASSUBFOLDER|SFGAO_FOLDER; SFGAO_FILESYSTEM sorgt dafür, daß "My Documents" anstatt von "Martin's Documents" angezeigt wird */

				hr = IShellFolder_GetAttributesOf(folder, 1, (LPCITEMIDLIST*)&pidls[n], &attribs);

				if (SUCCEEDED(hr)) {
					if (attribs != (SFGAOF)~SFGAO_FILESYSTEM) {
						fill_w32fdata_shell(folder, pidls[n], attribs, &entry->data);

						entry->bhfi_valid = TRUE;
					} else
						attribs = 0;
				} else
					attribs = 0;

				entry->pidl = pidls[n];

				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					hr = IShellFolder_BindToObject(folder, pidls[n], 0, &IID_IShellFolder, (void**)&child);

					if (SUCCEEDED(hr))
						entry->folder = child;
					else
						entry->folder = NULL;
				}
				else
					entry->folder = NULL;

				if (!entry->data.cFileName[0])
					/*hr = */name_from_pidl(folder, pidls[n], entry->data.cFileName, MAX_PATH, /*SHGDN_INFOLDER*/0x2000/*0x2000=SHGDN_INCLUDE_NONFILESYS*/);

				 /* get display icons for files and virtual objects */
				if (!(entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
					!(attribs & SFGAO_FILESYSTEM)) {
					entry->hicon = extract_icon(folder, pidls[n]);

					if (!entry->hicon)
						entry->hicon = (HICON)-1;	/* don't try again later */
				}

				entry->down = NULL;
				entry->up = dir;
				entry->expanded = FALSE;
				entry->scanned = FALSE;
				entry->level = level;

				entry->etype = ET_SHELL;
				entry->bhfi_valid = FALSE;

				last = entry;
			}
		}

		IEnumIDList_Release(idlist);
	}

	if (last)
		last->next = NULL;

	dir->down = first_entry;
	dir->scanned = TRUE;
}

/* sort order for different directory/file types */
enum TYPE_ORDER {
	TO_DIR = 0,
	TO_DOT = 1,
	TO_DOTDOT = 2,
	TO_OTHER_DIR = 3,
	TO_FILE = 4
};

/* distinguish between ".", ".." and any other directory names */
static int TypeOrderFromDirname(LPCWSTR name)
{
	if (name[0] == '.') {
		if (name[1] == '\0')
			return TO_DOT;	/* "." */

		if (name[1]=='.' && name[2]=='\0')
			return TO_DOTDOT;	/* ".." */
	}

	return TO_OTHER_DIR;	/* anything else */
}

/* directories first... */
static int compareType(const WIN32_FIND_DATAW* fd1, const WIN32_FIND_DATAW* fd2)
{
	int order1 = fd1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY? TO_DIR: TO_FILE;
	int order2 = fd2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY? TO_DIR: TO_FILE;

	/* Handle "." and ".." as special case and move them at the very first beginning. */
	if (order1==TO_DIR && order2==TO_DIR) {
		order1 = TypeOrderFromDirname(fd1->cFileName);
		order2 = TypeOrderFromDirname(fd2->cFileName);
	}

	return order2==order1? 0: order1<order2? -1: 1;
}


static int compareName(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATAW* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATAW* fd2 = &(*(const Entry* const*)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return lstrcmpiW(fd1->cFileName, fd2->cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATAW* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATAW* fd2 = &(*(const Entry* const*)arg2)->data;
	const WCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	name1 = fd1->cFileName;
	name2 = fd2->cFileName;

	ext1 = strrchrW(name1, '.');
	ext2 = strrchrW(name2, '.');

	if (ext1)
		ext1++;
	else
		ext1 = sEmpty;

	if (ext2)
		ext2++;
	else
		ext2 = sEmpty;

	cmp = lstrcmpiW(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpiW(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATAW* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATAW* fd2 = &(*(const Entry* const*)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	cmp = fd2->nFileSizeHigh - fd1->nFileSizeHigh;

	if (cmp < 0)
		return -1;
	else if (cmp > 0)
		return 1;

	cmp = fd2->nFileSizeLow - fd1->nFileSizeLow;

	return cmp<0? -1: cmp>0? 1: 0;
}

static int compareDate(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATAW* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATAW* fd2 = &(*(const Entry* const*)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return CompareFileTime(&fd2->ftLastWriteTime, &fd1->ftLastWriteTime);
}


static int (*sortFunctions[])(const void* arg1, const void* arg2) = {
	compareName,	/* SORT_NAME */
	compareExt,		/* SORT_EXT */
	compareSize,	/* SORT_SIZE */
	compareDate		/* SORT_DATE */
};


static void SortDirectory(Entry* dir, SORT_ORDER sortOrder)
{
	Entry* entry;
	Entry** array, **p;
	int len;

	len = 0;
	for(entry=dir->down; entry; entry=entry->next)
		len++;

	if (len) {
		array = HeapAlloc(GetProcessHeap(), 0, len*sizeof(Entry*));

		p = array;
		for(entry=dir->down; entry; entry=entry->next)
			*p++ = entry;

		/* call qsort with the appropriate compare function */
		qsort(array, len, sizeof(array[0]), sortFunctions[sortOrder]);

		dir->down = array[0];

		for(p=array; --len; p++)
			p[0]->next = p[1];

		(*p)->next = 0;

                HeapFree(GetProcessHeap(), 0, array);
	}
}


static void read_directory(Entry* dir, LPCWSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	WCHAR buffer[MAX_PATH];
	Entry* entry;
	LPCWSTR s;
	PWSTR d;

	if (dir->etype == ET_SHELL)
	{
		read_directory_shell(dir, hwnd);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = '\\';

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					read_directory_shell(entry, hwnd);
					SortDirectory(entry, sortOrder);
				}
		}
	}
	else
#ifdef __WINE__
	if (dir->etype == ET_UNIX)
	{
		read_directory_unix(dir, path);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = '/';

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpyW(d, entry->data.cFileName);
					read_directory_unix(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
	}
	else
#endif
	{
		read_directory_win(dir, path);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = '\\';

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpyW(d, entry->data.cFileName);
					read_directory_win(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
	}

	SortDirectory(dir, sortOrder);
}


static Entry* read_tree(Root* root, LPCWSTR path, LPITEMIDLIST pidl, LPWSTR drv, SORT_ORDER sortOrder, HWND hwnd)
{
#ifdef __WINE__
	static const WCHAR sSlash[] = {'/', '\0'};
#endif
	static const WCHAR sBackslash[] = {'\\', '\0'};

	if (pidl)
	{
		 /* read shell namespace tree */
		root->drive_type = DRIVE_UNKNOWN;
		drv[0] = '\\';
		drv[1] = '\0';
		load_string(root->volname, sizeof(root->volname)/sizeof(root->volname[0]), IDS_DESKTOP);
		root->fs_flags = 0;
		load_string(root->fs, sizeof(root->fs)/sizeof(root->fs[0]), IDS_SHELL);

		return read_tree_shell(root, pidl, sortOrder, hwnd);
	}
	else
#ifdef __WINE__
	if (*path == '/')
	{
		/* read unix file system tree */
		root->drive_type = GetDriveTypeW(path);

		lstrcatW(drv, sSlash);
		load_string(root->volname, sizeof(root->volname)/sizeof(root->volname[0]), IDS_ROOT_FS);
		root->fs_flags = 0;
		load_string(root->fs, sizeof(root->fs)/sizeof(root->fs[0]), IDS_UNIXFS);

		lstrcpyW(root->path, sSlash);

		return read_tree_unix(root, path, sortOrder, hwnd);
	}
#endif

	 /* read WIN32 file system tree */
       root->drive_type = GetDriveTypeW(path);

	lstrcatW(drv, sBackslash);
	GetVolumeInformationW(drv, root->volname, _MAX_FNAME, 0, 0, &root->fs_flags, root->fs, _MAX_DIR);

	lstrcpyW(root->path, drv);

	return read_tree_win(root, path, sortOrder, hwnd);
}


/* flags to filter different file types */
enum TYPE_FILTER {
	TF_DIRECTORIES	= 0x01,
	TF_PROGRAMS		= 0x02,
	TF_DOCUMENTS	= 0x04,
	TF_OTHERS		= 0x08,
	TF_HIDDEN		= 0x10,
	TF_ALL			= 0x1F
};


static ChildWnd* alloc_child_window(LPCWSTR path, LPITEMIDLIST pidl, HWND hwnd)
{
	WCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
	WCHAR dir_path[MAX_PATH];
	static const WCHAR sAsterics[] = {'*', '\0'};
	static const WCHAR sTitleFmt[] = {'%','s',' ','-',' ','%','s','\0'};

	ChildWnd* child = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
	Root* root = &child->root;
	Entry* entry;

	memset(child, 0, sizeof(ChildWnd));

	child->left.treePane = TRUE;
	child->left.visible_cols = 0;

	child->right.treePane = FALSE;
	child->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_INDEX|COL_LINKS;

	child->pos.length = sizeof(WINDOWPLACEMENT);
	child->pos.flags = 0;
	child->pos.showCmd = SW_SHOWNORMAL;
	child->pos.rcNormalPosition.left = CW_USEDEFAULT;
	child->pos.rcNormalPosition.top = CW_USEDEFAULT;
	child->pos.rcNormalPosition.right = CW_USEDEFAULT;
	child->pos.rcNormalPosition.bottom = CW_USEDEFAULT;

	child->focus_pane = 0;
	child->split_pos = DEFAULT_SPLIT_POS;
	child->sortOrder = SORT_NAME;
	child->header_wdths_ok = FALSE;

	if (path)
	{
        int pathlen = strlenW(path);
        const WCHAR *npath = path;

        if (path[0] == '"' && path[pathlen - 1] == '"')
        {
            npath++;
            pathlen--;
        }
        lstrcpynW(child->path, npath, pathlen + 1);

        _wsplitpath(child->path, drv, dir, name, ext);
	}

	lstrcpyW(child->filter_pattern, sAsterics);
	child->filter_flags = TF_ALL;

	root->entry.level = 0;

	lstrcpyW(dir_path, drv);
	lstrcatW(dir_path, dir);
	entry = read_tree(root, dir_path, pidl, drv, child->sortOrder, hwnd);

	if (root->entry.etype == ET_SHELL)
		load_string(root->entry.data.cFileName, sizeof(root->entry.data.cFileName)/sizeof(root->entry.data.cFileName[0]), IDS_DESKTOP);
	else
		wsprintfW(root->entry.data.cFileName, sTitleFmt, drv, root->fs);

	root->entry.data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

	child->left.root = &root->entry;
	child->right.root = NULL;

	set_curdir(child, entry, 0, hwnd);

	return child;
}


/* free all memory associated with a child window */
static void free_child_window(ChildWnd* child)
{
	free_entries(&child->root.entry);
	HeapFree(GetProcessHeap(), 0, child);
}


/* get full path of specified directory entry */
static void get_path(Entry* dir, PWSTR path)
{
	Entry* entry;
	int len = 0;
	int level = 0;

	if (dir->etype == ET_SHELL)
	{
		SFGAOF attribs;
		HRESULT hr = S_OK;

		path[0] = '\0';

		attribs = 0;

		if (dir->folder)
			hr = IShellFolder_GetAttributesOf(dir->folder, 1, (LPCITEMIDLIST*)&dir->pidl, &attribs);

		if (SUCCEEDED(hr) && (attribs&SFGAO_FILESYSTEM)) {
			IShellFolder* parent = dir->up? dir->up->folder: Globals.iDesktop;

			hr = path_from_pidlW(parent, dir->pidl, path, MAX_PATH);
		}
	}
	else
	{
		for(entry=dir; entry; level++) {
			LPCWSTR name;
			int l;

			{
				LPCWSTR s;
				name = entry->data.cFileName;
				s = name;

				for(l=0; *s && *s != '/' && *s != '\\'; s++)
					l++;
			}

			if (entry->up) {
				if (l > 0) {
					memmove(path+l+1, path, len*sizeof(WCHAR));
					memcpy(path+1, name, l*sizeof(WCHAR));
					len += l+1;

					if (entry->etype == ET_UNIX)
						path[0] = '/';
					else
                                                path[0] = '\\';
				}

				entry = entry->up;
			} else {
				memmove(path+l, path, len*sizeof(WCHAR));
				memcpy(path, name, l*sizeof(WCHAR));
				len += l;
				break;
			}
		}

		if (!level) {
			if (entry->etype == ET_UNIX)
				path[len++] = '/';
			else
				path[len++] = '\\';
		}

		path[len] = '\0';
	}
}

static windowOptions load_registry_settings(void)
{
	DWORD size;
	DWORD type;
	HKEY hKey;
	windowOptions opts;
	LOGFONTW logfont;

        RegOpenKeyExW( HKEY_CURRENT_USER, registry_key,
                       0, KEY_QUERY_VALUE, &hKey );

	size = sizeof(DWORD);

        if( RegQueryValueExW( hKey, reg_start_x, NULL, &type,
                              (LPBYTE) &opts.start_x, &size ) != ERROR_SUCCESS )
		opts.start_x = CW_USEDEFAULT;

        if( RegQueryValueExW( hKey, reg_start_y, NULL, &type,
                              (LPBYTE) &opts.start_y, &size ) != ERROR_SUCCESS )
		opts.start_y = CW_USEDEFAULT;

        if( RegQueryValueExW( hKey, reg_width, NULL, &type,
                              (LPBYTE) &opts.width, &size ) != ERROR_SUCCESS )
		opts.width = CW_USEDEFAULT;

        if( RegQueryValueExW( hKey, reg_height, NULL, &type,
                              (LPBYTE) &opts.height, &size ) != ERROR_SUCCESS )
		opts.height = CW_USEDEFAULT;
	size=sizeof(logfont);
	if( RegQueryValueExW( hKey, reg_logfont, NULL, &type,
                              (LPBYTE) &logfont, &size ) != ERROR_SUCCESS )
		GetObjectW(GetStockObject(DEFAULT_GUI_FONT),sizeof(logfont),&logfont);

	RegCloseKey( hKey );

	Globals.hfont = CreateFontIndirectW(&logfont);
	return opts;
}

static void save_registry_settings(void)
{
	WINDOWINFO wi;
	HKEY hKey;
	INT width, height;
	LOGFONTW logfont;

	wi.cbSize = sizeof( WINDOWINFO );
	GetWindowInfo(Globals.hMainWnd, &wi);
	width = wi.rcWindow.right - wi.rcWindow.left;
	height = wi.rcWindow.bottom - wi.rcWindow.top;

	if ( RegOpenKeyExW( HKEY_CURRENT_USER, registry_key,
                            0, KEY_SET_VALUE, &hKey ) != ERROR_SUCCESS )
	{
		/* Unable to save registry settings - try to create key */
                if ( RegCreateKeyExW( HKEY_CURRENT_USER, registry_key,
                                      0, NULL, REG_OPTION_NON_VOLATILE,
                                      KEY_SET_VALUE, NULL, &hKey, NULL ) != ERROR_SUCCESS )
		{
			/* FIXME: Cannot create key */
			return;
		}
	}
	/* Save all of the settings */
        RegSetValueExW( hKey, reg_start_x, 0, REG_DWORD,
                        (LPBYTE) &wi.rcWindow.left, sizeof(DWORD) );
        RegSetValueExW( hKey, reg_start_y, 0, REG_DWORD,
                        (LPBYTE) &wi.rcWindow.top, sizeof(DWORD) );
        RegSetValueExW( hKey, reg_width, 0, REG_DWORD,
                        (LPBYTE) &width, sizeof(DWORD) );
        RegSetValueExW( hKey, reg_height, 0, REG_DWORD,
                        (LPBYTE) &height, sizeof(DWORD) );
        GetObjectW(Globals.hfont, sizeof(logfont), &logfont);
        RegSetValueExW( hKey, reg_logfont, 0, REG_BINARY,
                        (LPBYTE)&logfont, sizeof(LOGFONTW) );

	/* TODO: Save more settings here (List vs. Detailed View, etc.) */
	RegCloseKey( hKey );
}

static void resize_frame_rect(HWND hwnd, PRECT prect)
{
	int new_top;
	RECT rt;

	if (IsWindowVisible(Globals.htoolbar)) {
		SendMessageW(Globals.htoolbar, WM_SIZE, 0, 0);
		GetClientRect(Globals.htoolbar, &rt);
		prect->top = rt.bottom+3;
		prect->bottom -= rt.bottom+3;
	}

	if (IsWindowVisible(Globals.hdrivebar)) {
		SendMessageW(Globals.hdrivebar, WM_SIZE, 0, 0);
		GetClientRect(Globals.hdrivebar, &rt);
		new_top = --prect->top + rt.bottom+3;
		MoveWindow(Globals.hdrivebar, 0, prect->top, rt.right, new_top, TRUE);
		prect->top = new_top;
		prect->bottom -= rt.bottom+2;
	}

	if (IsWindowVisible(Globals.hstatusbar)) {
		int parts[] = {300, 500};

		SendMessageW(Globals.hstatusbar, WM_SIZE, 0, 0);
		SendMessageW(Globals.hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		GetClientRect(Globals.hstatusbar, &rt);
		prect->bottom -= rt.bottom;
	}

	MoveWindow(Globals.hmdiclient, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
}

static void resize_frame(HWND hwnd, int cx, int cy)
{
	RECT rect;

	rect.left   = 0;
	rect.top    = 0;
	rect.right  = cx;
	rect.bottom = cy;

	resize_frame_rect(hwnd, &rect);
}

static void resize_frame_client(HWND hwnd)
{
	RECT rect;

	GetClientRect(hwnd, &rect);

	resize_frame_rect(hwnd, &rect);
}


static HHOOK hcbthook;
static ChildWnd* newchild = NULL;

static LRESULT CALLBACK CBTProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code==HCBT_CREATEWND && newchild) {
		ChildWnd* child = newchild;
		newchild = NULL;

		child->hwnd = (HWND) wparam;
		SetWindowLongPtrW(child->hwnd, GWLP_USERDATA, (LPARAM)child);
	}

	return CallNextHookEx(hcbthook, code, wparam, lparam);
}

static HWND create_child_window(ChildWnd* child)
{
	MDICREATESTRUCTW mcs;
	int idx;

	mcs.szClass = sWINEFILETREE;
	mcs.szTitle = child->path;
	mcs.hOwner  = Globals.hInstance;
	mcs.x       = child->pos.rcNormalPosition.left;
	mcs.y       = child->pos.rcNormalPosition.top;
	mcs.cx      = child->pos.rcNormalPosition.right-child->pos.rcNormalPosition.left;
	mcs.cy      = child->pos.rcNormalPosition.bottom-child->pos.rcNormalPosition.top;
	mcs.style   = 0;
	mcs.lParam  = 0;

	hcbthook = SetWindowsHookExW(WH_CBT, CBTProc, 0, GetCurrentThreadId());

	newchild = child;
	child->hwnd = (HWND)SendMessageW(Globals.hmdiclient, WM_MDICREATE, 0, (LPARAM)&mcs);
	if (!child->hwnd) {
		UnhookWindowsHookEx(hcbthook);
		return 0;
	}

	UnhookWindowsHookEx(hcbthook);

	SendMessageW(child->left.hwnd, LB_SETITEMHEIGHT, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
	SendMessageW(child->right.hwnd, LB_SETITEMHEIGHT, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));

	idx = SendMessageW(child->left.hwnd, LB_FINDSTRING, 0, (LPARAM)child->left.cur);
	SendMessageW(child->left.hwnd, LB_SETCURSEL, idx, 0);

	return child->hwnd;
}

#define	RFF_NODEFAULT		0x02	/* No default item selected. */

static void WineFile_OnRun( HWND hwnd )
{
	static const WCHAR shell32_dll[] = {'S','H','E','L','L','3','2','.','D','L','L',0};
        void (WINAPI *pRunFileDlgAW )(HWND, HICON, LPWSTR, LPWSTR, LPWSTR, DWORD);
	HMODULE hshell = GetModuleHandleW( shell32_dll );

	pRunFileDlgAW = (void*)GetProcAddress(hshell, (LPCSTR)61);
	if (pRunFileDlgAW) pRunFileDlgAW( hwnd, 0, NULL, NULL, NULL, RFF_NODEFAULT);
}

static INT_PTR CALLBACK DestinationDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	WCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	switch(nmsg) {
		case WM_INITDIALOG:
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, lparam);
			SetWindowTextW(GetDlgItem(hwnd, 201), (LPCWSTR)lparam);
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			switch(id) {
			  case IDOK: {
				LPWSTR dest = (LPWSTR)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
				GetWindowTextW(GetDlgItem(hwnd, 201), dest, MAX_PATH);
				EndDialog(hwnd, id);
				break;}

			  case IDCANCEL:
				EndDialog(hwnd, id);
				break;

			  case 254:
				MessageBoxW(hwnd, RS(b1,IDS_NO_IMPL), RS(b2,IDS_WINEFILE), MB_OK);
				break;
			}

			return 1;
		}
	}

	return 0;
}


struct FilterDialog {
	WCHAR	pattern[MAX_PATH];
	int		flags;
};

static INT_PTR CALLBACK FilterDialogDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct FilterDialog* dlg;

	switch(nmsg) {
		case WM_INITDIALOG:
			dlg = (struct FilterDialog*) lparam;
			SetWindowTextW(GetDlgItem(hwnd, IDC_VIEW_PATTERN), dlg->pattern);
			set_check(hwnd, IDC_VIEW_TYPE_DIRECTORIES, dlg->flags&TF_DIRECTORIES);
			set_check(hwnd, IDC_VIEW_TYPE_PROGRAMS, dlg->flags&TF_PROGRAMS);
			set_check(hwnd, IDC_VIEW_TYPE_DOCUMENTS, dlg->flags&TF_DOCUMENTS);
			set_check(hwnd, IDC_VIEW_TYPE_OTHERS, dlg->flags&TF_OTHERS);
			set_check(hwnd, IDC_VIEW_TYPE_HIDDEN, dlg->flags&TF_HIDDEN);
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			if (id == IDOK) {
				int flags = 0;

				GetWindowTextW(GetDlgItem(hwnd, IDC_VIEW_PATTERN), dlg->pattern, MAX_PATH);

				flags |= get_check(hwnd, IDC_VIEW_TYPE_DIRECTORIES) ? TF_DIRECTORIES : 0;
				flags |= get_check(hwnd, IDC_VIEW_TYPE_PROGRAMS) ? TF_PROGRAMS : 0;
				flags |= get_check(hwnd, IDC_VIEW_TYPE_DOCUMENTS) ? TF_DOCUMENTS : 0;
				flags |= get_check(hwnd, IDC_VIEW_TYPE_OTHERS) ? TF_OTHERS : 0;
				flags |= get_check(hwnd, IDC_VIEW_TYPE_HIDDEN) ? TF_HIDDEN : 0;

				dlg->flags = flags;

				EndDialog(hwnd, id);
			} else if (id == IDCANCEL)
				EndDialog(hwnd, id);

			return 1;}
	}

	return 0;
}


struct PropertiesDialog {
	WCHAR	path[MAX_PATH];
	Entry	entry;
	void*	pVersionData;
};

/* Structure used to store enumerated languages and code pages. */
struct LANGANDCODEPAGE {
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;

static LPCSTR InfoStrings[] = {
	"Comments",
	"CompanyName",
	"FileDescription",
	"FileVersion",
	"InternalName",
	"LegalCopyright",
	"LegalTrademarks",
	"OriginalFilename",
	"PrivateBuild",
	"ProductName",
	"ProductVersion",
	"SpecialBuild",
	NULL
};

static void PropDlg_DisplayValue(HWND hlbox, HWND hedit)
{
	int idx = SendMessageW(hlbox, LB_GETCURSEL, 0, 0);

	if (idx != LB_ERR) {
		LPCWSTR pValue = (LPCWSTR)SendMessageW(hlbox, LB_GETITEMDATA, idx, 0);

		if (pValue)
			SetWindowTextW(hedit, pValue);
	}
}

static void CheckForFileInfo(struct PropertiesDialog* dlg, HWND hwnd, LPCWSTR strFilename)
{
	static const WCHAR sBackSlash[] = {'\\','\0'};
	static const WCHAR sTranslation[] = {'\\','V','a','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n','\0'};
	static const WCHAR sStringFileInfo[] = {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o','\\',
										'%','0','4','x','%','0','4','x','\\','%','s','\0'};
        static const WCHAR sFmt[] = {'%','d','.','%','d','.','%','d','.','%','d','\0'};
	DWORD dwVersionDataLen = GetFileVersionInfoSizeW(strFilename, NULL);

	if (dwVersionDataLen) {
		dlg->pVersionData = HeapAlloc(GetProcessHeap(), 0, dwVersionDataLen);

		if (GetFileVersionInfoW(strFilename, 0, dwVersionDataLen, dlg->pVersionData)) {
			LPVOID pVal;
			UINT nValLen;

			if (VerQueryValueW(dlg->pVersionData, sBackSlash, &pVal, &nValLen)) {
				if (nValLen == sizeof(VS_FIXEDFILEINFO)) {
					VS_FIXEDFILEINFO* pFixedFileInfo = (VS_FIXEDFILEINFO*)pVal;
                                        WCHAR buffer[BUFFER_LEN];

                                        sprintfW(buffer, sFmt,
                                                 HIWORD(pFixedFileInfo->dwFileVersionMS), LOWORD(pFixedFileInfo->dwFileVersionMS),
                                                 HIWORD(pFixedFileInfo->dwFileVersionLS), LOWORD(pFixedFileInfo->dwFileVersionLS));

                                        SetDlgItemTextW(hwnd, IDC_STATIC_PROP_VERSION, buffer);
				}
			}

			/* Read the list of languages and code pages. */
			if (VerQueryValueW(dlg->pVersionData, sTranslation, &pVal, &nValLen)) {
				struct LANGANDCODEPAGE* pTranslate = (struct LANGANDCODEPAGE*)pVal;
				struct LANGANDCODEPAGE* pEnd = (struct LANGANDCODEPAGE*)((LPBYTE)pVal+nValLen);

				HWND hlbox = GetDlgItem(hwnd, IDC_LIST_PROP_VERSION_TYPES);

				/* Read the file description for each language and code page. */
				for(; pTranslate<pEnd; ++pTranslate) {
					LPCSTR* p;

					for(p=InfoStrings; *p; ++p) {
						WCHAR subblock[200];
						WCHAR infoStr[100];
						LPCWSTR pTxt;
						UINT nValLen;

						LPCSTR pInfoString = *p;
						MultiByteToWideChar(CP_ACP, 0, pInfoString, -1, infoStr, 100);
						wsprintfW(subblock, sStringFileInfo, pTranslate->wLanguage, pTranslate->wCodePage, infoStr);

						/* Retrieve file description for language and code page */
						if (VerQueryValueW(dlg->pVersionData, subblock, (PVOID)&pTxt, &nValLen)) {
							int idx = SendMessageW(hlbox, LB_ADDSTRING, 0L, (LPARAM)infoStr);
							SendMessageW(hlbox, LB_SETITEMDATA, idx, (LPARAM)pTxt);
						}
					}
				}

				SendMessageW(hlbox, LB_SETCURSEL, 0, 0);

				PropDlg_DisplayValue(hlbox, GetDlgItem(hwnd,IDC_LIST_PROP_VERSION_VALUES));
			}
		}
	}
}

static INT_PTR CALLBACK PropertiesDialogDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct PropertiesDialog* dlg;

	switch(nmsg) {
		case WM_INITDIALOG: {
			static const WCHAR sByteFmt[] = {'%','s',' ','B','y','t','e','s','\0'};
			WCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];
			LPWIN32_FIND_DATAW pWFD;

			dlg = (struct PropertiesDialog*) lparam;
			pWFD = (LPWIN32_FIND_DATAW)&dlg->entry.data;

			GetWindowTextW(hwnd, b1, MAX_PATH);
			wsprintfW(b2, b1, pWFD->cFileName);
			SetWindowTextW(hwnd, b2);

			format_date(&pWFD->ftLastWriteTime, b1, COL_DATE|COL_TIME);
			SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_PROP_LASTCHANGE), b1);

                        format_longlong( b1, ((ULONGLONG)pWFD->nFileSizeHigh << 32) | pWFD->nFileSizeLow );
			wsprintfW(b2, sByteFmt, b1);
			SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_PROP_SIZE), b2);

			SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_PROP_FILENAME), pWFD->cFileName);
			SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_PROP_PATH), dlg->path);

			set_check(hwnd, IDC_CHECK_READONLY, pWFD->dwFileAttributes&FILE_ATTRIBUTE_READONLY);
			set_check(hwnd, IDC_CHECK_ARCHIVE, pWFD->dwFileAttributes&FILE_ATTRIBUTE_ARCHIVE);
			set_check(hwnd, IDC_CHECK_COMPRESSED, pWFD->dwFileAttributes&FILE_ATTRIBUTE_COMPRESSED);
			set_check(hwnd, IDC_CHECK_HIDDEN, pWFD->dwFileAttributes&FILE_ATTRIBUTE_HIDDEN);
			set_check(hwnd, IDC_CHECK_SYSTEM, pWFD->dwFileAttributes&FILE_ATTRIBUTE_SYSTEM);

			CheckForFileInfo(dlg, hwnd, dlg->path);
			return 1;}

		case WM_COMMAND: {
			int id = (int)wparam;

			switch(HIWORD(wparam)) {
			  case LBN_SELCHANGE: {
				HWND hlbox = GetDlgItem(hwnd, IDC_LIST_PROP_VERSION_TYPES);
				PropDlg_DisplayValue(hlbox, GetDlgItem(hwnd,IDC_LIST_PROP_VERSION_VALUES));
				break;
			  }

			  case BN_CLICKED:
				if (id==IDOK || id==IDCANCEL)
					EndDialog(hwnd, id);
			}

			return 1;}

		case WM_NCDESTROY:
			HeapFree(GetProcessHeap(), 0, dlg->pVersionData);
			dlg->pVersionData = NULL;
			break;
	}

	return 0;
}

static void show_properties_dlg(Entry* entry, HWND hwnd)
{
	struct PropertiesDialog dlg;

	memset(&dlg, 0, sizeof(struct PropertiesDialog));
	get_path(entry, dlg.path);
	memcpy(&dlg.entry, entry, sizeof(Entry));

	DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_DIALOG_PROPERTIES), hwnd, PropertiesDialogDlgProc, (LPARAM)&dlg);
}

static struct FullScreenParameters {
	BOOL	mode;
	RECT	orgPos;
	BOOL	wasZoomed;
} g_fullscreen = {
    FALSE,	/* mode */
	{0, 0, 0, 0},
	FALSE
};

static void frame_get_clientspace(HWND hwnd, PRECT prect)
{
	RECT rt;

	if (!IsIconic(hwnd))
		GetClientRect(hwnd, prect);
	else {
		WINDOWPLACEMENT wp;

		GetWindowPlacement(hwnd, &wp);

		prect->left = prect->top = 0;
		prect->right = wp.rcNormalPosition.right-wp.rcNormalPosition.left-
						2*(GetSystemMetrics(SM_CXSIZEFRAME)+GetSystemMetrics(SM_CXEDGE));
		prect->bottom = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-
						2*(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYEDGE))-
						GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENUSIZE);
	}

	if (IsWindowVisible(Globals.htoolbar)) {
		GetClientRect(Globals.htoolbar, &rt);
		prect->top += rt.bottom+2;
	}

	if (IsWindowVisible(Globals.hdrivebar)) {
		GetClientRect(Globals.hdrivebar, &rt);
		prect->top += rt.bottom+2;
	}

	if (IsWindowVisible(Globals.hstatusbar)) {
		GetClientRect(Globals.hstatusbar, &rt);
		prect->bottom -= rt.bottom;
	}
}

static BOOL toggle_fullscreen(HWND hwnd)
{
	RECT rt;

	if ((g_fullscreen.mode=!g_fullscreen.mode)) {
		GetWindowRect(hwnd, &g_fullscreen.orgPos);
		g_fullscreen.wasZoomed = IsZoomed(hwnd);

		Frame_CalcFrameClient(hwnd, &rt);
                MapWindowPoints( hwnd, 0, (POINT *)&rt, 2 );

		rt.left = g_fullscreen.orgPos.left-rt.left;
		rt.top = g_fullscreen.orgPos.top-rt.top;
		rt.right = GetSystemMetrics(SM_CXSCREEN)+g_fullscreen.orgPos.right-rt.right;
		rt.bottom = GetSystemMetrics(SM_CYSCREEN)+g_fullscreen.orgPos.bottom-rt.bottom;

		MoveWindow(hwnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
	} else {
		MoveWindow(hwnd, g_fullscreen.orgPos.left, g_fullscreen.orgPos.top,
							g_fullscreen.orgPos.right-g_fullscreen.orgPos.left,
							g_fullscreen.orgPos.bottom-g_fullscreen.orgPos.top, TRUE);

		if (g_fullscreen.wasZoomed)
			ShowWindow(hwnd, WS_MAXIMIZE);
	}

	return g_fullscreen.mode;
}

static void fullscreen_move(HWND hwnd)
{
	RECT rt, pos;
	GetWindowRect(hwnd, &pos);

	Frame_CalcFrameClient(hwnd, &rt);
        MapWindowPoints( hwnd, 0, (POINT *)&rt, 2 );

	rt.left = pos.left-rt.left;
	rt.top = pos.top-rt.top;
	rt.right = GetSystemMetrics(SM_CXSCREEN)+pos.right-rt.right;
	rt.bottom = GetSystemMetrics(SM_CYSCREEN)+pos.bottom-rt.bottom;

	MoveWindow(hwnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
}

static void toggle_child(HWND hwnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(Globals.hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);

	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);

	if (g_fullscreen.mode)
		fullscreen_move(hwnd);

	resize_frame_client(hwnd);
}

static BOOL activate_drive_window(LPCWSTR path)
{
	WCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_wsplitpath(path, drv1, 0, 0, 0);

        /* search for an already open window for the same drive */
	for(child_wnd=GetNextWindow(Globals.hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* child = (ChildWnd*)GetWindowLongPtrW(child_wnd, GWLP_USERDATA);

		if (child) {
			_wsplitpath(child->root.path, drv2, 0, 0, 0);

			if (!lstrcmpiW(drv2, drv1)) {
				SendMessageW(Globals.hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsIconic(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return TRUE;
			}
		}
	}

	return FALSE;
}

static BOOL activate_fs_window(LPCWSTR filesys)
{
	HWND child_wnd;

        /* search for an already open window of the given file system name */
	for(child_wnd=GetNextWindow(Globals.hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* child = (ChildWnd*) GetWindowLongPtrW(child_wnd, GWLP_USERDATA);

		if (child) {
			if (!lstrcmpiW(child->root.fs, filesys)) {
				SendMessageW(Globals.hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsIconic(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return TRUE;
			}
		}
	}

	return FALSE;
}

static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	WCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	switch(nmsg) {
		case WM_CLOSE:
			if (Globals.saveSettings)
				save_registry_settings();

			DestroyWindow(hwnd);

			 /* clear handle variables */
			Globals.hMenuFrame = 0;
			Globals.hMenuView = 0;
			Globals.hMenuOptions = 0;
			Globals.hMainWnd = 0;
			Globals.hmdiclient = 0;
			Globals.hdrivebar = 0;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_INITMENUPOPUP: {
			HWND hwndClient = (HWND)SendMessageW(Globals.hmdiclient, WM_MDIGETACTIVE, 0, 0);

			if (!SendMessageW(hwndClient, WM_INITMENUPOPUP, wparam, lparam))
				return 0;
			break;}

		case WM_COMMAND: {
			UINT cmd = LOWORD(wparam);
			HWND hwndClient = (HWND)SendMessageW(Globals.hmdiclient, WM_MDIGETACTIVE, 0, 0);

			if (SendMessageW(hwndClient, WM_DISPATCH_COMMAND, wparam, lparam))
				break;

			if (cmd>=ID_DRIVE_FIRST && cmd<=ID_DRIVE_FIRST+0xFF) {
				WCHAR drv[_MAX_DRIVE], path[MAX_PATH];
				ChildWnd* child;
				LPCWSTR root = Globals.drives;
				int i;

				for(i=cmd-ID_DRIVE_FIRST; i--; root++)
					while(*root)
						root++;

				if (activate_drive_window(root))
					return 0;

				_wsplitpath(root, drv, 0, 0, 0);

				if (!SetCurrentDirectoryW(drv)) {
					display_error(hwnd, GetLastError());
					return 0;
				}

				GetCurrentDirectoryW(MAX_PATH, path); /*TODO: store last directory per drive */
				child = alloc_child_window(path, NULL, hwnd);

				if (!create_child_window(child))
					HeapFree(GetProcessHeap(), 0, child);
			} else switch(cmd) {
				case ID_FILE_EXIT:
					SendMessageW(hwnd, WM_CLOSE, 0, 0);
					break;

				case ID_WINDOW_NEW: {
					WCHAR path[MAX_PATH];
					ChildWnd* child;

					GetCurrentDirectoryW(MAX_PATH, path);
					child = alloc_child_window(path, NULL, hwnd);

					if (!create_child_window(child))
						HeapFree(GetProcessHeap(), 0, child);
					break;}

				case ID_REFRESH:
					refresh_drives();
					break;

				case ID_WINDOW_CASCADE:
					SendMessageW(Globals.hmdiclient, WM_MDICASCADE, 0, 0);
					break;

				case ID_WINDOW_TILE_HORZ:
					SendMessageW(Globals.hmdiclient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
					break;

				case ID_WINDOW_TILE_VERT:
					SendMessageW(Globals.hmdiclient, WM_MDITILE, MDITILE_VERTICAL, 0);
					break;

				case ID_WINDOW_ARRANGE:
					SendMessageW(Globals.hmdiclient, WM_MDIICONARRANGE, 0, 0);
					break;

				case ID_SELECT_FONT:
                                        choose_font(hwnd);
                                        break;

				case ID_VIEW_TOOL_BAR:
					toggle_child(hwnd, cmd, Globals.htoolbar);
					break;

				case ID_VIEW_DRIVE_BAR:
					toggle_child(hwnd, cmd, Globals.hdrivebar);
					break;

				case ID_VIEW_STATUSBAR:
					toggle_child(hwnd, cmd, Globals.hstatusbar);
					break;

				case ID_VIEW_SAVESETTINGS:
					Globals.saveSettings = !Globals.saveSettings;
					CheckMenuItem(Globals.hMenuOptions, ID_VIEW_SAVESETTINGS,
                                                      Globals.saveSettings ? MF_CHECKED : MF_UNCHECKED );
					break;

				case ID_RUN:
					WineFile_OnRun( hwnd );
					break;

				case ID_CONNECT_NETWORK_DRIVE: {
					DWORD ret = WNetConnectionDialog(hwnd, RESOURCETYPE_DISK);
					if (ret == NO_ERROR)
						refresh_drives();
					else if (ret != (DWORD)-1) {
						if (ret == ERROR_EXTENDED_ERROR)
							display_network_error(hwnd);
						else
							display_error(hwnd, ret);
					}
					break;}

				case ID_DISCONNECT_NETWORK_DRIVE: {
					DWORD ret = WNetDisconnectDialog(hwnd, RESOURCETYPE_DISK);
					if (ret == NO_ERROR)
						refresh_drives();
					else if (ret != (DWORD)-1) {
						if (ret == ERROR_EXTENDED_ERROR)
							display_network_error(hwnd);
						else
							display_error(hwnd, ret);
					}
					break;}

				case ID_HELP:
					WinHelpW(hwnd, RS(b1,IDS_WINEFILE), HELP_INDEX, 0);
					break;

				case ID_VIEW_FULLSCREEN:
					CheckMenuItem(Globals.hMenuOptions, cmd, toggle_fullscreen(hwnd)?MF_CHECKED:0);
					break;

#ifdef __WINE__
				case ID_DRIVE_UNIX_FS: {
					WCHAR path[MAX_PATH];
					char cpath[MAX_PATH];
					ChildWnd* child;

					if (activate_fs_window(RS(b1,IDS_UNIXFS)))
						break;

					getcwd(cpath, MAX_PATH);
					MultiByteToWideChar(CP_UNIXCP, 0, cpath, -1, path, MAX_PATH);
					child = alloc_child_window(path, NULL, hwnd);

					if (!create_child_window(child))
						HeapFree(GetProcessHeap(), 0, child);
					break;}
#endif
				case ID_DRIVE_SHELL_NS: {
					WCHAR path[MAX_PATH];
					ChildWnd* child;

					if (activate_fs_window(RS(b1,IDS_SHELL)))
						break;

					GetCurrentDirectoryW(MAX_PATH, path);
					child = alloc_child_window(path, get_path_pidl(path,hwnd), hwnd);

					if (!create_child_window(child))
						HeapFree(GetProcessHeap(), 0, child);
					break;}

				/*TODO: There are even more menu items! */

				case ID_ABOUT:
                                        ShellAboutW(hwnd, RS(b1,IDS_WINEFILE), NULL,
                                                   LoadImageW( Globals.hInstance, MAKEINTRESOURCEW(IDI_WINEFILE),
                                                              IMAGE_ICON, 48, 48, LR_SHARED ));
					break;

				default:
					/*TODO: if (wParam >= PM_FIRST_LANGUAGE && wParam <= PM_LAST_LANGUAGE)
						STRING_SelectLanguageByNumber(wParam - PM_FIRST_LANGUAGE);
					else */if ((cmd<IDW_FIRST_CHILD || cmd>=IDW_FIRST_CHILD+0x100) &&
						(cmd<SC_SIZE || cmd>SC_RESTORE))
						MessageBoxW(hwnd, RS(b2,IDS_NO_IMPL), RS(b1,IDS_WINEFILE), MB_OK);

					return DefFrameProcW(hwnd, Globals.hmdiclient, nmsg, wparam, lparam);
			}
			break;}

		case WM_SIZE:
			resize_frame(hwnd, LOWORD(lparam), HIWORD(lparam));
			break;	/* do not pass message to DefFrameProcW */

		case WM_DEVICECHANGE:
			SendMessageW(hwnd, WM_COMMAND, MAKELONG(ID_REFRESH,0), 0);
			break;

		case WM_GETMINMAXINFO: {
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

			lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
			lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
			break;}

		case FRM_CALC_CLIENT:
			frame_get_clientspace(hwnd, (PRECT)lparam);
			return TRUE;

		default:
			return DefFrameProcW(hwnd, Globals.hmdiclient, nmsg, wparam, lparam);
	}

	return 0;
}


static WCHAR g_pos_names[COLUMNS][40] = {
	{'\0'}	/* symbol */
};

static const int g_pos_align[] = {
	0,
	HDF_LEFT,	/* Name */
	HDF_RIGHT,	/* Size */
	HDF_LEFT,	/* CDate */
	HDF_LEFT,	/* ADate */
	HDF_LEFT,	/* MDate */
	HDF_LEFT,	/* Index */
	HDF_CENTER,	/* Links */
	HDF_CENTER,	/* Attributes */
	HDF_LEFT	/* Security */
};

static void resize_tree(ChildWnd* child, int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt;
        WINDOWPOS wp;
        HD_LAYOUT hdl;

	rt.left   = 0;
	rt.top    = 0;
	rt.right  = cx;
	rt.bottom = cy;

	cx = child->split_pos + SPLIT_WIDTH/2;
        hdl.prc   = &rt;
        hdl.pwpos = &wp;

        SendMessageW(child->left.hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);

        DeferWindowPos(hdwp, child->left.hwndHeader, wp.hwndInsertAfter,
                       wp.x-1, wp.y, child->split_pos-SPLIT_WIDTH/2+1, wp.cy, wp.flags);
        DeferWindowPos(hdwp, child->right.hwndHeader, wp.hwndInsertAfter,
                       rt.left+cx+1, wp.y, wp.cx-cx+2, wp.cy, wp.flags);
	DeferWindowPos(hdwp, child->left.hwnd, 0, rt.left, rt.top, child->split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	DeferWindowPos(hdwp, child->right.hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}

static HWND create_header(HWND parent, Pane* pane, UINT id)
{
	HDITEMW hdi;
	int idx;

	HWND hwnd = CreateWindowW(WC_HEADERW, 0, WS_CHILD|WS_VISIBLE|HDS_HORZ|HDS_FULLDRAG/*TODO: |HDS_BUTTONS + sort orders*/,
                                 0, 0, 0, 0, parent, (HMENU)ULongToHandle(id), Globals.hInstance, 0);
	if (!hwnd)
		return 0;

	SendMessageW(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

	hdi.mask = HDI_TEXT|HDI_WIDTH|HDI_FORMAT;

	for(idx=0; idx<COLUMNS; idx++) {
		hdi.pszText = g_pos_names[idx];
		hdi.fmt = HDF_STRING | g_pos_align[idx];
		hdi.cxy = pane->widths[idx];
		SendMessageW(hwnd, HDM_INSERTITEMW, idx, (LPARAM)&hdi);
	}

	return hwnd;
}

static void init_output(HWND hwnd)
{
	static const WCHAR s1000[] = {'1','0','0','0','\0'};
	WCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hwnd);

	if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, s1000, 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = '.';

	old_font = SelectObject(hdc, Globals.hfont);
	GetTextExtentPoint32W(hdc, sSpace, 1, &Globals.spaceSize);
	SelectObject(hdc, old_font);
	ReleaseDC(hwnd, hdc);
}

static void draw_item(Pane* pane, LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol);


/* calculate preferred width for all visible columns */

static BOOL calc_widths(Pane* pane, BOOL anyway)
{
	int col, x, cx, spc=3*Globals.spaceSize.cx;
	int entries = SendMessageW(pane->hwnd, LB_GETCOUNT, 0, 0);
	int orgWidths[COLUMNS];
	int orgPositions[COLUMNS+1];
	HFONT hfontOld;
	HDC hdc;
	int cnt;

	if (!anyway) {
		memcpy(orgWidths, pane->widths, sizeof(orgWidths));
		memcpy(orgPositions, pane->positions, sizeof(orgPositions));
	}

	for(col=0; col<COLUMNS; col++)
		pane->widths[col] = 0;

	hdc = GetDC(pane->hwnd);
	hfontOld = SelectObject(hdc, Globals.hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*)SendMessageW(pane->hwnd, LB_GETITEMDATA, cnt, 0);

		DRAWITEMSTRUCT dis;

		dis.CtlType		  = 0;
		dis.CtlID		  = 0;
		dis.itemID		  = 0;
		dis.itemAction	  = 0;
		dis.itemState	  = 0;
		dis.hwndItem	  = pane->hwnd;
		dis.hDC			  = hdc;
		dis.rcItem.left	  = 0;
		dis.rcItem.top    = 0;
		dis.rcItem.right  = 0;
		dis.rcItem.bottom = 0;
		/*dis.itemData	  = 0; */

		draw_item(pane, &dis, entry, COLUMNS);
	}

	SelectObject(hdc, hfontOld);
	ReleaseDC(pane->hwnd, hdc);

	x = 0;
	for(col=0; col<COLUMNS; col++) {
		pane->positions[col] = x;
		cx = pane->widths[col];

		if (cx) {
			cx += spc;

			if (cx < IMAGE_WIDTH)
				cx = IMAGE_WIDTH;

			pane->widths[col] = cx;
		}

		x += cx;
	}

	pane->positions[COLUMNS] = x;

	SendMessageW(pane->hwnd, LB_SETHORIZONTALEXTENT, x, 0);

	/* no change? */
	if (!anyway && !memcmp(orgWidths, pane->widths, sizeof(orgWidths)))
		return FALSE;

	/* don't move, if only collapsing an entry */
	if (!anyway && pane->widths[0]<orgWidths[0] &&
		!memcmp(orgWidths+1, pane->widths+1, sizeof(orgWidths)-sizeof(int))) {
		pane->widths[0] = orgWidths[0];
		memcpy(pane->positions, orgPositions, sizeof(orgPositions));

		return FALSE;
	}

	InvalidateRect(pane->hwnd, 0, TRUE);

	return TRUE;
}

/* calculate one preferred column width */
static void calc_single_width(Pane* pane, int col)
{
	HFONT hfontOld;
	int x, cx;
	int entries = SendMessageW(pane->hwnd, LB_GETCOUNT, 0, 0);
	int cnt;
	HDC hdc;

	pane->widths[col] = 0;

	hdc = GetDC(pane->hwnd);
	hfontOld = SelectObject(hdc, Globals.hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*)SendMessageW(pane->hwnd, LB_GETITEMDATA, cnt, 0);
		DRAWITEMSTRUCT dis;

		dis.CtlType		  = 0;
		dis.CtlID		  = 0;
		dis.itemID		  = 0;
		dis.itemAction	  = 0;
		dis.itemState	  = 0;
		dis.hwndItem	  = pane->hwnd;
		dis.hDC			  = hdc;
		dis.rcItem.left	  = 0;
		dis.rcItem.top    = 0;
		dis.rcItem.right  = 0;
		dis.rcItem.bottom = 0;
		/*dis.itemData	  = 0; */

		draw_item(pane, &dis, entry, col);
	}

	SelectObject(hdc, hfontOld);
	ReleaseDC(pane->hwnd, hdc);

	cx = pane->widths[col];

	if (cx) {
		cx += 3*Globals.spaceSize.cx;

		if (cx < IMAGE_WIDTH)
			cx = IMAGE_WIDTH;
	}

	pane->widths[col] = cx;

	x = pane->positions[col] + cx;

	for(; col<COLUMNS-1; ) {
		pane->positions[++col] = x;
		x += pane->widths[col];
	}

	SendMessageW(pane->hwnd, LB_SETHORIZONTALEXTENT, x, 0);
}

static BOOL pattern_match(LPCWSTR str, LPCWSTR pattern)
{
	for( ; *str&&*pattern; str++,pattern++) {
		if (*pattern == '*') {
			do pattern++;
			while(*pattern == '*');

			if (!*pattern)
				return TRUE;

			for(; *str; str++)
				if (*str==*pattern && pattern_match(str, pattern))
					return TRUE;

			return FALSE;
		}
		else if (*str!=*pattern && *pattern!='?')
			return FALSE;
	}

	if (*str || *pattern)
		if (*pattern!='*' || pattern[1]!='\0')
			return FALSE;

	return TRUE;
}

static BOOL pattern_imatch(LPCWSTR str, LPCWSTR pattern)
{
	WCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	lstrcpyW(b1, str);
	lstrcpyW(b2, pattern);
	CharUpperW(b1);
	CharUpperW(b2);

	return pattern_match(b1, b2);
}


enum FILE_TYPE {
	FT_OTHER		= 0,
	FT_EXECUTABLE	= 1,
	FT_DOCUMENT		= 2
};

static enum FILE_TYPE get_file_type(LPCWSTR filename);


/* insert listbox entries after index idx */

static int insert_entries(Pane* pane, Entry* dir, LPCWSTR pattern, int filter_flags, int idx)
{
	Entry* entry = dir;

	if (!entry)
		return idx;

	ShowWindow(pane->hwnd, SW_HIDE);

	for(; entry; entry=entry->next) {
		if (pane->treePane && !(entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			continue;

		if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			/* don't display entries "." and ".." in the left pane */
			if (pane->treePane && entry->data.cFileName[0] == '.')
                                if (entry->data.cFileName[1] == '\0' ||
                                    (entry->data.cFileName[1] == '.' &&
                                    entry->data.cFileName[2] == '\0'))
					continue;

			/* filter directories in right pane */
			if (!pane->treePane && !(filter_flags&TF_DIRECTORIES))
				continue;
		}

		/* filter using the file name pattern */
		if (pattern)
			if (!pattern_imatch(entry->data.cFileName, pattern))
				continue;

		/* filter system and hidden files */
		if (!(filter_flags&TF_HIDDEN) && (entry->data.dwFileAttributes&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)))
			continue;

		/* filter looking at the file type */
		if ((filter_flags&(TF_PROGRAMS|TF_DOCUMENTS|TF_OTHERS)) != (TF_PROGRAMS|TF_DOCUMENTS|TF_OTHERS))
			switch(get_file_type(entry->data.cFileName)) {
			  case FT_EXECUTABLE:
			  	if (!(filter_flags & TF_PROGRAMS))
					continue;
				break;

			  case FT_DOCUMENT:
				if (!(filter_flags & TF_DOCUMENTS))
					continue;
				break;

			  default: /* TF_OTHERS */
				if (!(filter_flags & TF_OTHERS))
					continue;
			}

		if (idx != -1)
			idx++;

		SendMessageW(pane->hwnd, LB_INSERTSTRING, idx, (LPARAM)entry);

		if (pane->treePane && entry->expanded)
			idx = insert_entries(pane, entry->down, pattern, filter_flags, idx);
	}

	ShowWindow(pane->hwnd, SW_SHOW);

	return idx;
}


static void format_bytes(LPWSTR buffer, LONGLONG bytes)
{
	static const WCHAR sFmtSmall[]  = {'%', 'u', 0};
	static const WCHAR sFmtBig[] = {'%', '.', '1', 'f', ' ', '%', 's', '\0'};

	if (bytes < 1024)
		sprintfW(buffer, sFmtSmall, (DWORD)bytes);
	else
	{
		WCHAR unit[64];
		UINT resid;
		float fBytes;
		if (bytes >= 1073741824)	/* 1 GB */
		{
			fBytes = ((float)bytes)/1073741824.f+.5f;
			resid = IDS_UNIT_GB;
		}
		else if (bytes >= 1048576)	/* 1 MB */
		{
			fBytes = ((float)bytes)/1048576.f+.5f;
			resid = IDS_UNIT_MB;
		}
		else /* bytes >= 1024 */	/* 1 kB */
		{
			fBytes = ((float)bytes)/1024.f+.5f;
			resid = IDS_UNIT_KB;
		}
		LoadStringW(Globals.hInstance, resid, unit, sizeof(unit)/sizeof(*unit));
		sprintfW(buffer, sFmtBig, fBytes, unit);
	}
}

static void set_space_status(void)
{
	ULARGE_INTEGER ulFreeBytesToCaller, ulTotalBytes, ulFreeBytes;
	WCHAR fmt[64], b1[64], b2[64], buffer[BUFFER_LEN];

	if (GetDiskFreeSpaceExW(NULL, &ulFreeBytesToCaller, &ulTotalBytes, &ulFreeBytes)) {
		DWORD_PTR args[2];
		format_bytes(b1, ulFreeBytesToCaller.QuadPart);
		format_bytes(b2, ulTotalBytes.QuadPart);
		args[0] = (DWORD_PTR)b1;
		args[1] = (DWORD_PTR)b2;
		FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
		               RS(fmt,IDS_FREE_SPACE_FMT), 0, 0, buffer,
		               sizeof(buffer)/sizeof(*buffer), (__ms_va_list*)args);
	} else
		lstrcpyW(buffer, sQMarks);

	SendMessageW(Globals.hstatusbar, SB_SETTEXTW, 0, (LPARAM)buffer);
}


static WNDPROC g_orgTreeWndProc;

static void create_tree_window(HWND parent, Pane* pane, UINT id, UINT id_header, LPCWSTR pattern, int filter_flags)
{
	static const WCHAR sListBox[] = {'L','i','s','t','B','o','x','\0'};

        static BOOL s_init = FALSE;
	Entry* entry = pane->root;

	pane->hwnd = CreateWindowW(sListBox, sEmpty, WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
                                  LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
                                  0, 0, 0, 0, parent, (HMENU)ULongToHandle(id), Globals.hInstance, 0);

	SetWindowLongPtrW(pane->hwnd, GWLP_USERDATA, (LPARAM)pane);
	g_orgTreeWndProc = (WNDPROC)SetWindowLongPtrW(pane->hwnd, GWLP_WNDPROC, (LPARAM)TreeWndProc);

	SendMessageW(pane->hwnd, WM_SETFONT, (WPARAM)Globals.hfont, FALSE);

	/* insert entries into listbox */
	if (entry)
		insert_entries(pane, entry, pattern, filter_flags, -1);

	/* calculate column widths */
	if (!s_init) {
                s_init = TRUE;
		init_output(pane->hwnd);
	}

	calc_widths(pane, TRUE);

	pane->hwndHeader = create_header(parent, pane, id_header);
}


static void InitChildWindow(ChildWnd* child)
{
	create_tree_window(child->hwnd, &child->left, IDW_TREE_LEFT, IDW_HEADER_LEFT, NULL, TF_ALL);
	create_tree_window(child->hwnd, &child->right, IDW_TREE_RIGHT, IDW_HEADER_RIGHT, child->filter_pattern, child->filter_flags);
}


static void format_date(const FILETIME* ft, WCHAR* buffer, int visible_cols)
{
	SYSTEMTIME systime;
	FILETIME lft;
	int len = 0;

	*buffer = '\0';

	if (!ft->dwLowDateTime && !ft->dwHighDateTime)
		return;

	if (!FileTimeToLocalFileTime(ft, &lft))
		{err: lstrcpyW(buffer,sQMarks); return;}

	if (!FileTimeToSystemTime(&lft, &systime))
		goto err;

	if (visible_cols & COL_DATE) {
		len = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &systime, 0, buffer, BUFFER_LEN);
		if (!len)
			goto err;
	}

	if (visible_cols & COL_TIME) {
		if (len)
			buffer[len-1] = ' ';

		buffer[len++] = ' ';

		if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime, 0, buffer+len, BUFFER_LEN-len))
			buffer[len] = '\0';
	}
}


static void calc_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCWSTR str)
{
	RECT rt = {0, 0, 0, 0};

	DrawTextW(dis->hDC, str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}

static void calc_tabbed_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCWSTR str)
{
	RECT rt = {0, 0, 0, 0};

	DrawTextW(dis->hDC, str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
	/*FIXME rt (0,0) ??? */

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}


static void output_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCWSTR str, DWORD flags)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	DrawTextW(dis->hDC, str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|flags);
}

static void output_tabbed_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCWSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	DrawTextW(dis->hDC, str, -1, &rt, DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
}

static void output_number(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCWSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;
	LPCWSTR s = str;
	WCHAR b[128];
	LPWSTR d = b;
	int pos;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	if (*s)
		*d++ = *s++;

	/* insert number separator characters */
	pos = lstrlenW(s) % 3;

	while(*s)
		if (pos--)
			*d++ = *s++;
		else {
			*d++ = Globals.num_sep;
			pos = 3;
		}

	DrawTextW(dis->hDC, b, d-b, &rt, DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
}


static BOOL is_exe_file(LPCWSTR ext)
{
	static const WCHAR executable_extensions[][4] = {
		{'C','O','M','\0'},
		{'E','X','E','\0'},
		{'B','A','T','\0'},
		{'C','M','D','\0'},
		{'C','M','M','\0'},
		{'B','T','M','\0'},
		{'A','W','K','\0'},
		{'\0'}
	};

	WCHAR ext_buffer[_MAX_EXT];
	const WCHAR (*p)[4];
	LPCWSTR s;
	LPWSTR d;

	for(s=ext+1,d=ext_buffer; (*d=tolower(*s)); s++)
		d++;

	for(p=executable_extensions; (*p)[0]; p++)
		if (!lstrcmpiW(ext_buffer, *p))
			return TRUE;

	return FALSE;
}

static BOOL is_registered_type(LPCWSTR ext)
{
	/* check if there exists a classname for this file extension in the registry */
	if (!RegQueryValueW(HKEY_CLASSES_ROOT, ext, NULL, NULL))
		return TRUE;

	return FALSE;
}

static enum FILE_TYPE get_file_type(LPCWSTR filename)
{
	LPCWSTR ext = strrchrW(filename, '.');
	if (!ext)
		ext = sEmpty;

	if (is_exe_file(ext))
		return FT_EXECUTABLE;
	else if (is_registered_type(ext))
		return FT_DOCUMENT;
	else
		return FT_OTHER;
}


static void draw_item(Pane* pane, LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol)
{
	WCHAR buffer[BUFFER_LEN];
	DWORD attrs;
	int visible_cols = pane->visible_cols;
	COLORREF bkcolor, textcolor;
	RECT focusRect = dis->rcItem;
	HBRUSH hbrush;
	enum IMAGE img;
	int img_pos, cx;
	int col = 0;

	if (entry) {
		attrs = entry->data.dwFileAttributes;

		if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			if (entry->data.cFileName[0] == '.' && entry->data.cFileName[1] == '.'
					&& entry->data.cFileName[2] == '\0')
				img = IMG_FOLDER_UP;
			else if (entry->data.cFileName[0] == '.' && entry->data.cFileName[1] == '\0')
				img = IMG_FOLDER_CUR;
                        else if (pane->treePane && (dis->itemState&ODS_FOCUS))
				img = IMG_OPEN_FOLDER;
			else
				img = IMG_FOLDER;
		} else {
			switch(get_file_type(entry->data.cFileName)) {
			  case FT_EXECUTABLE:	img = IMG_EXECUTABLE;	break;
			  case FT_DOCUMENT:		img = IMG_DOCUMENT;		break;
			  default:				img = IMG_FILE;
			}
		}
	} else {
		attrs = 0;
		img = IMG_NONE;
	}

	if (pane->treePane) {
		if (entry) {
			img_pos = dis->rcItem.left + entry->level*(IMAGE_WIDTH+TREE_LINE_DX);

			if (calcWidthCol == -1) {
				int x;
				int y = dis->rcItem.top + IMAGE_HEIGHT/2;
				Entry* up;
				RECT rt_clip;
				HRGN hrgn_org = CreateRectRgn(0, 0, 0, 0);
				HRGN hrgn;

				rt_clip.left   = dis->rcItem.left;
				rt_clip.top    = dis->rcItem.top;
				rt_clip.right  = dis->rcItem.left+pane->widths[col];
				rt_clip.bottom = dis->rcItem.bottom;

				hrgn = CreateRectRgnIndirect(&rt_clip);

				if (!GetClipRgn(dis->hDC, hrgn_org)) {
					DeleteObject(hrgn_org);
					hrgn_org = 0;
				}

				ExtSelectClipRgn(dis->hDC, hrgn, RGN_AND);
				DeleteObject(hrgn);

				if ((up=entry->up) != NULL) {
					MoveToEx(dis->hDC, img_pos-IMAGE_WIDTH/2, y, 0);
					LineTo(dis->hDC, img_pos-2, y);

					x = img_pos - IMAGE_WIDTH/2;

					do {
						x -= IMAGE_WIDTH+TREE_LINE_DX;

						if (up->next
							&& (up->next->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							) {
							MoveToEx(dis->hDC, x, dis->rcItem.top, 0);
							LineTo(dis->hDC, x, dis->rcItem.bottom);
						}
					} while((up=up->up) != NULL);
				}

				x = img_pos - IMAGE_WIDTH/2;

				MoveToEx(dis->hDC, x, dis->rcItem.top, 0);
				LineTo(dis->hDC, x, y);

				if (entry->next
                                        && (entry->next->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
					LineTo(dis->hDC, x, dis->rcItem.bottom);

				SelectClipRgn(dis->hDC, hrgn_org);
				if (hrgn_org) DeleteObject(hrgn_org);
			} else if (calcWidthCol==col || calcWidthCol==COLUMNS) {
				int right = img_pos + IMAGE_WIDTH - TREE_LINE_DX;

				if (right > pane->widths[col])
					pane->widths[col] = right;
			}
		} else  {
			img_pos = dis->rcItem.left;
		}
	} else {
		img_pos = dis->rcItem.left;

		if (calcWidthCol==col || calcWidthCol==COLUMNS)
			pane->widths[col] = IMAGE_WIDTH;
	}

	if (calcWidthCol == -1) {
		focusRect.left = img_pos -2;

		if (attrs & FILE_ATTRIBUTE_COMPRESSED)
			textcolor = COLOR_COMPRESSED;
		else
			textcolor = RGB(0,0,0);

		if (dis->itemState & ODS_FOCUS) {
			textcolor = RGB(255,255,255);
			bkcolor = COLOR_SELECTION;
		} else {
			bkcolor = RGB(255,255,255);
		}

		hbrush = CreateSolidBrush(bkcolor);
		FillRect(dis->hDC, &focusRect, hbrush);
		DeleteObject(hbrush);

		SetBkMode(dis->hDC, TRANSPARENT);
		SetTextColor(dis->hDC, textcolor);

		cx = pane->widths[col];

		if (cx && img!=IMG_NONE) {
			if (cx > IMAGE_WIDTH)
				cx = IMAGE_WIDTH;

			if (entry->hicon && entry->hicon!=(HICON)-1)
				DrawIconEx(dis->hDC, img_pos, dis->rcItem.top, entry->hicon, cx, GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
			else
				ImageList_DrawEx(Globals.himl, img, dis->hDC,
								 img_pos, dis->rcItem.top, cx,
								 IMAGE_HEIGHT, bkcolor, CLR_DEFAULT, ILD_NORMAL);
		}
	}

	if (!entry)
		return;

	col++;

	/* output file name */
	if (calcWidthCol == -1)
		output_text(pane, dis, col, entry->data.cFileName, 0);
	else if (calcWidthCol==col || calcWidthCol==COLUMNS)
		calc_width(pane, dis, col, entry->data.cFileName);

	col++;

        /* display file size */
	if (visible_cols & COL_SIZE) {
                format_longlong( buffer, ((ULONGLONG)entry->data.nFileSizeHigh << 32) | entry->data.nFileSizeLow );

                if (calcWidthCol == -1)
                        output_number(pane, dis, col, buffer);
                else if (calcWidthCol==col || calcWidthCol==COLUMNS)
                        calc_width(pane, dis, col, buffer);/*TODO: not ever time enough */

		col++;
	}

	/* display file date */
	if (visible_cols & (COL_DATE|COL_TIME)) {
		format_date(&entry->data.ftCreationTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			output_text(pane, dis, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(pane, dis, col, buffer);
		col++;

		format_date(&entry->data.ftLastAccessTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			output_text(pane, dis, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(pane, dis, col, buffer);
		col++;

		format_date(&entry->data.ftLastWriteTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			output_text(pane, dis, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(pane, dis, col, buffer);
		col++;
	}

	if (entry->bhfi_valid) {
		if (visible_cols & COL_INDEX) {
                        static const WCHAR fmtlow[] = {'%','X',0};
                        static const WCHAR fmthigh[] = {'%','X','%','0','8','X',0};

                        if (entry->bhfi.nFileIndexHigh)
                            wsprintfW(buffer, fmthigh,
                                     entry->bhfi.nFileIndexHigh, entry->bhfi.nFileIndexLow );
                        else
                            wsprintfW(buffer, fmtlow, entry->bhfi.nFileIndexLow );

			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_RIGHT);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);

			col++;
		}

		if (visible_cols & COL_LINKS) {
			wsprintfW(buffer, sNumFmt, entry->bhfi.nNumberOfLinks);

			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_CENTER);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);

			col++;
		}
	} else
		col += 2;

	/* show file attributes */
	if (visible_cols & COL_ATTRIBUTES) {
		static const WCHAR s11Tabs[] = {' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\0'};
		lstrcpyW(buffer, s11Tabs);

		if (attrs & FILE_ATTRIBUTE_NORMAL)					buffer[ 0] = 'N';
		else {
			if (attrs & FILE_ATTRIBUTE_READONLY)			buffer[ 2] = 'R';
			if (attrs & FILE_ATTRIBUTE_HIDDEN)				buffer[ 4] = 'H';
			if (attrs & FILE_ATTRIBUTE_SYSTEM)				buffer[ 6] = 'S';
			if (attrs & FILE_ATTRIBUTE_ARCHIVE)				buffer[ 8] = 'A';
			if (attrs & FILE_ATTRIBUTE_COMPRESSED)			buffer[10] = 'C';
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)			buffer[12] = 'D';
			if (attrs & FILE_ATTRIBUTE_ENCRYPTED)			buffer[14] = 'E';
			if (attrs & FILE_ATTRIBUTE_TEMPORARY)			buffer[16] = 'T';
			if (attrs & FILE_ATTRIBUTE_SPARSE_FILE)			buffer[18] = 'P';
			if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)		buffer[20] = 'Q';
			if (attrs & FILE_ATTRIBUTE_OFFLINE)				buffer[22] = 'O';
			if (attrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)	buffer[24] = 'X';
		}

		if (calcWidthCol == -1)
			output_tabbed_text(pane, dis, col, buffer);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_tabbed_width(pane, dis, col, buffer);

		col++;
	}
}

static void set_header(Pane* pane)
{
	HDITEMW item;
	int scroll_pos = GetScrollPos(pane->hwnd, SB_HORZ);
	int i=0, x=0;

	item.mask = HDI_WIDTH;
	item.cxy = 0;

	for(; (i < COLUMNS) && (x+pane->widths[i] < scroll_pos); i++) {
		x += pane->widths[i];
		SendMessageW(pane->hwndHeader, HDM_SETITEMW, i, (LPARAM)&item);
	}

	if (i < COLUMNS) {
		x += pane->widths[i];
		item.cxy = x - scroll_pos;
		SendMessageW(pane->hwndHeader, HDM_SETITEMW, i++, (LPARAM)&item);

		for(; i < COLUMNS; i++) {
			item.cxy = pane->widths[i];
			x += pane->widths[i];
			SendMessageW(pane->hwndHeader, HDM_SETITEMW, i, (LPARAM)&item);
		}
	}
}

static LRESULT pane_notify(Pane* pane, NMHDR* pnmh)
{
	switch(pnmh->code) {
		case HDN_ITEMCHANGEDW: {
			LPNMHEADERW phdn = (LPNMHEADERW)pnmh;
			int idx = phdn->iItem;
			int dx = phdn->pitem->cxy - pane->widths[idx];
			int i;

			RECT clnt;
			GetClientRect(pane->hwnd, &clnt);

			pane->widths[idx] += dx;

			for(i=idx; ++i<=COLUMNS; )
				pane->positions[i] += dx;

			{
				int scroll_pos = GetScrollPos(pane->hwnd, SB_HORZ);
				RECT rt_scr;
				RECT rt_clip;

				rt_scr.left   = pane->positions[idx+1]-scroll_pos;
				rt_scr.top    = 0;
				rt_scr.right  = clnt.right;
				rt_scr.bottom = clnt.bottom;

				rt_clip.left   = pane->positions[idx]-scroll_pos;
				rt_clip.top    = 0;
				rt_clip.right  = clnt.right;
				rt_clip.bottom = clnt.bottom;

				if (rt_scr.left < 0) rt_scr.left = 0;
				if (rt_clip.left < 0) rt_clip.left = 0;

				ScrollWindowEx(pane->hwnd, dx, 0, &rt_scr, &rt_clip, 0, 0, SW_INVALIDATE);

				rt_clip.right = pane->positions[idx+1];
				RedrawWindow(pane->hwnd, &rt_clip, 0, RDW_INVALIDATE|RDW_UPDATENOW);

				if (pnmh->code == HDN_ENDTRACKW) {
					SendMessageW(pane->hwnd, LB_SETHORIZONTALEXTENT, pane->positions[COLUMNS], 0);

					if (GetScrollPos(pane->hwnd, SB_HORZ) != scroll_pos)
						set_header(pane);
				}
			}

			return FALSE;
		}

		case HDN_DIVIDERDBLCLICKW: {
			LPNMHEADERW phdn = (LPNMHEADERW)pnmh;
			HDITEMW item;

			calc_single_width(pane, phdn->iItem);
			item.mask = HDI_WIDTH;
			item.cxy = pane->widths[phdn->iItem];

			SendMessageW(pane->hwndHeader, HDM_SETITEMW, phdn->iItem, (LPARAM)&item);
			InvalidateRect(pane->hwnd, 0, TRUE);
			break;}
	}

	return 0;
}

static void scan_entry(ChildWnd* child, Entry* entry, int idx, HWND hwnd)
{
	WCHAR path[MAX_PATH];
	HCURSOR old_cursor = SetCursor(LoadCursorW(0, (LPCWSTR)IDC_WAIT));

	/* delete sub entries in left pane */
	for(;;) {
		LRESULT res = SendMessageW(child->left.hwnd, LB_GETITEMDATA, idx+1, 0);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=entry->level)
			break;

		SendMessageW(child->left.hwnd, LB_DELETESTRING, idx+1, 0);
	}

	/* empty right pane */
	SendMessageW(child->right.hwnd, LB_RESETCONTENT, 0, 0);

	/* release memory */
	free_entries(entry);

	/* read contents from disk */
	if (entry->etype == ET_SHELL)
	{
		read_directory(entry, NULL, child->sortOrder, hwnd);
	}
	else
	{
		get_path(entry, path);
		read_directory(entry, path, child->sortOrder, hwnd);
	}

	/* insert found entries in right pane */
	insert_entries(&child->right, entry->down, child->filter_pattern, child->filter_flags, -1);
	calc_widths(&child->right, FALSE);
	set_header(&child->right);

	child->header_wdths_ok = FALSE;

	SetCursor(old_cursor);
}


/* expand a directory entry */

static BOOL expand_entry(ChildWnd* child, Entry* dir)
{
	int idx;
	Entry* p;

	if (!dir || dir->expanded || !dir->down)
		return FALSE;

	p = dir->down;

	if (p->data.cFileName[0]=='.' && p->data.cFileName[1]=='\0' && p->next) {
		p = p->next;

		if (p->data.cFileName[0]=='.' && p->data.cFileName[1]=='.' &&
				p->data.cFileName[2]=='\0' && p->next)
			p = p->next;
	}

	/* no subdirectories ? */
	if (!(p->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;

	idx = SendMessageW(child->left.hwnd, LB_FINDSTRING, 0, (LPARAM)dir);

	dir->expanded = TRUE;

	/* insert entries in left pane */
	insert_entries(&child->left, p, NULL, TF_ALL, idx);

	if (!child->header_wdths_ok) {
		if (calc_widths(&child->left, FALSE)) {
			set_header(&child->left);

			child->header_wdths_ok = TRUE;
		}
	}

	return TRUE;
}


static void collapse_entry(Pane* pane, Entry* dir)
{
        int idx;

        if (!dir) return;
        idx = SendMessageW(pane->hwnd, LB_FINDSTRING, 0, (LPARAM)dir);

	ShowWindow(pane->hwnd, SW_HIDE);

	/* hide sub entries */
	for(;;) {
		LRESULT res = SendMessageW(pane->hwnd, LB_GETITEMDATA, idx+1, 0);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=dir->level)
			break;

		SendMessageW(pane->hwnd, LB_DELETESTRING, idx+1, 0);
	}

	dir->expanded = FALSE;

	ShowWindow(pane->hwnd, SW_SHOW);
}


static void refresh_right_pane(ChildWnd* child)
{
	SendMessageW(child->right.hwnd, LB_RESETCONTENT, 0, 0);
	insert_entries(&child->right, child->right.root, child->filter_pattern, child->filter_flags, -1);
	calc_widths(&child->right, FALSE);

	set_header(&child->right);
}

static void set_curdir(ChildWnd* child, Entry* entry, int idx, HWND hwnd)
{
	WCHAR path[MAX_PATH];

	if (!entry)
		return;

	path[0] = '\0';

	child->left.cur = entry;

	child->right.root = entry->down? entry->down: entry;
	child->right.cur = entry;

	if (!entry->scanned)
		scan_entry(child, entry, idx, hwnd);
	else
		refresh_right_pane(child);

	get_path(entry, path);
	lstrcpyW(child->path, path);

	if (child->hwnd)	/* only change window title, if the window already exists */
		SetWindowTextW(child->hwnd, path);

	if (path[0])
		if (SetCurrentDirectoryW(path))
			set_space_status();
}


static void refresh_child(ChildWnd* child)
{
	WCHAR path[MAX_PATH], drv[_MAX_DRIVE+1];
	Entry* entry;
	int idx;

	get_path(child->left.cur, path);
	_wsplitpath(path, drv, NULL, NULL, NULL);

	child->right.root = NULL;

	scan_entry(child, &child->root.entry, 0, child->hwnd);

	if (child->root.entry.etype == ET_SHELL)
	{
		LPITEMIDLIST local_pidl = get_path_pidl(path,child->hwnd);
		if (local_pidl)
			entry = read_tree(&child->root, NULL, local_pidl , drv, child->sortOrder, child->hwnd);
		else
			entry = NULL;
	}
	else
		entry = read_tree(&child->root, path, NULL, drv, child->sortOrder, child->hwnd);

	if (!entry)
		entry = &child->root.entry;

	insert_entries(&child->left, child->root.entry.down, NULL, TF_ALL, 0);

	set_curdir(child, entry, 0, child->hwnd);

	idx = SendMessageW(child->left.hwnd, LB_FINDSTRING, 0, (LPARAM)child->left.cur);
	SendMessageW(child->left.hwnd, LB_SETCURSEL, idx, 0);
}


static void create_drive_bar(void)
{
	TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0};
	WCHAR b1[BUFFER_LEN];
	int btn = 1;
	PWSTR p;

	GetLogicalDriveStringsW(BUFFER_LEN, Globals.drives);

	Globals.hdrivebar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,
				IDW_DRIVEBAR, 2, Globals.hInstance, IDB_DRIVEBAR, &drivebarBtn,
				0, 16, 13, 16, 13, sizeof(TBBUTTON));

#ifdef __WINE__
	/* insert unix file system button */
	b1[0] = '/';
	b1[1] = '\0';
	b1[2] = '\0';
	SendMessageW(Globals.hdrivebar, TB_ADDSTRINGW, 0, (LPARAM)b1);

	drivebarBtn.idCommand = ID_DRIVE_UNIX_FS;
	SendMessageW(Globals.hdrivebar, TB_INSERTBUTTONW, btn++, (LPARAM)&drivebarBtn);
	drivebarBtn.iString++;
#endif
	/* insert shell namespace button */
	load_string(b1, sizeof(b1)/sizeof(b1[0]), IDS_SHELL);
	b1[lstrlenW(b1)+1] = '\0';
	SendMessageW(Globals.hdrivebar, TB_ADDSTRINGW, 0, (LPARAM)b1);

	drivebarBtn.idCommand = ID_DRIVE_SHELL_NS;
	SendMessageW(Globals.hdrivebar, TB_INSERTBUTTONW, btn++, (LPARAM)&drivebarBtn);
	drivebarBtn.iString++;

	/* register windows drive root strings */
	SendMessageW(Globals.hdrivebar, TB_ADDSTRINGW, 0, (LPARAM)Globals.drives);

	drivebarBtn.idCommand = ID_DRIVE_FIRST;

	for(p=Globals.drives; *p; ) {
		switch(GetDriveTypeW(p)) {
			case DRIVE_REMOVABLE:	drivebarBtn.iBitmap = 1;	break;
			case DRIVE_CDROM:		drivebarBtn.iBitmap = 3;	break;
			case DRIVE_REMOTE:		drivebarBtn.iBitmap = 4;	break;
			case DRIVE_RAMDISK:		drivebarBtn.iBitmap = 5;	break;
			default:/*DRIVE_FIXED*/	drivebarBtn.iBitmap = 2;
		}

		SendMessageW(Globals.hdrivebar, TB_INSERTBUTTONW, btn++, (LPARAM)&drivebarBtn);
		drivebarBtn.idCommand++;
		drivebarBtn.iString++;

		while(*p++);
	}
}

static void refresh_drives(void)
{
	RECT rect;

	/* destroy drive bar */
	DestroyWindow(Globals.hdrivebar);
	Globals.hdrivebar = 0;

	/* re-create drive bar */
	create_drive_bar();

	/* update window layout */
	GetClientRect(Globals.hMainWnd, &rect);
	SendMessageW(Globals.hMainWnd, WM_SIZE, 0, MAKELONG(rect.right, rect.bottom));
}


static BOOL launch_file(HWND hwnd, LPCWSTR cmd, UINT nCmdShow)
{
	HINSTANCE hinst = ShellExecuteW(hwnd, NULL/*operation*/, cmd, NULL/*parameters*/, NULL/*dir*/, nCmdShow);

	if (PtrToUlong(hinst) <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}


static BOOL launch_entry(Entry* entry, HWND hwnd, UINT nCmdShow)
{
	WCHAR cmd[MAX_PATH];

	if (entry->etype == ET_SHELL) {
		BOOL ret = TRUE;

		SHELLEXECUTEINFOW shexinfo;

		shexinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
		shexinfo.fMask = SEE_MASK_IDLIST;
		shexinfo.hwnd = hwnd;
		shexinfo.lpVerb = NULL;
		shexinfo.lpFile = NULL;
		shexinfo.lpParameters = NULL;
		shexinfo.lpDirectory = NULL;
		shexinfo.nShow = nCmdShow;
		shexinfo.lpIDList = get_to_absolute_pidl(entry, hwnd);

		if (!ShellExecuteExW(&shexinfo)) {
			display_error(hwnd, GetLastError());
			ret = FALSE;
		}

		if (shexinfo.lpIDList != entry->pidl)
			IMalloc_Free(Globals.iMalloc, shexinfo.lpIDList);

		return ret;
	}

	get_path(entry, cmd);

	 /* start program, open document... */
	return launch_file(hwnd, cmd, nCmdShow);
}


static void activate_entry(ChildWnd* child, Pane* pane, HWND hwnd)
{
	Entry* entry = pane->cur;

	if (!entry)
		return;

	if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		int scanned_old = entry->scanned;

		if (!scanned_old)
		{
			int idx = SendMessageW(child->left.hwnd, LB_GETCURSEL, 0, 0);
			scan_entry(child, entry, idx, hwnd);
		}

		if (entry->data.cFileName[0]=='.' && entry->data.cFileName[1]=='\0')
			return;

		if (entry->data.cFileName[0]=='.' && entry->data.cFileName[1]=='.' && entry->data.cFileName[2]=='\0') {
			entry = child->left.cur->up;
			collapse_entry(&child->left, entry);
			goto focus_entry;
		} else if (entry->expanded)
			collapse_entry(pane, child->left.cur);
		else {
			expand_entry(child, child->left.cur);

			if (!pane->treePane) focus_entry: {
				int idxstart = SendMessageW(child->left.hwnd, LB_GETCURSEL, 0, 0);
				int idx = SendMessageW(child->left.hwnd, LB_FINDSTRING, idxstart, (LPARAM)entry);
				SendMessageW(child->left.hwnd, LB_SETCURSEL, idx, 0);
				set_curdir(child, entry, idx, hwnd);
			}
		}

		if (!scanned_old) {
			calc_widths(pane, FALSE);

			set_header(pane);
		}
	} else {
		if (GetKeyState(VK_MENU) < 0)
			show_properties_dlg(entry, child->hwnd);
		else
			launch_entry(entry, child->hwnd, SW_SHOWNORMAL);
	}
}


static BOOL pane_command(Pane* pane, UINT cmd)
{
	switch(cmd) {
		case ID_VIEW_NAME:
			if (pane->visible_cols) {
				pane->visible_cols = 0;
				calc_widths(pane, TRUE);
				set_header(pane);
				InvalidateRect(pane->hwnd, 0, TRUE);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND|MF_CHECKED);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND);
			}
			break;

		case ID_VIEW_ALL_ATTRIBUTES:
			if (pane->visible_cols != COL_ALL) {
				pane->visible_cols = COL_ALL;
				calc_widths(pane, TRUE);
				set_header(pane);
				InvalidateRect(pane->hwnd, 0, TRUE);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND|MF_CHECKED);
			}
			break;

		case ID_PREFERRED_SIZES: {
			calc_widths(pane, TRUE);
			set_header(pane);
			InvalidateRect(pane->hwnd, 0, TRUE);
			break;}

		        /* TODO: more command ids... */

		default:
			return FALSE;
	}

	return TRUE;
}


static void set_sort_order(ChildWnd* child, SORT_ORDER sortOrder)
{
	if (child->sortOrder != sortOrder) {
		child->sortOrder = sortOrder;
		refresh_child(child);
	}
}

static void update_view_menu(ChildWnd* child)
{
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_NAME, child->sortOrder==SORT_NAME? MF_CHECKED: MF_UNCHECKED);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_TYPE, child->sortOrder==SORT_EXT? MF_CHECKED: MF_UNCHECKED);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_SIZE, child->sortOrder==SORT_SIZE? MF_CHECKED: MF_UNCHECKED);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_DATE, child->sortOrder==SORT_DATE? MF_CHECKED: MF_UNCHECKED);
}


static BOOL is_directory(LPCWSTR target)
{
	/*TODO correctly handle UNIX paths */
	DWORD target_attr = GetFileAttributesW(target);

	if (target_attr == INVALID_FILE_ATTRIBUTES)
		return FALSE;

	return (target_attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static BOOL prompt_target(Pane* pane, LPWSTR source, LPWSTR target)
{
	WCHAR path[MAX_PATH];
	int len;

	get_path(pane->cur, path);

	if (DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_SELECT_DESTINATION), pane->hwnd, DestinationDlgProc, (LPARAM)path) != IDOK)
		return FALSE;

	get_path(pane->cur, source);

	/* convert relative targets to absolute paths */
	if (path[0]!='/' && path[1]!=':') {
		get_path(pane->cur->up, target);
		len = lstrlenW(target);

		if (target[len-1]!='\\' && target[len-1]!='/')
			target[len++] = '/';

		lstrcpyW(target+len, path);
	} else
		lstrcpyW(target, path);

	/* If the target already exists as directory, create a new target below this. */
	if (is_directory(path)) {
		WCHAR fname[_MAX_FNAME], ext[_MAX_EXT];
		static const WCHAR sAppend[] = {'%','s','/','%','s','%','s','\0'};

		_wsplitpath(source, NULL, NULL, fname, ext);

		wsprintfW(target, sAppend, path, fname, ext);
	}

	return TRUE;
}


static IContextMenu2* s_pctxmenu2 = NULL;
static IContextMenu3* s_pctxmenu3 = NULL;

static void CtxMenu_reset(void)
{
	s_pctxmenu2 = NULL;
	s_pctxmenu3 = NULL;
}

static IContextMenu* CtxMenu_query_interfaces(IContextMenu* pcm1)
{
	IContextMenu* pcm = NULL;

	CtxMenu_reset();

	if (IContextMenu_QueryInterface(pcm1, &IID_IContextMenu3, (void**)&pcm) == NOERROR)
		s_pctxmenu3 = (LPCONTEXTMENU3)pcm;
	else if (IContextMenu_QueryInterface(pcm1, &IID_IContextMenu2, (void**)&pcm) == NOERROR)
		s_pctxmenu2 = (LPCONTEXTMENU2)pcm;

	if (pcm) {
		IContextMenu_Release(pcm1);
		return pcm;
	} else
		return pcm1;
}

static BOOL CtxMenu_HandleMenuMsg(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	if (s_pctxmenu3) {
		if (SUCCEEDED(IContextMenu3_HandleMenuMsg(s_pctxmenu3, nmsg, wparam, lparam)))
			return TRUE;
	}

	if (s_pctxmenu2)
		if (SUCCEEDED(IContextMenu2_HandleMenuMsg(s_pctxmenu2, nmsg, wparam, lparam)))
			return TRUE;

	return FALSE;
}

static HRESULT ShellFolderContextMenu(IShellFolder* shell_folder, HWND hwndParent, int cidl, LPCITEMIDLIST* apidl, int x, int y)
{
	IContextMenu* pcm;
	BOOL executed = FALSE;

	HRESULT hr = IShellFolder_GetUIObjectOf(shell_folder, hwndParent, cidl, apidl, &IID_IContextMenu, NULL, (LPVOID*)&pcm);

	if (SUCCEEDED(hr)) {
		HMENU hmenu = CreatePopupMenu();

		pcm = CtxMenu_query_interfaces(pcm);

		if (hmenu) {
			hr = IContextMenu_QueryContextMenu(pcm, hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, CMF_NORMAL);

			if (SUCCEEDED(hr)) {
				UINT idCmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON, x, y, 0, hwndParent, NULL);

				CtxMenu_reset();

				if (idCmd) {
				  CMINVOKECOMMANDINFO cmi;

				  cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
				  cmi.fMask = 0;
				  cmi.hwnd = hwndParent;
				  cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - FCIDM_SHVIEWFIRST);
				  cmi.lpParameters = NULL;
				  cmi.lpDirectory = NULL;
				  cmi.nShow = SW_SHOWNORMAL;
				  cmi.dwHotKey = 0;
				  cmi.hIcon = 0;

				  hr = IContextMenu_InvokeCommand(pcm, &cmi);
					executed = TRUE;
				}
			} else
				CtxMenu_reset();
		}

		IContextMenu_Release(pcm);
	}

	return FAILED(hr)? hr: executed? S_OK: S_FALSE;
}

static LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	ASSERT(child);

	switch(nmsg) {
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lparam;
			Entry* entry = (Entry*) dis->itemData;

			if (dis->CtlID == IDW_TREE_LEFT)
				draw_item(&child->left, dis, entry, -1);
			else if (dis->CtlID == IDW_TREE_RIGHT)
				draw_item(&child->right, dis, entry, -1);
			else
				goto draw_menu_item;

			return TRUE;}

		case WM_CREATE:
			InitChildWindow(child);
			break;

		case WM_NCDESTROY:
			free_child_window(child);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HBRUSH lastBrush;
			RECT rt;
			GetClientRect(hwnd, &rt);
			BeginPaint(hwnd, &ps);
			rt.left = child->split_pos-SPLIT_WIDTH/2;
			rt.right = child->split_pos+SPLIT_WIDTH/2+1;
			lastBrush = SelectObject(ps.hdc, GetStockObject(COLOR_SPLITBAR));
			Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
			SelectObject(ps.hdc, lastBrush);
			EndPaint(hwnd, &ps);
			break;}

		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);

				if (pt.x>=child->split_pos-SPLIT_WIDTH/2 && pt.x<child->split_pos+SPLIT_WIDTH/2+1) {
					SetCursor(LoadCursorW(0, (LPCWSTR)IDC_SIZEWE));
					return TRUE;
				}
			}
			goto def;

		case WM_LBUTTONDOWN: {
			RECT rt;
                        int x = (short)LOWORD(lparam);

			GetClientRect(hwnd, &rt);

			if (x>=child->split_pos-SPLIT_WIDTH/2 && x<child->split_pos+SPLIT_WIDTH/2+1) {
				last_split = child->split_pos;
				SetCapture(hwnd);
			}

			break;}

		case WM_LBUTTONUP:
                        if (GetCapture() == hwnd)
                                ReleaseCapture();
			break;

		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
				if (GetCapture() == hwnd) {
					RECT rt;
					child->split_pos = last_split;
					GetClientRect(hwnd, &rt);
					resize_tree(child, rt.right, rt.bottom);
					last_split = -1;
					ReleaseCapture();
					SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));
				}
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == hwnd) {
				RECT rt;
                                int x = (short)LOWORD(lparam);

				GetClientRect(hwnd, &rt);

				if (x>=0 && x<rt.right) {
					child->split_pos = x;
					resize_tree(child, rt.right, rt.bottom);
					rt.left = x-SPLIT_WIDTH/2;
					rt.right = x+SPLIT_WIDTH/2+1;
					InvalidateRect(hwnd, &rt, FALSE);
					UpdateWindow(child->left.hwnd);
					UpdateWindow(hwnd);
					UpdateWindow(child->right.hwnd);
				}
			}
			break;

		case WM_GETMINMAXINFO:
			DefMDIChildProcW(hwnd, nmsg, wparam, lparam);

			{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

			lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
			lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
			break;}

		case WM_SETFOCUS:
			if (SetCurrentDirectoryW(child->path))
				set_space_status();
			SetFocus(child->focus_pane? child->right.hwnd: child->left.hwnd);
			break;

		case WM_DISPATCH_COMMAND: {
			Pane* pane = GetFocus()==child->left.hwnd? &child->left: &child->right;

			switch(LOWORD(wparam)) {
				case ID_WINDOW_NEW: {
					ChildWnd* new_child = alloc_child_window(child->path, NULL, hwnd);

					if (!create_child_window(new_child))
						HeapFree(GetProcessHeap(), 0, new_child);

					break;}

				case ID_REFRESH:
					refresh_drives();
					refresh_child(child);
					break;

				case ID_ACTIVATE:
					activate_entry(child, pane, hwnd);
					break;

				case ID_FILE_MOVE: {
					WCHAR source[BUFFER_LEN], target[BUFFER_LEN];

					if (prompt_target(pane, source, target)) {
						SHFILEOPSTRUCTW shfo = {hwnd, FO_MOVE, source, target};

						source[lstrlenW(source)+1] = '\0';
						target[lstrlenW(target)+1] = '\0';

						if (!SHFileOperationW(&shfo))
							refresh_child(child);
					}
					break;}

				case ID_FILE_COPY: {
					WCHAR source[BUFFER_LEN], target[BUFFER_LEN];

					if (prompt_target(pane, source, target)) {
						SHFILEOPSTRUCTW shfo = {hwnd, FO_COPY, source, target};

						source[lstrlenW(source)+1] = '\0';
						target[lstrlenW(target)+1] = '\0';

						if (!SHFileOperationW(&shfo))
							refresh_child(child);
					}
					break;}

				case ID_FILE_DELETE: {
					WCHAR path[BUFFER_LEN];
					SHFILEOPSTRUCTW shfo = {hwnd, FO_DELETE, path, NULL, FOF_ALLOWUNDO};

					get_path(pane->cur, path);

					path[lstrlenW(path)+1] = '\0';

					if (!SHFileOperationW(&shfo))
						refresh_child(child);
					break;}

				case ID_VIEW_SORT_NAME:
					set_sort_order(child, SORT_NAME);
					break;

				case ID_VIEW_SORT_TYPE:
					set_sort_order(child, SORT_EXT);
					break;

				case ID_VIEW_SORT_SIZE:
					set_sort_order(child, SORT_SIZE);
					break;

				case ID_VIEW_SORT_DATE:
					set_sort_order(child, SORT_DATE);
					break;

				case ID_VIEW_FILTER: {
					struct FilterDialog dlg;

					memset(&dlg, 0, sizeof(struct FilterDialog));
					lstrcpyW(dlg.pattern, child->filter_pattern);
					dlg.flags = child->filter_flags;

					if (DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_DIALOG_VIEW_TYPE), hwnd, FilterDialogDlgProc, (LPARAM)&dlg) == IDOK) {
						lstrcpyW(child->filter_pattern, dlg.pattern);
						child->filter_flags = dlg.flags;
						refresh_right_pane(child);
					}
					break;}

				case ID_VIEW_SPLIT: {
					last_split = child->split_pos;
					SetCapture(hwnd);
					break;}

				case ID_EDIT_PROPERTIES:
					show_properties_dlg(pane->cur, child->hwnd);
					break;

				default:
					return pane_command(pane, LOWORD(wparam));
			}

			return TRUE;}

		case WM_COMMAND: {
			Pane* pane = GetFocus()==child->left.hwnd? &child->left: &child->right;

			switch(HIWORD(wparam)) {
				case LBN_SELCHANGE: {
					int idx = SendMessageW(pane->hwnd, LB_GETCURSEL, 0, 0);
					Entry* entry = (Entry*)SendMessageW(pane->hwnd, LB_GETITEMDATA, idx, 0);

					if (pane == &child->left)
						set_curdir(child, entry, idx, hwnd);
					else
						pane->cur = entry;
					break;}

				case LBN_DBLCLK:
					activate_entry(child, pane, hwnd);
					break;
			}
			break;}

		case WM_NOTIFY: {
			NMHDR* pnmh = (NMHDR*) lparam;
			return pane_notify(pnmh->idFrom==IDW_HEADER_LEFT? &child->left: &child->right, pnmh);}

		case WM_CONTEXTMENU: {
			POINT pt, pt_clnt;
			Pane* pane;
			int idx;

			 /* first select the current item in the listbox */
			HWND hpanel = (HWND) wparam;
			pt_clnt.x = pt.x = (short)LOWORD(lparam);
			pt_clnt.y = pt.y = (short)HIWORD(lparam);
			ScreenToClient(hpanel, &pt_clnt);
			SendMessageW(hpanel, WM_LBUTTONDOWN, 0, MAKELONG(pt_clnt.x, pt_clnt.y));
			SendMessageW(hpanel, WM_LBUTTONUP, 0, MAKELONG(pt_clnt.x, pt_clnt.y));

			 /* now create the popup menu using shell namespace and IContextMenu */
			pane = GetFocus()==child->left.hwnd? &child->left: &child->right;
			idx = SendMessageW(pane->hwnd, LB_GETCURSEL, 0, 0);

			if (idx != -1) {
				Entry* entry = (Entry*)SendMessageW(pane->hwnd, LB_GETITEMDATA, idx, 0);

				LPITEMIDLIST pidl_abs = get_to_absolute_pidl(entry, hwnd);

				if (pidl_abs) {
					IShellFolder* parentFolder;
					LPCITEMIDLIST pidlLast;

					 /* get and use the parent folder to display correct context menu in all cases */
					if (SUCCEEDED(SHBindToParent(pidl_abs, &IID_IShellFolder, (LPVOID*)&parentFolder, &pidlLast))) {
						if (ShellFolderContextMenu(parentFolder, hwnd, 1, &pidlLast, pt.x, pt.y) == S_OK)
							refresh_child(child);

						IShellFolder_Release(parentFolder);
					}

					IMalloc_Free(Globals.iMalloc, pidl_abs);
				}
			}
			break;}

		  case WM_MEASUREITEM:
		  draw_menu_item:
			if (!wparam)	/* Is the message menu-related? */
				if (CtxMenu_HandleMenuMsg(nmsg, wparam, lparam))
					return TRUE;

			break;

		  case WM_INITMENUPOPUP:
			if (CtxMenu_HandleMenuMsg(nmsg, wparam, lparam))
				return 0;

			update_view_menu(child);
			break;

		  case WM_MENUCHAR:	/* only supported by IContextMenu3 */
		   if (s_pctxmenu3) {
			   LRESULT lResult = 0;

			   IContextMenu3_HandleMenuMsg2(s_pctxmenu3, nmsg, wparam, lparam, &lResult);

			   return lResult;
		   }

		   break;

		case WM_SIZE:
			if (wparam != SIZE_MINIMIZED)
				resize_tree(child, LOWORD(lparam), HIWORD(lparam));
			/* fall through */

		default: def:
			return DefMDIChildProcW(hwnd, nmsg, wparam, lparam);
	}

	return 0;
}


static LRESULT CALLBACK TreeWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLongPtrW(GetParent(hwnd), GWLP_USERDATA);
	Pane* pane = (Pane*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	ASSERT(child);

	switch(nmsg) {
		case WM_HSCROLL:
			set_header(pane);
			break;

		case WM_SETFOCUS:
			child->focus_pane = pane==&child->right? 1: 0;
			SendMessageW(hwnd, LB_SETSEL, TRUE, 1);
			/*TODO: check menu items */
			break;

		case WM_KEYDOWN:
			if (wparam == VK_TAB) {
				/*TODO: SetFocus(Globals.hdrivebar) */
				SetFocus(child->focus_pane? child->left.hwnd: child->right.hwnd);
			}
	}

	return CallWindowProcW(g_orgTreeWndProc, hwnd, nmsg, wparam, lparam);
}


static void InitInstance(HINSTANCE hinstance)
{
	static const WCHAR sFont[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f','\0'};

	WNDCLASSEXW wcFrame;
	WNDCLASSW wcChild;
	int col;

	INITCOMMONCONTROLSEX icc = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_BAR_CLASSES
	};

	HDC hdc = GetDC(0);

	setlocale(LC_COLLATE, "");	/* set collating rules to local settings for compareName */

	InitCommonControlsEx(&icc);


	/* register frame window class */

	wcFrame.cbSize        = sizeof(WNDCLASSEXW);
	wcFrame.style         = 0;
	wcFrame.lpfnWndProc   = FrameWndProc;
	wcFrame.cbClsExtra    = 0;
	wcFrame.cbWndExtra    = 0;
	wcFrame.hInstance     = hinstance;
	wcFrame.hIcon         = LoadIconW(hinstance, MAKEINTRESOURCEW(IDI_WINEFILE));
	wcFrame.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
	wcFrame.hbrBackground = 0;
	wcFrame.lpszMenuName  = 0;
	wcFrame.lpszClassName = sWINEFILEFRAME;
	wcFrame.hIconSm       = LoadImageW(hinstance, MAKEINTRESOURCEW(IDI_WINEFILE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);

	Globals.hframeClass = RegisterClassExW(&wcFrame);


	/* register tree windows class */

	wcChild.style         = CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW;
	wcChild.lpfnWndProc   = ChildWndProc;
	wcChild.cbClsExtra    = 0;
	wcChild.cbWndExtra    = 0;
	wcChild.hInstance     = hinstance;
	wcChild.hIcon         = 0;
	wcChild.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
	wcChild.hbrBackground = 0;
	wcChild.lpszMenuName  = 0;
	wcChild.lpszClassName = sWINEFILETREE;

	RegisterClassW(&wcChild);


	Globals.haccel = LoadAcceleratorsW(hinstance, MAKEINTRESOURCEW(IDA_WINEFILE));

	Globals.hfont = CreateFontW(-MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sFont);

	ReleaseDC(0, hdc);

	Globals.hInstance = hinstance;

	CoInitialize(NULL);
	CoGetMalloc(MEMCTX_TASK, &Globals.iMalloc);
	SHGetDesktopFolder(&Globals.iDesktop);
	Globals.cfStrFName = RegisterClipboardFormatW(CFSTR_FILENAMEW);

	/* load column strings */
	col = 1;

	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_NAME);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_SIZE);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_CDATE);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_ADATE);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_MDATE);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_IDX);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_LINKS);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_ATTR);
	load_string(g_pos_names[col++], sizeof(g_pos_names[col])/sizeof(g_pos_names[col][0]), IDS_COL_SEC);
}


static BOOL show_frame(HWND hwndParent, int cmdshow, LPCWSTR path)
{
	static const WCHAR sMDICLIENT[] = {'M','D','I','C','L','I','E','N','T','\0'};

	WCHAR buffer[MAX_PATH], b1[BUFFER_LEN];
	ChildWnd* child;
	HMENU hMenuFrame, hMenuWindow;
	windowOptions opts;

	CLIENTCREATESTRUCT ccs;

	if (Globals.hMainWnd)
		return TRUE;

	opts = load_registry_settings();
	hMenuFrame = LoadMenuW(Globals.hInstance, MAKEINTRESOURCEW(IDM_WINEFILE));
	hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);

	Globals.hMenuFrame = hMenuFrame;
	Globals.hMenuView = GetSubMenu(hMenuFrame, 2);
	Globals.hMenuOptions = GetSubMenu(hMenuFrame, 3);

	ccs.hWindowMenu  = hMenuWindow;
	ccs.idFirstChild = IDW_FIRST_CHILD;


	/* create main window */
	Globals.hMainWnd = CreateWindowExW(0, MAKEINTRESOURCEW(Globals.hframeClass), RS(b1,IDS_WINEFILE), WS_OVERLAPPEDWINDOW,
					opts.start_x, opts.start_y, opts.width, opts.height,
					hwndParent, Globals.hMenuFrame, Globals.hInstance, 0/*lpParam*/);


	Globals.hmdiclient = CreateWindowExW(0, sMDICLIENT, NULL,
					WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
					0, 0, 0, 0,
					Globals.hMainWnd, 0, Globals.hInstance, &ccs);

	CheckMenuItem(Globals.hMenuOptions, ID_VIEW_DRIVE_BAR, MF_BYCOMMAND|MF_CHECKED);
	CheckMenuItem(Globals.hMenuOptions, ID_VIEW_SAVESETTINGS, MF_BYCOMMAND);

	create_drive_bar();

	{
		TBBUTTON toolbarBtns[] = {
			{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
			{0, ID_WINDOW_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		};

		Globals.htoolbar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE,
			IDW_TOOLBAR, 2, Globals.hInstance, IDB_TOOLBAR, toolbarBtns,
			sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));
		CheckMenuItem(Globals.hMenuOptions, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);
	}

	Globals.hstatusbar = CreateStatusWindowW(WS_CHILD|WS_VISIBLE, 0, Globals.hMainWnd, IDW_STATUSBAR);
	CheckMenuItem(Globals.hMenuOptions, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

	/*TODO: read paths from registry */

	if (!path || !*path) {
		GetCurrentDirectoryW(MAX_PATH, buffer);
		path = buffer;
	}

	ShowWindow(Globals.hMainWnd, cmdshow);

#ifndef __WINE__
	 /* Shell Namespace as default: */
	child = alloc_child_window(path, get_path_pidl((LPWSTR)path,Globals.hMainWnd), Globals.hMainWnd);
#else
	child = alloc_child_window(path, NULL, Globals.hMainWnd);
#endif

	child->pos.showCmd = SW_SHOWMAXIMIZED;
	child->pos.rcNormalPosition.left = 0;
	child->pos.rcNormalPosition.top = 0;
	child->pos.rcNormalPosition.right = 320;
	child->pos.rcNormalPosition.bottom = 280;

	if (!create_child_window(child)) {
		HeapFree(GetProcessHeap(), 0, child);
		return FALSE;
	}

	SetWindowPlacement(child->hwnd, &child->pos);

	Globals.himl = ImageList_LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDB_IMAGES), 16, 0, RGB(0,255,0), IMAGE_BITMAP, 0);

	Globals.prescan_node = FALSE;

	UpdateWindow(Globals.hMainWnd);

	if (child->hwnd && path && path[0])
	{
		int index,count;
		WCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
		WCHAR fullname[_MAX_FNAME+_MAX_EXT+1];

		memset(name,0,sizeof(name));
		memset(name,0,sizeof(ext));
		_wsplitpath(path, drv, dir, name, ext);
		if (name[0])
		{
			count = SendMessageW(child->right.hwnd, LB_GETCOUNT, 0, 0);
			lstrcpyW(fullname,name);
			lstrcatW(fullname,ext);

			for (index = 0; index < count; index ++)
			{
				Entry* entry = (Entry*)SendMessageW(child->right.hwnd, LB_GETITEMDATA, index, 0);
				if (lstrcmpW(entry->data.cFileName,fullname)==0 ||
						lstrcmpW(entry->data.cAlternateFileName,fullname)==0)
				{
					SendMessageW(child->right.hwnd, LB_SETCURSEL, index, 0);
					SetFocus(child->right.hwnd);
					break;
				}
			}
		}
	}
	return TRUE;
}

static void ExitInstance(void)
{
	IShellFolder_Release(Globals.iDesktop);
	IMalloc_Release(Globals.iMalloc);
	CoUninitialize();

	DeleteObject(Globals.hfont);
	ImageList_Destroy(Globals.himl);
}

int APIENTRY wWinMain(HINSTANCE hinstance, HINSTANCE previnstance, LPWSTR cmdline, int cmdshow)
{
	MSG msg;

	InitInstance(hinstance);

        if( !show_frame(0, cmdshow, cmdline) )
	{
		ExitInstance();
		return 1;
	}

	while(GetMessageW(&msg, 0, 0, 0)) {
		if (Globals.hmdiclient && TranslateMDISysAccel(Globals.hmdiclient, &msg))
			continue;

		if (Globals.hMainWnd && TranslateAcceleratorW(Globals.hMainWnd, Globals.haccel, &msg))
			continue;

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	ExitInstance();

	return msg.wParam;
}
