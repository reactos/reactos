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

#define	COMExceptionBase _com_error

#else

 /// COM ExceptionBase class as replacement for _com_error
struct COMExceptionBase
{
	COMExceptionBase(HRESULT hr)
	 :	_hr(hr)
	{
	}

	HRESULT Error() const
	{
		return _hr;
	}

	LPCTSTR ErrorMessage() const
	{
		if (_msg.empty()) {
			LPTSTR pBuf;

			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
				0, _hr, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPTSTR)&pBuf, 0, NULL)) {
				_msg = pBuf;
				LocalFree(pBuf);
			 } else {
				TCHAR buffer[128];
				_stprintf(buffer, TEXT("unknown COM Exception: 0x%08X"), _hr);
				_msg = buffer;
			 }
		}

		return _msg;
	}

protected:
	HRESULT	_hr;
	mutable String _msg;
};

#endif


 /// COM Exception with context information

struct COMException : public COMExceptionBase
{
	typedef COMExceptionBase super;

	COMException(HRESULT hr)
	 :	super(hr),
		_context(CURRENT_CONTEXT),
		_file(NULL), _line(0)
	{
		LOG(toString());
		LOG(CURRENT_CONTEXT.getStackTrace());
	}

	COMException(HRESULT hr, const char* file, int line)
	 :	super(hr),
		_context(CURRENT_CONTEXT),
		_file(file), _line(line)
	{
		LOG(toString());
		LOG(CURRENT_CONTEXT.getStackTrace());
	}

	COMException(HRESULT hr, const String& obj)
	 :	super(hr),
		_context(CURRENT_CONTEXT),
		_file(NULL), _line(0)
	{
		LOG(toString());
		LOG(CURRENT_CONTEXT.getStackTrace());
	}

	COMException(HRESULT hr, const String& obj, const char* file, int line)
	 :	super(hr),
		_context(CURRENT_CONTEXT),
		_file(file), _line(line)
	{
		LOG(toString());
		LOG(CURRENT_CONTEXT.getStackTrace());
	}

	String toString() const;

	Context _context;

	const char* _file;
	int _line;
};

#define	THROW_EXCEPTION(e) throw COMException(e, __FILE__, __LINE__)
#define	CHECKERROR(hr) ((void)(FAILED(hr)? THROW_EXCEPTION(hr): 0))


#ifdef _NO_COMUTIL

inline void CheckError(HRESULT hr)
{
	if (FAILED(hr))
		throw COMException(hr);
}

#endif


 /// COM Initialisation

struct ComInit
{
	ComInit()
	{
		CHECKERROR(CoInitialize(0));
	}

#if (_WIN32_WINNT>=0x0400) || defined(_WIN32_DCOM)
	ComInit(DWORD flag)
	{
		CHECKERROR(CoInitializeEx(0, flag));
	}
#endif

	~ComInit()
	{
		CoUninitialize();
	}
};


 /// OLE initialisation for drag drop support

struct OleInit
{
	OleInit()
	{
		CHECKERROR(OleInitialize(0));
	}

	~OleInit()
	{
		OleUninitialize();
	}
};


 /// Exception Handler for COM exceptions

extern void HandleException(COMException& e, HWND hwnd);


 /// We use a common IMalloc object for all shell memory allocations.

struct CommonShellMalloc
{
	CommonShellMalloc()
	{
		_p = NULL;
	}

	void init()
	{
		if (!_p)
			CHECKERROR(SHGetMalloc(&_p));
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


 /// wrapper class for IMalloc with usage of common allocator

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


 /// wrapper template class for pointers to shell objects managed by IMalloc

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
		_p = NULL;
	}

	T* _p;
	mutable ShellMalloc _malloc;	// IMalloc memory management object

private:
	 // disallow copying of SShellPtr objects
	SShellPtr(const SShellPtr&) {}
	void operator=(SShellPtr const&) {}
};


 /// wrapper class for COM interface pointers

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
		_p = NULL;

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



struct ShellFolder;


 /// caching of desktop ShellFolder object

struct CommonDesktop
{
	CommonDesktop()
	{
		_desktop = 0;
	}

	~CommonDesktop();

	void init();

	operator ShellFolder&()
	{
		return *_desktop;
	}

protected:
	ShellFolder* _desktop;
};


#ifndef _NO_COMUTIL	// _com_ptr available?

 /// IShellFolder smart pointer
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

#ifdef UNICODE
#define	IShellLinkPtr IShellLinkWPtr
#else
#define	IShellLinkPtr IShellLinkAPtr
#endif

 /// IShellLink smart pointer
struct ShellLinkPtr : public IShellLinkPtr
{
	typedef IShellLinkPtr super;

	ShellLinkPtr(IShellLink* p)
	 :	super(p)
	{
		p->AddRef();
	}

	bool	empty() const {return !operator bool();}	//NOTE: see SIfacePtr::empty()
};

#else // _com_ptr not available -> use SIfacePtr

 /// IShellFolder smart pointer
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

 /// IShellLink smart pointer
struct ShellLinkPtr : public SIfacePtr<IShellLink>
{
	typedef SIfacePtr<IShellLink> super;

