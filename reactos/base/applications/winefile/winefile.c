/*
 * Winefile
 *
 * Copyright 2000, 2003, 2004, 2005 Martin Fuchs
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


#ifdef _NO_EXTENSIONS
#undef _LEFT_FILES
#endif

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


#ifdef _SHELL_FOLDERS
#define	DEFAULT_SPLIT_POS	300
#else
#define	DEFAULT_SPLIT_POS	200
#endif


enum ENTRY_TYPE {
	ET_WINDOWS,
	ET_UNIX,
#ifdef _SHELL_FOLDERS
	ET_SHELL
#endif
};

typedef struct _Entry {
	struct _Entry*	next;
	struct _Entry*	down;
	struct _Entry*	up;

	BOOL			expanded;
	BOOL			scanned;
	int				level;

	WIN32_FIND_DATA	data;

#ifndef _NO_EXTENSIONS
	BY_HANDLE_FILE_INFORMATION bhfi;
	BOOL			bhfi_valid;
	enum ENTRY_TYPE	etype;
#endif
#ifdef _SHELL_FOLDERS
	LPITEMIDLIST	pidl;
	IShellFolder*	folder;
	HICON			hicon;
#endif
} Entry;

typedef struct {
	Entry	entry;
	TCHAR	path[MAX_PATH];
	TCHAR	volname[_MAX_FNAME];
	TCHAR	fs[_MAX_DIR];
	DWORD	drive_type;
	DWORD	fs_flags;
} Root;

enum COLUMN_FLAGS {
	COL_SIZE		= 0x01,
	COL_DATE		= 0x02,
	COL_TIME		= 0x04,
	COL_ATTRIBUTES	= 0x08,
	COL_DOSNAMES	= 0x10,
#ifdef _NO_EXTENSIONS
	COL_ALL = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES
#else
	COL_INDEX		= 0x20,
	COL_LINKS		= 0x40,
	COL_ALL = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES|COL_INDEX|COL_LINKS
#endif
};

typedef enum {
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
} SORT_ORDER;

typedef struct {
	HWND	hwnd;
#ifndef _NO_EXTENSIONS
	HWND	hwndHeader;
#endif

#ifndef _NO_EXTENSIONS
#define	COLUMNS	10
#else
#define	COLUMNS	5
#endif
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

	TCHAR	path[MAX_PATH];
	TCHAR	filter_pattern[MAX_PATH];
	int		filter_flags;
	Root	root;

	SORT_ORDER sortOrder;
} ChildWnd;



static void read_directory(Entry* dir, LPCTSTR path, SORT_ORDER sortOrder, HWND hwnd);
static void set_curdir(ChildWnd* child, Entry* entry, int idx, HWND hwnd);
static void refresh_child(ChildWnd* child);
static void refresh_drives(void);
static void get_path(Entry* dir, PTSTR path);
static void format_date(const FILETIME* ft, TCHAR* buffer, int visible_cols);

static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK TreeWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam);


/* globals */
WINEFILE_GLOBALS Globals;

static int last_split;


/* some common string constants */
const static TCHAR sEmpty[] = {'\0'};
const static TCHAR sSpace[] = {' ', '\0'};
const static TCHAR sNumFmt[] = {'%','d','\0'};
const static TCHAR sQMarks[] = {'?','?','?','\0'};

/* window class names */
const static TCHAR sWINEFILEFRAME[] = {'W','F','S','_','F','r','a','m','e','\0'};
const static TCHAR sWINEFILETREE[] = {'W','F','S','_','T','r','e','e','\0'};

#ifdef _MSC_VER
/* #define LONGLONGARG _T("I64") */
const static TCHAR sLongHexFmt[] = {'%','I','6','4','X','\0'};
const static TCHAR sLongNumFmt[] = {'%','I','6','4','d','\0'};
#else
/* #define LONGLONGARG _T("L") */
const static TCHAR sLongHexFmt[] = {'%','L','X','\0'};
const static TCHAR sLongNumFmt[] = {'%','L','d','\0'};
#endif


/* load resource string */
static LPTSTR load_string(LPTSTR buffer, UINT id)
{
	LoadString(Globals.hInstance, id, buffer, BUFFER_LEN);

	return buffer;
}

#define	RS(b, i) load_string(b, i)


/* display error message for the specified WIN32 error code */
static void display_error(HWND hwnd, DWORD error)
{
	TCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];
	PTSTR msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		MessageBox(hwnd, msg, RS(b2,IDS_WINEFILE), MB_OK);
	else
		MessageBox(hwnd, RS(b1,IDS_ERROR), RS(b2,IDS_WINEFILE), MB_OK);

	LocalFree(msg);
}


/* display network error message using WNetGetLastError() */
static void display_network_error(HWND hwnd)
{
	TCHAR msg[BUFFER_LEN], provider[BUFFER_LEN], b2[BUFFER_LEN];
	DWORD error;

	if (WNetGetLastError(&error, msg, BUFFER_LEN, provider, BUFFER_LEN) == NO_ERROR)
		MessageBox(hwnd, msg, RS(b2,IDS_WINEFILE), MB_OK);
}


#ifdef __WINE__

static VOID WineLicense(HWND Wnd)
{
	TCHAR cap[20], text[1024];
	LoadString(Globals.hInstance, IDS_LICENSE, text, 1024);
	LoadString(Globals.hInstance, IDS_LICENSE_CAPTION, cap, 20);
	MessageBox(Wnd, text, cap, MB_ICONINFORMATION | MB_OK);
}

static VOID WineWarranty(HWND Wnd)
{
	TCHAR cap[20], text[1024];
	LoadString(Globals.hInstance, IDS_WARRANTY, text, 1024);
	LoadString(Globals.hInstance, IDS_WARRANTY_CAPTION, cap, 20);
	MessageBox(Wnd, text, cap, MB_ICONEXCLAMATION | MB_OK);
}

#ifdef UNICODE

/* call vswprintf() in msvcrt.dll */
/*TODO: fix swprintf() in non-msvcrt mode, so that this dynamic linking function can be removed */
static int msvcrt_swprintf(WCHAR* buffer, const WCHAR* fmt, ...)
{
	static int (__cdecl *pvswprintf)(WCHAR*, const WCHAR*, va_list) = NULL;
	va_list ap;
	int ret;

	if (!pvswprintf) {
		HMODULE hModMsvcrt = LoadLibraryA("msvcrt");
		pvswprintf = (int(__cdecl*)(WCHAR*,const WCHAR*,va_list)) GetProcAddress(hModMsvcrt, "vswprintf");
	}

	va_start(ap, fmt);
	ret = (*pvswprintf)(buffer, fmt, ap);
	va_end(ap);

	return ret;
}

static LPCWSTR my_wcsrchr(LPCWSTR str, WCHAR c)
{
	LPCWSTR p = str;

	while(*p)
		++p;

	do {
		if (--p < str)
			return NULL;
	} while(*p != c);

	return p;
}

#define _tcsrchr my_wcsrchr
#else	/* UNICODE */
#define _tcsrchr strrchr
#endif	/* UNICODE */

#endif	/* __WINE__ */


/* allocate and initialise a directory entry */
static Entry* alloc_entry(void)
{
	Entry* entry = (Entry*) malloc(sizeof(Entry));

#ifdef _SHELL_FOLDERS
	entry->pidl = NULL;
	entry->folder = NULL;
	entry->hicon = 0;
#endif

	return entry;
}

/* free a directory entry */
static void free_entry(Entry* entry)
{
#ifdef _SHELL_FOLDERS
	if (entry->hicon && entry->hicon!=(HICON)-1)
		DestroyIcon(entry->hicon);

	if (entry->folder && entry->folder!=Globals.iDesktop)
		IShellFolder_Release(entry->folder);

	if (entry->pidl)
		IMalloc_Free(Globals.iMalloc, entry->pidl);
#endif

	free(entry);
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


static void read_directory_win(Entry* dir, LPCTSTR path)
{
	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;

	int level = dir->level + 1;
	WIN32_FIND_DATA w32fd;
	HANDLE hFind;
#ifndef _NO_EXTENSIONS
	HANDLE hFile;
#endif

	TCHAR buffer[MAX_PATH], *p;
	for(p=buffer; *path; )
		*p++ = *path++;

	*p++ = '\\';
	p[0] = '*';
	p[1] = '\0';

	hFind = FindFirstFile(buffer, &w32fd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
#ifdef _NO_EXTENSIONS
			/* hide directory entry "." */
			if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				LPCTSTR name = w32fd.cFileName;

				if (name[0]=='.' && name[1]=='\0')
					continue;
			}
#endif
			entry = alloc_entry();

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->next = entry;

			memcpy(&entry->data, &w32fd, sizeof(WIN32_FIND_DATA));
			entry->down = NULL;
			entry->up = dir;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;

#ifndef _NO_EXTENSIONS
			entry->etype = ET_WINDOWS;
			entry->bhfi_valid = FALSE;

			lstrcpy(p, entry->data.cFileName);

			hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

			if (hFile != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(hFile, &entry->bhfi))
					entry->bhfi_valid = TRUE;

				CloseHandle(hFile);
			}
#endif

			last = entry;
		} while(FindNextFile(hFind, &w32fd));

		if (last)
			last->next = NULL;

		FindClose(hFind);
	}

	dir->down = first_entry;
	dir->scanned = TRUE;
}


