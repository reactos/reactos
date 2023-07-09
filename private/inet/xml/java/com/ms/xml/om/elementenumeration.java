/*
 * @(#)ElementEnumeration.java 1.0 7/11/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Name;
import java.util.Enumeration;

/**
 * This class is a simple Enumeration for iterating over the immediate children
 * of a given node in the XML tree. This is not a hierarchical
 * iterator. It does not walk the entire tree.
 *
 * @version 1.0, 7/11/97
 * @see Element
 * @see Name
 */
public class ElementEnumeration implements Enumeration
{
    /**
     * Creates a new enumerator for enumerating over all of the children
     * of the given root node.
     * @param root The element whose children are going to be enumerated. 
     */
    public ElementEnumeration(Element root)
    {
        this.root = root;
        this.tag = null;
        this.next = null;
        this.children = (root != null)  ? root.getElements() : null;
        this.type =  -1;
    }

    /**
     * Creates a new enumerator for enumerating over the immediate children
     * of the given root node that have matching tag names and/or types.
     * @param root The element whose children are going to be enumerated. 
     
     * @param tag The name of the tag; this parameter can be null if the name is not important. 
     
     * @param type The element type. <code>Element.ELEMENT</code> is the most common. If the
     * element type is not important, pass -1.
     */
    public ElementEnumeration(Element root, Name tag, int type)
    {
        this.root = root;
        this.tag = tag;
        this.next = null;
        this.children = (root != null) ? root.getElements() : null;
        this.type = type;
    }

    /**
     * Determines if there are any more matching elements.
     * @return true if the next call to <code>nextElement</code> will return
     * a non-null result.
     */
    public boolean hasMoreElements()
    {
        if (next == null) 
        {
            next = getNext();
        }
        return (next != null) ? true : false;
    }

    /**
     * Retrieves the next matching element.
     * @return <code>Element</code> or null if there are no more matching elements.
     */
    public Object nextElement()
    {
        if (next != null) 
        {
            Element result = next;
            next = null;
            return result;
        }
        return getNext();
    }

    /**
     * Resets the iterator so that you can iterate through the elements again.
      * @return No return value.
     */
    public void reset()
    {
        children = (root != null) ? root.getElements() : null;
        next = null;
    }

    /**
     * Internal method for getting next element.
     */
    Element getNext()
    {
        if (children != null) 
        {
            while (children.hasMoreElements()) 
            {
                Element e = (Element)children.nextElement();
                if (tag == null && type == -1) 
                    return e;
                if (type != -1 && e.getType() != type)
                    continue;
                if (tag != null && e.getTagName() != tag)
                    continue;
                return e;
            }
        }
        return null;
    }

    Element root;
    Enumeration children;
    Name tag;
    Element next;
    int type;
}
