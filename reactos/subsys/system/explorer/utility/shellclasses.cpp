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
 // shellclasses.cpp
 //
 // C++ wrapper classes for COM interfaces and shell objects
 //
 // Martin Fuchs, 20.07.2003
 //


#include "utility.h"
#include "shellclasses.h"


#ifdef _MS_VER
#pragma comment(lib, "shell32")	// link to shell32.dll
#endif


 // helper functions for string copying

LPSTR strcpyn(LPSTR dest, LPCSTR source, size_t count)
{
	LPCSTR s;
	LPSTR d = dest;

	for(s=source; count&&(*d++=*s++); )
		count--;

	return dest;
}

LPWSTR wcscpyn(LPWSTR dest, LPCWSTR source, size_t count)
{
	LPCWSTR s;
	LPWSTR d = dest;

	for(s=source; count&&(*d++=*s++); )
		count--;

	return dest;
}


 // Exception Handler for COM exceptions

void HandleException(COMException& e, HWND hwnd)
{
	SetLastError(0);

	MessageBox(hwnd, e.ErrorMessage(), TEXT("ShellClasses COM Exception"), MB_ICONHAND|MB_OK);

	if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
		MessageBox(0, e.ErrorMessage(), TEXT("ShellClasses COM Exception"), MB_ICONHAND|MB_OK);
}


 // common IMalloc object

CommonShellMalloc ShellMalloc::s_cmn_shell_malloc;


 // common desktop object

ShellFolder& Desktop()
{
	static CommonDesktop s_desktop;

	 // initialize s_desktop
	s_desktop.init();

	return s_desktop;
}


void CommonDesktop::init()
{
	if (!_desktop)
		_desktop = new ShellFolder;
}

CommonDesktop::~CommonDesktop()
{
	if (_desktop)
		delete _desktop;
}


HRESULT path_from_pidlA(IShellFolder* folder, LPCITEMIDLIST pidl, LPSTR buffer, int len)
{
	StrRetA str;

	HRESULT hr = folder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);

	if (SUCCEEDED(hr))
		str.GetString(pidl->mkid, buffer, len);
	else
		buffer[0] = '\0';

	return hr;
}

HRESULT path_from_pidlW(IShellFolder* folder, LPCITEMIDLIST pidl, LPWSTR buffer, int len)
{
	StrRetW str;

	HRESULT hr = folder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);

	if (SUCCEEDED(hr))
		str.GetString(pidl->mkid, buffer, len);
	else
		buffer[0] = '\0';

	return hr;
}

HRESULT name_from_pidl(IShellFolder* folder, LPCITEMIDLIST pidl, LPTSTR buffer, int len, SHGDNF flags)
{
	StrRet str;

	HRESULT hr = folder->GetDisplayNameOf(pidl, flags, &str);

	if (SUCCEEDED(hr))
		str.GetString(pidl->mkid, buffer, len);
	else
		buffer[0] = '\0';

	return hr;
}


#ifndef _NO_COMUTIL

