/*
 * @(#)Atom.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.lang.String;
import java.util.Hashtable;

/**
 * This is a general purpose object to allow efficient
 * sharing of duplicate strings in the system.  It does this by
 * creating a global HashTable of all Atoms that have been 
 * constructed.
 *
 * @version 1.0, 6/3/97
 */
public class Atom
{
    /**
     * The shared string
     */
    String s;
    
    /**
     * Cached hash value for improved compare speed.
     */    
    int hash;
    
    /**
     * Hash table for shared atoms.
     */
    static Hashtable atoms = new Hashtable(500);

    /**
     * Creates Atom new object with the passed in collation key.
     * This is private because all Atoms should be constructed
     * via the static Atom.create() method.
     */
    Atom(String s, int h) 
    {
        this.s = s;
        this.hash = h;
    }

	/**
	 * private constructor
	 */
	Atom()
	{
	}

    /**
     * Create a Atom object for this string.
     * Atoms are case sensitive - i.e. it assumes any case folding
     * has already been done.
     */    
    public static Atom create(String s)
    {
        if (s == null) return null;

        Object o = atoms.get(s);
        if (o == null)
        {
            Atom n = new Atom(s, s.hashCode());
            atoms.put(s, n);
            return n;
        }
        else
        {
            return (Atom)o;
        }
    }
     
	/**
     * Return the hash code for the name.
     * @return returns the hash code for the name.
     */
    public int hashCode()
    {
        return hash;
    }
 
    /**
     * Return the string represented by the Atom.
     */
    public String toString() 
    {
        return s;
    }
    
    /**
     * Return whether this Atom is equal to another given Atom.
     */
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
        return s.equals(((Atom)that).s);
    }
}    