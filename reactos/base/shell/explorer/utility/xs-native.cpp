
 //
 // XML storage C++ classes version 1.3
 //
 // Copyright (c) 2006, 2007, 2008 Martin Fuchs <martin-fuchs@gmx.net>
 //

 /// \file xs-native.cpp
 /// native internal XMLStorage parser


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
#define XS_NO_COMMENT	// no #pragma comment(lib, ...) statements in .lib files to enable static linking
#endif

//#include "xmlstorage.h"
#include <precomp.h>


#if !defined(XS_USE_EXPAT) && !defined(XS_USE_XERCES)

namespace XMLStorage {


XMLReaderBase::~XMLReaderBase()
{
}

 /// read XML stream into XML tree below _pos
void XMLReaderBase::read()
{
	if (!parse()) {
		XMLError error;

		error._message = "XML parsing error";
		//error._line = ;
		//error._column = ;

		_errors.push_back(error);
	}

	finish_read();
}


 /// line buffer for XS-native parser

struct Buffer
{
	Buffer()
	{
		_buffer = (char*) malloc(BUFFER_LEN);
		_len = BUFFER_LEN;

		reset();
	}

	~Buffer()
	{
		free(_buffer);
	}

	void reset()
	{
		_wptr = _buffer;
		_buffer_str.erase();
	}

	void append(int c)
	{
		size_t wpos = _wptr-_buffer;

		if (wpos >= _len) {
			_len <<= 1;
			_buffer = (char*) realloc(_buffer, _len);
			_wptr = _buffer + wpos;
		}

		*_wptr++ = static_cast<char>(c);
	}

	const std::string& str(bool utf8)	// returns UTF-8 encoded buffer content
	{
#if defined(_WIN32) && !defined(XS_STRING_UTF8)
		if (utf8)
#endif
			_buffer_str.assign(_buffer, _wptr-_buffer);
#if defined(_WIN32) && !defined(XS_STRING_UTF8)
		else
			_buffer_str = get_utf8(_buffer, _wptr-_buffer);
#endif

		return _buffer_str;
	}

	size_t len() const
	{
		return _wptr - _buffer;
	}

	bool has_CDEnd() const
	{
		//if (_wptr-_buffer < 3)
		//	return false;

		return !strncmp(_wptr-3, "]]>", 3);
	}

	XS_String get_tag() const
	{
		const char* p = _buffer_str.c_str();

		if (*p == '<')
			++p;

		if (*p == '/')
			++p;

		const char* q = p;

		if (*q == '?')
			++q;

		q = get_xmlsym_end_utf8(q);

#ifdef XS_STRING_UTF8
		return XS_String(p, q-p);
#else
		XS_String tag;
		assign_utf8(tag, p, q-p);
		return tag;
#endif
	}

