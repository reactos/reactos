/*
 * Copyright 2003, 2004, 2005, 2006 Martin Fuchs
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
 // explorer.cpp
 //
 // Martin Fuchs, 23.07.2003
 //
 // Credits: Thanks to Leon Finker for his explorer cabinet window example
 //


#include <precomp.h>	// <precomp.h> instead of "precomp.h" because the ROS build system needs this to find the precompiled header file (*.gch) in the output directory tree

#include "resource.h"

#include <locale.h>	// for setlocale()

#ifndef __WINE__
#include <io.h>		// for dup2()
#include <fcntl.h>	// for _O_RDONLY
#endif

#include "dialogs/settings.h"	// for MdiSdiDlg

#include "services/shellservices.h"


extern "C" int initialize_gdb_stub();	// start up GDB stub


DynamicLoadLibFct<void(__stdcall*)(BOOL)> g_SHDOCVW_ShellDDEInit(TEXT("SHDOCVW"), 118);


ExplorerGlobals g_Globals;


ExplorerGlobals::ExplorerGlobals()
{
	_hInstance = 0;
	_cfStrFName = 0;

#ifndef ROSSHELL
	_hframeClass = 0;
	_hMainWnd = 0;
	_desktop_mode = false;
	_prescan_nodes = false;
#endif

	_log = NULL;
#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	_SHRestricted = 0;
#endif
	_hwndDesktopBar = 0;
	_hwndShellView = 0;
	_hwndDesktop = 0;
}


void ExplorerGlobals::init(HINSTANCE hInstance)
{
	_hInstance = hInstance;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	_SHRestricted = (DWORD(STDAPICALLTYPE*)(RESTRICTIONS)) GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "SHRestricted");
#endif

	_icon_cache.init();
}


void ExplorerGlobals::read_persistent()
{
	 // read configuration file
	_cfg_dir.printf(TEXT("%s\\ReactOS"), (LPCTSTR)SpecialFolderFSPath(CSIDL_APPDATA,0));
	_cfg_path.printf(TEXT("%s\\ros-explorer-cfg.xml"), _cfg_dir.c_str());

	if (!_cfg.read(_cfg_path)) {
		//if (_cfg._last_error != XML_ERROR_NO_ELEMENTS)
		MessageBox(_hwndDesktop, _cfg._errors.str(),
					TEXT("ROS Explorer - reading user settings"), MB_OK);

		_cfg.read(TEXT("explorer-cfg-template.xml"));
	}

	 // read bookmarks
	_favorites_path.printf(TEXT("%s\\ros-explorer-bookmarks.xml"), _cfg_dir.c_str());

	if (!_favorites.read(_favorites_path)) {
		_favorites.import_IE_favorites(0);
		_favorites.write(_favorites_path);
	}
}

void ExplorerGlobals::write_persistent()
{
	 // write configuration file
	RecursiveCreateDirectory(_cfg_dir);

	_cfg.write(_cfg_path);
	_favorites.write(_favorites_path);
}


XMLPos ExplorerGlobals::get_cfg()
{
	XMLPos cfg_pos(&_cfg);

	cfg_pos.smart_create("explorer-cfg");

	return cfg_pos;
}

XMLPos ExplorerGlobals::get_cfg(const char* path)
{
	XMLPos cfg_pos(&_cfg);

	cfg_pos.smart_create("explorer-cfg");
	cfg_pos.create_relative(path);

	return cfg_pos;
}


void _log_(LPCTSTR txt)
{
	FmtString msg(TEXT("%s\n"), txt);

	if (g_Globals._log)
		_fputts(msg, g_Globals._log);

	OutputDebugString(msg);
}


bool FileTypeManager::is_exe_file(LPCTSTR ext)
{
	static const LPCTSTR s_executable_extensions[] = {
		TEXT("COM"),
		TEXT("EXE"),
		TEXT("BAT"),
		TEXT("CMD"),
		TEXT("CMM"),
		TEXT("BTM"),
		TEXT("AWK"),
		0
	};

	TCHAR ext_buffer[_MAX_EXT];
	const LPCTSTR* p;
	LPCTSTR s;
	LPTSTR d;

	for(s=ext+1,d=ext_buffer; (*d=toupper(*s)); s++)
		++d;

	for(p=s_executable_extensions; *p; p++)
		if (!lstrcmp(ext_buffer, *p))
			return true;

	return false;
}


const FileTypeInfo& FileTypeManager::operator[](String ext)
{
	ext.toLower();

	iterator found = find(ext);
	if (found != end())
		return found->second;

	FileTypeInfo& ftype = super::operator[](ext);

	ftype._neverShowExt = false;

	HKEY hkey;
	TCHAR value[MAX_PATH], display_name[MAX_PATH];
	LONG valuelen = sizeof(value);

	if (!RegQueryValue(HKEY_CLASSES_ROOT, ext, value, &valuelen)) {
		ftype._classname = value;

		valuelen = sizeof(display_name);
		if (!RegQueryValue(HKEY_CLASSES_ROOT, ftype._classname, display_name, &valuelen))
			ftype._displayname = display_name;

		if (!RegOpenKey(HKEY_CLASSES_ROOT, ftype._classname, &hkey)) {
			if (!RegQueryValueEx(hkey, TEXT("NeverShowExt"), 0, NULL, NULL, NULL))
				ftype._neverShowExt = true;

			RegCloseKey(hkey);
		}
	}

	return ftype;
}

LPCTSTR FileTypeManager::set_type(Entry* entry, bool dont_hide_ext)
{
	LPCTSTR ext = _tcsrchr(entry->_data.cFileName, TEXT('.'));

	if (ext) {
		const FileTypeInfo& type = (*this)[ext];

		if (!type._displayname.empty())
			entry->_type_name = _tcsdup(type._displayname);

		 // hide some file extensions
		if (type._neverShowExt && !dont_hide_ext) {
			int len = ext - entry->_data.cFileName;
			entry->_display_name = (LPTSTR) malloc((len+1)*sizeof(TCHAR));
			lstrcpyn(entry->_display_name, entry->_data.cFileName, len + 1);
		}

		if (is_exe_file(ext))
			entry->_data.dwFileAttributes |= ATTRIBUTE_EXECUTABLE;
	}

	return ext;
}


Icon::Icon()
 :	_id(ICID_UNKNOWN),
	_itype(IT_STATIC),
	_hicon(0)
{
}

Icon::Icon(ICON_ID id, UINT nid)	//, int cx, int cy
 :	_id(id),
	_itype(IT_STATIC),
	_hicon(ResIcon(nid))	// ResIconEx(nid, cx, cy)
{
}

Icon::Icon(ICON_ID id, UINT nid, int icon_size)
 :	_id(id),
	_itype(IT_STATIC),
	_hicon(ResIconEx(nid, icon_size, icon_size))
{
}

Icon::Icon(ICON_TYPE itype, int id, HICON hIcon)
 :	_id((ICON_ID)id),
	_itype(itype),
	_hicon(hIcon)
{
}

Icon::Icon(ICON_TYPE itype, int id, int sys_idx)
 :	_id((ICON_ID)id),
	_itype(itype),
	_sys_idx(sys_idx)
{
}

void Icon::draw(HDC hdc, int x, int y, int cx, int cy, COLORREF bk_color, HBRUSH bk_brush) const
{
	if (_itype == IT_SYSCACHE)
		ImageList_DrawEx(g_Globals._icon_cache.get_sys_imagelist(), _sys_idx, hdc, x, y, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
	else
		DrawIconEx(hdc, x, y, _hicon, cx, cy, 0, bk_brush, DI_NORMAL);
}

HBITMAP	Icon::create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd) const
{
	if (_itype == IT_SYSCACHE) {
		HIMAGELIST himl = g_Globals._icon_cache.get_sys_imagelist();

		int cx, cy;
		ImageList_GetIconSize(himl, &cx, &cy);

		HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);
		HDC hdc = CreateCompatibleDC(hdc_wnd);
		HBITMAP hbmp_old = SelectBitmap(hdc, hbmp);
		ImageList_DrawEx(himl, _sys_idx, hdc, 0, 0, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
		SelectBitmap(hdc, hbmp_old);
		DeleteDC(hdc);

		return hbmp;
	} else
		return create_bitmap_from_icon(_hicon, hbrBkgnd, hdc_wnd);
}


int Icon::add_to_imagelist(HIMAGELIST himl, HDC hdc_wnd, COLORREF bk_color, HBRUSH bk_brush) const
{
	int ret;

	if (_itype == IT_SYSCACHE) {
		HIMAGELIST himl = g_Globals._icon_cache.get_sys_imagelist();

		int cx, cy;
		ImageList_GetIconSize(himl, &cx, &cy);

		HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);
		HDC hdc = CreateCompatibleDC(hdc_wnd);
		HBITMAP hbmp_old = SelectBitmap(hdc, hbmp);
		ImageList_DrawEx(himl, _sys_idx, hdc, 0, 0, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
		SelectBitmap(hdc, hbmp_old);
		DeleteDC(hdc);

		ret = ImageList_Add(himl, hbmp, 0);

		DeleteObject(hbmp);
	} else
		ret = ImageList_AddAlphaIcon(himl, _hicon, bk_brush, hdc_wnd);

	return ret;
}

HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd/*, int icon_size*/)
{
	int cx = ICON_SIZE_SMALL;
	int cy = ICON_SIZE_SMALL;
	HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);

	MemCanvas canvas;
	BitmapSelection sel(canvas, hbmp);

	RECT rect = {0, 0, cx, cy};
	FillRect(canvas, &rect, hbrush_bkgnd);

	DrawIconEx(canvas, 0, 0, hIcon, cx, cy, 0, hbrush_bkgnd, DI_NORMAL);

	return hbmp;
}

