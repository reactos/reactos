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
 // globals.h
 //
 // Martin Fuchs, 23.07.2003
 //


#include "utility/xmlstorage.h"

using namespace XMLStorage;

#include "taskbar/favorites.h"


 /// management of file types
struct FileTypeInfo {
	String	_classname;
	String	_displayname;
	bool	_neverShowExt;
};

struct FileTypeManager : public map<String, FileTypeInfo>
{
	typedef map<String, FileTypeInfo> super;

	const FileTypeInfo& operator[](String ext);

	static bool is_exe_file(LPCTSTR ext);

	LPCTSTR set_type(struct Entry* entry, bool dont_hide_ext=false);
};


enum ICON_TYPE {
	IT_STATIC,
	IT_CACHED,
	IT_DYNAMIC,
	IT_SYSCACHE
};

enum ICON_ID {
	ICID_UNKNOWN,
	ICID_NONE,

	ICID_FOLDER,
	//ICID_DOCUMENT,
	ICID_APP,
	ICID_EXPLORER,

	ICID_CONFIG,
	ICID_DOCUMENTS,
	ICID_FAVORITES,
	ICID_INFO,
	ICID_APPS,
	ICID_SEARCH,
	ICID_ACTION,
	ICID_SEARCH_DOC,
	ICID_PRINTER,
	ICID_NETWORK,
	ICID_COMPUTER,
	ICID_LOGOFF,
    ICID_SHUTDOWN,
    ICID_RESTART,
	ICID_BOOKMARK,
    ICID_MINIMIZE,
    ICID_CONTROLPAN,
    ICID_DESKSETTING,
    ICID_NETCONNS,
    ICID_ADMIN,
    ICID_RECENT,

	ICID_DYNAMIC
};

struct Icon {
	Icon();
	Icon(ICON_ID id, UINT nid);
	Icon(ICON_ID id, UINT nid, int icon_size);
	Icon(ICON_TYPE itype, int id, HICON hIcon);
	Icon(ICON_TYPE itype, int id, int sys_idx);

	operator ICON_ID() const {return _id;}

	void	draw(HDC hdc, int x, int y, int cx, int cy, COLORREF bk_color, HBRUSH bk_brush) const;
	HBITMAP	create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd) const;
	int		add_to_imagelist(HIMAGELIST himl, HDC hdc_wnd, COLORREF bk_color=GetSysColor(COLOR_WINDOW), HBRUSH bk_brush=GetSysColorBrush(COLOR_WINDOW)) const;

	int		get_sysiml_idx() const {return _itype==IT_SYSCACHE? _sys_idx: -1;}
	HICON	get_hicon() const {return _itype!=IT_SYSCACHE? _hicon: 0;}
	ICON_TYPE get_icontype() const { return _itype; }

	bool	destroy() {if (_itype == IT_DYNAMIC) {DestroyIcon(_hicon); return true;} else return false;}

protected:
	ICON_ID	_id;
	ICON_TYPE _itype;
	HICON	_hicon;
	int		_sys_idx;
};

struct SysCacheIcon : public Icon {
	SysCacheIcon(int id, int sys_idx)
	 :	Icon(IT_SYSCACHE, id, sys_idx) {}
};

struct IconCache {
	IconCache() : _himlSys_small(0) {}

	virtual ~IconCache();
	void	init();

	const Icon&	extract(LPCTSTR path, ICONCACHE_FLAGS flags=ICF_NORMAL);
	const Icon&	extract(LPCTSTR path, int icon_idx, ICONCACHE_FLAGS flags=ICF_HICON);
	const Icon&	extract(IExtractIcon* pExtract, LPCTSTR path, int icon_idx, ICONCACHE_FLAGS flags=ICF_HICON);
	const Icon&	extract(LPCITEMIDLIST pidl, ICONCACHE_FLAGS flags=ICF_NORMAL);

	const Icon&	add(HICON hIcon, ICON_TYPE type=IT_DYNAMIC);
	const Icon&	add(int sys_idx/*, ICON_TYPE type=IT_SYSCACHE*/);

	const Icon&	get_icon(int icon_id);

	HIMAGELIST get_sys_imagelist() const {return _himlSys_small;}

	void	free_icon(int icon_id);

protected:
	static int s_next_id;

	typedef map<int, Icon> IconMap;
	IconMap	_icons;

	typedef pair<String,int/*ICONCACHE_FLAGS*/> CacheKey;
	typedef map<CacheKey, ICON_ID> PathCacheMap;
	PathCacheMap _pathCache;

	typedef pair<String,pair<int,int/*ICONCACHE_FLAGS*/> > IdxCacheKey;
	typedef map<IdxCacheKey, ICON_ID> IdxCacheMap;
	IdxCacheMap _idxCache;

	typedef pair<ShellPath,int/*ICONCACHE_FLAGS*/> PidlCacheKey;
	typedef map<PidlCacheKey, ICON_ID> PidlCacheMap;
	PidlCacheMap _pidlcache;

