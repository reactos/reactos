/*
 * @(#)Document.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.om;

import com.ms.xml.parser.ElementDeclEnumeration;
import com.ms.xml.parser.Parser;
import com.ms.xml.parser.ParseException;
import com.ms.xml.parser.DTD;
import com.ms.xml.parser.ElementDecl;
import com.ms.xml.parser.Entity;
import com.ms.xml.util.Name;
import com.ms.xml.util.Attributes;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.util.Attribute;
import com.ms.xml.util.EnumWrapper;
import com.ms.xml.util.Atom;

import java.util.Vector;
import java.lang.String;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.net.URLConnection;

/**
 * This class implements an XML document, which can be thought of as the root of a tree.
 * Each XML tag can either represent a node or a leaf of this tree.  
 * The <code>Document</code> class allows you to load an XML document, manipulate it, 
 * and then save it back out again. The document can be loaded by specifying 
 * a URL or an input stream. 
 * <P>
 * According to the XML specification, the root of the tree consists of any combination
 * of comments and processing instructions, but only one root element.
 * A helper method <code>getRoot</code> is provided
 * as a short cut to finding the root element.
 *
 * @version 1.0, 6/3/97
 * @see Element
 */
public class Document extends ElementImpl implements ElementFactory
{
    /**
     * Constructs a new empty document using the default
     * element factory.
     */
    public Document()
    {        
        outputStyle = XMLOutputStream.DEFAULT;
        factory = new ElementFactoryImpl();
        dtd = new DTD();
        caseInsensitive = false;
        loadExternal = true;
    }

    /**
     * Constructs a new empty document using the given
     * <code>ElementFactory</code> object when building the XML element
     * hierarchy.
     * @param f The <code>ElementFactory</code> to use.
     */
    public Document(ElementFactory f)
    {
        outputStyle = XMLOutputStream.DEFAULT;
        factory = f;
        dtd = new DTD();
        caseInsensitive = false;
        loadExternal = true;
    }

    /**
     * Retrieves the document type.
     * @return the value defined by <code>Element.DOCUMENT</code>.
     */
    public int getType()
    {
        return DOCUMENT;
    }

    /**
     * Retrieves the document text.
     * @return a plain text (that is, not marked-up) representation of the entire document.
     */
    public String getText()
    {
        return (root != null) ? root.getText() : null;
    }

    /**
     * Passes the text through to the root node, if there is a root node. 
     * @param text The text to be set.
     */
    public void setText(String text)
    {
        if (root != null) root.setText(text);
    }

    /**
     * Retrieves the parent element. There is no parent element for <code>Document</code>, so this method
     * always returns null. 
      
     * @return null.      
	*/
    public Element getParent()
    {
        return null;
    }

    /**  
     * Adds a child <code>Element</code> to the document. 
     * This is an override of the <code>Element.addChild</code> method that 
      * stores 
     * the new child and stores the last element of type <code>Element.ELEMENT</code>, 
     * so that <code>getRoot</code> can be used as a short cut to find a particular element.
     * @param elem  The child element to add.
     * @param after  The element after which to add the <i>elem</i> element.
     */
	public void addChild(Element elem, Element after)
    {
        if (elem.getType() == Element.ELEMENT) 
        {
            root = elem;
        } 
        else if (elem.getType() == Element.PI && 
                    elem.getTagName() == nameXML) 
        {
            XML = elem;
        } 
        else if (elem.getType() == Element.DTD) 
        {
            DTDnode = elem;
        }
        
        super.addChild(elem,after);
	}

    /**
* Removes the specified child <code>Element</code> from the Document. 
* @param elem The child element to remove.
     */
	public void removeChild(Element elem)
	{
		super.removeChild(elem);

        if (root == elem) 
        {
            root = null;
        } 
        else if (XML == elem) 
        {
            XML = null;
        } 
        else if (XML == DTDnode) 
        {
            DTDnode = null;
        }
	}

