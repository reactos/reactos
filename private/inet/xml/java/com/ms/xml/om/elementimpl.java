/*
 * @(#)ElementImpl.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Attribute;
import com.ms.xml.util.Attributes;
import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.util.EnumWrapper;
import com.ms.xml.parser.ElementDecl;
import com.ms.xml.parser.AttDef;
import com.ms.xml.parser.DTD;

import java.io.IOException;
import java.lang.String;
import java.util.Vector;
import java.util.Enumeration;

/**
 * This class represents the default implementation of the <code>Element</code> interface. 
 * These are 
 * created by the XML parser when using the default element factory. 
 * @see Element
 * @see Parser
 * @see ElementFactory
 * @see ElementFactoryImpl
 *
 * @version 1.0, 6/3/97
 */
public class ElementImpl implements Element
{
    public ElementImpl()
    {
        this(null, Element.ELEMENT);
    }
    
    public ElementImpl(Name tag, int type)
    {
        this.tag = tag;
        this.type = type;
    }

    /** 
     * Retrieves the name of the tag as a string. The string
     * will be in uppercase.
     * 
     * @return the tag name or null for DATA and PCDATA elements. 
     
     */
    public Name getTagName()
    {
        return tag;
    }

    /**
     * Retrieves the type of the element.
     * This is always one of the following values:
     * <code>DOCUMENT</code>, <code>ELEMENT</code>, <code>PCDATA</code>, <code>PI</code>,
     * <code>META</code>, <code>COMMENT</code>, or <code>CDATA</code>.
     * 
     * @return the element type.
     */    
    public int getType()
    {
        return type;
    }

     /** 
     * Returns the non-marked-up text contained by this element.
     * For text elements, this is the raw data. For elements
     * with child nodes, this method traverses the entire subtree and
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
    public String getText()
    {
        if (type == ENTITYREF)
        {
            Document d = getDocument();
            if (d != null)
            {
                DTD dtd = d.getDTD();
                if (dtd != null) 
                {
                    Element ent = dtd.findEntity(getTagName());
                    if (ent != null) 
                    {
                        return ent.getText();
                    }
                }
            }
        }
        else if (children == null || type == CDATA || type == PCDATA || type == WHITESPACE)
        {
            return text == null ? new String("") : text;
        }
        else if (children != null)
        {
            StringBuffer sb = new StringBuffer();

            for (Enumeration en = children.elements(); en.hasMoreElements(); )
            {
                sb.append(((Element)en.nextElement()).getText());
            }

            return sb.toString();
        }
        
        return "";
    }
    
    /**
     * Sets the text for this element. This text is Only meaningful in 
     * <code>CDATA</code>, <code>PCDATA</code>, and <code>COMMENT</code> nodes.
     *
     * @param The text to set.
     */    
    public void setText(String text)
    {
        // No restrictions here, so that subclasses can use this.
        this.text = text;
    }

    /** 
     * Retrieves the parent of this element. 
     * Every element in the tree except the Document object  itself, has
     * a parent.  
     *
     * @return the parent element or null if  the element is at the root of 
     * the tree.
     */
    public Element getParent()
    {
        return parent;
    }

    /**
     * Sets the parent of this element.
     * @param parent The element to set as the parent.
      * @return No return value.
     */
    public void setParent(Element parent)
    {
        this.parent = parent;
        doc = null; // reset cached doc pointer.
    }

    /**
     * Returns an enumeration of the children of this element.
     * <code>Enumeration.nextElement</code> returns <code>Element</code> objects.
     * 
     * @return an enumeration of child objects. It must not return null. 
     * See <code>EnumWrapper<code> for an easy way to return an empty enumeration.
     */
    public Enumeration getElements()
    {
        if (children == null) return EnumWrapper.emptyEnumeration;
        return children.elements();
    }

    /**
     * Returns an element collection of the children of this element.
     * This method must not return null. 
     * See <code>EnumWrapper</code> for an empty enumeration. 
     
     * @return element collection of child objects. 
     
     */
    public ElementCollection getChildren()
    {
        return new ElementCollection(this);
    }

    /**
     * Retrieves the number of attributes.
     * @return the number of attributes.
     */
    public int numAttributes()
    {
        return (attrlist == null) ? 0 : attrlist.size();
    }

    /**
     * Retrieves the number of child elements.
     * @return the number of child elements.
     */
    public int numElements()
    {
        return (children == null) ? 0 : children.size();
    }

