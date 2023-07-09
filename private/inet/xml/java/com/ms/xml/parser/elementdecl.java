/*
 * @(#)ElementDecl.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.ElementImpl;
import com.ms.xml.om.Element;
import com.ms.xml.om.ElementFactoryImpl;
import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLOutputStream;

import java.lang.String;
import java.util.Vector;
import java.util.Enumeration;
import java.net.URL;
import java.io.IOException;

/**
 * The class represents an element declaration in an XML Document Type Definition (DTD).
 *
 * @version 1.0, 6/3/97
 */
public class ElementDecl extends ElementImpl
{    
    /**
     * name of element declared
     */
    Name name;
    
    /**
     * attribute list
     * @see AttDef
     */    
    private Vector attdefs;

    /**
     * Shared content
     */
    private ContentModel content;

	/**
	 * Super class
	 */
	ElementDecl base;

	/**
	 * Interfaces
	 */
	Vector interfaces;

    /**
     * Schema representation
     */    
    Element schema = null;

    ElementDecl(Name name)
    {
        super(name,Element.ELEMENTDECL);
        this.name = name;
    }
    
    /**
     * Parse element declaration:
     */
    final void parseModel(Parser parser) throws ParseException
    {
        content = new ContentModel();
        parser.parseKeyword(0, "EMPTY|...");

		// check for extends NAME
		if (parser.token == Parser.EXTENDS)
		{
            parser.parseToken(Parser.NAME, "base element declaration");
			base = parser.dtd.findElementDecl(parser.name);
			if (base == null)
			{
				parser.error("Didn't find element declaration to extend " + parser.name);
			}
	        parser.parseKeyword(0, "EMPTY|...");
		}

		// check for implements NAME, NAME, ...
		if (parser.token == Parser.IMPLEMENTS)
		{
			interfaces = new Vector();
			int i = parser.parseNames(interfaces, Parser.COMMA, null);
            for (--i; i >= 0; i--)
			{
				ElementDecl ed = parser.dtd.findElementDecl((Name)interfaces.elementAt(i));
				if (ed == null)
				{
					parser.error("Didn't find element declaration to implement " + interfaces.elementAt(i));
				}
				else
				{
					interfaces.setElementAt(ed, i);
				}
			}
			// parseNames called nextToken() already...
			if (parser.token == Parser.NAME)
			{
				parser.token = parser.lookup(name.getName());
			}
		}

		switch (parser.token)
        {
            case Parser.EMPTY:
                content.type = ContentModel.EMPTY;
                parser.nextToken();
                break;
            case Parser.ANY:
                content.type = ContentModel.ANY;
                parser.nextToken();
                break;
            case Parser.LPAREN:
                content.type = ContentModel.ELEMENTS;
                content.parseModel(parser);
                break;
			case Parser.TAGEND:
				// it's ok to not to have content model if inherited
				if (base != null)
				{
					content = base.content;
					break;
				}
			default:
				parser.error("Expected EMPTY, ANY, or '('.");
		}
		if (parser.token != Parser.TAGEND)
		{
			parser.error("Element declaration syntax error. Expected '>' instead of '" + parser.tokenString(parser.token) + "'");
		}
	}
    
