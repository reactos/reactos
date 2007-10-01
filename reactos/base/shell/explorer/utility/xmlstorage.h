
 //
 // XML storage C++ classes version 1.2
 //
 // Copyright (c) 2004, 2005, 2006, 2007 Martin Fuchs <martin-fuchs@gmx.net>
 //

 /// \file xmlstorage.h
 /// XMLStorage header file


/*

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in
	the documentation and/or other materials provided with the
	distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _XMLSTORAGE_H


#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#define UNICODE
#endif
#endif

#ifndef _WIN32
#ifdef UNICODE
#error no UNICODE build in Unix version available
#endif
#ifndef XS_STRING_UTF8
#define XS_STRING_UTF8
#endif
#endif


#if _MSC_VER>=1400
#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES			1
#endif
#endif


#ifdef XS_USE_XERCES

#ifndef UNICODE
#ifndef XS_STRING_UTF8
#define XS_STRING_UTF8
#endif
#endif

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>

using XERCES_CPP_NAMESPACE_QUALIFIER Locator;
using XERCES_CPP_NAMESPACE_QUALIFIER SAXParser;
using XERCES_CPP_NAMESPACE_QUALIFIER HandlerBase;
using XERCES_CPP_NAMESPACE_QUALIFIER InputSource;
using XERCES_CPP_NAMESPACE_QUALIFIER AttributeList;
using XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException;

typedef XMLCh XML_Char;

#elif defined(XS_USE_EXPAT)

#include <expat/expat.h>

#endif


#ifdef _MSC_VER
#pragma warning(disable: 4786)

#ifndef	XS_NO_COMMENT

#ifdef XS_USE_XERCES
#ifdef _DEBUG
#pragma comment(lib, "xerces-c_2D")
#else
#pragma comment(lib, "xerces-c_2")
#endif
#elif defined(XS_USE_EXPAT)
#ifdef XML_STATIC
#ifndef _DEBUG
#pragma comment(lib, "libexpatMT")
#endif
#else
#pragma comment(lib, "libexpat")
#endif
#endif

#ifndef _STRING_DEFINED	// _STRING_DEFINED only allowed if using xmlstorage.cpp embedded in the project
#if defined(_DEBUG) && defined(_DLL)	// DEBUG version only supported with MSVCRTD
#if _MSC_VER==1400
#pragma comment(lib, "xmlstorage-vc8d")
#else
#pragma comment(lib, "xmlstorage-vc6d")
#endif
#else
#ifdef _DLL
#if _MSC_VER==1400
#pragma comment(lib, "xmlstorage-vc8")
#else
#pragma comment(lib, "xmlstorage-vc6")
#endif
#elif defined(_MT)
#if _MSC_VER==1400
#pragma comment(lib, "xmlstorage-vc8t")
#else
#pragma comment(lib, "xmlstorage-vc6t")
#endif
#else
 // -ML is no more supported by VS2005.
#pragma comment(lib, "xmlstorage-vc6l")
#endif
#endif
#endif // _STRING_DEFINED

#endif // XS_NO_COMMENT

#endif // _MSC_VER


#ifdef _WIN32

#include <windows.h>	// for LPCTSTR
#include <tchar.h>
#include <malloc.h>

#ifndef _MSC_VER
#include <stdio.h>	// vsnprintf(), snprintf()
#endif

#else

#include <wchar.h>
#include <stdarg.h>

typedef char CHAR;
#ifdef _WCHAR_T_DEFINED
#define	__wchar_t wchar_t
#endif

typedef __wchar_t WCHAR;
typedef unsigned char UCHAR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef WCHAR* LPWSTR;
typedef const WCHAR* LPCWSTR;

#ifndef UNICODE
#define TEXT(x) x
typedef char TCHAR;
typedef unsigned char _TUCHAR;
typedef CHAR* PTSTR;
typedef CHAR* LPTSTR;
typedef const CHAR* LPCTSTR;

#define _ttoi atoi
#define _tfopen fopen
#define _tcstod strtod
#define _tcslen strlen
#define _tcsstr strstr
#define _snprintf snprintf
#define _sntprintf snprintf
#define _vsnprintf vsnprintf
#define _vsntprintf vsnprintf
#define _stricmp strcasecmp
#define _tcsicmp strcasecmp
#define strnicmp strncasecmp
#define _tcsnicmp strncasecmp
#endif

#endif


#include <fstream>
#include <sstream>
#include <string>
#include <stack>
#include <list>
#include <map>


#ifndef BUFFER_LEN
#define BUFFER_LEN 2048
#endif


namespace XMLStorage {


#ifndef XS_String

#ifdef XS_STRING_UTF8
#define	XS_CHAR char
#define	XS_TEXT(x) x
#define LPXSSTR LPSTR
#define LPCXSSTR LPCSTR
#define	XS_cmp strcmp
#define	XS_icmp _stricmp
#define	XS_ncmp strncmp
#define	XS_nicmp strnicmp
#define	XS_toi atoi
#define	XS_tod strtod
#define	XS_len strlen
#define	XS_snprintf _snprintf
#define	XS_vsnprintf _vsnprintf
#define	XS_strstr strstr
#else
#define	XS_CHAR TCHAR
#define	XS_TEXT(x) TEXT(x)
#define LPXSSTR LPTSTR
#define LPCXSSTR LPCTSTR
#define	XS_cmp _tcscmp
#define	XS_icmp _tcsicmp
#define	XS_ncmp _tcsncmp
#define	XS_nicmp _tcsnicmp
#define	XS_toi _ttoi
#define	XS_tod _tcstod
#define	XS_len _tcslen
#define	XS_snprintf _sntprintf
#define	XS_vsnprintf _vsntprintf
#define	XS_strstr _tcsstr
#endif

#ifndef COUNTOF
#if _MSC_VER>=1400
#define COUNTOF _countof
#else
#define COUNTOF(b) (sizeof(b)/sizeof(b[0]))
#endif
#endif


int inline isxmlsym(unsigned char c)
{
	return isalnum(c) || c=='_' || c=='-';
}


#if defined(_STRING_DEFINED) && !defined(XS_STRING_UTF8)

#define	XS_String String

#else // _STRING_DEFINED, !XS_STRING_UTF8

 /// string class for TCHAR strings

struct XS_String
#if defined(UNICODE) && !defined(XS_STRING_UTF8)
 : public std::wstring
#else
 : public std::string
#endif
{
#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	typedef std::wstring super;
#else
	typedef std::string super;
#endif

	XS_String() {}

	XS_String(LPCXSSTR s) {if (s) super::assign(s);}
	XS_String(LPCXSSTR s, size_t l) : super(s, l) {}

	XS_String(const super& other) : super(other) {}
	XS_String(const XS_String& other) : super(other) {}

#ifdef _WIN32
#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	XS_String(LPCSTR s) {assign(s);}
	XS_String(LPCSTR s, size_t l) {assign(s, l);}
	XS_String(const std::string& other) {assign(other.c_str());}
	XS_String& operator=(LPCSTR s) {assign(s); return *this;}
	void assign(LPCSTR s) {if (s) {size_t bl=strlen(s); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, bl, b, bl));} else erase();}
	void assign(LPCSTR s, size_t l) {if (s) {size_t bl=l; LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, l, b, bl));} else erase();}
#else
	XS_String(LPCWSTR s) {assign(s);}
	XS_String(LPCWSTR s, size_t l) {assign(s, l);}
	XS_String(const std::wstring& other) {assign(other.c_str());}
	XS_String& operator=(LPCWSTR s) {assign(s); return *this;}
#ifdef XS_STRING_UTF8
	void assign(LPCWSTR s) {if (s) {size_t bl=wcslen(s); LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_UTF8, 0, s, (int)bl, b, (int)bl, 0, 0));} else erase();}
	void assign(LPCWSTR s, size_t l) {size_t bl=l; if (s) {LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_UTF8, 0, s, (int)l, b, (int)bl, 0, 0));} else erase();}
#else // if !UNICODE && !XS_STRING_UTF8
	void assign(LPCWSTR s) {if (s) {size_t bl=wcslen(s); LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, (int)bl, b, (int)bl, 0, 0));} else erase();}
	void assign(LPCWSTR s, size_t l) {size_t bl=l; if (s) {LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, (int)l, b, (int)bl, 0, 0));} else erase();}
#endif
#endif
#endif // _WIN32

#ifdef XS_STRING_UTF8
	void assign(const XS_String& s) {assign(s.c_str());}
#endif

	XS_String& operator=(LPCXSSTR s) {if (s) super::assign(s); else erase(); return *this;}
	XS_String& operator=(const super& s) {super::assign(s); return *this;}
	void assign(LPCXSSTR s) {super::assign(s);}
	void assign(LPCXSSTR s, size_t l) {super::assign(s, l);}

	operator LPCXSSTR() const {return c_str();}

#ifdef _WIN32
#ifdef XS_STRING_UTF8
	operator std::wstring() const {size_t bl=length(); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); return std::wstring(b, MultiByteToWideChar(CP_UTF8, 0, c_str(), bl, b, bl));}
#elif defined(UNICODE)
	operator std::string() const {size_t bl=length(); LPSTR b=(LPSTR)alloca(bl); return std::string(b, WideCharToMultiByte(CP_ACP, 0, c_str(), bl, b, bl, 0, 0));}
#else
	operator std::wstring() const {size_t bl=length(); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); return std::wstring(b, MultiByteToWideChar(CP_ACP, 0, c_str(), (int)bl, b, (int)bl));}
#endif
#endif

	XS_String& printf(LPCXSSTR fmt, ...)
	{
		va_list l;
		XS_CHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::assign(b, XS_vsnprintf(b, COUNTOF(b), fmt, l));
		va_end(l);

		return *this;
	}

	XS_String& vprintf(LPCXSSTR fmt, va_list l)
	{
		XS_CHAR b[BUFFER_LEN];

		super::assign(b, XS_vsnprintf(b, COUNTOF(b), fmt, l));

		return *this;
	}

	XS_String& appendf(LPCXSSTR fmt, ...)
	{
		va_list l;
		XS_CHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::append(b, XS_vsnprintf(b, COUNTOF(b), fmt, l));
		va_end(l);

		return *this;
	}

	XS_String& vappendf(LPCXSSTR fmt, va_list l)
	{
		XS_CHAR b[BUFFER_LEN];

		super::append(b, XS_vsnprintf(b, COUNTOF(b), fmt, l));

		return *this;
	}
};

#endif // _STRING_DEFINED, !XS_STRING_UTF8

#endif // XS_String


#define	XS_EMPTY_STR XS_TEXT("")
#define	XS_TRUE_STR XS_TEXT("true")
#define	XS_FALSE_STR XS_TEXT("false")
#define	XS_INTFMT_STR XS_TEXT("%d")
#define	XS_FLOATFMT_STR XS_TEXT("%f")

 // work around GCC's wide string constant bug
#ifdef __GNUC__
extern const LPCXSSTR XS_EMPTY;
extern const LPCXSSTR XS_TRUE;
extern const LPCXSSTR XS_FALSE;
extern const LPCXSSTR XS_INTFMT;
extern const LPCXSSTR XS_FLOATFMT;
#else
#define	XS_EMPTY XS_EMPTY_STR
#define	XS_TRUE XS_TRUE_STR
#define	XS_FALSE XS_FALSE_STR
#define	XS_INTFMT XS_INTFMT_STR
#define	XS_FLOATFMT XS_FLOATFMT_STR
#endif


#ifndef XS_STRING_UTF8

 // from UTF-8 to XS internal string encoding
inline void assign_utf8(XS_String& s, const char* str, size_t lutf8)
{
#ifdef UNICODE
	LPTSTR buffer = (LPTSTR)alloca(sizeof(TCHAR)*lutf8);
	int l = MultiByteToWideChar(CP_UTF8, 0, str, (int)lutf8, buffer, (int)lutf8);
#else
	LPWSTR wbuffer = (LPWSTR)alloca(sizeof(WCHAR)*lutf8);
	int l = MultiByteToWideChar(CP_UTF8, 0, str, (int)lutf8, wbuffer, (int)lutf8);

	int bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_ACP, 0, wbuffer, l, buffer, bl, 0, 0);
#endif

	s.assign(buffer, l);
}

 // from UTF-8 to XS internal string encoding
inline void assign_utf8(XS_String& s, const char* str)
{
	assign_utf8(s, str, strlen(str));
}

 // from XS internal string encoding to UTF-8
inline std::string get_utf8(LPCTSTR s, size_t l)
{
#ifdef UNICODE
	size_t bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_UTF8, 0, s, (int)l, buffer, (int)bl, 0, 0);
#else
	LPWSTR wbuffer = (LPWSTR)alloca(sizeof(WCHAR)*l);
	l = MultiByteToWideChar(CP_ACP, 0, s, (int)l, wbuffer, (int)l);

	size_t bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_UTF8, 0, wbuffer, (int)l, buffer, (int)bl, 0, 0);
#endif

	return std::string(buffer, l);
}

#ifdef UNICODE
 // from XS internal string encoding to UTF-8
inline std::string get_utf8(const char* s, size_t l)
{
	LPWSTR wbuffer = (LPWSTR)alloca(sizeof(WCHAR)*l);
	l = MultiByteToWideChar(CP_ACP, 0, s, (int)l, wbuffer, (int)l);

	size_t bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_UTF8, 0, wbuffer, (int)l, buffer, (int)bl, 0, 0);

	return std::string(buffer, l);
}
#endif

 // from XS internal string encoding to UTF-8
inline std::string get_utf8(const XS_String& s)
{
	return get_utf8(s.c_str(), s.length());
}

#endif // XS_STRING_UTF8

extern std::string EncodeXMLString(const XS_String& str, bool cdata=false);
extern XS_String DecodeXMLString(const XS_String& str);


#ifdef __GNUC__
#include <ext/stdio_filebuf.h>
#define FILE_FILEBUF __gnu_cxx::stdio_filebuf<char>
#elif defined(_MSC_VER)
#define FILE_FILEBUF std::filebuf
#endif

#ifdef FILE_FILEBUF

 /// base class for XMLStorage::tifstream and XMLStorage::tofstream
struct FileHolder
{
	FileHolder(LPCTSTR path, LPCTSTR mode)
	{
#ifdef __STDC_WANT_SECURE_LIB__	// secure CRT functions using VS 2005
		if (_tfopen_s(&_pfile, path, mode) != 0)
			_pfile = NULL;
#else
		_pfile = _tfopen(path, mode);
#endif
	}

	~FileHolder()
	{
		if (_pfile)
			fclose(_pfile);
	}

protected:
	FILE*	_pfile;
};

 /// input file stream with ANSI/UNICODE file names
struct tifstream : public std::istream, FileHolder
{
	typedef std::istream super;

	tifstream(LPCTSTR path)
	 :	super(&_buf),
		FileHolder(path, TEXT("rb")),	// binary mode is important for XMLReader::read_buffer() with MinGW libraries
#ifdef __GNUC__
		_buf(_pfile, std::ios::in)
#else
		_buf(_pfile)
#endif
	{
	}

protected:
	FILE_FILEBUF _buf;
};

 /// output file stream with ANSI/UNICODE file names
struct tofstream : public std::ostream, FileHolder
{
	typedef std::ostream super;

	tofstream(LPCTSTR path)
	 :	super(&_buf),
		FileHolder(path, TEXT("wb")),
#ifdef __GNUC__
		_buf(_pfile, std::ios::out)
#else
		_buf(_pfile)
#endif
	{
	}

	~tofstream()
	{
		flush();
	}

protected:
	FILE_FILEBUF _buf;
};

#else // FILE_FILEBUF

#ifdef UNICODE
#error UNICODE not supported for this platform
#endif

struct tifstream : public std::ifstream
{
	typedef std::ifstream super;

	tifstream(const char* path)
	 : super(path, std::ios::in|std::ios::binary)
	{
	}
};

struct tofstream : public std::ofstream
{
	typedef std::ofstream super;

	tofstream(const char* path)
	 : super(path, std::ios::out|std::ios::binary)
	{
	}
};

#endif


 // write XML files with 2 spaces indenting
#define XML_INDENT_SPACE "  "


#if defined(XS_USE_XERCES) || defined(XS_USE_EXPAT)

#if defined(XML_UNICODE)/*Expat*/ || defined(XS_USE_XERCES)/*Xerces*/	// Are Expat/Xerces XML strings UTF-16 encoded?
typedef XS_String String_from_XML_Char;

