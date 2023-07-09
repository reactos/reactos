/*
 * @(#)ElementFactoryImpl.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Name;

/**
 * This class represents the default implementation for the element factory. 
 * This is 
 * what the XML parser uses if no other factory is specified. 
  *
 * @version 1.0, 6/3/97
 */
public class ElementFactoryImpl implements ElementFactory
{
    /**
     * Constructs the factory object.
     */
    public ElementFactoryImpl()
    {
    }


    /**
     * Retrieves a new element for the specified tag and type.
     * @param type The element type.
     * @param tag The element name.
     * @return the newly created element. 
     */
    public Element createElement(Element parent, int type, Name tag, String text)
    {
        Element e = new ElementImpl(tag, type);
        if (text != null) 
            e.setText(text);
        if (parent != null)
            parent.addChild(e,null);
        return e; 
    }

    /**
     * This method is called when the element is fully parsed.
     *
     * @param   elem  The element parsed.
     * @see     Element
      * @return No return value.
     */
    public void parsed(Element elem) 
    {
    }

    /**
     * this method is called when a new attribute is parsed for
     * the given element.
     */
    public void parsedAttribute(Element e, Name name, Object value)
    {
        if (e != null)
            e.setAttribute(name,value);
    }


}    