HBITMAP create_small_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
	int cx = GetSystemMetrics(SM_CXSMICON);
	int cy = GetSystemMetrics(SM_CYSMICON);
	HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, cx, cy);

	MemCanvas canvas;
	BitmapSelection sel(canvas, hbmp);

	RECT rect = {0, 0, cx, cy};
	FillRect(canvas, &rect, hbrush_bkgnd);

	DrawIconEx(canvas, 0, 0, hIcon, cx, cy, 0, hbrush_bkgnd, DI_NORMAL);

	return hbmp;
}

int ImageList_AddAlphaIcon(HIMAGELIST himl, HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
	HBITMAP hbmp = create_bitmap_from_icon(hIcon, hbrush_bkgnd, hdc_wnd);

	int ret = ImageList_Add(himl, hbmp, 0);

	DeleteObject(hbmp);

	return ret;
}


int IconCache::s_next_id = ICID_DYNAMIC;


void IconCache::init()
{
	int icon_size = STARTMENUROOT_ICON_SIZE;

	_icons[ICID_NONE]		= Icon(IT_STATIC, ICID_NONE, (HICON)0);

	_icons[ICID_FOLDER]		= Icon(ICID_FOLDER,		IDI_FOLDER);
	//_icons[ICID_DOCUMENT]	= Icon(ICID_DOCUMENT,	IDI_DOCUMENT);
	_icons[ICID_EXPLORER]	= Icon(ICID_EXPLORER,	IDI_EXPLORER);
//	_icons[ICID_APP]		= Icon(ICID_APP,		IDI_APPICON);

	_icons[ICID_CONFIG]		= Icon(ICID_CONFIG,		IDI_CONFIG,		icon_size);
	_icons[ICID_DOCUMENTS]	= Icon(ICID_DOCUMENTS,	IDI_DOCUMENTS,	icon_size);
	_icons[ICID_FAVORITES]	= Icon(ICID_FAVORITES,	IDI_FAVORITES,	icon_size);
	_icons[ICID_INFO]		= Icon(ICID_INFO,		IDI_INFO,		icon_size);
	_icons[ICID_APPS]		= Icon(ICID_APPS,		IDI_APPS,		icon_size);
	_icons[ICID_SEARCH] 	= Icon(ICID_SEARCH, 	IDI_SEARCH,		icon_size);
	_icons[ICID_ACTION] 	= Icon(ICID_ACTION, 	IDI_ACTION,		icon_size);
	_icons[ICID_SEARCH_DOC] = Icon(ICID_SEARCH_DOC, IDI_SEARCH_DOC,	icon_size);
	_icons[ICID_PRINTER]	= Icon(ICID_PRINTER,	IDI_PRINTER,	icon_size);
	_icons[ICID_NETWORK]	= Icon(ICID_NETWORK,	IDI_NETWORK,	icon_size);
	_icons[ICID_COMPUTER]	= Icon(ICID_COMPUTER,	IDI_COMPUTER,	icon_size);
	_icons[ICID_LOGOFF] 	= Icon(ICID_LOGOFF, 	IDI_LOGOFF,		icon_size);
	_icons[ICID_SHUTDOWN]	= Icon(ICID_SHUTDOWN,	IDI_SHUTDOWN,	icon_size);
	_icons[ICID_RESTART]	= Icon(ICID_RESTART,	IDI_RESTART,	icon_size);
	_icons[ICID_BOOKMARK]	= Icon(ICID_BOOKMARK,	IDI_DOT_TRANS,	icon_size);
	_icons[ICID_MINIMIZE]	= Icon(ICID_MINIMIZE,	IDI_MINIMIZE,	icon_size);
	_icons[ICID_CONTROLPAN] = Icon(ICID_CONTROLPAN, IDI_CONTROLPAN,	icon_size);
	_icons[ICID_DESKSETTING]= Icon(ICID_DESKSETTING,IDI_DESKSETTING,icon_size);
	_icons[ICID_NETCONNS]	= Icon(ICID_NETCONNS,	IDI_NETCONNS,	icon_size);
	_icons[ICID_ADMIN]		= Icon(ICID_ADMIN,		IDI_ADMIN,		icon_size);
	_icons[ICID_RECENT] 	= Icon(ICID_RECENT, 	IDI_RECENT,		icon_size);
}


