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


String COMException::toString() const
{
	TCHAR msg[4*BUFFER_LEN];
	LPTSTR p = msg;

	p += _stprintf(p, TEXT("%s\nContext: %s"), super::ErrorMessage(), (LPCTSTR)_context.toString());

	if (_file)
		p += _stprintf(p, TEXT("\nLocation: %hs:%d"), _file, _line);

	return msg;
}


 /// Exception Handler for COM exceptions

void HandleException(COMException& e, HWND hwnd)
{
	String msg = e.toString();

	SetLastError(0);

	if (hwnd && !IsWindowVisible(hwnd))
		hwnd = 0;

	MessageBox(hwnd, msg, TEXT("ShellClasses COM Exception"), MB_ICONHAND|MB_OK);

	 // If displaying the error message box _with_ parent was not successfull, display it now without a parent window.
	if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
		MessageBox(0, msg, TEXT("ShellClasses COM Exception"), MB_ICONHAND|MB_OK);
}


 // common IMalloc object

CommonShellMalloc ShellMalloc::s_cmn_shell_malloc;


 // common desktop object

ShellFolder& GetDesktopFolder()
{
	static CommonDesktop s_desktop;

	 // initialize s_desktop
	s_desktop.init();

	return s_desktop;
}


void CommonDesktop::init()
{
	CONTEXT("CommonDesktop::init()");

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
	CONTEXT("path_from_pidlA()");

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
	CONTEXT("path_from_pidlW()");

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
	CONTEXT("name_from_pidl()");

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
	CONTEXT("ShellFolder::ShellFolder()");

	IShellFolder* desktop;

	CHECKERROR(SHGetDesktopFolder(&desktop));

	super::Attach(desktop);
	desktop->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* p)
 :	super(p)
{
	CONTEXT("ShellFolder::ShellFolder(IShellFolder*)");

	p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::ShellFolder(IShellFolder*, LPCITEMIDLIST)");

	IShellFolder* ptr;

	if (!pidl)
		CHECKERROR(E_INVALIDARG);

	if (pidl && pidl->mkid.cb)
		CHECKERROR(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

ShellFolder::ShellFolder(LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::ShellFolder(LPCITEMIDLIST)");

	IShellFolder* ptr;
	IShellFolder* parent = GetDesktopFolder();

	if (pidl && pidl->mkid.cb)
		CHECKERROR(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

void ShellFolder::attach(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::attach(IShellFolder*, LPCITEMIDLIST)");

	IShellFolder* ptr;

	if (pidl && pidl->mkid.cb)
		CHECKERROR(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&ptr));
	else
		ptr = parent;

	super::Attach(ptr);
	ptr->AddRef();
}

#else // _com_ptr not available -> use SIfacePtr

ShellFolder::ShellFolder()
{
	CONTEXT("ShellFolder::ShellFolder()");

	CHECKERROR(SHGetDesktopFolder(&_p));

	_p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* p)
 :	super(p)
{
	CONTEXT("ShellFolder::ShellFolder(IShellFolder*)");

	_p->AddRef();
}

ShellFolder::ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::ShellFolder(IShellFolder*, LPCITEMIDLIST)");

	if (pidl && pidl->mkid.cb)
		CHECKERROR(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));
	else
		_p = GetDesktopFolder();

	_p->AddRef();
}

ShellFolder::ShellFolder(LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::ShellFolder(LPCITEMIDLIST)");

	if (pidl && pidl->mkid.cb)
		CHECKERROR(GetDesktopFolder()->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));
	else
		_p = GetDesktopFolder();

	_p->AddRef();
}

void ShellFolder::attach(IShellFolder* parent, LPCITEMIDLIST pidl)
{
	CONTEXT("ShellFolder::ShellFolder(IShellFolder*, LPCITEMIDLIST)");

	IShellFolder* h = _p;

	CHECKERROR(parent->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID*)&_p));

	_p->AddRef();
	h->Release();
}

#endif

String ShellFolder::get_name(LPCITEMIDLIST pidl, SHGDNF flags) const
{
	CONTEXT("ShellFolder::get_name()");

	TCHAR buffer[MAX_PATH];
	StrRet strret;

	HRESULT hr = ((IShellFolder*)*const_cast<ShellFolder*>(this))->GetDisplayNameOf(pidl, flags, &strret);

	if (hr == S_OK)
		strret.GetString(pidl->mkid, buffer, MAX_PATH);
	else {
		CHECKERROR(hr);
		*buffer = TEXT('\0');
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
	CONTEXT("ShellPath::GetUIObjectOf()");

	ShellPath parent, obj;

	split(parent, obj);

	LPCITEMIDLIST idl = obj;

	if (parent && parent->mkid.cb)
		 // use the IShellFolder of the parent
		CHECKERROR(ShellFolder((IShellFolder*)sf,parent)->GetUIObjectOf(hWnd, 1, &idl, riid, 0, ppvOut));
	else // else use desktop folder
		CHECKERROR(sf->GetUIObjectOf(hWnd, 1, &idl, riid, 0, ppvOut));
}

#ifndef __MINGW32__	// ILCombine() is currently missing in MinGW.

 // convert an item id list from relative to absolute (=relative to the desktop) format
ShellPath ShellPath::create_absolute_pidl(LPCITEMIDLIST parent_pidl) const
{
	CONTEXT("ShellPath::create_absolute_pidl()");

	return ILCombine(parent_pidl, _p);

/* seems to work only for NT upwards
	 // create a new item id list with _p append behind parent_pidl
	int l1 = ILGetSize(parent_pidl) - sizeof(USHORT/ SHITEMID::cb /);
	int l2 = ILGetSize(_p);

	LPITEMIDLIST p = (LPITEMIDLIST) _malloc->Alloc(l1+l2);

	memcpy(p, parent_pidl, l1);
	memcpy((LPBYTE)p+l1, _p, l2);

	return p;
*/
}

#else

ShellPath ShellPath::create_absolute_pidl(LPCITEMIDLIST parent_pidl) const
{
	CONTEXT("ShellPath::create_absolute_pidl()");

	static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST, LPCITEMIDLIST)> ILCombine(TEXT("SHELL32"), 25);

	if (ILCombine)
		return (*ILCombine)(parent_pidl, _p);

	 // create a new item id list with _p append behind parent_pidl
	int l1 = ILGetSize(parent_pidl) - sizeof(USHORT/*SHITEMID::cb*/);
	int l2 = ILGetSize(_p);

	LPITEMIDLIST p = (LPITEMIDLIST) _malloc->Alloc(l1+l2);

	memcpy(p, parent_pidl, l1);
	memcpy((LPBYTE)p+l1, _p, l2);

	return p;
}