#elif defined(XS_STRING_UTF8)
typedef XS_String String_from_XML_Char;

#else

 /// converter from Expat/Xerces strings to XMLStorage internal strings
struct String_from_XML_Char : public XS_String
{
	String_from_XML_Char(const XML_Char* str)
	{
		assign_utf8(*this, str);
	}
};

#endif

#endif // defined(XS_USE_XERCES) || defined(XS_USE_EXPAT)


#if defined(UNICODE) && !defined(XS_STRING_UTF8)

 // optimization for faster UNICODE/ASCII string comparison without temporary A/U conversion
inline bool operator==(const XS_String& s1, const char* s2)
{
	LPCWSTR p = s1;
	const unsigned char* q = (const unsigned char*)s2;

	while(*p && *q)
		if (*p++ != *q++)
			return false;

	return *p == *q;
};

#endif


 /// XML Error with message and location
struct XMLError
{
	XMLError()
	 :	_line(0),
		_column(0),
		_error_code(0)
	{
	}

	std::string str() const;
	friend std::ostream& operator<<(std::ostream&, const XMLError& err);

	XS_String _message;
	XS_String _systemId;
	int _line;
	int _column;
	int _error_code;
};

 /// list of XMLError entries
struct XMLErrorList : public std::list<XMLError>
{
	XS_String str() const;
};


