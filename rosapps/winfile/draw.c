/*
 *  ReactOS winfile
 *
 *  draw.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "main.h"
#include "utils.h"
#include "draw.h"


#define	COLOR_COMPRESSED	RGB(0,0,255)
#define	COLOR_SELECTION		RGB(0,0,128)


static void format_date(const FILETIME* ft, TCHAR* buffer, int visible_cols)
{
	SYSTEMTIME systime;
	FILETIME lft;
	int len = 0;

	*buffer = _T('\0');

	if (!ft->dwLowDateTime && !ft->dwHighDateTime)
		return;
		
	if (!FileTimeToLocalFileTime(ft, &lft))
		{err: _tcscpy(buffer,_T("???")); return;}

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
			buffer[len] = _T('\0');
	}
}


static void calc_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0};

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}

static void calc_tabbed_width(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	RECT rt = {0};

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
//@@ rt (0,0) ???

	if (rt.right > pane->widths[col])
		pane->widths[col] = rt.right;
}


static void output_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str, DWORD flags)
{
	int x = dis->rcItem.left;
	RECT rt = {x+pane->positions[col]+Globals.spaceSize.cx, dis->rcItem.top, x+pane->positions[col+1]-Globals.spaceSize.cx, dis->rcItem.bottom};

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|flags);
}

static void output_tabbed_text(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt = {x+pane->positions[col]+Globals.spaceSize.cx, dis->rcItem.top, x+pane->positions[col+1]-Globals.spaceSize.cx, dis->rcItem.bottom};

/*	DRAWTEXTPARAMS dtp = {sizeof(DRAWTEXTPARAMS), 2};
	DrawTextEx(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS|DT_TABSTOP, &dtp);*/

	DrawText(dis->hDC, (LPTSTR)str, -1, &rt, DT_SINGLELINE|DT_EXPANDTABS|DT_TABSTOP|(2<<8));
}

