;/*
;* 
;* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
;* 
;*/
;//                                                                           
;//                                                                           
;// MSXML specific errors                           
;//                                                                           
;//                                                                           

FacilityNames=(Internet=0xc:FACILITY_INTERNET)

OutputBase =16

;//                                                                           
;//                                                                           
;// New MSXML specific errors                           
;//                                                                           
;//                                                                           

;// -------- READ ME !!!! -------------
;//
;// The broken unix mc compiler (a perl script) needs the symbolic name
;// and Severity on the same lone as the Message ID, to do otherwise will break
;// it. Language can be on a seperate line
;//
;// -----------------------------------
;
;
;// Hack to get rid of build warning
;#ifdef FACILITY_INTERNET
;#undef FACILITY_INTERNET
;#endif
;
MessageId = 0xE000 Facility=Internet Severity = Error SymbolicName = XML_ERROR_MASK
Language=English
.

MessageId = 0xE001 Facility=Internet Severity = Error SymbolicName = XML_IOERROR
Language=English
Error opening input file: '%1'.
.

MessageId = 0xE002  Facility=Internet    Severity = Error SymbolicName = XML_ENTITY_UNDEFINED
Language=English
Reference to undefined entity '%1'.
.

MessageId = 0xE003 Facility=Internet Severity = Error SymbolicName = XML_INFINITE_ENTITY_LOOP
Language=English
Entity '%1' contains an infinite entity reference loop.
.

MessageId = 0xE004 Facility=Internet Severity = Error SymbolicName = XML_NDATA_INVALID_PE
Language=English
Cannot use the NDATA keyword in a parameter entity declaration.
.

MessageId = 0xE005 Facility=Internet Severity = Error SymbolicName = XML_REQUIRED_NDATA
Language=English
Cannot use a general parsed entity '%1' as the value for attribute '%2'.
.

MessageId = 0xE006 Facility=Internet Severity = Error SymbolicName = XML_NDATA_INVALID_REF
Language=English
Cannot use unparsed entity '%1' in an entity reference.
.

MessageId = 0xE007 Facility=Internet Severity = Error SymbolicName = XML_EXTENT_IN_ATTR
Language=English
Cannot reference an external general parsed entity '%1' in an attribute value.
.

MessageId = 0xE008 Facility=Internet Severity = Error SymbolicName = XML_STOPPED_BY_USER 
Language=English
XML parser stopped by user.
.

MessageId = 0xE009 Facility=Internet Severity = Error SymbolicName = XML_PARSING_ENTITY
Language=English
Error while parsing entity '%1'. %2
.

MessageId = 0xE00A  Facility=Internet    Severity = Error SymbolicName = XML_E_MISSING_PE_ENTITY
Language=English
Parameter entity must be defined before it is used.
.

MessageId = 0xE00B     Facility=Internet    Severity = Error SymbolicName = XML_E_MIXEDCONTENT_DUP_NAME
Language=English
The same name must not appear more than once in a single mixed-content declaration: '%1'. 
.

MessageId = 0xE00C     Facility=Internet    Severity = Error SymbolicName = XML_NAME_COLON
Language=English
Entity, EntityRef, PI, Notation names, or NMToken cannot contain a colon.
.

MessageId = 0xE00D    Facility=Internet   Severity = Error SymbolicName = XML_ELEMENT_UNDECLARED
Language=English
The element '%1' is used but not declared in the DTD/Schema.
.

MessageId = 0xE00E Facility=Internet Severity = Error SymbolicName = XML_ELEMENT_ID_NOT_FOUND
Language=English
The attribute '%1' references the ID '%2' which is not defined anywhere in the document.
.

MessageId = 0xE00F Facility=Internet Severity = Error SymbolicName = XML_DEFAULT_ATTRIBUTE
Language=English
Error in default attribute value defined in DTD/Schema.
.

MessageId = 0xE010 Facility=Internet Severity = Error SymbolicName = XML_XMLNS_RESERVED
Language=English
Reserved namespace '%1' can not be redeclared.
.

MessageId = 0xE011 Facility=Internet Severity = Error SymbolicName = XML_EMPTY_NOT_ALLOWED
Language=English
Element cannot be empty according to the DTD/Schema.
.

MessageId = 0xE012 Facility=Internet Severity = Error SymbolicName = XML_ELEMENT_NOT_COMPLETE
Language=English
Element content is incomplete according to the DTD/Schema.
.

MessageId = 0xE013 Facility=Internet Severity = Error SymbolicName = XML_ROOT_NAME_MISMATCH
Language=English
The name of the top most element must match the name of the DOCTYPE declaration.
.

