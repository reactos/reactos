
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


 /// store XML version and encoding into XML reader
void XMLCALL XMLReader::XML_XmlDeclHandler(void* userData, const XML_Char* version, const XML_Char* encoding, int standalone)
{
	XMLReader* pThis = (XMLReader*) userData;

	if (version) {
		pThis->_xml_version = version;
		pThis->_encoding = encoding;
	}
}

 /// notifications about XML tag start
void XMLCALL XMLReader::XML_StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts)
{
	XMLReader* pThis = (XMLReader*) userData;

	XMLNode* node = new XMLNode(String_from_XML_Char(name));

	pThis->_pos.add_down(node);

	while(*atts) {
		const XML_Char* attr_name = *atts++;
		const XML_Char* attr_value = *atts++;

		(*node)[String_from_XML_Char(attr_name)] = String_from_XML_Char(attr_value);
	}

	pThis->_in_tag = true;
}

 /// notifications about XML tag end
void XMLCALL XMLReader::XML_EndElementHandler(void* userData, const XML_Char* name)
{
	XMLReader* pThis = (XMLReader*) userData;

	pThis->_pos.back();

	pThis->_in_tag = false;
}

 /// store content, white space and comments
void XMLCALL XMLReader::XML_DefaultHandler(void* userData, const XML_Char* s, int len)
{
	XMLReader* pThis = (XMLReader*) userData;

	if (pThis->_in_tag)
		pThis->_pos->append_content(s, len);
	else
		pThis->_pos->append_trailing(s, len);
}


}	// namespace XMLStorage
