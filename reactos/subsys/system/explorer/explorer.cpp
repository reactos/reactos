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
 // explorer.cpp
 //
 // Martin Fuchs, 23.07.2003
 //
 // Credits: Thanks to Leon Finker for his explorer window example
 //


#include "utility/utility.h"

#include "explorer.h"
#include "desktop/desktop.h"

#include "globals.h"
#include "externals.h"

#include "explorer_intres.h"

#include <locale.h>	// for setlocale()

#ifndef __WINE__
#include <io.h>		// for dup2()
#include <fcntl.h>	// for _O_RDONLY
#endif


extern "C" int initialize_gdb_stub();	// start up GDB stub


ExplorerGlobals g_Globals;


ExplorerGlobals::ExplorerGlobals()
{
	_hInstance = 0;
	_hframeClass = 0;
	_cfStrFName = 0;
	_hMainWnd = 0;
	_prescan_nodes = false;
	_desktop_mode = false;
	_log = NULL;
#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	_SHRestricted = 0;
#endif
	_hwndDesktopBar = 0;
	_hwndShellView = 0;
}


void ExplorerGlobals::init(HINSTANCE hInstance)
{
	_hInstance = hInstance;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	_SHRestricted = (DWORD(STDAPICALLTYPE*)(RESTRICTIONS)) GetProcAddress(GetModuleHandle(TEXT("SHELL32")), "SHRestricted");
#endif

	_icon_cache.init();
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
#ifndef __WINE__ ///@todo
	_tcslwr((LPTSTR)ext.c_str());
#endif

	iterator found = find(ext);
	if (found != end())
		return found->second;

	FileTypeInfo& ftype = super::operator[](ext);

	ftype._neverShowExt = false;

	HKEY hkey;
	TCHAR value[MAX_PATH], display_name[MAX_PATH];;
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
			_tcsncpy(entry->_display_name, entry->_data.cFileName, len);
			entry->_display_name[len] = TEXT('\0');
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

HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
	HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, 16, 16);

	MemCanvas canvas;
	BitmapSelection sel(canvas, hbmp);

	RECT rect = {0, 0, 16, 16};
	FillRect(canvas, &rect, hbrush_bkgnd);

	DrawIconEx(canvas, 0, 0, hIcon, 16, 16, 0, hbrush_bkgnd, DI_NORMAL);

	return hbmp;
}


int IconCache::s_next_id = ICID_DYNAMIC;


void IconCache::init()
{
	_icons[ICID_NONE]		= Icon(IT_STATIC, ICID_NONE, (HICON)0);

	_icons[ICID_FOLDER]		= Icon(ICID_FOLDER,		IDI_FOLDER);
	//_icons[ICID_DOCUMENT] = Icon(ICID_DOCUMENT,	IDI_DOCUMENT);
	_icons[ICID_EXPLORER]	= Icon(ICID_EXPLORER,	IDI_EXPLORER);
	_icons[ICID_APP]		= Icon(ICID_APP,		IDI_APPICON);

	_icons[ICID_CONFIG]		= Icon(ICID_CONFIG,		IDI_CONFIG);
	_icons[ICID_DOCUMENTS]	= Icon(ICID_DOCUMENTS,	IDI_DOCUMENTS);
	_icons[ICID_FAVORITES]	= Icon(ICID_FAVORITES,	IDI_FAVORITES);
	_icons[ICID_INFO]		= Icon(ICID_INFO,		IDI_INFO);
	_icons[ICID_APPS]		= Icon(ICID_APPS,		IDI_APPS);
	_icons[ICID_SEARCH]		= Icon(ICID_SEARCH,		IDI_SEARCH);
	_icons[ICID_ACTION]		= Icon(ICID_ACTION,		IDI_ACTION);
	_icons[ICID_SEARCH_DOC] = Icon(ICID_SEARCH_DOC,	IDI_SEARCH_DOC);
	_icons[ICID_PRINTER]	= Icon(ICID_PRINTER,	IDI_PRINTER);
	_icons[ICID_NETWORK]	= Icon(ICID_NETWORK,	IDI_NETWORK);
	_icons[ICID_COMPUTER]	= Icon(ICID_COMPUTER,	IDI_COMPUTER);
	_icons[ICID_LOGOFF]		= Icon(ICID_LOGOFF,		IDI_LOGOFF);
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

#ifndef __WINE__ ///@todo
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
	} else
		return _icons[ICID_NONE];
}

const Icon& IconCache::extract(IExtractIcon* pExtract, LPCTSTR path, int idx)
{
	HICON hIconLarge = 0;
	HICON hIcon;

	HRESULT hr = pExtract->Extract(path, idx, &hIconLarge, &hIcon, MAKELONG(0/*GetSystemMetrics(SM_CXICON)*/,GetSystemMetrics(SM_CXSMICON)));

	if (hr == NOERROR) {
		if (hIconLarge)
			DestroyIcon(hIconLarge);

		if (hIcon)
			return add(hIcon);
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
	Window_SetIcon(hwnd, ICON_BIG, hIcon);

	HICON hIconSmall = SmallIcon(nid);
	Window_SetIcon(hwnd, ICON_SMALL, hIconSmall);
}


ResBitmap::ResBitmap(UINT nid)
{
	_hBmp = LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(nid));
}


