
 //
 // XML storage classes
 //
 // xmlstorage.h
 //
 // Copyright (c) 2004, Martin Fuchs <martin-fuchs@gmx.net>
 //


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

#include "expat.h"

#ifdef _MSC_VER
#pragma comment(lib, "libexpat.lib")
#pragma warning(disable: 4786)
#endif


#include <windows.h>	// for LPCTSTR

#ifdef UNICODE
#define _UNICODE
#endif

#include <tchar.h>
#include <malloc.h>

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
#define	XS_icmp stricmp
#define	XS_nicmp strnicmp
#define	XS_toi atoi
#define	XS_len strlen
#define	XS_sprintf sprintf
#define	XS_vsprintf vsprintf
#else
#define	XS_CHAR TCHAR
#define	XS_TEXT(x) TEXT(x)
#define LPXSSTR LPTSTR
#define LPCXSSTR LPCTSTR
#define	XS_icmp _tcsicmp
#define	XS_nicmp _tcsnicmp
#define	XS_toi _ttoi
#define	XS_len _tcslen
#define	XS_sprintf _stprintf
#define	XS_vsprintf _vstprintf
#endif

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
	XS_String(LPCXSSTR s, int l) : super(s, l) {}

	XS_String(const super& other) : super(other) {}
	XS_String(const XS_String& other) : super(other) {}

#if defined(UNICODE) && !defined(XS_STRING_UTF8)
	XS_String(LPCSTR s) {assign(s);}
	XS_String(LPCSTR s, int l) {assign(s, l);}
	XS_String(const std::string& other) {assign(other.c_str());}
	XS_String& operator=(LPCSTR s) {assign(s); return *this;}
	void assign(LPCSTR s) {if (s) {int bl=strlen(s); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, bl, b, bl));} else erase();}
	void assign(LPCSTR s, int l) {if (s) {int bl=l; LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, l, b, bl));} else erase();}
#else
	XS_String(LPCWSTR s) {assign(s);}
	XS_String(LPCWSTR s, int l) {assign(s, l);}
	XS_String(const std::wstring& other) {assign(other.c_str());}
	XS_String& operator=(LPCWSTR s) {assign(s); return *this;}
#ifdef XS_STRING_UTF8
	void assign(const XS_String& s) {assign(s.c_str());}
	void assign(LPCWSTR s) {if (s) {int bl=wcslen(s); LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_UTF8, 0, s, bl, b, bl, 0, 0));} else erase();}
	void assign(LPCWSTR s, int l) {int bl=l; if (s) {LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_UTF8, 0, s, l, b, bl, 0, 0));} else erase();}
#else // if !UNICODE && !XS_STRING_UTF8
	void assign(LPCWSTR s) {if (s) {int bl=wcslen(s); LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, bl, b, bl, 0, 0));} else erase();}
	void assign(LPCWSTR s, int l) {int bl=l; if (s) {LPSTR b=(LPSTR)alloca(bl); super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, l, b, bl, 0, 0));} else erase();}
#endif
#endif

	XS_String& operator=(LPCXSSTR s) {if (s) super::assign(s); else erase(); return *this;}
	XS_String& operator=(const super& s) {super::assign(s); return *this;}
	void assign(LPCXSSTR s) {super::assign(s);}
	void assign(LPCXSSTR s, int l) {super::assign(s, l);}

	operator LPCXSSTR() const {return c_str();}

#ifdef XS_STRING_UTF8
	operator std::wstring() const {int bl=length(); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); return std::wstring(b, MultiByteToWideChar(CP_UTF8, 0, c_str(), bl, b, bl));}
#elif defined(UNICODE)
	operator std::string() const {int bl=length(); LPSTR b=(LPSTR)alloca(bl); return std::string(b, WideCharToMultiByte(CP_ACP, 0, c_str(), bl, b, bl, 0, 0));}
#else
	operator std::wstring() const {int bl=length(); LPWSTR b=(LPWSTR)alloca(sizeof(WCHAR)*bl); return std::wstring(b, MultiByteToWideChar(CP_ACP, 0, c_str(), bl, b, bl));}
#endif

	XS_String& printf(LPCXSSTR fmt, ...)
	{
		va_list l;
		XS_CHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::assign(b, XS_vsprintf(b, fmt, l));
		va_end(l);

		return *this;
	}

	XS_String& vprintf(LPCXSSTR fmt, va_list l)
	{
		XS_CHAR b[BUFFER_LEN];

		super::assign(b, XS_vsprintf(b, fmt, l));

		return *this;
	}

	XS_String& appendf(LPCXSSTR fmt, ...)
	{
		va_list l;
		XS_CHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::append(b, XS_vsprintf(b, fmt, l));
		va_end(l);

		return *this;
	}

	XS_String& vappendf(LPCXSSTR fmt, va_list l)
	{
		XS_CHAR b[BUFFER_LEN];

		super::append(b, XS_vsprintf(b, fmt, l));

		return *this;
	}
};

