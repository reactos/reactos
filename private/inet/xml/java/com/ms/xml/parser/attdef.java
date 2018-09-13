/*
 * @(#)AttDef.java 1.0 6/3/97
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

import java.io.IOException;
import java.util.Vector;
import java.util.Enumeration;

/**
 * This object describes an attribute type and potential values.
 * This encapsulates the information for one Attribute in an
 * Attribute List in a DTD as described below:
 */
public class AttDef
{
    /**
     * type of attribute
     */    
    public static final int CDATA      = 0;
    public static final int ID         = 1;
    public static final int IDREF      = 2;
    public static final int IDREFS     = 3;
    public static final int ENTITY     = 4;
    public static final int ENTITIES   = 5;
    public static final int NMTOKEN    = 6;
    public static final int NMTOKENS   = 7;
    public static final int NOTATION   = 8;
    public static final int ENUMERATION= 9;
    
    String typeToString()
    {        		
        switch(type)
		{
        case ID:
            return "ID";
		case IDREF:
            return "IDREF";
		case IDREFS:
            return "IDREFS";
		case ENTITY:
            return "ENTITY";
		case ENTITIES:
            return "ENTITIES";
		case NMTOKEN:
            return "NMTOKEN";
		case NMTOKENS:
            return "NMTOKENS";
		case NOTATION:
            return "NOTATION";
		case ENUMERATION:
            return "ENUMERATION";
		case CDATA:
        default:
            return "CDATA";
		}
    }

    /**
     * presence of attribute
     */    
    public static final int DEFAULT    = 0;
    public static final int REQUIRED   = 1;
    public static final int IMPLIED    = 2;
    public static final int FIXED      = 3;

    String presenceToString()
    {
		switch(presence)
		{
		case IMPLIED:
            return "IMPLIED";
        case REQUIRED:
            return "REQUIRED";
		case FIXED:
            return "FIXED";
        case DEFAULT:
        default:
            return "DEFAULT";
        }               
    }

    /**
     * name of attribute declared
     */
    Name name;
    
    /**
     * Construct new object for given attribute type.
     * @param name the name of the attribute
     * @param type the attribute type
     */    
    AttDef(Name name, int type)
    {
        this(name, type, null, 0, null);
    }
    
    /**
     * Construct new object for given attribute type and
     * array of possible values.
     */    
    AttDef(Name name, int type, Vector values)
    {
        this(name, type, null, 0, values);
    }
    
    /**
     * Construct new object for given attribute type.
     */    
    AttDef(Name name, int type, Object def, int presence)
    {
        this(name, type, def, presence, null);
    }
    
    /**
     * Construct new object for given attribute type.
     */    
    AttDef(Name name, int type, Object def, int presence, Vector values)
    {
        this.name = name;
        this.type = (byte)type;
        this.def = def;
        this.presence = (byte)presence;
        this.values = values;
    }

    /**
     * Parse the attribute types.
     */    
	final Object parseAttribute(Element e, Parser parser) throws ParseException
	{
		parser.parseToken(Parser.QUOTE, "string");
		return parseAttValue(e, parser, false);
	}

    private void reportMismatch(Parser parser) throws ParseException
	{
		parser.error("Attribute " + name + " should have the fixed value \"" + def + "\"");
	}

	private void reportEmpty(Parser parser, String s) throws ParseException
	{
		parser.error("Expected " + parser.tokenString(parser.NAME, s) + " insteadof " + parser.tokenString(parser.token, null));
	}

