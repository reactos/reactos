/*
 * Copyright 2005 Martin Fuchs
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
 // ROS Internet Web Browser
 //
 // ibrowser.cpp
 //
 // Martin Fuchs, 24.01.2005
 //


#include "precomp.h"

#include "ibrowser_intres.h"

#include <locale.h>	// for setlocale()

#ifndef __WINE__
#include <io.h>		// for dup2()
#include <fcntl.h>	// for _O_RDONLY
#endif


 // globals

HINSTANCE g_hInstance;
IconCache g_icon_cache;
ATOM g_hframeClass;


/*@@
void ExplorerGlobals::read_persistent()
{
	 // read configuration file
	_cfg_dir.printf(TEXT("%s\\ReactOS"), (LPCTSTR)SpecialFolderFSPath(CSIDL_APPDATA,0));
	_cfg_path.printf(TEXT("%s\\ros-ibrowser-cfg.xml"), _cfg_dir.c_str());

	if (!_cfg.read(_cfg_path)) {
		if (_cfg._last_error != XML_ERROR_NO_ELEMENTS)
			MessageBox(g_Globals._hwndDesktop, String(_cfg._last_error_msg.c_str()),
						TEXT("ROS Explorer - reading user settings"), MB_OK);

		_cfg.read(TEXT("ibrowser-cfg-template.xml"));
	}

	 // read bookmarks
	_favorites_path.printf(TEXT("%s\\ros-ibrowser-bookmarks.xml"), _cfg_dir.c_str());

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

	cfg_pos.smart_create("ibrowser-cfg");

	return cfg_pos;
}

XMLPos ExplorerGlobals::get_cfg(const char* path)
{
	XMLPos cfg_pos(&_cfg);

	cfg_pos.smart_create("ibrowser-cfg");
	cfg_pos.create_relative(path);

	return cfg_pos;
}
*/


Icon::Icon()
 :	_id(ICID_UNKNOWN),
	_itype(IT_STATIC),
	_hicon(0)
{
}

Icon::Icon(ICON_ID id, UINT nid)
 :	_id(id),
	_itype(IT_STATIC),
	_hicon(SmallIcon(nid))
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
		ImageList_DrawEx(g_icon_cache.get_sys_imagelist(), _sys_idx, hdc, x, y, cx, cy, bk_color, CLR_DEFAULT, ILD_NORMAL);
	else
		DrawIconEx(hdc, x, y, _hicon, cx, cy, 0, bk_brush, DI_NORMAL);
}

HBITMAP	Icon::create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd) const
{
	if (_itype == IT_SYSCACHE) {
		HIMAGELIST himl = g_icon_cache.get_sys_imagelist();

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
		HIMAGELIST himl = g_icon_cache.get_sys_imagelist();

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

HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
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
	_icons[ICID_NONE]		= Icon(IT_STATIC, ICID_NONE, (HICON)0);

	_icons[ICID_IBROWSER]	= Icon(ICID_IBROWSER,	IDI_IBROWSER);
	_icons[ICID_BOOKMARK]	= Icon(ICID_BOOKMARK,	IDI_DOT_TRANS);
}


const Icon& IconCache::extract(const String& path)
{
	PathMap::iterator found = _pathMap.find(path);

	if (found != _pathMap.end())
		return _icons[found->second];

	SHFILEINFO sfi;

#if 1	// use system image list
	HIMAGELIST himlSys = (HIMAGELIST) SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX|SHGFI_SMALLICON);

	if (himlSys) {
		_himlSys = himlSys;

		const Icon& icon = add(sfi.iIcon/*, IT_SYSCACHE*/);
#else
	if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON)) {
		const Icon& icon = add(sfi.hIcon, IT_CACHED);
#endif

		///@todo limit cache size
		_pathMap[path] = icon;

		return icon;
	} else
		return _icons[ICID_NONE];
}

const Icon& IconCache::extract(LPCTSTR path, int idx)
{
	CachePair key(path, idx);

#ifndef __WINE__ ///@todo _tcslwr() for Wine
	_tcslwr((LPTSTR)key.first.c_str());
#endif

	PathIdxMap::iterator found = _pathIdxMap.find(key);

	if (found != _pathIdxMap.end())
		return _icons[found->second];

	HICON hIcon;

	if ((int)ExtractIconEx(path, idx, NULL, &hIcon, 1) > 0) {
		const Icon& icon = add(hIcon, IT_CACHED);

		_pathIdxMap[key] = icon;

		return icon;
	} else {

		///@todo retreive "http://.../favicon.ico" format icons

		return _icons[ICID_NONE];
	}
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

	int len = LoadString(g_hInstance, nid, buffer, sizeof(buffer)/sizeof(TCHAR));

	super::assign(buffer, len);
}


