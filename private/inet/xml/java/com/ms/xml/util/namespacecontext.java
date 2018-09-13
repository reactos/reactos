/*
 * @(#)XMLOutputStream.java 1.0 6/10/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */ 
package com.ms.xml.util;

import java.util.Hashtable;
import java.util.Stack;

public class NameSpaceContext
{
	/**
	 *	add name space. long name is the key
	 */
	public final void addNameSpace(Atom url, Atom n)
	{
		current.put(url, n);
	}

    /**
	 * find name space, long name is the key
	 */
	public final Atom findNameSpace(Atom n)
	{
		return (Atom)current.get(n);
	}

    
    public final void push()
    {
        contexts.push(current);
        current = (Hashtable)current.clone();
    }
    
    public final void pop()
    {
        current = (Hashtable)contexts.pop();
    }

	Hashtable current = new Hashtable();

    /**
     * Stack to keep track of contexts
     */
    Stack contexts = new Stack();
}