/*
 * @(#)ReadOnlyAttributes.java 1.0 97/5/6
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.util.Vector;
import java.util.Enumeration;


/**
 * Attributes wrapper class which provides read only access.
 *
 * @author  Istvan Cseri
 * @version 1.0, 5/6/97
 */
public class ReadOnlyAttributes
{
    /**
     * Collection of Attribute objects.
     */
    Vector attributes;

    /**
     * Construct empty attributes collection.
     */
	public ReadOnlyAttributes()
	{
        attributes = new Vector();
	}

    /**
    * Construct attributes collection using the vector of attributes.
    *
    * @param   v   Attribute vector
    */
    public ReadOnlyAttributes(Vector v)
    {
        attributes = v;
    }

    /**
     * Construct attributes collection with given number of empty slots.
     * The collection will grow automatically
     * if you add more than this number.
     * @param elems the number of attributes to reserve initially.
     */
	public ReadOnlyAttributes(int elems)
	{
        attributes = new Vector(elems);
	}

    /**
     * Return the number of attribute/value pairs in the collection.
     */
    public int size()
    {
        return attributes.size();
    }

    /**
     * Find the named attribute and return the associated value.
     */
    public Object get(Name name)
    {
        Attribute a = lookup(name);
        if (a == null) 
        {
            return null;
        }
        return a.getValue();
    }

    /**
     * Return an Enumeration for iterating through the attributes.
     */
    public Enumeration attributes()
    {
        return attributes.elements();
    }

     
    public Attribute lookup(Name name)
    {
        for (Enumeration e = attributes.elements(); e.hasMoreElements(); ) 
        {
            Attribute a = (Attribute)e.nextElement();
            if (a.name == name) 
            {
                return a;
            }
        }
        return null;
    }

    public String toString()
    {
        return getClass().getName();
	}
}    