	/**
	 *	parse attribute values
	 */
	final Object parseAttValue(Element e, Parser parser, boolean inDTD) throws ParseException
	{
		int i;
		Vector names;
		Name name = null;
		Object value;
        StringBuffer buf;

		switch(type)
		{
		case CDATA:
			parser.scanString(parser.quote, '&', '&', '<');
			value = parser.text;
			if (!inDTD && presence == FIXED && !def.toString().equals(parser.text))
				reportMismatch(parser);
			break;
		case ID:
			parser.simplename++;
			parser.parseToken(parser.NAME, "ID");
			parser.simplename--;
			parser.parseToken(parser.QUOTE, "string");
			if (!inDTD)
			{
				if (presence == FIXED && (Name)def != parser.name)
					reportMismatch(parser);
				Element e1 = parser.dtd.findID(parser.name);
				if (e1 != null)
				{
					parser.error("ID " + name + " is already used on element " + e1.getTagName());
				}
				parser.dtd.addID(parser.name, e);
			}
			value = parser.name;
			break;
		case IDREF:
		case IDREFS:
            names = new Vector();
            buf = new StringBuffer();
			parser.simplename++;
			i = parser.parseNames(names, Parser.INVALIDTOKEN, buf);
			parser.simplename--;
			if (i == 0)
				reportEmpty(parser, "IDREF");
			if (i > 1 && type == IDREF)
				parser.error("IDREF type attribute \"" + this.name + "\" cannot refer to more than one ID.");
            if (!inDTD)
			{
				for (--i; i >= 0; --i)
				{
			        name = (Name)names.elementAt(i);
					if (presence == FIXED)
					{
						if (type == IDREFS)
						    checkFixed(name, parser);
						else if ((Name)def != name) reportMismatch(parser);
					}
					if (parser.dtd.findID(name) == null)
					{
						// add it to linked list to check later
						parser.dtd.addIDCheck(name, 
							parser.reader.line, parser.reader.column - 1, 
							parser.reader.owner);
					}
				}
			}          

            value = type == IDREF ? (Object)names.elementAt(0) : (Object)names;
			break;
		case ENTITY:
		case ENTITIES:
            buf = new StringBuffer();
            names = new Vector();
			parser.nouppercase++;
			i = parser.parseNames(names, Parser.INVALIDTOKEN, buf);
			parser.nouppercase--;
			if (i == 0)
				reportEmpty(parser, "ENTITY name");
 			if (i > 1 && type == ENTITY) 
				parser.error("ENTITY type attribute \"" + this.name + "\" cannot refer to more than one entity.");
			for (--i; i >= 0; --i)
			{
				name = (Name)names.elementAt(i);
				if (parser.dtd.findEntity(name) == null)
				{
					parser.error("Couldn't find entity '" + name + "'");
				}
				if (!inDTD && presence == FIXED) 
				{
					if (type == ENTITIES)
						checkFixed(name, parser);
					else if (name != (Name)def) reportMismatch(parser);
				}
			}

            value = type == ENTITY ? (Object)names.elementAt(0) : (Object)names;
			break;
		case NMTOKEN:
		case NMTOKENS:
            buf = new StringBuffer();
            names = new Vector();
			parser.nametoken++;
			i = parser.parseNames(names, Parser.INVALIDTOKEN, buf);
			parser.nametoken--;
			if (i == 0)
				reportEmpty(parser, "NMTOKEN");
			if (i > 1 && type == NMTOKEN)
				parser.error("NMTOKEN type attribute \"" + this.name + "\" cannot refer to more than one name token.");
			if (!inDTD && presence == FIXED)
			{
				if (type == NMTOKEN)
				{
					if ((Name)def != (Name)names.elementAt(0))
						reportMismatch(parser);
				}
				else 
				{
				    for (i = names.size() - 1; i >= 0; i--)
					{
					    checkFixed((Name)names.elementAt(i), parser);
					}
				}
			}

			value = type == NMTOKEN ? (Object)names.elementAt(0) : (Object)names;
			break;
		case NOTATION:
		case ENUMERATION:
			if (type == ENUMERATION)
				parser.nametoken++;
			parser.parseToken(parser.NAME, "name");
			if (type == ENUMERATION)
				parser.nametoken--;
			parser.parseToken(parser.QUOTE, "string");
			for (i = values.size() - 1; i >= 0; i--)
			{
				if ((Name)values.elementAt(i) == parser.name)
				{
					break;
				}
			}
			if (i < 0)
			{
				parser.error("Attribute value '" + parser.name + "' is  not in the allowed set.");
			}	
			if (!inDTD && presence == FIXED && parser.name != def)
			    reportMismatch(parser);

			value = parser.name;
			break;
		default:
			value = null;
		}
		return value;
	}

	private void checkFixed(Name name, Parser parser) throws ParseException
	{
		Vector v = (Vector)def;
		int i;
		for (i = v.size() - 1; i >= 0; i--)
		{
			if ((Name)v.elementAt(i) == name)
			{
				break;
			}
		}
		if (i < 0)
		{
			parser.error("Attribue value " + name + " is not in the fixed value set.");
		}
	}

