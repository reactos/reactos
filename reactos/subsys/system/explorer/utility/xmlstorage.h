
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
#define	_UNICODE
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

extern std::string XMLString(LPCTSTR s);


 // write XML files with 2 spaces indenting
#define	XML_INDENT_SPACE "  "


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
	const char* q = s2;

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
	friend struct XMLReader;

	XMLNode(const String& name)
	 :	String(name)
	{
	}

	XMLNode(const XMLNode& other)
	 :	_attributes(other._attributes),
		_content(other._content),
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

	XMLNode& operator=(const XMLNode& other)
	{
		_children.clear();

		for(Children::const_iterator it=other._children.begin(); it!=other._children.end(); ++it)
			_children.push_back(new XMLNode(**it));

		_attributes = other._attributes;

		_content = other._content;
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
	String operator[](const String& attr_name) const
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
			return (*node)[attr_name];
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
	 /// read only access to an attribute
	String operator[](const char* attr_name) const
	{
		AttributeMap::const_iterator found = _attributes.find(attr_name);

		if (found != _attributes.end())
			return found->second;
		else
			return TEXT("");
	}

	 /// convenient value access in children node
	String value(const char* name, const char* attr_name) const
	{
		const XMLNode* node = find_first(name);

		if (node)
			return (*node)[attr_name];
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

	enum WRITE_MODE {
		FORMAT_SMART	= 0,	/// preserve original white space and comments if present; pretty print otherwise
		FORMAT_ORIGINAL = 1,	/// write XML stream preserving original white space and comments
		FORMAT_PRETTY	= 2		/// pretty print node to stream without preserving original white space
	};

	 /// write node with children tree to output stream
	std::ostream& write(std::ostream& out, WRITE_MODE mode=FORMAT_SMART, int indent=0) const
	{
		switch(mode) {
		  case FORMAT_PRETTY:
			pretty_write_worker(out, mode, indent);
			break;

		  case FORMAT_ORIGINAL:
			write_worker(out, mode, indent);
			break;

		default:	 // FORMAT_SMART
			smart_write_worker(out, indent, _content.empty() && _trailing.empty());
		}

		return out;
	}

protected:
	Children _children;
	AttributeMap _attributes;

	std::string	_content;
	std::string	_trailing;

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

			if (node==name && node[attr_name]==attr_value)
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

			if (node==name && node[attr_name]==attr_value)
				return *it;
		}

		return NULL;
	}
