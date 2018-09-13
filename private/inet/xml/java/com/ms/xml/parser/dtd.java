/*
 * @(#)Document.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.Element;
import com.ms.xml.om.ElementImpl;
import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.util.EnumWrapper;

import java.util.Vector;
import java.lang.String;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;


/**
 * This class contains all the Document Type Definition (DTD) 
 * information for an XML document.
 *
 * @version 1.0, 6/17/97
 */
public class DTD
{
    /**
     * Creates a new empty DTD.
     */
    public DTD()
    {        
    }

    /** 
     * Finds a named entity in the DTD.
	 * @param n  The name of the entity.
	 * @return  the specified <code>Entity</code> object; returns null if it 
      * is not found. 
     */
    final void addEntity(Entity en)
    {        
        if (entities == null)
        {
            entities = new Hashtable();
        }        
        entities.put(en.name, en);
    }
    
    public final Entity findEntity(Name n)
    {
        // First see if fully qualified entity is defined in DTD
        Object o = null;
        if (entities != null) {
            o = (Entity)entities.get(n);
        }
        // Then see if the entity minus the namespace matches 
        // a built in entity.  This allows &amp; to be used in
        // the scope of another namespace, otherwise the user
        // would have to use &::amp; which is wierd.
        if (o == null) {
            o = builtin.get(n.getName());
        }
        if (o != null)
        {
            return (Entity)o;
        }
		return null;
    }

    final void addElementDecl(ElementDecl ed)
    {        
        if (elementdecls == null)
        {
            elementdecls = new Hashtable();
        }
        elementdecls.put(ed.name, ed);
    }
    
    /** 
     * Finds an element declaration for the given tag name.
	 * @param name  The tag name.
	 * @return  the element declaration object. 
     */
    public final ElementDecl findElementDecl(Name name)
    {
        return elementdecls == null ? null : (ElementDecl)elementdecls.get(name);
    }

    /**
     * Retrieves an object for enumerating the element declarations.
     * 
     * @return  an <code>Enumeration</code> object that returns 
     * <code>ElementDecl</code> objects when enumerated. 
     */
	public final Enumeration elementDeclarations()
	{
        return elementdecls == null ? EnumWrapper.emptyEnumeration : elementdecls.elements();
	}

    /**
     * Return an enumeration for enumerating the entities
     * The enumeration returns entity objects.
     */
	public final Enumeration entityDeclarations()
	{
        return entities == null ? EnumWrapper.emptyEnumeration : entities.elements();
	}

    /**
     * Return an enumeration for enumerating the notations
     * The enumeration returns notation objects.
     */
	public final Enumeration notationDeclarations()
	{
        return notations == null ? EnumWrapper.emptyEnumeration : notations.elements();
	}

    final void addNotation(Notation no)
    {        
        if (notations == null)
        {
            notations = new Hashtable();
        }
        notations.put(no.name, no);
    }
    
	/**
     * Retrieves the named XML notation in the DTD.
     *
	 * @param name  The name of the notation.
     * @return the <code>Notation</code> object; returns null if it is not 
      * found.
	 */
    public final Notation findNotation(Name name)
    {
        return notations == null ? null : (Notation)notations.get(name);
    }

	/**
	 *	add a <long name, short name> pair to the name space hashtable 
	 */
	public final void addNameSpace(Atom as, Atom href)
	{
		if (shortNameSpaces == null)
		{
			shortNameSpaces = new Hashtable();
			longNameSpaces = new Hashtable();
		}
		shortNameSpaces.put(href, as);
		longNameSpaces.put(as, href);
	}

	/**
	 * get the short name of a specified name space
	 */
	public final Atom findShortNameSpace(Atom href)
	{
		return shortNameSpaces == null ? null : (Atom)shortNameSpaces.get(href);
	}

	public final Atom findLongNameSpace(Atom as)
	{
		return longNameSpaces == null ? null : (Atom)longNameSpaces.get(as);
	}

	static public boolean isReservedNameSpace(Atom n)
	{
		return (n == nameXML);
	}

    /**
	 *	keep a list of all name spaces loaded so they are loaded twice if 
	 *  two name spaces happen to refer to the same DTD file
	 */
	public final Atom findLoadedNameSpace(Atom url)
	{
		if (url == null) return null;
		return loadedNameSpaces == null ? null : (Atom)loadedNameSpaces.get(url);
	}

	/**
	 * add a loaded name space name to the list
	 */
	public final void addLoadedNameSpace(Atom url)
	{
		if (loadedNameSpaces == null)
		{
			loadedNameSpaces = new Hashtable();
		}
		loadedNameSpaces.put(url, url);
	}
    
	final void addID(Name name, Element e)
    {        
        if (ids == null)
        {
            ids = new Hashtable();
        }
        ids.put(name, e);
    }

    final Element findID(Name name)
    {
        return ids == null ? null : (Element)ids.get(name);
    }

    final void addIDCheck(Name name, int line, int col, Object owner)
    {
        idCheck = new IDCheck(idCheck, name, line, col, owner);
    }
    