MessageId = 0xE014 Facility=Internet Severity = Error SymbolicName = XML_INVALID_CONTENT
Language=English
Element content is invalid according to the DTD/Schema.
.

MessageId = 0xE015 Facility=Internet Severity = Error SymbolicName = XML_ATTRIBUTE_NOT_DEFINED
Language=English
The attribute '%1' on this element is not defined in the DTD/Schema.
.

MessageId = 0xE016 Facility=Internet Severity = Error SymbolicName = XML_ATTRIBUTE_FIXED
Language=English
Attribute '%1' has a value which does not match the fixed value defined in the DTD/Schema.
.

MessageId = 0xE017 Facility=Internet Severity = Error SymbolicName = XML_ATTRIBUTE_VALUE
Language=English
Attribute '%1' has an invalid value according to the DTD/Schema.
.

MessageId = 0xE018 Facility=Internet Severity = Error SymbolicName = XML_ILLEGAL_TEXT
Language=English
Text is not allowed in this element according to DTD/Schema.
.

MessageId = 0xE019 Facility=Internet Severity = Error SymbolicName = XML_MULTI_FIXED_VALUES
Language=English
An attribute declaration cannot contain multiple fixed values: '%1'.
.

MessageId = 0xE01A Facility=Internet Severity = Error SymbolicName = XML_NOTATION_DEFINED
Language=English
The notation '%1' is already declared.
.

MessageId = 0xE01B Facility=Internet Severity = Error SymbolicName = XML_ELEMENT_DEFINED
Language=English
The element '%1' is already declared.
.

MessageId = 0xE01C Facility=Internet Severity = Error SymbolicName = XML_ELEMENT_UNDEFINED
Language=English
Reference to undeclared element: '%1'.
.

MessageId = 0xE01D Facility=Internet Severity = Error SymbolicName = XML_XMLNS_UNDEFINED
Language=English
Reference to undeclared namespace prefix: '%1'.
.

MessageId = 0xE01E Facility=Internet Severity = Error SymbolicName = XML_XMLNS_FIXED
Language=English
Attribute '%1' must be a #FIXED attribute.
.

MessageId = 0xE01F    Facility=Internet   Severity = Error SymbolicName = XML_E_UNKNOWNERROR
Language=English
Unknown error: %1.
.

MessageId = 0xE020    Facility=Internet   Severity = Error SymbolicName = XML_REQUIRED_ATTRIBUTE_MISSING
Language=English
Required attribute '%1' is missing.
.

MessageId = 0xE021    Facility=Internet   Severity = Error SymbolicName = XML_MISSING_NOTATION
Language=English
Declaration '%1' contains reference to undefined notation '%2'.
.

MessageId = 0xE022    Facility=Internet   Severity = Error SymbolicName = XML_ATTLIST_DUPLICATED_ID
Language=English
Cannot define multiple ID attributes on the same element.
.

MessageId = 0xE023    Facility=Internet   Severity = Error SymbolicName = XML_ATTLIST_ID_PRESENCE
Language=English
An attribute of type ID must have a declared default of #IMPLIED or #REQUIRED.
.

MessageId = 0xE024    Facility=Internet   Severity = Error SymbolicName = XML_XMLLANG_INVALIDID
Language=English
The language ID "%1" is invalid.
.

MessageId = 0xE025    Facility=Internet   Severity = Error SymbolicName = XML_PUBLICID_INVALID
Language=English
The public ID "%1" is invalid.
.

MessageId = 0xE026 Facility=Internet Severity = Error SymbolicName = XML_DTD_EXPECTING
Language=English
Expecting: %1.
.

MessageId = 0xE027 Facility=Internet Severity = Error SymbolicName = XML_NAMESPACE_URI_EMPTY
Language=English
Only a default namespace can have an empty URI.
.

MessageId = 0xE028 Facility=Internet Severity = Error SymbolicName = XML_LOAD_EXTERNALENTITY
Language=English
Could not load '%1'.
.

MessageId = 0xE029 Facility=Internet Severity = Error SymbolicName = XML_BAD_ENCODING
Language=English
Unable to save character to '%1' encoding.
.

;//
;// Schema Error Messages
;//

MessageId = 0xE100    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATTRIBUTEVALUE_NOSUPPORT
Language=English
A namespace was found but not supported at current location.
.

MessageId = 0xE101    Facility=Internet   Severity = Error SymbolicName = SCHEMA_SCHEMAROOT_EXPECTED
Language=English
Incorrect definition for the root element in schema.
.

MessageId = 0xE102    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENT_NOSUPPORT
Language=English
Element "%1" is not allowed in this context.
.

