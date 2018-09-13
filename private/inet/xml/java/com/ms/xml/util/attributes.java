/*
 * @(#)Attributes.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
package com.ms.xml.util;

import java.util.Vector;
import java.util.Enumeration;

/**
 * A class that encapsulates a list of attribute/value pairs.
 *
 * @version 1.0, 6/3/97
 */
public class Attributes extends ReadOnlyAttributes
{

    /**
     * Construct empty attributes collection.
     */
	public Attributes()
	{
	}

    /**
     * Construct attributes collection with given number of empty slots.
     * The collection will grow automatically
     * if you add more than this number.
     * @param elems the number of attributes to reserve initially.
     */
	public Attributes(int elems)
	{
        super(elems);
	}

    /**
     * Construct attributes collection by copying the passed attributes.
     *
     * @param   attrs   attributes to clone
     */
    public Attributes(ReadOnlyAttributes attrs)
    {
        Vector v = attrs.attributes;
        if (v != null)
        {
            int l = v.size();
            attributes = new Vector(l);
            attributes.setSize(l);
            for (int i = 0; i < l; i++)
            {
                Attribute a = (Attribute)v.elementAt(i);
                attributes.setElementAt(new Attribute(a.name, a.getValue()), i);
            }
        }
    }

    /**
    * Construct attributes collection using the vector of attributes.
    *
    * @param   v   Attribute vector
    */
    public Attributes(Vector v)
    {
        super(v);
    }

    /**
     * Removed the named attribute from the collection.
     */
    public void remove(Name name)
	{
        Attribute a = lookup(name);
        if (a != null) 
        {
            attributes.removeElement(a);
        }
	}

    /**
     * Add a new attribute/value pair, or replace the value for
     * attribute if it already exists.
     * @return the previous value for the name attribute or null.
     */
    public Object put(Name name, Object value) 
    {
        Attribute a = lookup(name);
        Object o = null;
        if (a != null) 
        {
            o = a.getValue();
            a.setValue(value);
        } 
        else 
        {
            attributes.addElement(new Attribute(name, value));
        }
        return o;
    }

    /**
     * Add a new attribute or replace the attribute
     * if it already exists.
     * @return the previous value for the name attribute or null.
     */
    public Object put(Attribute v) 
    {
        Object o = null;
        Attribute a = lookup(v.getName());
        if (a != null) 
        {
            o = a.getValue();
            attributes.removeElement(a);
        }
        attributes.addElement(v);
        return o;
    }

    public void removeAll()
    {
        attributes.removeAllElements();
    }
}    

