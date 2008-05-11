/*
 * Copyright 2003, 2004 Martin Fuchs
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
 // winfs.h
 //
 // Martin Fuchs, 23.07.2003
 //


 /// Windows file system file-entry
struct WinEntry : public Entry
{
	WinEntry(Entry* parent) : Entry(parent, ET_WINDOWS) {}

protected:
	WinEntry() : Entry(ET_WINDOWS) {}

	virtual bool get_path(PTSTR path, size_t path_count) const;
	virtual ShellPath create_absolute_pidl() const;
};


 /// Windows file system directory-entry
struct WinDirectory : public WinEntry, public Directory
{
	WinDirectory(LPCTSTR root_path)
	 :	WinEntry()
	{
		_path = _tcsdup(root_path);
	}

	WinDirectory(Entry* parent, LPCTSTR path)
	 :	WinEntry(parent)
	{
		_path = _tcsdup(path);
	}

	~WinDirectory()
	{
		free(_path);
		_path = NULL;
	}

	virtual void read_directory(int scan_flags=0);
	virtual const void* get_next_path_component(const void*) const;
	virtual Entry* find_entry(const void*);
};

extern int ScanNTFSStreams(Entry* entry, HANDLE hFile);