#ifdef XMLNODE_LOCATION
 /// location of XML Node including XML file name
struct XMLLocation
{
	XMLLocation()
	 :	_pdisplay_path(NULL),
		_line(0),
		_column(0)
	{
	}

	XMLLocation(const char* display_path, int line, int column)
	 :	_pdisplay_path(display_path),
		_line(line),
		_column(column)
	{
	}

	std::string str() const;

protected:
	const char*	_pdisplay_path;	// character pointer for fast reference
	int	_line;
	int	_column;
};
#endif


enum PRETTY_FLAGS {
	PRETTY_PLAIN	= 0,
	PRETTY_LINEFEED	= 1,
	PRETTY_INDENT	= 2
};


 /// XML Stylesheet entry
struct StyleSheet
{
	std::string	_href;		// CDATA #REQUIRED
	std::string	_type;		// CDATA #REQUIRED
	std::string	_title;		// CDATA #IMPLIED
	std::string	_media;		// CDATA #IMPLIED
	std::string	_charset;	// CDATA #IMPLIED
	bool		_alternate;	// (yes|no) "no"

	StyleSheet() : _alternate(false) {}

	StyleSheet(const std::string& href, const std::string& type="text/xsl", bool alternate=false)
	 :	_href(href),
		_type(type),
		_alternate(alternate)
	{
	}

	bool empty() const {return _href.empty();}
	void print(std::ostream& out) const;
};

 /// list of StyleSheet entries
struct StyleSheetList : public std::list<StyleSheet>
{
	void set(const StyleSheet& stylesheet)
	{
		clear();
		push_back(stylesheet);
	}
};


 /// XML document type description
struct DocType
{
	std::string	_name;

	 // External Document Types are noted, but not parsed.
	std::string	_public;
	std::string	_system;

	 // Internal DTDs are not supported.

	void parse(const char* str);
	bool empty() const {return _name.empty();}
};

 /// Management of XML file headers and formating
struct XMLFormat
{
	XMLFormat(PRETTY_FLAGS pretty=PRETTY_INDENT, const std::string& xml_version="1.0", const std::string& encoding="utf-8", const DocType& doctype=DocType())
	 :	_pretty(pretty),
		_endl("\n"),
		_version(xml_version),
		_encoding(encoding),
		_doctype(doctype),
		_standalone(-1)
	{
	}

	void print_header(std::ostream& out, bool lf=true) const;

	PRETTY_FLAGS _pretty;
	const char*	_endl;		// line ending string: "\n" or "\r\n"

	std::string _version;
	std::string _encoding;

	DocType		_doctype;

	StyleSheetList _stylesheets;

//	std::string _additional;

	int		_standalone;
};


enum WRITE_MODE {
	FORMAT_PLAIN,		/// write XML without any white space
	FORMAT_SMART,		/// preserve original white space and comments if present; pretty print otherwise
	FORMAT_ORIGINAL,	/// write XML stream preserving original white space and comments
	FORMAT_PRETTY 		/// pretty print node to stream without preserving original white space
};


 /// in memory representation of an XML node
struct XMLNode : public XS_String
{
#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	 /// map of XML node attributes
	 // optimized read access without temporary A/U conversion when using ASCII attribute names
	struct AttributeMap : public std::map<XS_String, XS_String>
	{
		typedef std::map<XS_String, XS_String> super;

		const_iterator find(const char* x) const
		{
			for(const_iterator it=begin(); it!=end(); ++it)
				if (it->first == x)
					return it;

			return end();
		}

		const_iterator find(const key_type& x) const
		{
			return super::find(x);
		}

		iterator find(const key_type& x)
		{
			return super::find(x);
		}

		XS_String get(const char* x, LPCXSSTR def=XS_EMPTY_STR) const
		{
			const_iterator found = find(x);

			if (found != end())
				return found->second;
			else
				return def;
		}
	};
#else
	 /// map of XML node attributes
	struct AttributeMap : public std::map<XS_String, XS_String>
	{
		XS_String get(const char* x, LPCXSSTR def=XS_EMPTY_STR) const
		{
			const_iterator found = find(x);

			if (found != end())
				return found->second;
			else
				return def;
		}
	};
#endif

	 /// internal children node list
	struct Children : public std::list<XMLNode*>
	{
		void assign(const Children& other)
		{
			clear();

			for(Children::const_iterator it=other.begin(); it!=other.end(); ++it)
				push_back(new XMLNode(**it));
		}

		void clear()
		{
			while(!empty()) {
				XMLNode* node = back();
				pop_back();

				node->clear();
				delete node;
			}
		}
	};

	 // access to protected class members for XMLPos and XMLReader
	friend struct XMLPos;
	friend struct const_XMLPos;
	friend struct XMLReaderBase;

	XMLNode(const XS_String& name)
	 :	XS_String(name)
	{
	}

	XMLNode(const XS_String& name, const std::string& leading)
	 :	XS_String(name),
		_leading(leading)
	{
	}

	XMLNode(const XMLNode& other)
	 :	XS_String(other),
		_attributes(other._attributes),
		_leading(other._leading),
		_content(other._content),
		_end_leading(other._end_leading),
		_trailing(other._trailing)
#ifdef XMLNODE_LOCATION
		, _location(other._location)
#endif
	{
		for(Children::const_iterator it=other._children.begin(); it!=other._children.end(); ++it)
			_children.push_back(new XMLNode(**it));
	}

