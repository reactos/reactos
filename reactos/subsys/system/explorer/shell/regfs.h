/*
 * Copyright 2004 Martin Fuchs
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
 // regfs.h
 //
 // Martin Fuchs, 31.01.2004
 //


 /// Registry entry
struct RegEntry : public Entry
{
	RegEntry(Entry* parent) : Entry(parent, ET_REGISTRY) {}

protected:
	RegEntry() : Entry(ET_REGISTRY) {}

	virtual bool get_path(PTSTR path) const;
	virtual BOOL launch_entry(HWND hwnd, UINT nCmdShow);
};


 /// Registry key entry
struct RegDirectory : public RegEntry, public Directory
{
	RegDirectory(Entry* parent, LPCTSTR path, HKEY hKeyRoot);

	~RegDirectory()
	{
		free(_path);
		_path = NULL;
	}

	virtual void read_directory(int scan_flags=SCAN_ALL);
	virtual Entry* find_entry(const void*);

protected:
	HKEY	_hKeyRoot;
};


 /// Registry key entry
struct RegistryRoot : public RegEntry, public Directory
{
	RegistryRoot()
	{
	}

	RegistryRoot(Entry* parent, LPCTSTR path)
	 :	RegEntry(parent)
	{
		_path = _tcsdup(path);
	}

	~RegistryRoot()
	{
		free(_path);
		_path = NULL;
	}

	virtual void read_directory(int scan_flags=SCAN_ALL);
};
