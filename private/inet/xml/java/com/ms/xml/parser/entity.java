/*
 * @(#)Entity.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.om.Element;
import com.ms.xml.om.ElementImpl;
import com.ms.xml.util.EnumWrapper;
import com.ms.xml.util.StringInputStream;

import java.io.*;
import java.util.Enumeration;
import java.util.Vector;

/**
 * This class implements an <code>Entity</code> object representing an XML internal 
 * or external entity as defined in the XML Document Type Definition (DTD).
 *
 * @version 1.0, 6/3/97
 */
public class Entity extends ElementImpl
{
    static String nameENTITY = "ENTITY";
    static Atom nameXML = Atom.create("XML");
	static Atom nameSpaceID = Atom.create("//XML/NAMESPACE");

    Entity(Name name, boolean par)
    {
		super(Name.create(nameENTITY, nameXML), Element.ENTITY);
        this.name = name;
        this.par = par;
        this.parsed = false;
    }

    Entity(Name name, boolean par, String text)
    {
        this(name, par);
        setText(text);
        setPosition(0, 0);
    }
    
    Entity(Name name, boolean par, int c)
    {
        this(name, par);
        cdata = (char)c;
        super.setText(String.valueOf(cdata));
        setPosition(0, 0);
    }
    
    EntityReader getReader(EntityReader prev)
    {
        return new EntityReader(
                new StringInputStream(text),
                line, column, prev, this);
    }

    void setURL(String url)
    {
        this.url = url;
        sys = true;
    }
   
	String getURL()
	{
		return url;
	}

    void setNDATA(Name name)
    {
        ndata = name;
    }

    /**
    * Changes the text of entity.
    * @param text  The new text of the entity.
    */
    public void setText(String text)
    {
        this.text = text;
        sys = false;
    }

    void setPosition(int line, int column)
    {
        this.line = line;
        this.column = column;
    }

    int getLength()
    {
		if (cdata > 0)
			return -1;
        else if (text == null)
            return 0;
        else return text.length();
    }

    char getChar(int index)
    {
        if (text == null)
            return cdata;
        else
            return text.charAt(index);
    }

    public Object getAttribute(Name attName)
	{
        // lazy construct the ElementImpl set of attributes.
        getAttributes();
        return super.getAttribute(attName);
    }

    public Enumeration getAttributes()
	{
        // lazy construct the ElementImpl set of attributes.
        if (super.getAttribute(nameNAME) == null) {
             setAttribute(nameNAME,name);
             if (pubid != null) setAttribute(namePUBLICID, pubid);
             if (url != null) setAttribute(nameSYSTEMID, url);
             if (ndata != null) setAttribute(nameNOTATION, ndata);
             // setAttribute(namePAR, (par == true) ? "TRUE" : "FALSE");
        }
		return super.getAttributes();
	}

	/**
    * Saves the entity to the given output stream with indenting and new lines.
    * @param o  The output stream.
    * @exception IOException if there is a problem writing to the output stream. 
    */
    public void save(XMLOutputStream o) throws IOException
    {
        if (o.savingDTD) {
            o.writeIndent();
            saveEntity(o);
            o.writeNewLine();
        } else {
            super.save(o);
        }
    }

	/**
    * Saves the entity to the given output stream.
    * @param o  The output stream.
    * @exception  IOException if there is a problem writing to the output 
      * stream. 
    */
    public void saveEntity(XMLOutputStream o) throws IOException
    {
        o.writeChars("<!ENTITY ");
        if (par) o.writeChars("% ");
		o.writeQualifiedName(name, null);
        o.writeChars(" ");

		if (url != null) {
			if (pubid == null)
				o.writeChars("SYSTEM ");
			else 
			{
			    o.writeChars("PUBLIC ");
                o.writeQuotedString(pubid);
                o.write(' ');
			}
            o.writeQuotedString(url.toString());
			if (ndata != null) 
			{
				o.writeChars(" NDATA ");
	            o.writeQualifiedName(ndata, name.getNameSpace());
			}
		}
        else if (text != null) o.writeQuotedString(text);

		o.write('>');
    }

   /**
    * Retrieves the name of the entity.
	* @return the <code>Name</code> object containing the entity name.
    */
    public Name getName()
    {
        return name;
    }

    public Name getTagName()
    {
        if( sys )
            return nameEXTENTITYDCL;
        else
            return nameINTENTITYDCL;
    }

    /**
     * Name of entity
     */
    Name name;

    /**
     * Url for external entity
     */
    String url;

	/**
	 * Pubid for external entity
	 */
	String pubid;

    /**
     * Text for internal entity
     */
    String text;

    /**
     * Char for internal entity
     */
    char cdata;

    /**
     * NDATA identifier
     */    
    Name ndata;

    /**
     * line number
     */    
    int line;
    
    /**
     * character pos
     */    
    int column;
    
    /**
     * set if paramater entity
     */    
    boolean par;
    
    /**
     * set if external entity
     */    
    boolean sys;

    /**
     * whether this entity has been used (i.e. it has been
     * included in document some place, like &foo;).
     */
    boolean parsed;

    static Name nameNAME         = Name.create("NAME","XML");
    static Name namePUBLICID     = Name.create("PUBLICID","XML");
	static Name nameSYSTEMID     = Name.create("SYSTEMID","XML");
  	static Name nameINTENTITYDCL = Name.create("INTENTITYDCL","XML");
	static Name nameEXTENTITYDCL = Name.create("EXTENTITYDCL","XML");
    static Name nameNOTATION     = Name.create("NOTATION","XML");
    static Name namePAR          = Name.create("PAR","XML");
}