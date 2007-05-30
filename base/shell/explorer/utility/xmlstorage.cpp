
 //
 // XML storage classes version 1.2
 //
 // Copyright (c) 2004, 2005, 2006, 2007 Martin Fuchs <martin-fuchs@gmx.net>
 //

 /// \file xmlstorage.cpp
 /// XMLStorage implementation file


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

#ifndef XS_NO_COMMENT
#define XS_NO_COMMENT	// no #pragma comment(lib, ...) statements in .lib files
#endif

//#include "xmlstorage.h"
#include <precomp.h>


namespace XMLStorage {


 // work around GCC's wide string constant bug
#ifdef __GNUC__
const LPCXSSTR XS_EMPTY = XS_EMPTY_STR;
const LPCXSSTR XS_TRUE = XS_TRUE_STR;
const LPCXSSTR XS_FALSE = XS_FALSE_STR;
const LPCXSSTR XS_INTFMT = XS_INTFMT_STR;
const LPCXSSTR XS_FLOATFMT = XS_FLOATFMT_STR;
#endif


 /// remove escape characters from zero terminated string
static std::string unescape(const char* s, char b, char e)
{
	const char* end = s + strlen(s);

//	if (*s == b)
//		++s;
//
//	if (end>s && end[-1]==e)
//		--end;

	if (*s == b)
		if (end>s && end[-1]==e)
			++s, --end;

	return std::string(s, end-s);
}

inline std::string unescape(const char* s)
{
	return unescape(s, '"', '"');
}

 /// remove escape characters from string with specified length
static std::string unescape(const char* s, size_t l, char b, char e)
{
	const char* end = s + l;

//	if (*s == b)
//		++s;
//
//	if (end>s && end[-1]==e)
//		--end;

	if (*s == b)
		if (end>s && end[-1]==e)
			++s, --end;

	return std::string(s, end-s);
}

inline std::string unescape(const char* s, size_t l)
{
	return unescape(s, l, '"', '"');
}


 /// move XPath like to position in XML tree
bool XMLPos::go(const char* path)
{
	XMLNode* node = _cur;

	 // Is this an absolute path?
	if (*path == '/') {
		node = _root;
		++path;
	}

	node = node->find_relative(path);

	if (node) {
		go_to(node);
		return true;
	} else
		return false;
}

 /// move XPath like to position in XML tree
bool const_XMLPos::go(const char* path)
{
	const XMLNode* node = _cur;

	 // Is this an absolute path?
	if (*path == '/') {
		node = _root;
		++path;
	}

	node = node->find_relative(path);

	if (node) {
		go_to(node);
		return true;
	} else
		return false;
}


const XMLNode* XMLNode::find_relative(const char* path) const
{
	const XMLNode* node = this;

	 // parse relative path
	while(*path) {
		const char* slash = strchr(path, '/');
		if (slash == path)
			return NULL;

		size_t l = slash? slash-path: strlen(path);
		std::string comp(path, l);
		path += l;

		 // look for [n] and [@attr_name="attr_value"] expressions in path components
		const char* bracket = strchr(comp.c_str(), '[');
		l = bracket? bracket-comp.c_str(): comp.length();
		std::string child_name(comp.c_str(), l);
		std::string attr_name, attr_value;

		int n = 0;
		if (bracket) {
			std::string expr = unescape(bracket, '[', ']');
			const char* p = expr.c_str();

			n = atoi(p);	// read index number

			if (n)
				n = n - 1;	// convert into zero based index

			const char* at = strchr(p, '@');

			if (at) {
				p = at + 1;
				const char* equal = strchr(p, '=');

				 // read attribute name and value
				if (equal) {
					attr_name = unescape(p, equal-p);
					attr_value = unescape(equal+1);
				}
			}
		}

		if (attr_name.empty())
			 // search n.th child node with specified name
			node = node->find(child_name, n);
		else
			 // search n.th child node with specified name and matching attribute value
			node = node->find(child_name, attr_name, attr_value, n);

		if (!node)
			return NULL;

		if (*path == '/')
			++path;
	}

	return node;
}

XMLNode* XMLNode::create_relative(const char* path)
{
	XMLNode* node = this;

	 // parse relative path
	while(*path) {
		const char* slash = strchr(path, '/');
		if (slash == path)
			return NULL;

		size_t l = slash? slash-path: strlen(path);
		std::string comp(path, l);
		path += l;

		 // look for [n] and [@attr_name="attr_value"] expressions in path components
		const char* bracket = strchr(comp.c_str(), '[');
		l = bracket? bracket-comp.c_str(): comp.length();
		std::string child_name(comp.c_str(), l);
		std::string attr_name, attr_value;

		int n = 0;
		if (bracket) {
			std::string expr = unescape(bracket, '[', ']');
			const char* p = expr.c_str();

			n = atoi(p);	// read index number

			if (n)
				n = n - 1;	// convert into zero based index

			const char* at = strchr(p, '@');

			if (at) {
				p = at + 1;
				const char* equal = strchr(p, '=');

				 // read attribute name and value
				if (equal) {
					attr_name = unescape(p, equal-p);
					attr_value = unescape(equal+1);
				}
			}
		}

		XMLNode* child;

		if (attr_name.empty())
			 // search n.th child node with specified name
			child = node->find(child_name, n);
		else
			 // search n.th child node with specified name and matching attribute value
			child = node->find(child_name, attr_name, attr_value, n);

		if (!child) {
			child = new XMLNode(child_name);
			node->add_child(child);

			if (!attr_name.empty())
				(*node)[attr_name] = attr_value;
		}

		node = child;

		if (*path == '/')
			++path;
	}

	return node;
}


