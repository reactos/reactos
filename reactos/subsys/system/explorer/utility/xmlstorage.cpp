
 //
 // XML storage classes
 //
 // xmlstorage.cpp
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

//#include "xmlstorage.h"
#include "precomp.h"


 // work around GCC's wide string constant bug
#ifdef __GNUC__
const LPCXSSTR XMLStorage::XS_TRUE = XS_TEXT("true");
const LPCXSSTR XMLStorage::XS_FALSE = XS_TEXT("false");
const LPCXSSTR XMLStorage::XS_NUMBERFMT = XS_TEXT("%d");
#endif


namespace XMLStorage {


static std::string unescape(const char* s, char b='"', char e='"')
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

static std::string unescape(const char* s, int l, char b='"', char e='"')
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

		int l = slash? slash-path: strlen(path);
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

		int l = slash? slash-path: strlen(path);
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


 /// read XML stream into XML tree below _pos
XML_Status XMLReaderBase::read()
{
	XML_Status status = XML_STATUS_OK;

	while(status == XML_STATUS_OK) {
		char* buffer = (char*) XML_GetBuffer(_parser, BUFFER_LEN);

		int l = read_buffer(buffer, BUFFER_LEN);
		if (l < 0)
			break;

		status = XML_ParseBuffer(_parser, l, false);
	}

	if (status != XML_STATUS_ERROR)
		status = XML_ParseBuffer(_parser, 0, true);

	if (_pos->_children.empty())
		_pos->_trailing.append(_content);
	else
		_pos->_children.back()->_trailing.append(_content);

	_content.erase();

	return status;
}


 /// store XML version and encoding into XML reader
void XMLCALL XMLReaderBase::XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone)
{
	XMLReaderBase* pReader = (XMLReaderBase*) userData;

	if (version)
		pReader->_xml_version = version;

	if (encoding)
		pReader->_encoding = encoding;
}

 /// notifications about XML start tag
void XMLCALL XMLReaderBase::XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts)
{
	XMLReaderBase* pReader = (XMLReaderBase*) userData;
	XMLPos& pos = pReader->_pos;

	 // search for end of first line
	const char* s = pReader->_content.c_str();
	const char* p = s;
	const char* e = p + pReader->_content.length();

	for(; p<e; ++p)
		if (*p == '\n') {
			++p;
			break;
		}

	if (p != s)
		if (pos->_children.empty()) {	// no children in last node?
			if (pReader->_last_tag == TAG_START)
				pos->_content.append(s, p-s);
			else if (pReader->_last_tag == TAG_END)
				pos->_trailing.append(s, p-s);
			// else TAG_NONE -> don't store white space in root node
		} else
			pos->_children.back()->_trailing.append(s, p-s);

	std::string leading;

	if (p != e)
		leading.assign(p, e-p);

	XMLNode* node = new XMLNode(String_from_XML_Char(name), leading);

	pos.add_down(node);

	while(*atts) {
		const XML_Char* attr_name = *atts++;
		const XML_Char* attr_value = *atts++;

		(*node)[String_from_XML_Char(attr_name)] = String_from_XML_Char(attr_value);
	}

	pReader->_last_tag = TAG_START;
	pReader->_content.erase();
}

 /// notifications about XML end tag
void XMLCALL XMLReaderBase::XML_EndElementHandler(void* userData, const XML_Char* name)
{
	XMLReaderBase* pReader = (XMLReaderBase*) userData;
	XMLPos& pos = pReader->_pos;

	 // search for end of first line
	const char* s = pReader->_content.c_str();
	const char* p = s;
	const char* e = p + pReader->_content.length();

	for(; p<e; ++p)
		if (*p == '\n') {
			++p;
			break;
		}

	if (p != s)
		if (pos->_children.empty())	// no children in current node?
			pos->_content.append(s, p-s);
		else
			if (pReader->_last_tag == TAG_START)
				pos->_content.append(s, p-s);
			else
				pos->_children.back()->_trailing.append(s, p-s);

	if (p != e)
		pos->_end_leading.assign(p, e-p);

	pos.back();

	pReader->_last_tag = TAG_END;
	pReader->_content.erase();
}

 /// store content, white space and comments
void XMLCALL XMLReaderBase::XML_DefaultHandler(void* userData, const XML_Char* s, int len)
{
	XMLReaderBase* pReader = (XMLReaderBase*) userData;

	pReader->_content.append(s, len);
}


