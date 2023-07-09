/*
 * @(#)Element.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Name;
import com.ms.xml.util.Attributes;
import com.ms.xml.util.XMLOutputStream;

import java.lang.String;
import java.util.Enumeration;
import java.io.IOException;

/**
 * 
 * This interface implements an Element object. Each XML tag in a document is represented by an Element object in
 * the XML parse tree. The elements are named with a string, have 
 * attributes, and can contain child nodes.
 * <P>
 * There are seven types of elements, DOCUMENT, ELEMENT, PCDATA, PI,
 * META, COMMENT, and CDATA.
 *
 * @version 1.0, 6/3/97
 */
public interface Element
{
//  Types 0 through 4 match the element types exposed by the
//  IDL interface to the C++ parser.

    /**
     * A general container element having optional attributes
     * and optional child elements.
     */
    static public final int ELEMENT = 0;
    /**
     * A text element that has no children or attributes and that
     * contains parsed character data.
     */
    static public final int PCDATA = 1;
    /**
     * An XML comment ( &lt;!-- ... --&gt; ).
     */
    static public final int COMMENT = 2;
    /**
     * Reserved for use by the Document node only.
     */
    static public final int DOCUMENT = 3;
    /**
     * Reserved for use by the DTD node only.
     */
    static public final int DTD = 4;
    /**
     * A processing instruction node ( &lt;? ... ?&gt; ).
     */
    static public final int PI = 5;
    /**
     * Raw character data specified with special CDATA construct:
     * &lt;![CDATA[...]]&gt;
     * where ... can be anything except ]]&gt;, including HTML tags. 
     */
    static public final int CDATA = 6;     
    /**
     * An entity node. 
     */
    static public final int ENTITY = 7;     
    /**
     * A notation node. 
     */
    static public final int NOTATION = 8;     
    /**
     * An element declaration node. 
     */
    static public final int ELEMENTDECL = 9;     
    /**
     * A namespace node that declares new namespaces in the element tree.
     */
    static public final int NAMESPACE = 10;
    /**
     * Entity reference nodes.
     */
    static public final int ENTITYREF = 11;
    /**
     * Ignorable white space between elements.
     */
    static public final int WHITESPACE = 12;
    /**
     * INCLUDE conditional section
     */
    static public final int INCLUDESECTION = 13;
    /**
     * IGNORE conditional section
     */
    static public final int IGNORESECTION = 14;

    /** 
     * Retrieves the name of the tag as a string. The string
     * will be in uppercase.
     * 
     * @return the tag name or null for DATA and PCDATA elements. 
     */
    public Name getTagName();

    /**
     * Retrieves the type of the element.
     * This is always one of the following values:
     * <code>DOCUMENT</code>, <code>ELEMENT</code>, <code>PCDATA</code>, <code>PI</code>,
     * <code>META</code>, <code>COMMENT</code>, or <code>CDATA</code>.
     * 
     * @return element type.
     */    
    public int getType();

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
    public String getText();
    
    /**
     * Sets the text for this element. Only meaningful in 
     * <code>CDATA</code>, <code>PCDATA</code>, and <code>COMMENT</code> nodes.
     *
     * @param text The text to set.
      * @return No return value.
     */    
    public void setText(String text);

    /** 
     * Retrieves the parent of this element. 
     * Every element in the tree except the Document itself, has
     * a parent.  
     *
     * @return the parent element or null if at the root of 
     * the tree.
     */
    public Element getParent();

    /**
     * Returns an enumeration of the children of this element.
     * <code>Enumeration.nextElement</code> returns <code>Element</code> objects.
     * 
     * @return an enumeration of child objects. It must not return null. 
     * See <code>EnumWrapper</code> for an easy way to return an empty enumeration.
     */
    public Enumeration getElements();

    /**
     * Returns an element collection of the children of this element.
     * It must not return null. See <code>EnumWrapper</code> for an emptyEnumeration. 
     * @return ElementCollection of child objects.
     */
    public ElementCollection getChildren();

    /**
     * Retrieves the number of child elements.
     * @return the number of child elements.
     */
    public int numElements();

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
    public void addChild(Element elem, Element after);
    /**
     * Adds a child to this element. 
     * @param elem The element to add.
     * @param pos  The position to add this element (calling <code>getChild(pos)</code> 
     * will return this element). If <i>pos</i> is less than 0, <i>elem</i> becomes 
     * the new last element.
     * @param reserved The reserved parameter.
      * @return No return value.
     */
    public void addChild(Element elem, int pos, int reserved);

     /**
      * Retrieves the child element by index.
      * @param index The index of the child element.
      * @return null if there is no child by that index.
      */   
    public Element getChild(int index);
    
     /**
      * Removes a child element from the tree.
      *
      * @param elem  The element to remove.
      */   
    public void removeChild(Element elem);
    
    /**
     * Retrieves an enumeration for the element attributes.
     * 
     * The enumeration returns <code>Attribute</code> objects.
     * @return the enumeration. It must not return null (see <code>EnumWrapper</code>
     * for returning empty enumerations).
     * @see Attribute
     */
    public Enumeration getAttributes();

    /**
     * Retrieves the number of attributes.
     * @return the number of attributes.
     */
    public int numAttributes();

    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute 
     * or null if the attribute is not found.
     */    
    public Object getAttribute(String name);
    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute 
     * or null if the attribute is not found.
     */    
    public Object getAttribute(Name n);
    
    /**
     * Sets the attribute of this element.    
     *
     * @param name  The attribute name.
     * @param value The attribute value.
     */    
    public void setAttribute(String name, Object value);
    /**
     * Sets the attribute of this element.    
     *
     * @param name  The attribute name.
     * @param value The attribute value.
     */    
    public void setAttribute(Name name, Object value);

    
    /**
     * Deletes an attribute from an element.
     * @param name The attribute to delete.
      * @return No return value.
     */    
    public void removeAttribute(String name);
    /**
     * Deletes an attribute from an element.
     * @param name The attribute to delete.
      * @return No return value.
     */    
    public void removeAttribute(Name name);

    /**
     * Sets the parent of this element.
     * @param parent The element to set as parent.
      * @return No return value.
     */
    public void setParent(Element parent);

    /**
     * Returns the XML-DATA specification for the DTD element.
     * (Only applies to ElementDecl and Entity nodes).
     * This is DTD information represented in an XML Object Model.
     * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">Specification for XML-Data</a> for details.
     */
    public Element toSchema(); 

    /**
     * Saves this element.
     * @param o The output stream to save to.
     * @exception IOException if there is a problem saving the output.
      * @return No return value.
     */
    public void save(XMLOutputStream o) throws IOException;

}