 /// encode XML string literals
std::string EncodeXMLString(const XS_String& str, bool cdata)
{
	LPCXSSTR s = str.c_str();
	size_t l = XS_len(s);

	if (cdata) {
		 // encode the whole string in a CDATA section
		std::string ret = "<![CDATA[";

#ifdef XS_STRING_UTF8
		ret += str;
#else
		ret += get_utf8(str);
#endif

		ret += "]]>";

		return ret;
	} else if (l <= BUFFER_LEN) {
		LPXSSTR buffer = (LPXSSTR)alloca(6*sizeof(XS_CHAR)*XS_len(s));	// worst case "&quot;" / "&apos;"
		LPXSSTR o = buffer;

		for(LPCXSSTR p=s; *p; ++p)
			switch(*p) {
			  case '&':
				*o++ = '&';	*o++ = 'a';	*o++ = 'm';	*o++ = 'p';	*o++ = ';';				// "&amp;"
				break;

			  case '<':
				*o++ = '&';	*o++ = 'l'; *o++ = 't';	*o++ = ';';							// "&lt;"
				break;

			  case '>':
				*o++ = '&';	*o++ = 'g'; *o++ = 't';	*o++ = ';';							// "&gt;"
				break;

			  case '"':
				*o++ = '&';	*o++ = 'q'; *o++ = 'u'; *o++ = 'o'; *o++ = 't';	*o++ = ';';	// "&quot;"
				break;

			  case '\'':
				*o++ = '&';	*o++ = 'a'; *o++ = 'p'; *o++ = 'o'; *o++ = 's';	*o++ = ';';	// "&apos;"
				break;

			  default:
				if ((unsigned)*p<20 && *p!='\t' && *p!='\r' && *p!='\n') {
					char b[16];
					sprintf(b, "&%d;", (unsigned)*p);
					for(const char*q=b; *q; )
						*o++ = *q++;
				} else
					*o++ = *p;
			}

#ifdef XS_STRING_UTF8
		return XS_String(buffer, o-buffer);
#else
		return get_utf8(buffer, o-buffer);
#endif
	} else { // l > BUFFER_LEN
		 // alternative code for larger strings using ostringstream
		 // and avoiding to use alloca() for preallocated memory
		fast_ostringstream out;

		LPCXSSTR s = str.c_str();

		for(LPCXSSTR p=s; *p; ++p)
			switch(*p) {
			  case '&':
				out << "&amp;";
				break;

			  case '<':
				out << "&lt;";
				break;

			  case '>':
				out << "&gt;";
				break;

			  case '"':
				out << "&quot;";
				break;

			  case '\'':
				out << "&apos;";
				break;

			  default:
				if ((unsigned)*p<20 && *p!='\t' && *p!='\r' && *p!='\n')
					out << "&" << (unsigned)*p << ";";
				else
					out << *p;
			}

#ifdef XS_STRING_UTF8
		return XS_String(out.str());
#else
		return get_utf8(out.str());
#endif
	}
}

