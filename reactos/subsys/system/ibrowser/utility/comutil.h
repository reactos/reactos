/*
 * Copyright 2005 Martin Fuchs
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
 // ROS Internet Web Browser
 // 
 // comutil.h
 //
 // C++ wrapper classes for COM interfaces
 //
 // Martin Fuchs, 25.01.2005
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


 // Exception Handling

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
				_stprintf(buffer, TEXT("unknown Exception: 0x%08lX"), _hr);
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


 /// Exception with context information

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

#define	THROW_EXCEPTION(hr) throw COMException(hr, __FILE__, __LINE__)
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

	SIfacePtr(IUnknown* unknown, REFIID riid)
	{
		CHECKERROR(unknown->QueryInterface(riid, (LPVOID*)&_p));
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

	bool empty() const	//NOTE: GCC seems not to work correctly when defining operator bool() AND operator T*() at one time
	{
		return !_p;
	}

	SIfacePtr& operator=(T* p)
	{
		Free();

		if (p) {
			p->AddRef();
			_p = p;
		}

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

	HRESULT CreateInstance(REFIID clsid, REFIID riid)
	{
		return CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, riid, (LPVOID*)&_p);
	}

	template<typename I> HRESULT QueryInterface(REFIID riid, I* p)
	{
		return _p->QueryInterface(riid, (LPVOID*)p);
	}

	T* get()
	{
		return _p;
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
