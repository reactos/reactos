/*
 * @(#)ElementDeclEnumeration.java 1.0 8/25/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.Element;
import com.ms.xml.util.Name;
import java.util.Enumeration;


/**
 * A simple Enumeration for iterating over Element declarations.
 * Returns the XML-DATA specification for each element.
 * This is DTD information represented in an XML Object Model.
 * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">Specification for XML-Data</a> for details.
 *
 * @version 1.0, 8/25/97
 * @see ElementDecl
 * @see Name
 */
public class ElementDeclEnumeration implements Enumeration
{
    /**
     * Creates new enumerator for enumerating over all of the children
     * of the given root node.
     */
    public ElementDeclEnumeration(Enumeration elemDecls)
    {
        this.elemDecls = elemDecls;
    }

    /**
     * Creates new enumerator for enumerating over all of the children
     * of the given root node.
     */
    public ElementDeclEnumeration(Element e)
    {
        this.elemDecls = (e != null) ? e.getElements() : null;
    }

    /**
     * Return whether or not there are any more matching elements.
     * @return true if the next call to nextElement will return
     * non null result.
     */
    public boolean hasMoreElements()
    {
        return elemDecls.hasMoreElements();
    }

    /**
     * Return the next matching element.
     * @return Element or null of there are no more matching elements.
     */
    public Object nextElement()
    {
        Element elemDecl = (Element)elemDecls.nextElement();
        
        return elemDecl.toSchema();
    }

    Enumeration elemDecls;
}
