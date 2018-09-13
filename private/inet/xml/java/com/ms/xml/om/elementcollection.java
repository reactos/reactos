/*
 * @(#)ElementCollection.java 1.0 8/7/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.util.Name;
import com.ms.xml.om.ElementCollection;

/**
 * This class provides a collection interface to elements
 * similar to the element collections found in the Internet Explorer 4.0
 * Dynamic HTML object model.
 *
 * @version 1.0, 8/7/97
 */
public class ElementCollection
{
    /**
     * Creates new collection object for given element.
     */
    public ElementCollection(Element root)
    {
        this.root = root;
        this.items = new ElementEnumeration(root,null,Element.ELEMENT);
        currentindex = 0; // not initialized
        current = (Element)items.nextElement();
        length = -1;
    }

    /**
     * Creates a new collection for iterating over the immediate children
     * of the given root node that have matching tag names and/or
     * element types.   
     * @param root The root to form the collection around.
     * @param tag The name of the tag; this parameter can be null if the name is not important. 
     * @param type The element type. <code>Element.ELEMENT</code> is the most
      * common. If
     * the element type is not important, pass -1.
     */
    public ElementCollection(Element root, Name tag, int type)
    {
        this.root = root;
        this.items = new ElementEnumeration(root, tag, type);
        length = -1; // not initialized
        currentindex = 0; // not initialized
        current = (Element)items.nextElement();
    }

    /**
     * Retrieves the number of items in the collection.
     * @return the item count.
     */
    public int getLength()
    {
        if (length == -1) 
        {
            items.reset();
            length = 0;
            while (items.hasMoreElements()) 
            {
                items.nextElement();
                length++;
            }
            items.reset();
            currentindex = 0; // not initialized
            current = (Element)items.nextElement();
        }
        return length;
    }

    /**
     * Retrieves a named item or a collection of matching items.
     * @param name The name of the item or collection of matching items.
     * @return the requested item. Possible types of objects 
     * returned are <code>Element</code>, <code>ElementCollection</code>, 
     * or null.
     */
    public Object item(String name)
    {
        try {
            int i = Integer.parseInt(name);
            return getChild(i);
        } catch (Exception e) {
        }

        ElementCollection col = new ElementCollection(root,Name.create(name),Element.ELEMENT);
        if (col.getLength() == 1) 
        {
            // only one match, so return it.
            return col.getChild(0);
        } 
        // otherwise return the new collection.
        return col;
    }

    /**
     * Retrieves a specified item from the collection by index.
     * @param index  The index of the item in the element collection.
     * @return the requested child node; returns null if not found.
     */
    public Element getChild(int index)
    {
        // This is an optimization to help sequential
        // calls to item with sequentially increasing
        // indexes be linear in performance.
        if (currentindex == index) {
            return current;
        } else if (currentindex > index) {
            // have to reset and start over.
            currentindex = 0;
            items.reset();
        } 
        // must not use else here since we change currentindex above.
        if (currentindex < index) { 
            while (currentindex < index && items.hasMoreElements())
            {
                current = (Element)items.nextElement();
                currentindex++;
            }
        }
        if (currentindex != index) {
            // Still didn't find it.  Mustn't be there.
            return null;
        }
        return current;
    }

    /**
     * Retrieves a specified item from the collection of matching items.
     * @param name  The name of the matching items.
     * @param index  The index of the specific matching item to return.
     * @return the requested item if it is found; returns null if it is not 
      * found.
     */
    public Element item(String name, int index)
    {
        ElementCollection col = new ElementCollection(root,Name.create(name),Element.ELEMENT);
        return col.getChild(index);
    }

    Element root;
    ElementEnumeration items;
    int length;
    Element current;
    int currentindex;
}