#endif // _STRING_DEFINED, !XS_STRING_UTF8

#endif // XS_String


#ifndef XS_STRING_UTF8

inline void assign_utf8(XS_String& s, const char* str)
{
	int lutf8 = strlen(str);

#ifdef UNICODE
	LPTSTR buffer = (LPTSTR)alloca(sizeof(TCHAR)*lutf8);
	int l = MultiByteToWideChar(CP_UTF8, 0, str, lutf8, buffer, lutf8);
#else
	LPWSTR wbuffer = (LPWSTR)alloca(sizeof(WCHAR)*lutf8);
	int l = MultiByteToWideChar(CP_UTF8, 0, str, lutf8, wbuffer, lutf8);

	int bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_ACP, 0, wbuffer, l, buffer, bl, 0, 0);
#endif

	s.assign(buffer, l);
}

inline std::string get_utf8(LPCTSTR s, int l)
{
#ifdef UNICODE
	int bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_UTF8, 0, s, l, buffer, bl, 0, 0);
#else
	LPWSTR wbuffer = (LPWSTR)alloca(sizeof(WCHAR)*l);
	l = MultiByteToWideChar(CP_ACP, 0, s, l, wbuffer, l);

	int bl=2*l; LPSTR buffer = (LPSTR)alloca(bl);
	l = WideCharToMultiByte(CP_UTF8, 0, wbuffer, l, buffer, bl, 0, 0);
#endif

	return std::string(buffer, l);
}

inline std::string get_utf8(const XS_String& s)
{
	return get_utf8(s.c_str(), s.length());
}

#endif // XS_STRING_UTF8

extern std::string EncodeXMLString(const XS_String& str);
extern XS_String DecodeXMLString(const XS_String& str);


#ifdef __GNUC__
#include <ext/stdio_filebuf.h>
typedef __gnu_cxx::stdio_filebuf<char> STDIO_FILEBUF;
#else
typedef std::filebuf STDIO_FILEBUF;
#endif

 /// input file stream with ANSI/UNICODE file names
struct tifstream : public std::istream
{
	typedef std::istream super;

	tifstream(LPCTSTR path)
	 :	super(&_buf),
		_pfile(_tfopen(path, TEXT("r"))),
#ifdef __GNUC__
		_buf(_pfile, ios::in)
#else
		_buf(_pfile)
#endif
	{
	}

	~tifstream()
	{
		if (_pfile)
			fclose(_pfile);
	}

protected:
	FILE*	_pfile;
	STDIO_FILEBUF _buf;
};

 /// output file stream with ANSI/UNICODE file names
struct tofstream : public std::ostream
{
	typedef std::ostream super;

	tofstream(LPCTSTR path)
	 :	super(&_buf),
		_pfile(_tfopen(path, TEXT("w"))),
#ifdef __GNUC__
		_buf(_pfile, ios::out)
#else
		_buf(_pfile)
#endif
	{
	}

	~tofstream()
	{
		flush();

		if (_pfile)
			fclose(_pfile);
	}

protected:
	FILE*	_pfile;
	STDIO_FILEBUF _buf;
};


 // write XML files with 2 spaces indenting
#define XML_INDENT_SPACE "  "


#ifdef XML_UNICODE	// Are XML_Char strings UTF-16 encoded?

typedef XS_String String_from_XML_Char;

#elif defined(XS_STRING_UTF8)

typedef XS_String String_from_XML_Char;

#else

struct String_from_XML_Char : public XS_String
{
	String_from_XML_Char(const XML_Char* str)
	{
		assign_utf8(*this, str);
	}
};

#endif


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


 /// in memory representation of an XML node
struct XMLNode : public XS_String
{
#if defined(UNICODE) && !defined(XS_STRING_UTF8)
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
	};
#else
	typedef std::map<XS_String, XS_String> AttributeMap;