static void output_number(Pane* pane, LPDRAWITEMSTRUCT dis, int col, LPCTSTR str)
{
	int x = dis->rcItem.left;
	RECT rt = {x+pane->positions[col]+Globals.spaceSize.cx, dis->rcItem.top, x+pane->positions[col+1]-Globals.spaceSize.cx, dis->rcItem.bottom};
	LPCTSTR s = str;
	TCHAR b[128];
	LPTSTR d = b;
	int pos;

	if (*s)
		*d++ = *s++;

	 // insert number separator characters
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


void draw_item(Pane* pane, LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol)
{
#if 0
	TCHAR buffer[BUFFER_LEN];
	DWORD attrs;
	int visible_cols = pane->visible_cols;
	COLORREF bkcolor, textcolor;
	RECT focusRect = dis->rcItem;
	HBRUSH hbrush;
	enum IMAGE img;
#ifndef _NO_EXTENSIONS
	QWORD index;
#endif
	int img_pos, cx;
	int col = 0;

	if (entry) {
		attrs = entry->data.dwFileAttributes;

		if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			if (entry->data.cFileName[0]==_T('.') && entry->data.cFileName[1]==_T('.')
					&& entry->data.cFileName[2]==_T('\0'))
				img = IMG_FOLDER_UP;
#ifndef _NO_EXTENSIONS
			else if (entry->data.cFileName[0]==_T('.') && entry->data.cFileName[1]==_T('\0'))
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
			LPCTSTR ext = _tcsrchr(entry->data.cFileName, '.');
			if (!ext)
				ext = _T("");

			if (is_exe_file(ext))
				img = IMG_EXECUTABLE;
			else if (is_registered_type(ext))
				img = IMG_DOCUMENT;
			else
				img = IMG_FILE;
		}
	} else {
		attrs = 0;
		img = IMG_NONE;
	}

	if (pane->treePane) {
		if (entry) {
			img_pos = dis->rcItem.left + entry->level*(IMAGE_WIDTH+Globals.spaceSize.cx);

			if (calcWidthCol == -1) {
				int x;
				int y = dis->rcItem.top + IMAGE_HEIGHT/2;
				Entry* up;
				RECT rt_clip = {dis->rcItem.left, dis->rcItem.top, dis->rcItem.left+pane->widths[col], dis->rcItem.bottom};
				HRGN hrgn_org = CreateRectRgn(0, 0, 0, 0);
				HRGN hrgn = CreateRectRgnIndirect(&rt_clip);

				if (!GetClipRgn(dis->hDC, hrgn_org)) {
					DeleteObject(hrgn_org);
					hrgn_org = 0;
				}

//				HGDIOBJ holdPen = SelectObject(dis->hDC, GetStockObject(BLACK_PEN));
				ExtSelectClipRgn(dis->hDC, hrgn, RGN_AND);
				DeleteObject(hrgn);

				if ((up=entry->up) != NULL) {
					MoveToEx(dis->hDC, img_pos-IMAGE_WIDTH/2, y, 0);
					LineTo(dis->hDC, img_pos-2, y);

					x = img_pos - IMAGE_WIDTH/2;

					do {
						x -= IMAGE_WIDTH+Globals.spaceSize.cx;

						if (up->next
#ifndef _LEFT_FILES
							&& (up->next->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
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

				if (entry->down && entry->expanded) {
					x += IMAGE_WIDTH+Globals.spaceSize.cx;
					MoveToEx(dis->hDC, x, dis->rcItem.top+IMAGE_HEIGHT, 0);
					LineTo(dis->hDC, x, dis->rcItem.bottom);
				}

				SelectClipRgn(dis->hDC, hrgn_org);
				if (hrgn_org) DeleteObject(hrgn_org);
//				SelectObject(dis->hDC, holdPen);
			} else if (calcWidthCol==col || calcWidthCol==COLUMNS) {
				int right = img_pos + IMAGE_WIDTH - Globals.spaceSize.cx;

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

#ifdef _NO_EXTENSIONS
		if (pane->treePane && entry) {
			RECT rt = {0};

			DrawText(dis->hDC, entry->data.cFileName, -1, &rt, DT_CALCRECT|DT_SINGLELINE|DT_NOPREFIX);

			focusRect.right = dis->rcItem.left+pane->positions[col+1]+Globals.spaceSize.cx + rt.right +2;
		}
#else

		if (attrs & FILE_ATTRIBUTE_COMPRESSED)
			textcolor = COLOR_COMPRESSED;
		else
#endif
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

	 // ouput file name
	if (calcWidthCol == -1)
		output_text(pane, dis, col, entry->data.cFileName, 0);
	else if (calcWidthCol==col || calcWidthCol==COLUMNS)
		calc_width(pane, dis, col, entry->data.cFileName);

	col++;

#ifdef _NO_EXTENSIONS
  if (!pane->treePane) {
#endif

	 // display file size
	if (visible_cols & COL_SIZE) {
#ifdef _NO_EXTENSIONS
		if (!(attrs&FILE_ATTRIBUTE_DIRECTORY))
#endif
		{
			QWORD size;

			*(DWORD*)(&size) = entry->data.nFileSizeLow;	//TODO: platform spefific
			*(((DWORD*)&size)+1) = entry->data.nFileSizeHigh;

			_stprintf(buffer, _T("%") LONGLONGARG _T("d"), size);

			if (calcWidthCol == -1)
				output_number(pane, dis, col, buffer);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);//TODO: not ever time enough
		}

		col++;
	}

	 // display file date
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
#endif

		format_date(&entry->data.ftLastWriteTime, buffer, visible_cols);
		if (calcWidthCol == -1)
			output_text(pane, dis, col, buffer, 0);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_width(pane, dis, col, buffer);
		col++;
	}

#ifndef _NO_EXTENSIONS
	if (entry->bhfi_valid) {
		((DWORD*)&index)[0] = entry->bhfi.nFileIndexLow;	//TODO: platform spefific
		((DWORD*)&index)[1] = entry->bhfi.nFileIndexHigh;

		if (visible_cols & COL_INDEX) {
			_stprintf(buffer, _T("%") LONGLONGARG _T("X"), index);
			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_RIGHT);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);
			col++;
		}

		if (visible_cols & COL_LINKS) {
			wsprintf(buffer, _T("%d"), entry->bhfi.nNumberOfLinks);
			if (calcWidthCol == -1)
				output_text(pane, dis, col, buffer, DT_CENTER);
			else if (calcWidthCol==col || calcWidthCol==COLUMNS)
				calc_width(pane, dis, col, buffer);
			col++;
		}
	} else
		col += 2;
#endif

	 // show file attributes
	if (visible_cols & COL_ATTRIBUTES) {
#ifdef _NO_EXTENSIONS
		_tcscpy(buffer, _T(" \t \t \t \t "));
#else
		_tcscpy(buffer, _T(" \t \t \t \t \t \t \t \t \t \t \t "));
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
#endif
		}

		if (calcWidthCol == -1)
			output_tabbed_text(pane, dis, col, buffer);
		else if (calcWidthCol==col || calcWidthCol==COLUMNS)
			calc_tabbed_width(pane, dis, col, buffer);

		col++;
	}

/*TODO
	if (flags.security) {
		DWORD rights = get_access_mask();

		tcscpy(buffer, _T(" \t \t \t  \t  \t \t \t  \t  \t \t \t "));

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

	 // draw focus frame
	if ((dis->itemState&ODS_FOCUS) && calcWidthCol==-1) {
		 // Currently [04/2000] Wine neither behaves exactly the same
		 // way as WIN 95 nor like Windows NT...
#ifdef WINELIB
		DrawFocusRect(dis->hDC, &focusRect);
#else
		HGDIOBJ lastBrush;
		HPEN lastPen;
		HPEN hpen;

		if (!(GetVersion() & 0x80000000)) {	// Windows NT?
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
#endif
	}
#endif
#endif
}

