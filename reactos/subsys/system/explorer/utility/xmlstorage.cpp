
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

#include "utility.h"

#include "xmlstorage.h"


namespace XMLStorage {


 /// move X-Path like to position in XML tree
bool XMLPos::go(const char* path)
{

	///@todo

	return false;
}


 /// read XML stream into XML tree below _pos
XML_Status XMLReader::read(std::istream& in)
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
		cerr << get_error_string();
*/

	_pos->append_trailing(_content.c_str(), _content.length());
	_content.erase();

	return status;
}


 /// store XML version and encoding into XML reader
void XMLCALL XMLReader::XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone)
{
	XMLReader* pThis = (XMLReader*) userData;

	if (version) {
		pThis->_xml_version = version;
		pThis->_encoding = encoding;
	}
}

 /// notifications about XML start tag
void XMLCALL XMLReader::XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts)
{
	XMLReader* pThis = (XMLReader*) userData;

	 // search for end of first line
	const char* s = pThis->_content.c_str();
	const char* p = s;
	const char* e = p + pThis->_content.length();

	for(; p<e; ++p)
		if (*p == '\n') {
			++p;
			break;
		}

	if (p != s)
		pThis->_pos->append_trailing(s, p-s);

	std::string leading;

	if (p != e)
		leading.assign(p, e-p);

	XMLNode* node = new XMLNode(String_from_XML_Char(name), leading);

	pThis->_pos.add_down(node);

	while(*atts) {
		const XML_Char* attr_name = *atts++;
		const XML_Char* attr_value = *atts++;

		(*node)[String_from_XML_Char(attr_name)] = String_from_XML_Char(attr_value);
	}

	pThis->_in_node = true;
	pThis->_content.erase();
}

 /// notifications about XML end tag
void XMLCALL XMLReader::XML_EndElementHandler(void* userData, const XML_Char* name)
{
	XMLReader* pThis = (XMLReader*) userData;

	 // search for end of first line
	const char* s = pThis->_content.c_str();
	const char* p = s;
	const char* e = p + pThis->_content.length();

	for(; p<e; ++p)
		if (*p == '\n') {
			++p;
			break;
		}

	if (p != s)
		pThis->_pos->append_content(s, p-s);

	std::string leading;

	if (p != e)
		leading.assign(p, e-p);

	if (leading.empty())
		pThis->_pos->_end_leading = leading;

	pThis->_pos.back();

	pThis->_in_node = false;
	pThis->_content.erase();
}

 /// store content, white space and comments
void XMLCALL XMLReader::XML_DefaultHandler(void* userData, const XML_Char* s, int len)
{
	XMLReader* pThis = (XMLReader*) userData;

	pThis->_content.append(s, len);
}


std::string EncodeXMLString(LPCTSTR s)
{
	TCHAR buffer[BUFFER_LEN];
	LPTSTR o = buffer;

	for(LPCTSTR p=s; *p; ++p)
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

		  default:
			*o++ = *p;
		}

	return get_utf8(buffer, o-buffer);
}

String DecodeXMLString(LPCTSTR s)
{
	TCHAR buffer[BUFFER_LEN];
	LPTSTR o = buffer;

	for(LPCTSTR p=s; *p; ++p)
		if (*p == '&') {
			if (!_tcsnicmp(p+1, TEXT("amp;"), 4)) {
				*o++ = '&';
				p += 4;
			} else if (!_tcsnicmp(p+1, TEXT("lt;"), 3)) {
				*o++ = '<';
				p += 3;
			} else if (!_tcsnicmp(p+1, TEXT("gt;"), 3)) {
				*o++ = '>';
				p += 3;
			} else
				*o++ = *p;
		} else
			*o++ = *p;

	return String(buffer, o-buffer);
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

	if (!_children.empty() || !_content.empty()) {
		out << '>';

		if (_content.empty())
			out << '\n';
		else
			out << _content;

		Children::const_iterator it = _children.begin();

		if (it != _children.end())
			for(; it!=_children.end(); ++it)
				(*it)->smart_write_worker(out, indent+1);

		if (_end_leading.empty())
			for(int i=indent; i--; )
				out << XML_INDENT_SPACE;
		else
			out << _end_leading;

		out << "</" << EncodeXMLString(*this) << '>';
	} else
		out << "/>";

	if (_trailing.empty())
		out << '\n';
	else
		out << _trailing;
}


}	// namespace XMLStorage