#endif

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
	 :	_attributes(other._attributes),
		_leading(other._leading),
		_content(other._content),
		_end_leading(other._end_leading),
		_trailing(other._trailing)
	{
		for(Children::const_iterator it=other._children.begin(); it!=other._children.end(); ++it)
			_children.push_back(new XMLNode(**it));
	}

	~XMLNode()
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

	 /// C++ write access to an attribute
	XS_String& operator[](const XS_String& attr_name)
	{
		return _attributes[attr_name];
	}

	 /// read only access to an attribute
	template<typename T> XS_String get(const T& attr_name) const
	{
		AttributeMap::const_iterator found = _attributes.find(attr_name);

		if (found != _attributes.end())
			return found->second;
		else
			return XS_String();
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

	XS_String get_content() const
	{
#ifdef XS_STRING_UTF8
		const XS_String& ret = _content;
#else
		XS_String ret;
		assign_utf8(ret, _content.c_str());
#endif

		return DecodeXMLString(ret.c_str());
	}

	void set_content(const XS_String& s)
	{
		_content.assign(EncodeXMLString(s.c_str()));
	}

	enum WRITE_MODE {
		FORMAT_SMART	= 0,	/// preserve original white space and comments if present; pretty print otherwise
		FORMAT_ORIGINAL = 1,	/// write XML stream preserving original white space and comments
		FORMAT_PRETTY	= 2 	/// pretty print node to stream without preserving original white space
	};

	 /// write node with children tree to output stream
	std::ostream& write(std::ostream& out, WRITE_MODE mode=FORMAT_SMART, int indent=0) const
	{
		switch(mode) {
		  case FORMAT_PRETTY:
			pretty_write_worker(out, indent);
			break;

		  case FORMAT_ORIGINAL:
			write_worker(out, indent);
			break;

		default:	 // FORMAT_SMART
			smart_write_worker(out, indent);
		}

		return out;
	}

protected:
	Children _children;
	AttributeMap _attributes;

	std::string _leading;
	std::string _content;
	std::string _end_leading;
	std::string _trailing;

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

	 /// XPath find functions
	const XMLNode* find_relative(const char* path) const;

	XMLNode* find_relative(const char* path)
		{return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->find_relative(path));}

	 /// relative XPath create function
	XMLNode* create_relative(const char* path);

	void write_worker(std::ostream& out, int indent) const;
	void pretty_write_worker(std::ostream& out, int indent) const;
	void smart_write_worker(std::ostream& out, int indent) const;
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

	 /// C++ access to current node
	operator const XMLNode*() const {return _cur;}
	operator XMLNode*() {return _cur;}

	const XMLNode* operator->() const {return _cur;}
	XMLNode* operator->() {return _cur;}

	const XMLNode& operator*() const {return *_cur;}
	XMLNode& operator*() {return *_cur;}

	 /// attribute access
	XS_String get(const XS_String& attr_name) const
	{
		return _cur->get(attr_name);
	}

	void put(const XS_String& attr_name, const XS_String& value)
	{
		_cur->put(attr_name, value);
	}

	 /// C++ attribute access
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

	 /// C++ access to current node
	operator const XMLNode*() const {return _cur;}

	const XMLNode* operator->() const {return _cur;}

	const XMLNode& operator*() const {return *_cur;}

	 /// attribute access
	XS_String get(const XS_String& attr_name) const
	{
		return _cur->get(attr_name);
	}

	 /// C++ attribute access
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


 // work around GCC's wide string constant bug
#ifdef __GNUC__
extern const LPCXSSTR XS_TRUE;
extern const LPCXSSTR XS_FALSE;
extern const LPCXSSTR XS_NUMBERFMT;
#else
#define	XS_TRUE XS_TEXT("true")
#define	XS_FALSE XS_TEXT("false")
#define	XS_NUMBERFMT XS_TEXT("%d")
#endif


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
		XS_sprintf(buffer, XS_NUMBERFMT, _value);
		return buffer;
	}

protected:
	int _value;

private:
	void operator=(const XMLInt&); // disallow assignment operations
};

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
		XS_sprintf(buffer, XS_NUMBERFMT, value);
		_ref.assign(buffer);
	}

protected:
	XS_String& _ref;
};


struct XMLString
{
	XMLString(const XS_String& value)
	 :	_value(value)
	{
	}

	XMLString(LPCXSSTR value, LPCXSSTR def=XS_TEXT(""))
	{
		if (value && *value)
			_value = value;
		else
			_value = def;
	}

	XMLString(const XMLNode* node, const XS_String& attr_name, LPCXSSTR def=XS_TEXT(""))
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

struct XMStringRef
{
	XMStringRef(XMLNode* node, const XS_String& attr_name, LPCXSSTR def=XS_TEXT(""))
	 :	_ref((*node)[attr_name])
	{
		if (_ref.empty())
			assign(def);
	}

	XMStringRef(XMLNode* node, const XS_String& node_name, const XS_String& attr_name, LPCXSSTR def=XS_TEXT(""))
	 :	_ref(node->subvalue(node_name, attr_name))
	{
		if (_ref.empty())
			assign(def);
	}