    Element toSchema()
    {
        Element attribute = new ElementImpl( nameATTRIBUTE, Element.ELEMENT );
        attribute.setAttribute( nameID, name.getName() );
        attribute.setAttribute( nameTYPE, typeToString() );

        if( presence != DEFAULT )
            attribute.setAttribute( namePRESENCE, presenceToString() );
                
        if( type == ENUMERATION )
        {
            String valueString = "";
            for (int i = 0; i < values.size(); i++) {   
                Name name = (Name)values.elementAt(i);
                valueString = valueString + name.toString();
                if (i < values.size() - 1)
                    valueString = valueString + " ";
            }
            attribute.setAttribute( nameVALUES, valueString );
        }

        if( def != null )
            attribute.setAttribute( nameDEFAULT, def );

        return attribute;
    }

    void save(Atom ns, XMLOutputStream o) throws IOException
    {
        if (name == Parser.nameXMLSpace)
            o.writeChars(name.getName());
        else
            o.writeQualifiedName(name, ns);
		switch(type)
		{
		case CDATA:
            o.writeChars(" CDATA");
            break;
		case ID:
            o.writeChars(" ID");
            break;
		case IDREF:
            o.writeChars(" IDREF");
            break;
		case IDREFS:
            o.writeChars(" IDREFS");
            break;
		case ENTITY:
            o.writeChars(" ENTITY");
            break;
		case ENTITIES:
            o.writeChars(" ENTITIES");
            break;
		case NMTOKEN:
            o.writeChars(" NMTOKEN");
            break;
		case NMTOKENS:
            o.writeChars(" NMTOKENS");
            break;
		case NOTATION:
            o.writeChars(" NOTATION");
            // fall through
		case ENUMERATION:
            o.writeChars(" (");
            for (int i = 0; i < values.size(); i++) {   
                Name name = (Name)values.elementAt(i);
				if (type == NOTATION)
                    o.writeQualifiedName(name, ns);
				else o.writeChars(name.toString());
                if (i < values.size() - 1)
                    o.write('|');
            }
            o.write(')');
            break;
        }
        switch (presence) {
        case REQUIRED:
            o.writeChars(" #REQUIRED");
            break;
        case IMPLIED:
            o.writeChars(" #IMPLIED");
            break;
		case DEFAULT:
        case FIXED:
			if (presence == FIXED)
			    o.writeChars(" #FIXED");
            if (def != null) {
				o.writeChars(" ");
				switch (type)
				{
					case CDATA:
                    case ID:
                    case IDREF:
                    case NMTOKEN:
					case ENUMERATION:
						o.writeQuotedString(def.toString());
						break;
					case NOTATION:
                    case ENTITY:
						o.write('\"');
						o.writeQualifiedName((Name)def, ns);
                        o.write('\"');
						break;
                    case ENTITIES:
                    case IDREFS:
                    case NMTOKENS:
                        o.write('\"');
						int i = 0;
						for (Enumeration e = ((Vector)def).elements(); e.hasMoreElements();)
						{
							if (i++ > 0)
								o.write(Parser.OR);
							if (type == ENTITIES)
							    o.writeQualifiedName((Name)e.nextElement(), ns);
                            else o.writeChars(e.nextElement().toString());
						}
						o.write('\"');
						break;
				}
            }
            break;
        }
    }
   
    byte type;    
    byte presence;    
    
    /**
     * default value, can be null
     */    
    Object def;
    
    /**
     * array of values for enumerated and notation types
     */    
    Vector values;

    /**
     * Return the default value for the attribute.
     */    
    public Object getDefault()
	{
		return def;
	}

    /**
     * Return the name of the attribute.
     */
    public Name getName()
    {
        return name;
    }

    /** 
     * Return the attribute type.
     */
    public int getType()
    {
        return (int)type;
    }

    /**
     * Return the precence type for the attribute.
     */
    public int getPresence()
    {
        return (int)presence;
    }

    static Name nameATTRIBUTE = Name.create("ATTRIBUTE","XML");
    static Name nameID        = Name.create("ID","XML");
    static Name nameDEFAULT   = Name.create("DEFAULT","XML");
    static Name namePRESENCE  = Name.create("PRESENCE","XML");
    static Name nameVALUES    = Name.create("VALUES","XML");
    static Name nameTYPE      = Name.create("TYPE","XML");
}    
