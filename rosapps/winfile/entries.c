/*
 *  ReactOS winfile
 *
 *  entries.c
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
    
#include <windowsx.h>

#include "main.h"
#include "entries.h"
#include "utils.h"



Entry* find_entry_win(Entry* parent, LPCTSTR name)
{
	Entry* entry;

	for(entry=parent->down; entry; entry=entry->next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->data.cFileName;

		do {
			if (!*p || *p==_T('\\') || *p==_T('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->data.cAlternateFileName;

		do {
			if (!*p || *p==_T('\\') || *p==_T('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return 0;
}


Entry* read_tree_win(Root* root, LPCTSTR path, int sortOrder)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCTSTR s = path;
	PTSTR d = buffer;

#ifndef _NO_EXTENSIONS
	entry->unix_dir = FALSE;
#endif
	
	while(entry) {
		while(*s && *s!=_T('\\') && *s!=_T('/'))
			*d++ = *s++;

		while(*s==_T('\\') || *s==_T('/'))
			s++;

		*d++ = _T('\\');
		*d = _T('\0');

		read_directory(entry, buffer, sortOrder);

		if (entry->down)
			entry->expanded = TRUE;

		if (!*s)
			break;

		entry = find_entry_win(entry, s);
	}

	return entry;
}


#if !defined(_NO_EXTENSIONS) && defined(__linux__)

static Entry* find_entry_unix(Entry* parent, LPCTSTR name)
{
	Entry* entry;

	for(entry=parent->down; entry; entry=entry->next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->data.cFileName;

		do {
			if (!*p || *p==_T('/'))
				return entry;
		} while(*p++ == *q++);
	}

	return 0;
}

static Entry* read_tree_unix(Root* root, LPCTSTR path, int sortOrder)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCTSTR s = path;
	PTSTR d = buffer;

	entry->unix_dir = TRUE;

	while(entry) {
		while(*s && *s!=_T('/'))
			*d++ = *s++;

		while(*s == _T('/'))
			s++;

		*d++ = _T('/');
		*d = _T('\0');

		read_directory(entry, buffer, sortOrder);

		if (entry->down)
			entry->expanded = TRUE;

		if (!*s)
			break;

		entry = find_entry_unix(entry, s);
	}

	return entry;
}

#endif


 // recursively free all child entries
void free_entries(Entry* parent)
{
	Entry *entry, *next=parent->down;

	if (next) {
		parent->down = 0;
		do {
			entry = next;
			next = entry->next;
			free_entries(entry);
			free(entry);
		} while(next);
	}
}

// insert listbox entries after index idx
void insert_entries(Pane* pane, Entry* parent, int idx)
{
/*
	Entry* entry = parent;

	if (!entry)
		return;
	ShowWindow(pane->hWnd, SW_HIDE);
	for(; entry; entry=entry->next) {
#ifndef _LEFT_FILES
		if (pane->treePane && !(entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			continue;
#endif
		 // don't display entries "." and ".." in the left pane
		if (pane->treePane && (entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				&& entry->data.cFileName[0]==_T('.'))
			if (
#ifndef _NO_EXTENSIONS
				entry->data.cFileName[1]==_T('\0') ||
#endif
				(entry->data.cFileName[1]==_T('.') && entry->data.cFileName[2]==_T('\0')))
				continue;
		if (idx != -1)
			idx++;
		ListBox_InsertItemData(pane->hWnd, idx, entry);
		if (pane->treePane && entry->expanded)
			insert_entries(pane, entry->down, idx);
	}
	ShowWindow(pane->hWnd, SW_SHOW);
 */
}


void scan_entry(ChildWnd* child, Entry* entry)
{
	TCHAR path[MAX_PATH];
/*
	int idx = ListBox_GetCurSel(child->left.hWnd);
	HCURSOR crsrOld = SetCursor(LoadCursor(0, IDC_WAIT));

	 // delete sub entries in left pane
	for(;;) {
		LRESULT res = ListBox_GetItemData(child->left.hWnd, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=entry->level)
			break;

		ListBox_DeleteString(child->left.hWnd, idx+1);
	}

	 // empty right pane
	ListBox_ResetContent(child->right.hWnd);
 */
	 // release memory
	free_entries(entry);

	 // read contents from disk
	get_path(entry, path);
	read_directory(entry, path, child->sortOrder);
/*
	 // insert found entries in right pane
	insert_entries(&child->right, entry->down, -1);
	calc_widths(&child->right, FALSE);
#ifndef _NO_EXTENSIONS
	set_header(&child->right);
#endif

	child->header_wdths_ok = FALSE;

	SetCursor(crsrOld);
 */
}


// expand a directory entry
BOOL expand_entry(ChildWnd* child, Entry* dir)
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

	 // no subdirectories ?
	if (!(p->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
/*
	idx = ListBox_FindItemData(child->left.hWnd, 0, dir);
 */
	dir->expanded = TRUE;

	 // insert entries in left pane
	insert_entries(&child->left, p, idx);
/*
	if (!child->header_wdths_ok) {
		if (calc_widths(&child->left, FALSE)) {
#ifndef _NO_EXTENSIONS
			set_header(&child->left);
#endif

			child->header_wdths_ok = TRUE;
		}
	}
 */
	return TRUE;
}


void collapse_entry(Pane* pane, Entry* dir)
{
	int idx = ListBox_FindItemData(pane->hWnd, 0, dir);

	ShowWindow(pane->hWnd, SW_HIDE);

	 // hide sub entries
	for(;;) {
		LRESULT res = ListBox_GetItemData(pane->hWnd, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->level<=dir->level)
			break;

		ListBox_DeleteString(pane->hWnd, idx+1);
	}

	dir->expanded = FALSE;

	ShowWindow(pane->hWnd, SW_SHOW);
}


void activate_entry(ChildWnd* child, Pane* pane)
{
	Entry* entry = pane->cur;

	if (!entry)
		return;

	if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		int scanned_old = entry->scanned;

		if (!scanned_old)
			scan_entry(child, entry);

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
				int idx = ListBox_FindItemData(child->left.hWnd, ListBox_GetCurSel(child->left.hWnd), entry);
				ListBox_SetCurSel(child->left.hWnd, idx);
				set_curdir(child, entry);
			}
		}

		if (!scanned_old) {
			calc_widths(pane, FALSE);

#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
		}
	} else {

		//TODO: start program, open document...

	}
}