MessageId = 0xE103    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETNAME_MISSING
Language=English
An ElementType declaration must contain a "name" attribute.
.

MessageId = 0xE104    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETYPE_MISSING
Language=English
An element declaration must contain a "type" attribute.
.

MessageId = 0xE105    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETORDER_UNKNOWN
Language=English
Schema only supports order type "seq", "one" and "many".
.

MessageId = 0xE106    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENTDT_NOSUPPORT
Language=English
Content must be "textOnly" when using datatype on an Element Type.
.

MessageId = 0xE107    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETORDER_DISABLED
Language=English
Order must be "many" when content is "mixed".
.

MessageId = 0xE108    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETCONTENT_UNKNOWN
Language=English
Content must be of type "empty","eltOnly","textOnly" or "mixed".
.

MessageId = 0xE109    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ETMODEL_UNKNOWN
Language=English
The value of model must be either "open" or "closed".
.

MessageId = 0xE10A    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENT_DISABLED
Language=English
Cannot contain child elements because content is set to "textOnly".
.

MessageId = 0xE10B    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENT_MISSING
Language=English
Must provide at least one "element" in a group.
.

MessageId = 0xE10C    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATTRIBUTE_NOTSUPPORT
Language=English
The attribute "%1" on an %2 is not supported.
.

;// AttributeType & attribute
MessageId = 0xE10D    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATNAME_MISSING
Language=English
AttributeType declaration must contain a "name" attribute.
.

MessageId = 0xE10E    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATNAME_DUPLICATED
Language=English
Duplicated attribute declaration.  
.

MessageId = 0xE111    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATREQUIRED_INVALID
Language=English
Invalid value for "required" attribute.
.

MessageId = 0xE112    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTTYPE_UNKNOWN
Language=English
Unknown Attribute datatype.
.

MessageId = 0xE113    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTTYPE_DUPLICATED
Language=English
Duplicated datatype declaration.
.

MessageId = 0xE114    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ENUMERATION_MISSING
Language=English
An element with a "values" attribute must contain a type attribute of the value "enumeration".
.

MessageId = 0xE115    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTVALUES_MISSING
Language=English
Must provide a "values" attribute on an element that contains a type attribute of the value "enumeration".
.

MessageId = 0xE116    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATYPE_MISSING
Language=English
Attribute declaration must contain a "type" attribute.
.

MessageId = 0xE117    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATYPE_UNDECLARED
Language=English
The specified attribute must first be declared using an AttributeType declaration.
.

MessageId = 0xE118    Facility=Internet   Severity = Error SymbolicName = SCHEMA_GROUP_DISABLED
Language=English
A "group" is not allowed within an ElementType that has a "textOnly" content model.
.

MessageId = 0xE119    Facility=Internet   Severity = Error SymbolicName = SCHEMA_GMATTRIBUTE_NOTSUPPORT
Language=English
The attribute "%1" on a group is not supported.
.

MessageId = 0xE11A    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTVALUES_VALUES_MISSING
Language=English
The values for enumeration type are missing.
.

MessageId = 0xE11B    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ATTRIBUTE_DEFAULTVALUE
Language=English
The default value "%1" is invalid.
.

MessageId = 0xE11C    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTTYPE_DISABLED
Language=English
Datatype is not allowed when content model is not "textOnly".
.

MessageId = 0xE11D    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENT_EMPTY
Language=English
Child element is not allowed when content model is "empty".
.

MessageId = 0xE11F    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENT_DATATYPE
Language=English
Child element is not allowed when datatype is set.
.

MessageId = 0xE120    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DTTYPE_MISSING
Language=English
Type is missing on the datatype element.
.

MessageId = 0xE121    Facility=Internet   Severity = Error SymbolicName = SCHEMA_MINOCCURS_INVALIDVALUE
Language=English
The value of attribute "minOccurs" should be "0" or "1".
.

MessageId = 0xE122    Facility=Internet   Severity = Error SymbolicName = SCHEMA_MAXOCCURS_INVALIDVALUE
Language=English
The value of attribute "maxOccurs" should be "1" or "*".
.

MessageId = 0xE123    Facility=Internet   Severity = Error SymbolicName = SCHEMA_MAXOCCURS_MUSTBESTAR
Language=English
The value of attribute "maxOccurs" must be "*" when attribute "order" is set to "many".
.

MessageId = 0xE124    Facility=Internet   Severity = Error SymbolicName = SCHEMA_ELEMENTDT_EMPTY
Language=English
The value of data type attribute can not be empty.
.

