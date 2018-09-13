/*
 * @(#)Element.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _PARSER_OM_ELEMENT
#define _PARSER_OM_ELEMENT

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx"
#endif

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

#ifndef _XQL_QUERY_OPERANDVALUE
#include "xql/query/operandvalue.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(ElementCollection);
DEFINE_CLASS(Document);

class NameDef;

class OutputHelper;

/**
 * This interface implements an Element object. Each XML tag in a document is represented 
 * by an Element object in the XML parse tree. The elements are named with a string, have 
 * attributes, and can contain child nodes.
 * <P>
 * There are seven types of elements, DOCUMENT, ELEMENT, PCDATA, PI,
 * META, COMMENT, and CDATA.
 *
 * @version 1.0, 6/3/97
 */

const IID IID_Element = {0x4014F154,0x4CD2,0x11d1,{0xA6, 0x96,0x00,0xC0,0x4F,0xD9,0x15,0x55}};

class DECLSPEC_UUID("4014F154-4CD2-11d1-A696-00C04FD91555")
NOVTABLE Element : public Base
{
    protected:  Element() : Base() {}

    //  Types 0 through 4 match the element types exposed by the
    //  IDL interface to the C++ parser.
    public: enum NodeType 
    {
    /**
     * ANY is the null value for Element types. Elements never have this
     * value but it is used by XQL to indicate no type has been set.
     */
    ANY = -1,
    /**
     * A general container element having optional attributes
     * and optional child elements.
     */
    ELEMENT = 0,
    /**
     * A text element that has no children or attributes and that
     * contains parsed character data.
     */
    PCDATA = 1,
    /**
     * An XML comment ( &lt;!-- ... --&gt; ).
     */
    COMMENT = 2,
    /**
     * Reserved for use by the Document node only.
     */
    DOCUMENT = 3,
    /**
     * Reserved for use by the DTD node only.
     */
    DOCTYPE = 4,
    /**
     * A processing instruction node ( &lt;? ... ?&gt; ).
     */
    PI = 5,
    /**
     * Raw character data specified with special CDATA construct:
     * &lt;![CDATA[...]]&gt;
     * where ... can be anything except ]]&gt; including HTML tags. 
     */
    CDATA = 6,     
    /**
     * An entity nodes. 
     */
    ENTITY = 7,     
    /**
     * A notation nodes. 
     */
    NOTATION = 8,     
    /**
     * An element * declaration nodes. 
     */
    ELEMENTDECL = 9,     
    /**
     * A name space node that declares new name spaces in the element tree
     */
    __NAMESPACE = 10,
    /**
     * Entity reference nodes
     */
    ENTITYREF = 11,
    /**
     * Ignorable white space between elements.
     */
    __WHITESPACE = 12,
     /**
     * INCLUDE conditional section
     */
    __INCLUDESECTION = 13,
    /**
     * IGNORE conditional section
     */
    __IGNORESECTION = 14,
    /**
     * ATTRIBUTE node
     */
    ATTRIBUTE = 15,
    /**
     *  node storing typed value
     */
    TYPEDVALUE = 16,
    /**
     * Document Fragment
     */
    DOCFRAG = 17,
    /**
     * XML Decl PI
     */
    XMLDECL = 18,
    };

    /** 
     * Retrieves the name of the tag as a string. 
     * 
     * @return the tag name or null for DATA and PCDATA elements. 
     */
    public: virtual Name * getTagName() = 0;

    public: virtual NameDef * getNameDef() = 0;

    /** 
     * Set the name of the tag as a string. 
     */
    public: virtual void setTagName(String* name) = 0;

    /**
     * Retrieves the type of the element.
     * This is always one of the following values:
     * <code>DOCUMENT</code>, <code>ELEMENT</code>, <code>PCDATA</code>, <code>PI</code>,
     * <code>META</code>, <code>COMMENT</code>, or <code>CDATA</code>.
     * 
     * @return element type.
     */    
    public: virtual int getType() = 0;

    /** 
     * Returns the non-marked-up text contained by this element.
     * For text elements, this is the raw data.  For elements
     * with child nodes, this traverses the entire subtree and
     * appends the text for each terminal text element, effectively
     * stripping out the XML markup for the subtree. For example,
     * if the XML document contains the following:
     * <xmp>
     *      <AUTHOR>
     *          <FIRST>William</FIRST>
     *          <LAST>Shakespeare</LAST>
     *      </AUTHOR>
     * </xmp>
     * <p><code>Document.getText</code> returns "William Shakespeare".
     */
    public: virtual String * getText(bool fForceCollapse = false, bool fNormalize = false) = 0;
    public: virtual String * getInnerText(bool fPreserve=false, bool fIgnoreXmlSpace=false, bool fNormalize=true) = 0;
    
    /**
     * Sets the text for this element. Only meaningful in 
     * <code>CDATA</code>, <code>PCDATA</code>, and <code>COMMENT</code> nodes.
     *
     * @param text The text to set.
     * @return No return value.
     */    
    public: virtual void setText(String * text) = 0;

    /** 
     * Retrieves the parent of this element. 
     * Every element in the tree except the Document itself, has
     * a parent.  
     *
     * @return the parent element or null if at the root of 
     * the tree.
     */
    public: virtual Element * getParent() = 0; 


    /**
     * Returns the first child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getFirstChild(HANDLE * ph) = 0;

    /**
     * Returns the next child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getNextChild(HANDLE * ph) = 0;

    /**
     * Retrieves the number of child elements.
     * @return the number of child elements.
     */
    public: virtual int numElements() = 0;

    public: virtual int hasChildren() = 0;

    /**
     * Adds a child to this element. Any element can only
     * have one parent element and so the previous parent will lose this child
     * from its subtree.  
     *
     * @param elem  The element to add.      
     * The child element becomes the last element if <i>after</i> is null.
     * The child is added to the beginning of the list if <i>after</i> is this object.
     * @param after The element after which to add it. 
     * @return No return value.
     */
    public: virtual void addChild(Element * elem, Element * eBefore) = 0;

    /**
     * Adds a child to this element. 
     * @param elem The element to add.
     * @param pos  The position to add this element (calling <code>getChild(pos)</code> 
     * will return this element). If <i>pos</i> is less than 0, <i>elem</i> becomes 
     * the new last element.
     * @param reserved The reserved parameter.
     * @return No return value.
     */
    public: virtual void addChildAt(Element * elem, int pos) = 0;

    /**
     * Removes a child element from the tree.
     *
     * @param elem  The element to remove.
     */   
    public: virtual void removeChild(Element * elem) = 0;

    /**
     * Returns the index of an element
     *
     * @param name Optional name to filter on
    */
    public: virtual long getIndex(bool fRelative) = 0;
    
    /**
     * Returns the first child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getFirstAttribute(HANDLE * ph) = 0;

    /**
     * Returns the next child of this element. The <code>HANDLE</code> must be
     * passed to <code>getNextChild</code>.
     */
    public: virtual Element * getNextAttribute(HANDLE * ph) = 0;

    /**
     * Retrieves the number of attributes.
     * @return the number of attributes.
     */
    public: virtual int numAttributes() = 0;

    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute 
     * or null if the attribute is not found.
     */    
    public: virtual Object * getAttribute(Name * n) = 0;
    
    /**
     * Sets the attribute of this element.    
     *
     * @param name  The attribute name.
     * @param value The attribute value.
     */
    public: virtual void setAttribute(Name * name, Object * value) = 0;

    /**
     * Deletes an attribute from an element.
     * @param name The attribute to delete.
     * @return No return value.
     */    
    public: virtual void removeAttribute(Name * name) = 0;

    public: virtual Document * getDocument() = 0;

    // ***** new type support *****

    /**
     * Determines if element is typed.
     */
    public: virtual unsigned isTyped() = 0;

    /**
     * Gets typed value into the variant.
     */
    public: virtual void getTypedValue(VARIANT * pVar) = 0;

    public: virtual void getTypedValue(DataType dt, VARIANT * pVar) = 0;

    /**
     * Sets typed value.
     */
    public: virtual void setTypedValue(VARIANT * pVar) = 0;

    /**
     * Gets typed attribute into the variant.
     */
    public: virtual void getTypedAttribute(Name * n, VARIANT * pVar) = 0;

    /**
     * Sets typed attribute from the variant.
     */
    public: virtual void setTypedAttribute(Name * n, VARIANT * pVar) = 0;

    /**
     * Gets data type string.
     */
    public: virtual String * getDataTypeString() = 0;

    /**
     * Sets data type string.
     */
    public: virtual void setDataTypeString(String * pS) = 0;

    // Methods so that Element and Attribute can be merged

    /**
     * Return the attribute type.
     */
    public: virtual DataType getAttributeType() = 0;

    /**
     * Return attribute's value.
     */
    public: virtual Object * getValue() = 0;

    /**
     * Set attribute's value.
     */
    public: virtual void setValue(Object * o) = 0;

    /**
     * Whitespace inside of this element, e.g.
     *      <TR>
     *          <TD>XYZ</TD>
     *      </TR>
     *  
     *  hasWSInside is true for the TR because of the WS between it and the first TD
     */

    public: virtual bool hasWSInside() = 0;

    /**
     * Whitespace inside of this element, e.g.
     *      <TR>
     *          <TD>XYZ</TD>
     *      </TR>
     *  
     *  hasWSAfter  is true for the TD because of the WS between it and the first </TD>
     */

    public: virtual bool hasWSAfter() = 0;


    /**
     * Compare element with operand using relop and return TRI_TRUE, TRI_FALSE or TRI_UNKNOWN
     */

    public: virtual TriState compare(OperandValue::RelOp op, DataType dt, OperandValue * popval) = 0;

    /**
     * Compare element with operand
     * op is the relop to use (<, <=, = , >=, >, !=)
     * dt is the datatype to use when comparing
     * returns true if element can be compared with operand using the datatype
     *  if true then presult is either -1, 0, 1 with the usable meaning of less than, equal or 
     *  greater than
     *  if false then presult is undefined
     */

    public: virtual int compare(DWORD dwCmpFlags, DataType dt, OperandValue * popval, int * presult) = 0;

    /**
     * Gets the element's value as an OperandValue
     */

    public: virtual void getValue(DataType dt, OperandValue * popval) = 0;

    /**
     * Returns an element's data type
     */

    public: virtual DataType getDataType() = 0;

};

Element * GetElement(IDispatch * p);

class ElementLock : public MutexLock
{
    public: ElementLock(Element * pElem);
};



#endif _PARSER_OM_ELEMENT