    /**  
     * Retrieves the root node of the XML parse tree. This is
     * guaranteed to be of type <code>Element.ELEMENT</code>.
     * @return the root node.
     */
    public final Element getRoot()
    {
        return root;
    }

    /**
     * Returns the information stored in the &lt;?XML ...?&gt; tag
     * as an Element.  Typically this has two attributes
     * named VERSION and ENCODING.
     * It is a private function because users should not be able to mess around
     * with the XML element.  Set/getVersion and set/getEncoding are available 
     * for those purposes.
     */
    private final Element getXML()
    {
        return XML;
    }

    /**
     * Retrieves the version information.
     * @return the version number stored in the &lt;?XML ...?&gt; tag. 
     */
    public final String getVersion()
    {
        if (getXML() != null) 
        {
    		Object v = getXML().getAttribute(nameVERSION);
	    	if (v != null)
		    	return v.toString();
        }
		return defaultVersion;
    }

    /**
     * Sets the version number stored in the &lt;?XML ...?&gt; tag.      
     * @param version The version information to set.
      * @return No return value.
     */
    public final void setVersion( String version )
    {
        if (XML == null)
        {
            XML = createElement(this, Element.PI, nameXML, null);
        }
    	XML.setAttribute(nameVERSION, version );
    }

    /**
     * Retrieves the character encoding information.
     * @return the encoding information  stored in the &lt;?XML ...?&gt; tag or the user-defined output encoding 
     * if it has been more recently set.
     */
    public final String getEncoding()
    {
        if( outputEncoding != null )
            return outputEncoding;
        else
		{
            if (getXML() != null) 
            {
    			Object v = getXML().getAttribute(nameENCODING);
	    		if (v != null)
		    		return v.toString();
            }
			return defaultEncoding;
		}
    }

    /**
     * Sets the character encoding for output. Eventually it sets the ENCODING 
     * stored in the &lt;?XML ...?&gt; tag, but not until the document is saved.
     * You should not call this method until the Document has been loaded.
      * @return No return value.
     */
    public final void setEncoding( String encoding )
    {
        outputEncoding = encoding;        
        if (XML == null)
        {
            XML = createElement(this, Element.PI, nameXML, null);
            XML.setAttribute(nameENCODING, encoding);
        }
    }

    /**
     * Retrieves the character set. This is
     * an alias for the <code>setEncoding</code> method and exists for 
      * compatibility
     * reasons only. 
     * @return  the character set. 
     */
    public final String getCharset()
    {
        return getEncoding();
    }

    /**
     * Sets the character set.
     * This is an alias for the <code>getEncoding</code> method and exists for compatibility
     * reasons only.
     * @param encoding The encoding information.
     * 
      * @return No return value.
     */
    public final void setCharset(String encoding)
    {
        setEncoding(encoding);
    }

    /**
     * Retrieves the standalone information.
     * @return the standalone attribute stored in the &lt;?XML ...?&gt; tag. 
     */
    public final String getStandalone()
    {
        if (getXML() != null) 
        {
    		Object v = getXML().getAttribute(nameStandalone);
	    	if (v != null)
		    	return v.toString();
        }
		return null;
    }

    /**
     * Sets the RMD stored in the &lt;?XML ...?&gt; tag.      
     * @param value The attribute value ('yes' or 'no').
     * @return No return value.
     */
    public final void setStandalone( String value )
    {
        if (XML == null)
        {
            XML = createElement(this, Element.PI, nameXML, null);
        }
    	XML.setAttribute(nameStandalone, value );
    }

    /**
     * Returns the name specified in the &lt;!DOCTYPE&gt; tag.
     */
    public final Name getDocType()
    {
        if (DTDnode == null)
			return null;
		Object v = DTDnode.getAttribute(nameNAME);
		if (v != null)
			return (Name)v;
		return null;
    }

    /**
     * Retrieves the document type URL.
     * @return the URL specified in the &lt;!DOCTYPE&gt; tag or null 
     * if an internal DTD was specified.
     */
    public final String getDTDURL()
    {
		if (DTDnode == null) 
		    return null;
		Object v = DTDnode.getAttribute(nameURL);
		if (v != null)
			return v.toString();
		return null;
    }