MessageId = 0xE125    Facility=Internet   Severity = Error SymbolicName = SCHEMA_DOCTYPE_INVALID
Language=English
DOCTYPE is not allowed in Schema.
.


;//
;//
;// DOM Object Model specific errors                           
;//
;//

MessageId = 0xE200    Facility=Internet   Severity = Error SymbolicName = XMLOM_DUPLICATE_ID
Language=English
The ID '%1' is duplicated.
.

MessageId = 0xE201    Facility=Internet   Severity = Error SymbolicName = XMLOM_DATATYPE_PARSE_ERROR
Language=English
Error parsing '%1' as %2 datatype.
.

MessageId = 0xE202    Facility=Internet   Severity = Error SymbolicName = XMLOM_NAMESPACE_CONFLICT
Language=English
There was a Namespace conflict for the '%1' Namespace.
.

MessageId = 0xE204   Facility=Internet   Severity = Error SymbolicName = XMLOM_OBJECT_EXPAND_NOTIMPL
Language=English
Unable to expand an attribute with Object value
.

MessageId = 0xE205   Facility=Internet   Severity = Error SymbolicName = XMLOM_DTDT_DUP
Language=English
Can not have 2 datatype attributes on one element.
.

MessageId = 0xE206   Facility=Internet   Severity = Error SymbolicName = XMLOM_INSERTPOS_NOTFOUND
Language=English
Insert position node not found
.

MessageId = 0xE207   Facility=Internet   Severity = Error SymbolicName = XMLOM_NODE_NOTFOUND
Language=English
Node not found
.

MessageId = 0xE208   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALIDTYPE
Language=English
This operation can not be performed with a Node of type %1.
.

MessageId = 0xE209   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_XMLDECL_ATTR
Language=English
'%1' is not a valid attribute on the XML Declaration.
Only 'version', 'encoding', or 'standalone' attributes are allowed.
.

MessageId = 0xE20A   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_INSERT_PARENT
Language=English
Inserting a Node or its ancestor under itself is not allowed.
.

MessageId = 0xE20B   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_INSERT_POS
Language=English
Insert position Node must be a Child of the Node to insert under.
.

MessageId = 0xE20C   Facility=Internet   Severity = Error SymbolicName = XMLOM_NO_ATTRIBUTES
Language=English
Attributes are not allowed on Nodes of type '%1'.
.

MessageId = 0xE20D   Facility=Internet   Severity = Error SymbolicName = XMLOM_NOTCHILD
Language=English
The parameter Node is not a child of this Node.
.

MessageId = 0xE20E   Facility=Internet   Severity = Error SymbolicName = XMLOM_CREATENODE_NEEDNAME
Language=English
createNode requires a name for given NodeType.
.

MessageId = 0xE20F   Facility=Internet   Severity = Error SymbolicName = XMLOM_UNEXPECTED_NS
Language=English
Unexpected NameSpace parameter.
.

MessageId = 0xE210   Facility=Internet   Severity = Error SymbolicName = XMLOM_MISSING_PARAM
Language=English
Required parameter is missing (or null/empty).
.

MessageId = 0xE211   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_NAMESPACENODE
Language=English
NameSpace Node is invalid.
.

MessageId = 0xE212   Facility=Internet   Severity = Error SymbolicName = XMLOM_READONLY
Language=English
Attempt to modify a read-only node.
.

MessageId = 0xE213   Facility=Internet   Severity = Error SymbolicName = XMLOM_ACCESSDENIED
Language=English
Access Denied.
.

MessageId = 0xE214   Facility=Internet   Severity = Error SymbolicName = XMLOM_ATTRMOVE
Language=English
Attributes must be removed before adding them to a different node.
.

MessageId = 0xE215   Facility=Internet   Severity = Error SymbolicName = XMLOM_BADVALUE
Language=English
Invalid data for a node of type '%1'.
.

MessageId = 0xE216   Facility=Internet   Severity = Error SymbolicName = XMLOM_USERABORT
Language=English
Operation aborted by caller.
.

MessageId = 0xE217   Facility=Internet   Severity = Error SymbolicName = XMLOM_NEXTNODEABORT
Language=English
Unable to recover node list iterator position.
.

MessageId = 0xE218   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_INDEX
Language=English
The offset must be 0 or a positive number that is not greater than the number of characters in the data.
.

MessageId = 0xE219   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_ATTR
Language=English
The provided node is not a specified attribute on this node.
.

;//

MessageId = 0xE21A   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_ONDOCTYPE
Language=English
This operation can not be performed on DOCTYPE node.
.