UINT ILGetSize(LPCITEMIDLIST pidl)
{
	if (!pidl)
		return 0;

	int l = sizeof(USHORT/*SHITEMID::cb*/);

	while(pidl->mkid.cb) {
		l += pidl->mkid.cb;
		pidl = LPCITEMIDLIST((LPBYTE)pidl+pidl->mkid.cb);
	}

	return l;
}

#endif


#ifndef	_SHFOLDER_H_
#define	CSIDL_FLAG_CREATE	0x8000
#endif

 /// file system path of special folder
SpecialFolderFSPath::SpecialFolderFSPath(int folder, HWND hwnd)
{
	_fullpath[0] = '\0';

#ifdef UNICODE
	static DynamicFct<BOOL (__stdcall*)(HWND hwnd, LPTSTR pszPath, int csidl, BOOL fCreate)> s_pSHGetSpecialFolderPath(TEXT("shell32"), "SHGetSpecialFolderPathW");
#else
	static DynamicFct<BOOL (__stdcall*)(HWND hwnd, LPTSTR pszPath, int csidl, BOOL fCreate)> s_pSHGetSpecialFolderPath(TEXT("shell32"), "SHGetSpecialFolderPathA");
#endif
	if (*s_pSHGetSpecialFolderPath)
		(*s_pSHGetSpecialFolderPath)(hwnd, _fullpath, folder, TRUE);
	else {
		 // SHGetSpecialFolderPath() is not compatible to WIN95/NT4
#ifdef UNICODE
		static DynamicFct<HRESULT (__stdcall*)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)> s_pSHGetFolderPath_shell32(TEXT("shell32"), "SHGetFolderPathW");
#else
		static DynamicFct<HRESULT (__stdcall*)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)> s_pSHGetFolderPath_shell32(TEXT("shell32"), "SHGetFolderPathA");
#endif
		if (*s_pSHGetFolderPath_shell32)
			(*s_pSHGetFolderPath_shell32)(hwnd, folder|CSIDL_FLAG_CREATE, 0, 0, _fullpath);
		else {
			 // SHGetFolderPath() is only present in shfolder.dll on some platforms.
#ifdef UNICODE
			static DynamicLoadLibFct<HRESULT (__stdcall*)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)> s_pSHGetFolderPath_shfolder(TEXT("shfolder"), "SHGetFolderPathW");
#else
			static DynamicLoadLibFct<HRESULT (__stdcall*)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)> s_pSHGetFolderPath_shfolder(TEXT("shfolder"), "SHGetFolderPathA");
#endif
			if (*s_pSHGetFolderPath_shfolder)
				(*s_pSHGetFolderPath_shfolder)(hwnd, folder|CSIDL_FLAG_CREATE, 0, 0, _fullpath);
		}
	}
}


HRESULT ShellFolderContextMenu(IShellFolder* shell_folder, HWND hwndParent, int cidl, LPCITEMIDLIST* apidl, int x, int y)
{
	IContextMenu* pcm;

	HRESULT hr = shell_folder->GetUIObjectOf(hwndParent, cidl, apidl, IID_IContextMenu, NULL, (LPVOID*)&pcm);
//	HRESULT hr = CDefFolderMenu_Create2(dir?dir->_pidl:DesktopFolder(), hwndParent, 1, &pidl, shell_folder, NULL, 0, NULL, &pcm);

	if (SUCCEEDED(hr)) {
		HMENU hmenu = CreatePopupMenu();

		if (hmenu) {
			hr = pcm->QueryContextMenu(hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, CMF_NORMAL|CMF_EXPLORE);

			if (SUCCEEDED(hr)) {
				UINT idCmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON, x, y, 0, hwndParent, NULL);

				if (idCmd) {
				  CMINVOKECOMMANDINFO cmi;

				  cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
				  cmi.fMask = 0;
				  cmi.hwnd = hwndParent;
				  cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - FCIDM_SHVIEWFIRST);
				  cmi.lpParameters = NULL;
				  cmi.lpDirectory = NULL;
				  cmi.nShow = SW_SHOWNORMAL;
				  cmi.dwHotKey = 0;
				  cmi.hIcon = 0;

				  hr = pcm->InvokeCommand(&cmi);
				}
			}
		}

		pcm->Release();
	}

	return hr;
}