static Entry* find_entry_win(Entry* dir, LPCTSTR name)
{
	Entry* entry;

	for(entry=dir->down; entry; entry=entry->next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->data.cFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->data.cAlternateFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return 0;
}


static Entry* read_tree_win(Root* root, LPCTSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCTSTR s = path;
	PTSTR d = buffer;

	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

#ifndef _NO_EXTENSIONS
	entry->etype = ET_WINDOWS;
#endif

	while(entry) {
		while(*s && *s!=TEXT('\\') && *s!=TEXT('/'))
			*d++ = *s++;

		while(*s==TEXT('\\') || *s==TEXT('/'))
			s++;

		*d++ = TEXT('\\');
		*d = TEXT('\0');

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


#if !defined(_NO_EXTENSIONS) && defined(__WINE__)

static BOOL time_to_filetime(const time_t* t, FILETIME* ftime)
{
	struct tm* tm = gmtime(t);
	SYSTEMTIME stime;

	if (!tm)
		return FALSE;

	stime.wYear = tm->tm_year+1900;
	stime.wMonth = tm->tm_mon+1;
	/* stime.wDayOfWeek */
	stime.wDay = tm->tm_mday;
	stime.wHour = tm->tm_hour;
	stime.wMinute = tm->tm_min;
	stime.wSecond = tm->tm_sec;

	return SystemTimeToFileTime(&stime, ftime);
}

static void read_directory_unix(Entry* dir, LPCTSTR path)
{
	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;
	DIR* pdir;

	int level = dir->level + 1;
#ifdef UNICODE
	char cpath[MAX_PATH];

	WideCharToMultiByte(CP_UNIXCP, 0, path, -1, cpath, MAX_PATH, NULL, NULL);
#else
	const char* cpath = path;
#endif

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
#ifdef UNICODE
			MultiByteToWideChar(CP_UNIXCP, 0, p, -1, entry->data.cFileName, MAX_PATH);
#else
			lstrcpy(entry->data.cFileName, p);
#endif

			if (!stat(buffer, &st)) {
				entry->data.dwFileAttributes = p[0]=='.'? FILE_ATTRIBUTE_HIDDEN: 0;

				if (S_ISDIR(st.st_mode))
					entry->data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				entry->data.nFileSizeLow = st.st_size & 0xFFFFFFFF;
				entry->data.nFileSizeHigh = st.st_size >> 32;

				memset(&entry->data.ftCreationTime, 0, sizeof(FILETIME));
				time_to_filetime(&st.st_atime, &entry->data.ftLastAccessTime);
				time_to_filetime(&st.st_mtime, &entry->data.ftLastWriteTime);

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

static Entry* find_entry_unix(Entry* dir, LPCTSTR name)
{
	Entry* entry;

	for(entry=dir->down; entry; entry=entry->next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->data.cFileName;

		do {
			if (!*p || *p==TEXT('/'))
				return entry;
		} while(*p++ == *q++);
	}

	return 0;
}

static Entry* read_tree_unix(Root* root, LPCTSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCTSTR s = path;
	PTSTR d = buffer;

	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

	entry->etype = ET_UNIX;

	while(entry) {
		while(*s && *s!=TEXT('/'))
			*d++ = *s++;

		while(*s == TEXT('/'))
			s++;

		*d++ = TEXT('/');
		*d = TEXT('\0');

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

#endif /* !defined(_NO_EXTENSIONS) && defined(__WINE__) */


#ifdef _SHELL_FOLDERS

#ifdef UNICODE
#define	get_strret get_strretW
#define	path_from_pidl path_from_pidlW
#else
#define	get_strret get_strretA
#define	path_from_pidl path_from_pidlA
#endif


static void free_strret(STRRET* str)
{
	if (str->uType == STRRET_WSTR)
		IMalloc_Free(Globals.iMalloc, str->UNION_MEMBER(pOleStr));
}


#ifndef UNICODE

static LPSTR strcpyn(LPSTR dest, LPCSTR source, size_t count)
{
 LPCSTR s;
 LPSTR d = dest;

 for(s=source; count&&(*d++=*s++); )
  count--;

 return dest;
}

static void get_strretA(STRRET* str, const SHITEMID* shiid, LPSTR buffer, int len)
{
 switch(str->uType) {
  case STRRET_WSTR:
	WideCharToMultiByte(CP_ACP, 0, str->UNION_MEMBER(pOleStr), -1, buffer, len, NULL, NULL);
	break;

  case STRRET_OFFSET:
	strcpyn(buffer, (LPCSTR)shiid+str->UNION_MEMBER(uOffset), len);
	break;

  case STRRET_CSTR:
	strcpyn(buffer, str->UNION_MEMBER(cStr), len);
 }
}

static HRESULT path_from_pidlA(IShellFolder* folder, LPITEMIDLIST pidl, LPSTR buffer, int len)
{
	STRRET str;

	 /* SHGDN_FORPARSING: get full path of id list */
	HRESULT hr = IShellFolder_GetDisplayNameOf(folder, pidl, SHGDN_FORPARSING, &str);

	if (SUCCEEDED(hr)) {
		get_strretA(&str, &pidl->mkid, buffer, len);
		free_strret(&str);
	} else
		buffer[0] = '\0';

	return hr;
}

#endif

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


static HRESULT name_from_pidl(IShellFolder* folder, LPITEMIDLIST pidl, LPTSTR buffer, int len, SHGDNF flags)
{
	STRRET str;

	HRESULT hr = IShellFolder_GetDisplayNameOf(folder, pidl, flags, &str);

	if (SUCCEEDED(hr)) {
		get_strret(&str, &pidl->mkid, buffer, len);
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

static LPITEMIDLIST get_path_pidl(LPTSTR path, HWND hwnd)
{
	LPITEMIDLIST pidl;
	HRESULT hr;
	ULONG len;

#ifdef UNICODE
	LPWSTR buffer = path;
#else
	WCHAR buffer[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, path, -1, buffer, MAX_PATH);
#endif

	hr = IShellFolder_ParseDisplayName(Globals.iDesktop, hwnd, NULL, buffer, &len, &pidl, NULL);
	if (FAILED(hr))
		return NULL;

	return pidl;
}


 /* convert an item id list from relative to absolute (=relative to the desktop) format */

static LPITEMIDLIST get_to_absolute_pidl(Entry* entry, HWND hwnd)
{
	if (entry->up && entry->up->etype==ET_SHELL) {
		IShellFolder* folder = entry->up->folder;
		WCHAR buffer[MAX_PATH];

		HRESULT hr = path_from_pidlW(folder, entry->pidl, buffer, MAX_PATH);

		if (SUCCEEDED(hr)) {
			LPITEMIDLIST pidl;
			ULONG len;

			hr = IShellFolder_ParseDisplayName(Globals.iDesktop, hwnd, NULL, buffer, &len, &pidl, NULL);

			if (SUCCEEDED(hr))
				return pidl;
		}
	} else if (entry->etype == ET_WINDOWS) {
		TCHAR path[MAX_PATH];

		get_path(entry, path);

		return get_path_pidl(path, hwnd);
	} else if (entry->pidl)
		return ILClone(entry->pidl);

	return NULL;
}


static HICON extract_icon(IShellFolder* folder, LPCITEMIDLIST pidl)
{
	IExtractIcon* pExtract;

	if (SUCCEEDED(IShellFolder_GetUIObjectOf(folder, 0, 1, (LPCITEMIDLIST*)&pidl, &IID_IExtractIcon, 0, (LPVOID*)&pExtract))) {
		TCHAR path[_MAX_PATH];
		unsigned flags;
		HICON hicon;
		int idx;

		if (SUCCEEDED((*pExtract->lpVtbl->GetIconLocation)(pExtract, GIL_FORSHELL, path, _MAX_PATH, &idx, &flags))) {
			if (!(flags & GIL_NOTFILENAME)) {
				if (idx == -1)
					idx = 0;	/* special case for some control panel applications */

				if ((int)ExtractIconEx(path, idx, 0, &hicon, 1) > 0)
					flags &= ~GIL_DONTCACHE;
			} else {
				HICON hIconLarge = 0;

				HRESULT hr = (*pExtract->lpVtbl->Extract)(pExtract, path, idx, &hIconLarge, &hicon, MAKELONG(0/*GetSystemMetrics(SM_CXICON)*/,GetSystemMetrics(SM_CXSMICON)));

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

	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

#ifndef _NO_EXTENSIONS
	entry->etype = ET_SHELL;
#endif

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
		if (!SUCCEEDED(hr))
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


static void fill_w32fdata_shell(IShellFolder* folder, LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA* w32fdata)
{
	if (!(attribs & SFGAO_FILESYSTEM) ||
		  FAILED(SHGetDataFromIDList(folder, pidl, SHGDFIL_FINDDATA, w32fdata, sizeof(WIN32_FIND_DATA)))) {
		WIN32_FILE_ATTRIBUTE_DATA fad;
		IDataObject* pDataObj;

		STGMEDIUM medium = {0, {0}, 0};
		FORMATETC fmt = {Globals.cfStrFName, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		HRESULT hr = IShellFolder_GetUIObjectOf(folder, 0, 1, &pidl, &IID_IDataObject, 0, (LPVOID*)&pDataObj);

		if (SUCCEEDED(hr)) {
			hr = IDataObject_GetData(pDataObj, &fmt, &medium);

			IDataObject_Release(pDataObj);

			if (SUCCEEDED(hr)) {
				LPCTSTR path = (LPCTSTR)GlobalLock(medium.UNION_MEMBER(hGlobal));
				UINT sem_org = SetErrorMode(SEM_FAILCRITICALERRORS);

				if (GetFileAttributesEx(path, GetFileExInfoStandard, &fad)) {
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
			if (!SUCCEEDED(hr))
				break;

			if (hr == S_FALSE)
				break;

			for(n=0; n<cnt; ++n) {
				entry = alloc_entry();

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->next = entry;

				memset(&entry->data, 0, sizeof(WIN32_FIND_DATA));
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

#ifndef _NO_EXTENSIONS
				entry->etype = ET_SHELL;
				entry->bhfi_valid = FALSE;
#endif

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

#endif /* _SHELL_FOLDERS */


/* sort order for different directory/file types */
enum TYPE_ORDER {
	TO_DIR = 0,
	TO_DOT = 1,
	TO_DOTDOT = 2,
	TO_OTHER_DIR = 3,
	TO_FILE = 4
};

/* distinguish between ".", ".." and any other directory names */
static int TypeOrderFromDirname(LPCTSTR name)
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
static int compareType(const WIN32_FIND_DATA* fd1, const WIN32_FIND_DATA* fd2)
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
	const WIN32_FIND_DATA* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(const Entry* const*)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return lstrcmpi(fd1->cFileName, fd2->cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(const Entry* const*)arg2)->data;
	const TCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	name1 = fd1->cFileName;
	name2 = fd2->cFileName;

	ext1 = _tcsrchr(name1, TEXT('.'));
	ext2 = _tcsrchr(name2, TEXT('.'));

	if (ext1)
		ext1++;
	else
		ext1 = sEmpty;

	if (ext2)
		ext2++;
	else
		ext2 = sEmpty;

	cmp = lstrcmpi(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpi(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(const Entry* const*)arg2)->data;

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
	const WIN32_FIND_DATA* fd1 = &(*(const Entry* const*)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(const Entry* const*)arg2)->data;

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
	Entry* entry = dir->down;
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


static void read_directory(Entry* dir, LPCTSTR path, SORT_ORDER sortOrder, HWND hwnd)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry;
	LPCTSTR s;
	PTSTR d;

#ifdef _SHELL_FOLDERS
	if (dir->etype == ET_SHELL)
	{
		read_directory_shell(dir, hwnd);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = TEXT('\\');

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					read_directory_shell(entry, hwnd);
					SortDirectory(entry, sortOrder);
				}
		}
	}
	else
#endif
#if !defined(_NO_EXTENSIONS) && defined(__WINE__)
	if (dir->etype == ET_UNIX)
	{
		read_directory_unix(dir, path);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = TEXT('/');

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpy(d, entry->data.cFileName);
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

			*d++ = TEXT('\\');

			for(entry=dir->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpy(d, entry->data.cFileName);
					read_directory_win(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
	}

	SortDirectory(dir, sortOrder);
}


static Entry* read_tree(Root* root, LPCTSTR path, LPITEMIDLIST pidl, LPTSTR drv, SORT_ORDER sortOrder, HWND hwnd)
{
#if !defined(_NO_EXTENSIONS) && defined(__WINE__)
	const static TCHAR sSlash[] = {'/', '\0'};
#endif
	const static TCHAR sBackslash[] = {'\\', '\0'};

#ifdef _SHELL_FOLDERS
	if (pidl)
	{
		 /* read shell namespace tree */
		root->drive_type = DRIVE_UNKNOWN;
		drv[0] = '\\';
		drv[1] = '\0';
		load_string(root->volname, IDS_DESKTOP);
		root->fs_flags = 0;
		load_string(root->fs, IDS_SHELL);

		return read_tree_shell(root, pidl, sortOrder, hwnd);
	}
	else
#endif
#if !defined(_NO_EXTENSIONS) && defined(__WINE__)
	if (*path == '/')
	{
		 /* read unix file system tree */
		root->drive_type = GetDriveType(path);

		lstrcat(drv, sSlash);
		load_string(root->volname, IDS_ROOT_FS);
		root->fs_flags = 0;
		load_string(root->fs, IDS_UNIXFS);

		lstrcpy(root->path, sSlash);

		return read_tree_unix(root, path, sortOrder, hwnd);
	}
#endif

	 /* read WIN32 file system tree */
	root->drive_type = GetDriveType(path);

	lstrcat(drv, sBackslash);
	GetVolumeInformation(drv, root->volname, _MAX_FNAME, 0, 0, &root->fs_flags, root->fs, _MAX_DIR);

	lstrcpy(root->path, drv);

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


static ChildWnd* alloc_child_window(LPCTSTR path, LPITEMIDLIST pidl, HWND hwnd)
{
	TCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
	TCHAR dir_path[MAX_PATH];
	TCHAR b1[BUFFER_LEN];
	const static TCHAR sAsterics[] = {'*', '\0'};

	ChildWnd* child = (ChildWnd*) malloc(sizeof(ChildWnd));
	Root* root = &child->root;
	Entry* entry;

	memset(child, 0, sizeof(ChildWnd));

	child->left.treePane = TRUE;
	child->left.visible_cols = 0;

	child->right.treePane = FALSE;
#ifndef _NO_EXTENSIONS
	child->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_INDEX|COL_LINKS;
#else
	child->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES;
#endif

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
		lstrcpy(child->path, path);

		_tsplitpath(path, drv, dir, name, ext);
	}

	lstrcpy(child->filter_pattern, sAsterics);
	child->filter_flags = TF_ALL;

	root->entry.level = 0;

	lstrcpy(dir_path, drv);
	lstrcat(dir_path, dir);
	entry = read_tree(root, dir_path, pidl, drv, child->sortOrder, hwnd);

#ifdef _SHELL_FOLDERS
	if (root->entry.etype == ET_SHELL)
		load_string(root->entry.data.cFileName, IDS_DESKTOP);
	else
#endif
		wsprintf(root->entry.data.cFileName, RS(b1,IDS_TITLEFMT), drv, root->fs);

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
	free(child);
}


/* get full path of specified directory entry */
static void get_path(Entry* dir, PTSTR path)
{
	Entry* entry;
	int len = 0;
	int level = 0;

#ifdef _SHELL_FOLDERS
	if (dir->etype == ET_SHELL)
	{
		SFGAOF attribs;
		HRESULT hr = S_OK;

		path[0] = TEXT('\0');

		attribs = 0;

		if (dir->folder)
			hr = IShellFolder_GetAttributesOf(dir->folder, 1, (LPCITEMIDLIST*)&dir->pidl, &attribs);

		if (SUCCEEDED(hr) && (attribs&SFGAO_FILESYSTEM)) {
			IShellFolder* parent = dir->up? dir->up->folder: Globals.iDesktop;

			hr = path_from_pidl(parent, dir->pidl, path, MAX_PATH);
		}
	}
	else
#endif
	{
		for(entry=dir; entry; level++) {
			LPCTSTR name;
			int l;

			{
				LPCTSTR s;
				name = entry->data.cFileName;
				s = name;

				for(l=0; *s && *s!=TEXT('/') && *s!=TEXT('\\'); s++)
					l++;
			}

			if (entry->up) {
				if (l > 0) {
					memmove(path+l+1, path, len*sizeof(TCHAR));
					memcpy(path+1, name, l*sizeof(TCHAR));
					len += l+1;

#ifndef _NO_EXTENSIONS
					if (entry->etype == ET_UNIX)
						path[0] = TEXT('/');
					else
#endif
					path[0] = TEXT('\\');
				}

				entry = entry->up;
			} else {
				memmove(path+l, path, len*sizeof(TCHAR));
				memcpy(path, name, l*sizeof(TCHAR));
				len += l;
				break;
			}
		}

		if (!level) {
#ifndef _NO_EXTENSIONS
			if (entry->etype == ET_UNIX)
				path[len++] = TEXT('/');
			else
#endif
				path[len++] = TEXT('\\');
		}

		path[len] = TEXT('\0');
	}
}


static void resize_frame_rect(HWND hwnd, PRECT prect)
{
	int new_top;
	RECT rt;

	if (IsWindowVisible(Globals.htoolbar)) {
		SendMessage(Globals.htoolbar, WM_SIZE, 0, 0);
		GetClientRect(Globals.htoolbar, &rt);
		prect->top = rt.bottom+3;
		prect->bottom -= rt.bottom+3;
	}

	if (IsWindowVisible(Globals.hdrivebar)) {
		SendMessage(Globals.hdrivebar, WM_SIZE, 0, 0);
		GetClientRect(Globals.hdrivebar, &rt);
		new_top = --prect->top + rt.bottom+3;
		MoveWindow(Globals.hdrivebar, 0, prect->top, rt.right, new_top, TRUE);
		prect->top = new_top;
		prect->bottom -= rt.bottom+2;
	}

	if (IsWindowVisible(Globals.hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(Globals.hstatusbar, WM_SIZE, 0, 0);
		SendMessage(Globals.hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
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
		SetWindowLongPtr(child->hwnd, GWLP_USERDATA, (LPARAM)child);
	}

	return CallNextHookEx(hcbthook, code, wparam, lparam);
}

static HWND create_child_window(ChildWnd* child)
{
	MDICREATESTRUCT mcs;
	int idx;

	mcs.szClass = sWINEFILETREE;
	mcs.szTitle = (LPTSTR)child->path;
	mcs.hOwner  = Globals.hInstance;
	mcs.x       = child->pos.rcNormalPosition.left;
	mcs.y       = child->pos.rcNormalPosition.top;
	mcs.cx      = child->pos.rcNormalPosition.right-child->pos.rcNormalPosition.left;
	mcs.cy      = child->pos.rcNormalPosition.bottom-child->pos.rcNormalPosition.top;
	mcs.style   = 0;
	mcs.lParam  = 0;

	hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());

	newchild = child;
	child->hwnd = (HWND) SendMessage(Globals.hmdiclient, WM_MDICREATE, 0, (LPARAM)&mcs);
	if (!child->hwnd) {
		UnhookWindowsHookEx(hcbthook);
		return 0;
	}

	UnhookWindowsHookEx(hcbthook);

	(void)ListBox_SetItemHeight(child->left.hwnd, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
	(void)ListBox_SetItemHeight(child->right.hwnd, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));

	idx = ListBox_FindItemData(child->left.hwnd, 0, child->left.cur);
	(void)ListBox_SetCurSel(child->left.hwnd, idx);

	return child->hwnd;
}


struct ExecuteDialog {
	TCHAR	cmd[MAX_PATH];
	int		cmdshow;
};

static INT_PTR CALLBACK ExecuteDialogDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct ExecuteDialog* dlg;

	switch(nmsg) {
		case WM_INITDIALOG:
			dlg = (struct ExecuteDialog*) lparam;
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			if (id == IDOK) {
				GetWindowText(GetDlgItem(hwnd, 201), dlg->cmd, MAX_PATH);
				dlg->cmdshow = Button_GetState(GetDlgItem(hwnd,214))&BST_CHECKED?
												SW_SHOWMINIMIZED: SW_SHOWNORMAL;
				EndDialog(hwnd, id);
			} else if (id == IDCANCEL)
				EndDialog(hwnd, id);

			return 1;}
	}

	return 0;
}


static INT_PTR CALLBACK DestinationDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	TCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	switch(nmsg) {
		case WM_INITDIALOG:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
			SetWindowText(GetDlgItem(hwnd, 201), (LPCTSTR)lparam);
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			switch(id) {
			  case IDOK: {
				LPTSTR dest = (LPTSTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
				GetWindowText(GetDlgItem(hwnd, 201), dest, MAX_PATH);
				EndDialog(hwnd, id);
				break;}

			  case IDCANCEL:
				EndDialog(hwnd, id);
				break;

			  case 254:
				MessageBox(hwnd, RS(b1,IDS_NO_IMPL), RS(b2,IDS_WINEFILE), MB_OK);
				break;
			}

			return 1;
		}
	}

	return 0;
}


struct FilterDialog {
	TCHAR	pattern[MAX_PATH];
	int		flags;
};

static INT_PTR CALLBACK FilterDialogDlgProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct FilterDialog* dlg;

	switch(nmsg) {
		case WM_INITDIALOG:
			dlg = (struct FilterDialog*) lparam;
			SetWindowText(GetDlgItem(hwnd, IDC_VIEW_PATTERN), dlg->pattern);
			Button_SetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_DIRECTORIES), (dlg->flags&TF_DIRECTORIES? BST_CHECKED: BST_UNCHECKED));
			Button_SetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_PROGRAMS), dlg->flags&TF_PROGRAMS? BST_CHECKED: BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_DOCUMENTS), dlg->flags&TF_DOCUMENTS? BST_CHECKED: BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_OTHERS), dlg->flags&TF_OTHERS? BST_CHECKED: BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_HIDDEN), dlg->flags&TF_HIDDEN? BST_CHECKED: BST_UNCHECKED);
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			if (id == IDOK) {
				int flags = 0;

				GetWindowText(GetDlgItem(hwnd, IDC_VIEW_PATTERN), dlg->pattern, MAX_PATH);

				flags |= Button_GetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_DIRECTORIES))&BST_CHECKED? TF_DIRECTORIES: 0;
				flags |= Button_GetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_PROGRAMS))&BST_CHECKED? TF_PROGRAMS: 0;
				flags |= Button_GetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_DOCUMENTS))&BST_CHECKED? TF_DOCUMENTS: 0;
				flags |= Button_GetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_OTHERS))&BST_CHECKED? TF_OTHERS: 0;
				flags |= Button_GetCheck(GetDlgItem(hwnd,IDC_VIEW_TYPE_HIDDEN))&BST_CHECKED? TF_HIDDEN: 0;

				dlg->flags = flags;

				EndDialog(hwnd, id);
			} else if (id == IDCANCEL)
				EndDialog(hwnd, id);

			return 1;}
	}

	return 0;
}


struct PropertiesDialog {
	TCHAR	path[MAX_PATH];
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
	int idx = ListBox_GetCurSel(hlbox);

	if (idx != LB_ERR) {
		LPCTSTR pValue = (LPCTSTR) ListBox_GetItemData(hlbox, idx);

		if (pValue)
			SetWindowText(hedit, pValue);
	}
}

static void CheckForFileInfo(struct PropertiesDialog* dlg, HWND hwnd, LPCTSTR strFilename)
{
	static TCHAR sBackSlash[] = {'\\','\0'};
	static TCHAR sTranslation[] = {'\\','V','a','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n','\0'};
	static TCHAR sStringFileInfo[] = {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o','\\',
										'%','0','4','x','%','0','4','x','\\','%','s','\0'};
	DWORD dwVersionDataLen = GetFileVersionInfoSize((LPTSTR)strFilename, NULL);	/* VC6 and MinGW headers use LPTSTR instead of LPCTSTR */

	if (dwVersionDataLen) {
		dlg->pVersionData = malloc(dwVersionDataLen);

		if (GetFileVersionInfo((LPTSTR)strFilename, 0, dwVersionDataLen, dlg->pVersionData)) {	/* VC6 and MinGW headers use LPTSTR instead of LPCTSTR */
			LPVOID pVal;
			UINT nValLen;

			if (VerQueryValue(dlg->pVersionData, sBackSlash, &pVal, &nValLen)) {
				if (nValLen == sizeof(VS_FIXEDFILEINFO)) {
					VS_FIXEDFILEINFO* pFixedFileInfo = (VS_FIXEDFILEINFO*)pVal;
					char buffer[BUFFER_LEN];

					sprintf(buffer, "%d.%d.%d.%d",
						HIWORD(pFixedFileInfo->dwFileVersionMS), LOWORD(pFixedFileInfo->dwFileVersionMS),
						HIWORD(pFixedFileInfo->dwFileVersionLS), LOWORD(pFixedFileInfo->dwFileVersionLS));

					SetDlgItemTextA(hwnd, IDC_STATIC_PROP_VERSION, buffer);
				}
			}

			/* Read the list of languages and code pages. */
			if (VerQueryValue(dlg->pVersionData, sTranslation, &pVal, &nValLen)) {
				struct LANGANDCODEPAGE* pTranslate = (struct LANGANDCODEPAGE*)pVal;
				struct LANGANDCODEPAGE* pEnd = (struct LANGANDCODEPAGE*)((LPBYTE)pVal+nValLen);

				HWND hlbox = GetDlgItem(hwnd, IDC_LIST_PROP_VERSION_TYPES);

				/* Read the file description for each language and code page. */
				for(; pTranslate<pEnd; ++pTranslate) {
					LPCSTR* p;

					for(p=InfoStrings; *p; ++p) {
						TCHAR subblock[200];
#ifdef UNICODE
						TCHAR infoStr[100];
#endif
						LPCTSTR pTxt;
						UINT nValLen;

						LPCSTR pInfoString = *p;
#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, 0, pInfoString, -1, infoStr, 100);
#else
#define	infoStr pInfoString
#endif
						wsprintf(subblock, sStringFileInfo, pTranslate->wLanguage, pTranslate->wCodePage, infoStr);

						/* Retrieve file description for language and code page */
						if (VerQueryValue(dlg->pVersionData, subblock, (PVOID)&pTxt, &nValLen)) {
							int idx = ListBox_AddString(hlbox, infoStr);
							(void)ListBox_SetItemData(hlbox, idx, pTxt);
						}
					}
				}

				(void)ListBox_SetCurSel(hlbox, 0);

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
			const static TCHAR sByteFmt[] = {'%','s',' ','B','y','t','e','s','\0'};
			TCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];
			LPWIN32_FIND_DATA pWFD;
			ULONGLONG size;

			dlg = (struct PropertiesDialog*) lparam;
			pWFD = (LPWIN32_FIND_DATA) &dlg->entry.data;

			GetWindowText(hwnd, b1, MAX_PATH);
			wsprintf(b2, b1, pWFD->cFileName);
			SetWindowText(hwnd, b2);

			format_date(&pWFD->ftLastWriteTime, b1, COL_DATE|COL_TIME);
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC_PROP_LASTCHANGE), b1);

			size = ((ULONGLONG)pWFD->nFileSizeHigh << 32) | pWFD->nFileSizeLow;
			_stprintf(b1, sLongNumFmt, size);
			wsprintf(b2, sByteFmt, b1);
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC_PROP_SIZE), b2);

			SetWindowText(GetDlgItem(hwnd, IDC_STATIC_PROP_FILENAME), pWFD->cFileName);
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC_PROP_PATH), dlg->path);

			Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK_READONLY), (pWFD->dwFileAttributes&FILE_ATTRIBUTE_READONLY? BST_CHECKED: BST_UNCHECKED));
			Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK_ARCHIVE), (pWFD->dwFileAttributes&FILE_ATTRIBUTE_ARCHIVE? BST_CHECKED: BST_UNCHECKED));
			Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK_COMPRESSED), (pWFD->dwFileAttributes&FILE_ATTRIBUTE_COMPRESSED? BST_CHECKED: BST_UNCHECKED));
			Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK_HIDDEN), (pWFD->dwFileAttributes&FILE_ATTRIBUTE_HIDDEN? BST_CHECKED: BST_UNCHECKED));
			Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK_SYSTEM), (pWFD->dwFileAttributes&FILE_ATTRIBUTE_SYSTEM? BST_CHECKED: BST_UNCHECKED));

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
			free(dlg->pVersionData);
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

	DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_PROPERTIES), hwnd, PropertiesDialogDlgProc, (LPARAM)&dlg);
}