MessageId = 0xE21B   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_MODEL
Language=English
Cannot mix different threading models in document.
.

MessageId = 0xE21C   Facility=Internet   Severity = Error SymbolicName = XMLOM_INVALID_DATATYPE
Language=English
Datatype '%1' is not supported.
.

;// XSL Error Messages
;//

MessageId = 0xE300    Facility=Internet   Severity = Error SymbolicName = XSL_PROCESSOR_STACKOVERFLOW
Language=English
The XSL processor stack has overflowed - probable cause is infinite template recursion.
.

MessageId = 0xE301    Facility=Internet   Severity = Error SymbolicName = XSL_PROCESSOR_UNEXPECTEDKEYWORD
Language=English
Keyword %1 may not be used here.
.

MessageId = 0xE303    Facility=Internet   Severity = Error SymbolicName = XSL_PROCESSOR_BADROOTELEMENT
Language=English
The root element of XSL stylesheet must be <xsl:document> or <xsl:template>.
.

MessageId = 0xE304    Facility=Internet   Severity = Error SymbolicName = XSL_PROCESSOR_KEYWORDMAYNOTFOLLOW
Language=English
Keyword %1 may not follow %2.
.

MessageId = 0xE305    Facility=Internet   Severity = Error SymbolicName = XSL_PROCESSOR_INVALIDSCRIPTENGINE
Language=English
%1 is not a scripting language.
.

MessageId = 0xE306    Facility=Internet    Severity = Error SymbolicName = MSG_E_FORMATINDEX_BADINDEX
Language=English
The value passed in to formatIndex needs to be greater than 0.
.

MessageId = 0xE307    Facility=Internet    Severity = Error SymbolicName = MSG_E_FORMATINDEX_BADFORMAT
Language=English
Invalid format string.
.

MessageId = 0xE308    Facility=Internet    Severity = Error SymbolicName = XSL_PROCESSOR_SCRIPTERROR_LINE
Language=English
line = %1, col = %2 (line is offset from the <xsl:script> tag).
.

MessageId = 0xE309    Facility=Internet    Severity = Error SymbolicName = XSL_PROCESSOR_METHODERROR
Language=English
Error returned from property or method call.
.

;//
MessageId = 0xE30A    Facility=Internet    Severity = Error SymbolicName = MSG_E_SYSTEM_ERROR
Language=English
System error: %1.
.

MessageId = 0xE30B    Facility=Internet    Severity = Error SymbolicName = XSL_KEYWORD_MAYNOTCONTAIN
Language=English
Keyword %1 may not contain %2.
.


;// XQL Error Messages
;//
MessageId = 0xE380    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTED_TOKEN
Language=English
Expected token %1 found %2.
.

MessageId = 0xE381    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTED_TOKEN
Language=English
Unexpected token %1.
.

MessageId = 0xE382    Facility=Internet   Severity = Error SymbolicName = XQL_EXPR_NOT_DOM_NODE
Language=English
Expression does not return a DOM node.
.

MessageId = 0xE383    Facility=Internet   Severity = Error SymbolicName = XQL_EXPR_NOT_QUERY_OR_INTEGER
Language=English
Expression must be a query or an integer constant.
.

MessageId = 0xE384    Facility=Internet   Severity = Error SymbolicName = XQL_INCOMPLETE_QUERY
Language=English
Incomplete query expression.
.

MessageId = 0xE385    Facility=Internet   Severity = Error SymbolicName = XQL_UNKNOWN_METHOD
Language=English
Unknown method.
.

MessageId = 0xE386    Facility=Internet   Severity = Error SymbolicName = XQL_UNEXPECTED_CHAR
Language=English
Unexpected character in query string.
.

MessageId = 0xE387    Facility=Internet   Severity = Error SymbolicName = XQL_QUERY_INVALID_HERE
Language=English
%1 may not appear to the right of / or // or be used with |.
.

MessageId = 0xE388    Facility=Internet   Severity = Error SymbolicName = XQL_EXPR_NOT_STRING
Language=English
Expression must be a string constant.
.

MessageId = 0xE389    Facility=Internet   Severity = Error SymbolicName = XQL_METHOD_NOT_SUPPORTED
Language=English
Object does not support this method.
.

MessageId = 0xE38A    Facility=Internet   Severity = Error SymbolicName = XQL_INVALID_CAST
Language=English
Expression can't be cast to this data type.
.

MessageId = 0xE38C    Facility=Internet    Severity = Error SymbolicName = XMLISLANDS_NOSCRIPTLETS
Language=English
The XML script engine does not support script fragments.  This error was probably caused by having a script tag with the language attribute set to "XML" or the text attribute set to "text/xml" before any other script tag on the page.
.