	HIMAGELIST _himlSys_small;
};


#define ICON_SIZE_SMALL		16	// GetSystemMetrics(SM_CXSMICON)
#define ICON_SIZE_MIDDLE	24	// special size for start menu root icons
#define ICON_SIZE_LARGE		32	// GetSystemMetrics(SM_CXICON)

#define STARTMENUROOT_ICON_SIZE		ICON_SIZE_MIDDLE	// ICON_SIZE_LARGE

#define ICON_SIZE_FROM_ICF(flags)	(flags&ICF_LARGE? ICON_SIZE_LARGE: flags&ICF_MIDDLE? ICON_SIZE_MIDDLE: ICON_SIZE_SMALL)
#define ICF_FROM_ICON_SIZE(size)	(size>=ICON_SIZE_LARGE? ICF_LARGE: size>=ICON_SIZE_MIDDLE? ICF_MIDDLE: (ICONCACHE_FLAGS)0)


 /// create a bitmap from an icon
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd/*, int icon_size*/);

 /// add icon with alpha channel to imagelist using the specified background color
extern int ImageList_AddAlphaIcon(HIMAGELIST himl, HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);

 /// retrieve icon from window
extern HICON get_window_icon_small(HWND hwnd);
extern HICON get_window_icon_big(HWND hwnd, bool allow_from_class=true);


 /// desktop management
#ifdef _USE_HDESK

typedef auto_ptr<struct DesktopThread> DesktopThreadPtr;

struct Desktop
{
	HDESK	_hdesktop;
//	HWINSTA	_hwinsta;
	DesktopThreadPtr _pThread;
	WindowHandle _hwndDesktop;

	Desktop(HDESK hdesktop=0/*, HWINSTA hwinsta=0*/);
	~Desktop();
};

typedef auto_ptr<Desktop> DesktopPtr;
typedef DesktopPtr DesktopRef;

 /// Thread class for additional desktops
struct DesktopThread : public Thread
{
	DesktopThread(Desktop& desktop)
	 :	_desktop(desktop)
	{
	}

	int	Run();

protected:
	Desktop&	_desktop;
};

#else

typedef pair<HWND, DWORD> MinimizeStruct;

struct Desktop
{
	set<HWND> _windows;
	WindowHandle _hwndForeground;
	list<MinimizeStruct> _minimized;
};
typedef Desktop DesktopRef;

#endif


#define	DESKTOP_COUNT	2

struct Desktops : public vector<DesktopRef>
{
	Desktops();
	~Desktops();

	void	init();
	void	SwitchToDesktop(int idx);
	void	ToggleMinimize();

#ifdef _USE_HDESK
	DesktopRef& get_current_Desktop() {return (*this)[_current_desktop];}
#endif

	int		_current_desktop;
};


 /// structure containing global variables of Explorer
extern struct ExplorerGlobals
{
	ExplorerGlobals();

	void	init(HINSTANCE hInstance);

	void	read_persistent();
	void	write_persistent();

	XMLPos	get_cfg();
	XMLPos	get_cfg(const char* path);

	HINSTANCE	_hInstance;
	UINT		_cfStrFName;

#ifndef ROSSHELL
	ATOM		_hframeClass;
	HWND		_hMainWnd;
	bool		_desktop_mode;
	bool		_prescan_nodes;
#endif

	FILE*		_log;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	DWORD(STDAPICALLTYPE* _SHRestricted)(RESTRICTIONS);
#endif

	FileTypeManager	_ftype_mgr;
	IconCache	_icon_cache;

	HWND		_hwndDesktopBar;
	HWND		_hwndShellView;
	HWND		_hwndDesktop;

	Desktops	_desktops;

	XMLDoc		_cfg;
	String		_cfg_dir;
	String		_cfg_path;

	Favorites	_favorites;
	String		_favorites_path;
} g_Globals;


 /// convenient loading of string resources
struct ResString : public String
{
	ResString(UINT nid);
};

 /// convenient loading of standard (32x32) icon resources
struct ResIcon
{
	ResIcon(UINT nid);

	operator HICON() const {return _hicon;}

protected:
	HICON	_hicon;
};

 /// convenient loading of small (16x16) icon resources
struct SmallIcon
{
	SmallIcon(UINT nid);

	operator HICON() const {return _hicon;}

protected:
	HICON	_hicon;
};

 /// convenient loading of icon resources with specified sizes
struct ResIconEx
{
	ResIconEx(UINT nid, int w, int h);

	operator HICON() const {return _hicon;}

protected:
	HICON	_hicon;
};

 /// set big and small icons out of the resources for a window
extern void SetWindowIcon(HWND hwnd, UINT nid);

 /// convenient loading of bitmap resources
struct ResBitmap
{
	ResBitmap(UINT nid);
	~ResBitmap() {DeleteObject(_hBmp);}

	operator HBITMAP() const {return _hBmp;}

protected:
	HBITMAP	_hBmp;
};
