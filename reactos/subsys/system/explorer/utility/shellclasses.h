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
 // shellclasses.h
 //
 // C++ wrapper classes for COM interfaces and shell objects
 //
 // Martin Fuchs, 20.07.2003
 //


 // windows shell headers
#include <shellapi.h>
#include <shlobj.h>

/*@@
#if _MSC_VER>=1300	// VS.Net
#include <comdefsp.h>
using namespace _com_util;
#endif
*/

#ifndef _INC_COMUTIL	// is comutil.h of MS headers not available?
#ifndef _NO_COMUTIL
#define	_NO_COMUTIL
#endif
#endif


 // COM Exception Handling

#ifndef _NO_COMUTIL

#define	COMException _com_error

#else

struct COMException
{
	COMException(HRESULT hr)
	 :	_hr(hr)
	{
	}

	LPCTSTR ErrorMessage() const
	{
		if (_msg.empty()) {
			LPTSTR pBuf;

			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
				0, _hr, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)&pBuf, 0, NULL)) {
				_msg = pBuf;
				LocalFree(pBuf);
			 } else {
				TCHAR buffer[128];
				_stprintf(buffer, _T("unknown COM Exception: 0x%08X"), _hr);
				_msg = buffer;
			 }
		}

		return _msg;
	}

protected:
	HRESULT	_hr;
	mutable String _msg;
};

inline void CheckError(HRESULT hr)
{
	if (FAILED(hr)) {
		throw COMException(hr);
	}
}

#endif


 // COM Initialisation

struct ComInit
{
	ComInit()
	{
		CheckError(CoInitialize(0));
	}

#if (_WIN32_WINNT>=0x0400) || defined(_WIN32_DCOM)
	ComInit(DWORD flag)
	{
		CheckError(CoInitializeEx(0, flag));
	}
#endif

	~ComInit()
	{
		CoUninitialize();
	}
};


 // OLE initialisation for drag drop support

struct OleInit
{
	OleInit()
	{
		CheckError(OleInitialize(0));
	}

	~OleInit()
	{
		OleUninitialize();
	}
};


 // Exception Handler for COM exceptions

extern void HandleException(COMException& e, HWND hwnd);


 // We use a common IMalloc object for all shell memory allocations.

struct CommonShellMalloc
{
	CommonShellMalloc()
	{
		_p = 0;
	}

	void init()
	{
		if (!_p)
			CheckError(SHGetMalloc(&_p));
	}

	~CommonShellMalloc()
	{
		if (_p)
			_p->Release();
	}

	operator IMalloc*()
	{
		return _p;
	}

	IMalloc* _p;
};


 // wrapper class for IMalloc with usage of common allocator

struct ShellMalloc
{
	ShellMalloc()
	{
		 // initialize s_cmn_shell_malloc
		s_cmn_shell_malloc.init();
	}

	IMalloc* operator->()
	{
		return s_cmn_shell_malloc;
	}

	static CommonShellMalloc s_cmn_shell_malloc;
};


 // wrapper template class for pointers to shell objects managed by IMalloc

template<typename T> struct SShellPtr
{
	~SShellPtr()
	{
		_malloc->Free(_p);
	}

	T* operator->()
	{
		return _p;
	}

	T const* operator->() const
	{
		return _p;
	}

	operator T const *() const
	{
		return _p;
	}

	const T& operator*() const
	{
		return *_p;
	}

	T& operator*()
	{
		return *_p;
	}

protected:
	SShellPtr()
	 :	_p(0)
	{
	}

	SShellPtr(T* p)
	 :	_p(p)
	{
	}

	void Free()
	{
		_malloc->Free(_p);
		_p = 0;
	}

	T* _p;
	mutable ShellMalloc _malloc;	// IMalloc memory management object

private:
	 // disallow copying of SShellPtr objects
	SShellPtr(const SShellPtr&) {}
	void operator=(SShellPtr const&) {}
};


 // wrapper class for COM interface pointers

