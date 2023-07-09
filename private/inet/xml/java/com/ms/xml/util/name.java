/*
 * @(#)Name.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.io.*;
import java.util.Hashtable;

/**
 * This is a general purpose name object to allow efficient
 * sharing of duplicate names in the system.  It does this by
 * creating a global HashTable of all names that have been 
 * constructed.  Names are different from Atoms in that they can
 * be qualified by a separate namespace string - so in effect
 * that are compound atoms.
 *
 * @version 1.0, 6/3/97
 */
public class Name
{

    /**
     * Hash table for shared qualified names.
     */
    static Hashtable names = new Hashtable(500);

        /**
     * Hash table for shared simple names.
     */
    static StringHashtable snames = new StringHashtable(500);


    /**
     * A name is a compound object containing two atoms.
     */
	Atom nameSpace;
	String name;
    int  hash;

    /**
     * The constructor is private because all names should
     * be constructed using the static Name.create() methods
     */
	Name(String name, Atom nameSpace, int hash)
	{	
        this.name = name;
		this.nameSpace = nameSpace;
        this.hash = hash;
	}   

	public String getName()
	{
		return name;
	}

	public Atom getNameSpace()
	{
		return nameSpace;
	}

    public boolean equals(Object that) 
    {
        if (this == that) 
        {
            return true;
        }
        if (that == null || getClass() != that.getClass()) 
        {
            return false;
        }

		Name t = (Name)that;
		if (nameSpace != null) {
			if (!nameSpace.equals(t.nameSpace))
				return false;
		}
		else if (t.nameSpace != null)
			return false;
		return this.name.equals(t.name);        
    }
    
    /**
     * Create an unqualified Name.
     */    
    public static Name create(String name)
    {
        if (name == null) return null;
        Object o = snames.get(name);
        if (o == null)
        {
            // add new name to simple names hash table.
            int h = name.hashCode();
            Name result = new Name(name, null, h);
            snames.put(name, result);
            return result;
        }
        // return existing name and drop 'result' on the floor.
        return (Name)o;
    }

    public static Name create(char val[], int offset, int len)
    {
        Object o = snames.get(val, offset, len);
        if (o == null)
        {
            // add new name to simple names hash table.
			String name = new String(val, offset, len);
            int h = name.hashCode();
            Name result = new Name(name, null, h);
            snames.put(name, result);
            return result;
        }
        // return existing name and drop 'result' on the floor.
        return (Name)o;
    }

    /**
     * Create a Name object for the given name and namespace.
     * The strings are case sensitive.
     */    
    public static Name create(String name, String ns)
    {
        if (name == null) return null;
        if (ns == null) return create(name);        
        return create(name,Atom.create(ns));
    }

    /**
     * Create a Name object for the given name and namespace
     * where the name and Namespace are already Atoms.
     * The strings are case sensitive.
     */    
    public static Name create(String name, Atom nameSpace)
    {
        if (name == null) return null;
        if (nameSpace == null) return create(name);
        
        int h = name.hashCode() + nameSpace.hashCode();
        Name result = new Name(name, nameSpace, h);
        Object o = names.get(result);
        if (o == null)
        {
            // add new name to hash table.
            names.put(result, result);
            return result;
        }
        else
        {
            // return existing name and drop 'result' on the floor.
            return (Name)o;
        }
    }

 
	public String toString()
	{
		if (nameSpace != null)
			return nameSpace.toString() + name;
		else return name;
	}

    /**
     * return the hash code for this name object
     */
    public int hashCode()
    {
        return hash;
    }

}
