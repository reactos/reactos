/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef __MSXMLDID_H__
#define __MSXMLDID_H__

#define DISPID_XOBJ_MIN                     0x10000
#define DISPID_XOBJ_MAX                     0x1FFFF
#define DISPID_XOBJ_BASE                    0x10000

#define DISPID_XMLELEMENTCOLLECTION         0x10000
#define DISPID_XMLELEMENTCOLLECTION_LENGTH  0x10001
#define DISPID_XMLELEMENTCOLLECTION_NEWENUM DISPID_NEWENUM
#define DISPID_XMLELEMENTCOLLECTION_ITEM    0x10003

#define DISPID_XMLDOCUMENT                  0x10064
#define DISPID_XMLDOCUMENT_ROOT             0x10065
#define DISPID_XMLDOCUMENT_FILESIZE         0x10066
#define DISPID_XMLDOCUMENT_FILEMODIFIEDDATE 0x10067
#define DISPID_XMLDOCUMENT_FILEUPDATEDDATE  0x10068
#define DISPID_XMLDOCUMENT_URL              0x10069
#define DISPID_XMLDOCUMENT_MIMETYPE         0x1006a
#define DISPID_XMLDOCUMENT_READYSTATE       0x1006b
#define DISPID_XMLDOCUMENT_CREATEELEMENT    0x1006c
#define DISPID_XMLDOCUMENT_CHARSET          0x1006d
#define DISPID_XMLDOCUMENT_VERSION          0x1006e
#define DISPID_XMLDOCUMENT_DOCTYPE          0x1006f
#define DISPID_XMLDOCUMENT_DTDURL           0x10070
#define DISPID_XMLDOCUMENT_ASYNC            0x10071
#define DISPID_XMLDOCUMENT_CASEINSENSITIVE  0x10072

#define DISPID_XMLELEMENT                   0x100c8
#define DISPID_XMLELEMENT_TAGNAME           0x100c9
#define DISPID_XMLELEMENT_PARENT            0x100ca
#define DISPID_XMLELEMENT_SETATTRIBUTE      0x100cb
#define DISPID_XMLELEMENT_GETATTRIBUTE      0x100cc
#define DISPID_XMLELEMENT_REMOVEATTRIBUTE   0x100cd
#define DISPID_XMLELEMENT_CHILDREN          0x100ce
#define DISPID_XMLELEMENT_TYPE              0x100cf
#define DISPID_XMLELEMENT_TEXT              0x100d0
#define DISPID_XMLELEMENT_ADDCHILD          0x100d1
#define DISPID_XMLELEMENT_REMOVECHILD       0x100d2
#define DISPID_XMLELEMENT_ATTRIBUTES        0x100d3

#define DISPID_XMLNOTIFSINK                 0x1012c
#define DISPID_XMLNOTIFSINK_CHILDADDED      0x1012d

#define DISPID_XMLATTRIBUTE                 0x10190
#define DISPID_XMLATTRIBUTE_NAME            0x10191
#define DISPID_XMLATTRIBUTE_VALUE           0x10192

#endif /* __MSXMLDID_H__ */