const Icon& IconCache::extract(LPCTSTR path, ICONCACHE_FLAGS flags)
{
	 // search for matching icon with unchanged flags in the cache
	CacheKey mapkey(path, flags);
	PathCacheMap::iterator found = _pathCache.find(mapkey);

	if (found != _pathCache.end())
		return _icons[found->second];

	 // search for matching icon with handle
	CacheKey mapkey_hicon(path, flags|ICF_HICON);
	if (flags != mapkey_hicon.second) {
		found = _pathCache.find(mapkey_hicon);

		if (found != _pathCache.end())
			return _icons[found->second];
	}

	 // search for matching icon in the system image list cache
	CacheKey mapkey_syscache(path, flags|ICF_SYSCACHE);
	if (flags != mapkey_syscache.second) {
		found = _pathCache.find(mapkey_syscache);

		if (found != _pathCache.end())
			return _icons[found->second];
	}

	SHFILEINFO sfi;

	int shgfi_flags = 0;

	if (flags & ICF_OPEN)
		shgfi_flags |= SHGFI_OPENICON;

	if ((flags&(ICF_LARGE|ICF_MIDDLE|ICF_OVERLAYS|ICF_HICON)) && !(flags&ICF_SYSCACHE)) {
		shgfi_flags |= SHGFI_ICON;

		if (!(flags & (ICF_LARGE|ICF_MIDDLE)))
			shgfi_flags |= SHGFI_SMALLICON;

		if (flags & ICF_OVERLAYS)
			shgfi_flags |= SHGFI_ADDOVERLAYS;

		 // get small/big icons with/without overlays
		if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), shgfi_flags)) {
			const Icon& icon = add(sfi.hIcon, IT_CACHED);

			///@todo limit cache size
			_pathCache[mapkey_hicon] = icon;

			return icon;
		}
	} else {
		assert(!(flags&ICF_OVERLAYS));

		shgfi_flags |= SHGFI_SYSICONINDEX|SHGFI_SMALLICON;

		 // use system image list - the "search program dialog" needs it
		HIMAGELIST himlSys_small = (HIMAGELIST) SHGetFileInfo(path, 0, &sfi, sizeof(sfi), shgfi_flags);

		if (himlSys_small) {
			_himlSys_small = himlSys_small;

			const Icon& icon = add(sfi.iIcon/*, IT_SYSCACHE*/);

			///@todo limit cache size
			_pathCache[mapkey_syscache] = icon;

			return icon;
		}
	}

	return _icons[ICID_NONE];
}