#ifndef _NO_EXTENSIONS

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

		(void)Frame_CalcFrameClient(hwnd, &rt);
		ClientToScreen(hwnd, (LPPOINT)&rt.left);
		ClientToScreen(hwnd, (LPPOINT)&rt.right);

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

	(void)Frame_CalcFrameClient(hwnd, &rt);
	ClientToScreen(hwnd, (LPPOINT)&rt.left);
	ClientToScreen(hwnd, (LPPOINT)&rt.right);

	rt.left = pos.left-rt.left;
	rt.top = pos.top-rt.top;
	rt.right = GetSystemMetrics(SM_CXSCREEN)+pos.right-rt.right;
	rt.bottom = GetSystemMetrics(SM_CYSCREEN)+pos.bottom-rt.bottom;

	MoveWindow(hwnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
}

#endif


static void toggle_child(HWND hwnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(Globals.hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);

	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);

#ifndef _NO_EXTENSIONS
	if (g_fullscreen.mode)
		fullscreen_move(hwnd);
#endif

	resize_frame_client(hwnd);
}

static BOOL activate_drive_window(LPCTSTR path)
{
	TCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_tsplitpath(path, drv1, 0, 0, 0);

	/* search for a already open window for the same drive */
	for(child_wnd=GetNextWindow(Globals.hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* child = (ChildWnd*) GetWindowLongPtr(child_wnd, GWLP_USERDATA);

		if (child) {
			_tsplitpath(child->root.path, drv2, 0, 0, 0);

			if (!lstrcmpi(drv2, drv1)) {
				SendMessage(Globals.hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return TRUE;
			}
		}
	}

	return FALSE;
}

static BOOL activate_fs_window(LPCTSTR filesys)
{
	HWND child_wnd;

	/* search for a already open window of the given file system name */
	for(child_wnd=GetNextWindow(Globals.hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* child = (ChildWnd*) GetWindowLongPtr(child_wnd, GWLP_USERDATA);

		if (child) {
			if (!lstrcmpi(child->root.fs, filesys)) {
				SendMessage(Globals.hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return TRUE;
			}
		}
	}

	return FALSE;
}

static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	TCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	switch(nmsg) {
		case WM_CLOSE:
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
			HWND hwndClient = (HWND) SendMessage(Globals.hmdiclient, WM_MDIGETACTIVE, 0, 0);

			if (!SendMessage(hwndClient, WM_INITMENUPOPUP, wparam, lparam))
				return 0;
			break;}

		case WM_COMMAND: {
			UINT cmd = LOWORD(wparam);
			HWND hwndClient = (HWND) SendMessage(Globals.hmdiclient, WM_MDIGETACTIVE, 0, 0);

			if (SendMessage(hwndClient, WM_DISPATCH_COMMAND, wparam, lparam))
				break;

			if (cmd>=ID_DRIVE_FIRST && cmd<=ID_DRIVE_FIRST+0xFF) {
				TCHAR drv[_MAX_DRIVE], path[MAX_PATH];
				ChildWnd* child;
				LPCTSTR root = Globals.drives;
				int i;

				for(i=cmd-ID_DRIVE_FIRST; i--; root++)
					while(*root)
						root++;

				if (activate_drive_window(root))
					return 0;

				_tsplitpath(root, drv, 0, 0, 0);

				if (!SetCurrentDirectory(drv)) {
					display_error(hwnd, GetLastError());
					return 0;
				}

				GetCurrentDirectory(MAX_PATH, path); /*TODO: store last directory per drive */
				child = alloc_child_window(path, NULL, hwnd);

				if (!create_child_window(child))
					free(child);
			} else switch(cmd) {
				case ID_FILE_EXIT:
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					break;

				case ID_WINDOW_NEW: {
					TCHAR path[MAX_PATH];
					ChildWnd* child;

					GetCurrentDirectory(MAX_PATH, path);
					child = alloc_child_window(path, NULL, hwnd);

					if (!create_child_window(child))
						free(child);
					break;}

				case ID_REFRESH:
					refresh_drives();
					break;

				case ID_WINDOW_CASCADE:
					SendMessage(Globals.hmdiclient, WM_MDICASCADE, 0, 0);
					break;

				case ID_WINDOW_TILE_HORZ:
					SendMessage(Globals.hmdiclient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
					break;

				case ID_WINDOW_TILE_VERT:
					SendMessage(Globals.hmdiclient, WM_MDITILE, MDITILE_VERTICAL, 0);
					break;

				case ID_WINDOW_ARRANGE:
					SendMessage(Globals.hmdiclient, WM_MDIICONARRANGE, 0, 0);
					break;

				case ID_SELECT_FONT: {
					TCHAR dlg_name[BUFFER_LEN], dlg_info[BUFFER_LEN];
					CHOOSEFONT chFont;
					LOGFONT lFont;

					HDC hdc = GetDC(hwnd);
					chFont.lStructSize = sizeof(CHOOSEFONT);
					chFont.hwndOwner = hwnd;
					chFont.hDC = NULL;
					chFont.lpLogFont = &lFont;
					chFont.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_LIMITSIZE | CF_NOSCRIPTSEL;
					chFont.rgbColors = RGB(0,0,0);
					chFont.lCustData = 0;
					chFont.lpfnHook = NULL;
					chFont.lpTemplateName = NULL;
					chFont.hInstance = Globals.hInstance;
					chFont.lpszStyle = NULL;
					chFont.nFontType = SIMULATED_FONTTYPE;
					chFont.nSizeMin = 0;
					chFont.nSizeMax = 24;

					if (ChooseFont(&chFont)) {
						HWND childWnd;
						HFONT hFontOld;

						DeleteObject(Globals.hfont);
						Globals.hfont = CreateFontIndirect(&lFont);
						hFontOld = SelectFont(hdc, Globals.hfont);
						GetTextExtentPoint32(hdc, sSpace, 1, &Globals.spaceSize);

						/* change font in all open child windows */
						for(childWnd=GetWindow(Globals.hmdiclient,GW_CHILD); childWnd; childWnd=GetNextWindow(childWnd,GW_HWNDNEXT)) {
							ChildWnd* child = (ChildWnd*) GetWindowLongPtr(childWnd, GWLP_USERDATA);
							SetWindowFont(child->left.hwnd, Globals.hfont, TRUE);
							SetWindowFont(child->right.hwnd, Globals.hfont, TRUE);
							(void)ListBox_SetItemHeight(child->left.hwnd, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
							(void)ListBox_SetItemHeight(child->right.hwnd, 1, max(Globals.spaceSize.cy,IMAGE_HEIGHT+3));
							InvalidateRect(child->left.hwnd, NULL, TRUE);
							InvalidateRect(child->right.hwnd, NULL, TRUE);
						}

						(void)SelectFont(hdc, hFontOld);
					}
					else if (CommDlgExtendedError()) {
						LoadString(Globals.hInstance, IDS_FONT_SEL_DLG_NAME, dlg_name, BUFFER_LEN);
						LoadString(Globals.hInstance, IDS_FONT_SEL_ERROR, dlg_info, BUFFER_LEN);
						MessageBox(hwnd, dlg_info, dlg_name, MB_OK);
					}

					ReleaseDC(hwnd, hdc);
					break;
				}

				case ID_VIEW_TOOL_BAR:
					toggle_child(hwnd, cmd, Globals.htoolbar);
					break;

				case ID_VIEW_DRIVE_BAR:
					toggle_child(hwnd, cmd, Globals.hdrivebar);
					break;

				case ID_VIEW_STATUSBAR:
					toggle_child(hwnd, cmd, Globals.hstatusbar);
					break;

				case ID_EXECUTE: {
					struct ExecuteDialog dlg;

					memset(&dlg, 0, sizeof(struct ExecuteDialog));

					if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_EXECUTE), hwnd, ExecuteDialogDlgProc, (LPARAM)&dlg) == IDOK) {
						HINSTANCE hinst = ShellExecute(hwnd, NULL/*operation*/, dlg.cmd/*file*/, NULL/*parameters*/, NULL/*dir*/, dlg.cmdshow);

						if ((int)hinst <= 32)
							display_error(hwnd, GetLastError());
					}
					break;}

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

#ifndef __MINGW32__	/* SHFormatDrive missing in MinGW (as of 13.5.2005) */
				case ID_FORMAT_DISK: {
					UINT sem_org = SetErrorMode(0); /* Get the current Error Mode settings. */
					SetErrorMode(sem_org & ~SEM_FAILCRITICALERRORS); /* Force O/S to handle */
					SHFormatDrive(hwnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
					SetErrorMode(sem_org); /* Put it back the way it was. */
					break;}
#endif

				case ID_HELP:
					WinHelp(hwnd, RS(b1,IDS_WINEFILE), HELP_INDEX, 0);
					break;

#ifndef _NO_EXTENSIONS
				case ID_VIEW_FULLSCREEN:
					CheckMenuItem(Globals.hMenuOptions, cmd, toggle_fullscreen(hwnd)?MF_CHECKED:0);
					break;

#ifdef __WINE__
				case ID_DRIVE_UNIX_FS: {
					TCHAR path[MAX_PATH];
#ifdef UNICODE
					char cpath[MAX_PATH];
#endif
					ChildWnd* child;

					if (activate_fs_window(RS(b1,IDS_UNIXFS)))
						break;


#ifdef UNICODE
					getcwd(cpath, MAX_PATH);
					MultiByteToWideChar(CP_UNIXCP, 0, cpath, -1, path, MAX_PATH);
#else
					getcwd(path, MAX_PATH);
#endif
					child = alloc_child_window(path, NULL, hwnd);

					if (!create_child_window(child))
						free(child);
					break;}
#endif

#ifdef _SHELL_FOLDERS
				case ID_DRIVE_SHELL_NS: {
					TCHAR path[MAX_PATH];
					ChildWnd* child;

					if (activate_fs_window(RS(b1,IDS_SHELL)))
						break;

					GetCurrentDirectory(MAX_PATH, path);
					child = alloc_child_window(path, get_path_pidl(path,hwnd), hwnd);

					if (!create_child_window(child))
						free(child);
					break;}
#endif
#endif

				/*TODO: There are even more menu items! */

#ifndef _NO_EXTENSIONS
#ifdef __WINE__
				case ID_LICENSE:
					WineLicense(Globals.hMainWnd);
					break;

				case ID_NO_WARRANTY:
					WineWarranty(Globals.hMainWnd);
					break;

				case ID_ABOUT_WINE:
					ShellAbout(hwnd, RS(b2,IDS_WINE), RS(b1,IDS_WINEFILE), 0);
					break;
#endif

				case ID_ABOUT:
					ShellAbout(hwnd, RS(b1,IDS_WINEFILE), NULL, 0);
					break;
#endif	/* _NO_EXTENSIONS */

				default:
					/*TODO: if (wParam >= PM_FIRST_LANGUAGE && wParam <= PM_LAST_LANGUAGE)
						STRING_SelectLanguageByNumber(wParam - PM_FIRST_LANGUAGE);
					else */if ((cmd<IDW_FIRST_CHILD || cmd>=IDW_FIRST_CHILD+0x100) &&
						(cmd<SC_SIZE || cmd>SC_RESTORE))
						MessageBox(hwnd, RS(b2,IDS_NO_IMPL), RS(b1,IDS_WINEFILE), MB_OK);

					return DefFrameProc(hwnd, Globals.hmdiclient, nmsg, wparam, lparam);
			}
			break;}

		case WM_SIZE:
			resize_frame(hwnd, LOWORD(lparam), HIWORD(lparam));
			break;	/* do not pass message to DefFrameProc */

#ifndef _NO_EXTENSIONS
		case WM_GETMINMAXINFO: {
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

			lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
			lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
			break;}

		case FRM_CALC_CLIENT:
			frame_get_clientspace(hwnd, (PRECT)lparam);
			return TRUE;
#endif /* _NO_EXTENSIONS */

		default:
			return DefFrameProc(hwnd, Globals.hmdiclient, nmsg, wparam, lparam);
	}

	return 0;
}


static TCHAR g_pos_names[COLUMNS][20] = {
	{'\0'}	/* symbol */
};

static const int g_pos_align[] = {
	0,
	HDF_LEFT,	/* Name */
	HDF_RIGHT,	/* Size */
	HDF_LEFT,	/* CDate */
#ifndef _NO_EXTENSIONS
	HDF_LEFT,	/* ADate */
	HDF_LEFT,	/* MDate */
	HDF_LEFT,	/* Index */
	HDF_CENTER,	/* Links */
#endif
	HDF_CENTER,	/* Attributes */
#ifndef _NO_EXTENSIONS
	HDF_LEFT	/* Security */
#endif
};

static void resize_tree(ChildWnd* child, int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt;

	rt.left   = 0;
	rt.top    = 0;
	rt.right  = cx;
	rt.bottom = cy;

	cx = child->split_pos + SPLIT_WIDTH/2;

#ifndef _NO_EXTENSIONS
	{
		WINDOWPOS wp;
		HD_LAYOUT hdl;

		hdl.prc   = &rt;
		hdl.pwpos = &wp;

		(void)Header_Layout(child->left.hwndHeader, &hdl);

		DeferWindowPos(hdwp, child->left.hwndHeader, wp.hwndInsertAfter,
						wp.x-1, wp.y, child->split_pos-SPLIT_WIDTH/2+1, wp.cy, wp.flags);
		DeferWindowPos(hdwp, child->right.hwndHeader, wp.hwndInsertAfter,
						rt.left+cx+1, wp.y, wp.cx-cx+2, wp.cy, wp.flags);
	}
#endif /* _NO_EXTENSIONS */

	DeferWindowPos(hdwp, child->left.hwnd, 0, rt.left, rt.top, child->split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	DeferWindowPos(hdwp, child->right.hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}


#ifndef _NO_EXTENSIONS

static HWND create_header(HWND parent, Pane* pane, int id)
{
	HD_ITEM hdi;
	int idx;

	HWND hwnd = CreateWindow(WC_HEADER, 0, WS_CHILD|WS_VISIBLE|HDS_HORZ/*TODO: |HDS_BUTTONS + sort orders*/,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);
	if (!hwnd)
		return 0;

	SetWindowFont(hwnd, GetStockObject(DEFAULT_GUI_FONT), FALSE);

	hdi.mask = HDI_TEXT|HDI_WIDTH|HDI_FORMAT;

	for(idx=0; idx<COLUMNS; idx++) {
		hdi.pszText = g_pos_names[idx];
		hdi.fmt = HDF_STRING | g_pos_align[idx];
		hdi.cxy = pane->widths[idx];
		(void)Header_InsertItem(hwnd, idx, &hdi);
	}

	return hwnd;
}

#endif /* _NO_EXTENSIONS */


static void init_output(HWND hwnd)
{
	const static TCHAR s1000[] = {'1','0','0','0','\0'};

	TCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hwnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, s1000, 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = TEXT('.');

	old_font = SelectFont(hdc, Globals.hfont);
	GetTextExtentPoint32(hdc, sSpace, 1, &Globals.spaceSize);
	(void)SelectFont(hdc, old_font);
	ReleaseDC(hwnd, hdc);
}

static void draw_item(Pane* pane, LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol);


/* calculate preferred width for all visible columns */

static BOOL calc_widths(Pane* pane, BOOL anyway)
{
	int col, x, cx, spc=3*Globals.spaceSize.cx;
	int entries = ListBox_GetCount(pane->hwnd);
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
	hfontOld = SelectFont(hdc, Globals.hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, cnt);

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

	ListBox_SetHorizontalExtent(pane->hwnd, x);

	/* no change? */
	if (!memcmp(orgWidths, pane->widths, sizeof(orgWidths)))
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
	int entries = ListBox_GetCount(pane->hwnd);
	int cnt;
	HDC hdc;

	pane->widths[col] = 0;

	hdc = GetDC(pane->hwnd);
	hfontOld = SelectFont(hdc, Globals.hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, cnt);
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

	for(; col<COLUMNS; ) {
		pane->positions[++col] = x;
		x += pane->widths[col];
	}

	ListBox_SetHorizontalExtent(pane->hwnd, x);
}


static BOOL pattern_match(LPCTSTR str, LPCTSTR pattern)
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

static BOOL pattern_imatch(LPCTSTR str, LPCTSTR pattern)
{
	TCHAR b1[BUFFER_LEN], b2[BUFFER_LEN];

	lstrcpy(b1, str);
	lstrcpy(b2, pattern);
	CharUpper(b1);
	CharUpper(b2);

	return pattern_match(b1, b2);
}


enum FILE_TYPE {
	FT_OTHER		= 0,
	FT_EXECUTABLE	= 1,
	FT_DOCUMENT		= 2
};

static enum FILE_TYPE get_file_type(LPCTSTR filename);


/* insert listbox entries after index idx */

static int insert_entries(Pane* pane, Entry* dir, LPCTSTR pattern, int filter_flags, int idx)
{
	Entry* entry = dir;

	if (!entry)
		return idx;

	ShowWindow(pane->hwnd, SW_HIDE);

	for(; entry; entry=entry->next) {
#ifndef _LEFT_FILES
		if (pane->treePane && !(entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			continue;
#endif

		if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			/* don't display entries "." and ".." in the left pane */
			if (pane->treePane && entry->data.cFileName[0]==TEXT('.'))
				if (
	#ifndef _NO_EXTENSIONS
					entry->data.cFileName[1]==TEXT('\0') ||
	#endif
					(entry->data.cFileName[1]==TEXT('.') && entry->data.cFileName[2]==TEXT('\0')))
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

		(void)ListBox_InsertItemData(pane->hwnd, idx, entry);

		if (pane->treePane && entry->expanded)
			idx = insert_entries(pane, entry->down, pattern, filter_flags, idx);
	}

	ShowWindow(pane->hwnd, SW_SHOW);

	return idx;
}


static void format_bytes(LPTSTR buffer, LONGLONG bytes)
{
	const static TCHAR sFmtGB[] = {'%', '.', '1', 'f', ' ', 'G', 'B', '\0'};
	const static TCHAR sFmtMB[] = {'%', '.', '1', 'f', ' ', 'M', 'B', '\0'};
	const static TCHAR sFmtkB[] = {'%', '.', '1', 'f', ' ', 'k', 'B', '\0'};

	float fBytes = (float)bytes;

#ifdef __WINE__	/* work around for incorrect implementation of wsprintf()/_stprintf() in WINE */
	if (bytes >= 1073741824)	/* 1 GB */
		wsprintf(buffer, sFmtGB, fBytes/1073741824.f+.5f);
	else if (bytes >= 1048576)	/* 1 MB */
		wsprintf(buffer, sFmtMB, fBytes/1048576.f+.5f);
	else if (bytes >= 1024)		/* 1 kB */
		wsprintf(buffer, sFmtkB, fBytes/1024.f+.5f);
#else
	if (bytes >= 1073741824)	/* 1 GB */
		_stprintf(buffer, sFmtGB, fBytes/1073741824.f+.5f);
	else if (bytes >= 1048576)	/* 1 MB */
		_stprintf(buffer, sFmtMB, fBytes/1048576.f+.5f);
	else if (bytes >= 1024)		/* 1 kB */
		_stprintf(buffer, sFmtkB, fBytes/1024.f+.5f);
#endif
	else
		_stprintf(buffer, sLongNumFmt, bytes);
}

static void set_space_status(void)
{
	ULARGE_INTEGER ulFreeBytesToCaller, ulTotalBytes, ulFreeBytes;
	TCHAR fmt[64], b1[64], b2[64], buffer[BUFFER_LEN];

	if (GetDiskFreeSpaceEx(NULL, &ulFreeBytesToCaller, &ulTotalBytes, &ulFreeBytes)) {
		format_bytes(b1, ulFreeBytesToCaller.QuadPart);
		format_bytes(b2, ulTotalBytes.QuadPart);
		wsprintf(buffer, RS(fmt,IDS_FREE_SPACE_FMT), b1, b2);
	} else
		lstrcpy(buffer, sQMarks);

	SendMessage(Globals.hstatusbar, SB_SETTEXT, 0, (LPARAM)buffer);
}


static WNDPROC g_orgTreeWndProc;

static void create_tree_window(HWND parent, Pane* pane, int id, int id_header, LPCTSTR pattern, int filter_flags)
{
	const static TCHAR sListBox[] = {'L','i','s','t','B','o','x','\0'};

	static int s_init = 0;
	Entry* entry = pane->root;

	pane->hwnd = CreateWindow(sListBox, sEmpty, WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
								LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);

	SetWindowLongPtr(pane->hwnd, GWLP_USERDATA, (LPARAM)pane);
	g_orgTreeWndProc = SubclassWindow(pane->hwnd, TreeWndProc);

	SetWindowFont(pane->hwnd, Globals.hfont, FALSE);

	/* insert entries into listbox */
	if (entry)
		insert_entries(pane, entry, pattern, filter_flags, -1);

	/* calculate column widths */
	if (!s_init) {
		s_init = 1;
		init_output(pane->hwnd);
	}

	calc_widths(pane, TRUE);

#ifndef _NO_EXTENSIONS
	pane->hwndHeader = create_header(parent, pane, id_header);
#endif
}


static void InitChildWindow(ChildWnd* child)
{
	create_tree_window(child->hwnd, &child->left, IDW_TREE_LEFT, IDW_HEADER_LEFT, NULL, TF_ALL);
	create_tree_window(child->hwnd, &child->right, IDW_TREE_RIGHT, IDW_HEADER_RIGHT, child->filter_pattern, child->filter_flags);
}


static void format_date(const FILETIME* ft, TCHAR* buffer, int visible_cols)
{
	SYSTEMTIME systime;
	FILETIME lft;
	int len = 0;

	*buffer = TEXT('\0');

	if (!ft->dwLowDateTime && !ft->dwHighDateTime)
		return;

	if (!FileTimeToLocalFileTime(ft, &lft))
		{err: lstrcpy(buffer,sQMarks); return;}

	if (!FileTimeToSystemTime(&lft, &systime))
		goto err;

	if (visible_cols & COL_DATE) {
		len = GetDateFormat(LOCALE_USER_DEFAULT, 0, &systime, 0, buffer, BUFFER_LEN);
		if (!len)
			goto err;
	}

	if (visible_cols & COL_TIME) {
		if (len)
			buffer[len-1] = ' ';

		buffer[len++] = ' ';

		if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systime, 0, buffer+len, BUFFER_LEN-len))
			buffer[len] = TEXT('\0');
	}
}


static void calc_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0, 0, 0, 0};

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}

static void calc_tabbed_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0, 0, 0, 0};

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
	/*FIXME rt (0,0) ??? */

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}