ShellFolder::ShellFolder()
{
	IShellFolder* desktop;

	CheckError(SHGetDesktopFolder(&desktop));

	super::Attach(desktop);
	desktop->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* p)
 :	IShellFolderPtr(p)
{
	p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	IShellFolder* ptr;

	if (!pidl)
		CheckError(E_INVALIDARG);

	if (pidl && pidl->mkid.cb)
		CheckError(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

ShellFolder::ShellFolder(LPCITEMIDLIST pidl)
{
	IShellFolder* ptr;
	IShellFolder* parent = Desktop();

	if (pidl && pidl->mkid.cb)
		CheckError(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

void ShellFolder::attach(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	IShellFolder* ptr;

	if (pidl && pidl->mkid.cb)
		CheckError(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

#else // _com_ptr not available -> use SIfacePtr

ShellFolder::ShellFolder()
{
	CheckError(SHGetDesktopFolder(&_p));

	_p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* p)
 :	SIfacePtr<IShellFolder>(p)
{
	_p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	if (pidl && pidl->mkid.cb)
		CheckError(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));
	else
		_p = Desktop();

	_p->AddRef();
}

ShellFolder::ShellFolder(LPCITEMIDLIST pidl)
{
	if (pidl && pidl->mkid.cb)
		CheckError(Desktop()->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));
	else
		_p = Desktop();

	_p->AddRef();
}

void ShellFolder::attach(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	IShellFolder* h = _p;

	CheckError(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));

	_p->AddRef();
	h->Release();
}

#endif

String ShellFolder::get_name(LPCITEMIDLIST pidl, SHGDNF flags) const
{
	TCHAR buffer[MAX_PATH];
	StrRet strret;

	HRESULT hr = ((IShellFolder*)*const_cast<ShellFolder*>(this))->GetDisplayNameOf(pidl, flags, &strret);

	if (hr == S_OK)
		strret.GetString(pidl->mkid, buffer, MAX_PATH);
	else {
		CheckError(hr);
		*buffer = _T('\0');
	}

	return buffer;
}


void ShellPath::split(ShellPath& parent, ShellPath& obj) const
{
	SHITEMID *piid, *piidLast;
	int size = 0;

	 // find last item-id and calculate total size of pidl
	for(piid=piidLast=&_p->mkid; piid->cb; ) {
		piidLast = piid;
		size += (piid->cb);
		piid = (SHITEMID*)((LPBYTE)piid + (piid->cb));
	}

	 // copy parent folder portion
	size -= piidLast->cb;  // don't count "object" item-id

	if (size > 0)
		parent.assign(_p, size);

	 // copy "object" portion
	obj.assign((ITEMIDLIST*)piidLast, piidLast->cb);
}

void ShellPath::GetUIObjectOf(REFIID riid, LPVOID* ppvOut, HWND hWnd, ShellFolder& sf)
{
	ShellPath parent, obj;

	split(parent, obj);

	LPCITEMIDLIST idl = obj;

	if (parent && parent->mkid.cb)
		 // use the IShellFolder of the parent
		CheckError(ShellFolder((IShellFolder*)sf,parent)->GetUIObjectOf(hWnd, 1, &idl, riid, 0, ppvOut));
	else // else use desktop folder
		CheckError(sf->GetUIObjectOf(hWnd, 1, &idl, riid, 0, ppvOut));
}

#ifndef __MINGW32__	// ILCombine() is currently missing in MinGW.

 // convert an item id list from relative to absolute (=relative to the desktop) format
LPITEMIDLIST ShellPath::create_absolute_pidl(LPCITEMIDLIST parent_pidl, HWND hwnd) const
{
	return ILCombine(parent_pidl, _p);

/* seems to work only for NT upwards
	 // create a new item id list with _p append behind parent_pidl
	int l1 = _malloc->GetSize((void*)parent_pidl) - sizeof(USHORT/ SHITEMID::cb /);
	int l2 = _malloc->GetSize(_p);

	LPITEMIDLIST p = (LPITEMIDLIST) _malloc->Alloc(l1+l2);

	memcpy(p, parent_pidl, l1);
	memcpy((LPBYTE)p+l1, _p, l2);

	return p;
*/
}

#else

LPITEMIDLIST ShellPath::create_absolute_pidl(LPCITEMIDLIST parent_pidl, HWND hwnd) const
{
	static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST, LPCITEMIDLIST)> ILCombine(_T("SHELL32"), 25);

	if (ILCombine)
		return (*ILCombine)(parent_pidl, _p);

	 // create a new item id list with _p append behind parent_pidl
	int l1 = _malloc->GetSize((void*)parent_pidl) - sizeof(USHORT/*SHITEMID::cb*/);
	int l2 = _malloc->GetSize(_p);

	LPITEMIDLIST p = (LPITEMIDLIST) _malloc->Alloc(l1+l2);

	memcpy(p, parent_pidl, l1);
	memcpy((LPBYTE)p+l1, _p, l2);

	return p;
}

#endif
