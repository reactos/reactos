
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


#ifndef _STRING_DEFINED

 /// string class for TCHAR strings

struct String
#ifdef UNICODE
 : public std::wstring
#else
 : public std::string
#endif
{
#ifdef UNICODE
	typedef std::wstring super;
#else
	typedef std::string super;
#endif

	String() {}
	String(LPCTSTR s) {if (s) super::assign(s);}
	String(LPCTSTR s, int l) : super(s, l) {}
	String(const super& other) : super(other) {}
	String(const String& other) : super(other) {}

#ifdef UNICODE
	String(LPCSTR s) {assign(s);}
	String(LPCSTR s, int l) {assign(s, l);}
	String(const std::string& other) {assign(other.c_str());}
	String& operator=(LPCSTR s) {assign(s); return *this;}
	void assign(LPCSTR s) {if (s) {TCHAR b[BUFFER_LEN]; super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, -1, b, BUFFER_LEN)-1);} else erase();}
	void assign(LPCSTR s, int l) {if (s) {TCHAR b[BUFFER_LEN]; super::assign(b, MultiByteToWideChar(CP_ACP, 0, s, l, b, BUFFER_LEN));} else erase();}
#else
	String(LPCWSTR s) {assign(s);}
	String(LPCWSTR s, int l) {assign(s, l);}
	String(const std::wstring& other) {assign(other.c_str());}
	String& operator=(LPCWSTR s) {assign(s); return *this;}
	void assign(LPCWSTR s) {if (s) {char b[BUFFER_LEN]; super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, -1, b, BUFFER_LEN, 0, 0)-1);} else erase();}
	void assign(LPCWSTR s, int l) {if (s) {char b[BUFFER_LEN]; super::assign(b, WideCharToMultiByte(CP_ACP, 0, s, l, b, BUFFER_LEN, 0, 0));} else erase();}
#endif

	String& operator=(LPCTSTR s) {if (s) super::assign(s); else erase(); return *this;}
	String& operator=(const super& s) {super::assign(s); return *this;}
	void assign(LPCTSTR s) {super::assign(s);}
	void assign(LPCTSTR s, int l) {super::assign(s, l);}

	operator LPCTSTR() const {return c_str();}

#ifdef UNICODE
	operator std::string() const {char b[BUFFER_LEN]; return std::string(b, WideCharToMultiByte(CP_ACP, 0, c_str(), -1, b, BUFFER_LEN, 0, 0)-1);}
#else
	operator std::wstring() const {WCHAR b[BUFFER_LEN]; return std::wstring(b, MultiByteToWideChar(CP_ACP, 0, c_str(), -1, b, BUFFER_LEN)-1);}
#endif

	String& printf(LPCTSTR fmt, ...)
	{
		va_list l;
		TCHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::assign(b, _vstprintf(b, fmt, l));
		va_end(l);

		return *this;
	}

	String& vprintf(LPCTSTR fmt, va_list l)
	{
		TCHAR b[BUFFER_LEN];

		super::assign(b, _vstprintf(b, fmt, l));

		return *this;
	}

	String& appendf(LPCTSTR fmt, ...)
	{
		va_list l;
		TCHAR b[BUFFER_LEN];

		va_start(l, fmt);
		super::append(b, _vstprintf(b, fmt, l));
		va_end(l);

		return *this;
	}

	String& vappendf(LPCTSTR fmt, va_list l)
	{
		TCHAR b[BUFFER_LEN];

		super::append(b, _vstprintf(b, fmt, l));

		return *this;
	}
};

#endif


inline void assign_utf8(String& s, const char* str)
{
	TCHAR buffer[BUFFER_LEN];

#ifdef UNICODE
	int l = MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, BUFFER_LEN) - 1;
#else
	WCHAR wbuffer[BUFFER_LEN];

	int l = MultiByteToWideChar(CP_UTF8, 0, str, -1, wbuffer, BUFFER_LEN) - 1;
	l = WideCharToMultiByte(CP_ACP, 0, wbuffer, l, buffer, BUFFER_LEN, 0, 0);
#endif

	s.assign(buffer, l);
}