	XMStringRef& operator=(const XS_String& value)
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
{
	XMLReaderBase(XMLNode* node)
	 :	_pos(node),
		_parser(XML_ParserCreate(NULL))
	{
		XML_SetUserData(_parser, this);
		XML_SetXmlDeclHandler(_parser, XML_XmlDeclHandler);
		XML_SetElementHandler(_parser, XML_StartElementHandler, XML_EndElementHandler);
		XML_SetDefaultHandler(_parser, XML_DefaultHandler);

		_last_tag = TAG_NONE;
	}

	virtual ~XMLReaderBase()
	{
		XML_ParserFree(_parser);
	}

	XML_Status read();

	virtual int read_buffer(char* buffer, int len) = 0;

	std::string get_position() const
	{
		int line = XML_GetCurrentLineNumber(_parser);
		int column = XML_GetCurrentColumnNumber(_parser);

		std::ostringstream out;
		out << "(" << line << ") : [column " << column << "]";

		return out.str();
	}

	XML_Error	get_error_code() {return XML_GetErrorCode(_parser);}
	std::string get_error_string() const;

protected:
	XMLPos		_pos;
	XML_Parser	_parser;
	std::string _xml_version;
	std::string _encoding;

	std::string _content;
	enum {TAG_NONE, TAG_START, TAG_END} _last_tag;

	static void XMLCALL XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone);
	static void XMLCALL XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
	static void XMLCALL XML_EndElementHandler(void* userData, const XML_Char* name);
	static void XMLCALL XML_DefaultHandler(void* userData, const XML_Char* s, int len);
};


 /// XML file reader
struct XMLReader : public XMLReaderBase
{
	XMLReader(XMLNode* node, std::istream& in)
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


struct XMLHeader
{
	XMLHeader(const std::string& xml_version="1.0", const std::string& encoding="UTF-8", const std::string& doctype="")
	 :	_version(xml_version),
		_encoding(encoding),
		_doctype(doctype)
	{
	}

	void print(std::ostream& out) const
	{
		out << "<?xml version=\"" << _version << "\" encoding=\"" << _encoding << "\"?>\n";

		if (!_doctype.empty())
			out << _doctype << '\n';
	}

	std::string _version;
	std::string _encoding;
	std::string _doctype;
};


struct XMLDoc : public XMLNode
{
	XMLDoc()
	 :	XMLNode(""),
		_last_error(XML_ERROR_NONE)
	{
	}

	XMLDoc(LPCTSTR path)
	 :	XMLNode(""),
		_last_error(XML_ERROR_NONE)
	{
		read(path);
	}

	std::istream& read(std::istream& in)
	{
		XMLReader reader(this, in);

		read(reader);

		return in;
	}

	bool read(LPCTSTR path)
	{
		tifstream in(path);
		XMLReader reader(this, in);

//#if defined(_STRING_DEFINED) && !defined(XS_STRING_UTF8)
//		return read(reader, std::string(ANS(path)));
//#else
		return read(reader, XS_String(path));
//#endif
	}

	bool read(XMLReaderBase& reader)
	{
		XML_Status status = reader.read();

		if (status == XML_STATUS_ERROR) {
			std::ostringstream out;

			out << "input stream" << reader.get_position() << " " << reader.get_error_string();

			_last_error = reader.get_error_code();
			_last_error_msg = out.str();
		}

		return status != XML_STATUS_ERROR;
	}

	bool read(XMLReaderBase& reader, const std::string& display_path)
	{
		XML_Status status = reader.read();

		if (status == XML_STATUS_ERROR) {
			std::ostringstream out;

			out << display_path << reader.get_position() << " " << reader.get_error_string();

			_last_error = reader.get_error_code();
			_last_error_msg = out.str();
		}

		return status != XML_STATUS_ERROR;
	}

	 /// write XML stream preserving previous white space and comments
	std::ostream& write(std::ostream& out, WRITE_MODE mode=FORMAT_SMART, const XMLHeader& header=XMLHeader()) const
	{
		header.print(out);

		if (!_children.empty())
			_children.front()->write(out);

		return out;
	}

	 /// write XML stream with formating
	std::ostream& write_formating(std::ostream& out) const
	{
		return write(out, FORMAT_PRETTY);
	}

	void write(LPCTSTR path, WRITE_MODE mode=FORMAT_SMART, const XMLHeader& header=XMLHeader()) const
	{
		tofstream out(path);

		write(out, mode, header);
	}

	void write_formating(LPCTSTR path) const
	{
		tofstream out(path);

		write_formating(out);
	}

	XML_Error	_last_error;
	std::string _last_error_msg;
};


struct XMLMessage : public XMLDoc
{
	XMLMessage(const char* name)
	 :	_pos(this)
	{
		_pos.create(name);
	}

	XMLPos	_pos;
};


}	// namespace XMLStorage

#define _XMLSTORAGE_H
#endif // _XMLSTORAGE_H
