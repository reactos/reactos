/*
 *    MSXML Class Factory
 *
 * Copyright 2005 Mike McCormack
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __MSXML_PRIVATE__
#define __MSXML_PRIVATE__

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_LIBXML2

#ifdef HAVE_LIBXML_PARSER_H
#include <libxml/parser.h>
#endif

/* constructors */
extern IUnknown         *create_domdoc( void );
extern IUnknown         *create_xmldoc( void );
extern IXMLDOMNode      *create_node( xmlNodePtr node );
extern IUnknown         *create_basic_node( xmlNodePtr node, IUnknown *pUnkOuter );
extern IUnknown         *create_element( xmlNodePtr element, IUnknown *pUnkOuter );
extern IUnknown         *create_attribute( xmlNodePtr attribute );
extern IUnknown         *create_text( xmlNodePtr text );
extern IUnknown         *create_pi( xmlNodePtr pi );
extern IUnknown         *create_comment( xmlNodePtr comment );
extern IXMLDOMNodeList  *create_children_nodelist( xmlNodePtr );
extern IXMLDOMNamedNodeMap *create_nodemap( IXMLDOMNode *node );

extern HRESULT queryresult_create( xmlNodePtr, LPWSTR, IXMLDOMNodeList ** );

extern void attach_xmlnode( IXMLDOMNode *node, xmlNodePtr xmlnode );

/* data accessors */
xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type );

/* helpers */
extern xmlChar *xmlChar_from_wchar( LPWSTR str );
extern BSTR bstr_from_xmlChar( const xmlChar *buf );

extern LONG xmldoc_add_ref( xmlDocPtr doc );
extern LONG xmldoc_release( xmlDocPtr doc );

extern HRESULT XMLElement_create( IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj );
extern HRESULT XMLElementCollection_create( IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj );

extern xmlDocPtr parse_xml(char *ptr, int len);

#endif

extern IXMLDOMParseError *create_parseError( LONG code, BSTR url, BSTR reason, BSTR srcText,
                                             LONG line, LONG linepos, LONG filepos );
extern HRESULT DOMDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT SchemaCache_create( IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT XMLDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );

#endif /* __MSXML_PRIVATE__ */