const Icon& IconCache::extract(LPCTSTR path, int icon_idx, ICONCACHE_FLAGS flags)
{
	IdxCacheKey key(path, make_pair(icon_idx, (flags|ICF_HICON)&~ICF_SYSCACHE));

	key.first.toLower();

	IdxCacheMap::iterator found = _idxCache.find(key);

	if (found != _idxCache.end())
		return _icons[found->second];

	HICON hIcon;

	if ((int)ExtractIconEx(path, icon_idx, NULL, &hIcon, 1) > 0) {
		const Icon& icon = add(hIcon, IT_CACHED);

		_idxCache[key] = icon;

		return icon;
	} else {

		///@todo retreive "http://.../favicon.ico" format icons

		return _icons[ICID_NONE];
	}
}

const Icon& IconCache::extract(IExtractIcon* pExtract, LPCTSTR path, int icon_idx, ICONCACHE_FLAGS flags)
{
	HICON hIconLarge = 0;
	HICON hIcon;

	int icon_size = ICON_SIZE_FROM_ICF(flags);
	HRESULT hr = pExtract->Extract(path, icon_idx, &hIconLarge, &hIcon, MAKELONG(GetSystemMetrics(SM_CXICON), icon_size));

	if (hr == NOERROR) {	//@@ oder SUCCEEDED(hr) ?
		if (icon_size > ICON_SIZE_SMALL) {	//@@ OK?
			if (hIcon)
				DestroyIcon(hIcon);

			hIcon = hIconLarge;
		} else {
			if (hIconLarge)
				DestroyIcon(hIconLarge);
		}

		if (hIcon)
			return add(hIcon);	//@@ When do we want not to free this icons?
	}

	return _icons[ICID_NONE];
}