	ShellLinkPtr(IShellLink* p)
	 :	super(p)
	{
		_p->AddRef();
	}

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


#ifdef __MINGW32__	// ILGetSize() is currently missing in MinGW.
extern "C" UINT STDCALL ILGetSize(LPCITEMIDLIST pidl);

#ifdef UNICODE		// CFSTR_FILENAME is defined wrong in MinGW.
#define CFSTR_FILENAMEW TEXT("FileNameW")
#undef CFSTR_FILENAME
#define CFSTR_FILENAME CFSTR_FILENAMEW
#endif

#endif


 /// wrapper class for item ID lists

struct ShellPath : public SShellPtr<ITEMIDLIST>
{
	typedef SShellPtr<ITEMIDLIST> super;

	ShellPath()
	{
	}

	ShellPath(IShellFolder* folder, LPCWSTR path)
	{
		CONTEXT("ShellPath::ShellPath(IShellFolder*, LPCWSTR)");

		if (path)
			CHECKERROR(folder->ParseDisplayName(0, 0, (LPOLESTR)path, NULL, &_p, 0));
		else
			_p = NULL;
	}

	ShellPath(LPCWSTR path)
	{
		OBJ_CONTEXT("ShellPath::ShellPath(LPCWSTR)", path);

		if (path)
			CHECKERROR(Desktop()->ParseDisplayName(0, 0, (LPOLESTR)path, NULL, &_p, 0));
		else
			_p = NULL;
	}

	ShellPath(IShellFolder* folder, LPCSTR path)
	{
		CONTEXT("ShellPath::ShellPath(IShellFolder*, LPCSTR)");

		WCHAR b[MAX_PATH];

		if (path) {
			MultiByteToWideChar(CP_ACP, 0, path, -1, b, MAX_PATH);
			CHECKERROR(folder->ParseDisplayName(0, 0, b, NULL, &_p, 0));
		} else
			_p = NULL;
	}

	ShellPath(LPCSTR path)
	{
		CONTEXT("ShellPath::ShellPath(LPCSTR)");

		WCHAR b[MAX_PATH];

		if (path) {
			MultiByteToWideChar(CP_ACP, 0, path, -1, b, MAX_PATH);
			CHECKERROR(Desktop()->ParseDisplayName(0, 0, b, NULL, &_p, 0));
		} else
			_p = NULL;
	}

	ShellPath(const ShellPath& o)
	 :	super(NULL)
	{
		//CONTEXT("ShellPath::ShellPath(const ShellPath&)");

		if (o._p) {
			int l = ILGetSize(o._p);
			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			if (_p) memcpy(_p, o._p, l);
		}
	}

	explicit ShellPath(LPITEMIDLIST p)
	 :	super(p)
	{
	}

	ShellPath(LPCITEMIDLIST p)
	{
		//CONTEXT("ShellPath::ShellPath(LPCITEMIDLIST)");

		if (p) {
			int l = ILGetSize(p);
			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			if (_p) memcpy(_p, p, l);
		}
	}

	void operator=(const ShellPath& o)
	{
		//CONTEXT("ShellPath::operator=(const ShellPath&)");

		ITEMIDLIST* h = _p;

		if (o._p) {
			int l = ILGetSize(o._p);

			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			if (_p) memcpy(_p, o._p, l);
		}
		else
			_p = NULL;

		_malloc->Free(h);
	}

	void operator=(ITEMIDLIST* p)
	{
		//CONTEXT("ShellPath::operator=(ITEMIDLIST*)");

		ITEMIDLIST* h = _p;

		if (p) {
			int l = ILGetSize(p);
			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			if (_p) memcpy(_p, p, l);
		}
		else
			_p = NULL;

		_malloc->Free(h);
	}

	void operator=(const SHITEMID& o)
	{
		ITEMIDLIST* h = _p;

		LPBYTE p = (LPBYTE)_malloc->Alloc(o.cb+2);
		if (p) *(PWORD)((LPBYTE)memcpy(p, &o, o.cb)+o.cb) = 0;
		_p = (ITEMIDLIST*)p;

		_malloc->Free(h);
	}

	void operator+=(const SHITEMID& o)
	{
		int l0 = ILGetSize(_p);
		LPBYTE p = (LPBYTE)_malloc->Alloc(l0+o.cb);
		int l = l0 - 2;

		if (p) {
			memcpy(p, _p, l);
			*(PWORD)((LPBYTE)memcpy(p+l, &o, o.cb)+o.cb) = 0;
		}

		_malloc->Free(_p);
		_p = (ITEMIDLIST*)p;
	}

	void assign(LPCITEMIDLIST pidl, size_t size)
	{
		//CONTEXT("ShellPath::assign(LPCITEMIDLIST, size_t)");

		ITEMIDLIST* h = _p;

		_p = (ITEMIDLIST*) _malloc->Alloc(size+sizeof(USHORT/*SHITEMID::cb*/));

		if (_p) {
			memcpy(_p, pidl, size);
			((ITEMIDLIST*)((LPBYTE)_p+size))->mkid.cb = 0; // terminator
		}

		_malloc->Free(h);
	}

