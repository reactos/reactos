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
 // shellfs.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include <precomp.h>

//#include "shellfs.h"
//#include "winfs.h"

#include <shlwapi.h>


bool ShellDirectory::fill_w32fdata_shell(LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA* pw32fdata, BY_HANDLE_FILE_INFORMATION* pbhfi, bool do_access)
{
	CONTEXT("ShellDirectory::fill_w32fdata_shell()");

	bool bhfi_valid = false;

	if (do_access && !( (attribs&SFGAO_FILESYSTEM) && SUCCEEDED(
				SHGetDataFromIDList(_folder, pidl, SHGDFIL_FINDDATA, pw32fdata, sizeof(WIN32_FIND_DATA))) )) {
		WIN32_FILE_ATTRIBUTE_DATA fad;
		IDataObject* pDataObj;

		STGMEDIUM medium = {0, {0}, 0};
		FORMATETC fmt = {g_Globals._cfStrFName, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		HRESULT hr = _folder->GetUIObjectOf(0, 1, &pidl, IID_IDataObject, 0, (LPVOID*)&pDataObj);

		if (SUCCEEDED(hr)) {
			hr = pDataObj->GetData(&fmt, &medium);

			pDataObj->Release();

			if (SUCCEEDED(hr)) {
				LPCTSTR path = (LPCTSTR)GlobalLock(medium.UNION_MEMBER(hGlobal));

				if (path) {
					 // fill with drive names "C:", ...
					assert(_tcslen(path) < GlobalSize(medium.UNION_MEMBER(hGlobal)));
					_tcscpy(pw32fdata->cFileName, path);

					UINT sem_org = SetErrorMode(SEM_FAILCRITICALERRORS);

					if (GetFileAttributesEx(path, GetFileExInfoStandard, &fad)) {
						pw32fdata->dwFileAttributes = fad.dwFileAttributes;
						pw32fdata->ftCreationTime = fad.ftCreationTime;
						pw32fdata->ftLastAccessTime = fad.ftLastAccessTime;
						pw32fdata->ftLastWriteTime = fad.ftLastWriteTime;

						if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
							 // copy file size
							pw32fdata->nFileSizeLow = fad.nFileSizeLow;
							pw32fdata->nFileSizeHigh = fad.nFileSizeHigh;
						} else {
							 // ignore FILE_ATTRIBUTE_HIDDEN attribute of NTFS drives - this would hide those drives in ShellBrowser
							if (pw32fdata->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
								if (path[1]==':' && path[2]=='\\' && !path[3])	// Is it a drive path?
									pw32fdata->dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
							}
						}
					}

					HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
												0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

					if (hFile != INVALID_HANDLE_VALUE) {
						if (GetFileInformationByHandle(hFile, pbhfi))
							bhfi_valid = true;

						CloseHandle(hFile);
					}

					SetErrorMode(sem_org);

					GlobalUnlock(medium.UNION_MEMBER(hGlobal));
					GlobalFree(medium.UNION_MEMBER(hGlobal));
				}
			}
		}
	}

	if (!do_access || !(attribs&SFGAO_FILESYSTEM))	// Archiv files should not be displayed as folders in explorer view.
		if (attribs & (SFGAO_FOLDER|SFGAO_HASSUBFOLDER))
			pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (attribs & SFGAO_READONLY)
		pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

	if (attribs & SFGAO_COMPRESSED)
		pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;

	return bhfi_valid;
}


ShellPath ShellEntry::create_absolute_pidl() const
{
	CONTEXT("ShellEntry::create_absolute_pidl()");

	if (_up)
	{
		if (_up->_etype == ET_SHELL) {
			ShellDirectory* dir = static_cast<ShellDirectory*>(_up);

			if (dir->_pidl->mkid.cb)	// Caching of absolute PIDLs could enhance performance.
				return _pidl.create_absolute_pidl(dir->create_absolute_pidl());
		} else
			return _pidl.create_absolute_pidl(_up->create_absolute_pidl());
	}
	return _pidl;
}


 // get full path of a shell entry
