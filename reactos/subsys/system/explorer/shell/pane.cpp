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
 // pane.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "../explorer_intres.h"


enum IMAGE {
	IMG_NONE=-1,	IMG_FILE=0, 		IMG_DOCUMENT,	IMG_EXECUTABLE,
	IMG_FOLDER, 	IMG_OPEN_FOLDER,	IMG_FOLDER_PLUS,IMG_OPEN_PLUS,	IMG_OPEN_MINUS,
	IMG_FOLDER_UP,	IMG_FOLDER_CUR
};


#define IMAGE_WIDTH 		16
#define IMAGE_HEIGHT		13


static const LPTSTR g_pos_names[COLUMNS] = {
	TEXT(""),			/* symbol */
	TEXT("Name"),
	TEXT("Type"),
	TEXT("Size"),
	TEXT("CDate"),
	TEXT("ADate"),
	TEXT("MDate"),
	TEXT("Index/Inode"),
	TEXT("Links"),
	TEXT("Attributes"),
	TEXT("Security")
};

static const int g_pos_align[] = {
	0,
	HDF_LEFT,	/* Name */
	HDF_LEFT,	/* Type */
	HDF_RIGHT,	/* Size */
	HDF_LEFT,	/* CDate */
	HDF_LEFT,	/* ADate */
	HDF_LEFT,	/* MDate */
	HDF_LEFT,	/* Index */
	HDF_CENTER,	/* Links */
	HDF_CENTER,	/* Attributes */
	HDF_LEFT	/* Security */
};


Pane::Pane(HWND hparent, int id, int id_header, Entry* root, bool treePane, int visible_cols)
 :	super(CreateWindow(TEXT("ListBox"), TEXT(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
			LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
			0, 0, 0, 0, hparent, (HMENU)id, g_Globals._hInstance, 0)),
	_root(root),
	_visible_cols(visible_cols),
	_treePane(treePane)
{
	 // insert entries into listbox
	Entry* entry = _root;

	if (entry)
		insert_entries(entry, -1);

	init();

	create_header(hparent, id_header);
}


LRESULT Pane::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_HSCROLL:
		set_header();
		break;

	  case WM_SETFOCUS: {
		FileChildWindow* child = (FileChildWindow*) SendMessage(GetParent(_hwnd), PM_GET_FILEWND_PTR, 0, 0);

		child->set_focus_pane(this);
		ListBox_SetSel(_hwnd, TRUE, 1);
		/*@todo check menu items */
		break;}

	  case WM_KEYDOWN: {
		FileChildWindow* child = (FileChildWindow*) SendMessage(GetParent(_hwnd), PM_GET_FILEWND_PTR, 0, 0);

		if (wparam == VK_TAB) {
			/*@todo SetFocus(g_Globals.hdrivebar) */
			child->switch_focus_pane();
		}
		break;}
	}

	return super::WndProc(nmsg, wparam, lparam);
}


bool Pane::create_header(HWND hparent, int id)
{
	HWND hwnd = CreateWindow(WC_HEADER, 0, WS_CHILD|WS_VISIBLE|HDS_HORZ/*@todo |HDS_BUTTONS + sort orders*/,
								0, 0, 0, 0, hparent, (HMENU)id, g_Globals._hInstance, 0);
	if (!hwnd)
		return false;

	SetWindowFont(hwnd, GetStockFont(DEFAULT_GUI_FONT), FALSE);

	HD_ITEM hdi;

	hdi.mask = HDI_TEXT|HDI_WIDTH|HDI_FORMAT;

	for(int idx=0; idx<COLUMNS; idx++) {
		hdi.pszText = g_pos_names[idx];
		hdi.fmt = HDF_STRING | g_pos_align[idx];
		hdi.cxy = _widths[idx];
		Header_InsertItem(hwnd, idx, &hdi);
	}

	_hwndHeader = hwnd;

	return true;
}