static void output_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str, DWORD flags)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|flags);
}

static void output_tabbed_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
}

static void output_number(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;
	LPCTSTR s = str;
	TCHAR b[128];
	LPTSTR d = b;
	int pos;

	rt.left   = x+pane->positions[col]+Globals.spaceSize.cx;
	rt.top    = dis->rcItem.top;
	rt.right  = x+pane->positions[col+1]-Globals.spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	if (*s)
		*d++ = *s++;

	/* insert number separator characters */
	pos = lstrlen(s) % 3;

	while(*s)
		if (pos--)
			*d++ = *s++;
		else {
			*d++ = Globals.num_sep;
			pos = 3;
		}

	DrawText(dis->hDC, b, d-b, &rt, DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
}


static BOOL is_exe_file(LPCTSTR ext)
{
	static const TCHAR executable_extensions[][4] = {
		{'C','O','M','\0'},
		{'E','X','E','\0'},
		{'B','A','T','\0'},
		{'C','M','D','\0'},
#ifndef _NO_EXTENSIONS
		{'C','M','M','\0'},
		{'B','T','M','\0'},
		{'A','W','K','\0'},
#endif /* _NO_EXTENSIONS */
		{'\0'}
	};

	TCHAR ext_buffer[_MAX_EXT];
	const TCHAR (*p)[4];
	LPCTSTR s;
	LPTSTR d;

	for(s=ext+1,d=ext_buffer; (*d=tolower(*s)); s++)
		d++;

	for(p=executable_extensions; (*p)[0]; p++)
		if (!lstrcmpi(ext_buffer, *p))
			return TRUE;

	return FALSE;
}

static BOOL is_registered_type(LPCTSTR ext)
{
	/* check if there exists a classname for this file extension in the registry */
	if (!RegQueryValue(HKEY_CLASSES_ROOT, ext, NULL, NULL))
		return TRUE;

	return FALSE;
}

static enum FILE_TYPE get_file_type(LPCTSTR filename)
{
	LPCTSTR ext = _tcsrchr(filename, '.');
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
	TCHAR buffer[BUFFER_LEN];
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
			if (entry->data.cFileName[0]==TEXT('.') && entry->data.cFileName[1]==TEXT('.')
					&& entry->data.cFileName[2]==TEXT('\0'))
				img = IMG_FOLDER_UP;
#ifndef _NO_EXTENSIONS
			else if (entry->data.cFileName[0]==TEXT('.') && entry->data.cFileName[1]==TEXT('\0'))
				img = IMG_FOLDER_CUR;
#endif
			else if (
#ifdef _NO_EXTENSIONS
					 entry->expanded ||
#endif
					 (pane->treePane && (dis->itemState&ODS_FOCUS)))
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

				/* HGDIOBJ holdPen = SelectObject(dis->hDC, GetStockObject(BLACK_PEN)); */
				ExtSelectClipRgn(dis->hDC, hrgn, RGN_AND);
				DeleteObject(hrgn);

				if ((up=entry->up) != NULL) {
					MoveToEx(dis->hDC, img_pos-IMAGE_WIDTH/2, y, 0);
					LineTo(dis->hDC, img_pos-2, y);

					x = img_pos - IMAGE_WIDTH/2;

					do {
						x -= IMAGE_WIDTH+TREE_LINE_DX;

						if (up->next
#ifndef _LEFT_FILES
							&& (up->next->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#endif
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
#ifndef _LEFT_FILES
					&& (entry->next->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
#endif
					)
					LineTo(dis->hDC, x, dis->rcItem.bottom);

				SelectClipRgn(dis->hDC, hrgn_org);
				if (hrgn_org) DeleteObject(hrgn_org);
				/* SelectObject(dis->hDC, holdPen); */
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
		focusRect.left = img_pos - 2;

#ifdef _NO_EXTENSIONS
		if (pane->treePane && entry) {
			RECT rt = {0};

			DrawText(dis->hDC, entry->data.cFileName, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

			focusRect.right = dis->rcItem.left+pane->positions[col+1]+TREE_LINE_DX + rt.right +2;
		}
#else

		if (attrs & FILE_ATTRIBUTE_COMPRESSED)
			textcolor = COLOR_COMPRESSED;
		else
#endif /* _NO_EXTENSIONS */
			textcolor = RGB(0,0,0);

		if (dis->itemState & ODS_FOCUS) {
			textcolor = COLOR_SELECTION_TXT;
			bkcolor = COLOR_SELECTION;
		} else {
			bkcolor = GetSysColor(COLOR_WINDOW);
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

#ifdef _SHELL_FOLDERS
			if (entry->hicon && entry->hicon!=(HICON)-1)
				DrawIconEx(dis->hDC, img_pos, dis->rcItem.top, entry->hicon, cx, GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
			else
#endif
				ImageList_DrawEx(Globals.himl, img, dis->hDC,
								 img_pos, dis->rcItem.top, cx,
								 IMAGE_HEIGHT, bkcolor, CLR_DEFAULT, ILD_NORMAL);
		}
	}

	if (!entry)
		return;

#ifdef _NO_EXTENSIONS
	if (img >= IMG_FOLDER_UP)
		return;
#endif

	col++;

	/* ouput file name */
	if (calcWidthCol == -1)
		output_text(pane, dis, col, entry->data.cFileName, 0);
	else if (calcWidthCol==col || calcWidthCol==COLUMNS)
		calc_width(pane, dis, col, entry->data.cFileName);

	col++;

#ifdef _NO_EXTENSIONS
  if (!pane->treePane) {
#endif

        /* display file size */
	if (visible_cols & COL_SIZE) {
#ifdef _NO_EXTENSIONS
		if (!(attrs&FILE_ATTRIBUTE_DIRECTORY))
#endif
		{
			ULONGLONG size;

			size = ((ULONGLONG)entry->data.nFileSizeHigh << 32) | entry->data.nFileSizeLow;

			_stprintf(buffer, sLongNumFmt, size);

			if (calcWidthCol == -1)
				output_number(pane, dis, col, buffer);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);/*TODO: not in every case time enough */
		}

		col++;
	}

	/* display file date */
	if (visible_cols & (COL_DATE|COL_TIME)) {
#ifndef _NO_EXTENSIONS
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
#endif /* _NO_EXTENSIONS */

		format_date(&entry->data.ftLastWriteTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			output_text(pane, dis, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(pane, dis, col, buffer);
		col++;
	}

#ifndef _NO_EXTENSIONS
	if (entry->bhfi_valid) {
            ULONGLONG index = ((ULONGLONG)entry->bhfi.nFileIndexHigh << 32) | entry->bhfi.nFileIndexLow;

		if (visible_cols & COL_INDEX) {
			_stprintf(buffer, sLongHexFmt, index);

			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_RIGHT);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);

			col++;
		}

		if (visible_cols & COL_LINKS) {
			wsprintf(buffer, sNumFmt, entry->bhfi.nNumberOfLinks);

			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_CENTER);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);

			col++;
		}
	} else
		col += 2;
#endif /* _NO_EXTENSIONS */

	/* show file attributes */
	if (visible_cols & COL_ATTRIBUTES) {
#ifdef _NO_EXTENSIONS
		const static TCHAR s4Tabs[] = {' ','\t',' ','\t',' ','\t',' ','\t',' ','\0'};
		lstrcpy(buffer, s4Tabs);
#else
		const static TCHAR s11Tabs[] = {' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\t',' ','\0'};
		lstrcpy(buffer, s11Tabs);
#endif

		if (attrs & FILE_ATTRIBUTE_NORMAL)					buffer[ 0] = 'N';
		else {
			if (attrs & FILE_ATTRIBUTE_READONLY)			buffer[ 2] = 'R';
			if (attrs & FILE_ATTRIBUTE_HIDDEN)				buffer[ 4] = 'H';
			if (attrs & FILE_ATTRIBUTE_SYSTEM)				buffer[ 6] = 'S';
			if (attrs & FILE_ATTRIBUTE_ARCHIVE)				buffer[ 8] = 'A';
			if (attrs & FILE_ATTRIBUTE_COMPRESSED)			buffer[10] = 'C';
#ifndef _NO_EXTENSIONS
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)			buffer[12] = 'D';
			if (attrs & FILE_ATTRIBUTE_ENCRYPTED)			buffer[14] = 'E';
			if (attrs & FILE_ATTRIBUTE_TEMPORARY)			buffer[16] = 'T';
			if (attrs & FILE_ATTRIBUTE_SPARSE_FILE)			buffer[18] = 'P';
			if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)		buffer[20] = 'Q';
			if (attrs & FILE_ATTRIBUTE_OFFLINE)				buffer[22] = 'O';
			if (attrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)	buffer[24] = 'X';
#endif /* _NO_EXTENSIONS */
		}

		if (calcWidthCol == -1)
			output_tabbed_text(pane, dis, col, buffer);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_tabbed_width(pane, dis, col, buffer);

		col++;
	}

/*TODO
	if (flags.security) {
		const static TCHAR sSecTabs[] = {
			' ','\t',' ','\t',' ','\t',' ',
			' ','\t',' ',
			' ','\t',' ','\t',' ','\t',' ',
			' ','\t',' ',
			' ','\t',' ','\t',' ','\t',' ',
			'\0'
		};

		DWORD rights = get_access_mask();

		lstrcpy(buffer, sSecTabs);

		if (rights & FILE_READ_DATA)			buffer[ 0] = 'R';
		if (rights & FILE_WRITE_DATA)			buffer[ 2] = 'W';
		if (rights & FILE_APPEND_DATA)			buffer[ 4] = 'A';
		if (rights & FILE_READ_EA)				{buffer[6] = 'entry'; buffer[ 7] = 'R';}
		if (rights & FILE_WRITE_EA)				{buffer[9] = 'entry'; buffer[10] = 'W';}
		if (rights & FILE_EXECUTE)				buffer[12] = 'X';
		if (rights & FILE_DELETE_CHILD)			buffer[14] = 'D';
		if (rights & FILE_READ_ATTRIBUTES)		{buffer[16] = 'a'; buffer[17] = 'R';}
		if (rights & FILE_WRITE_ATTRIBUTES)		{buffer[19] = 'a'; buffer[20] = 'W';}
		if (rights & WRITE_DAC)					buffer[22] = 'C';
		if (rights & WRITE_OWNER)				buffer[24] = 'O';
		if (rights & SYNCHRONIZE)				buffer[26] = 'S';

		output_text(dis, col++, buffer, DT_LEFT, 3, psize);
	}

	if (flags.description) {
		get_description(buffer);
		output_text(dis, col++, buffer, 0, psize);
	}
*/

#ifdef _NO_EXTENSIONS
  }

        /* draw focus frame */
	if ((dis->itemState&ODS_FOCUS) && calcWidthCol==-1) {
	        /* Currently [04/2000] Wine neither behaves exactly the same */
	        /* way as WIN 95 nor like Windows NT... */
		HGDIOBJ lastBrush;
		HPEN lastPen;
		HPEN hpen;

		if (!(GetVersion() & 0x80000000)) {	/* Windows NT or higher? */
			LOGBRUSH lb = {PS_SOLID, RGB(255,255,255)};
			hpen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, 0);
		} else
			hpen = CreatePen(PS_DOT, 0, RGB(255,255,255));

		lastPen = SelectPen(dis->hDC, hpen);
		lastBrush = SelectObject(dis->hDC, GetStockObject(HOLLOW_BRUSH));
		SetROP2(dis->hDC, R2_XORPEN);
		Rectangle(dis->hDC, focusRect.left, focusRect.top, focusRect.right, focusRect.bottom);
		SelectObject(dis->hDC, lastBrush);
		SelectObject(dis->hDC, lastPen);
		DeleteObject(hpen);
	}
#endif /* _NO_EXTENSIONS */
}