void explorer_show_frame(int cmdshow, LPTSTR lpCmdLine)
{
	if (g_Globals._hMainWnd) {
		if (IsIconic(g_Globals._hMainWnd))
			ShowWindow(g_Globals._hMainWnd, SW_RESTORE);
		else
			SetForegroundWindow(g_Globals._hMainWnd);

		return;
	}

	g_Globals._prescan_nodes = false;

	 // create main window
	HWND hMainFrame = MainFrame::Create();

	if (hMainFrame) {
		g_Globals._hMainWnd = hMainFrame;

		ShowWindow(hMainFrame, cmdshow);
		UpdateWindow(hMainFrame);

		bool valid_dir = false;

		if (lpCmdLine) {
			DWORD attribs = GetFileAttributes(lpCmdLine);

			if (attribs!=INVALID_FILE_ATTRIBUTES && (attribs&FILE_ATTRIBUTE_DIRECTORY))
				valid_dir = true;
			else if (*lpCmdLine==':' || *lpCmdLine=='"')
				valid_dir = true;
		}

		 // Open the first child window after initializing the application
		if (valid_dir)
			PostMessage(hMainFrame, PM_OPEN_WINDOW, 0, (LPARAM)lpCmdLine);
		else
			PostMessage(hMainFrame, PM_OPEN_WINDOW, OWM_EXPLORE|OWM_DETAILS, 0);
	}
}


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

	 // register frame window class
	g_Globals._hframeClass = IconWindowClass(CLASSNAME_FRAME,IDI_EXPLORER);

	 // register child windows class
	WindowClass(CLASSNAME_CHILDWND, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	 // register tree windows class
	WindowClass(CLASSNAME_WINEFILETREE, CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW).Register();

	g_Globals._cfStrFName = RegisterClipboardFormat(CFSTR_FILENAME);
}


int explorer_main(HINSTANCE hInstance, LPTSTR lpCmdLine, int cmdshow)
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

	if (cmdshow != SW_HIDE) {
/*	// don't maximize if being called from the ROS desktop
		if (cmdshow == SW_SHOWNORMAL)
				///@todo read window placement from registry
			cmdshow = SW_MAXIMIZE;
*/

		explorer_show_frame(cmdshow, lpCmdLine);
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

	 // create desktop window and task bar only, if there is no other shell and we are
	 // the first explorer instance
	BOOL startup_desktop = !IsAnyDesktopRunning();

	bool autostart = true;

#ifdef _DEBUG	//MF: disabled for debugging
	autostart = false;
#endif

	 // If there is given the command line option "-desktop", create desktop window anyways
	if (_tcsstr(lpCmdLine,TEXT("-desktop")))
		startup_desktop = TRUE;
	else if (_tcsstr(lpCmdLine,TEXT("-nodesktop")))
		startup_desktop = FALSE;

	 // Don't display cabinet window in desktop mode
	if (startup_desktop && !_tcsstr(lpCmdLine,TEXT("-explorer")))
		nShowCmd = SW_HIDE;

	if (_tcsstr(lpCmdLine,TEXT("-noautostart")))
		autostart = false;

#ifndef __WINE__
	if (_tcsstr(lpCmdLine,TEXT("-console"))) {
		AllocConsole();

		_dup2(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY), 0);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0), 1);
		_dup2(_open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), 0), 2);

		g_Globals._log = fdopen(1, "w");
		setvbuf(g_Globals._log, 0, _IONBF, 0);

		LOG(TEXT("starting explorer debug log\n"));
	}
#endif

	bool use_gdb_stub = false;	// !IsDebuggerPresent();

	if (_tcsstr(lpCmdLine,TEXT("-debug")))
		use_gdb_stub = true;

	if (_tcsstr(lpCmdLine,TEXT("-break"))) {
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

	HWND hwndDesktop = 0;

	if (startup_desktop) {
		g_Globals._desktops.init();

		hwndDesktop = DesktopWindow::Create();
#ifdef _USE_HDESK
		g_Globals._desktops.get_current_Desktop()->_hwndDesktop = hwndDesktop;
#endif

		if (autostart) {
			char* argv[] = {"", "s"};	// call startup routine in SESSION_START mode
			startup(2, argv);
		}
	}

	/**TODO fix command line handling */
	if (*lpCmdLine=='"' && lpCmdLine[_tcslen(lpCmdLine)-1]=='"') {
		++lpCmdLine;
		lpCmdLine[_tcslen(lpCmdLine)-1] = '\0';
	}

	if (hwndDesktop)
		g_Globals._desktop_mode = true;

	int ret = explorer_main(hInstance, lpCmdLine, nShowCmd);

	return ret;
}