void Pane::init()
{
	_himl = ImageList_LoadBitmap(g_Globals._hInstance, MAKEINTRESOURCE(IDB_IMAGES), 16, 0, RGB(0,255,0));

	SetWindowFont(_hwnd, _out_wrkr._hfont, FALSE);

	 // read the color for compressed files from registry
	HKEY hkeyExplorer = 0;
	DWORD len = sizeof(_clrCompressed);

	if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), &hkeyExplorer) ||
		RegQueryValueEx(hkeyExplorer, TEXT("AltColor"), 0, NULL, (LPBYTE)&_clrCompressed, &len) || len!=sizeof(_clrCompressed))
		_clrCompressed = RGB(0,0,255);

	if (hkeyExplorer)
		RegCloseKey(hkeyExplorer);

	 // calculate column widths
	_out_wrkr.init_output(_hwnd);
	calc_widths(true);
}


 // calculate prefered width for all visible columns

bool Pane::calc_widths(bool anyway)
{
	int col, x, cx, spc=3*_out_wrkr._spaceSize.cx;
	int entries = ListBox_GetCount(_hwnd);
	int orgWidths[COLUMNS];
	int orgPositions[COLUMNS+1];
	HFONT hfontOld;
	HDC hdc;
	int cnt;

	if (!anyway) {
		memcpy(orgWidths, _widths, sizeof(orgWidths));
		memcpy(orgPositions, _positions, sizeof(orgPositions));
	}

	for(col=0; col<COLUMNS; col++)
		_widths[col] = 0;

	hdc = GetDC(_hwnd);
	hfontOld = SelectFont(hdc, _out_wrkr._hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(_hwnd, cnt);

		DRAWITEMSTRUCT dis;

		dis.CtlType		  = 0;
		dis.CtlID		  = 0;
		dis.itemID		  = 0;
		dis.itemAction	  = 0;
		dis.itemState	  = 0;
		dis.hwndItem	  = _hwnd;
		dis.hDC			  = hdc;
		dis.rcItem.left	  = 0;
		dis.rcItem.top    = 0;
		dis.rcItem.right  = 0;
		dis.rcItem.bottom = 0;
		/*dis.itemData	  = 0; */

		draw_item(&dis, entry, COLUMNS);
	}

	SelectObject(hdc, hfontOld);
	ReleaseDC(_hwnd, hdc);

	x = 0;
	for(col=0; col<COLUMNS; col++) {
		_positions[col] = x;
		cx = _widths[col];

		if (cx) {
			cx += spc;

			if (cx < IMAGE_WIDTH)
				cx = IMAGE_WIDTH;

			_widths[col] = cx;
		}

		x += cx;
	}

	_positions[COLUMNS] = x;

	ListBox_SetHorizontalExtent(_hwnd, x);

	 // no change?
	if (!memcmp(orgWidths, _widths, sizeof(orgWidths)))
		return FALSE;

	 // don't move, if only collapsing an entry
	if (!anyway && _widths[0]<orgWidths[0] &&
		!memcmp(orgWidths+1, _widths+1, sizeof(orgWidths)-sizeof(int))) {
		_widths[0] = orgWidths[0];
		memcpy(_positions, orgPositions, sizeof(orgPositions));

		return FALSE;
	}

	InvalidateRect(_hwnd, 0, TRUE);

	return TRUE;
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
		{err: lstrcpy(buffer,TEXT("???")); return;}

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


void Pane::draw_item(LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol)
{
	TCHAR buffer[BUFFER_LEN];
	DWORD attrs;
	int visible_cols = _visible_cols;
	COLORREF bkcolor, textcolor;
	RECT focusRect = dis->rcItem;
	enum IMAGE img;
	int img_pos, cx;
	int col = 0;

	if (entry) {
		attrs = entry->_data.dwFileAttributes;

		if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			if (entry->_data.cFileName[0]==TEXT('.') && entry->_data.cFileName[1]==TEXT('.')
					&& entry->_data.cFileName[2]==TEXT('\0'))
				img = IMG_FOLDER_UP;
			else if (entry->_data.cFileName[0]==TEXT('.') && entry->_data.cFileName[1]==TEXT('\0'))
				img = IMG_FOLDER_CUR;
			else if ((_treePane && (dis->itemState&ODS_FOCUS)))
				img = IMG_OPEN_FOLDER;
			else
				img = IMG_FOLDER;
		} else {
			if (attrs & ATTRIBUTE_EXECUTABLE)
				img = IMG_EXECUTABLE;
			else if (entry->_type_name)
				img = IMG_DOCUMENT;
			else
				img = IMG_FILE;
		}
	} else {
		attrs = 0;
		img = IMG_NONE;
	}

	if (_treePane) {
		if (entry) {
			img_pos = dis->rcItem.left + entry->_level*(IMAGE_WIDTH+_out_wrkr._spaceSize.cx);

			if (calcWidthCol == -1) {
				int x;
				int y = dis->rcItem.top + IMAGE_HEIGHT/2;
				Entry* up;
				RECT rt_clip;
				HRGN hrgn_org = CreateRectRgn(0, 0, 0, 0);
				HRGN hrgn;

				rt_clip.left   = dis->rcItem.left;
				rt_clip.top    = dis->rcItem.top;
				rt_clip.right  = dis->rcItem.left+_widths[col];
				rt_clip.bottom = dis->rcItem.bottom;

				hrgn = CreateRectRgnIndirect(&rt_clip);

				if (!GetClipRgn(dis->hDC, hrgn_org)) {
					DeleteObject(hrgn_org);
					hrgn_org = 0;
				}

				//HGDIOBJ holdPen = SelectObject(dis->hDC, GetStockObject(BLACK_PEN));
				ExtSelectClipRgn(dis->hDC, hrgn, RGN_AND);
				DeleteObject(hrgn);

				if ((up=entry->_up) != NULL) {
					MoveToEx(dis->hDC, img_pos-IMAGE_WIDTH/2, y, 0);
					LineTo(dis->hDC, img_pos-2, y);

					x = img_pos - IMAGE_WIDTH/2;

					do {
						x -= IMAGE_WIDTH+_out_wrkr._spaceSize.cx;

						if (up->_next) {
#ifndef _LEFT_FILES
							bool following_child = (up->_next->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0;	// a directory?

							if (!following_child)
								for(Entry*n=up->_next; n; n=n->_next)
									if (n->_down) {	// any file with NTFS sub-streams?
										following_child = true;
										break;
									}

							if (following_child)
#endif
							{
								MoveToEx(dis->hDC, x, dis->rcItem.top, 0);
								LineTo(dis->hDC, x, dis->rcItem.bottom);
							}
						}
					} while((up=up->_up) != NULL);
				}

				x = img_pos - IMAGE_WIDTH/2;

				MoveToEx(dis->hDC, x, dis->rcItem.top, 0);
				LineTo(dis->hDC, x, y);

				if (entry->_next) {
#ifndef _LEFT_FILES
					bool following_child = (entry->_next->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0;	// a directory?

					if (!following_child)
						for(Entry*n=entry->_next; n; n=n->_next)
							if (n->_down) {	// any file with NTFS sub-streams?
								following_child = true;
								break;
							}

					if (following_child)
#endif
						LineTo(dis->hDC, x, dis->rcItem.bottom);
				}

				if (entry->_down && entry->_expanded) {
					x += IMAGE_WIDTH + _out_wrkr._spaceSize.cx;
					MoveToEx(dis->hDC, x, dis->rcItem.top+IMAGE_HEIGHT, 0);
					LineTo(dis->hDC, x, dis->rcItem.bottom);
				}

				SelectClipRgn(dis->hDC, hrgn_org);
				if (hrgn_org) DeleteObject(hrgn_org);
				//SelectObject(dis->hDC, holdPen);
			} else if (calcWidthCol==col || calcWidthCol==COLUMNS) {
				int right = img_pos + IMAGE_WIDTH - _out_wrkr._spaceSize.cx;

				if (right > _widths[col])
					_widths[col] = right;
			}
		} else	{
			img_pos = dis->rcItem.left;
		}
	} else {
		img_pos = dis->rcItem.left;

		if (calcWidthCol==col || calcWidthCol==COLUMNS)
			_widths[col] = IMAGE_WIDTH;
	}

	if (calcWidthCol == -1) {
		focusRect.left = img_pos -2;

		if (attrs & FILE_ATTRIBUTE_COMPRESSED)
			textcolor = _clrCompressed;
		else
			textcolor = RGB(0,0,0);

		if (dis->itemState & ODS_FOCUS) {
			textcolor = GetSysColor(COLOR_HIGHLIGHTTEXT);
			bkcolor = GetSysColor(COLOR_HIGHLIGHT);
		} else {
			bkcolor = GetSysColor(COLOR_WINDOW);
		}

		HBRUSH hbrush = CreateSolidBrush(bkcolor);
		FillRect(dis->hDC, &focusRect, hbrush);
		DeleteObject(hbrush);

		SetBkMode(dis->hDC, TRANSPARENT);
		SetTextColor(dis->hDC, textcolor);

		cx = _widths[col];

		if (cx && img!=IMG_NONE) {
			if (cx > IMAGE_WIDTH)
				cx = IMAGE_WIDTH;

			if (entry->_icon_id > ICID_NONE)
				g_Globals._icon_cache.get_icon(entry->_icon_id).draw(dis->hDC, img_pos, dis->rcItem.top, cx, GetSystemMetrics(SM_CYSMICON), bkcolor, 0);
			else
				ImageList_DrawEx(_himl, img, dis->hDC,
								 img_pos, dis->rcItem.top, cx,
								 IMAGE_HEIGHT, bkcolor, CLR_DEFAULT, ILD_NORMAL);
		}
	}

	if (!entry)
		return;

	++col;

	 // ouput file name
	if (calcWidthCol == -1)
		_out_wrkr.output_text(dis, _positions, col, entry->_display_name, 0);
	else if (calcWidthCol==col || calcWidthCol==COLUMNS)
		calc_width(dis, col, entry->_display_name);

	++col;

	 // ouput type/class name
	if (calcWidthCol == -1)
		_out_wrkr.output_text(dis, _positions, col, entry->_type_name, 0);
	else if (calcWidthCol==col || calcWidthCol==COLUMNS)
		calc_width(dis, col, entry->_type_name);

	++col;

	 // display file size
	if (visible_cols & COL_SIZE) {
		ULONGLONG size = ((ULONGLONG)entry->_data.nFileSizeHigh << 32) | entry->_data.nFileSizeLow;

		_stprintf(buffer, TEXT("%") LONGLONGARG TEXT("d"), size);

		if (calcWidthCol == -1)
			_out_wrkr.output_number(dis, _positions, col, buffer);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(dis, col, buffer);	///@todo not in every case time enough

		++col;
	}

	 // display file date
	if (visible_cols & (COL_DATE|COL_TIME)) {
		format_date(&entry->_data.ftCreationTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			_out_wrkr.output_text(dis, _positions, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(dis, col, buffer);
		++col;

		format_date(&entry->_data.ftLastAccessTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			_out_wrkr.output_text(dis,_positions,  col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(dis, col, buffer);
		++col;

		format_date(&entry->_data.ftLastWriteTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			_out_wrkr.output_text(dis, _positions, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(dis, col, buffer);
		++col;
	}

	if (entry->_bhfi_valid) {
		ULONGLONG index = ((ULONGLONG)entry->_bhfi.nFileIndexHigh << 32) | entry->_bhfi.nFileIndexLow;

		if (visible_cols & COL_INDEX) {
			_stprintf(buffer, TEXT("%") LONGLONGARG TEXT("X"), index);
			if (calcWidthCol == -1)
				_out_wrkr.output_text(dis, _positions, col, buffer, DT_RIGHT);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(dis, col, buffer);
			++col;
		}

		if (visible_cols & COL_LINKS) {
			wsprintf(buffer, TEXT("%d"), entry->_bhfi.nNumberOfLinks);
			if (calcWidthCol == -1)
				_out_wrkr.output_text(dis, _positions, col, buffer, DT_CENTER);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(dis, col, buffer);
			++col;
		}
	} else
		col += 2;

	 // show file attributes
	if (visible_cols & COL_ATTRIBUTES) {
		lstrcpy(buffer, TEXT(" \t \t \t \t \t \t \t \t \t \t \t "));

		if (attrs & FILE_ATTRIBUTE_NORMAL)					buffer[ 0] = 'N';
		else {
			if (attrs & FILE_ATTRIBUTE_READONLY)			buffer[ 2] = 'R';
			if (attrs & FILE_ATTRIBUTE_HIDDEN)				buffer[ 4] = 'H';
			if (attrs & FILE_ATTRIBUTE_SYSTEM)				buffer[ 6] = 'S';
			if (attrs & FILE_ATTRIBUTE_ARCHIVE) 			buffer[ 8] = 'A';
			if (attrs & FILE_ATTRIBUTE_COMPRESSED)			buffer[10] = 'C';
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)			buffer[12] = 'D';
			if (attrs & FILE_ATTRIBUTE_ENCRYPTED)			buffer[14] = 'E';
			if (attrs & FILE_ATTRIBUTE_TEMPORARY)			buffer[16] = 'T';
			if (attrs & FILE_ATTRIBUTE_SPARSE_FILE) 		buffer[18] = 'P';
			if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)		buffer[20] = 'Q';
			if (attrs & FILE_ATTRIBUTE_OFFLINE) 			buffer[22] = 'O';
			if (attrs & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) buffer[24] = 'X';
		}

		if (calcWidthCol == -1)
			_out_wrkr.output_tabbed_text(dis, _positions, col, buffer);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_tabbed_width(dis, col, buffer);

		++col;
	}

/*TODO
	if (flags.security) {
		DWORD rights = get_access_mask();

		tcscpy(buffer, TEXT(" \t \t \t  \t  \t \t \t  \t  \t \t \t "));

		if (rights & FILE_READ_DATA)			buffer[ 0] = 'R';
		if (rights & FILE_WRITE_DATA)			buffer[ 2] = 'W';
		if (rights & FILE_APPEND_DATA)			buffer[ 4] = 'A';
		if (rights & FILE_READ_EA)				{buffer[6] = 'entry'; buffer[ 7] = 'R';}
		if (rights & FILE_WRITE_EA) 			{buffer[9] = 'entry'; buffer[10] = 'W';}
		if (rights & FILE_EXECUTE)				buffer[12] = 'X';
		if (rights & FILE_DELETE_CHILD) 		buffer[14] = 'D';
		if (rights & FILE_READ_ATTRIBUTES)		{buffer[16] = 'a'; buffer[17] = 'R';}
		if (rights & FILE_WRITE_ATTRIBUTES) 	{buffer[19] = 'a'; buffer[20] = 'W';}
		if (rights & WRITE_DAC) 				buffer[22] = 'C';
		if (rights & WRITE_OWNER)				buffer[24] = 'O';
		if (rights & SYNCHRONIZE)				buffer[26] = 'S';

		output_text(dis, col++, buffer, DT_LEFT, 3, psize);
	}

	if (flags.description) {
		get_description(buffer);
		output_text(dis, col++, buffer, 0, psize);
	}
*/
}


void Pane::calc_width(LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0, 0, 0, 0};

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

	if (rt.right > _widths[col])
		_widths[col] = rt.right;
}

void Pane::calc_tabbed_width(LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0, 0, 0, 0};

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
	//FIXME rt (0,0) ???

	if (rt.right > _widths[col])
		_widths[col] = rt.right;
}


 // insert listbox entries after index idx

void Pane::insert_entries(Entry* dir, int idx)
{
	Entry* entry = dir;

	if (!entry)
		return;

	SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);	//ShowWindow(_hwnd, SW_HIDE);

	for(; entry; entry=entry->_next) {
#ifndef _LEFT_FILES
		if (_treePane &&
			!(entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) &&	// not a directory?
			!entry->_down)	// not a file with NTFS sub-streams?
			continue;
#endif

		 // don't display entries "." and ".." in the left pane
		if (_treePane && (entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				&& entry->_data.cFileName[0]==TEXT('.'))
			if (entry->_data.cFileName[1]==TEXT('\0') ||
				(entry->_data.cFileName[1]==TEXT('.') && entry->_data.cFileName[2]==TEXT('\0')))
				continue;

		if (idx != -1)
			++idx;

		ListBox_InsertItemData(_hwnd, idx, entry);

		if (_treePane && entry->_expanded)
			insert_entries(entry->_down, idx);
	}

	SendMessage(_hwnd, WM_SETREDRAW, TRUE, 0);	//ShowWindow(_hwnd, SW_SHOW);
}


void Pane::set_header()
{
	HD_ITEM item;
	int scroll_pos = GetScrollPos(_hwnd, SB_HORZ);
	int i=0, x=0;

	item.mask = HDI_WIDTH;
	item.cxy = 0;

	for(; x+_widths[i]<scroll_pos && i<COLUMNS; i++) {
		x += _widths[i];
		Header_SetItem(_hwndHeader, i, &item);
	}

	if (i < COLUMNS) {
		x += _widths[i];
		item.cxy = x - scroll_pos;
		Header_SetItem(_hwndHeader, i++, &item);

		for(; i<COLUMNS; i++) {
			item.cxy = _widths[i];
			x += _widths[i];
			Header_SetItem(_hwndHeader, i, &item);
		}
	}
}


 // calculate one prefered column width

void Pane::calc_single_width(int col)
{
	HFONT hfontOld;
	int x, cx;
	int cnt;
	HDC hdc;

	int entries = ListBox_GetCount(_hwnd);

	_widths[col] = 0;

	hdc = GetDC(_hwnd);
	hfontOld = SelectFont(hdc, _out_wrkr._hfont);

	for(cnt=0; cnt<entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(_hwnd, cnt);

		DRAWITEMSTRUCT dis;

		dis.CtlType		  = 0;
		dis.CtlID		  = 0;
		dis.itemID		  = 0;
		dis.itemAction	  = 0;
		dis.itemState	  = 0;
		dis.hwndItem	  = _hwnd;
		dis.hDC			  = hdc;
		dis.rcItem.left	  = 0;
		dis.rcItem.top    = 0;
		dis.rcItem.right  = 0;
		dis.rcItem.bottom = 0;
		/*dis.itemData	  = 0; */

		draw_item(&dis, entry, col);
	}

	SelectObject(hdc, hfontOld);
	ReleaseDC(_hwnd, hdc);

	cx = _widths[col];

	if (cx) {
		cx += 3*_out_wrkr._spaceSize.cx;

		if (cx < IMAGE_WIDTH)
			cx = IMAGE_WIDTH;
	}

	_widths[col] = cx;

	x = _positions[col] + cx;

	for(; col<COLUMNS; ) {
		_positions[++col] = x;
		x += _widths[col];
	}

	ListBox_SetHorizontalExtent(_hwnd, x);
}


int Pane::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
		case HDN_TRACK:
		case HDN_ENDTRACK: {
			HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
			int idx = phdn->iItem;
			int dx = phdn->pitem->cxy - _widths[idx];
			int i;

			ClientRect clnt(_hwnd);

			 // move immediate to simulate HDS_FULLDRAG (for now [04/2000] not realy needed with WINELIB)
			Header_SetItem(_hwndHeader, idx, phdn->pitem);

			_widths[idx] += dx;

			for(i=idx; ++i<=COLUMNS; )
				_positions[i] += dx;

			{
				int scroll_pos = GetScrollPos(_hwnd, SB_HORZ);
				RECT rt_scr;
				RECT rt_clip;

				rt_scr.left   = _positions[idx+1]-scroll_pos;
				rt_scr.top    = 0;
				rt_scr.right  = clnt.right;
				rt_scr.bottom = clnt.bottom;

				rt_clip.left   = _positions[idx]-scroll_pos;
				rt_clip.top    = 0;
				rt_clip.right  = clnt.right;
				rt_clip.bottom = clnt.bottom;

				if (rt_scr.left < 0) rt_scr.left = 0;
				if (rt_clip.left < 0) rt_clip.left = 0;

				ScrollWindowEx(_hwnd, dx, 0, &rt_scr, &rt_clip, 0, 0, SW_INVALIDATE);

				rt_clip.right = _positions[idx+1];
				RedrawWindow(_hwnd, &rt_clip, 0, RDW_INVALIDATE|RDW_UPDATENOW);

				if (pnmh->code == HDN_ENDTRACK) {
					ListBox_SetHorizontalExtent(_hwnd, _positions[COLUMNS]);

					if (GetScrollPos(_hwnd, SB_HORZ) != scroll_pos)
						set_header();
				}
			}

			return 0;
		}

		case HDN_DIVIDERDBLCLICK: {
			HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
			HD_ITEM item;

			calc_single_width(phdn->iItem);
			item.mask = HDI_WIDTH;
			item.cxy = _widths[phdn->iItem];

			Header_SetItem(_hwndHeader, phdn->iItem, &item);
			InvalidateRect(_hwnd, 0, TRUE);
			break;}

		default:
			return super::Notify(id, pnmh);
	}

	return 0;
}


OutputWorker::OutputWorker()
{
	_hfont = CreateFont(-MulDiv(8,GetDeviceCaps(WindowCanvas(0),LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("MS Sans Serif"));
}

void OutputWorker::init_output(HWND hwnd)
{
	TCHAR b[16];

	WindowCanvas canvas(hwnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, TEXT("1000"), 0, b, 16) > 4)
		_num_sep = b[1];
	else
		_num_sep = TEXT('.');

	FontSelection font(canvas, _hfont);
	GetTextExtentPoint32(canvas, TEXT(" "), 1, &_spaceSize);
}


void OutputWorker::output_text(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str, DWORD flags)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+positions[col]+_spaceSize.cx;
	rt.top	  = dis->rcItem.top;
	rt.right  = x+positions[col+1]-_spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|flags);
}

void OutputWorker::output_tabbed_text(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;

	rt.left   = x+positions[col]+_spaceSize.cx;
	rt.top	  = dis->rcItem.top;
	rt.right  = x+positions[col+1]-_spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
}

void OutputWorker::output_number(LPDRAWITEMSTRUCT dis, int* positions, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt;
	LPCTSTR s = str;
	TCHAR b[128];
	LPTSTR d = b;
	int pos;

	rt.left   = x+positions[col]+_spaceSize.cx;
	rt.top	  = dis->rcItem.top;
	rt.right  = x+positions[col+1]-_spaceSize.cx;
	rt.bottom = dis->rcItem.bottom;

	if (*s)
		*d++ = *s++;

	 // insert number separator characters
	pos = lstrlen(s) % 3;

	while(*s)
		if (pos--)
			*d++ = *s++;
		else {
			*d++ = _num_sep;
			pos = 3;
		}

	DrawText(dis->hDC, b, d-b, &rt, DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
}


BOOL Pane::command(UINT cmd)
{
	switch(cmd) {
		case ID_VIEW_NAME:
			if (_visible_cols) {
				_visible_cols = 0;
				calc_widths(true);
				set_header();
				InvalidateRect(_hwnd, 0, TRUE);
				MenuInfo* menu_info = Frame_GetMenuInfo(GetParent(_hwnd));
				if (menu_info) {
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_NAME, MF_BYCOMMAND|MF_CHECKED);
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND);
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
				}
			}
			break;

		case ID_VIEW_ALL_ATTRIBUTES:
			if (_visible_cols != COL_ALL) {
				_visible_cols = COL_ALL;
				calc_widths(true);
				set_header();
				InvalidateRect(_hwnd, 0, TRUE);
				MenuInfo* menu_info = Frame_GetMenuInfo(GetParent(_hwnd));
				if (menu_info) {
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_NAME, MF_BYCOMMAND);
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND|MF_CHECKED);
					CheckMenuItem(menu_info->_hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
				}
			}
			break;

		case ID_PREFERED_SIZES: {
			calc_widths(true);
			set_header();
			InvalidateRect(_hwnd, 0, TRUE);
			break;}

		        /*@todo more command ids... */

		default:
			return FALSE;
	}

	return TRUE;
}

MainFrame* Pane::get_frame()
{
	HWND owner = GetParent(_hwnd);

	return (MainFrame*)owner;
}
