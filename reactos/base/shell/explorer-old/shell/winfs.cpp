/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // winfs.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include <precomp.h>

#ifndef _NO_WIN_FS

//#include "winfs.h"


#ifdef BACKUP_READ_IMPLEMENTED
int ScanNTFSStreams(Entry* entry, HANDLE hFile)
{
	PVOID ctx = 0;
	DWORD read, seek_high;
	Entry** pnext = &entry->_down;
	int cnt = 0;

	for(;;) {
		struct NTFS_StreamHdr : public WIN32_STREAM_ID {
			WCHAR name_padding[_MAX_FNAME];	// room for reading stream name
		} hdr;

		if (!BackupRead(hFile, (LPBYTE)&hdr, (LPBYTE)&hdr.cStreamName-(LPBYTE)&hdr, &read, FALSE, FALSE, &ctx) ||
			(long)read!=(LPBYTE)&hdr.cStreamName-(LPBYTE)&hdr)
			break;

		if (hdr.dwStreamId == BACKUP_ALTERNATE_DATA) {
			if (hdr.dwStreamNameSize &&
				BackupRead(hFile, (LPBYTE)hdr.cStreamName, hdr.dwStreamNameSize, &read, FALSE, FALSE, &ctx) &&
				read==hdr.dwStreamNameSize)
			{
				++cnt;

				int l = hdr.dwStreamNameSize / sizeof(WCHAR);
				LPCWSTR p = hdr.cStreamName;
				LPCWSTR e = hdr.cStreamName + l;

				if (l>0 && *p==':') {
					++p, --l;

					e = p;

					while(l>0 && *e!=':')
						++e, --l;

					l = e - p;
				}

				Entry* stream_entry = new WinEntry(entry);

				memcpy(&stream_entry->_data, &entry->_data, sizeof(WIN32_FIND_DATA));
				lstrcpy(stream_entry->_data.cFileName, String(p, l));

				stream_entry->_down = NULL;
				stream_entry->_expanded = false;
				stream_entry->_scanned = false;
				stream_entry->_level = entry->_level + 1;

				*pnext = stream_entry;
				pnext = &stream_entry->_next;
			}
		}

		 // jump to the next stream header
		if (!BackupSeek(hFile, ~0, ~0, &read, &seek_high, &ctx)) {
			DWORD error = GetLastError();

			if (error != ERROR_SEEK) {
				BackupRead(hFile, 0, 0, &read, TRUE, FALSE, &ctx);	// terminate BackupRead() loop
				THROW_EXCEPTION(error);
				//break;
			}

			hdr.Size.QuadPart -= read;
			hdr.Size.HighPart -= seek_high;

			BYTE buffer[4096];

			while(hdr.Size.QuadPart > 0) {
				if (!BackupRead(hFile, buffer, sizeof(buffer), &read, FALSE, FALSE, &ctx) || read!=sizeof(buffer))
					break;

				hdr.Size.QuadPart -= read;
			}
		}
	}

	if (ctx)
		if (!BackupRead(hFile, 0, 0, &read, TRUE, FALSE, &ctx))	// terminate BackupRead() loop
			THROW_EXCEPTION(GetLastError());

	return cnt;
}
#endif


void WinDirectory::read_directory(int scan_flags)
{
	CONTEXT("WinDirectory::read_directory()");

	int level = _level + 1;

	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;

	LPCTSTR path = (LPCTSTR)_path;
	TCHAR buffer[MAX_PATH], *pname;
	for(pname=buffer; *path; )
		*pname++ = *path++;

	lstrcpy(pname, TEXT("\\*"));

	WIN32_FIND_DATA w32fd;
	HANDLE hFind = FindFirstFile(buffer, &w32fd);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			lstrcpy(pname+1, w32fd.cFileName);

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

			 // display file type names, but don't hide file extensions
			g_Globals._ftype_mgr.set_type(entry, true);

			if (!(scan_flags & SCAN_DONT_ACCESS)) {
				HANDLE hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
											0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

				if (hFile != INVALID_HANDLE_VALUE) {
					if (GetFileInformationByHandle(hFile, &entry->_bhfi))
						entry->_bhfi_valid = true;

#ifdef BACKUP_READ_IMPLEMENTED
					if (ScanNTFSStreams(entry, hFile))
						entry->_scanned = true;	// There exist named NTFS sub-streams in this file.
#endif

					CloseHandle(hFile);
				}
			}

			last = entry;	// There is always at least one entry, because FindFirstFile() succeeded and we don't filter the file entries.
		} while(FindNextFile(hFind, &w32fd));

		if (last)
			last->_next = NULL;

		FindClose(hFind);
	}

	_down = first_entry;
	_scanned = true;
}


const void* WinDirectory::get_next_path_component(const void* p) const
{
	LPCTSTR s = (LPCTSTR) p;

	while(*s && *s!=TEXT('\\') && *s!=TEXT('/'))
		++s;

	while(*s==TEXT('\\') || *s==TEXT('/'))
		++s;

	if (!*s)
		return NULL;

	return s;
}


Entry* WinDirectory::find_entry(const void* p)
{
	LPCTSTR name = (LPCTSTR)p;

	for(Entry*entry=_down; entry; entry=entry->_next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->_data.cFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->_data.cAlternateFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return NULL;
}


 // get full path of specified directory entry
bool WinEntry::get_path(PTSTR path, size_t path_count) const
{
	return get_path_base(path, path_count, ET_WINDOWS);
}

ShellPath WinEntry::create_absolute_pidl() const
{
	CONTEXT("WinEntry::create_absolute_pidl()");

	TCHAR path[MAX_PATH];

	if (get_path(path, COUNTOF(path)))
		return ShellPath(path);

	return ShellPath();
}

#endif // _NO_WIN_FS