 /// decode XML string literals
XS_String DecodeXMLString(const XS_String& str)
{
	LPCXSSTR s = str.c_str();
	LPXSSTR buffer = (LPXSSTR)alloca(sizeof(XS_CHAR)*XS_len(s));
	LPXSSTR o = buffer;

	for(LPCXSSTR p=s; *p; ++p)
		if (*p == '&') {
			if (!XS_nicmp(p+1, XS_TEXT("lt;"), 3)) {
				*o++ = '<';
				p += 3;
			} else if (!XS_nicmp(p+1, XS_TEXT("gt;"), 3)) {
				*o++ = '>';
				p += 3;
			} else if (!XS_nicmp(p+1, XS_TEXT("amp;"), 4)) {
				*o++ = '&';
				p += 4;
			} else if (!XS_nicmp(p+1, XS_TEXT("quot;"), 5)) {
				*o++ = '"';
				p += 5;
			} else if (!XS_nicmp(p+1, XS_TEXT("apos;"), 5)) {
				*o++ = '\'';
				p += 5;
			} else
				*o++ = *p;
		} else if (*p=='<' && !XS_nicmp(p+1,XS_TEXT("![CDATA["),8)) {
			LPCXSSTR e = XS_strstr(p+9, XS_TEXT("]]>"));
			if (e) {
				p += 9;
				size_t l = e - p;
				memcpy(o, p, l);
				o += l;
				p = e + 2;
			} else
				*o++ = *p;
		} else
			*o++ = *p;

	return XS_String(buffer, o-buffer);
}


 /// write node with children tree to output stream using original white space
void XMLNode::write_worker(std::ostream& out, int indent) const
{
	out << _leading << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (!_children.empty() || !_content.empty()) {
		out << '>' << _content;

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->write_worker(out, indent+1);

		out << _end_leading << "</" << EncodeXMLString(*this) << '>';
	} else
		out << "/>";

	out << _trailing;
}


 /// print node without any white space
void XMLNode::plain_write_worker(std::ostream& out) const
{
	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (!_children.empty()/*@@ || !_content.empty()*/) {
		out << ">";

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->plain_write_worker(out);

		out << "</" << EncodeXMLString(*this) << ">";
	} else
		out << "/>";
}


 /// pretty print node with children tree to output stream
void XMLNode::pretty_write_worker(std::ostream& out, const XMLFormat& format, int indent) const
{
	for(int i=indent; i--; )
		out << XML_INDENT_SPACE;

	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (!_children.empty()/*@@ || !_content.empty()*/) {
		out << '>' << format._endl;

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->pretty_write_worker(out, format, indent+1);

		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;

		out << "</" << EncodeXMLString(*this) << '>' << format._endl;
	} else
		out << "/>" << format._endl;
}


 /// write node with children tree to output stream using smart formating
void XMLNode::smart_write_worker(std::ostream& out, const XMLFormat& format, int indent) const
{
	if (_leading.empty())
		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;
	else
		out << _leading;

	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (_children.empty() && _content.empty())
		out << "/>";
	else {
		out << '>';

		if (_content.empty())
			out << format._endl;
		else
			out << _content;

		Children::const_iterator it = _children.begin();

		if (it != _children.end()) {
			for(; it!=_children.end(); ++it)
				(*it)->smart_write_worker(out, format, indent+1);

			if (_end_leading.empty())
				for(int i=indent; i--; )
					out << XML_INDENT_SPACE;
			else
				out << _end_leading;
		} else
			out << _end_leading;

		out << "</" << EncodeXMLString(*this) << '>';
	}

	if (_trailing.empty())
		out << format._endl;
	else
		out << _trailing;
}


std::ostream& operator<<(std::ostream& out, const XMLError& err)
{
	out << err._systemId << "(" << err._line << ") [column " << err._column << "] : "
		<< err._message;

	return out;
}


void DocType::parse(const char* p)
{
	while(isspace((unsigned char)*p)) ++p;

	const char* start = p;
	while(isxmlsym(*p)) ++p;
	_name.assign(start, p-start);

	while(isspace((unsigned char)*p)) ++p;

	start = p;
	while(isxmlsym(*p)) ++p;
	std::string keyword(p, p-start);	// "PUBLIC" or "SYSTEM"

	while(isspace((unsigned char)*p)) ++p;

	if (*p=='"' || *p=='\'') {
		char delim = *p;

		start = ++p;
		while(*p && *p!=delim) ++p;

		if (*p == delim)
			_public.assign(start, p++-start);
	} else
		_public.erase();

	while(isspace((unsigned char)*p)) ++p;

	if (*p=='"' || *p=='\'') {
		char delim = *p;

		start = ++p;
		while(*p && *p!=delim) ++p;

		if (*p == delim)
			_system.assign(start, p++-start);
	} else
		_system.erase();
}


void XMLFormat::print_header(std::ostream& out, bool lf) const
{
	out << "<?xml version=\"" << _version << "\" encoding=\"" << _encoding << "\"";

	if (_standalone != -1)
		out << " standalone=\"yes\"";

	out << "?>";

	if (lf)
		out << _endl;

	if (!_doctype.empty()) {
		out << "<!DOCTYPE " << _doctype._name;

		if (!_doctype._public.empty()) {
			out << " PUBLIC \"" << _doctype._public << '"';

			if (lf)
				out << _endl;

			out << " \"" << _doctype._system << '"';
		} else if (!_doctype._system.empty())
			out << " SYSTEM \"" << _doctype._system << '"';

		out << "?>";

		if (lf)
			out << _endl;
	}

	for(StyleSheetList::const_iterator it=_stylesheets.begin(); it!=_stylesheets.end(); ++it) {
		it->print(out);

		if (lf)
			out << _endl;
	}

/*	if (!_additional.empty()) {
		out << _additional;

		if (lf)
			out << _endl;
	} */
}

void StyleSheet::print(std::ostream& out) const
{
	out << "<?xml-stylesheet"
			" href=\"" << _href << "\""
			" type=\"" << _type << "\"";

	if (!_title.empty())
		out << " title=\"" << _title << "\"";

	if (!_media.empty())
		out << " media=\"" << _media << "\"";

	if (!_charset.empty())
		out << " charset=\"" << _charset << "\"";

	if (_alternate)
		out << " alternate=\"yes\"";

	out << "?>";
}