MessageId = 0xE38D    Facility=Internet   Severity = Error SymbolicName = XQL_EXPR_NOT_QUERY_OR_STRING
Language=English
Parameter must be a query or a string constant.
.

MessageId = 0xE38E    Facility=Internet   Severity = Error SymbolicName = XQL_EXPR_NOT_INTEGER
Language=English
Parameter must be a integer constant.
.

;//
;// MIMETYPE Viewer Errors
;//
MessageId = 0xE400    Facility=Internet   Severity = Error SymbolicName = XMLMIME_ERROR
Language=English
<%1TABLE width=400>
<%1P style="font:13pt/15pt verdana"> The XML page cannot be displayed
<%1P style="font:8pt/11pt verdana">Cannot view XML input using %3 style sheet.
Please correct the error and then click the 
<%1a href="javascript:location.reload()" target="_self">
Refresh</%1a> button, or try again later.
<%1HR><%1P style="font:bold 8pt/11pt verdana">%2</%1P><%1/TABLE>
<%1pre style="font:10pt/12pt"><%1font color="blue">%4</%1font></%1pre>
.

MessageId = 0xE401    Facility=Internet   Severity = Error SymbolicName = XMLMIME_LINEPOS
Language=English
Line %1, Position %2
.

MessageId = 0xE402    Facility=Internet   Severity = Error SymbolicName = XMLMIME_SOURCENA
Language=English
The XML source file is unavailable for viewing.
.

;//                                                                           
;//                                                                           
;// XMLPSR specific errors                           
;//             
;// I had to change the symbolic names to MSG_ so they didn't clash with
;// the named defined in xmlparser.idl.  The important thing is that they
;// have the same message id.                                                              
;//                                                                           

;// -------- READ ME !!!! -------------
;//
;// The broken unix mc compiler (a perl script) needs the symbolic name
;// and Severity on the same line as the Message ID, to do otherwise will break
;// it. Language can be on a seperate line
;//
;// -----------------------------------

;// ----------- character level error codes -----------------------------

MessageId = 0xE501    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSINGEQUALS
Language=English
Missing equals sign between attribute and attribute value.
.

MessageId = 0xE502    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSINGQUOTE
Language=English
A string literal was expected, but no opening quote character was found.
.

MessageId = 0xE503    Facility=Internet   Severity = Error SymbolicName = MSG_E_COMMENTSYNTAX
Language=English
Incorrect syntax was used in a comment.
.

MessageId = 0xE504    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADSTARTNAMECHAR
Language=English
A name was started with an invalid character.
.

MessageId = 0xE505    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADNAMECHAR
Language=English
A name contained an invalid character.
.

MessageId = 0xE506    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINSTRING
Language=English
The character '<' cannot be used in an attribute value.
.

MessageId = 0xE507    Facility=Internet   Severity = Error SymbolicName = MSG_E_XMLDECLSYNTAX
Language=English
Invalid syntax for an xml declaration.
.

MessageId = 0xE508    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARDATA
Language=English
An Invalid character was found in text content.
.

MessageId = 0xE509    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSINGWHITESPACE
Language=English
Required white space was missing.
.

MessageId = 0xE50A    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTINGTAGEND
Language=English
The character '>' was expected.
.

MessageId = 0xE50B    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINDTD
Language=English
Invalid character found in DTD.
.

MessageId = 0xE50C    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINDECL
Language=English
An invalid character was found inside a DTD declaration.
.

MessageId = 0xE50D    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSINGSEMICOLON
Language=English
A semi colon character was expected.
.

MessageId = 0xE50E    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINENTREF
Language=English
An invalid character was found inside an entity reference.
.

MessageId = 0xE50F    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNBALANCEDPAREN
Language=English
Unbalanced parentheses.
.

MessageId = 0xE510    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTINGOPENBRACKET
Language=English
An opening '[' character was expected.
.

MessageId = 0xE511    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADENDCONDSECT
Language=English
Invalid syntax in a conditional section.
.

MessageId = 0xE512    Facility=Internet   Severity = Error SymbolicName = MSG_E_INTERNALERROR
Language=English
Internal error.
.

MessageId = 0xE513    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTED_WHITESPACE
Language=English
Whitespace is not allowed at this location.
.

MessageId = 0xE514    Facility=Internet   Severity = Error SymbolicName = MSG_E_INCOMPLETE_ENCODING
Language=English
End of file reached in invalid state for current encoding.
.

MessageId = 0xE515    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINMIXEDMODEL
Language=English
Mixed content model cannot contain this character.
.