    final void checkIDs() throws ParseException
    {
        while(idCheck != null)
        {
    		if (findID(idCheck.name) == null)
            {
                throw new ParseException("Couldn't find element with ID '" + idCheck.name.toString() + "'", 
                    idCheck.line, idCheck.col, idCheck.owner);
            }
            idCheck = idCheck.next;
        }
    }

    /**
     * Convert everything stored in the DTD to schema.  (Does not retain original order)
     */
    final public Element getSchema()
    {
        if( schema == null )
        {
            schema = new ElementImpl( Name.create("SCHEMA","XML"), Element.ELEMENT );
            if( docType != null )
                schema.setAttribute( Name.create("ID","XML"), docType );

            Enumeration e;
            if (longNameSpaces != null)
            {
                e = longNameSpaces.keys();
                for( ; e.hasMoreElements(); )
                {
                    Atom as = (Atom)e.nextElement();
                    Element ns = new ElementImpl( Name.create("NAMESPACE","XML"), Element.PI );
                    ns.setAttribute(Name.create("HREF","XML"),longNameSpaces.get(as));
                    ns.setAttribute(Name.create("AS","XML"),as);
                    schema.addChild( ns, null );
                }
            }

            e = notationDeclarations();
            for( ; e.hasMoreElements(); )
            {
                Element curr = (Element)e.nextElement();
                Element currSchema = curr.toSchema();
                if( currSchema != null )
                    schema.addChild( currSchema, null );
            }

            e = entityDeclarations();
            for( ; e.hasMoreElements(); )
            {
                Element curr = (Element)e.nextElement();
                Element currSchema = curr.toSchema();
                if( currSchema != null )
                    schema.addChild( currSchema, null );
            }

            e = elementDeclarations();
            for( ; e.hasMoreElements(); )
            {
                Element curr = (Element)e.nextElement();
                Element currSchema = curr.toSchema();
                if( currSchema != null )
                    schema.addChild( currSchema, null );
            }
        }

        return schema;
    }

    /**
     * Saves the DTD information to the given XML output stream.
     * @param o  The output stream to write to.
      * @return No return value.
     * @exception  IOException if there are problems writing to the output 
      * stream.
     */
    final public void save(XMLOutputStream o) throws IOException {
        if (notations != null) {
            Enumeration e = notations.elements();
            while (e.hasMoreElements()) {
                Notation n = (Notation)e.nextElement();
                o.writeIndent();
                n.save(o);
            }
        }
        if (entities != null) {
            Enumeration e = entities.elements();
            while (e.hasMoreElements()) {
                Entity en = (Entity)e.nextElement();
                o.writeIndent();
                en.save(o);
            }
        }
        if (elementdecls != null) {
            Enumeration e = elementdecls.elements();
            while (e.hasMoreElements()) {
                ElementDecl decl = (ElementDecl)e.nextElement();
                o.writeIndent();
                decl.save(o);
            }
        }
    }

    /**
     * Resets the DTD to its initial state.
      * @return No return value.
     */
    final public void clear()
    {
        if (ids != null) ids.clear();
        idCheck = null;
    }

    /**
     * Retrieves the name specified in the <code>DOCTYPE</code> tag.
     * @return  the name of the document type.
     */
    final public Name getDocType()
    {
        return docType;
    }

    /**
     * Entity storage
     */    
    static Hashtable builtin;
    Hashtable entities;

    /**
     * ElementDecl storage
     */
    Hashtable elementdecls;

    /**
     * Notation storage
     */    
    Hashtable notations;

	/**
	 *	Name space storage, index from long name to short name
	 */
	Hashtable shortNameSpaces;

	/**
	 *  Name space storage, index from short name to long name
	 */
	Hashtable longNameSpaces;

	/**
	 * a hashtable of loaded name spaces
	 */
	Hashtable loadedNameSpaces;

	/**
	 * ID storage
	 */
	transient Hashtable ids;

    /**
     * IDs to check later for matching elements
     */
    transient IDCheck idCheck;

	/**
	 * document type name
	 */
	Name docType;

    /**
     * Schema representation
     */    
    Element schema = null;

    static Atom nameXML = Atom.create("XML");
		
	static
    {
        builtin = new Hashtable();
        String n = "apos";
        builtin.put("apos", new Entity(Name.create(n), false, 39));
        n = "quot";
        builtin.put(n, new Entity(Name.create(n), false, 34));  
        n = "amp";
        builtin.put(n, new Entity(Name.create(n), false, 38));
        n = "lt";
        builtin.put(n, new Entity(Name.create(n), false, 60));
        n = "gt";
        builtin.put(n, new Entity(Name.create(n), false, 62));
        n = "nbsp";
        builtin.put(n, new Entity(Name.create(n), false, 160));
    }
}

class IDCheck
{
    Name name;
    int line;
    int col;
    Object owner;
    IDCheck next;

    IDCheck(IDCheck next, Name name, int line, int col, Object owner)
    {
        this.next = next;
        this.name = name;
        this.line = line;
        this.col = col;
        this.owner = owner;
    }
}