    /**
     * Retrieves the URL.
     * @return the last URL sent to the load() method  or null 
     * if an input stream was used.
     */
    public final String getURL()
    {
        return URLdoc.toString();
    }


    /**
     * Retrieves the last modified date on the source of the URL.
     * @return the modified date.
     */
    public long getFileModifiedDate()
    {
        if (URLdoc != null)
        {
            if (URLdoc.getProtocol().equals("file"))
            {
                File f = new File(URLdoc.getFile());
                return f.lastModified();
            }
            else
            {
                try
                {
                    URLConnection URLcon = URLdoc.openConnection();
                    URLcon.connect();
                    return URLcon.getLastModified();
                }
                catch(IOException ie)
                {
                    return 0;
                }
            }
        }
        else
        {
            return 0;
        }
    }

    /**
     * Retrieves the external identifier. 
     * @return the external identifier specified in the &lt;!DOCTYPE&gt; tag 
     * or null if no &lt;!DOCTYPE&gt; tag was specified. 
     */
    public final String getId()
    {
        if (DTDnode == null)
			return null;
		Object v = DTDnode.getAttribute(namePUBLICID);
		if (v != null)
			return v.toString();
		return null;
    }

    /**
     * Creates a new element for the given element type and tag name using
     * the <code>ElementFactory</code> for this <code>Document</code>. 
     * This method allows the <code>Document</code> class to be used as an 
     * <code>ElementFactory</code> itself.
     * @param type The element type.
     * @param tag The element tag.
     */    
    public final Element createElement(Element parent, int type, Name tag, String text)
    {
        return factory.createElement(parent,type,tag,text);
    }

    /**
     * Creates a new element for the given element type and tag name.
     * tag is case sensitive.
     * @param type The element type.
     * @param tag The element tag name.
     */
    public final Element createElement(int type, String tag)
    {
        return factory.createElement(null,type, Name.create(tag),null);
    }

    /**
     * Creates a new element for a given element type with
     * a null tag name. 
     * @param type The element type.
     * @return the element created.
     * 
     */
    public final Element createElement(int type)
    {
        return factory.createElement(null,type,null,null);
    }

    /**
     * Called when the given element is completely parsed.
     * @param e The Element that has been parsed.
      * @return No return value.
     */
    public void parsed(Element e)
    {
        factory.parsed(e);
    }

    /**
     * delegate to contained element factory 
     */
    public void parsedAttribute(Element e, Name name, Object value)
    {
        factory.parsedAttribute(e,name,value);
    }

    /** 
     * Switch to determine whether next XML loaded will be treated
     * as case sensitive or not.  If not, names are folded to uppercase.
     */
    public void setCaseInsensitive(boolean yes)
    {
        caseInsensitive = yes;
    }

    /**
     * Return whether we're case insensitive.
     */
    public boolean isCaseInsensitive()
    {
        return caseInsensitive;
    }

    /** 
     * Switch to determine whether to load external DTD's
     * or entities.
     */
    public void setLoadExternal(boolean yes)
    {
        loadExternal = yes;
    }

    /**
     * Return whether to load external DTD's or entities.
     */
    public boolean loadExternal()
    {
        return loadExternal;
    }

	public void setShortEndTags(boolean yes)
	{
		shortendtags = yes;
	}
	
	public boolean shortEndTags()
	{
		return shortendtags;
	}
	
    /**
     * Loads the document from the given URL string.
     * @param urlstr The URL specifying the address of the document.
     * @exception ParseException if the file contains errors.
      * @return No return value.
     */
    public void load(String urlstr) throws ParseException
    {
        URL url;
        try {
            url = new URL( urlstr );
        } catch( MalformedURLException e ) {
            throw new ParseException("MalformedURLException: " + e);
        }        
        load(url);
    }
   