    /**
     * Adds a child to this element. Any element can only
     * have one parent element, and so the previous parent will lose this child
     * from its subtree.  
     
     * @param elem  The element to add.      
     * The child element becomes the last element if <i>after</i> is null.
     * The child is added to the beginning of the list if <i>after</i> is this object.
     * @param after  The element after which to add it.
      * @return No return value.
     */
    public void addChild(Element elem, Element after)
    {
        if (children == null)
        {
            children = new Vector(4);
        }
        if (after == null)
        {
            children.addElement(elem);
        }
        else if (after == this)
        {
            children.insertElementAt(elem, 0);
        }
        else
        {
            children.insertElementAt(elem, children.indexOf(after) + 1);
        }
        elem.setParent(this);
    }

    /**
     * Adds a child to this element. 
     * @param elem The element to add.
     * @param pos  The position to add this element (calling <code>getChild(pos)</code> 
     * will return this element). If <i>pos</i> is less than 0, <i>elem</i> becomes 
     * the new last element.
     * @param reserved The reserved parameter.
      * @return No return value.
     */
    public void addChild(Element elem, int pos, int reserved)
    {
        if (pos == 0)
            addChild(elem, this);
        else if (pos < 0 || pos > numElements()-1)
            addChild(elem, null);
        else
            addChild(elem, getChild(pos-1));
    }

     /**
      * Retrieves the child element by index.
      * @param index The index of the child element.
      * @return null if there is no child by that index.
      */   
    public Element getChild(int index)
    {
        if (children != null && index >= 0 && index < children.size()) 
        {
            try {
                return (Element)children.elementAt(index);
            } catch (Exception e) {
            }
        }
        return null;
    }

     /**
      * Removes a child element from the tree.
      * @return No return value.
      * @param elem  The element to remove.
      */   
    public void removeChild(Element elem)
    {
        if (children != null)
        {
            elem.setParent(null);
            children.removeElement(elem);
        }
    }
    
    /**
     * Retrieves an enumeration for the element attributes.
     * 
     * The enumeration returns <code>Attribute</code> objects.
     * @return the enumeration. 
     * This method must not return null (see <code>EnumWrapper</code>
     * for returning empty enumerations).
     * @see Attribute
     */
    public Enumeration getAttributes()
    {
        if (attrlist == null) return EnumWrapper.emptyEnumeration;
        return attrlist.attributes();
    }

    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute; returns 
     * null if the attribute is not found.
     */    
    public Object getAttribute(String name)
    {
        return getAttribute(qualifyName(name));
    }

    /**
     * Retrieves an attribute's value given its name.
     * @param name The name of the attribute.
     * @return the value of the attribute; returns 
     * null if the attribute is not found.
     */    
    public Object getAttribute(Name attrName)
    {
        Object obj = null;
        if (attrlist != null)
        {
            obj = attrlist.get(attrName);
        }
        return obj;
    }
    
    /**
     * Sets the attribute of this element.    
     *
     * @param name  The attribute name.
     * @param value The attribute value.
     * @return any previous value, if any.
     */    
    public void setAttribute(String name, Object value)
    {
        setAttribute(qualifyName(name), value);
    }

    /**
     * Deletes an attribute from an element.
     * @param name The attribute to delete.
      * @return No return value.
     */    
    public void removeAttribute(String name)
    {
        removeAttribute(qualifyName(name));
    }
    
    /**
     * Deletes an attribute from an element.
     * @param attrName The attribute name to delete. 
     */    
    public void removeAttribute(Name attrName)
    {
        if (attrlist != null)
        {
            attrlist.remove(attrName);
        }
    }
    
    /**
     * Retrieves the string representation of this element.
     * @return a string representing the element.
     */
    public String toString()
    {
        return getClass().getName() + "[tag=" + tag + 
            ",type=" + type + 
            ",text=" + text + "]";
    }

    /**
     * Sets the attribute of this element.    
     *
     * @param attrName The attribute name. 
     * @param value The attribute value.
     */    
    public void setAttribute(Name attrName, Object value)
    {
        if (attrlist == null)
        {
            attrlist = new Attributes();
        }
        attrlist.put(attrName, value);
    }
    
    /**
     * Removes child nodes and attributes.
     */
    void clear()
    {
        if (children != null)
            children.removeAllElements();
        text = null;
        parent = null;
        if (attrlist != null)
            attrlist.removeAll(); 
    }

