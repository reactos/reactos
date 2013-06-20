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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // unixfs.h
 //
 // Martin Fuchs, 23.07.2003
 //


#ifdef __WINE__

struct UnixEntry : public Entry
{
	UnixEntry(Entry* parent) : Entry(parent) {}

protected:
	UnixEntry() : Entry(ET_UNIX) {}

	virtual bool get_path(PTSTR path, size_t path_count) const;
};

struct UnixDirectory : public UnixEntry, public Directory
{
	UnixDirectory(LPCTSTR root_path)
	 :	UnixEntry()
	{
		_path = _tcsdup(root_path);
	}

	UnixDirectory(UnixDirectory* parent, LPCTSTR path)
	 :	UnixEntry(parent)
	{
		_path = _tcsdup(path);
	}

	~UnixDirectory()
	{
		free(_path);
		_path = NULL;
	}

	virtual void read_directory();
	virtual const void* get_next_path_component(const void*);
	virtual Entry* find_entry(const void*);
};

#endif