const Icon&	IconCache::extract(LPCITEMIDLIST pidl, ICONCACHE_FLAGS flags)
{
	 // search for matching icon with unchanged flags in the cache
	PidlCacheKey mapkey(pidl, flags);
	PidlCacheMap::iterator found = _pidlcache.find(mapkey);

	if (found != _pidlcache.end())
		return _icons[found->second];

	 // search for matching icon with handle
	PidlCacheKey mapkey_hicon(pidl, flags|ICF_HICON);
	if (flags != mapkey_hicon.second) {
		found = _pidlcache.find(mapkey_hicon);

		if (found != _pidlcache.end())
			return _icons[found->second];
	}

	 // search for matching icon in the system image list cache
	PidlCacheKey mapkey_syscache(pidl, flags|ICF_SYSCACHE);
	if (flags != mapkey_syscache.second) {
		found = _pidlcache.find(mapkey_syscache);

		if (found != _pidlcache.end())
			return _icons[found->second];
	}

	SHFILEINFO sfi;

	int shgfi_flags = SHGFI_PIDL;

	if (!(flags & (ICF_LARGE|ICF_MIDDLE)))
		shgfi_flags |= SHGFI_SMALLICON;

	if (flags & ICF_OPEN)
		shgfi_flags |= SHGFI_OPENICON;

	if (flags & ICF_SYSCACHE) {
		assert(!(flags&ICF_OVERLAYS));

		HIMAGELIST himlSys = (HIMAGELIST) SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX|shgfi_flags);
		if (himlSys) {
			const Icon& icon = add(sfi.iIcon/*, IT_SYSCACHE*/);

			///@todo limit cache size
			_pidlcache[mapkey_syscache] = icon;

			return icon;
		}
	} else {
		if (flags & ICF_OVERLAYS)
			shgfi_flags |= SHGFI_ADDOVERLAYS;

		if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON|shgfi_flags)) {
			const Icon& icon = add(sfi.hIcon, IT_CACHED);

			///@todo limit cache size
			_pidlcache[mapkey_hicon] = icon;

			return icon;
		}
	}

	return _icons[ICID_NONE];
}


const Icon& IconCache::add(HICON hIcon, ICON_TYPE type)
{
	int id = ++s_next_id;

	return _icons[id] = Icon(type, id, hIcon);
}

const Icon&	IconCache::add(int sys_idx/*, ICON_TYPE type=IT_SYSCACHE*/)
{
	int id = ++s_next_id;

	return _icons[id] = SysCacheIcon(id, sys_idx);
}

const Icon& IconCache::get_icon(int id)
{
	return _icons[id];
}

void IconCache::free_icon(int icon_id)
{
	IconMap::iterator found = _icons.find(icon_id);

	if (found != _icons.end()) {
		Icon& icon = found->second;

		if (icon.destroy())
			_icons.erase(found);
	}
}


ResString::ResString(UINT nid)
{
	TCHAR buffer[BUFFER_LEN];

	int len = LoadString(g_Globals._hInstance, nid, buffer, sizeof(buffer)/sizeof(TCHAR));

	super::assign(buffer, len);
}