    /**
     * Save the element attributes in XML format
     */
    public void saveAttributes(Atom ns, XMLOutputStream o) throws IOException
    {
        for (Enumeration en = getAttributes(); en.hasMoreElements(); ) 
        {
            Attribute a = (Attribute)en.nextElement();
            Name n = a.getName();
            o.write(' ');
            if (n == nameXMLSPACE)
                o.writeChars(n.getName());
            else
                o.writeQualifiedName(n, ns);
            o.write('=');
            boolean qualified = isAttributeQualified(n,o.dtd);
            Object attr = a.getValue();
            if (attr instanceof Name)
            {
                o.write('\"');
                if (qualified)
                    o.writeQualifiedName((Name)attr, ns);
                else
                    o.writeChars(attr.toString());
                o.write('\"');
            }
            else if (attr instanceof Vector)
            {
                Vector v = (Vector)attr;
                o.write('\"');
                for (Enumeration av = v.elements(); av.hasMoreElements();)
                {
                    Name avn = (Name)av.nextElement();                    
                    if (qualified)
                        o.writeQualifiedName(avn, ns);
                    else
                        o.writeChars(avn.toString());
                    if (av.hasMoreElements())
                        o.write(' ');    
                }
                o.write('\"');
            }
            else
                o.writeQuotedString(attr.toString());
        }
    }