	virtual ~XMLNode()
	{
		while(!_children.empty()) {
			delete _children.back();
			_children.pop_back();
		}
	}

	void clear()
	{
		_leading.erase();
		_content.erase();
		_end_leading.erase();
		_trailing.erase();

		_attributes.clear();
		_children.clear();

		XS_String::erase();
	}

	XMLNode& operator=(const XMLNode& other)
	{
		_children.assign(other._children);

		_attributes = other._attributes;

		_leading = other._leading;
		_content = other._content;
		_end_leading = other._end_leading;
		_trailing = other._trailing;

		return *this;
	}

	 /// add a new child node
	void add_child(XMLNode* child)
	{
		_children.push_back(child);
	}

	 /// write access to an attribute
	void put(const XS_String& attr_name, const XS_String& value)
	{
		_attributes[attr_name] = value;
	}

	 /// index operator write access to an attribute
	XS_String& operator[](const XS_String& attr_name)
	{
		return _attributes[attr_name];
	}

	 /// read only access to an attribute
	template<typename T> XS_String get(const T& attr_name, LPCXSSTR def=XS_EMPTY_STR) const
	{
		AttributeMap::const_iterator found = _attributes.find(attr_name);

		if (found != _attributes.end())
			return found->second;
		else
			return def;
	}

	 /// convenient value access in children node
	XS_String subvalue(const XS_String& name, const XS_String& attr_name, int n=0) const
	{
		const XMLNode* node = find(name, n);

		if (node)
			return node->get(attr_name);
		else
			return XS_String();
	}

	 /// convenient storage of distinct values in children node
	XS_String& subvalue(const XS_String& name, const XS_String& attr_name, int n=0)
	{
		XMLNode* node = find(name, n);

		if (!node) {
			node = new XMLNode(name);
			add_child(node);
		}

		return (*node)[attr_name];
	}

#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	 /// convenient value access in children node
	XS_String subvalue(const char* name, const char* attr_name, int n=0) const
	{
		const XMLNode* node = find(name, n);

		if (node)
			return node->get(attr_name);
		else
			return XS_String();
	}

	 /// convenient storage of distinct values in children node
	XS_String& subvalue(const char* name, const XS_String& attr_name, int n=0)
	{
		XMLNode* node = find(name, n);

		if (!node) {
			node = new XMLNode(name);
			add_child(node);
		}

		return (*node)[attr_name];
	}
#endif

	const Children& get_children() const
	{
		return _children;
	}

	Children& get_children()
	{
		return _children;
	}

	const AttributeMap& get_attributes() const
	{
		return _attributes;
	}

	AttributeMap& get_attributes()
	{
		return _attributes;
	}

	XS_String get_content() const
	{
#ifdef XS_STRING_UTF8
		const XS_String& ret = _content;
#else
		XS_String ret;
		assign_utf8(ret, _content.c_str(), _content.length());
#endif

		return DecodeXMLString(ret.c_str());
	}

	void set_content(const XS_String& s, bool cdata=false)
	{
		_content.assign(EncodeXMLString(s.c_str(), cdata));
	}

#ifdef XMLNODE_LOCATION
	const XMLLocation& get_location() const {return _location;}
#endif

	 /// write node with children tree to output stream
	bool write(std::ostream& out, const XMLFormat& format, WRITE_MODE mode=FORMAT_SMART, int indent=0) const
	{
		switch(mode) {
		  case FORMAT_PLAIN:
			plain_write_worker(out);
			break;

		  case FORMAT_PRETTY:
			pretty_write_worker(out, format, indent);
			break;

		  case FORMAT_ORIGINAL:
			write_worker(out);
			break;

		default:	 // FORMAT_SMART
			smart_write_worker(out, format, indent);
		}

		return out.good();
	}

protected:
	Children _children;
	AttributeMap _attributes;

	std::string _leading;		// UTF-8 encoded
	std::string _content;		// UTF-8 and entity encoded, may contain CDATA sections; decode with DecodeXMLString()
	std::string _end_leading;	// UTF-8 encoded
	std::string _trailing;		// UTF-8 encoded

#ifdef XMLNODE_LOCATION
	XMLLocation	_location;
#endif

	XMLNode* get_first_child() const
	{
		if (!_children.empty())
			return _children.front();
		else
			return NULL;
	}

	XMLNode* find(const XS_String& name, int n=0) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			if (**it == name)
				if (!n--)
					return *it;

		return NULL;
	}

	XMLNode* find(const XS_String& name, const XS_String& attr_name, const XS_String& attr_value, int n=0) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
			const XMLNode& node = **it;

			if (node==name && node.get(attr_name)==attr_value)
				if (!n--)
					return *it;
		}

		return NULL;
	}

#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	XMLNode* find(const char* name, int n=0) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			if (**it == name)
				if (!n--)
					return *it;

		return NULL;
	}

	template<typename T, typename U>
	XMLNode* find(const char* name, const T& attr_name, const U& attr_value, int n=0) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
			const XMLNode& node = **it;

			if (node==name && node.get(attr_name)==attr_value)
				if (!n--)
					return *it;
		}

		return NULL;
	}
#endif

	 /// XPath find function (const)
	const XMLNode* find_relative(const char* path) const;

	 /// XPath find function
	XMLNode* find_relative(const char* path)
		{return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->find_relative(path));}

	 /// relative XPath create function
	XMLNode* create_relative(const char* path);

	void	write_worker(std::ostream& out) const;
	void	plain_write_worker(std::ostream& out) const;
	void	pretty_write_worker(std::ostream& out, const XMLFormat& format, int indent) const;
	void	smart_write_worker(std::ostream& out, const XMLFormat& format, int indent) const;

protected:
	XMLNode* get_child_relative(const char*& path, bool create); // mutable for create==true
};


 /// iterator access to children nodes with name filtering
struct XMLChildrenFilter
{
	XMLChildrenFilter(XMLNode::Children& children, const XS_String& name)
	 :	_begin(children.begin(), children.end(), name),
		_end(children.end(), children.end(), name)
	{
	}

	XMLChildrenFilter(XMLNode* node, const XS_String& name)
	 :	_begin(node->get_children().begin(), node->get_children().end(), name),
		_end(node->get_children().end(), node->get_children().end(), name)
	{
	}

	 /// internal iterator class
	struct iterator
	{
		typedef XMLNode::Children::iterator BaseIterator;

		iterator(BaseIterator begin, BaseIterator end, const XS_String& filter_name)
		 :	_cur(begin),
			_end(end),
			_filter_name(filter_name)
		{
			search_next();
		}

		operator BaseIterator()
		{
			return _cur;
		}

		const XMLNode* operator*() const
		{
			return *_cur;
		}

		XMLNode* operator*()
		{
			return *_cur;
		}

		iterator& operator++()
		{
			++_cur;
			search_next();

			return *this;
		}

		iterator operator++(int)
		{
			iterator ret = *this;

			++_cur;
			search_next();

			return ret;
		}

		bool operator==(const BaseIterator& other) const
		{
			return _cur == other;
		}

		bool operator!=(const BaseIterator& other) const
		{
			return _cur != other;
		}

	protected:
		BaseIterator	_cur;
		BaseIterator	_end;
		XS_String	_filter_name;

		void search_next()
		{
			while(_cur!=_end && **_cur!=_filter_name)
				++_cur;
		}
	};

	iterator begin()
	{
		return _begin;
	}

	iterator end()
	{
		return _end;
	}

protected:
	iterator	_begin;
	iterator	_end;
};


 /// read only iterator access to children nodes with name filtering