ResIcon::ResIcon(UINT nid)
{
	_hicon = LoadIcon(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}

SmallIcon::SmallIcon(UINT nid)
{
	_hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

ResIconEx::ResIconEx(UINT nid, int w, int h)
{
	_hicon = (HICON)LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, w, h, LR_SHARED);
}


void SetWindowIcon(HWND hwnd, UINT nid)
{
	HICON hIcon = ResIcon(nid);
	(void)Window_SetIcon(hwnd, ICON_BIG, hIcon);

	HICON hIconSmall = SmallIcon(nid);
	(void)Window_SetIcon(hwnd, ICON_SMALL, hIconSmall);
}


ResBitmap::ResBitmap(UINT nid)
{
	_hBmp = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}


#ifndef ROSSHELL

void explorer_show_frame(int cmdShow, LPTSTR lpCmdLine)
{
	ExplorerCmd cmd;

	if (g_Globals._hMainWnd) {
		if (IsIconic(g_Globals._hMainWnd))
			ShowWindow(g_Globals._hMainWnd, SW_RESTORE);
		else
			SetForegroundWindow(g_Globals._hMainWnd);

		return;
	}

	g_Globals._prescan_nodes = false;

	XMLPos explorer_options = g_Globals.get_cfg("general/explorer");
	XS_String mdiStr = XMLString(explorer_options, "mdi");

	 // If there isn't yet the "mdi" setting in the configuration, display MDI/SDI dialog.
	if (mdiStr.empty())
		Dialog::DoModal(IDD_MDI_SDI, WINDOW_CREATOR(MdiSdiDlg), g_Globals._hwndDesktop);

	 // Now read the MDI attribute again and interpret it as boolean value.
	cmd._mdi = XMLBool(explorer_options, "mdi", true);

	cmd._cmdShow = cmdShow;

	 // parse command line options, which may overwrite the MDI flag
	if (lpCmdLine)
		cmd.ParseCmdLine(lpCmdLine);

	 // create main window
	MainFrameBase::Create(cmd);
}

bool ExplorerCmd::ParseCmdLine(LPCTSTR lpCmdLine)
{
	bool ok = true;

	LPCTSTR b = lpCmdLine;
	LPCTSTR p = b;

	while(*b) {
		 // remove leading space
		while(_istspace((unsigned)*b))
			++b;

		p = b;

		bool quote = false;

		 // options are separated by ','
		for(; *p; ++p) {
			if (*p == '"')	// Quote characters may appear at any position in the command line.
				quote = !quote;
			else if (*p==',' && !quote)
				break;
		}

		if (p > b) {
			int l = p - b;

			 // remove trailing space
			while(l>0 && _istspace((unsigned)b[l-1]))
				--l;

			if (!EvaluateOption(String(b, l)))
				ok = false;

			if (*p)
				++p;

			b = p;
		}
	}

	return ok;
}

bool ExplorerCmd::EvaluateOption(LPCTSTR option)
{
	String opt_str;

	 // Remove quote characters, as they are evaluated at this point.
	for(; *option; ++option)
		if (*option != '"')
			opt_str += *option;

	option = opt_str;

	if (option[0] == '/') {
		++option;

		 // option /e for windows in explorer mode
		if (!_tcsicmp(option, TEXT("e")))
			_flags |= OWM_EXPLORE;
		 // option /root for rooted explorer windows
		else if (!_tcsicmp(option, TEXT("root")))
			_flags |= OWM_ROOTED;
		 // non-standard options: /mdi, /sdi
		else if (!_tcsicmp(option, TEXT("mdi")))
			_mdi = true;
		else if (!_tcsicmp(option, TEXT("sdi")))
			_mdi = false;
		else
			return false;
	} else {
		if (!_path.empty())
			return false;

		_path = opt_str;
	}

	return true;
}

bool ExplorerCmd::IsValidPath() const
{
	if (!_path.empty()) {
		DWORD attribs = GetFileAttributes(_path);

		if (attribs!=INVALID_FILE_ATTRIBUTES && (attribs&FILE_ATTRIBUTE_DIRECTORY))
			return true;	// file system path
		else if (*_path==':' && _path.at(1)==':')
			return true;	// text encoded IDL
	}

	return false;
}

#else

void explorer_show_frame(int cmdShow, LPTSTR lpCmdLine)
{
	if (!lpCmdLine)
		lpCmdLine = TEXT("explorer.exe");

	launch_file(GetDesktopWindow(), lpCmdLine, cmdShow);
}

#endif


PopupMenu::PopupMenu(UINT nid)
{
	HMENU hMenu = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(nid));
	_hmenu = GetSubMenu(hMenu, 0);
}


 /// "About Explorer" Dialog
struct ExplorerAboutDlg : public
			CtlColorParent<
				OwnerDrawParent<Dialog>
			>
{
	typedef CtlColorParent<
				OwnerDrawParent<Dialog>
			> super;

	ExplorerAboutDlg(HWND hwnd)
	 :	super(hwnd)
	{
		SetWindowIcon(hwnd, IDI_REACTOS);

		new FlatButton(hwnd, IDOK);

		_hfont = CreateFont(20, 0, 0, 0, FW_BOLD, TRUE, 0, 0, 0, 0, 0, 0, 0, TEXT("Sans Serif"));
		new ColorStatic(hwnd, IDC_ROS_EXPLORER, RGB(32,32,128), 0, _hfont);

		new HyperlinkCtrl(hwnd, IDC_WWW);

		FmtString ver_txt(ResString(IDS_EXPLORER_VERSION_STR), (LPCTSTR)ResString(IDS_VERSION_STR));
		SetWindowText(GetDlgItem(hwnd, IDC_VERSION_TXT), ver_txt);

		HWND hwnd_winver = GetDlgItem(hwnd, IDC_WIN_VERSION);
		SetWindowText(hwnd_winver, get_windows_version_str());
		SetWindowFont(hwnd_winver, GetStockFont(DEFAULT_GUI_FONT), FALSE);

		CenterWindow(hwnd);
	}

	~ExplorerAboutDlg()
	{
		DeleteObject(_hfont);
	}

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case WM_PAINT:
			Paint();
			break;

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}

		return 0;
	}

	void Paint()
	{
		PaintCanvas canvas(_hwnd);

		HICON hicon = (HICON) LoadImage(g_Globals._hInstance, MAKEINTRESOURCE(IDI_REACTOS_BIG), IMAGE_ICON, 0, 0, LR_SHARED);

		DrawIconEx(canvas, 20, 10, hicon, 0, 0, 0, 0, DI_NORMAL);
	}