template<typename T> struct SIfacePtr
{
	SIfacePtr()
	 :	_p(0)
	{
	}

	SIfacePtr(T* p)
	 :	_p(p)
	{
		if (p)
			p->AddRef();
	}

	~SIfacePtr()
	{
		Free();
	}

	T* operator->()
	{
		return _p;
	}

	const T* operator->() const
	{
		return _p;
	}

/* not GCC compatible
	operator const T*() const
	{
		return _p;
	} */

	operator T*()
	{
		return _p;
	}

	T** operator&()
	{
		return &_p;
	}

	bool empty() const	//NOTE: GCC seems not to work correctly when defining operator bool() AND operator T*()
	{
		return !_p;
	}

	SIfacePtr& operator=(T* p)
	{
		Free();
		p->AddRef();
		_p = p;

		return *this;
	}

	void operator=(SIfacePtr const& o)
	{
		T* h = _p;

		if (o._p)
			o._p->AddRef();

		_p = o._p;

		if (h)
			h->Release();
	}

	void Free()
	{
		T* h = _p;
		_p = 0;

		if (h)
			h->Release();
	}

protected:
	SIfacePtr(const SIfacePtr& o)
	 :	_p(o._p)
	{
		if (_p)
			_p->AddRef();
	}

	T* _p;
};



 // caching of desktop ShellFolder object

struct ShellFolder;

struct CommonDesktop
{
	CommonDesktop()
	{
		_desktop = 0;
	}

	~CommonDesktop();

	void init();

	operator struct ShellFolder&()
	{
		return *_desktop;
	}

protected:
	ShellFolder* _desktop;
};


#ifndef _NO_COMUTIL	// _com_ptr available?

struct ShellFolder : public IShellFolderPtr	// IShellFolderPtr uses intrinsic extensions of the VC++ compiler.
{
	typedef IShellFolderPtr super;

	ShellFolder();	// desktop folder
	ShellFolder(IShellFolder* p);
	ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl);
	ShellFolder(LPCITEMIDLIST pidl);

	void	attach(IShellFolder* parent, LPCITEMIDLIST pidl);
	String	get_name(LPCITEMIDLIST pidl=NULL, SHGDNF flags=SHGDN_NORMAL) const;

	bool	empty() const {return !operator bool();}	//NOTE: see SIfacePtr::empty()
};

#else // _com_ptr not available -> use SIfacePtr

struct ShellFolder : public SIfacePtr<IShellFolder>
{
	typedef SIfacePtr<IShellFolder> super;

	ShellFolder();
	ShellFolder(IShellFolder* p);
	ShellFolder(IShellFolder* parent, LPCITEMIDLIST pidl);
	ShellFolder(LPCITEMIDLIST pidl);

	void	attach(IShellFolder* parent, LPCITEMIDLIST pidl);
	String	get_name(LPCITEMIDLIST pidl, SHGDNF flags=SHGDN_NORMAL) const;
};

#endif


extern ShellFolder& Desktop();


#ifdef UNICODE
#define	path_from_pidl path_from_pidlW
#else
#define	path_from_pidl path_from_pidlA
#endif

extern HRESULT path_from_pidlA(IShellFolder* folder, LPCITEMIDLIST pidl, LPSTR buffer, int len);
extern HRESULT path_from_pidlW(IShellFolder* folder, LPCITEMIDLIST pidl, LPWSTR buffer, int len);
extern HRESULT name_from_pidl(IShellFolder* folder, LPCITEMIDLIST pidl, LPTSTR buffer, int len, SHGDNF flags);


 // wrapper class for item ID lists

struct ShellPath : public SShellPtr<ITEMIDLIST>
{
	typedef SShellPtr<ITEMIDLIST> super;

	ShellPath()
	{
	}

	ShellPath(IShellFolder* folder, LPCWSTR path)
	{
		ULONG l;
		CheckError(folder->ParseDisplayName(0, 0, (LPOLESTR)path, &l, &_p, 0));
	}

	ShellPath(LPCWSTR path)
	{
		ULONG l;
		CheckError(Desktop()->ParseDisplayName(0, 0, (LPOLESTR)path, &l, &_p, 0));
	}

	ShellPath(IShellFolder* folder, LPCSTR path)
	{
		ULONG l;
		WCHAR b[MAX_PATH];

		MultiByteToWideChar(CP_ACP, 0, path, -1, b, MAX_PATH);
		CheckError(folder->ParseDisplayName(0, 0, b, &l, &_p, 0));
	}

	ShellPath(LPCSTR path)
	{
		ULONG l;
		WCHAR b[MAX_PATH];

		MultiByteToWideChar(CP_ACP, 0, path, -1, b, MAX_PATH);
		CheckError(Desktop()->ParseDisplayName(0, 0, b, &l, &_p, 0));
	}