bool ShellEntry::get_path(PTSTR path, size_t path_count) const
{
	if (!path || path_count==0)
		return false;
/*
	path[0] = TEXT('\0');

	if (FAILED(path_from_pidl(get_parent_folder(), &*_pidl, path, path_count)))
		return false;
*/
	FileSysShellPath fs_path(create_absolute_pidl());
	LPCTSTR ret = fs_path;

	if (ret) {
		lstrcpyn(path, ret, path_count);
		return true;
	} else
		return false;
}


 // get full path of a shell folder
bool ShellDirectory::get_path(PTSTR path, size_t path_count) const
{
	CONTEXT("ShellDirectory::get_path()");

	if (!path || path_count==0)
		return false;

	path[0] = TEXT('\0');

	if (_folder.empty())
		return false;

	SFGAOF attribs = SFGAO_FILESYSTEM;

	if (FAILED(const_cast<ShellFolder&>(_folder)->GetAttributesOf(1, (LPCITEMIDLIST*)&_pidl, &attribs)))
		return false;

	if (!(attribs & SFGAO_FILESYSTEM))
		return false;

	if (FAILED(path_from_pidl(get_parent_folder(), &*_pidl, path, path_count)))
		return false;

	return true;
}


BOOL ShellEntry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	CONTEXT("ShellEntry::launch_entry()");

	SHELLEXECUTEINFO shexinfo;

	shexinfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shexinfo.fMask = SEE_MASK_INVOKEIDLIST;	// SEE_MASK_IDLIST is also possible.
	shexinfo.hwnd = hwnd;
	shexinfo.lpVerb = NULL;
	shexinfo.lpFile = NULL;
	shexinfo.lpParameters = NULL;
	shexinfo.lpDirectory = NULL;
	shexinfo.nShow = nCmdShow;

	ShellPath shell_path = create_absolute_pidl();
	shexinfo.lpIDList = &*shell_path;

	 // add PIDL to the recent file list
	SHAddToRecentDocs(SHARD_PIDL, shexinfo.lpIDList);

	BOOL ret = TRUE;

	if (!ShellExecuteEx(&shexinfo)) {
		display_error(hwnd, GetLastError());
		ret = FALSE;
	}

	return ret;
}


HRESULT ShellEntry::do_context_menu(HWND hwnd, LPPOINT pptScreen, CtxMenuInterfaces& cm_ifs)
{
	ShellDirectory* dir = static_cast<ShellDirectory*>(_up);

	ShellFolder folder = dir? dir->_folder: GetDesktopFolder();
	LPCITEMIDLIST pidl = _pidl;

	return ShellFolderContextMenu(folder, hwnd, 1, &pidl, pptScreen->x, pptScreen->y, cm_ifs);
}


HRESULT ShellEntry::GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut)
{
	LPCITEMIDLIST pidl = _pidl;

	return get_parent_folder()->GetUIObjectOf(hWnd, 1, &pidl, riid, NULL, ppvOut);
}


ShellFolder Entry::get_shell_folder() const
{
	return ShellFolder(create_absolute_pidl());
}

ShellFolder ShellEntry::get_shell_folder() const
{
	return get_parent_folder();
}

ShellFolder ShellDirectory::get_shell_folder() const
{
	return _folder;
}