inline std::string get_utf8(LPCTSTR s, int l)
{
	char buffer[BUFFER_LEN];

#ifdef UNICODE
	l = WideCharToMultiByte(CP_UTF8, 0, s, l, buffer, BUFFER_LEN, 0, 0);
#else
	WCHAR wbuffer[BUFFER_LEN];

	l = MultiByteToWideChar(CP_ACP, 0, s, l, wbuffer, BUFFER_LEN);
	l = WideCharToMultiByte(CP_UTF8, 0, wbuffer, l, buffer, BUFFER_LEN, 0, 0);
#endif

	return std::string(buffer, l);
}

inline std::string get_utf8(const String& s)
{
	return get_utf8(s.c_str(), s.length());
}

extern std::string EncodeXMLString(LPCTSTR s);
extern String DecodeXMLString(LPCTSTR s);


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

typedef String String_from_XML_Char;

#else

struct String_from_XML_Char : public String
{
	String_from_XML_Char(const XML_Char* str)
	{
		assign_utf8(*this, str);
	}
};

#endif


#ifdef UNICODE

 // optimization for faster UNICODE/ASCII string comparison without temporary A/U conversion
inline bool operator==(const String& s1, const char* s2)
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
struct XMLNode : public String
{
#ifdef UNICODE
	 // optimized read access without temporary A/U conversion when using ASCII attribute names
	struct AttributeMap : public std::map<String, String>
	{
		typedef std::map<String, String> super;

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
	typedef std::map<String, String> AttributeMap;
#endif
	typedef std::list<XMLNode*> Children;

	 // access to protected class members for XMLPos and XMLReader
	friend struct XMLPos;
	friend struct const_XMLPos;
	friend struct XMLReaderBase;

	XMLNode(const String& name)
	 :	String(name)
	{
	}

	XMLNode(const String& name, const std::string& leading)
	 :	String(name),
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

		while(!_children.empty()) {
			XMLNode* node = _children.back();
			_children.pop_back();

			node->clear();
			delete node;
		}

		String::erase();
	}

	XMLNode& operator=(const XMLNode& other)
	{
		_children.clear();

		for(Children::const_iterator it=other._children.begin(); it!=other._children.end(); ++it)
			_children.push_back(new XMLNode(**it));

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
	String& operator[](const String& attr_name)
	{
		return _attributes[attr_name];
	}

	 /// read only access to an attribute
	template<typename T> String get(const T& attr_name) const
	{
		AttributeMap::const_iterator found = _attributes.find(attr_name);

		if (found != _attributes.end())
			return found->second;
		else
			return TEXT("");
	}

	 /// convenient value access in children node
	String value(const String& name, const String& attr_name) const
	{
		const XMLNode* node = find_first(name);

		if (node)
			return node->get(attr_name);
		else
			return TEXT("");
	}

	 /// convenient storage of distinct values in children node
	String& value(const String& name, const String& attr_name)
	{
		XMLNode* node = find_first(name);

		if (!node) {
			node = new XMLNode(name);
			add_child(node);
		}

		return (*node)[attr_name];
	}

#ifdef UNICODE
	 /// convenient value access in children node
	String value(const char* name, const char* attr_name) const
	{
		const XMLNode* node = find_first(name);

		if (node)
			return node->get(attr_name);
		else
			return TEXT("");
	}

	 /// convenient storage of distinct values in children node
	String& value(const char* name, const String& attr_name)
	{
		XMLNode* node = find_first(name);

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

	String get_content() const
	{
		String ret;

		assign_utf8(ret, _content.c_str());

		return DecodeXMLString(ret);
	}

	void set_content(const String& s)
	{
		_content.assign(EncodeXMLString(s));
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

	XMLNode* find_first(const String& name) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			if (**it == name)
				return *it;

		return NULL;
	}

	XMLNode* find_first(const String& name, const String& attr_name, const String& attr_value) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
			const XMLNode& node = **it;

			if (node==name && node.get(attr_name)==attr_value)
				return *it;
		}

		return NULL;
	}

#ifdef UNICODE
	XMLNode* find_first(const char* name) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			if (**it == name)
				return *it;

		return NULL;
	}

	template<typename T, typename U>
	XMLNode* find_first(const char* name, const T& attr_name, const U& attr_value) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
			const XMLNode& node = **it;

			if (node==name && node.get(attr_name)==attr_value)
				return *it;
		}

		return NULL;
	}
#endif

	void write_worker(std::ostream& out, int indent) const;
	void pretty_write_worker(std::ostream& out, int indent) const;
	void smart_write_worker(std::ostream& out, int indent) const;
};


 /// iterator access to children nodes with name filtering