	ShellPath(const ShellPath& o)
	 :	super(NULL)
	{
		if (o._p) {
			int l = _malloc->GetSize(o._p);
			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			memcpy(_p, o._p, l);
		}
	}

	ShellPath(ITEMIDLIST* p)
	 :	SShellPtr<ITEMIDLIST>(p)
	{
	}

	void operator=(const ShellPath& o)
	{
		ITEMIDLIST* h = _p;

		if (o._p) {
			int l = _malloc->GetSize(o._p);

			_p = (ITEMIDLIST*)_malloc->Alloc(l);
			memcpy(_p, o._p, l);
		}
		else
			_p = 0;

		_malloc->Free(h);
	}

	void operator=(ITEMIDLIST* p)
	{
		ITEMIDLIST* h = _p;

		if (p) {
			int l = _malloc->GetSize(p);

			_p = (ITEMIDLIST*)_malloc->Alloc(l);
			memcpy(_p, p, l);
		}
		else
			_p = 0;

		_malloc->Free(h);
	}

	void operator=(const SHITEMID& o)
	{
		ITEMIDLIST* h = _p;

		LPBYTE p = (LPBYTE)_malloc->Alloc(o.cb+2);
		*(PWORD)((LPBYTE)memcpy(p, &o, o.cb)+o.cb) = 0;
		_p = (ITEMIDLIST*)p;

		_malloc->Free(h);
	}

	void operator+=(const SHITEMID& o)
	{
		int l0 = _malloc->GetSize(_p);
		LPBYTE p = (LPBYTE)_malloc->Alloc(l0+o.cb);
		int l = l0 - 2;

		memcpy(p, _p, l);
		*(PWORD)((LPBYTE)memcpy(p+l, &o, o.cb)+o.cb) = 0;

		_malloc->Free(_p);
		_p = (ITEMIDLIST*)p;
	}

	void assign(ITEMIDLIST* pidl, size_t size)
	{
		ITEMIDLIST* h = _p;

		_p = (ITEMIDLIST*) _malloc->Alloc(size+sizeof(USHORT/*SHITEMID::cb*/));
		memcpy(_p, pidl, size);
		((ITEMIDLIST*)((LPBYTE)_p+size))->mkid.cb = 0; // terminator

		_malloc->Free(h);
	}

	void assign(ITEMIDLIST* pidl)
	{
		ITEMIDLIST* h = _p;

		if (pidl) {
			int l = _malloc->GetSize(pidl);
			_p = (ITEMIDLIST*)_malloc->Alloc(l);
			memcpy(_p, pidl, l);
		} else
			_p = 0; 

		_malloc->Free(h);
	}

	void split(ShellPath& parent, ShellPath& obj) const
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

	void GetUIObjectOf(REFIID riid, LPVOID* ppvOut, HWND hWnd=0, ShellFolder& sf=Desktop())
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

	ShellFolder get_folder()
	{
		return ShellFolder(_p);
	}

	ShellFolder get_folder(IShellFolder* parent)
	{
		return ShellFolder(parent, _p);
	}


	 // convert an item id list from relative to absolute (=relative to the desktop) format
	LPITEMIDLIST create_absolute_pidl(LPCITEMIDLIST parent_pidl, HWND hwnd) const
	{
		 // create a new item id list with _p append behind parent_pidl
		int l1 = _malloc->GetSize((void*)parent_pidl) - sizeof(USHORT/*SHITEMID::cb*/);
		int l2 = _malloc->GetSize(_p);

		LPITEMIDLIST p = (LPITEMIDLIST) _malloc->Alloc(l1+l2);

		memcpy(p, parent_pidl, l1);
		memcpy((LPBYTE)p+l1, _p, l2);

		return p;
	}
};


#ifdef __GCC__	// Wine doesn't know of unnamed union members and uses some macros instead.
#define	UNION_MEMBER(x) DUMMYUNIONNAME.##x
#else
#define	UNION_MEMBER(x) x
#endif


 // encapsulation of STRRET structure for easy string retrieval with conversion

#ifdef UNICODE
#define	StrRet StrRetW
#define	tcscpyn wcscpyn
#else
#define	StrRet StrRetA
#define	tcscpyn strcpyn
#endif

extern LPSTR strcpyn(LPSTR dest, LPCSTR source, size_t count);
extern LPWSTR wcscpyn(LPWSTR dest, LPCWSTR source, size_t count);

struct StrRetA : public STRRET
{
	~StrRetA()
	{
		if (uType == STRRET_WSTR)
			ShellMalloc()->Free(pOleStr);
	}

