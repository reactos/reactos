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
 // ntobjfs.h
 //
 // Martin Fuchs, 31.01.2004
 //


enum OBJECT_TYPE {
	DIRECTORY_OBJECT, SYMBOLICLINK_OBJECT,
	MUTANT_OBJECT, SECTION_OBJECT, EVENT_OBJECT, SEMAPHORE_OBJECT,
	TIMER_OBJECT, KEY_OBJECT, EVENTPAIR_OBJECT, IOCOMPLETITION_OBJECT,
	DEVICE_OBJECT, FILE_OBJECT, CONTROLLER_OBJECT, PROFILE_OBJECT,
	TYPE_OBJECT, DESKTOP_OBJECT, WINDOWSTATION_OBJECT, DRIVER_OBJECT,
	TOKEN_OBJECT, PROCESS_OBJECT, THREAD_OBJECT, ADAPTER_OBJECT, PORT_OBJECT,

	UNKNOWN_OBJECT_TYPE=-1
};

struct RtlAnsiString {
	WORD	string_len;
	WORD	alloc_len;
	LPSTR	string_ptr;
};

struct RtlUnicodeString {
	WORD	string_len;
	WORD	alloc_len;
	LPWSTR	string_ptr;
};

struct NtObjectInfo {
	RtlUnicodeString name;
	RtlUnicodeString type;
	BYTE	padding[16];
};

struct OpenStruct {
	DWORD	size;
	DWORD	_1;
	RtlUnicodeString* string;
	DWORD	_3;
	DWORD	_4;
	DWORD	_5;
};

struct NtObject {
	DWORD	_0;
	DWORD	_1;
	DWORD	handle_count;
	DWORD	reference_count;
	DWORD	_4;
	DWORD	_5;
	DWORD	_6;
	DWORD	_7;
	DWORD	_8;
	DWORD	_9;
	DWORD	_A;
	DWORD	_B;
	FILETIME creation_time;
};


 /// NtObj file system file-entry
struct NtObjEntry : public Entry
{
	NtObjEntry(Entry* parent, OBJECT_TYPE type) : Entry(parent, ET_NTOBJS), _type(type) {}

	OBJECT_TYPE	_type;

protected:
	NtObjEntry(OBJECT_TYPE type) : Entry(ET_NTOBJS), _type(type) {}

	virtual bool get_path(PTSTR path) const;
	virtual BOOL launch_entry(HWND hwnd, UINT nCmdShow);
};


 /// NtObj file system directory-entry
struct NtObjDirectory : public NtObjEntry, public Directory
{
	NtObjDirectory(LPCTSTR root_path)
	 :	NtObjEntry(DIRECTORY_OBJECT)
	{
		_path = _tcsdup(root_path);
	}

	NtObjDirectory(Entry* parent, LPCTSTR path)
	 :	NtObjEntry(parent, DIRECTORY_OBJECT)
	{
		_path = _tcsdup(path);
	}

	~NtObjDirectory()
	{
		free(_path);
		_path = NULL;
	}

	virtual void read_directory(int scan_flags=SCAN_ALL);
	virtual Entry* find_entry(const void*);
};