 /// return formated error message
std::string XMLError::str() const
{
	std::ostringstream out;

	out << *this;

	return out.str();
}


 /// return merged error strings
XS_String XMLErrorList::str() const
{
	std::ostringstream out;

	for(const_iterator it=begin(); it!=end(); ++it)
		out << *it << std::endl;

	return out.str();
}


void XMLReaderBase::finish_read()
{
	if (_pos->_children.empty())
		_pos->_trailing.append(_content);
	else
		_pos->_children.back()->_trailing.append(_content);

	_content.erase();
}


 /// store XML version and encoding into XML reader
void XMLReaderBase::XmlDeclHandler(const char* version, const char* encoding, int standalone)
{
	if (version)
		_format._version = version;

	if (encoding)
		_format._encoding = encoding;

	_format._standalone = standalone;
}


 /// notifications about XML start tag
void XMLReaderBase::StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attributes)
{
	 // search for end of first line
	const char* s = _content.c_str();
	const char* p = s;
	const char* e = p + _content.length();

	for(; p<e; ++p)
		if (*p == '\n') {
			++p;
			break;
		}

	if (p != s)
		if (_pos->_children.empty()) {	// no children in last node?
			if (_last_tag == TAG_START)
				_pos->_content.append(s, p-s);
			else if (_last_tag == TAG_END)
				_pos->_trailing.append(s, p-s);
			else // TAG_NONE at root node
				p = s;
		} else
			_pos->_children.back()->_trailing.append(s, p-s);

	std::string leading;

	if (p != e)
		leading.assign(p, e-p);

	XMLNode* node = new XMLNode(name, leading);

	_pos.add_down(node);

#ifdef XMLNODE_LOCATION
	node->_location = get_location();
#endif

	node->_attributes = attributes;

	_last_tag = TAG_START;
	_content.erase();
}

 /// notifications about XML end tag
void XMLReaderBase::EndElementHandler()
{
	 // search for end of first line
	const char* s = _content.c_str();
	const char* p = s;
	const char* e = p + _content.length();

	if (!strncmp(s,"<![CDATA[",9) && !strncmp(e-3,"]]>",3)) {
		s += 9;
		p = (e-=3);
	} else
		for(; p<e; ++p)
			if (*p == '\n') {
				++p;
				break;
			}

	if (p != s)
		if (_pos->_children.empty())	// no children in current node?
			_pos->_content.append(s, p-s);
		else
			if (_last_tag == TAG_START)
				_pos->_content.append(s, p-s);
			else
				_pos->_children.back()->_trailing.append(s, p-s);

	if (p != e)
		_pos->_end_leading.assign(p, e-p);

	_pos.back();

	_last_tag = TAG_END;
	_content.erase();
}

#if defined(XS_USE_XERCES) || defined(XS_USE_EXPAT)
 /// store content, white space and comments
void XMLReaderBase::DefaultHandler(const XML_Char* s, int len)
{
#if defined(XML_UNICODE) || defined(XS_USE_XERCES)
	_content.append(String_from_XML_Char(s, len));
#else
	_content.append(s, len);
#endif
}
#endif


XS_String XMLWriter::s_empty_attr;


}	// namespace XMLStorage
