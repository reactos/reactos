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
 // ibrowser.h
 //
 // Martin Fuchs, 25.01.2005
 //


#include "utility/window.h"


#define	IDW_STATUSBAR			0x100
#define	IDW_TOOLBAR				0x101
#define	IDW_EXTRABAR			0x102
#define	IDW_ADDRESSBAR			0x104
#define	IDW_SIDEBAR				0x106
#define	IDW_FIRST_CHILD			0xC000	/*0x200*/


#define	PM_GET_FILEWND_PTR		(WM_APP+0x05)
#define	PM_GET_SHELLBROWSER_PTR	(WM_APP+0x06)

#define	PM_GET_CONTROLWINDOW	(WM_APP+0x16)

#define	PM_RESIZE_CHILDREN		(WM_APP+0x17)
#define	PM_GET_WIDTH			(WM_APP+0x18)

#define	PM_REFRESH				(WM_APP+0x1B)
#define	PM_REFRESH_CONFIG		(WM_APP+0x1C)


#define	CLASSNAME_FRAME 		TEXT("IBrowserFrameWClass")

#define	CLASSNAME_CHILDWND		TEXT("IBrowserChildWClass")


#include "mainframe.h"


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


enum ICON_TYPE {
	IT_STATIC,
	IT_CACHED,
	IT_DYNAMIC,
	IT_SYSCACHE
};

enum ICON_ID {
	ICID_UNKNOWN,
	ICID_NONE,

	ICID_IBROWSER,
	ICID_BOOKMARK,

	ICID_DYNAMIC
};

struct Icon {
	Icon();
	Icon(ICON_ID id, UINT nid);
	Icon(ICON_TYPE itype, int id, HICON hIcon);
	Icon(ICON_TYPE itype, int id, int sys_idx);

	operator ICON_ID() const {return _id;}

	void	draw(HDC hdc, int x, int y, int cx, int cy, COLORREF bk_color, HBRUSH bk_brush) const;
	HBITMAP	create_bitmap(COLORREF bk_color, HBRUSH hbrBkgnd, HDC hdc_wnd) const;
	int		add_to_imagelist(HIMAGELIST himl, HDC hdc_wnd, COLORREF bk_color=GetSysColor(COLOR_WINDOW), HBRUSH bk_brush=GetSysColorBrush(COLOR_WINDOW)) const;

	int		get_sysiml_idx() const {return _itype==IT_SYSCACHE? _sys_idx: -1;}

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
	IconCache() : _himlSys(0) {}

	void	init();

	const Icon&	extract(const String& path);
	const Icon&	extract(LPCTSTR path, int idx);
	const Icon&	extract(IExtractIcon* pExtract, LPCTSTR path, int idx);

	const Icon&	add(HICON hIcon, ICON_TYPE type=IT_DYNAMIC);
	const Icon&	add(int sys_idx/*, ICON_TYPE type=IT_SYSCACHE*/);

	const Icon&	get_icon(int icon_id);
	HIMAGELIST get_sys_imagelist() const {return _himlSys;}

	void	free_icon(int icon_id);

protected:
	static int s_next_id;

	typedef map<int, Icon> IconMap;
	IconMap	_icons;

	typedef map<String, ICON_ID> PathMap;
	PathMap	_pathMap;

	typedef pair<String, int> CachePair;
	typedef map<CachePair, ICON_ID> PathIdxMap;
	PathIdxMap _pathIdxMap;

	HIMAGELIST _himlSys;
};


 /// create a bitmap from an icon
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);

 /// add icon with alpha channel to imagelist using the specified background color
extern int ImageList_AddAlphaIcon(HIMAGELIST himl, HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);


#include "utility/xmlstorage.h"

using namespace XMLStorage;

#include "favorites.h"


 // globals
extern HINSTANCE g_hInstance;
extern IconCache g_icon_cache;
extern ATOM g_hframeClass;


 // display explorer "About" dialog
extern void ibrowser_about(HWND hwndParent);

 // display explorer "open" dialog
extern void ibrowser_open(HWND hwndParent);

 // declare shell32's "Run..." dialog export function
typedef	void (WINAPI* RUNFILEDLG)(HWND hwndOwner, HICON hIcon, LPCSTR lpstrDirectory, LPCSTR lpstrTitle, LPCSTR lpstrDescription, UINT uFlags);

 //
 // Flags for RunFileDlg
 //

#define	RFF_NOBROWSE		0x01	// Removes the browse button.
#define	RFF_NODEFAULT		0x02	// No default item selected.
#define	RFF_CALCDIRECTORY	0x04	// Calculates the working directory from the file name.
#define	RFF_NOLABEL			0x08	// Removes the edit box label.
#define	RFF_NOSEPARATEMEM	0x20	// Removes the Separate Memory Space check box (Windows NT only).