#ifdef _NO_EXTENSIONS

static void draw_splitbar(HWND hwnd, int x)
{
	RECT rt;
	HDC hdc = GetDC(hwnd);

	GetClientRect(hwnd, &rt);

	rt.left = x - SPLIT_WIDTH/2;
	rt.right = x + SPLIT_WIDTH/2+1;

	InvertRect(hdc, &rt);

	ReleaseDC(hwnd, hdc);
}

#endif /* _NO_EXTENSIONS */


#ifndef _NO_EXTENSIONS

static void set_header(Pane* pane)
{
	HD_ITEM item;
	int scroll_pos = GetScrollPos(pane->hwnd, SB_HORZ);
	int i=0, x=0;

	item.mask = HDI_WIDTH;
	item.cxy = 0;

	for(; x+pane->widths[i]<scroll_pos && i<COLUMNS; i++) {
		x += pane->widths[i];
		(void)Header_SetItem(pane->hwndHeader, i, &item);
	}

	if (i < COLUMNS) {
		x += pane->widths[i];
		item.cxy = x - scroll_pos;
		(void)Header_SetItem(pane->hwndHeader, i++, &item);

		for(; i<COLUMNS; i++) {
			item.cxy = pane->widths[i];
			x += pane->widths[i];
			(void)Header_SetItem(pane->hwndHeader, i, &item);
		}
	}
}