struct const_XMLChildrenFilter
{
	const_XMLChildrenFilter(const XMLNode::Children& children, const XS_String& name)
	 :	_begin(children.begin(), children.end(), name),
		_end(children.end(), children.end(), name)
	{
	}

	const_XMLChildrenFilter(const XMLNode* node, const XS_String& name)
	 :	_begin(node->get_children().begin(), node->get_children().end(), name),
		_end(node->get_children().end(), node->get_children().end(), name)
	{
	}

	 /// internal iterator class
	struct const_iterator
	{
		typedef XMLNode::Children::const_iterator BaseIterator;

		const_iterator(BaseIterator begin, BaseIterator end, const XS_String& filter_name)
		 :	_cur(begin),
			_end(end),
			_filter_name(filter_name)
		{
			search_next();
		}

		operator BaseIterator()
		{
			return _cur;
		}

		const XMLNode* operator*() const
		{
			return *_cur;
		}

		const_iterator& operator++()
		{
			++_cur;
			search_next();

			return *this;
		}

		const_iterator operator++(int)
		{
			const_iterator ret = *this;

			++_cur;
			search_next();

			return ret;
		}

		bool operator==(const BaseIterator& other) const
		{
			return _cur == other;
		}

		bool operator!=(const BaseIterator& other) const
		{
			return _cur != other;
		}

	protected:
		BaseIterator	_cur;
		BaseIterator	_end;
		XS_String	_filter_name;

		void search_next()
		{
			while(_cur!=_end && **_cur!=_filter_name)
				++_cur;
		}
	};

	const_iterator begin()
	{
		return _begin;
	}

	const_iterator end()
	{
		return _end;
	}

protected:
	const_iterator	_begin;
	const_iterator	_end;
};


 /// iterator for XML trees
struct XMLPos
{
	XMLPos(XMLNode* root)
	 :	_root(root),
		_cur(root)
	{
	}

	XMLPos(const XMLPos& other)
	 :	_root(other._root),
		_cur(other._cur)
	{	// don't copy _stack
	}

	XMLPos(XMLNode* node, const XS_String& name)
	 :	_root(node),
		_cur(node)
	{
		smart_create(name);
	}

	XMLPos(XMLNode* node, const XS_String& name, const XS_String& attr_name, const XS_String& attr_value)
	 :	_root(node),
		_cur(node)
	{
		smart_create(name, attr_name, attr_value);
	}

	XMLPos(const XMLPos& other, const XS_String& name)
	 :	_root(other._root),
		_cur(other._cur)
	{
		smart_create(name);
	}

	XMLPos(const XMLPos& other, const XS_String& name, const XS_String& attr_name, const XS_String& attr_value)
	 :	_root(other._root),
		_cur(other._cur)
	{
		smart_create(name, attr_name, attr_value);
	}

	 /// access to current node
	XMLNode& cur()
	{
		return *_cur;
	}

	const XMLNode& cur() const
	{
		return *_cur;
	}

	 /// automatic access to current node
	operator const XMLNode*() const {return _cur;}
	operator XMLNode*() {return _cur;}

	const XMLNode* operator->() const {return _cur;}
	XMLNode* operator->() {return _cur;}

	const XMLNode& operator*() const {return *_cur;}
	XMLNode& operator*() {return *_cur;}

	 /// attribute access
	XS_String get(const XS_String& attr_name, LPCXSSTR def=XS_EMPTY_STR) const
	{
		return _cur->get(attr_name, def);
	}

	 /// attribute setting
	void put(const XS_String& attr_name, const XS_String& value)
	{
		_cur->put(attr_name, value);
	}

	 /// index operator attribute access
	template<typename T> XS_String get(const T& attr_name) const {return (*_cur)[attr_name];}
	XS_String& operator[](const XS_String& attr_name) {return (*_cur)[attr_name];}

	 /// insert children when building tree
	void add_down(XMLNode* child)
	{
		_cur->add_child(child);
		go_to(child);
	}

	 /// go back to previous position
	bool back()
	{
		if (!_stack.empty()) {
			_cur = _stack.top();
			_stack.pop();
			return true;
		} else
			return false;
	}