protected:
	HFONT	_hfont;
};

void explorer_about(HWND hwndParent)
{
	Dialog::DoModal(IDD_ABOUT_EXPLORER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}


static void InitInstance(HINSTANCE hInstance)
{
	CONTEXT("InitInstance");

	setlocale(LC_COLLATE, "");	// set collating rules to local settings for compareName

#ifndef ROSSHELL
	 // register frame window class
	g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME,IDI_EXPLORER);

	 // register child window class
	WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	 // register tree window class
	WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();
#endif

	g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hInstance, LPTSTR lpCmdLine, int cmdShow)
{
	CONTEXT("explorer_main");

	 // initialize Common Controls library
	CommonControlInit usingCmnCtrl;

	try {
		InitInstance(hInstance);
	} catch(COMException& e) {
		HandleException(e, GetDesktopWindow());
		return -1;
	}

#ifndef ROSSHELL
	if (cmdShow != SW_HIDE) {
/*	// don't maximize if being called from the ROS desktop
		if (cmdShow == SW_SHOWNORMAL)
				///@todo read window placement from registry
			cmdShow = SW_MAXIMIZE;
*/

		explorer_show_frame(cmdShow, lpCmdLine);
	}
#endif

	return Window::MessageLoop();
}


 // MinGW does not provide a Unicode startup routine, so we have to implement an own.
#if defined(__MINGW32__) && defined(UNICODE)

#define _tWinMain wWinMain
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);

int main(int argc, char* argv[])
{
	CONTEXT("main");

	STARTUPINFO startupinfo;
	int nShowCmd = SW_SHOWNORMAL;

	GetStartupInfo(&startupinfo);

	if (startupinfo.dwFlags & STARTF_USESHOWWINDOW)
		nShowCmd = startupinfo.wShowWindow;

	LPWSTR cmdline = GetCommandLineW();

	while(*cmdline && !_istspace((unsigned)*cmdline))
		++cmdline;

	while(_istspace((unsigned)*cmdline))
		++cmdline;

	return wWinMain(GetModuleHandle(NULL), 0, cmdline, nShowCmd);
}

#endif	// __MINGW && UNICODE


static bool SetShellReadyEvent(LPCTSTR evtName)
{
	HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, evtName);
	if (!hEvent)
		return false;

	SetEvent(hEvent);
	CloseHandle(hEvent);

	return true;
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	CONTEXT("WinMain()");

	BOOL any_desktop_running = IsAnyDesktopRunning();

	BOOL startup_desktop;

	 // strip extended options from the front of the command line
	String ext_options;

	while(*lpCmdLine == '-') {
		while(*lpCmdLine && !_istspace((unsigned)*lpCmdLine))
			ext_options += *lpCmdLine++;

		while(_istspace((unsigned)*lpCmdLine))
			++lpCmdLine;
	}

	 // command line option "-install" to replace previous shell application with ROS Explorer
	if (_tcsstr(ext_options,TEXT("-install"))) {
		 // install ROS Explorer into the registry
		TCHAR path[MAX_PATH];

		int l = GetModuleFileName(0, path, COUNTOF(path));
		if (l) {
			HKEY hkey;

			if (!RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

				///@todo save previous shell application in config file

				RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)path, l*sizeof(TCHAR));
				RegCloseKey(hkey);
			}

			if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hkey)) {

				///@todo save previous shell application in config file

				RegSetValueEx(hkey, TEXT("Shell"), 0, REG_SZ, (LPBYTE)TEXT(""), l*sizeof(TCHAR));
				RegCloseKey(hkey);
			}
		}

		HWND shellWindow = GetShellWindow();

		if (shellWindow) {
			DWORD pid;

			 // terminate shell process for NT like systems
			GetWindowThreadProcessId(shellWindow, &pid);
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

			 // On Win 9x it's sufficient to destroy the shell window.
			DestroyWindow(shellWindow);

			if (TerminateProcess(hProcess, 0))
				WaitForSingleObject(hProcess, INFINITE);

			CloseHandle(hProcess);
		}

		startup_desktop = TRUE;
	} else {
		 // create desktop window and task bar only, if there is no other shell and we are
		 // the first explorer instance
		 // MS Explorer looks additionally into the registry entry HKCU\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\shell,
		 // to decide wether it is currently configured as shell application.
		startup_desktop = !any_desktop_running;
	}


	bool autostart = !any_desktop_running;

	 // disable autostart if the SHIFT key is pressed
	if (GetAsyncKeyState(VK_SHIFT) < 0)
		autostart = false;