    final void parseAttList(Parser parser) throws ParseException
    {
        int i;
        Vector names;
        AttDef attdef;       
        parser.nextToken();
        while (parser.token == Parser.NAME)
        {
            Name n = parser.name;

            // make sure XML-SPACE belongs to the XML namespace.
            if (n.getName().equals(parser.nameXMLSpace.getName()))
                n = parser.nameXMLSpace;

            parser.parseKeyword(0, "Attdef");
            switch (parser.token)
            {
                case Parser.CDATA:
                case Parser.ID:
                case Parser.IDREF:
                case Parser.IDREFS:
                case Parser.ENTITY:
                case Parser.ENTITIES:
                case Parser.NMTOKEN:
                case Parser.NMTOKENS:
                   attdef = new AttDef(n, Parser.CDATA - parser.token + AttDef.CDATA);
                   break;
                case Parser.NOTATION:
                    parser.parseToken(Parser.LPAREN, "(");
                    names = new Vector();
                    i = parser.parseNames(names, Parser.INVALIDTOKEN, null);
                    for (--i; i >= 0; i--)
					{
                        Name name = (Name)names.elementAt(i);
                        if (parser.dtd.findNotation(name) == null)
                        {
                            parser.error("Notation not declared '" + name + "'");
                        }
                    }
                    parser.parseToken(Parser.RPAREN, ")");
                    attdef = new AttDef(n, AttDef.NOTATION, names);
                    break;
                case Parser.LPAREN:
                    parser.nametoken++;
                    names = new Vector();
					i = parser.parseNames(names, Parser.OR, null);
                    parser.nametoken--;
                    if (parser.token != Parser.RPAREN)
                    {
                        parser.error("Expected )");
                    }
                    attdef = new AttDef(n, AttDef.ENUMERATION, names);
                    break;
                default:
                    attdef = null;
                    parser.error("Unknown token in attlist declaration " + parser.token);
                    break;
            }
       
            if (parser.nextToken() == Parser.HASH)
			{
                parser.parseKeyword(0, "attribute default");
                switch (parser.token)
                {
                    case Parser.REQUIRED:
                        attdef.presence = (byte)AttDef.REQUIRED;
                        break;
                    case Parser.IMPLIED:
                        attdef.presence = (byte)AttDef.IMPLIED;
                        break;
                    case Parser.FIXED:    
						attdef.presence = (byte)AttDef.FIXED;
                        break;
					default:
						parser.error("Expected 'REQUIRED', 'IMPLIED', or 'FIXED'.");
                }
				parser.nextToken();
            }
            if (attdef.presence == (byte)AttDef.FIXED || 
                attdef.presence == (byte)AttDef.DEFAULT)
            {
                switch(parser.token)
                {
                    case Parser.QUOTE:
						attdef.def = attdef.parseAttValue(this, parser, true);
						parser.nextToken();
                        break;
                    default:
                        parser.error("Expected FIXED or DEFAULT value for " + attdef.name);
                }
            }
            addAttDef(attdef);
        }
    }
    
    final void initContent(Context context, Parser parser) throws ParseException
    {
        context.ed = this;
        AttDef attdef = findAttDef(parser.nameXMLSpace);
        if (attdef != null) {
            Object defval = attdef.getDefault();
            if (defval != null) {
                // The XML-SPACE declaration has a default value, which
                // should be used as default for context.preserveWS field.
                String s = (String)defval;
                context.preserveWS = s.equalsIgnoreCase("preserve");
            }
        }
        content.initContent(context, parser);
    }

    final boolean checkContent(Context context, Element e, Parser parser) throws ParseException
    {
        return content.checkContent(context, e, parser);
    }

	final boolean acceptEmpty(Context context)
	{
		return content.acceptEmpty(context);
	}

	final Vector expectedElements(int state)
	{
		return content.expectedElements(state);
	}

	final void checkOwnAttributes(Element e, Parser parser) throws ParseException
	{
		if (attdefs == null) return;

		for (Enumeration en =  attdefs.elements(); en.hasMoreElements();)
		{
			AttDef ad = (AttDef)en.nextElement(); 
			if (ad.getPresence() == AttDef.REQUIRED && e.getAttribute(ad.getName()) == null)
				parser.error("Attribute '" + ad.getName() + "' is required.");
		}
	}

	final void checkAttributes(Element e, Parser parser) throws ParseException
	{
		checkOwnAttributes(e, parser);
		if (base != null)
		{
			base.checkOwnAttributes(e, parser);
		}
		if (interfaces != null)
		{
			for (Enumeration en = interfaces.elements(); en.hasMoreElements(); )
			{
				((ElementDecl)en.nextElement()).checkOwnAttributes(e, parser);
			}
		}
	}