	void GetString(const SHITEMID& shiid, LPSTR b, int l)
	{
		switch(uType) {
		  case STRRET_WSTR:
			WideCharToMultiByte(CP_ACP, 0, UNION_MEMBER(pOleStr), -1, b, l, NULL, NULL);
			break;

		  case STRRET_OFFSET:
			strcpyn(b, (LPCSTR)&shiid+UNION_MEMBER(uOffset), l);
			break;

		  case STRRET_CSTR:
			strcpyn(b, UNION_MEMBER(cStr), l);
		}
	}
};

struct StrRetW : public STRRET
{
	~StrRetW()
	{
		if (uType == STRRET_WSTR)
			ShellMalloc()->Free(pOleStr);
	}

	void GetString(const SHITEMID& shiid, LPWSTR b, int l)
	{
		switch(uType) {
		  case STRRET_WSTR:
			wcscpyn(b, UNION_MEMBER(pOleStr), l);
			break;

		  case STRRET_OFFSET:
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&shiid+UNION_MEMBER(uOffset), -1, b, l);
			break;

		  case STRRET_CSTR:
			MultiByteToWideChar(CP_ACP, 0, UNION_MEMBER(cStr), -1, b, l);
		}
	}
};


class FileSysShellPath : public ShellPath
{
	TCHAR	_fullpath[MAX_PATH];

protected:
	FileSysShellPath() {_fullpath[0] = '\0';}

public:
	FileSysShellPath(const ShellPath& o) : ShellPath(o) {_fullpath[0] = '\0';}

	operator LPCTSTR() {SHGetPathFromIDList(_p, _fullpath); return _fullpath;}
};


struct FolderBrowser : public FileSysShellPath
{
	FolderBrowser(HWND owner, UINT flags, LPCTSTR title, LPCITEMIDLIST root=0)
	{
		_displayname[0] = '\0';
		_browseinfo.hwndOwner = owner;
		_browseinfo.pidlRoot = root;
		_browseinfo.pszDisplayName = _displayname;
		_browseinfo.lpszTitle = title;
		_browseinfo.ulFlags = flags;
		_browseinfo.lpfn = 0;
		_browseinfo.lParam = 0;
		_browseinfo.iImage = 0;

		_p = SHBrowseForFolder(&_browseinfo);
	}

	LPCTSTR GetDisplayName()
	{
		return _displayname;
	}

	bool IsOK()
	{
		return _p != 0;
	}

private:
	BROWSEINFO _browseinfo;
	TCHAR	_displayname[MAX_PATH];
};


struct SpecialFolderPath : public ShellPath
{
	SpecialFolderPath(int folder, HWND hwnd)
	{
		/*HRESULT hr = */SHGetSpecialFolderLocation(hwnd, folder, &_p);
	}
};

struct DesktopFolder : public SpecialFolderPath
{
	DesktopFolder()
	 :	SpecialFolderPath(CSIDL_DESKTOP, 0)
	{
	}
};

struct SpecialFolder : public ShellFolder
{
	SpecialFolder(int folder, HWND hwnd)
	 :	ShellFolder(Desktop(), SpecialFolderPath(folder, hwnd))
	{
	}
};


#if _WIN32_IE>=0x400 // is SHGetSpecialFolderPath() available?

 /// file system path of special folder
struct SpecialFolderFSPath
{
	SpecialFolderFSPath(int folder/*e.g. CSIDL_DESKTOP*/, HWND hwnd)
	{
		_fullpath[0] = '\0';

		SHGetSpecialFolderPath(hwnd, _fullpath, folder, TRUE);
	}

	operator LPCTSTR()
	{
		return _fullpath;
	}

protected:
	TCHAR	_fullpath[MAX_PATH];
};

#else // _WIN32_IE<0x400 -> use SHGetSpecialFolderLocation()

struct SpecialFolderFSPath : public FileSysShellPath
{
	SpecialFolderFSPath(int folder, HWND hwnd)
	{
		HRESULT hr = SHGetSpecialFolderLocation(hwnd, folder, &_p);
	}
};

#endif


 // wrapper class for enumerating shell namespace objects

struct ShellItemEnumerator : public SIfacePtr<IEnumIDList>
{
	ShellItemEnumerator(IShellFolder* folder, DWORD flags=SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN)
	{
		CheckError(folder->EnumObjects(0, flags, &_p));
	}
};