static LRESULT pane_notify(Pane* pane, NMHDR* pnmh)
{
	switch(pnmh->code) {
		case HDN_TRACK:
		case HDN_ENDTRACK: {
			HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
			int idx = phdn->iItem;
			int dx = phdn->pitem->cxy - pane->widths[idx];
			int i;

			RECT clnt;
			GetClientRect(pane->hwnd, &clnt);

			/* move immediate to simulate HDS_FULLDRAG (for now [04/2000] not really needed with WINELIB) */
			(void)Header_SetItem(pane->hwndHeader, idx, phdn->pitem);

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

				if (pnmh->code == HDN_ENDTRACK) {
					ListBox_SetHorizontalExtent(pane->hwnd, pane->positions[COLUMNS]);

					if (GetScrollPos(pane->hwnd, SB_HORZ) != scroll_pos)
						set_header(pane);
				}
			}

			return FALSE;
		}

		case HDN_DIVIDERDBLCLICK: {
			HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
			HD_ITEM item;

			calc_single_width(pane, phdn->iItem);
			item.mask = HDI_WIDTH;
			item.cxy = pane->widths[phdn->iItem];

			(void)Header_SetItem(pane->hwndHeader, phdn->iItem, &item);
			InvalidateRect(pane->hwnd, 0, TRUE);
			break;}
	}

	return 0;
}

#endif /* _NO_EXTENSIONS */


static void scan_entry(ChildWnd* child, Entry* entry, int idx, HWND hwnd)
{
	TCHAR path[MAX_PATH];
	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

	/* delete sub entries in left pane */
	for(;;) {
		LRESULT res = ListBox_GetItemData(child->left.hwnd, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=entry->level)
			break;

		(void)ListBox_DeleteString(child->left.hwnd, idx+1);
	}

	/* empty right pane */
	(void)ListBox_ResetContent(child->right.hwnd);

	/* release memory */
	free_entries(entry);

	/* read contents from disk */
#ifdef _SHELL_FOLDERS
	if (entry->etype == ET_SHELL)
	{
		read_directory(entry, NULL, child->sortOrder, hwnd);
	}
	else
#endif
	{
		get_path(entry, path);
		read_directory(entry, path, child->sortOrder, hwnd);
	}

	/* insert found entries in right pane */
	insert_entries(&child->right, entry->down, child->filter_pattern, child->filter_flags, -1);
	calc_widths(&child->right, FALSE);
#ifndef _NO_EXTENSIONS
	set_header(&child->right);
#endif

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

	idx = ListBox_FindItemData(child->left.hwnd, 0, dir);

	dir->expanded = TRUE;

	/* insert entries in left pane */
	insert_entries(&child->left, p, NULL, TF_ALL, idx);

	if (!child->header_wdths_ok) {
		if (calc_widths(&child->left, FALSE)) {
#ifndef _NO_EXTENSIONS
			set_header(&child->left);
#endif

			child->header_wdths_ok = TRUE;
		}
	}

	return TRUE;
}


static void collapse_entry(Pane* pane, Entry* dir)
{
	int idx = ListBox_FindItemData(pane->hwnd, 0, dir);

	ShowWindow(pane->hwnd, SW_HIDE);

	/* hide sub entries */
	for(;;) {
		LRESULT res = ListBox_GetItemData(pane->hwnd, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=dir->level)
			break;

		(void)ListBox_DeleteString(pane->hwnd, idx+1);
	}

	dir->expanded = FALSE;

	ShowWindow(pane->hwnd, SW_SHOW);
}


static void refresh_right_pane(ChildWnd* child)
{
	(void)ListBox_ResetContent(child->right.hwnd);
	insert_entries(&child->right, child->right.root, child->filter_pattern, child->filter_flags, -1);
	calc_widths(&child->right, FALSE);

#ifndef _NO_EXTENSIONS
	set_header(&child->right);
#endif
}

static void set_curdir(ChildWnd* child, Entry* entry, int idx, HWND hwnd)
{
	TCHAR path[MAX_PATH];

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
	lstrcpy(child->path, path);

	if (child->hwnd)	/* only change window title, if the window already exists */
		SetWindowText(child->hwnd, path);

	if (path[0])
		if (SetCurrentDirectory(path))
			set_space_status();
}


static void refresh_child(ChildWnd* child)
{
	TCHAR path[MAX_PATH], drv[_MAX_DRIVE+1];
	Entry* entry;
	int idx;

	get_path(child->left.cur, path);
	_tsplitpath(path, drv, NULL, NULL, NULL);

	child->right.root = NULL;

	scan_entry(child, &child->root.entry, 0, child->hwnd);

#ifdef _SHELL_FOLDERS
	if (child->root.entry.etype == ET_SHELL)
		entry = read_tree(&child->root, NULL, get_path_pidl(path,child->hwnd), drv, child->sortOrder, child->hwnd);
	else
#endif
		entry = read_tree(&child->root, path, NULL, drv, child->sortOrder, child->hwnd);

	if (!entry)
		entry = &child->root.entry;

	insert_entries(&child->left, child->root.entry.down, NULL, TF_ALL, 0);

	set_curdir(child, entry, 0, child->hwnd);

	idx = ListBox_FindItemData(child->left.hwnd, 0, child->left.cur);
	(void)ListBox_SetCurSel(child->left.hwnd, idx);
}


static void create_drive_bar(void)
{
	TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0};
#ifndef _NO_EXTENSIONS
	TCHAR b1[BUFFER_LEN];
#endif
	int btn = 1;
	PTSTR p;

	GetLogicalDriveStrings(BUFFER_LEN, Globals.drives);

	Globals.hdrivebar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,
				IDW_DRIVEBAR, 2, Globals.hInstance, IDB_DRIVEBAR, &drivebarBtn,
				0, 16, 13, 16, 13, sizeof(TBBUTTON));

