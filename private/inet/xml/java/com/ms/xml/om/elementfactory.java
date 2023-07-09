/*
 * @(#)ElementFactory.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Name;

/**
 * This interface specifies a method to create elements 
 * for an XML element tree. This is used by the XML Document to create
 * nodes in the tree as it parses the elements. 
 * 
 * @version     1.0, 10 Mar 1997
 * @author      Istvan Cseri
 */
public interface ElementFactory
{

    /**
     * Creates an element with the specified tag for the
     * specified type of element.
     *
     * @param   tag The name of the element.
     * @param   type One of the predefined Element types that
     * can be returned from <code>Element.getType</code>.
     * @return  the created element.
     * @see     Element
     */
	Element createElement(Element parent, int type, Name tag, String text);

    /**
     * Notifies that the element is fully parsed.
     *
     * @param   elem  The element parsed.
     * @see     Element
     */
    void parsed(Element elem);

    /**
     * Notification that an attribute just got parsed.
     * Value may be a String, Name or Vector of Names.
     */
    void parsedAttribute(Element e, Name name, Object value);

}    