	 /// read attributes and values
	void get_attributes(XMLNode::AttributeMap& attributes) const
	{
		const char* p = _buffer_str.c_str();

		 // find end of tag name
		if (*p == '<')
			++p;

		if (*p == '/')
			++p;
		else if (*p == '?')
			++p;

		p = get_xmlsym_end_utf8(p);

		 // read attributes from buffer
		while(*p && *p!='>' && *p!='/') {
			while(isspace((unsigned char)*p))
				++p;

			const char* attr_name = p;

			p = get_xmlsym_end_utf8(p);

			if (*p != '=')
				break;	//@TODO error handling

			size_t attr_len = p - attr_name;

			if (*++p!='"' && *p!='\'')
				break;	//@TODO error handling

			char delim = *p;
			const char* value = ++p;

			while(*p && *p!=delim)
				++p;

			size_t value_len = p - value;

			if (*p)
				++p;	// '"'

#ifdef XS_STRING_UTF8
			XS_String name_str(attr_name, attr_len);
#else
			XS_String name_str;
			assign_utf8(name_str, attr_name, attr_len);
#endif

			attributes[name_str] = DecodeXMLString(std::string(value, value_len));
		}
	}

protected:
	char*	_buffer;
	char*	_wptr;
	size_t	_len;
	std::string	_buffer_str;	// UTF-8 encoded
};

bool XMLReaderBase::parse()
{
	Buffer buffer;
	int c = get();
	bool in_comment = false;

	while(c != EOF) {
		if (in_comment || c=='<') {
			buffer.append(c);

			 // read start or end tag
			for(;;) {
				c = get();

				if (c == EOF)
					break;

				buffer.append(c);

				if (c == '>')
					break;
			}

			const std::string& b = buffer.str(_utf8);
			const char* str = b.c_str();

			if (in_comment || !strncmp(str+1, "!--", 3)) {
				 // XML comment
				DefaultHandler(b);

				if (strcmp(str+b.length()-3, "-->"))
					in_comment = true;
				else
					in_comment = false;

				c = get();
			} else if (str[1] == '/') {
				 // end tag

				/*@TODO error handling
				const XS_String& tag = buffer.get_tag();

					if (tag != last_opened_tag) {
						ERROR
					}
				*/

				EndElementHandler();

				c = get();
			} else if (str[1] == '?') {
				 // XML declaration
				const XS_String& tag = buffer.get_tag();

				if (tag == "?xml") {
					XMLNode::AttributeMap attributes;
					buffer.get_attributes(attributes);

					const std::string& version = attributes.get("version");
					const std::string& encoding = attributes.get("encoding");

					int standalone;
					XMLNode::AttributeMap::const_iterator found =	// const_cast for ISO C++ compatibility error of GCC
							const_cast<const XMLNode::AttributeMap&>(attributes).find("standalone");
					if (found != attributes.end())
						standalone = !XS_icmp(found->second.c_str(), XS_TEXT("yes"));
					else
						standalone = -1;

					XmlDeclHandler(version.empty()?NULL:version.c_str(), encoding.empty()?NULL:encoding.c_str(), standalone);

					if (!encoding.empty() && !_stricmp(encoding.c_str(), "utf-8"))
						_utf8 = true;

					c = eat_endl();
				} else if (tag == "?xml-stylesheet") {
					XMLNode::AttributeMap attributes;
					buffer.get_attributes(attributes);

					StyleSheet stylesheet(attributes.get("href"), attributes.get("type"), !XS_icmp(attributes.get("alternate"), XS_TEXT("yes")));
					stylesheet._title = attributes.get("title");
					stylesheet._media = attributes.get("media");
					stylesheet._charset = attributes.get("charset");

					_format._stylesheets.push_back(stylesheet);

					c = eat_endl();
				} else {
					DefaultHandler(b);
					c = get();
				}
			} else if (str[1] == '!') {
				if (!strncmp(str+2, "DOCTYPE ", 8)) {
					_format._doctype.parse(str+10);

					c = eat_endl();
				} else if (!strncmp(str+2, "[CDATA[", 7)) {
					 // parse <![CDATA[ ... ]]> strings
					while(!buffer.has_CDEnd()) {
						c = get();

						if (c == EOF)
							break;

						buffer.append(c);
					}

					DefaultHandler(buffer.str(_utf8));

					c = get();
				}
			} else {
				 // start tag
				const XS_String& tag = buffer.get_tag();

				if (!tag.empty()) {
				    XMLNode::AttributeMap attributes;
				    buffer.get_attributes(attributes);

				    StartElementHandler(tag, attributes);

				    if (str[b.length()-2] == '/')
					    EndElementHandler();
			    }

				c = get();
			}
		} else {
			buffer.append(c);

			 // read white space
			for(;;) {
				 // check for the encoding of the first line end
				if (!_endl_defined) {
					if (c == '\n') {
						_format._endl = "\n";
						_endl_defined = true;
					} else if (c == '\r') {
						_format._endl = "\r\n";
						_endl_defined = true;
					}
				}

				c = get();

				if (c == EOF)
					break;

				if (c == '<')
					break;

				buffer.append(c);
			}

			DefaultHandler(buffer.str(_utf8));
		}

		buffer.reset();
	}

	return true; //TODO return false on invalid XML
}

int XMLReaderBase::eat_endl()
{
	int c = get();

	if (c == '\r')
		c = get();

	if (c == '\n')
		c = get();

	return c;
}

 /// return current parser position as string
std::string XMLReaderBase::get_position() const
{
/*@TODO display parser position in case of errors
	int line = XML_GetCurrentLineNumber(_parser);
	int column = XML_GetCurrentColumnNumber(_parser);

	std::ostringstream out;
	out << "(" << line << ") : [column " << column << "]";

	return out.str();
*/
	return "";
}


#ifdef XMLNODE_LOCATION

XMLLocation XMLReaderBase::get_location() const
{
	return XMLLocation();	//@TODO XMLLocation for XS-native
}

std::string XMLLocation::str() const
{
	return "";	//TODO
}

#endif


 /// store content, white space and comments
void XMLReaderBase::DefaultHandler(const std::string& s)
{
	_content.append(s);
}


}	// namespace XMLStorage

#endif // !defined(XS_USE_EXPAT) && !defined(XS_USE_XERCES)