std::string XMLReaderBase::get_error_string() const
{
	XML_Error error = XML_GetErrorCode(_parser);

	switch(error) {
	  case XML_ERROR_NONE:								return "XML_ERROR_NONE";
	  case XML_ERROR_NO_MEMORY:							return "XML_ERROR_NO_MEMORY";
	  case XML_ERROR_SYNTAX:							return "XML_ERROR_SYNTAX";
	  case XML_ERROR_NO_ELEMENTS:						return "XML_ERROR_NO_ELEMENTS";
	  case XML_ERROR_INVALID_TOKEN:						return "XML_ERROR_INVALID_TOKEN";
	  case XML_ERROR_UNCLOSED_TOKEN:					return "XML_ERROR_UNCLOSED_TOKEN";
	  case XML_ERROR_PARTIAL_CHAR:						return "XML_ERROR_PARTIAL_CHAR";
	  case XML_ERROR_TAG_MISMATCH:						return "XML_ERROR_TAG_MISMATCH";
	  case XML_ERROR_DUPLICATE_ATTRIBUTE:				return "XML_ERROR_DUPLICATE_ATTRIBUTE";
	  case XML_ERROR_JUNK_AFTER_DOC_ELEMENT:			return "XML_ERROR_JUNK_AFTER_DOC_ELEMENT";
	  case XML_ERROR_PARAM_ENTITY_REF:					return "XML_ERROR_PARAM_ENTITY_REF";
	  case XML_ERROR_UNDEFINED_ENTITY:					return "XML_ERROR_UNDEFINED_ENTITY";
	  case XML_ERROR_RECURSIVE_ENTITY_REF:				return "XML_ERROR_RECURSIVE_ENTITY_REF";
	  case XML_ERROR_ASYNC_ENTITY:						return "XML_ERROR_ASYNC_ENTITY";
	  case XML_ERROR_BAD_CHAR_REF:						return "XML_ERROR_BAD_CHAR_REF";
	  case XML_ERROR_BINARY_ENTITY_REF:					return "XML_ERROR_BINARY_ENTITY_REF";
	  case XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF:		return "XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF";
	  case XML_ERROR_MISPLACED_XML_PI:					return "XML_ERROR_MISPLACED_XML_PI";
	  case XML_ERROR_UNKNOWN_ENCODING:					return "XML_ERROR_UNKNOWN_ENCODING";
	  case XML_ERROR_INCORRECT_ENCODING:				return "XML_ERROR_INCORRECT_ENCODING";
	  case XML_ERROR_UNCLOSED_CDATA_SECTION:			return "XML_ERROR_UNCLOSED_CDATA_SECTION";
	  case XML_ERROR_EXTERNAL_ENTITY_HANDLING:			return "XML_ERROR_EXTERNAL_ENTITY_HANDLING";
	  case XML_ERROR_NOT_STANDALONE:					return "XML_ERROR_NOT_STANDALONE";
	  case XML_ERROR_UNEXPECTED_STATE:					return "XML_ERROR_UNEXPECTED_STATE";
	  case XML_ERROR_ENTITY_DECLARED_IN_PE:				return "XML_ERROR_ENTITY_DECLARED_IN_PE";
	  case XML_ERROR_FEATURE_REQUIRES_XML_DTD:			return "XML_ERROR_FEATURE_REQUIRES_XML_DTD";
	  case XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING:	return "XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING";
	  case XML_ERROR_UNBOUND_PREFIX:					return "XML_ERROR_UNBOUND_PREFIX";
 // EXPAT version >= 1.95.8
#if XML_MAJOR_VERSION>1 || (XML_MAJOR_VERSION==1 && XML_MINOR_VERSION>95) || (XML_MAJOR_VERSION==1 && XML_MINOR_VERSION==95 && XML_MICRO_VERSION>7)
	  case XML_ERROR_SUSPENDED:							return "XML_ERROR_SUSPENDED";
	  case XML_ERROR_NOT_SUSPENDED:						return "XML_ERROR_NOT_SUSPENDED";
	  case XML_ERROR_ABORTED:							return "XML_ERROR_ABORTED";
	  case XML_ERROR_FINISHED:							return "XML_ERROR_FINISHED";
	  case XML_ERROR_SUSPEND_PE:						return "XML_ERROR_SUSPEND_PE";
#endif
	}

	std::ostringstream out;

	out << "XML parser error #" << error;

	return out.str();
}


std::string EncodeXMLString(const XS_String& str)
{
	LPCXSSTR s = str.c_str();
	LPXSSTR buffer = (LPXSSTR)alloca(5*sizeof(XS_CHAR)*XS_len(s));	// worst case. "&amp;"
	LPXSSTR o = buffer;

	for(LPCXSSTR p=s; *p; ++p)
		switch(*p) {
		  case '&':
			*o++ = '&';	*o++ = 'a';	*o++ = 'm';	*o++ = 'p';	*o++ = ';';
			break;

		  case '<':
			*o++ = '&';	*o++ = 'l'; *o++ = 't';	*o++ = ';';
			break;

		  case '>':
			*o++ = '&';	*o++ = 'g'; *o++ = 't';	*o++ = ';';
			break;

		  case '"':
			*o++ = '&';	*o++ = 'q'; *o++ = 'u'; *o++ = 'o'; *o++ = 't';	*o++ = ';';
			break;

		  case '\'':
			*o++ = '&';	*o++ = 'a'; *o++ = 'p'; *o++ = 'o'; *o++ = 's';	*o++ = ';';
			break;

		  default:
			*o++ = *p;
		}

#ifdef XS_STRING_UTF8
	return XS_String(buffer, o-buffer);
#else
	return get_utf8(buffer, o-buffer);
#endif
}

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


 /// pretty print node with children tree to output stream
void XMLNode::pretty_write_worker(std::ostream& out, int indent) const
{
	for(int i=indent; i--; )
		out << XML_INDENT_SPACE;

	out << '<' << EncodeXMLString(*this);

	for(AttributeMap::const_iterator it=_attributes.begin(); it!=_attributes.end(); ++it)
		out << ' ' << EncodeXMLString(it->first) << "=\"" << EncodeXMLString(it->second) << "\"";

	if (!_children.empty() || !_content.empty()) {
		out << ">\n";

		for(Children::const_iterator it=_children.begin(); it!=_children.end(); ++it)
			(*it)->pretty_write_worker(out, indent+1);

		for(int i=indent; i--; )
			out << XML_INDENT_SPACE;

		out << "</" << EncodeXMLString(*this) << ">\n";
	} else
		out << "/>\n";
}


 /// write node with children tree to output stream using smart formating
void XMLNode::smart_write_worker(std::ostream& out, int indent) const
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
			out << '\n';
		else
			out << _content;

		Children::const_iterator it = _children.begin();

		if (it != _children.end()) {
			for(; it!=_children.end(); ++it)
				(*it)->smart_write_worker(out, indent+1);

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
		out << '\n';
	else
		out << _trailing;
}


}	// namespace XMLStorage
