//*********************************************************************
//*                  Microsoft Windows                               **
//*         Copyright (c) 1996 - 1999 Microsoft Corporation. All rights reserved.**///*********************************************************************

//;begin_internal
/***********************************************************************************************

  This is a distributed SDK component - do not put any #includes or other directives that rely
  upon files not dropped. If in doubt - build iedev

  If you add comments please include either ;BUGBUG at the beginning of a single line OR
  enclose in a ;begin_internal, ;end_internal block - such as this one!

 ***********************************************************************************************/
//;end_internal

//;begin_internal
#ifndef __MSXMLDID_H__
#define __MSXMLDID_H__
//;end_internal

#define DISPID_XOBJ_MIN                 0x00010000
#define DISPID_XOBJ_MAX                 0x0001FFFF
#define DISPID_XOBJ_BASE                DISPID_XOBJ_MIN


#define DISPID_XMLELEMENTCOLLECTION             DISPID_XOBJ_BASE

#define DISPID_XMLELEMENTCOLLECTION_LENGTH     DISPID_XMLELEMENTCOLLECTION + 1
#define DISPID_XMLELEMENTCOLLECTION_NEWENUM     DISPID_NEWENUM
#define DISPID_XMLELEMENTCOLLECTION_ITEM        DISPID_XMLELEMENTCOLLECTION + 3


#define DISPID_XMLDOCUMENT                      DISPID_XMLELEMENTCOLLECTION + 100

#define DISPID_XMLDOCUMENT_ROOT                 DISPID_XMLDOCUMENT + 1
#define DISPID_XMLDOCUMENT_FILESIZE             DISPID_XMLDOCUMENT + 2
#define DISPID_XMLDOCUMENT_FILEMODIFIEDDATE     DISPID_XMLDOCUMENT + 3
#define DISPID_XMLDOCUMENT_FILEUPDATEDDATE      DISPID_XMLDOCUMENT + 4
#define DISPID_XMLDOCUMENT_URL                  DISPID_XMLDOCUMENT + 5
#define DISPID_XMLDOCUMENT_MIMETYPE             DISPID_XMLDOCUMENT + 6
#define DISPID_XMLDOCUMENT_READYSTATE           DISPID_XMLDOCUMENT + 7
#define DISPID_XMLDOCUMENT_CREATEELEMENT        DISPID_XMLDOCUMENT + 8
#define DISPID_XMLDOCUMENT_CHARSET              DISPID_XMLDOCUMENT + 9
#define DISPID_XMLDOCUMENT_VERSION              DISPID_XMLDOCUMENT + 10
#define DISPID_XMLDOCUMENT_DOCTYPE              DISPID_XMLDOCUMENT + 11
#define DISPID_XMLDOCUMENT_DTDURL               DISPID_XMLDOCUMENT + 12
#define DISPID_XMLDOCUMENT_ASYNC                DISPID_XMLDOCUMENT + 13
#define DISPID_XMLDOCUMENT_CASEINSENSITIVE      DISPID_XMLDOCUMENT + 14
#define DISPID_XMLDOCUMENT_BASEURL              DISPID_XMLDOCUMENT + 15
#define DISPID_XMLDOCUMENT_XML                  DISPID_XMLDOCUMENT + 16
#define DISPID_XMLDOCUMENT_LASTERROR            DISPID_XMLDOCUMENT + 17
#define DISPID_XMLDOCUMENT_TRIMWHITESPACE       DISPID_XMLDOCUMENT + 18
#define DISPID_XMLDOCUMENT_COMMIT				DISPID_XMLDOCUMENT + 19

#define DISPID_XMLELEMENT                       DISPID_XMLDOCUMENT + 100

#define DISPID_XMLELEMENT_TAGNAME               DISPID_XMLELEMENT + 1
#define DISPID_XMLELEMENT_PARENT                DISPID_XMLELEMENT + 2
#define DISPID_XMLELEMENT_SETATTRIBUTE          DISPID_XMLELEMENT + 3
#define DISPID_XMLELEMENT_GETATTRIBUTE          DISPID_XMLELEMENT + 4
#define DISPID_XMLELEMENT_REMOVEATTRIBUTE       DISPID_XMLELEMENT + 5
#define DISPID_XMLELEMENT_CHILDREN              DISPID_XMLELEMENT + 6
#define DISPID_XMLELEMENT_TYPE                  DISPID_XMLELEMENT + 7
#define DISPID_XMLELEMENT_TEXT                  DISPID_XMLELEMENT + 8
#define DISPID_XMLELEMENT_ADDCHILD              DISPID_XMLELEMENT + 9
#define DISPID_XMLELEMENT_REMOVECHILD           DISPID_XMLELEMENT + 10
#define DISPID_XMLELEMENT_ATTRIBUTES            DISPID_XMLELEMENT + 11

#define DISPID_XMLNOTIFSINK                     DISPID_XMLELEMENT + 100 

#define DISPID_XMLNOTIFSINK_CHILDADDED          DISPID_XMLNOTIFSINK + 1

#define DISPID_XMLATTRIBUTE                     DISPID_XMLNOTIFSINK + 100

#define DISPID_XMLATTRIBUTE_NAME                DISPID_XMLATTRIBUTE + 1
#define DISPID_XMLATTRIBUTE_VALUE               DISPID_XMLATTRIBUTE + 2


// IXMLError2
#define DISPID_XMLERROR                         DISPID_XMLNOTIFSINK + 100
#define DISPID_XMLERROR_REASON                  DISPID_XMLERROR + 1
#define DISPID_XMLERROR_LINE                    DISPID_XMLERROR + 2
#define DISPID_XMLERROR_POS                     DISPID_XMLERROR + 3

// INode
#define DISPID_NODE                             DISPID_XMLERROR + 100
#define DISPID_NODE_NAME                        DISPID_NODE + 1
#define DISPID_NODE_PARENT                      DISPID_NODE + 2
#define DISPID_NODE_TYPE                        DISPID_NODE + 3

#define DISPID_NODE_VALUE                       DISPID_NODE + 4

#define DISPID_NODE_SETATTRIBUTE                DISPID_NODE + 5
#define DISPID_NODE_GETATTRIBUTE                DISPID_NODE + 6
#define DISPID_NODE_REMOVEATTRIBUTE             DISPID_NODE + 7
#define DISPID_NODE_ATTRIBUTES                  DISPID_NODE + 8

#define DISPID_NODE_ADD                         DISPID_NODE + 9
#define DISPID_NODE_REMOVE                      DISPID_NODE + 10
#define DISPID_NODE_CHILDREN                    DISPID_NODE + 11

// INodeList
#define DISPID_NODELIST                         DISPID_NODE + 100
#define DISPID_NODELIST_NEWENUM                 DISPID_NODELIST + 1
#define DISPID_NODELIST_NEXT                    DISPID_NODELIST + 2
#define DISPID_NODELIST_CURRENT                 DISPID_NODELIST + 3
#define DISPID_NODELIST_MOVE                    DISPID_NODELIST + 4
#define DISPID_NODELIST_MOVETONODE              DISPID_NODELIST + 5

#define DISPID_NODELIST_LENGTH                  DISPID_NODELIST + 6
#define DISPID_NODELIST_ITEM                    DISPID_NODELIST + 7

//;begin_internal
#endif // __MSXMLDID_H__
//;end_internal
