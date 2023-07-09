/*
 * @(#)Context.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import java.util.Hashtable;
import com.ms.xml.om.Element;
import com.ms.xml.util.Atom;
import com.ms.xml.util.Name;

/**
 * The class defines the context for an element currently being parsed
 *
 * @version 1.0, 6/3/97
 */
class Context
{
    /**
     * current element - which may be null
     */
    Element e;
    
    /**
     * current tag name
     */
    Name tagName;

    /**
     * current element type
     */
    int type;

    /**
     * current element declaration
     */
    ElementDecl ed;    

    /**
     * a linked Context for proper validation of entities
     * (see comment in Parser.addNewElement).
     */
    Context parent;

    /**
     * state used in DFA to validate content
     */
    int state;
    
    /**
     * set if content model pattern matched
     */   
    boolean matched;

    /**
     * whether to preserve white space
     */
    boolean preserveWS;
    boolean lastWasWS;

	/**
	 *	name space
	 */
	Atom nameSpace;

	/**
	 *	default name space 
	 */
	Atom defaultNameSpace;

	/**
	 *	Name space storage, index from long name to short name
	 */
	Hashtable spaceTable;

    Context(Element e, Name name, int type, boolean preserveWS, Atom nameSpace, Hashtable spaceTable)
    {
        this.e = e;
        this.tagName = name;
        this.type = type;
        this.preserveWS = preserveWS;
		this.nameSpace = nameSpace;
		this.defaultNameSpace = nameSpace;
        this.lastWasWS = false;
		if (spaceTable != null)
			this.spaceTable =(Hashtable)spaceTable.clone();
    }

    void reset(Element e, Name name, int type, boolean preserveWS, Atom nameSpace, Hashtable spaceTable)
    {
		this.ed = null;    
        this.tagName = name;
        this.parent = null;
        this.type = type;
		this.state = 0;
		this.e = e;
		this.preserveWS = preserveWS;
		this.nameSpace = nameSpace;
		this.defaultNameSpace = nameSpace;
		this.lastWasWS = false;
		if (spaceTable != null)
			this.spaceTable =(Hashtable)spaceTable.clone();
		else
			spaceTable = null;
    }

	/**
	 *	add name space. short name is the key
	 */
	final void addNameSpace(Atom n, Atom url)
	{
		if (spaceTable == null)
		{
			spaceTable = new Hashtable();
		}
		spaceTable.put(n, url);
	}

    /**
	 * find name space, short name is the key
	 */
	final Atom findNameSpace(Atom n)
	{
		if (n == null) return null;
		return spaceTable == null ? null : (Atom)spaceTable.get(n);
	}

 }