    /**
     * An alias for load and is here for compatibility
     * reasons only. 
     * @param urlstr The URL string.
      * 
      * @return No return value.
      * @exception ParseException if an error occurs while parsing the URL string.
      * 
     */
    public void setURL( String urlstr ) throws ParseException
    {
        load( urlstr);
    }

    /**
     * Loads the document from the given URL.
      * 
     * @param url The URL string.
     * @return No return value.
     * @exception ParseException if a syntax error is found.
     */
    public void load(URL url) throws ParseException
    {
        clear();
        URLdoc = url;
        parser = new Parser();
		parser.setShortEndTags(shortendtags);
        parser.parse(url,this,dtd,this,caseInsensitive,loadExternal);
    }

    /**
     * Loads the document using the given input stream.
     * This is useful if you have the XML data already in memory.
     * @param in The input stream.
     * 
     * @return No return value.
     * @exception ParseException when a syntax error is found.
     */
    public void load(InputStream in) throws ParseException
    {
        clear();
        parser = new Parser();
		parser.setShortEndTags(shortendtags);
        parser.parse(in,this,dtd,this,caseInsensitive,loadExternal);
    }

    /**
     * Returns information about the given parse exception that
     * was generated during the load.
     *  
     * @param e The exception to report on. 
     * @param out The output stream to report to. 
     * @return No return value.
     */
    public void reportError(ParseException e, OutputStream out)
    {
        if (parser != null)
            parser.report(e,out);
    }

    /**
     * Sets the style for writing to an output stream.
     * Use XMLOutputStream.PRETTY or .COMPACT.
     * 
     * @param int The style to set.
     * @return No return value.
     * @see XMLOutputStream
     */
    public void setOutputStyle(int style)
    {
        outputStyle = style;
    }

    /**
     * Retrieves the current output style.  
     * @return the output style. The default style is XMLOutputStream.PRETTY.
     */
    public int getOutputStyle()
    {
        return outputStyle;
    }

    /**
     * Creates an XML output stream matching the format found on <code>load()</code>.
     * @param out The output stream. 
      * 
     * @exception IOException if the output stream cannot be created.
     * @return the XML output stream.
	 */
    public XMLOutputStream createOutputStream(OutputStream out) throws IOException
    {
        XMLOutputStream o = null;

        if( outputEncoding == null )
        {
            // Create an output stream that will save in same format as
            // was found in input stream.

            if (parser != null)
                o = parser.createOutputStream( out );
        }
        if (o == null)
        {
            // Create an output stream that is appropriate for
            // encoding specified in the setEncoding method.
            o = new XMLOutputStream( out );
            String encoding = getEncoding();
            if (encoding != null)
            {
                o.setEncoding( encoding, true, true );
            }
        }
        
        o.setOutputStyle(outputStyle);
        o.dtd = getDTD();
        return o;
    }

    /**
     * Saves the document to the given output stream.
     * @param o  The output stream.
      * @return No return value.
     * @exception IOException if there is a problem saving the output.
     */
    public void save(XMLOutputStream o) throws IOException 
    {
        o.dtd = dtd; // make sure dtd is set correctly.

        for (ElementEnumeration ee = new ElementEnumeration(this); ee.hasMoreElements(); )
        {
            Element e = (Element)ee.nextElement();
            if (e == DTDnode) 
            {
                // Make sure we write the DOCTYPE in correct syntax and in
                // the correct location - relative to surrounding COMMENT
                // and PI elements.
                o.writeChars("<!DOCTYPE ");
                Name docType = getDocType();
                if (docType.getNameSpace() != null)
                    o.writeQualifiedName(getDocType(),Atom.create(""));
                else
                    o.writeChars(docType.getName());

                if (getDTDURL() != null) 
                {
                    if (getId() != null) 
                    {
                        o.writeChars(" PUBLIC ");
                        o.writeQuotedString(getId());
                        o.write(' ');
                    } 
                    else 
                    {
                        o.writeChars(" SYSTEM ");
                    }
                    o.writeQuotedString(getDTDURL());
                } 

                // If we have children then there was an internal
                // subset that also needs to be written out.
                if (e.numElements() > 0) 
                {
                    o.writeChars(" [");
                    o.writeNewLine();
                    o.addIndent(1);
                    o.savingDTD = true;
                    for (ElementEnumeration en = new ElementEnumeration(e); 
                        en.hasMoreElements();)
                    {
                        Element child = (Element)en.nextElement();
                        child.save(o);
                    }
                    o.savingDTD = false;
                    o.addIndent(-1);
                    o.write(']');
                }
                o.write('>');
                o.writeNewLine();
            } 
            else if (e == XML) 
            {
                o.writeChars("<?xml version=\"1.0\""); // version is fixed.
                // The following attributes must be stored in the following
                // order, otherwise the parser complains:
                // ENCODING, RMD
                Name names[] = new Name[2];
                names[0] = nameENCODING;
                names[1] = nameStandalone;
                for (int i = 0; i < names.length; i++)
                {
                    Name n = names[i];
                    Object a = e.getAttribute(n);
                    if (a != null)
                    {
                        o.writeChars(" " + n + "=");
                        o.writeQuotedString(a.toString());
                    }
                }
                o.writeChars("?>");
                o.writeNewLine();
            }
            else
            {
                e.save(o);
            }
        }
    }