    /**
     * Retrieves the content model object for the element declaration.
     * @return  the content model for the element.
     */
    public final ContentModel getContent()
    {
        return content;
    }

   /**
    * Retrieves the name of the element declaration.
    * @return  the <code>Name</code> object containing the element declaration 
      * name.
    */
    public final Name getName()
    {
        return name;
    }

    final void parseAttribute(Element e, Name name, Parser parser) throws ParseException
    {
		AttDef attdef = findAttDef(name);
		if (attdef == null)
		{
			parser.error("Illegal attribute name " + name);
		}
		Object value = attdef.parseAttribute(e, parser);
		e.setAttribute(name, value);
    }
    
    final void addAttDef(AttDef attdef)
    {
        if (attdefs == null)
        {
            attdefs = new Vector();
        }
        attdefs.addElement(attdef);
    }

    /**
     * Retrieves the attribute definition of the named attribute.
     * @param name  The name of the attribute.
     * @return  an attribute definition object; returns null if it is not found.
     */
	public final AttDef findAttDef(Name name)
	{
		if (attdefs != null)
		{
            for (Enumeration en =  attdefs.elements(); en.hasMoreElements();)
		    {
		    	AttDef attdef = (AttDef)en.nextElement(); 
				if (attdef.name == name)
					return attdef;
			}
		}
		if (base != null)
		{
			AttDef attdef = base.findAttDef(name);
			if (attdef != null)
				return attdef;
		}
		if (interfaces != null)
		{
			for (Enumeration en = interfaces.elements(); en.hasMoreElements(); )
			{
				AttDef attdef = ((ElementDecl)en.nextElement()).findAttDef(name);
				if (attdef != null)
					return attdef;
			}
		}
		return null;
	}

    public Element toSchema()
    {
        if( schema == null )
        {
            Element elementType = new ElementImpl( nameELEMENTTYPE, Element.ELEMENT );
            elementType.setAttribute( nameID, name );

            Element elementContent = content.toSchema();
            elementType.addChild( elementContent, null );

            if (attdefs != null) 
            {
                Enumeration e = attdefs.elements();
                while (e.hasMoreElements()) 
                {
                    AttDef def = (AttDef)e.nextElement();
                    elementType.addChild( def.toSchema(), null );
                }
            }

            schema = elementType;
        }
          
        return schema;
    }

    /**
     * Saves the element declaration in DTD syntax to the given
     * output stream.
     * @param o  The output stream to write to.
      * @return No return value.
     * @exception  IOException if there is a problem writing to the output 
      * stream.
     */
    public void save(XMLOutputStream o) throws IOException
    {
        o.writeIndent();
        o.writeChars("<!ELEMENT ");
        o.writeQualifiedName(name, null);
	    o.writeChars(" ");	
		Atom ns = name.getNameSpace();
		content.save(ns, o);
        o.write('>');

        if (attdefs != null) {
            // have to turn on PRETTY output for this to work properly.
            int style = o.getOutputStyle();
            if (style != XMLOutputStream.COMPACT)
                o.setOutputStyle(XMLOutputStream.PRETTY);
    		o.writeNewLine();
            Enumeration e = attdefs.elements();
            o.writeIndent();
            o.writeChars("<!ATTLIST ");
			o.writeQualifiedName(name, null);
			o.writeChars(" ");
            while (e.hasMoreElements()) {
                o.writeNewLine();
                AttDef def = (AttDef)e.nextElement();
                o.addIndent(1);
                o.writeIndent();
                o.addIndent(-1);
                def.save(ns, o);
            }
            o.write('>');
			o.setOutputStyle(style);
        } 
        o.writeNewLine();        
    }

    static Name nameELEMENTTYPE = Name.create("ELEMENTTYPE","XML");
    static Name nameID = Name.create("ID", "XML");
}