	 /// go down to first child
	bool go_down()
	{
		XMLNode* node = _cur->get_first_child();

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// search for child and go down
	bool go_down(const XS_String& name, int n=0)
	{
		XMLNode* node = _cur->find(name, n);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// move XPath like to position in XML tree
	bool go(const char* path);

	 /// create child nodes using XPath notation and move to the deepest child
	bool create_relative(const char* path)
	{
		XMLNode* node = _cur->create_relative(path);
		if (!node)
			return false;	// invalid path specified

		go_to(node);
		return true;
	}

	 /// create node and move to it
	void create(const XS_String& name)
	{
		add_down(new XMLNode(name));
	}

	 /// create node if not already existing and move to it
	void smart_create(const XS_String& name)
	{
		XMLNode* node = _cur->find(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	void smart_create(const XS_String& name, const XS_String& attr_name, const XS_String& attr_value)
	{
		XMLNode* node = _cur->find(name, attr_name, attr_value);

		if (node)
			go_to(node);
		else {
			node = new XMLNode(name);
			add_down(node);
			(*node)[attr_name] = attr_value;
		}
	}

#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	 /// search for child and go down
	bool go_down(const char* name, int n=0)
	{
		XMLNode* node = _cur->find(name, n);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// create node and move to it
	void create(const char* name)
	{
		add_down(new XMLNode(name));
	}

	 /// create node if not already existing and move to it
	void smart_create(const char* name)
	{
		XMLNode* node = _cur->find(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	template<typename T, typename U>
	void smart_create(const char* name, const T& attr_name, const U& attr_value)
	{
		XMLNode* node = _cur->find(name, attr_name, attr_value);

		if (node)
			go_to(node);
		else {
			XMLNode* node = new XMLNode(name);
			add_down(node);
			(*node)[attr_name] = attr_value;
		}
	}
#endif

	XS_String& str() {return *_cur;}
	const XS_String& str() const {return *_cur;}

protected:
	XMLNode* _root;
	XMLNode* _cur;
	std::stack<XMLNode*> _stack;

	 /// go to specified node
	void go_to(XMLNode* child)
	{
		_stack.push(_cur);
		_cur = child;
	}
};


 /// iterator for XML trees
struct const_XMLPos
{
	const_XMLPos(const XMLNode* root)
	 :	_root(root),
		_cur(root)
	{
	}

	const_XMLPos(const const_XMLPos& other)
	 :	_root(other._root),
		_cur(other._cur)
	{	// don't copy _stack
	}

	 /// access to current node
	const XMLNode& cur() const
	{
		return *_cur;
	}

	 /// automatic access to current node
	operator const XMLNode*() const {return _cur;}

	const XMLNode* operator->() const {return _cur;}

	const XMLNode& operator*() const {return *_cur;}

	 /// attribute access
	XS_String get(const XS_String& attr_name) const
	{
		return _cur->get(attr_name);
	}

	 /// index operator attribute access
	template<typename T> XS_String get(const T& attr_name) const {return _cur->get(attr_name);}

	 /// go back to previous position
	bool back()
	{
		if (!_stack.empty()) {
			_cur = _stack.top();
			_stack.pop();
			return true;
		} else
			return false;
	}

	 /// go down to first child
	bool go_down()
	{
		const XMLNode* node = _cur->get_first_child();

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// search for child and go down
	bool go_down(const XS_String& name, int n=0)
	{
		XMLNode* node = _cur->find(name, n);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// move XPath like to position in XML tree
	bool go(const char* path);

#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	 /// search for child and go down
	bool go_down(const char* name, int n=0)
	{
		XMLNode* node = _cur->find(name, n);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}
#endif

	const XS_String& str() const {return *_cur;}

protected:
	const XMLNode* _root;
	const XMLNode* _cur;
	std::stack<const XMLNode*> _stack;

	 /// go to specified node
	void go_to(const XMLNode* child)
	{
		_stack.push(_cur);
		_cur = child;
	}
};


 /// type converter for boolean data
struct XMLBool
{
	XMLBool(bool value=false)
	 :	_value(value)
	{
	}

	XMLBool(LPCXSSTR value, bool def=false)
	{
		if (value && *value)
			_value = !XS_icmp(value, XS_TRUE);
		else
			_value = def;
	}

	XMLBool(const XMLNode* node, const XS_String& attr_name, bool def=false)
	{
		const XS_String& value = node->get(attr_name);

		if (!value.empty())
			_value = !XS_icmp(value.c_str(), XS_TRUE);
		else
			_value = def;
	}

	operator bool() const
	{
		return _value;
	}

	bool operator!() const
	{
		return !_value;
	}

	operator LPCXSSTR() const
	{
		return _value? XS_TRUE: XS_FALSE;
	}

protected:
	bool	_value;

private:
	void operator=(const XMLBool&); // disallow assignment operations
};

 /// type converter for boolean data with write access
struct XMLBoolRef
{
	XMLBoolRef(XMLNode* node, const XS_String& attr_name, bool def=false)
	 :	_ref((*node)[attr_name])
	{
		if (_ref.empty())
			assign(def);
	}

	operator bool() const
	{
		return !XS_icmp(_ref.c_str(), XS_TRUE);
	}

	bool operator!() const
	{
		return XS_icmp(_ref.c_str(), XS_TRUE)? true: false;
	}

	XMLBoolRef& operator=(bool value)
	{
		assign(value);

		return *this;
	}

	void assign(bool value)
	{
		_ref.assign(value? XS_TRUE: XS_FALSE);
	}

	void toggle()
	{
		assign(!operator bool());
	}

protected:
	XS_String& _ref;
};


 /// type converter for integer data
struct XMLInt
{
	XMLInt(int value)
	 :	_value(value)
	{
	}

	XMLInt(LPCXSSTR value, int def=0)
	{
		if (value && *value)
			_value = XS_toi(value);
		else
			_value = def;
	}

	XMLInt(const XMLNode* node, const XS_String& attr_name, int def=0)
	{
		const XS_String& value = node->get(attr_name);

		if (!value.empty())
			_value = XS_toi(value.c_str());
		else
			_value = def;
	}

	operator int() const
	{
		return _value;
	}

	operator XS_String() const
	{
		XS_CHAR buffer[32];
		XS_snprintf(buffer, COUNTOF(buffer), XS_INTFMT, _value);
		return buffer;
	}

protected:
	int _value;

private:
	void operator=(const XMLInt&); // disallow assignment operations
};

 /// type converter for integer data with write access
struct XMLIntRef
{
	XMLIntRef(XMLNode* node, const XS_String& attr_name, int def=0)
	 :	_ref((*node)[attr_name])
	{
		if (_ref.empty())
			assign(def);
	}

	XMLIntRef& operator=(int value)
	{
		assign(value);

		return *this;
	}

	operator int() const
	{
		return XS_toi(_ref.c_str());
	}

	void assign(int value)
	{
		XS_CHAR buffer[32];
		XS_snprintf(buffer, COUNTOF(buffer), XS_INTFMT, value);
		_ref.assign(buffer);
	}

protected:
	XS_String& _ref;
};


 /// type converter for numeric data
struct XMLDouble
{
	XMLDouble(double value)
	 :	_value(value)
	{
	}

	XMLDouble(LPCXSSTR value, double def=0.)
	{
		LPTSTR end;

		if (value && *value)
			_value = XS_tod(value, &end);
		else
			_value = def;
	}

	XMLDouble(const XMLNode* node, const XS_String& attr_name, double def=0.)
	{
		LPTSTR end;
		const XS_String& value = node->get(attr_name);

		if (!value.empty())
			_value = XS_tod(value.c_str(), &end);
		else
			_value = def;
	}

	operator double() const
	{
		return _value;
	}

	operator XS_String() const
	{
		XS_CHAR buffer[32];
		XS_snprintf(buffer, COUNTOF(buffer), XS_FLOATFMT, _value);
		return buffer;
	}

protected:
	double _value;

private:
	void operator=(const XMLDouble&); // disallow assignment operations
};

 /// type converter for numeric data with write access
struct XMLDoubleRef
{
	XMLDoubleRef(XMLNode* node, const XS_String& attr_name, double def=0.)
	 :	_ref((*node)[attr_name])
	{
		if (_ref.empty())
			assign(def);
	}

	XMLDoubleRef& operator=(double value)
	{
		assign(value);

		return *this;
	}

	operator double() const
	{
		LPTSTR end;
		return XS_tod(_ref.c_str(), &end);
	}

	void assign(double value)
	{
		XS_CHAR buffer[32];
		XS_snprintf(buffer, COUNTOF(buffer), XS_FLOATFMT, value);
		_ref.assign(buffer);
	}

protected:
	XS_String& _ref;
};


 /// type converter for string data
struct XMLString
{
	XMLString(const XS_String& value)
	 :	_value(value)
	{
	}

	XMLString(LPCXSSTR value, LPCXSSTR def=XS_EMPTY)
	{
		if (value && *value)
			_value = value;
		else
			_value = def;
	}

	XMLString(const XMLNode* node, const XS_String& attr_name, LPCXSSTR def=XS_EMPTY)
	{
		const XS_String& value = node->get(attr_name);

		if (!value.empty())
			_value = value;
		else
			_value = def;
	}

	operator const XS_String&() const
	{
		return _value;
	}

	const XS_String& c_str() const
	{
		return _value;
	}

protected:
	XS_String	_value;

private:
	void operator=(const XMLString&); // disallow assignment operations
};

 /// type converter for string data with write access
struct XMLStringRef
{
	XMLStringRef(XMLNode* node, const XS_String& attr_name, LPCXSSTR def=XS_EMPTY)
	 :	_ref((*node)[attr_name])
	{
		if (_ref.empty())
			assign(def);
	}

	XMLStringRef(const XS_String& node_name, XMLNode* node, const XS_String& attr_name, LPCXSSTR def=XS_EMPTY)
	 :	_ref(node->subvalue(node_name, attr_name))
	{
		if (_ref.empty())
			assign(def);
	}

	XMLStringRef& operator=(const XS_String& value)
	{
		assign(value);

		return *this;
	}

	operator const XS_String&() const
	{
		return _ref;
	}

	void assign(const XS_String& value)
	{
		_ref.assign(value);
	}

protected:
	XS_String& _ref;
};


template<typename T>
	inline void read_option(T& var, const_XMLPos& cfg, LPCXSSTR key)
	{
		const XS_String& val = cfg.get(key);

		if (!val.empty())
			var = val;
	}

template<>
	inline void read_option(int& var, const_XMLPos& cfg, LPCXSSTR key)
	{
		const XS_String& val = cfg.get(key);

		if (!val.empty())
			var = XS_toi(val.c_str());
	}


#ifdef _MSC_VER
#pragma warning(disable: 4355)
#endif

 /// XML reader base class
struct XMLReaderBase
#ifdef XS_USE_XERCES
 : public HandlerBase
#endif
{
#ifdef XS_USE_XERCES

	XMLReaderBase(XMLNode* node, InputSource* source, bool adoptSource=false);
	virtual ~XMLReaderBase();

	void read();

protected:
	SAXParser*	_parser;
	InputSource* _source;
	bool		_deleteSource;

	virtual void XMLDecl(const XMLCh* const versionStr, const XMLCh* const encodingStr,
						 const XMLCh* const standaloneStr, const XMLCh* const actualEncodingStr);

     // Handlers for the SAX DocumentHandler interface
	virtual void setDocumentLocator(const Locator* const locator);
	virtual void startElement(const XMLCh* const name, AttributeList& attributes);
    virtual void endElement(const XMLCh* const name);
    virtual void characters(const XMLCh* const chars, const unsigned int length);
    virtual void ignorableWhitespace(const XMLCh* const chars, const unsigned int length);

     // Handlers for the SAX ErrorHandler interface
    virtual void error(const SAXParseException& e);
    virtual void fatalError(const SAXParseException& e);
	virtual void warning(const SAXParseException& e);
    virtual void resetErrors();

#elif defined(XS_USE_EXPAT) // !XS_USE_XERCES

	XMLReaderBase(XMLNode* node);
	virtual ~XMLReaderBase();

protected:
	XML_Parser	_parser;

	static void XMLCALL XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone=-1);
	static void XMLCALL XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
	static void XMLCALL XML_EndElementHandler(void* userData, const XML_Char* name);
	static void XMLCALL XML_DefaultHandler(void* userData, const XML_Char* s, int len);

	static std::string get_expat_error_string(XML_Error error_code);

#else // XS_USE_EXPAT

	XMLReaderBase(XMLNode* node)
	 :	_pos(node),
		_endl_defined(false),
		_utf8(false)
	{
		_last_tag = TAG_NONE;
	}

	virtual ~XMLReaderBase();

	bool	parse();

#endif

public:
#ifndef XS_USE_XERCES
	void read();

	std::string	get_position() const;
#endif
	const XMLFormat& get_format() const {return _format;}
	const char* get_endl() const {return _endl_defined? _format._endl: "\n";}

	const XMLErrorList& get_errors() const {return _errors;}
	const XMLErrorList& get_warnings() const {return _warnings;}

	void clear_errors() {_errors.clear(); _warnings.clear();}

#ifdef XMLNODE_LOCATION
	const char* _display_path;	// character pointer for fast reference in XMLLocation

#ifdef XS_USE_XERCES
	const Locator* _locator;
#endif

	XMLLocation get_location() const;
#endif

protected:
	XMLPos		_pos;

	std::string _content;		// UTF-8 encoded
	enum {TAG_NONE, TAG_START, TAG_END} _last_tag;

	XMLErrorList _errors;
	XMLErrorList _warnings;

	XMLFormat	_format;
	bool	_endl_defined;

#ifdef XS_USE_XERCES
	//@@
#elif defined(XS_USE_EXPAT)
	virtual int read_buffer(char* buffer, int len) = 0;
#else
	virtual int get() = 0;
	int		eat_endl();

	bool	_utf8;
#endif

	void	finish_read();

	virtual void XmlDeclHandler(const char* version, const char* encoding, int standalone);
	virtual void StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attributes);
	virtual void EndElementHandler();
#if defined(XS_USE_XERCES) || defined(XS_USE_EXPAT)
	virtual void DefaultHandler(const XML_Char* s, int len);
#else
	virtual void DefaultHandler(const std::string& s);
#endif
};


 /// XML file reader

#ifdef XS_USE_XERCES

struct XercesXMLReader : public XMLReaderBase
{
	XercesXMLReader(XMLNode* node, InputSource* source, bool adoptSource=false)
	 :	XMLReaderBase(node, source, adoptSource)
	{
	}

	XercesXMLReader(XMLNode* node, LPCTSTR path);
	XercesXMLReader(XMLNode* node, const XMLByte* buffer, size_t bytes, const std::string& system_id=std::string());
};

#define XMLReader XercesXMLReader

#elif defined(XS_USE_EXPAT)

struct ExpatXMLReader : public XMLReaderBase
{
	ExpatXMLReader(XMLNode* node, std::istream& in)
	 :	XMLReaderBase(node),
		_in(in)
	{
	}

	 /// read XML stream into XML tree below _pos
	int read_buffer(char* buffer, int len)
	{
		if (!_in.good())
			return -1;

		_in.read(buffer, len);

		return _in.gcount();
	}

protected:
	std::istream&	_in;
};

#define XMLReader ExpatXMLReader

#else // XS_USE_XERCES, XS_USE_EXPAT

struct XMLReader : public XMLReaderBase
{
	XMLReader(XMLNode* node, std::istream& in)
	 :	XMLReaderBase(node),
		_in(in)
	{
	}

	 /// read one character from XML stream
	int get()
	{
		return _in.get();
	}

protected:
	std::istream&	_in;
};

#endif // XS_USE_XERCES


#if defined(_MSC_VER) && _MSC_VER<1400

struct fast_ostringbuffer : public std::streambuf
{
	typedef char _E;
	typedef std::char_traits<_E> _Tr;

	explicit fast_ostringbuffer()
		{_Init(0, 0, std::_Noread);}	// optimized for ios::out mode

	virtual ~fast_ostringbuffer()
		{_Tidy();}

	std::string str() const
		{if (pptr() != 0)
			{std::string _Str(pbase(),
				(_Seekhigh<pptr()? pptr(): _Seekhigh) - pbase());
			return _Str;}
		else
			return std::string();}

protected:
	virtual int_type overflow(int_type _C = _Tr::eof())
		{if (_Tr::eq_int_type(_Tr::eof(), _C))
			return _Tr::not_eof(_C);
		else if (pptr() != 0 && pptr() < epptr())
			{*_Pninc() = _Tr::to_char_type(_C);
			return _C;}
		else
			{size_t _Os = gptr() == 0 ? 0 : epptr() - eback();
			size_t _Ns = _Os + _Alsize;
			_E *_P = _Al.allocate(_Ns, (void *)0);
			if (0 < _Os)
				_Tr::copy(_P, eback(), _Os);
			else if (_ALSIZE < _Alsize)
				_Alsize = _ALSIZE;

			if (_Strmode & std::_Allocated)
				_Al.deallocate(eback(), _Os);

			_Strmode |= std::_Allocated;

			if (_Os == 0)
				{_Seekhigh = _P;
				setp(_P, _P + _Ns);
				setg(_P, _P, _P); }
			else
				{_Seekhigh = _Seekhigh - eback() + _P;
				setp(pbase() - eback() + _P, pptr() - eback() + _P, _P + _Ns);
				setg(_P, _P, _P);}
			*_Pninc() = _Tr::to_char_type(_C);

			return _C;}}

	void _Init(const _E *_S, size_t _N, std::_Strstate _M)
		{_Pendsave = 0, _Seekhigh = 0;
		_Alsize = _MINSIZE, _Strmode = _M;
		setg(0, 0, 0);
		setp(0, 0);}

	void _Tidy()
		{if (_Strmode & std::_Allocated)
			_Al.deallocate(eback(), (pptr() != 0 ? epptr() : egptr()) - eback());
		_Seekhigh = 0;
		_Strmode &= ~std::_Allocated;}

private:
	enum {_ALSIZE = 65536/*512*/, _MINSIZE = 32768/*32*/};	// bigger buffer sizes

	_E *_Pendsave, *_Seekhigh;
	int _Alsize;
	std::_Strstate _Strmode;
	std::allocator<_E> _Al;
};

struct fast_ostringstream : public std::iostream
{
	typedef std::iostream super;

	explicit fast_ostringstream()
		: super(&_Sb) {}

	std::string str() const
		{return _Sb.str();}

private:
	fast_ostringbuffer _Sb;
};

#else

typedef std::ostringstream fast_ostringstream;

#endif


 /// XML document holder
struct XMLDoc : public XMLNode
{
	XMLDoc()
	 :	XMLNode("")
	{
	}

	XMLDoc(LPCTSTR path)
	 :	XMLNode("")
	{
		read_file(path);
	}

#ifdef XS_USE_XERCES
	bool read_file(LPCTSTR path)
	{
		XMLReader reader(this, path);

#if defined(_STRING_DEFINED) && !defined(XS_STRING_UTF8)
		return read(reader, std::string(ANS(path)));
#else
		return read(reader, XS_String(path));
#endif
	}

	bool read_buffer(const char* buffer, size_t len, const std::string& system_id=std::string())
	{
		XMLReader reader(this, (const XMLByte*)buffer, len, system_id);

		return read(reader, system_id);
	}

#else // XS_USE_XERCES

	bool read_file(LPCTSTR path)
	{
		tifstream in(path);
		XMLReader reader(this, in);

#if defined(_STRING_DEFINED) && !defined(XS_STRING_UTF8)
		return read(reader, std::string(ANS(path)));
#else
		return read(reader, XS_String(path));
#endif
	}

	bool read_buffer(const char* buffer, size_t len, const std::string& system_id=std::string())
	{
		std::istringstream in(std::string(buffer, len));

		return read(in, system_id);
	}

	bool read(std::istream& in, const std::string& system_id=std::string())
	{
		XMLReader reader(this, in);

		return read(reader, system_id);
	}
#endif // XS_USE_XERCES

	bool read(XMLReaderBase& reader, const std::string& display_path)
	{
#ifdef XMLNODE_LOCATION
		 // make a string copy to handle temporary string objects
		_display_path = display_path;
		reader._display_path = _display_path.c_str();
#endif

		reader.clear_errors();
		reader.read();

		_format = reader.get_format();
		_format._endl = reader.get_endl();

		if (!reader.get_errors().empty()) {
			_errors = reader.get_errors();
			return false;
		}

		return true;
	}

	 /// write XML stream
	 // FORMAT_SMART: preserving previous white space and comments
	bool write(std::ostream& out, WRITE_MODE mode=FORMAT_SMART) const
	{
		_format.print_header(out, mode!=FORMAT_PLAIN);

		if (_children.size() == 1)
			_children.front()->write(out, _format, mode);
		else if (!_children.empty()) {
			//throw Exception("more than one XML root!");
			return false;
		}

		return out.good();
	}

	 /// write XML stream with formating
	bool write_formating(std::ostream& out) const
	{
		return write(out, FORMAT_PRETTY);
	}

	bool write_file(LPCTSTR path, WRITE_MODE mode=FORMAT_SMART) const
	{
		tofstream out(path);

		return write(out, mode);
	}

	bool write_formating(LPCTSTR path) const
	{
		tofstream out(path);

		return write_formating(out);
	}

	XMLFormat		_format;
	XMLErrorList	_errors;

#ifdef XMLNODE_LOCATION
	std::string		_display_path;
#endif
};


 /// XML message wrapper
struct XMLMessage : public XMLDoc
{
	XMLMessage(const char* name)
	 :	_pos(this)
	{
		_pos.create(name);
	}

	std::string toString() const
	{
		std::ostringstream out;

		write(out);

		return out.str();
	}

	XMLPos	_pos;

protected:
	XMLMessage()
	 :	_pos(this)
	{
	}
};


 /// helper structure to read XML messages from strings
struct XMLMessageFromString : public XMLMessage
{
	XMLMessageFromString(const std::string& xml_str, const std::string& system_id=std::string())
	{
		read_buffer(xml_str.c_str(), xml_str.length(), system_id);
	}
};


 /// Reader for XML Messages
struct XMLMessageReader : public XMLPos
{
	XMLMessageReader(const std::string& xml_str, const std::string& system_id=std::string())
	 :	XMLPos(&_msg)
	{
		_msg.read_buffer(xml_str.c_str(), xml_str.length(), system_id);
	}

	const XMLDoc& get_document()
	{
		return _msg;
	}

protected:
	XMLDoc	_msg;
};


 /// on the fly XML writer
struct XMLWriter
{
	XMLWriter(std::ostream& out, const XMLFormat& format=XMLFormat())
	 :	_pofstream(NULL),
		_out(out),
		_format(format)
	{
		format.print_header(_out, false);	// _format._endl is printed in write_pre()
	}

	XMLWriter(LPCTSTR path, const XMLFormat& format=XMLFormat())
	 :	_pofstream(new tofstream(path)),
		_out(*_pofstream),
		_format(format)
	{
		format.print_header(_out, false);	// _format._endl is printed in write_pre()
	}

	~XMLWriter()
	{
		_out << _format._endl;
		delete _pofstream;
	}

	 /// create node and move to it
	void create(const XS_String& name)
	{
		if (!_stack.empty()) {
			StackEntry& last = _stack.top();

			if (last._state < PRE_CLOSED) {
				write_attributes(last);
				close_pre(last);
			}

			++last._children;
		}

		StackEntry entry;
		entry._node_name = name;
		_stack.push(entry);

		write_pre(entry);
	}

	 /// go back to previous position
	bool back()
	{
		if (!_stack.empty()) {
			write_post(_stack.top());

			_stack.pop();
			return true;
		} else
			return false;
	}

	 /// attribute setting
	void put(const XS_String& attr_name, const XS_String& value)
	{
		if (!_stack.empty())
			_stack.top()._attributes[attr_name] = value;
	}

	 /// index operator write access to an attribute
	XS_String& operator[](const XS_String& attr_name)
	{
		if (_stack.empty())
			return s_empty_attr;

		return _stack.top()._attributes[attr_name];
	}

	void set_content(const XS_String& s, bool cdata=false)
	{
		if (!_stack.empty())
			_stack.top()._content = EncodeXMLString(s.c_str(), cdata);
	}

	 // public for access in StackEntry
	enum WRITESTATE {
		NOTHING, /*PRE,*/ ATTRIBUTES, PRE_CLOSED, /*CONTENT,*/ POST
	};

protected:
	tofstream*		_pofstream;
	std::ostream&	_out;
	const XMLFormat&_format;

	typedef XMLNode::AttributeMap AttrMap;

	 /// container for XMLWriter state information
	struct StackEntry {
		XS_String	_node_name;
		AttrMap		_attributes;
		std::string	_content;
		WRITESTATE	_state;
		bool		_children;

		StackEntry() : _state(NOTHING), _children(false) {}
	};

	std::stack<StackEntry> _stack;

	static XS_String s_empty_attr;

	void close_pre(StackEntry& entry)
	{
		_out << '>';

		entry._state = PRE_CLOSED;
	}

	void write_pre(StackEntry& entry)
	{
		if (_format._pretty >= PRETTY_LINEFEED)
			_out << _format._endl;

		if (_format._pretty == PRETTY_INDENT)
			for(size_t i=_stack.size(); --i>0; )
				_out << XML_INDENT_SPACE;

		_out << '<' << EncodeXMLString(entry._node_name);
		//entry._state = PRE;
	}

	void write_attributes(StackEntry& entry)
	{
		for(AttrMap::const_iterator it=entry._attributes.begin(); it!=entry._attributes.end(); ++it)
			_out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

		entry._state = ATTRIBUTES;
	}

	void write_post(StackEntry& entry)
	{
		if (entry._state < ATTRIBUTES)
			write_attributes(entry);

		if (entry._children || !entry._content.empty()) {
			if (entry._state < PRE_CLOSED)
				close_pre(entry);

			_out << entry._content;
			//entry._state = CONTENT;

			if (_format._pretty>=PRETTY_LINEFEED && entry._content.empty())
				_out << _format._endl;

			if (_format._pretty==PRETTY_INDENT && entry._content.empty())
				for(size_t i=_stack.size(); --i>0; )
					_out << XML_INDENT_SPACE;

			_out << "</" << EncodeXMLString(entry._node_name) << ">";
		} else {
			_out << "/>";
		}

		entry._state = POST;
	}
};


}	// namespace XMLStorage

#define _XMLSTORAGE_H
#endif // _XMLSTORAGE_H