void ShellDirectory::read_directory(int scan_flags)
{
	CONTEXT("ShellDirectory::read_directory()");

	int level = _level + 1;

	Entry* first_entry = NULL;
	Entry* last = NULL;

	/*if (_folder.empty())
		return;*/

#ifndef _NO_WIN_FS
	TCHAR buffer[_MAX_PATH+_MAX_FNAME];

	if (!(scan_flags&SCAN_NO_FILESYSTEM) && get_path(buffer, COUNTOF(buffer)) && _tcsncmp(buffer,TEXT("::{"),3)) {
		Entry* entry = NULL;	// eliminate useless GCC warning by initializing entry

		LPTSTR p = buffer + _tcslen(buffer);

		lstrcpy(p, TEXT("\\*"));

		WIN32_FIND_DATA w32fd;
		HANDLE hFind = FindFirstFile(buffer, &w32fd);

		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				 // ignore hidden files (usefull in the start menu)
				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
					continue;

				 // ignore directory entries "." and ".."
				if ((w32fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) &&
					w32fd.cFileName[0]==TEXT('.') &&
					(w32fd.cFileName[1]==TEXT('\0') ||
					(w32fd.cFileName[1]==TEXT('.') && w32fd.cFileName[2]==TEXT('\0'))))
					continue;

				lstrcpy(p+1, w32fd.cFileName);

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					entry = new WinDirectory(this, buffer);
				else
					entry = new WinEntry(this);

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->_next = entry;

				memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

				entry->_level = level;

				if (!(scan_flags & SCAN_DONT_ACCESS)) {
					HANDLE hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
												0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

					if (hFile != INVALID_HANDLE_VALUE) {
						if (GetFileInformationByHandle(hFile, &entry->_bhfi))
							entry->_bhfi_valid = true;

						if (ScanNTFSStreams(entry, hFile))
							entry->_scanned = true;	// There exist named NTFS sub-streams in this file.

						CloseHandle(hFile);
					}
				}

				 // set file type name
				LPCTSTR ext = g_Globals._ftype_mgr.set_type(entry);

				DWORD attribs = SFGAO_FILESYSTEM;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					attribs |= SFGAO_FOLDER|SFGAO_HASSUBFOLDER;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					attribs |= SFGAO_READONLY;

				//if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				//	attribs |= SFGAO_HIDDEN;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
					attribs |= SFGAO_COMPRESSED;

				if (ext && !_tcsicmp(ext, _T(".lnk"))) {
					attribs |= SFGAO_LINK;
					w32fd.dwFileAttributes |= ATTRIBUTE_SYMBOLIC_LINK;
				}

				entry->_shell_attribs = attribs;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					entry->_icon_id = ICID_FOLDER;
				else if (!(scan_flags & SCAN_DONT_EXTRACT_ICONS))
					entry->_icon_id = entry->safe_extract_icon();	// Assume small icon, we can extract the large icon later on demand.

				last = entry;
			} while(FindNextFile(hFind, &w32fd));

			FindClose(hFind);
		}
	}
	else // SCAN_NO_FILESYSTEM
