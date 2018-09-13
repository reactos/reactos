/*
 * @(#)Attribute.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
package com.ms.xml.util;

/**
 * This class encapsulates an attribute name and value pair.
 *
 * @version 1.0, 6/3/97
 */
public class Attribute
{
    Name    name;
    Object  value;

    /**
     * Construct empty attribute.
     */
	public Attribute()
	{
	}

    /**
     * Construct attribute with given name and value.
     */
	public Attribute(Name n, Object v)
	{
        name = n;
        value = v;
	}

    /**
     * Return the name.
     */
    public Name getName()
    {
        return name;
    }

    /**
     * Return the value.
     */
    public Object getValue()
    {
		return value;
	}

    /**
     * Used by Attributes, private to this package
     */
    void setValue(Object o)
    {
        value = o;
    }
}    