#ifndef _NO_EXTENSIONS
#ifdef __WINE__
	/* insert unix file system button */
	b1[0] = '/';
	b1[1] = '\0';
	b1[2] = '\0';
	SendMessage(Globals.hdrivebar, TB_ADDSTRING, 0, (LPARAM)b1);

	drivebarBtn.idCommand = ID_DRIVE_UNIX_FS;
	SendMessage(Globals.hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	drivebarBtn.iString++;
#endif
#ifdef _SHELL_FOLDERS
	/* insert shell namespace button */
	load_string(b1, IDS_SHELL);
	b1[lstrlen(b1)+1] = '\0';
	SendMessage(Globals.hdrivebar, TB_ADDSTRING, 0, (LPARAM)b1);

	drivebarBtn.idCommand = ID_DRIVE_SHELL_NS;
	SendMessage(Globals.hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	drivebarBtn.iString++;
#endif

	/* register windows drive root strings */
	SendMessage(Globals.hdrivebar, TB_ADDSTRING, 0, (LPARAM)Globals.drives);
#endif

	drivebarBtn.idCommand = ID_DRIVE_FIRST;

	for(p=Globals.drives; *p; ) {
#ifdef _NO_EXTENSIONS
		/* insert drive letter */
		TCHAR b[3] = {tolower(*p)};
		SendMessage(Globals.hdrivebar, TB_ADDSTRING, 0, (LPARAM)b);
#endif
		switch(GetDriveType(p)) {
			case DRIVE_REMOVABLE:	drivebarBtn.iBitmap = 1;	break;
			case DRIVE_CDROM:		drivebarBtn.iBitmap = 3;	break;
			case DRIVE_REMOTE:		drivebarBtn.iBitmap = 4;	break;
			case DRIVE_RAMDISK:		drivebarBtn.iBitmap = 5;	break;
			default:/*DRIVE_FIXED*/	drivebarBtn.iBitmap = 2;
		}

		SendMessage(Globals.hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
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
	SendMessage(Globals.hMainWnd, WM_SIZE, 0, MAKELONG(rect.right, rect.bottom));
}


static BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow)
{
	HINSTANCE hinst = ShellExecute(hwnd, NULL/*operation*/, cmd, NULL/*parameters*/, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}


static BOOL launch_entry(Entry* entry, HWND hwnd, UINT nCmdShow)
{
	TCHAR cmd[MAX_PATH];

#ifdef _SHELL_FOLDERS
	if (entry->etype == ET_SHELL) {
		BOOL ret = TRUE;

		SHELLEXECUTEINFO shexinfo;

		shexinfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shexinfo.fMask = SEE_MASK_IDLIST;
		shexinfo.hwnd = hwnd;
		shexinfo.lpVerb = NULL;
		shexinfo.lpFile = NULL;
		shexinfo.lpParameters = NULL;
		shexinfo.lpDirectory = NULL;
		shexinfo.nShow = nCmdShow;
		shexinfo.lpIDList = get_to_absolute_pidl(entry, hwnd);

		if (!ShellExecuteEx(&shexinfo)) {
			display_error(hwnd, GetLastError());
			ret = FALSE;
		}

		if (shexinfo.lpIDList != entry->pidl)
			IMalloc_Free(Globals.iMalloc, shexinfo.lpIDList);

		return ret;
	}
#endif

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
			scan_entry(child, entry, ListBox_GetCurSel(child->left.hwnd), hwnd);

#ifndef _NO_EXTENSIONS
		if (entry->data.cFileName[0]=='.' && entry->data.cFileName[1]=='\0')
			return;
#endif

		if (entry->data.cFileName[0]=='.' && entry->data.cFileName[1]=='.' && entry->data.cFileName[2]=='\0') {
			entry = child->left.cur->up;
			collapse_entry(&child->left, entry);
			goto focus_entry;
		} else if (entry->expanded)
			collapse_entry(pane, child->left.cur);
		else {
			expand_entry(child, child->left.cur);

			if (!pane->treePane) focus_entry: {
				int idx = ListBox_FindItemData(child->left.hwnd, ListBox_GetCurSel(child->left.hwnd), entry);
				(void)ListBox_SetCurSel(child->left.hwnd, idx);
				set_curdir(child, entry, idx, hwnd);
			}
		}

		if (!scanned_old) {
			calc_widths(pane, FALSE);

#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
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
#ifndef _NO_EXTENSIONS
				set_header(pane);
#endif
				InvalidateRect(pane->hwnd, 0, TRUE);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND|MF_CHECKED);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
			}
			break;

		case ID_VIEW_ALL_ATTRIBUTES:
			if (pane->visible_cols != COL_ALL) {
				pane->visible_cols = COL_ALL;
				calc_widths(pane, TRUE);
#ifndef _NO_EXTENSIONS
				set_header(pane);
#endif
				InvalidateRect(pane->hwnd, 0, TRUE);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND|MF_CHECKED);
				CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
			}
			break;

#ifndef _NO_EXTENSIONS
		case ID_PREFERRED_SIZES: {
			calc_widths(pane, TRUE);
			set_header(pane);
			InvalidateRect(pane->hwnd, 0, TRUE);
			break;}
#endif

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


static BOOL is_directory(LPCTSTR target)
{
	/*TODO correctly handle UNIX paths */
	DWORD target_attr = GetFileAttributes(target);

	if (target_attr == INVALID_FILE_ATTRIBUTES)
		return FALSE;

	return target_attr&FILE_ATTRIBUTE_DIRECTORY? TRUE: FALSE;
}

static BOOL prompt_target(Pane* pane, LPTSTR source, LPTSTR target)
{
	TCHAR path[MAX_PATH];
	int len;

	get_path(pane->cur, path);

	if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_SELECT_DESTINATION), pane->hwnd, DestinationDlgProc, (LPARAM)path) != IDOK)
		return FALSE;

	get_path(pane->cur, source);

	/* convert relative targets to absolute paths */
	if (path[0]!='/' && path[1]!=':') {
		get_path(pane->cur->up, target);
		len = lstrlen(target);

		if (target[len-1]!='\\' && target[len-1]!='/')
			target[len++] = '/';

		lstrcpy(target+len, path);
	} else
		lstrcpy(target, path);

	/* If the target already exists as directory, create a new target below this. */
	if (is_directory(path)) {
		TCHAR fname[_MAX_FNAME], ext[_MAX_EXT];
		const static TCHAR sAppend[] = {'%','s','/','%','s','%','s','\0'};

		_tsplitpath(source, NULL, NULL, fname, ext);

		wsprintf(target, sAppend, path, fname, ext);
	}

	return TRUE;
}


static IContextMenu2* s_pctxmenu2 = NULL;

#ifndef __MINGW32__	/* IContextMenu3 missing in MinGW (as of 6.2.2005) */
static IContextMenu3* s_pctxmenu3 = NULL;
#endif

static void CtxMenu_reset(void)
{
	s_pctxmenu2 = NULL;

#ifndef __MINGW32__	/* IContextMenu3 missing in MinGW (as of 6.2.2005) */
	s_pctxmenu3 = NULL;
#endif
}

static IContextMenu* CtxMenu_query_interfaces(IContextMenu* pcm1)
{
	IContextMenu* pcm = NULL;

	CtxMenu_reset();

#ifndef __MINGW32__	/* IContextMenu3 missing in MinGW (as of 6.2.2005) */
	if (IUnknown_QueryInterface(pcm1, &IID_IContextMenu3, (void**)&pcm) == NOERROR)
		s_pctxmenu3 = (LPCONTEXTMENU3)pcm;
	else
#endif
	if (IUnknown_QueryInterface(pcm1, &IID_IContextMenu2, (void**)&pcm) == NOERROR)
		s_pctxmenu2 = (LPCONTEXTMENU2)pcm;

	if (pcm) {
		IUnknown_Release(pcm1);
		return pcm;
	} else
		return pcm1;
}

static BOOL CtxMenu_HandleMenuMsg(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
#ifndef __MINGW32__	/* IContextMenu3 missing in MinGW (as of 6.2.2005) */
	if (s_pctxmenu3) {
		if (SUCCEEDED((*s_pctxmenu3->lpVtbl->HandleMenuMsg)(s_pctxmenu3, nmsg, wparam, lparam)))
			return TRUE;
	}
#endif

	if (s_pctxmenu2)
		if (SUCCEEDED((*s_pctxmenu2->lpVtbl->HandleMenuMsg)(s_pctxmenu2, nmsg, wparam, lparam)))
			return TRUE;

	return FALSE;
}


static HRESULT ShellFolderContextMenu(IShellFolder* shell_folder, HWND hwndParent, int cidl, LPCITEMIDLIST* apidl, int x, int y)
{
	IContextMenu* pcm;
	BOOL executed = FALSE;

	HRESULT hr = IShellFolder_GetUIObjectOf(shell_folder, hwndParent, cidl, apidl, &IID_IContextMenu, NULL, (LPVOID*)&pcm);
/*	HRESULT hr = CDefFolderMenu_Create2(dir?dir->_pidl:DesktopFolder(), hwndParent, 1, &pidl, shell_folder, NULL, 0, NULL, &pcm); */

	if (SUCCEEDED(hr)) {
		HMENU hmenu = CreatePopupMenu();

		pcm = CtxMenu_query_interfaces(pcm);

		if (hmenu) {
			hr = (*pcm->lpVtbl->QueryContextMenu)(pcm, hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, CMF_NORMAL);

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

					hr = (*pcm->lpVtbl->InvokeCommand)(pcm, &cmi);
					executed = TRUE;
				}
			} else
				CtxMenu_reset();
		}

		IUnknown_Release(pcm);
	}

	return FAILED(hr)? hr: executed? S_OK: S_FALSE;
}


static LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	ChildWnd* child = (ChildWnd*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HBRUSH lastBrush;
			RECT rt;
			GetClientRect(hwnd, &rt);
			BeginPaint(hwnd, &ps);
			rt.left = child->split_pos-SPLIT_WIDTH/2;
			rt.right = child->split_pos+SPLIT_WIDTH/2+1;
			lastBrush = SelectBrush(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
			Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
			SelectObject(ps.hdc, lastBrush);
#ifdef _NO_EXTENSIONS
			rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
			FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
#endif
			EndPaint(hwnd, &ps);
			break;}

		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);

				if (pt.x>=child->split_pos-SPLIT_WIDTH/2 && pt.x<child->split_pos+SPLIT_WIDTH/2+1) {
					SetCursor(LoadCursor(0, IDC_SIZEWE));
					return TRUE;
				}
			}
			goto def;

		case WM_LBUTTONDOWN: {
			RECT rt;
			int x = GET_X_LPARAM(lparam);

			GetClientRect(hwnd, &rt);

			if (x>=child->split_pos-SPLIT_WIDTH/2 && x<child->split_pos+SPLIT_WIDTH/2+1) {
				last_split = child->split_pos;
#ifdef _NO_EXTENSIONS
				draw_splitbar(hwnd, last_split);
#endif
				SetCapture(hwnd);
			}

			break;}

		case WM_LBUTTONUP:
			if (GetCapture() == hwnd) {
#ifdef _NO_EXTENSIONS
				RECT rt;
				int x = LOWORD(lparam);
				draw_splitbar(hwnd, last_split);
				last_split = -1;
				GetClientRect(hwnd, &rt);
				child->split_pos = x;
				resize_tree(child, rt.right, rt.bottom);
#endif
				ReleaseCapture();
			}
			break;

#ifdef _NO_EXTENSIONS
		case WM_CAPTURECHANGED:
			if (GetCapture()==hwnd && last_split>=0)
				draw_splitbar(hwnd, last_split);
			break;
#endif

		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
				if (GetCapture() == hwnd) {
					RECT rt;
#ifdef _NO_EXTENSIONS
					draw_splitbar(hwnd, last_split);
#else
					child->split_pos = last_split;
#endif
					GetClientRect(hwnd, &rt);
					resize_tree(child, rt.right, rt.bottom);
					last_split = -1;
					ReleaseCapture();
					SetCursor(LoadCursor(0, IDC_ARROW));
				}
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == hwnd) {
				RECT rt;
				int x = LOWORD(lparam);

#ifdef _NO_EXTENSIONS
				HDC hdc = GetDC(hwnd);
				GetClientRect(hwnd, &rt);

				rt.left = last_split-SPLIT_WIDTH/2;
				rt.right = last_split+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				last_split = x;
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				ReleaseDC(hwnd, hdc);
#else
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
#endif
			}
			break;

#ifndef _NO_EXTENSIONS
		case WM_GETMINMAXINFO:
			DefMDIChildProc(hwnd, nmsg, wparam, lparam);

			{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

			lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
			lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
			break;}
#endif /* _NO_EXTENSIONS */

		case WM_SETFOCUS:
			if (SetCurrentDirectory(child->path))
				set_space_status();
			SetFocus(child->focus_pane? child->right.hwnd: child->left.hwnd);
			break;

		case WM_DISPATCH_COMMAND: {
			Pane* pane = GetFocus()==child->left.hwnd? &child->left: &child->right;

			switch(LOWORD(wparam)) {
				case ID_WINDOW_NEW: {
					ChildWnd* new_child = alloc_child_window(child->path, NULL, hwnd);

					if (!create_child_window(new_child))
						free(new_child);

					break;}

				case ID_REFRESH:
					refresh_drives();
					refresh_child(child);
					break;

				case ID_ACTIVATE:
					activate_entry(child, pane, hwnd);
					break;

				case ID_FILE_MOVE: {
					TCHAR source[BUFFER_LEN], target[BUFFER_LEN];

					if (prompt_target(pane, source, target)) {
						SHFILEOPSTRUCT shfo = {hwnd, FO_MOVE, source, target};

						source[lstrlen(source)+1] = '\0';
						target[lstrlen(target)+1] = '\0';

						if (!SHFileOperation(&shfo))
							refresh_child(child);
					}
					break;}

				case ID_FILE_COPY: {
					TCHAR source[BUFFER_LEN], target[BUFFER_LEN];

					if (prompt_target(pane, source, target)) {
						SHFILEOPSTRUCT shfo = {hwnd, FO_COPY, source, target};

						source[lstrlen(source)+1] = '\0';
						target[lstrlen(target)+1] = '\0';

						if (!SHFileOperation(&shfo))
							refresh_child(child);
					}
					break;}

				case ID_FILE_DELETE: {
					TCHAR path[BUFFER_LEN];
					SHFILEOPSTRUCT shfo = {hwnd, FO_DELETE, path};

					get_path(pane->cur, path);

					path[lstrlen(path)+1] = '\0';

					if (!SHFileOperation(&shfo))
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
					lstrcpy(dlg.pattern, child->filter_pattern);
					dlg.flags = child->filter_flags;

					if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_VIEW_TYPE), hwnd, FilterDialogDlgProc, (LPARAM)&dlg) == IDOK) {
						lstrcpy(child->filter_pattern, dlg.pattern);
						child->filter_flags = dlg.flags;
						refresh_right_pane(child);
					}
					break;}

				case ID_VIEW_SPLIT: {
					last_split = child->split_pos;
#ifdef _NO_EXTENSIONS
					draw_splitbar(hwnd, last_split);
#endif
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
					int idx = ListBox_GetCurSel(pane->hwnd);
					Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, idx);

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