struct XMLChildrenFilter
{
	XMLChildrenFilter(XMLNode::Children& children, const String& name)
	 :	_begin(children.begin(), children.end(), name),
		_end(children.end(), children.end(), name)
	{
	}

	XMLChildrenFilter(XMLNode* node, const String& name)
	 :	_begin(node->get_children().begin(), node->get_children().end(), name),
		_end(node->get_children().end(), node->get_children().end(), name)
	{
	}

	struct iterator
	{
		typedef XMLNode::Children::iterator BaseIterator;

		iterator(BaseIterator begin, BaseIterator end, const String& filter_name)
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
		String	_filter_name;

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
	const_XMLChildrenFilter(const XMLNode::Children& children, const String& name)
	 :	_begin(children.begin(), children.end(), name),
		_end(children.end(), children.end(), name)
	{
	}

	const_XMLChildrenFilter(const XMLNode* node, const String& name)
	 :	_begin(node->get_children().begin(), node->get_children().end(), name),
		_end(node->get_children().end(), node->get_children().end(), name)
	{
	}

	struct const_iterator
	{
		typedef XMLNode::Children::const_iterator BaseIterator;

		const_iterator(BaseIterator begin, BaseIterator end, const String& filter_name)
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
		String	_filter_name;

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

	 /// access to current node
	operator const XMLNode*() const {return _cur;}
	operator XMLNode*() {return _cur;}

	const XMLNode* operator->() const {return _cur;}
	XMLNode* operator->() {return _cur;}

	const XMLNode& operator*() const {return *_cur;}
	XMLNode& operator*() {return *_cur;}