MessageId = 0xE516    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSING_STAR
Language=English
Mixed content model must be defined as zero or more('*').
.

MessageId = 0xE517    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINMODEL
Language=English
Invalid character in content model.
.

MessageId = 0xE518    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSING_PAREN
Language=English
Missing parenthesis.
.

MessageId = 0xE519    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADCHARINENUMERATION
Language=English
Invalid character found in ATTLIST enumeration.
.

MessageId = 0xE51A   Facility=Internet   Severity = Error SymbolicName = MSG_E_PIDECLSYNTAX
Language=English
Invalid syntax in PI declaration.
.

MessageId = 0xE51B    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTINGCLOSEQUOTE
Language=English
A single or double closing quote character (\' or \") is missing.
.

MessageId = 0xE51C    Facility=Internet   Severity = Error SymbolicName = MSG_E_MULTIPLE_COLONS
Language=English
Multiple colons are not allowed in a name.
.

MessageId = 0xE51D  Facility=Internet    Severity = Error SymbolicName = MSG_E_INVALID_DECIMAL
Language=English
Invalid character for decimal digit.
.

MessageId = 0xE51E  Facility=Internet    Severity = Error SymbolicName = MSG_E_INVALID_HEXIDECIMAL
Language=English
Invalid character for hexidecimal digit.
.

MessageId = 0xE51F  Facility=Internet    Severity = Error SymbolicName = MSG_E_INVALID_UNICODE
Language=English
Invalid unicode character value for this platform.
.

MessageId = 0xE520  Facility=Internet    Severity = Error SymbolicName = MSG_E_WHITESPACEORQUESTIONMARK
Language=English
Expecting whitespace or '?'.
.


;// ----------- token level error codes -----------------------------

MessageId = 0xE550   Facility=Internet   Severity = Error SymbolicName = MSG_E_SUSPENDED
Language=English
The parser is suspended.
.

MessageId = 0xE551   Facility=Internet   Severity = Error SymbolicName = MSG_E_STOPPED
Language=English
The parser is stopped.
.

MessageId = 0xE552    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTEDENDTAG
Language=English
End tag was not expected at this location.
.

MessageId = 0xE553   Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDTAG
Language=English
The following tags were not closed: %1.
.

MessageId = 0xE554    Facility=Internet   Severity = Error SymbolicName = MSG_E_DUPLICATEATTRIBUTE
Language=English
Duplicate attribute.
.

MessageId = 0xE555    Facility=Internet   Severity = Error SymbolicName = MSG_E_MULTIPLEROOTS
Language=English
Only one top level element is allowed in an XML document.
.

MessageId = 0xE556    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALIDATROOTLEVEL
Language=English
Invalid at the top level of the document.
.

MessageId = 0xE557    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADXMLDECL
Language=English
Invalid xml declaration.
.

MessageId = 0xE558    Facility=Internet   Severity = Error SymbolicName = MSG_E_MISSINGROOT
Language=English
XML document must have a top level element.
.

MessageId = 0xE559    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTEDEOF
Language=English
Unexpected end of file.
.

MessageId = 0xE55A    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADPEREFINSUBSET
Language=English
Parameter entities cannot be used inside markup declarations in an internal subset.
.

MessageId = 0xE55B    Facility=Internet   Severity = Error SymbolicName = MSG_E_PE_NESTING
Language=English
The replacement text for a parameter entity must be properly
nested with parenthesized groups.
.

MessageId = 0xE55C    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_CDATACLOSINGTAG
Language=English
The literal string ']]>' is not allowed in element content.
.

MessageId = 0xE55D    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDPI
Language=English
Processing instruction was not closed.
.

MessageId = 0xE55E    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDSTARTTAG
Language=English
Element was not closed.
.

MessageId = 0xE55F    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDENDTAG
Language=English
End element was missing the character '>'.
.

MessageId = 0xE560    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDSTRING
Language=English
A string literal was not closed.
.

MessageId = 0xE561    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDCOMMENT
Language=English
A comment was not closed.
.

MessageId = 0xE562    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDDECL
Language=English
A declaration was not closed.
.

MessageId = 0xE563    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDMARKUPDECL
Language=English
A markup declaration was not closed.
.

MessageId = 0xE564    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNCLOSEDCDATA
Language=English
A CDATA section was not closed.
.
MessageId = 0xE565    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADDECLNAME
Language=English
Declaration has an invalid name.
.

MessageId = 0xE566    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADEXTERNALID
Language=English
External ID is invalid.
.

MessageId = 0xE567    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADELEMENTINDTD
Language=English
An XML element is not allowed inside a DTD.
.

MessageId = 0xE568    Facility=Internet   Severity = Error SymbolicName = MSG_E_RESERVEDNAMESPACE
Language=English
The namespace prefix is not allowed to start with the reserved string "xml".
.

MessageId = 0xE569    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTING_VERSION
Language=English
The 'version' attribute is required at this location.
.

MessageId = 0xE56A    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTING_ENCODING
Language=English
The 'encoding' attribute is required at this location.
.

MessageId = 0xE56B    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTING_NAME
Language=English
At least one name is required at this location.
.

MessageId = 0xE56C    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTED_ATTRIBUTE
Language=English
The specified attribute was not expected at this location.
The attribute may be case sensitive.
.

MessageId = 0xE56D    Facility=Internet   Severity = Error SymbolicName = MSG_E_ENDTAGMISMATCH
Language=English
End tag '%2' does not match the start tag '%1'.
.

MessageId = 0xE56E    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALIDENCODING
Language=English
System does not support the specified encoding.
.

MessageId = 0xE56F    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALIDSWITCH
Language=English
Switch from current encoding to specified encoding not supported.
.

MessageId = 0xE570    Facility=Internet   Severity = Error SymbolicName = MSG_E_EXPECTING_NDATA
Language=English
NDATA keyword is missing.
.

MessageId = 0xE571    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_MODEL
Language=English
Content model is invalid.
.

MessageId = 0xE572    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_TYPE
Language=English
Invalid type defined in ATTLIST.
.

MessageId = 0xE573    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALIDXMLSPACE
Language=English
XML space attribute has invalid value.  Must specify 'default' or 'preserve'.
.

MessageId = 0xE574    Facility=Internet   Severity = Error SymbolicName = MSG_E_MULTI_ATTR_VALUE
Language=English
Multiple names found in attribute value when only one was expected.
.

MessageId = 0xE575    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_PRESENCE
Language=English
Invalid ATTDEF declaration.  Expected #REQUIRED, #IMPLIED or #FIXED.
.

MessageId = 0xE576    Facility=Internet   Severity = Error SymbolicName = MSG_E_BADXMLCASE
Language=English
The name 'xml' is reserved and must be lower case.
.

MessageId = 0xE577    Facility=Internet   Severity = Error SymbolicName = MSG_E_CONDSECTINSUBSET
Language=English
Conditional sections are not allowed in an internal subset.
.

MessageId = 0xE578    Facility=Internet   Severity = Error SymbolicName = MSG_E_CDATAINVALID
Language=English
CDATA is not allowed in a DTD.
.

MessageId = 0xE579    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_STANDALONE
Language=English
The standalone attribute must have the value 'yes' or 'no'.
.

MessageId = 0xE57A    Facility=Internet   Severity = Error SymbolicName = MSG_E_UNEXPECTED_STANDALONE
Language=English
The standalone attribute cannot be used in external entities.
.

MessageId = 0xE57B    Facility=Internet   Severity = Error SymbolicName = MSG_E_DOCTYPE_IN_DTD
Language=English
Cannot have a DOCTYPE declaration in a DTD.
.

MessageId = 0xE57C     Facility=Internet    Severity = Error SymbolicName = MSG_E_MISSING_ENTITY
Language=English
Reference to undefined entity.
.

MessageId = 0xE57D     Facility=Internet    Severity = Error SymbolicName = MSG_E_ENTITYREF_INNAME
Language=English
Entity reference is resolved to an invalid name character.
.

MessageId = 0xE57E    Facility=Internet   Severity = Error SymbolicName = MSG_E_DOCTYPE_OUTSIDE_PROLOG
Language=English
Cannot have a DOCTYPE declaration outside of a prolog.
.

MessageId = 0xE57F    Facility=Internet   Severity = Error SymbolicName = MSG_E_INVALID_VERSION
Language=English
Invalid version number.
.

MessageId = 0xE580    Facility=Internet   Severity = Error SymbolicName = MSG_E_DTDELEMENT_OUTSIDE_DTD
Language=English
Cannot have a DTD declaration outside of a DTD.
.

MessageId = 0xE581    Facility=Internet   Severity = Error SymbolicName = MSG_E_DUPLICATEDOCTYPE
Language=English
Cannot have multiple DOCTYPE declarations.
.

MessageId = 0xE582  Facility=Internet    Severity = Error SymbolicName = MSG_E_RESOURCE
Language=English
Error processing resource '%1'.
.

;//  DO NOT ADD ANYTHING OTHER THAN TOKENIZER ERRORS HERE -- SEE OTHER SECTIONS ABOVE...
