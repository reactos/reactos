/*
 * Copyright 2004 Martin Fuchs
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
 // XML storage classes
 //
 // xmlstorage.h
 //
 // Martin Fuchs, 22.03.2004
 //


#include "expat.h"

#include <fstream>
#include <sstream>
#include <stack>

#ifdef _MSC_VER
#pragma comment(lib, "libexpat.lib")
#endif


 // write XML files with 2 spaces indenting
#define	XML_INDENT_SPACE "  "


 /// in memory representation of an XML node
struct XMLNode : public string
{
	typedef map<string, string> AttributeMap;
	typedef list<XMLNode*> Children;

	XMLNode(const string& name)
	 :	string(name)
	{
	}

	~XMLNode()
	{
		while(!_children.empty()) {
			delete _children.back();
			_children.pop_back();
		}
	}

	 /// add a new child node
	void add_child(XMLNode* child)
	{
		_children.push_back(child);
	}

	 /// write access to an attribute
	string& operator[](const string& attr_name)
	{
		return _attributes[attr_name];
	}

	 /// read only access to an attribute
	string operator[](const string& attr_name) const
	{
		AttributeMap::const_iterator found = _attributes.find(attr_name);

		if (found != _attributes.end())
			return found->second;
		else
			return "";
	}

	const Children& get_children() const
	{
		return _children;
	}

	XMLNode* get_first_child() const
	{
		if (!_children.empty())
			return _children.front();
		else
			return NULL;
	}

	XMLNode* find_first(const string& name) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			if (**it == name)
				return *it;

		return NULL;
	}

	XMLNode* find_first(const string& name, const string& attr_name, const string& attr_value) const
	{
		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it) {
			const XMLNode& node = **it;

			if (node==name &&
#ifdef UNICODE
				!strcmp(node[attr_name].c_str(), attr_value.c_str())	// workaround because of STL bug
#else
				node[attr_name]==attr_value
#endif
				)
				return *it;
		}

		return NULL;
	}

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

	 /// write XML stream preserving original white space and comments
	ostream& write(ostream& out)
	{
		out << "<" << *this;

		for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
			out << " " << it->first << "=\"" << it->second << "\"";

		if (!_children.empty() || !_content.empty()) {
			out << ">" << _content;

			for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
				(*it)->write(out);

			out << "</" << *this << ">" << _trailing;
		} else {
			out << "/>" << _trailing;
		}

		return out;
	}

	 /// print write node to stream without preserving original white space
	ostream& write_formating(ostream& out, int indent=0)
	{
		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;

		out << "<" << *this;

		for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
			out << " " << it->first << "=\"" << it->second << "\"";

		if (!_children.empty()) {
			out << ">\n";

			for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
				(*it)->write_formating(out, indent+1);

			for(int i=indent; i--; )
				out << XML_INDENT_SPACE;

			out << "</" << *this << ">\n";
		} else {
			out << "/>\n";
		}

		return out;
	}

protected:
	Children _children;
	AttributeMap _attributes;

	string	_content;
	string	_trailing;
};


 /// iterator for XML trees
struct XMLPos
{
	XMLPos(XMLNode* root)
	 :	_root(root),
		_cur(root)
	{
	}

	 /// access to current node
	XMLNode* operator->() {return _cur;}
	const XMLNode* operator->() const {return _cur;}

	XMLNode& operator*() {return *_cur;}
	const XMLNode& operator*() const {return *_cur;}

	 /// attribute access
	string& operator[](const string& attr_name) {return (*_cur)[attr_name];}
	string operator[](const string& attr_name) const {return (*_cur)[attr_name];}

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
	bool go_down(const string& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node) {
			go_to(node);
			return true;
		} else
			return false;
	}

	 /// create node if not already existing and move to it
	void create(const string& name)
	{
		XMLNode* node = _cur->find_first(name);

		if (node)
			go_to(node);
		else
			add_down(new XMLNode(name));
	}

	 /// search matching child node identified by key name and an attribute value
	void create(const string& name, const string& attr_name, const string& attr_value)
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

	bool go(const char* path);

protected:
	XMLNode* _root;
	XMLNode* _cur;
	stack<XMLNode*> _stack;

	 /// go to specified node
	void go_to(XMLNode* child)
	{
		_stack.push(_cur);
		_cur = child;
	}
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

	XML_Status read(istream& in)
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

	string get_position() const
	{
		int line = XML_GetCurrentLineNumber(_parser);
		int column = XML_GetCurrentColumnNumber(_parser);

		ostringstream out;
		out << "(" << line << ") : [column " << column << "]";

		return out.str();
	}

	string get_error_string() const
	{
		XML_Error error = XML_GetErrorCode(_parser);

		ostringstream out;
		out << get_position() << " XML parser error #" << error << "\n";

		return out.str();
	}

protected:
	XMLPos		_pos;
	XML_Parser	_parser;
	string		_xml_version;
	string		_encoding;
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

	XMLDoc(const string& path)
	 :	XMLNode("")
	{
		read(path);
	}

	istream& read(istream& in)
	{
		XMLReader(this).read(in);

		return in;
	}

	bool read(const string& path)
	{
		ifstream in(path.c_str());

		return XMLReader(this).read(in) != XML_STATUS_ERROR;
	}

	 /// write XML stream preserving previous white space and comments
	ostream& write(ostream& out, const string& xml_version="1.0", const string& encoding="UTF-8")
	{
		out << "<?xml version=\"" << xml_version << "\" encoding=\"" << encoding << "\"?>\n";

		if (!_children.empty())
			_children.front()->write(out);

		return out;
	}

	 /// write XML stream with formating
	ostream& write_formating(ostream& out)
	{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

		if (!_children.empty())
			_children.front()->write_formating(out);

		return out;
	}

	void write(const string& path, const string& xml_version="1.0", const string& encoding="UTF-8")
	{
		ofstream out(path.c_str());

		write(out, xml_version);
	}

	void write_formating(const string& path)
	{
		ofstream out(path.c_str());

		write_formating(out);
	}
};
