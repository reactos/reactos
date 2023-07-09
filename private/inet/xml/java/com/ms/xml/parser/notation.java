/*
 * @(#)Notation.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.ElementImpl;
import com.ms.xml.om.Element;

import com.ms.xml.util.Name;
import com.ms.xml.util.XMLOutputStream;
import java.io.*;

/**
 * This class implements an entity object representing an XML notation.
 *
 * @version 1.0, 6/3/97
 */
public class Notation extends ElementImpl
{
    Notation(Name name)
    {
        super(name,Element.NOTATION);
        this.name = name;
    }

    void setURL(String url)
    {
        this.url = url;
    }

    public Element toSchema()
    {
        if( schema == null )
        {
            Element notationElement = new ElementImpl( nameNOTATION, Element.ELEMENT );
            notationElement.setAttribute( nameNAME, name.toString() );
            if( url != null )
            {
                notationElement.setAttribute( nameSYSTEMID, url );
            }
            if( pubid != null )
            {
                notationElement.setAttribute( namePUBLICID, pubid );
            }
            schema = notationElement;
        }         

        return schema;
    }
   
	/**
    * Saves the notation to the given output stream with indenting and new lines.
    * @param o The output stream.
    * @exception IOException if there is a problem writing to the output stream. 
    */
    public void save(XMLOutputStream o) throws IOException
    {
        o.writeIndent();
        o.writeChars("<!NOTATION ");
		o.writeQualifiedName(name, null);
        o.writeChars(" ");

		if (type == Parser.PUBLIC) {
			o.writeChars("PUBLIC ");
            o.writeQuotedString(pubid);
            o.write(' ');
            o.writeQuotedString(url);
		} else {
			o.writeChars("SYSTEM ");
            o.writeQuotedString(url);
		}
		o.write('>');
        o.writeNewLine();
    }

    /**
     * Name of Notation
     */
    Name name;

    /**
     * Url 
     */
    String url;

    /**
	 * pubid
	 */
    String pubid; 

    /**
     * Type of notation (Parser.SYSTEM or Parser.PUBLIC)
     */
    int type;

    /**
     * Schema representation
     */
    Element schema = null;

	static Name nameNAME         = Name.create("NAME");
	static Name nameSYSTEMID     = Name.create("SYSTEMID");
	static Name namePUBLICID     = Name.create("PUBLICID");
    static Name nameNOTATION     = Name.create("NOTATION");

}