	void assign(LPCITEMIDLIST pidl)
	{
		//CONTEXT("ShellPath::assign(LPCITEMIDLIST)");

		ITEMIDLIST* h = _p;

		if (pidl) {
			int l = ILGetSize(pidl);
			_p = (ITEMIDLIST*) _malloc->Alloc(l);
			if (_p) memcpy(_p, pidl, l);
		} else
			_p = NULL; 

		_malloc->Free(h);
	}

	void split(ShellPath& parent, ShellPath& obj) const;

	void GetUIObjectOf(REFIID riid, LPVOID* ppvOut, HWND hWnd=0, ShellFolder& sf=Desktop());

	ShellFolder get_folder()
	{
		return ShellFolder(_p);
	}

	ShellFolder get_folder(IShellFolder* parent)
	{
		CONTEXT("ShellPath::get_folder()");
		return ShellFolder(parent, _p);
	}

	 // convert an item id list from relative to absolute (=relative to the desktop) format
	ShellPath create_absolute_pidl(LPCITEMIDLIST parent_pidl) const;
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

 /// easy retrieval of multi byte strings out of STRRET structures
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

 /// easy retrieval of wide char strings out of STRRET structures
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


 /// Retrieval of file system paths of ShellPath objects
class FileSysShellPath : public ShellPath
{
	TCHAR	_fullpath[MAX_PATH];

protected:
	FileSysShellPath() {_fullpath[0] = '\0';}

public:
	FileSysShellPath(const ShellPath& o) : ShellPath(o) {_fullpath[0] = '\0';}

	operator LPCTSTR() {if (!SHGetPathFromIDList(_p, _fullpath)) return NULL; return _fullpath;}
};


 /// Browse dialog operating on shell namespace
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


 /// Retrieval of special shell folder paths
struct SpecialFolderPath : public ShellPath
{
	SpecialFolderPath(int folder, HWND hwnd)
	{
		HRESULT hr = SHGetSpecialFolderLocation(hwnd, folder, &_p);
		CHECKERROR(hr);
	}
};

 /// Shell folder path of the desktop
struct DesktopFolderPath : public SpecialFolderPath
{
	DesktopFolderPath()
	 :	SpecialFolderPath(CSIDL_DESKTOP, 0)
	{
	}
};

 /// Retrieval of special shell folder
struct SpecialFolder : public ShellFolder
{
	SpecialFolder(int folder, HWND hwnd)
	 :	ShellFolder(Desktop(), SpecialFolderPath(folder, hwnd))
	{
	}
};

 /// Shell folder of the desktop
struct DesktopFolder : public ShellFolder
{
};


 /// file system path of special folder
struct SpecialFolderFSPath
{
	SpecialFolderFSPath(int folder/*e.g. CSIDL_DESKTOP*/, HWND hwnd);

	operator LPCTSTR()
	{
		return _fullpath;
	}

protected:
	TCHAR	_fullpath[MAX_PATH];
};

/*
 /// file system path of special folder
struct SpecialFolderFSPath : public FileSysShellPath
{
	SpecialFolderFSPath(int folder, HWND hwnd)
	{
		CONTEXT("SpecialFolderFSPath::SpecialFolderFSPath()");

		HRESULT hr = SHGetSpecialFolderLocation(hwnd, folder, &_p);
		CHECKERROR(hr);
	}
};
*/


 /// wrapper class for enumerating shell namespace objects

struct ShellItemEnumerator : public SIfacePtr<IEnumIDList>
{
	ShellItemEnumerator(IShellFolder* folder, DWORD flags=SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN)
	{
		CONTEXT("ShellItemEnumerator::ShellItemEnumerator()");

		CHECKERROR(folder->EnumObjects(0, flags, &_p));
	}
};


 /// list of PIDLs
struct PIDList
{
	PIDList()
	{
		memset(&_stgm, 0, sizeof(STGMEDIUM));
	}

	~PIDList()
	{
		if (_stgm.hGlobal) {
			GlobalUnlock(_stgm.hGlobal);
			ReleaseStgMedium(&_stgm);
		}
	}

	HRESULT GetData(IDataObject* selection)
	{
		static UINT CF_IDLIST = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

		FORMATETC fetc;
		fetc.cfFormat = CF_IDLIST;
		fetc.ptd = NULL;
		fetc.dwAspect = DVASPECT_CONTENT;
		fetc.lindex = -1;
		fetc.tymed = TYMED_HGLOBAL;

		HRESULT hr = selection->QueryGetData(&fetc);
		if (FAILED(hr))
			return hr;

		hr = selection->GetData(&fetc, &_stgm);
		if (FAILED(hr))
			return hr;

		_pIDList = (LPIDA)GlobalLock(_stgm.hGlobal);

		return hr;
	}

	operator LPIDA() {return _pIDList;}

protected:
	STGMEDIUM _stgm;
	LPIDA _pIDList;
};

extern HRESULT ShellFolderContextMenu(IShellFolder* shell_folder, HWND hwndParent, int cidl, LPCITEMIDLIST* ppidl, int x, int y);