	 /// attribute access
	template<typename T> String get(const T& attr_name) const {return (*_cur)[attr_name];}
	String& operator[](const String& attr_name) {return (*_cur)[attr_name];}

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
	bool go_down(const String& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// move X-Path like to position in XML tree
	bool go(const char* path);

	 /// create node and move to it
	void create(const String& name)
	{
		add_down(new XMLNode(name));
	}

	 /// create node if not already existing and move to it
	void smart_create(const String& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	void smart_create(const String& name, const String& attr_name, const String& attr_value)
	{
		XMLNode* node = _cur->find_first(name, attr_name, attr_value);

		if (node)
			go_to(node);
		else {
			XMLNode* node = new XMLNode(name);
			add_down(node);
			(*node)[attr_name] = attr_value;
		}
	}

#ifdef UNICODE
	 /// search for child and go down
	bool go_down(const char* name)
	{
		XMLNode* node = _cur->find_first(name);

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
		XMLNode* node = _cur->find_first(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	template<typename T, typename U>
	void smart_create(const char* name, const T& attr_name, const U& attr_value)
	{
		XMLNode* node = _cur->find_first(name, attr_name, attr_value);

		if (node)
			go_to(node);
		else {
			XMLNode* node = new XMLNode(name);
			add_down(node);
			(*node)[attr_name] = attr_value;
		}
	}
#endif

	String& str() {return *_cur;}
	const String& str() const {return *_cur;}

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
	operator const XMLNode*() const {return _cur;}

	const XMLNode* operator->() const {return _cur;}

	const XMLNode& operator*() const {return *_cur;}

	 /// attribute access
	template<typename T> String get(const T& attr_name) const {return _cur->get(attr_name);}

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
	bool go_down(const String& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// move X-Path like to position in XML tree
	bool go(const char* path);

#ifdef UNICODE
	 /// search for child and go down
	bool go_down(const char* name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}
#endif

	const String& str() const {return *_cur;}

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


struct XMLBool
{
	XMLBool(bool value)
	 :	_value(value)
	{
	}

	XMLBool(LPCTSTR value, bool def=false)
	{
		if (value && *value)
			_value = !_tcsicmp(value, TEXT("TRUE"));
		else
			_value = def;
	}

	XMLBool(const XMLNode* node, const String& attr_name, bool def=false)
	{
		const String& value = node->get(attr_name);

		if (!value.empty())
			_value = !_tcsicmp(value, TEXT("TRUE"));
		else
			_value = def;
	}

	XMLBool(const XMLNode* node, const String& name, const String& attr_name, bool def=false)
	{
		const String& value = node->value(name, attr_name);

		if (!value.empty())
			_value = !_tcsicmp(value, TEXT("TRUE"));
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

	operator LPCTSTR() const
	{
		return _value? TEXT("TRUE"): TEXT("FALSE");
	}

protected:
	bool	_value;

private:
	void operator=(const XMLBool&); // disallow assignment operations
};

struct XMLBoolRef
{
	XMLBoolRef(XMLNode* node, const String& name, const String& attr_name, bool def=false)
	 :	_ref(node->value(name, attr_name))
	{
		if (_ref.empty())
			assign(def);
	}

	operator bool() const
	{
		return !_tcsicmp(_ref, TEXT("TRUE"));
	}

	bool operator!() const
	{
		return _tcsicmp(_ref, TEXT("TRUE"))? true: false;
	}

	XMLBoolRef& operator=(bool value)
	{
		assign(value);

		return *this;
	}

	void assign(bool value)
	{
		_ref.assign(value? TEXT("TRUE"): TEXT("FALSE"));
	}

	void toggle()
	{
		assign(!operator bool());
	}

protected:
	String& _ref;
};


struct XMLInt
{
	XMLInt(int value)
	 :	_value(value)
	{
	}

	XMLInt(LPCTSTR value, int def=0)
	{
		if (value && *value)
			_value = _ttoi(value);
		else
			_value = def;
	}

	XMLInt(const XMLNode* node, const String& attr_name, int def=0)
	{
		const String& value = node->get(attr_name);

		if (!value.empty())
			_value = _ttoi(value);
		else
			_value = def;
	}

	XMLInt(const XMLNode* node, const String& name, const String& attr_name, int def=0)
	{
		const String& value = node->value(name, attr_name);

		if (!value.empty())
			_value = _ttoi(value);
		else
			_value = def;
	}

	operator int() const
	{
		return _value;
	}

	operator String() const
	{
		TCHAR buffer[32];
		_stprintf(buffer, TEXT("%d"), _value);
		return buffer;
	}

protected:
	int _value;

private:
	void operator=(const XMLInt&); // disallow assignment operations
};

struct XMLIntRef
{
	XMLIntRef(XMLNode* node, const String& name, const String& attr_name, int def=0)
	 :	_ref(node->value(name, attr_name))
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
		return _ttoi(_ref);
	}

	void assign(int value)
	{
		TCHAR buffer[32];

		_stprintf(buffer, TEXT("%d"), value);
		_ref.assign(buffer);
	}

protected:
	String& _ref;
};


struct XMLString
{
	XMLString(const String& value)
	 :	_value(value)
	{
	}

	XMLString(LPCTSTR value, LPCTSTR def=TEXT(""))
	{
		if (value && *value)
			_value = value;
		else
			_value = def;
	}

	XMLString(const XMLNode* node, const String& attr_name, LPCTSTR def=TEXT(""))
	{
		const String& value = node->get(attr_name);

		if (!value.empty())
			_value = value;
		else
			_value = def;
	}

	XMLString(const XMLNode* node, const String& name, const String& attr_name, LPCTSTR def=TEXT(""))
	{
		const String& value = node->value(name, attr_name);

		if (!value.empty())
			_value = value;
		else
			_value = def;
	}

	operator const String&() const
	{
		return _value;
	}

protected:
	String	_value;

private:
	void operator=(const XMLString&); // disallow assignment operations
};

struct XMStringRef
{
	XMStringRef(XMLNode* node, const String& name, const String& attr_name, LPCTSTR def=TEXT(""))
	 :	_ref(node->value(name, attr_name))
	{
		if (_ref.empty())
			assign(def);
	}

	XMStringRef& operator=(const String& value)
	{
		assign(value);

		return *this;
	}

	operator const String&() const
	{
		return _ref;
	}

	void assign(const String& value)
	{
		_ref.assign(value);
	}

protected:
	String& _ref;
};


#ifdef _MSC_VER
#pragma warning(disable: 4355)
#endif

 /// XML file reader
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

		_in.read(buffer, BUFFER_LEN);

		return _in.gcount();
	}

protected:
	std::istream&	_in;
};


struct XMLHeader : public std::string
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

	std::string	_version;
	std::string	_encoding;
	std::string	_doctype;
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

		return read(reader, String(path));
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
	std::string	_last_error_msg;
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