    /**
    * Determines if the attribute is qualified. 
     * @param attr The name of the attribute.
     * @param dtd The Document Definition Type (DTD) in which the attribute 
     * is found.
     * @return true if the attribute is qualified; otherwise, returns false.
      * 
     */
    public boolean isAttributeQualified(Name attr, DTD dtd)
    {
        if (dtd != null) {
            ElementDecl ed = dtd.findElementDecl(tag);
            if (ed != null) {
                AttDef ad = ed.findAttDef(attr);
                if (ad != null) {
                    int t = ad.getType();
                    if (t == AttDef.ENTITY ||
                        t == AttDef.ENTITIES ||
                        t == AttDef.NOTATION)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Returns the XML-DATA specification for the DTD element.
     * (Only applies to ElementDecl and Entity nodes).
     * This is DTD information represented in an XML Object Model.
     * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">Specification for XML-Data</a> for details.
     */
    public Element toSchema()
    {
        return this;
    }


    /**
     * Writes tag name
     */
    void writeTagName(XMLOutputStream o) throws IOException
    {
        Element parent = getParent();
        Name tagName = getTagName();
        if (parent != null && parent.getTagName() != null && parent.getTagName().getNameSpace() != null) {
            o.writeQualifiedName(tagName, parent.getTagName().getNameSpace());
        } else if (tagName.getNameSpace() != null) {
            o.writeQualifiedName(tagName, Atom.create(""));
        } else {
            o.writeChars(tagName.getName());
        }
    }

    /**
     * Saves the element in XML format.
     * @param o The output stream to save to.
     * @exception IOException if an error occurs when writing to the output 
     * stream.
     * @return No return value.
     */
    public void save(XMLOutputStream o) throws IOException
    {
        if (getType() != WHITESPACE)
            o.writeIndent();
        int type = getType();
        switch (type)
        {
            case Element.WHITESPACE:
                if (o.getOutputStyle() == XMLOutputStream.DEFAULT)
                    o.writeChars(getText());
                break;
            case Element.ELEMENT:
            case Element.ENTITY: // in schema format
                {
                    int oldStyle = -1;
                    boolean wasmixed = o.mixed;
                    if (tag != null) 
                    {
                        o.writeChars("<");
                        // Write out the tag name
                        writeTagName(o);
                        // Write out the attributes.
                        saveAttributes(getTagName().getNameSpace(), o);
                        // Find out if we're preserving white space.
                        Object v = getAttribute(nameXMLSPACE);
                        String value = null;
                        if (v != null) value = v.toString();
                        if (value != null) 
                        {
                            oldStyle = o.getOutputStyle();
                            boolean preserveWS = value.equalsIgnoreCase("preserve");
                            o.setOutputStyle(preserveWS ? XMLOutputStream.COMPACT : 
                                XMLOutputStream.PRETTY);
                        }    
                        // Write the close start tag.
                        if (numElements() > 0)
                        {
                            o.write('>');                            
                        }
                        else
                        {
                            o.writeChars("/>");
                        }                                                
                    }
                    // find out if this is a mixed element.
                    ElementEnumeration ee = null;
                    o.mixed = false; // not inherited down heirarchy.
                    for (ee = new ElementEnumeration(this); ee.hasMoreElements(); )
                    {
                        Element ne = (Element)ee.nextElement();
                        if (ne.getType() == PCDATA || ne.getType() == ENTITYREF)
                        {
                            o.mixed = true;
                            break;
                        }
                    }
                    o.writeNewLine();
                    o.addIndent(1);
                    o.nameSpaceContext.push();
                    for (ee = new ElementEnumeration(this); ee.hasMoreElements(); )
                    {
                        Element ne = (Element)ee.nextElement();
                        ne.save(o);
                    }
                    o.nameSpaceContext.pop();
                    o.addIndent(-1);
                    if (tag != null && numElements() > 0) 
                    {                        
                        o.writeIndent();
                        o.writeChars("</");
                        writeTagName(o);
                        o.writeChars(">");
                        if (oldStyle != -1) 
                        {
                            o.setOutputStyle(oldStyle);
                        }
                        o.mixed = wasmixed;
                        o.writeNewLine();
                    }
                }
                break;
            case Element.PI:
                o.writeChars("<?");
                o.writeQualifiedName(getTagName(), null);
                saveAttributes(getTagName().getNameSpace(), o);
                o.writeChars(getText() + "?>");
                o.writeNewLine();
                break;
            case Element.CDATA:
                o.writeChars("<![CDATA[" + getText() + "]]>");
                o.writeNewLine();
                break;
            case Element.PCDATA:
                o.writeChars(getText());
                o.writeNewLine();
                break;
            case Element.ENTITYREF:
                if (o.savingDTD) 
                {
                    o.write('%');
                }
                else
                {
                    o.write('&');
                }
                o.writeQualifiedName(getTagName(), getParent().getTagName().getNameSpace());
                o.writeChars(";");
                if (o.savingDTD)
                    o.writeNewLine();
                break;
            case Element.COMMENT:
                o.writeChars("<!--" + getText() + "-->");
                o.writeNewLine();
                break;
            case Element.NAMESPACE:
                o.writeChars("<?xml:namespace ");
                saveAttributes(nameXML, o);
                o.writeChars("?>");
                o.writeNewLine();
                // record name space context
                Atom as, ns;
                
                as = Atom.create(getAttribute(nameXMLAS).toString());
                ns = Atom.create(getAttribute(nameXMLNS).toString());
                o.nameSpaceContext.addNameSpace(ns, as);
                break;
            case Element.INCLUDESECTION:
                if (tag == null)
                {
                    o.writeChars("<![INCLUDE[");
                }
                else
                {
                    o.writeChars("<![%");
                    o.writeChars(tag.toString());
                    o.writeChars(";[");
                }
                for (ElementEnumeration ee = new ElementEnumeration(this); ee.hasMoreElements(); )
                {
                    Element e = (Element)ee.nextElement();
                    e.save(o);
                }
                o.writeChars("]]>");
                break;
            case Element.IGNORESECTION:
                if (tag == null)
                {
                    o.writeChars("<![IGNORE[");
                }
                else
                {
                    o.writeChars("<![%");
                    o.writeChars(tag.toString());
                    o.writeChars(";[");
                }
                for (ElementEnumeration ee = new ElementEnumeration(this); ee.hasMoreElements(); )
                {
                    Element e = (Element)ee.nextElement();
                    e.save(o);
                }
                break;
        }
        o.flush();
    }

    /**
     * This is a useful method for creating a qualified Name object
     * from the given string.  This will parse the string and lookup
     * the namespace, if specified, and create the fully qualified
     * name.  If no namespace is given, it will inherit the namespace
     * from this element.
     */
    public Name qualifyName(String string)
    {
        int pos = string.indexOf(":");
        if (pos != -1) {
            int len = 1;
            // support for old "::" namespace syntax.
            if (pos+1 < string.length() && string.charAt(pos+1) == ':')
                len++;
            String sn = string.substring(0,pos);
            Atom namespace = getNamespace(sn);
            String name = string.substring(pos+len);
            return Name.create(name, namespace);
        } else if (tag != null) {
            return Name.create(string, tag.getNameSpace());
        }
        return Name.create(string);
    }

    Atom getNamespace(String str)
    {   
        Atom name = Atom.create(str);
        Document d = getDocument();
        if (d != null) 
        {
            DTD dtd = d.getDTD();
            if (dtd != null)
            {
                Atom namespace = dtd.findLongNameSpace(name);
                if (namespace != null)
                    return namespace;
            }
        }
        return name;
    }

    Document getDocument()
    {
        if (doc == null) 
        {
            Element e = this;
            while (e.getParent() != null) 
            {
                e = e.getParent();
            }
            if (e instanceof Document) 
            {
                doc = (Document)e;
            }
        }
        return doc;
    }

    /**
     * Tag of the element (markup).
     */
    Name tag;

    /**
     * type of element.
     */    
    int type;
    
    /**
     * Text for element.
     */    
    String text;
    
    /**
     * Parent of element.
     */    
    Element parent;    
    
    /**
     * Children element list.
     */    
    Vector children; 

    /**
     * Cached pointer to owning Document
     */
    Document doc;
    
    /**
     * The attribute list.
     * 
     */
    protected Attributes attrlist;

    static Atom nameXML = Atom.create("xml");
    static Name nameXMLSPACE = Name.create("xml-space","xml");
    static Name nameXMLAS = Name.create("prefix", "xml");
    static Name nameXMLHREF = Name.create("src", "xml");
    static Name nameXMLNS = Name.create("ns", "xml");
}
