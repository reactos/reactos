/*
 *  ReactOS winfile
 *
 *  entries.h
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

#ifndef __ENTRIES_H__
#define __ENTRIES_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _Entry {
	struct _Entry*	next;
	struct _Entry*	down;
	struct _Entry*	up;

	BOOL	expanded;
	BOOL	scanned;
	int		level;

	WIN32_FIND_DATA	data;
    HTREEITEM hTreeItem;

#ifndef _NO_EXTENSIONS
	BY_HANDLE_FILE_INFORMATION bhfi;
	BOOL	bhfi_valid;
	BOOL	unix_dir;
#endif
} Entry;


typedef enum {
	SORT_NAME,
	SORT_EXT,
	SORT_SIZE,
	SORT_DATE
} SORT_ORDER;

//enum SORT_ORDER { SORT_BY_NAME, SORT_BY_TYPE, SORT_BY_SIZE, SORT_BY_DATE };

typedef struct {
	Entry	entry;
	TCHAR	path[MAX_PATH];
	TCHAR	volname[_MAX_FNAME];
	TCHAR	fs[_MAX_DIR];
	DWORD	drive_type;
	DWORD	fs_flags;
} Root;


typedef struct {
	HWND	hWnd;

#ifndef _NO_EXTENSIONS
#define	COLUMNS	10
#else
#define	COLUMNS	5
#endif
	int		widths[COLUMNS];
	int		positions[COLUMNS+1];

	Entry*	root;
	Entry*	cur;
} Pane;

typedef struct {
	HWND	hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      // 0: left  1: right
	int		nSplitPos;
	WINDOWPLACEMENT pos;
	TCHAR	szPath[MAX_PATH];

//	Root	root;
	Root*	pRoot;
	Pane	left;
	Pane	right;
	SORT_ORDER sortOrder;
	int last_split;

} ChildWnd;


void insert_entries(HWND hWnd, Entry* parent, int idx);
void scan_entry(ChildWnd* child, Entry* entry);
void activate_entry(ChildWnd* child, Pane* pane);
void collapse_entry(Pane* pane, Entry* dir);
BOOL expand_entry(ChildWnd* child, Entry* dir);
Entry* find_entry_win(Entry* parent, LPCTSTR name);
void free_entries(Entry* parent);
Entry* read_tree_win(Root* root, LPCTSTR path, int sortOrder);



#ifdef __cplusplus
};
#endif

#endif // __ENTRIES_H__
