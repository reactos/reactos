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
 // xmlstorage.cpp
 //
 // Martin Fuchs, 22.03.2004
 //


#include "utility.h"

#include "xmlstorage.h"


void xml_test()	//@@
{
	XMLDoc doc;

	doc.read("explorer-cfg.xml");
	doc.write("out.xml");

	XMLPos pos(&doc);

	if (pos.go("\\explorer-cfg\\startmenu")) {

		pos.back();
	}
}


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