#endif
	{
		ShellItemEnumerator enumerator(_folder, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN|SHCONTF_SHAREABLE|SHCONTF_STORAGE);

		TCHAR name[MAX_PATH];
		TCHAR path[MAX_PATH];
		HRESULT hr_next = S_OK;

		do {
#define FETCH_ITEM_COUNT	32
			LPITEMIDLIST pidls[FETCH_ITEM_COUNT];
			ULONG cnt = 0;

			memset(pidls, 0, sizeof(pidls));

			hr_next = enumerator->Next(FETCH_ITEM_COUNT, pidls, &cnt);

			/* don't break yet now: Registry Explorer Plugin returns E_FAIL!
			if (!SUCCEEDED(hr_next))
				break; */

			if (hr_next == S_FALSE)
				break;

			for(ULONG n=0; n<cnt; ++n) {
				WIN32_FIND_DATA w32fd;
				BY_HANDLE_FILE_INFORMATION bhfi;
				bool bhfi_valid = false;

				memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

				SFGAOF attribs_before = ~SFGAO_READONLY & ~SFGAO_VALIDATE;
				SFGAOF attribs = attribs_before;
				HRESULT hr = _folder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidls[n], &attribs);
				bool removeable = false;

				if (SUCCEEDED(hr) && attribs!=attribs_before) {
					 // avoid accessing floppy drives when browsing "My Computer"
					if (attribs & SFGAO_REMOVABLE) {
						attribs |= SFGAO_HASSUBFOLDER;
						removeable = true;
					} else if (!(scan_flags & SCAN_DONT_ACCESS)) {
						SFGAOF attribs2 = SFGAO_READONLY;

						HRESULT hr = _folder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidls[n], &attribs2);

						if (SUCCEEDED(hr))
							attribs |= attribs2;
					}
				} else
					attribs = 0;

				bhfi_valid = fill_w32fdata_shell(pidls[n], attribs, &w32fd, &bhfi,
												 !(scan_flags&SCAN_DONT_ACCESS) && !removeable);

				try {
					Entry* entry = NULL;	// eliminate useless GCC warning by initializing entry

					if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						entry = new ShellDirectory(this, pidls[n], _hwnd);
					else
						entry = new ShellEntry(this, pidls[n]);

					if (!first_entry)
						first_entry = entry;

					if (last)
						last->_next = entry;

					memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

					if (bhfi_valid)
						memcpy(&entry->_bhfi, &bhfi, sizeof(BY_HANDLE_FILE_INFORMATION));

					 // store path in entry->_data.cFileName in case fill_w32fdata_shell() didn't already fill it
					if (!entry->_data.cFileName[0])
						if (SUCCEEDED(path_from_pidl(_folder, pidls[n], path, COUNTOF(path))))
							_tcscpy(entry->_data.cFileName, path);

					if (SUCCEEDED(name_from_pidl(_folder, pidls[n], name, COUNTOF(name), SHGDN_INFOLDER|0x2000/*0x2000=SHGDN_INCLUDE_NONFILESYS*/))) {
						if (!entry->_data.cFileName[0])
							_tcscpy(entry->_data.cFileName, name);
						else if (_tcscmp(entry->_display_name, name))
							entry->_display_name = _tcsdup(name);	// store display name separate from file name; sort display by file name
					}

					if (attribs & SFGAO_LINK)
						w32fd.dwFileAttributes |= ATTRIBUTE_SYMBOLIC_LINK;

					entry->_level = level;
					entry->_shell_attribs = attribs;
					entry->_bhfi_valid = bhfi_valid;

					 // set file type name
					g_Globals._ftype_mgr.set_type(entry);

					 // get icons for files and virtual objects
					if (!(entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
						!(attribs & SFGAO_FILESYSTEM)) {
						if (!(scan_flags & SCAN_DONT_EXTRACT_ICONS))
							entry->_icon_id = entry->safe_extract_icon();	// Assume small icon, we can extract the large icon later on demand.
					} else if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						entry->_icon_id = ICID_FOLDER;
					else
						entry->_icon_id = ICID_NONE;	// don't try again later

					last = entry;
				} catch(COMException& e) {
					HandleException(e, _hwnd);
				}
			}
		} while(SUCCEEDED(hr_next));
	}

	if (last)
		last->_next = NULL;

	_down = first_entry;
	_scanned = true;
}

const void* ShellDirectory::get_next_path_component(const void* p) const
{
	LPITEMIDLIST pidl = (LPITEMIDLIST)p;

	if (!pidl || !pidl->mkid.cb)
		return NULL;

	 // go to next element
	pidl = (LPITEMIDLIST)((LPBYTE)pidl+pidl->mkid.cb);

	return pidl;
}

Entry* ShellDirectory::find_entry(const void* p)
{
	LPITEMIDLIST pidl = (LPITEMIDLIST) p;

	 // handle special case of empty trailing id list entry
	if (!pidl->mkid.cb)
		return this;

	for(Entry*entry=_down; entry; entry=entry->_next)
		if (entry->_etype == ET_SHELL) {
			ShellEntry* se = static_cast<ShellEntry*>(entry);

			if (se->_pidl && se->_pidl->mkid.cb==pidl->mkid.cb && !memcmp(se->_pidl, pidl, se->_pidl->mkid.cb))
				return entry;
		} else {
			const ShellPath& sp = entry->create_absolute_pidl();
			static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST)> ILFindLastID(TEXT("SHELL32"), "ILFindLastID");

			if (ILFindLastID) {
				LPCITEMIDLIST entry_pidl = (*ILFindLastID)(sp);

				if (entry_pidl && entry_pidl->mkid.cb==pidl->mkid.cb && !memcmp(entry_pidl, pidl, entry_pidl->mkid.cb))
					return entry;
			}
		}

	return NULL;
}

int ShellDirectory::extract_icons(ICONCACHE_FLAGS flags)
{
	int cnt = 0;

	for(Entry*entry=_down; entry; entry=entry->_next)
		if (entry->_icon_id == ICID_UNKNOWN) {
			entry->_icon_id = entry->extract_icon(flags);

			if (entry->_icon_id != ICID_NONE)
				++cnt;
		}

	return cnt;
}