ResIcon::ResIcon(UINT nid)
{
	_hicon = LoadIcon(g_hInstance, MAKEINTRESOURCE(nid));
}

SmallIcon::SmallIcon(UINT nid)
{
	_hicon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
}

ResIconEx::ResIconEx(UINT nid, int w, int h)
{
	_hicon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(nid), IMAGE_ICON, w, h, LR_SHARED);
}


void SetWindowIcon(HWND hwnd, UINT nid)
{
	HICON hIcon = ResIcon(nid);
	Window_SetIcon(hwnd, ICON_BIG, hIcon);

	HICON hIconSmall = SmallIcon(nid);
	Window_SetIcon(hwnd, ICON_SMALL, hIconSmall);
}


ResBitmap::ResBitmap(UINT nid)
{
	_hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(nid));
}


void ibrowser_show_frame(int cmdshow, LPTSTR lpCmdLine)
{
	MainFrameBase::Create(lpCmdLine, cmdshow);
}


PopupMenu::PopupMenu(UINT nid)
{
	HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(nid));
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
		new ColorStatic(hwnd, IDC_ROS_IBROWSER, RGB(32,32,128), 0, _hfont);

		new HyperlinkCtrl(hwnd, IDC_WWW);

		FmtString ver_txt(ResString(IDS_IBROWSER_VERSION_STR), (LPCTSTR)ResString(IDS_VERSION_STR));
		SetWindowText(GetDlgItem(hwnd, IDC_VERSION_TXT), ver_txt);

		/*@@
		HWND hwnd_winver = GetDlgItem(hwnd, IDC_WIN_VERSION);
		SetWindowText(hwnd_winver, get_windows_version_str());
		SetWindowFont(hwnd_winver, GetStockFont(DEFAULT_GUI_FONT), FALSE);
		*/

		CenterWindow(hwnd);
	}

	~ExplorerAboutDlg()
	{
		DeleteObject(_hfont);
	}

protected:
	HFONT	_hfont;
};

void ibrowser_about(HWND hwndParent)
{
	Dialog::DoModal(IDD_ABOUT_IBROWSER, WINDOW_CREATOR(ExplorerAboutDlg), hwndParent);
}


static void InitInstance(HINSTANCE hInstance)
{
	CONTEXT("InitInstance");

	 // register frame window class
	g_hframeClass = IconWindowClass(CLASSNAME_FRAME,IDI_IBROWSER);

	 // register child window class
	WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC|CS_VREDRAW).Register();
}


int ibrowser_main(HINSTANCE hInstance, LPTSTR lpCmdLine, int cmdshow)
{
	CONTEXT("ibrowser_main");

	 // initialize Common Controls library
	CommonControlInit usingCmnCtrl;

	try {
		InitInstance(hInstance);
	} catch(COMException& e) {
		HandleException(e, GetDesktopWindow());
		return -1;
	}

	if (cmdshow != SW_HIDE) {
/*	// don't maximize if being called from the ROS desktop
		if (cmdshow == SW_SHOWNORMAL)
				///@todo read window placement from registry
			cmdshow = SW_MAXIMIZE;
*/

		ibrowser_show_frame(cmdshow, lpCmdLine);
	}

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

	while(*cmdline && !_istspace(*cmdline))
		++cmdline;

	return wWinMain(GetModuleHandle(NULL), 0, cmdline, nShowCmd);
}

#endif	// __MINGW && UNICODE


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	CONTEXT("WinMain()");

	g_hInstance = hInstance;

	 // initialize COM and OLE before creating the desktop window
	OleInit usingCOM;

	 // init common controls library
	CommonControlInit usingCmnCtrl;

//@@	g_Globals.read_persistent();

	/**TODO fix command line handling */
	if (*lpCmdLine=='"' && lpCmdLine[_tcslen(lpCmdLine)-1]=='"') {
		++lpCmdLine;
		lpCmdLine[_tcslen(lpCmdLine)-1] = '\0';
	}

	int ret = ibrowser_main(hInstance, lpCmdLine, nShowCmd);

	 // write configuration file
//@@	g_Globals.write_persistent();

	return ret;
}