#endif

	void append_content(const char* s, int l)
	{
		if (_children.empty())
			_content.append(s, l);
		else
			_children.back()->_content.append(s, l);
	}

	void append_trailing(const char* s, int l)
	{
		if (_children.empty())
			_trailing.append(s, l);
		else
			_children.back()->_trailing.append(s, l);
	}

	void write_worker(std::ostream& out, WRITE_MODE mode, int indent) const;
	void pretty_write_worker(std::ostream& out, WRITE_MODE mode, int indent) const;
	bool smart_write_worker(std::ostream& out, int indent, bool next_format) const;
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
	operator XMLNode*() {return _cur;}
	operator const XMLNode*() const {return _cur;}

	const XMLNode* operator->() const {return _cur;}
	XMLNode* operator->() {return _cur;}

	const XMLNode& operator*() const {return *_cur;}
	XMLNode& operator*() {return *_cur;}

	 /// attribute access
	String& operator[](const String& attr_name) {return (*_cur)[attr_name];}
	String operator[](const String& attr_name) const {return (*_cur)[attr_name];}

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

	 /// create node if not already existing and move to it
	void create(const String& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	void create(const String& name, const String& attr_name, const String& attr_value)
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

	 /// create node if not already existing and move to it
	void create(const char* name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	template<typename T, typename U>
	void create(const char* name, const T& attr_name, const U& attr_value)
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

	XMLBool(XMLNode* node, const String& name, const String& attr_name, bool def=false)
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

	operator LPCTSTR() const
	{
		return _value? TEXT("TRUE"): TEXT("FALSE");
	}

protected:
	bool	_value;

private:
	void operator=(const XMLBool&);	// disallow assignment operations
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
	String&	_ref;
};


struct XMLNumber
{
	XMLNumber(int value)
	 :	_value(value)
	{
	}

	XMLNumber(LPCTSTR value, int def=0)
	{
		if (value && *value)
			_value = _ttoi(value);
		else
			_value = def;
	}

	XMLNumber(XMLNode* node, const String& name, const String& attr_name, int def=0)
	{
		const String& value = node->value(name, attr_name);

		if (!value.empty())
			_value = _ttoi(node->value(name, attr_name));
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
	int	_value;

private:
	void operator=(const XMLBool&);	// disallow assignment operations
};

struct XMLNumberRef
{
	XMLNumberRef(XMLNode* node, const String& name, const String& attr_name, int def=0)
	 :	_ref(node->value(name, attr_name))
	{
		if (_ref.empty())
			assign(def);
	}

	XMLNumberRef& operator=(int value)
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
	String&	_ref;
};


#ifdef _MSC_VER
#pragma warning(disable: 4355)
#endif

 /// XML file reader
struct XMLReader
{
	XMLReader(XMLNode* node)
	 :	_pos(node),
		_parser(XML_ParserCreate(NULL))
	{
		XML_SetUserData(_parser, this);
		XML_SetXmlDeclHandler(_parser, XML_XmlDeclHandler);
		XML_SetElementHandler(_parser, XML_StartElementHandler, XML_EndElementHandler);
		XML_SetDefaultHandler(_parser, XML_DefaultHandler);

		_in_tag = false;
	}

	~XMLReader()
	{
		XML_ParserFree(_parser);
	}

	XML_Status read(std::istream& in)
	{
		XML_Status status = XML_STATUS_OK;

		while(in.good() && status==XML_STATUS_OK) {
			char* buffer = (char*) XML_GetBuffer(_parser, BUFFER_LEN);

			in.read(buffer, BUFFER_LEN);

			status = XML_ParseBuffer(_parser, in.gcount(), false);
		}

		if (status != XML_STATUS_ERROR)
			status = XML_ParseBuffer(_parser, 0, true);

	/*
		if (status == XML_STATUS_ERROR)
			cerr << path << get_error_string();
	*/

		return status;
	}

	std::string get_position() const
	{
		int line = XML_GetCurrentLineNumber(_parser);
		int column = XML_GetCurrentColumnNumber(_parser);

		std::ostringstream out;
		out << "(" << line << ") : [column " << column << "]";

		return out.str();
	}

	std::string get_error_string() const
	{
		XML_Error error = XML_GetErrorCode(_parser);

		std::ostringstream out;
		out << get_position() << " XML parser error #" << error << "\n";

		return out.str();
	}

protected:
	XMLPos		_pos;
	XML_Parser	_parser;
	std::string	_xml_version;
	std::string	_encoding;
	bool		_in_tag;

	static void XMLCALL XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone);
	static void XMLCALL XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
	static void XMLCALL XML_EndElementHandler(void* userData, const XML_Char* name);
	static void XMLCALL XML_DefaultHandler(void* userData, const XML_Char* s, int len);
};


struct XMLDoc : public XMLNode
{
	XMLDoc()
	 :	XMLNode("")
	{
	}

	XMLDoc(const std::string& path)
	 :	XMLNode("")
	{
		read(path);
	}

	std::istream& read(std::istream& in)
	{
		XMLReader(this).read(in);

		return in;
	}

	bool read(const std::string& path)
	{
		std::ifstream in(path.c_str());

		return XMLReader(this).read(in) != XML_STATUS_ERROR;
	}

	 /// write XML stream preserving previous white space and comments
	std::ostream& write(std::ostream& out, WRITE_MODE mode=FORMAT_SMART,
						const std::string& xml_version="1.0", const std::string& encoding="UTF-8") const
	{
		out << "<?xml version=\"" << xml_version << "\" encoding=\"" << encoding << "\"?>\n";

		if (!_children.empty())
			_children.front()->write(out);

		return out;
	}

	 /// write XML stream with formating
	std::ostream& write_formating(std::ostream& out) const
	{
		return write(out, FORMAT_PRETTY);
	}

	void write(const std::string& path, WRITE_MODE mode=FORMAT_SMART,
				const std::string& xml_version="1.0", const std::string& encoding="UTF-8") const
	{
		std::ofstream out(path.c_str());

		write(out, mode, xml_version, encoding);
	}

	void write_formating(const std::string& path) const
	{
		std::ofstream out(path.c_str());

		write_formating(out);
	}
};


}	// namespace XMLStorage