    /**
     * Retrieves an enumeration of the element declarations
     * from the DTD. These are returned as <code>ElementDecl</code> objects.
     * @return the element declarations enumeration object. 
     
     */
	public final Enumeration elementDeclarations()
	{
		return (Enumeration)(new ElementDeclEnumeration(dtd.elementDeclarations()));
	}

    /**
     * Returns the XML-DATA specification for the named element.
     * This is DTD information represented in an XML Object Model.
     * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">Specification for XML-Data</a> for details.
     */
    public Element getElementDecl( Name name )
    {
        ElementDecl ed = dtd.findElementDecl(name);
        if (ed == null) return null;
        return ed.toSchema();
    }

    /**
     * Returns the XML-DATA specification for the named entity.
     * This is DTD information represented in an XML Object Model.
     * See <a href="http://www.microsoft.com/standards/xml/xmldata.htm">Specification for XML-Data</a> for details.
     */
    public Element findEntity( Name name )
    {
        Entity ent = dtd.findEntity(name);
        if (ent == null) return null;
        return ent.toSchema();
    }

    /**
     * Sets the Document back to its initial empty state retaining 
     * only the <code>ElementFactory</code> association.
      * @return No return value.
     */
    public void clear()
    {
        DTDnode = null;
        dtd = new DTD(); // use DTD clear when reusing a DTD
        root = null;
        XML = null;
        URLdoc = null;
        outputEncoding = null;
        super.clear();
    }

    /**
     * Retrieves the document's DTD. 
     * @return the DTD.
     */

    public DTD getDTD()
    {
        return dtd;
    }

    /** 
     * URL if passed in.
     */
    URL URLdoc;

    /**
     * Root of External DTD tree.
     */
    Element DTDnode;

    /**
     * Root of element tree.
     */
    Element root;

    /**
     * XML declaration
     */
    Element XML;
    
    /**
     * The Document Type Definition (DTD). 
     */    
    protected DTD dtd;

    /**
     * character encoding of output.
     */
    String outputEncoding;
	
	boolean shortendtags;

    /**
     * The factory used to create the elements in the document.
     */
    protected ElementFactory factory;

    Parser parser;
    int outputStyle;
    boolean caseInsensitive;
    boolean loadExternal;

    static Name nameVERSION = Name.create("version");
    static Name nameENCODING = Name.create("encoding");
    static Name nameStandalone = Name.create("standalone");
    static Name nameDOCTYPE = Name.create("DOCTYPE");
    static Name nameNAME = Name.create("NAME");
    static Name nameURL = Name.create("URL");
    static Name namePUBLICID = Name.create("PUBLICID");
    static Name nameXML = Name.create("xml");
    static String defaultEncoding = "UTF-8";
    static String defaultVersion = "1.0";
}