#ifndef _NO_EXTENSIONS
		case WM_NOTIFY: {
			NMHDR* pnmh = (NMHDR*) lparam;
			return pane_notify(pnmh->idFrom==IDW_HEADER_LEFT? &child->left: &child->right, pnmh);}
#endif

#ifdef _SHELL_FOLDERS
		case WM_CONTEXTMENU: {
			POINT pt, pt_clnt;
			Pane* pane;
			int idx;

			 /* first select the current item in the listbox */
			HWND hpanel = (HWND) wparam;
			pt_clnt.x = pt.x = (short)LOWORD(lparam);
			pt_clnt.y = pt.y = (short)HIWORD(lparam);
			ScreenToClient(hpanel, &pt_clnt);
			SendMessage(hpanel, WM_LBUTTONDOWN, 0, MAKELONG(pt_clnt.x, pt_clnt.y));
			SendMessage(hpanel, WM_LBUTTONUP, 0, MAKELONG(pt_clnt.x, pt_clnt.y));

			 /* now create the popup menu using shell namespace and IContextMenu */
			pane = GetFocus()==child->left.hwnd? &child->left: &child->right;
			idx = ListBox_GetCurSel(pane->hwnd);

			if (idx != -1) {
				Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, idx);

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
#endif

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

#ifndef __MINGW32__	/* IContextMenu3 missing in MinGW (as of 6.2.2005) */
		  case WM_MENUCHAR:	/* only supported by IContextMenu3 */
		   if (s_pctxmenu3) {
			   LRESULT lResult = 0;

			   (*s_pctxmenu3->lpVtbl->HandleMenuMsg2)(s_pctxmenu3, nmsg, wparam, lparam, &lResult);

			   return lResult;
		   }

		   break;
#endif

		case WM_SIZE:
			if (wparam != SIZE_MINIMIZED)
				resize_tree(child, LOWORD(lparam), HIWORD(lparam));
			/* fall through */

		default: def:
			return DefMDIChildProc(hwnd, nmsg, wparam, lparam);
	}

	return 0;
}


static LRESULT CALLBACK TreeWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	ChildWnd* child = (ChildWnd*) GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
	Pane* pane = (Pane*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	ASSERT(child);

	switch(nmsg) {
#ifndef _NO_EXTENSIONS
		case WM_HSCROLL:
			set_header(pane);
			break;
#endif

		case WM_SETFOCUS:
			child->focus_pane = pane==&child->right? 1: 0;
			(void)ListBox_SetSel(hwnd, TRUE, 1);
			/*TODO: check menu items */
			break;

		case WM_KEYDOWN:
			if (wparam == VK_TAB) {
				/*TODO: SetFocus(Globals.hdrivebar) */
				SetFocus(child->focus_pane? child->left.hwnd: child->right.hwnd);
			}
	}

	return CallWindowProc(g_orgTreeWndProc, hwnd, nmsg, wparam, lparam);
}


static void InitInstance(HINSTANCE hinstance)
{
	const static TCHAR sFont[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f','\0'};

	WNDCLASSEX wcFrame;
	WNDCLASS wcChild;
	ATOM hChildClass;
	int col;

	INITCOMMONCONTROLSEX icc = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_BAR_CLASSES
	};

	HDC hdc = GetDC(0);

	setlocale(LC_COLLATE, "");	/* set collating rules to local settings for compareName */

	InitCommonControlsEx(&icc);


	/* register frame window class */

	wcFrame.cbSize        = sizeof(WNDCLASSEX);
	wcFrame.style         = 0;
	wcFrame.lpfnWndProc   = FrameWndProc;
	wcFrame.cbClsExtra    = 0;
	wcFrame.cbWndExtra    = 0;
	wcFrame.hInstance     = hinstance;
	wcFrame.hIcon         = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_WINEFILE));
	wcFrame.hCursor       = LoadCursor(0, IDC_ARROW);
	wcFrame.hbrBackground = 0;
	wcFrame.lpszMenuName  = 0;
	wcFrame.lpszClassName = sWINEFILEFRAME;
	wcFrame.hIconSm       = (HICON)LoadImage(hinstance,
											 MAKEINTRESOURCE(IDI_WINEFILE),
											 IMAGE_ICON,
											 GetSystemMetrics(SM_CXSMICON),
											 GetSystemMetrics(SM_CYSMICON),
											 LR_SHARED);

	Globals.hframeClass = RegisterClassEx(&wcFrame);


	/* register tree windows class */

	wcChild.style         = CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW;
	wcChild.lpfnWndProc   = ChildWndProc;
	wcChild.cbClsExtra    = 0;
	wcChild.cbWndExtra    = 0;
	wcChild.hInstance     = hinstance;
	wcChild.hIcon         = 0;
	wcChild.hCursor       = LoadCursor(0, IDC_ARROW);
	wcChild.hbrBackground = 0;
	wcChild.lpszMenuName  = 0;
	wcChild.lpszClassName = sWINEFILETREE;

	hChildClass = RegisterClass(&wcChild);


	Globals.haccel = LoadAccelerators(hinstance, MAKEINTRESOURCE(IDA_WINEFILE));

	Globals.hfont = CreateFont(-MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sFont);

	ReleaseDC(0, hdc);

	Globals.hInstance = hinstance;

#ifdef _SHELL_FOLDERS
	CoInitialize(NULL);
	CoGetMalloc(MEMCTX_TASK, &Globals.iMalloc);
	SHGetDesktopFolder(&Globals.iDesktop);
#ifdef __WINE__
	Globals.cfStrFName = RegisterClipboardFormatA(CFSTR_FILENAME);
#else
	Globals.cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
#endif
#endif

	/* load column strings */
	col = 1;

	load_string(g_pos_names[col++], IDS_COL_NAME);
	load_string(g_pos_names[col++], IDS_COL_SIZE);
	load_string(g_pos_names[col++], IDS_COL_CDATE);
#ifndef _NO_EXTENSIONS
	load_string(g_pos_names[col++], IDS_COL_ADATE);
	load_string(g_pos_names[col++], IDS_COL_MDATE);
	load_string(g_pos_names[col++], IDS_COL_IDX);
	load_string(g_pos_names[col++], IDS_COL_LINKS);
#endif
	load_string(g_pos_names[col++], IDS_COL_ATTR);
#ifndef _NO_EXTENSIONS
	load_string(g_pos_names[col++], IDS_COL_SEC);
#endif
}


static void show_frame(HWND hwndParent, int cmdshow, LPCTSTR path)
{
	const static TCHAR sMDICLIENT[] = {'M','D','I','C','L','I','E','N','T','\0'};

	TCHAR buffer[MAX_PATH], b1[BUFFER_LEN];
	ChildWnd* child;
	HMENU hMenuFrame, hMenuWindow;

	CLIENTCREATESTRUCT ccs;

	if (Globals.hMainWnd)
		return;

	hMenuFrame = LoadMenu(Globals.hInstance, MAKEINTRESOURCE(IDM_WINEFILE));
	hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);

	Globals.hMenuFrame = hMenuFrame;
	Globals.hMenuView = GetSubMenu(hMenuFrame, 3);
	Globals.hMenuOptions = GetSubMenu(hMenuFrame, 4);

	ccs.hWindowMenu  = hMenuWindow;
	ccs.idFirstChild = IDW_FIRST_CHILD;


	/* create main window */
	Globals.hMainWnd = CreateWindowEx(0, (LPCTSTR)(int)Globals.hframeClass, RS(b1,IDS_WINE_FILE), WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					hwndParent, Globals.hMenuFrame, Globals.hInstance, 0/*lpParam*/);


	Globals.hmdiclient = CreateWindowEx(0, sMDICLIENT, NULL,
					WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
					0, 0, 0, 0,
					Globals.hMainWnd, 0, Globals.hInstance, &ccs);


	CheckMenuItem(Globals.hMenuOptions, ID_VIEW_DRIVE_BAR, MF_BYCOMMAND|MF_CHECKED);

	create_drive_bar();

	{
		TBBUTTON toolbarBtns[] = {
			{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
			{0, ID_WINDOW_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
/*TODO
			{4, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
			{5, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
*/		};

		Globals.htoolbar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE,
			IDW_TOOLBAR, 2, Globals.hInstance, IDB_TOOLBAR, toolbarBtns,
			sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));
		CheckMenuItem(Globals.hMenuOptions, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);
	}

	Globals.hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, Globals.hMainWnd, IDW_STATUSBAR);
	CheckMenuItem(Globals.hMenuOptions, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

/* CreateStatusWindow does not accept WS_BORDER
	Globals.hstatusbar = CreateWindowEx(WS_EX_NOPARENTNOTIFY, STATUSCLASSNAME, 0,
					WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CCS_NODIVIDER, 0,0,0,0,
					Globals.hMainWnd, (HMENU)IDW_STATUSBAR, hinstance, 0);*/

	/*TODO: read paths and window placements from registry */

	if (!path || !*path) {
		GetCurrentDirectory(MAX_PATH, buffer);
		path = buffer;
	}

	ShowWindow(Globals.hMainWnd, cmdshow);

//#if defined(_SHELL_FOLDERS) && !defined(__WINE__)
//	 // Shell Namespace as default:
//	child = alloc_child_window(path, get_path_pidl(path,Globals.hMainWnd), Globals.hMainWnd);
//#else
	child = alloc_child_window(path, NULL, Globals.hMainWnd);
//#endif

	child->pos.showCmd = SW_SHOWMAXIMIZED;
	child->pos.rcNormalPosition.left = 0;
	child->pos.rcNormalPosition.top = 0;
	child->pos.rcNormalPosition.right = 320;
	child->pos.rcNormalPosition.bottom = 280;

	if (!create_child_window(child))
		free(child);

	SetWindowPlacement(child->hwnd, &child->pos);

	Globals.himl = ImageList_LoadBitmap(Globals.hInstance, MAKEINTRESOURCE(IDB_IMAGES), 16, 0, RGB(0,255,0));

	Globals.prescan_node = FALSE;

	UpdateWindow(Globals.hMainWnd);

	if (path && path[0])
	{
		int index,count;
		TCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
		TCHAR fullname[_MAX_FNAME+_MAX_EXT+1];

		memset(name,0,sizeof(name));
		memset(name,0,sizeof(ext));
		_tsplitpath(path, drv, dir, name, ext);
		if (name[0])
		{
			count = ListBox_GetCount(child->right.hwnd);
			lstrcpy(fullname,name);
			lstrcat(fullname,ext);

			for (index = 0; index < count; index ++)
			{
				Entry* entry = (Entry*) ListBox_GetItemData(child->right.hwnd,
						index);
				if (lstrcmp(entry->data.cFileName,fullname)==0 ||
						lstrcmp(entry->data.cAlternateFileName,fullname)==0)
				{
					(void)ListBox_SetCurSel(child->right.hwnd, index);
					SetFocus(child->right.hwnd);
					break;
				}
			}
		}
	}
}

static void ExitInstance(void)
{
#ifdef _SHELL_FOLDERS
	IShellFolder_Release(Globals.iDesktop);
	IMalloc_Release(Globals.iMalloc);
	CoUninitialize();
#endif

	DeleteObject(Globals.hfont);
	ImageList_Destroy(Globals.himl);
}

#ifdef _NO_EXTENSIONS

/* search for already running win[e]files */

static int g_foundPrevInstance = 0;

static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TCHAR cls[128];

	GetClassName(hwnd, cls, 128);

	if (!lstrcmp(cls, (LPCTSTR)lparam)) {
		g_foundPrevInstance++;
		return FALSE;
	}

	return TRUE;
}

/* search for window of given class name to allow only one running instance */
static int find_window_class(LPCTSTR classname)
{
	EnumWindows(EnumWndProc, (LPARAM)classname);

	if (g_foundPrevInstance)
		return 1;

	return 0;
}

#endif

static int winefile_main(HINSTANCE hinstance, int cmdshow, LPCTSTR path)
{
	MSG msg;

	InitInstance(hinstance);

	if (cmdshow == SW_SHOWNORMAL)
	        /*TODO: read window placement from registry */
		cmdshow = SW_MAXIMIZE;

	show_frame(0, cmdshow, path);

	while(GetMessage(&msg, 0, 0, 0)) {
		if (Globals.hmdiclient && TranslateMDISysAccel(Globals.hmdiclient, &msg))
			continue;

		if (Globals.hMainWnd && TranslateAccelerator(Globals.hMainWnd, Globals.haccel, &msg))
			continue;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ExitInstance();

	return msg.wParam;
}


#if defined(UNICODE) && defined(_MSC_VER)
int APIENTRY wWinMain(HINSTANCE hinstance, HINSTANCE previnstance, LPWSTR cmdline, int cmdshow)
#else
int APIENTRY WinMain(HINSTANCE hinstance, HINSTANCE previnstance, LPSTR cmdline, int cmdshow)
#endif
{
#ifdef _NO_EXTENSIONS
	if (find_window_class(sWINEFILEFRAME))
		return 1;
#endif

#if defined(UNICODE) && !defined(_MSC_VER)
	{ /* convert ANSI cmdline into WCS path string */
	TCHAR buffer[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, cmdline, -1, buffer, MAX_PATH);
	winefile_main(hinstance, cmdshow, buffer);
	}
#else
	winefile_main(hinstance, cmdshow, cmdline);
#endif

	return 0;
}