#ifdef _DEBUG	//MF: disabled for debugging
	autostart = false;
#endif

	 // If there is given the command line option "-desktop", create desktop window anyways
	if (_tcsstr(ext_options,TEXT("-desktop")))
		startup_desktop = TRUE;
#ifndef ROSSHELL
	else if (_tcsstr(ext_options,TEXT("-nodesktop")))
		startup_desktop = FALSE;

	 // Don't display cabinet window in desktop mode
	if (startup_desktop && !_tcsstr(ext_options,TEXT("-explorer")))
		nShowCmd = SW_HIDE;
#endif

	if (_tcsstr(ext_options,TEXT("-noautostart")))
		autostart = false;
	else if (_tcsstr(ext_options,TEXT("-autostart")))
		autostart = true;

#ifndef __WINE__
	if (_tcsstr(ext_options,TEXT("-console"))) {
		AllocConsole();

		_dup2(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY), 0);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0), 1);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), 0), 2);

		g_Globals._log = _fdopen(1, "w");
		setvbuf(g_Globals._log, 0, _IONBF, 0);

		LOG(TEXT("starting explorer debug log\n"));
	}
#endif


	if (startup_desktop) {
		 // hide the XP login screen (Credit to Nicolas Escuder)
		 // another undocumented event: "Global\\msgina: ReturnToWelcome"
		if (!SetShellReadyEvent(TEXT("msgina: ShellReadyEvent")))
			SetShellReadyEvent(TEXT("Global\\msgina: ShellReadyEvent"));
	}
#ifdef ROSSHELL
	else
		return 0;	// no shell to launch, so exit immediatelly
#endif


	if (!any_desktop_running) {
		 // launch the shell DDE server
		if (g_SHDOCVW_ShellDDEInit)
			(*g_SHDOCVW_ShellDDEInit)(TRUE);
	}


	bool use_gdb_stub = false;	// !IsDebuggerPresent();

	if (_tcsstr(ext_options,TEXT("-debug")))
		use_gdb_stub = true;

	if (_tcsstr(ext_options,TEXT("-break"))) {
		LOG(TEXT("debugger breakpoint"));
#ifdef _MSC_VER
		__asm int 3
#else
		asm("int3");
#endif
	}

	 // activate GDB remote debugging stub if no other debugger is running
	if (use_gdb_stub) {
		LOG(TEXT("waiting for debugger connection...\n"));

		initialize_gdb_stub();
	}

	g_Globals.init(hInstance);

	 // initialize COM and OLE before creating the desktop window
	OleInit usingCOM;

	 // init common controls library
	CommonControlInit usingCmnCtrl;

	g_Globals.read_persistent();

	if (startup_desktop) {
		WaitCursor wait;

		g_Globals._desktops.init();

		g_Globals._hwndDesktop = DesktopWindow::Create();
#ifdef _USE_HDESK
		g_Globals._desktops.get_current_Desktop()->_hwndDesktop = g_Globals._hwndDesktop;
#endif
	}

	Thread* pSSOThread = NULL;

	if (startup_desktop) {
		 // launch SSO thread to allow message processing independent from the explorer main thread
		pSSOThread = new SSOThread;
		pSSOThread->Start();
	}

	/**TODO launching autostart programs can be moved into a background thread. */
	if (autostart) {
		const char* argv[] = {"", "s"};	// call startup routine in SESSION_START mode
		startup(2, argv);
	}

#ifndef ROSSHELL
	if (g_Globals._hwndDesktop)
		g_Globals._desktop_mode = true;
#endif


	int ret = explorer_main(hInstance, lpCmdLine, nShowCmd);


	 // write configuration file
	g_Globals.write_persistent();

	if (pSSOThread) {
		pSSOThread->Stop();
		delete pSSOThread;
	}

	if (!any_desktop_running) {
		 // shutdown the shell DDE server
		if (g_SHDOCVW_ShellDDEInit)
			(*g_SHDOCVW_ShellDDEInit)(FALSE);
	}

	return ret;
}
