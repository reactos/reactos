/*
 * @(#)Parser.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.Element;
import com.ms.xml.om.ElementImpl;
import com.ms.xml.om.ElementFactory;
import com.ms.xml.om.ElementFactoryImpl;
import com.ms.xml.util.EnumWrapper;
import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLInputStream;
import com.ms.xml.util.XMLOutputStream;

import java.lang.String;
import java.util.Hashtable;
import java.util.Stack;
import java.util.Enumeration;
import java.util.Vector;
import java.io.*;
import java.net.*;

/**
 * This class implements an eXtensible Markup Language (XML) parser according to the 
 * latest World Wide Web Consortium (W3C) working draft of the XML specification. 
 * This parser class is used internally by the XML document
 * load method, so you shouldn't need to use it directly.
 * @version 1.0, 6/3/97
 */
public class Parser
{    
    static final int TAGSTART       = '<';
    static final int TAGEND         = '>';
    static final int SLASH          = '/';
    static final int EQ             = '=';
    static final int LPAREN         = '(';
    static final int RPAREN         = ')';
    static final int BANG           = '!';
    static final int QMARK          = '?';
    static final int DASH           = '-';
    static final int PERCENT        = '%';
    static final int AMP            = '&';
    static final int LEFTSQB        = '[';
    static final int RIGHTSQB       = ']';
    static final int QUOTE          = '\'';
    static final int OR             = '|';
    static final int ASTERISK       = '*';
    static final int PLUS           = '+';
    static final int HASH           = '#';
    static final int COMMA          = ',';
    static final int INVALIDTOKEN   = 0;
    static final int EOF            = -1;
    static final int WHITESPACE     = -2;
    static final int WORDCHAR       = -3;   
    static final int NAME           = -4;
    static final int TEXT           = -5;
    static final int PITAGSTART     = -6;
    static final int PITAGEND       = -7;
    static final int DECLTAGSTART   = -8;
    static final int CLOSETAGSTART  = -9;
    static final int EMPTYTAGEND    = -10;
    static final int COMMENT        = -11;
    static final int DOCTYPE        = -12;
    static final int SYSTEM         = -13;
    static final int CDATATAGSTART  = -14;
    static final int ELEMENT        = -15;
    static final int EMPTY          = -16;
    static final int ANY            = -17;
    static final int PCDATA         = -18;
    static final int ATTLIST        = -19;
    static final int CDATA          = -20;
    static final int ID             = -21;
    static final int IDREF          = -22;
    static final int IDREFS         = -23;
    static final int ENTITY         = -24;
    static final int ENTITIES       = -25;
    static final int NMTOKEN        = -26;
    static final int NMTOKENS       = -27;
    static final int NOTATION       = -28;
    static final int ENUMERATION    = -29;
    static final int FIXED          = -30;
    static final int REQUIRED       = -31;
    static final int IMPLIED        = -32;
    static final int NDATA          = -33;
    static final int INCLUDETAGSTART= -34;
    static final int IGNORETAGSTART = -35;
    static final int NAMESPACE      = -36;
    static final int EXTENDS        = -37;
    static final int IMPLEMENTS     = -38;
    static final int XML            = -39;
    static final int VERSION        = -40;
    static final int ENCODING       = -41;
    static final int STANDALONE     = -42;
    static final int CDEND          = -43;
    static final int PUBLIC         = -100;


    /**
     * Creates a new parser object.
     */ 
    public Parser() 
    {
		jdk11 = true;
        caseInsensitive = false;
		shortendtags = false;
    }
	
	public void setShortEndTags(boolean shortendtags)
	{
		this.shortendtags = shortendtags;
	}
        
    /**
     * Parses the XML document pointed to by the given URL and
     * creates the corresponding XML document hierarchy.
     * @param url the url points to the XML document to parse.
     * @param factory used to create XML Elements during parsing.
     * @param dtd the object that the parser stores DTD information in.
     * @param root the root node to start with and add children to
     * during parsing.
     * @param loadext whether to load external DTD's and/or entities
     * @exception ParseException if syntax or other error encountered.
     */    
    public final void parse(URL url, ElementFactory factory, DTD dtd, Element root, boolean caseInsensitive, boolean loadExt) throws ParseException
    {
        this.dtd = dtd;
        this.root = root;
        this.loadexternal = loadExt;
        setURL(url);
        setFactory(factory);
        this.caseInsensitive = caseInsensitive;
        safeParse();
    }

    final void safeParse() throws ParseException
    {
        try {
            parseDocument();
        } catch (ParseException e)
        {
            if (xmlIn != null)
            {
                try {
                    xmlIn.close();
                } catch (Exception f)
                {
                }
            }
            throw e;
        }
        try {
            xmlIn.close();
        } catch (Exception f)
        {
        }
    }

    /**
     * Parses the XML from given input stream.
     * @param in the input stream containing XML data to parse.
     * @param factory used to create XML Elements during parsing.
     * @param dtd the object that the parser stores DTD information in.
     * @param root the root node to start with and add children to
     * during parsing.
     * @exception ParseException if syntax or other error encountered.
     */
    final public void parse(InputStream in, ElementFactory factory, DTD dtd, Element root, boolean caseInsensitive, boolean loadExt) throws ParseException
    {
        this.dtd = dtd;
        url = null;
        this.root = root;
        this.loadexternal = loadExt;
        setInputStream(in);
        setFactory(factory);
        this.caseInsensitive = caseInsensitive;
        safeParse();
    }

    /**
     * Reports errors to the specified output stream including parsing
     * context and stack trace where the error occurred.
     * @param e The exception to report.
     * @param out The output stream to write the report to.
     * @return No return value.
     */
    final public void report(ParseException e, OutputStream out)
    {
        PrintStream o = new PrintStream(out);
        String s = null;

        o.println(e.getMessage());

        if (e.owner instanceof Parser)
        {
            URL u = ((Parser)e.owner).url;
            if (u != null)
                s = u.toString();
        }
        else if (e.owner instanceof Entity)
        {  
            s = "Parsing <" + ((Entity)e.owner).name + ">";
        }
        else
        {
            s = "Parsing";
        }
        o.println("Location: " + s + "(" + e.line + "," + e.column + ")");
        o.print("Context: ");
        for (int i = 0; i < contextAt; i++)
        {
            Name name = ((Context)contexts.elementAt(i)).e.getTagName();
            if (name != null)
                o.print("<" + name + ">");
        }
        o.print("<");
        if (current != null) o.print(current.e.getTagName());
        o.println(">");
    }

    /**
     * Creates an output stream that best matches the XML data format
     * found during parsing. 
     * For example, this will match big endian or little endian
     * XML data formats.
     * @param out  The output stream.
     * @return an <code>XMLOutputStream</code> object that uses the newline 
     * separator
     * defined by the system property "line.separator". 
     */
    public final XMLOutputStream createOutputStream(OutputStream out)
    {
        if (xmlIn != null)
            return xmlIn.createOutputStream(out);
        return null;
    }

    /**
     * throw error
     */                                 
    final void error(String s) throws ParseException
    {
        int i = 1;
        // BUGBUG: the position may still be incorrect
        if (token == NAME)
            i = name.toString().length(); 
        throw new ParseException(s, reader.line, reader.column - 1 - i, reader.owner);
    }
        
        
    /**
     * get next char and update line number / char position
     */    
    final void advance() throws ParseException
    {
        lookahead = reader.read();
        // if EOF and reading 'included' entity pop it...
        while (lookahead == -1 && reader.owner != this)
        {   
            // For external text entities there may be some PCDATA
            // left over that needs to be added also.
            if (charAt != 0) 
            {
                addPCDATA();
            }
            reader = reader.prev;            
            pop(); // pop the entity element
            if (! inTag)
                charAt = 0;
            lookahead = reader.read();
        }
    }
            
    /**
     * return next token
     * @exception ParseException when syntax or other error is encountered.
     */    
    final int nextToken() throws ParseException
    {
        bufAt = 0;
        int bufStart = bufAt;
        if (inTag || ! current.preserveWS) 
        {
            while (isWhiteSpaceChar((char)lookahead))
            {
                if (! inTag) 
                {
                    buf[bufAt++] = (char)lookahead;
                    seenWS = true;
                }
                advance();
            }
        }
        if (inTag)
        {
            switch (lookahead)
            {
                case -1:
                    token = EOF;
                    break;
                case '>':
                    token = TAGEND;
                    inTag = false;
                    advance();
                    break;
                case '/':
                    advance();
                    if (lookahead == '>')
                    {
                        token = EMPTYTAGEND;
                        inTag = false;
                        advance();
                    }
                    break;
                case '?':
                    advance();
                    if (current.type == Element.ELEMENTDECL)
                    {
                        token = QMARK;
                    }
                    else {
                        if (lookahead == '>')
                        {
                            token = PITAGEND;
                            inTag = false;
                            advance();
                        }
                        else
                        {
                            token = QMARK;
                        }
                    }
                    break;
                case '=':
                case '(':
                case ')':
                case ',':
                case '|':
                case '[':
                case ']':
                case '*':
                case '+':
                case '#':
                    token = lookahead;
                    advance();
                    break;
                case '%':
                    advance();
                    if (substitution > 0 && isNameChar((char)lookahead))
                    {
                        scanEntityRef(true);
                        return nextToken();
                    }
                    token = PERCENT;
                    break;
                case '\"':
                case '\'':
                    quote = (char)lookahead;
                    token = QUOTE;
                    advance();
                    break;
                default:
                    if (isNameChar((char)lookahead) || nameSpaceSeparator == (char)lookahead)
                    {
                        scanName("name");

                        if (keyword > 0)
                        {
                            token = lookup(name.getName());
                        }
                    }
                    else
                    {
                        error("Unexpected token '" + (char)lookahead + "' inside tag <" + current.e.getTagName() + ">");
                    }
            }
        }
        else
        {
            if (seenWS && ! current.lastWasWS && 
                (lookahead == -1 || lookahead == '<') ) {
                addNewElement( Element.WHITESPACE , null, false, 
                    new String(buf, bufStart, bufAt - bufStart));
            }
            switch (lookahead)
            {
                case -1:
                    token = EOF;
                    break;
                case '<':
                    inTag = true;
                    seenWS = false; // reset
                    advance();
                    switch (lookahead)
                    {
                        case '?':
                            token = PITAGSTART;
                            advance();
                            break;
                        case '!':
                            token = DECLTAGSTART;
                            advance();
                            if (lookahead == '-')
                            {
                                advance();
                                if (lookahead == '-')
                                {
                                    token = COMMENT;
                                    advance();
                                }
                                else
                                {
                                    error("Bad comment start syntax.  Expected '<!--'");
                                }
                            }
                            else if (lookahead == '[')
                            {
                                advance();
                                boolean ref = false;   // whether the keyword is a parameter entity reference
                                if (lookahead == '%')
                                {
                                    advance();
                                    Entity n = scanEntityRef(true);
                                    conditionRef = n.getName();
                                    ref = true;       // entity reference encountered
                                }
                                else 
                                {
                                    conditionRef = null;
                                }

                                parseKeyword(0, "CDATA or Conditional section start tag");

                                if (token == INCLUDETAGSTART || token == IGNORETAGSTART 
                                    || token == CDATA && !ref)
                                {
                                    // scanned <![CDATA
                                    if (token == CDATA)
                                        token = CDATATAGSTART;
                                    else
                                        inTag = false;

                                    if (lookahead == '[')
                                    {
                                        advance();
                                    }
                                    else 
                                    {
                                        if (token == CDATATAGSTART)
                                            error("Bad CDATA start syntax. Expected '['");
                                        else
                                            error("Bad conditional section start syntax. Expected '['");
                                    }
                                }
                                else
                                {
                                    error("Bad start tag: '<!['" + tokenString(token) + token);
                                }
                            }
                            break;
                        case '/':
                            token = CLOSETAGSTART;
                            advance();
                            break;
                        default:
                            token = TAGSTART;
                    }
                    break;
                case ']':
                    // check CDEND "]]>' first
                    advance();
                    if (lookahead == ']')
                    {
                        advance();
                        if (lookahead == TAGEND)
                        {
                            advance();
                            token = CDEND;
                            break;
                        }
                        else 
                        {
                            reader.push((char)lookahead); 
                            reader.push(']');
                            lookahead = ']';
                        }
                    }
                    else
                    {
                        reader.push((char)lookahead);
                        lookahead = ']';
                    }

                    // check end of internal set of DTD ']'
                    if (breakText == lookahead)
                    {
                        advance();
                        token = RIGHTSQB;
                        break;
                    }

                    // fall thru to text otherwise
                default:
                    token = TEXT;
                    break;
            }
        }
        return token;
    }

    final String tokenString(int token)
    {
        return tokenString(token,null);
    }

    final String tokenString(int token, String s)
    {
        switch (token) 
        {            
            case TAGSTART       : return "start tag(<)";
            case TAGEND         : return "tag end(>)";
            case SLASH          : return "/";
            case EQ             : return "=";
            case LPAREN         : return "(";
            case RPAREN         : return ")";
            case BANG           : return "!";
            case QMARK          : return "question mark(?)";
            case DASH           : return "-";
            case PERCENT        : return "percent(%)";
            case AMP            : return "&";
            case LEFTSQB        : return "[";
            case RIGHTSQB       : return "]";
            case QUOTE          : return "quote(' or \")"; 
            case OR             : return "|";
            case ASTERISK       : return "*";
            case PLUS           : return "+";
            case HASH           : return "#";
            case COMMA          : return ",";
            case INVALIDTOKEN   : return "invalidtoken";
            case EOF            : return "EOF";
            case WHITESPACE     : return "whitespace";
            case WORDCHAR       : return "word character";   
            case NAME           : if (s != null) return s;
                                  else return "NAME '" + name + "'";
            case TEXT           : return "TEXT '" + text + "'";
            case PITAGSTART     : return "<?";
            case PITAGEND       : return "?>";
            case CDEND          : return "]]>";
            case DECLTAGSTART   : return "<!";
            case CLOSETAGSTART  : return "</";
            case EMPTYTAGEND    : return "/>";
            case COMMENT        : return "<!--";
            case DOCTYPE        : return "DOCTYPE";
            case SYSTEM         : return "SYSTEM";
            case CDATATAGSTART  : return "<![CDATA";
            case ELEMENT        : return "ELEMENT";
            case EMPTY          : return "EMPTY";
            case ANY            : return "ANY";
            case PCDATA         : return "PCDATA";
            case ATTLIST        : return "ATTLIST";
            case CDATA          : return "CDATA";
            case ID             : return "ID";
            case IDREF          : return "IDREF";
            case IDREFS         : return "IDREFS";
            case ENTITY         : return "ENTITY";
            case ENTITIES       : return "ENTITIES";
            case NMTOKEN        : return "NMTOKEN";
            case NMTOKENS       : return "NMTOKENS";
            case NOTATION       : return "NOTATION";
            case ENUMERATION    : return "ENUMERATION";
            case FIXED          : return "FIXED";
            case REQUIRED       : return "REQUIRED";
            case IMPLIED        : return "IMPLIED";
            case NDATA          : return "NDATA";
            case INCLUDETAGSTART: return "INCLUDETAGSTART";
            case IGNORETAGSTART : return "IGNORETAGSTART";
            case NAMESPACE      : return "NAMESPACE";
            case PUBLIC         : return "PUBLIC";
    
            default:            return s;
        }
    }
    
    final int lookup(String n)
    {
        Object o = tokens.get(n);
        if (o != null)
        {
            token = ((Integer)o).intValue();
        }
        else
        {
            token = NAME;
        }

        return token;
    }
    
    /**
     * return true if character is whitespace
     */    
    static final boolean isWhiteSpaceChar(char c)
    {
        if (c < 256)
        {
            return (chartype[c] & FWHITESPACE) != 0;
        }
        else
        {
            if (jdk11)
                return Character.isWhitespace(c);

            return Character.isSpace(c);    
        }
    }

    /**
     * return true if character can be part of a name
     */    
    static final boolean isNameChar(char c)
    {
        if (c < 256)
        {
            return (chartype[c] & (FLETTER | FDIGIT | FMISCNAME)) != 0;
        }
        else
        {
            return  Character.isLetter(c) || 
                    Character.isDigit(c) ||
                    c == '-' ||  
                    c == '_' ||
                    c == '.';
        }
        // BUGBUG:: according to spec, should allow combiningChar, Ignorable, and Extender chars as well
    }

    /**
     * Scan a name
     */    
    final void scanName(String s) throws ParseException
    {
        String n = null;
        Atom ns = null;
        boolean scanned = false;
        
        if (nameappend == 0)
        {
            bufAt = 0;
        }
        int bufStart = bufAt;
        if (nameSpaceSeparator != (char)lookahead)
        {
            bufAt = scanSimpleName(bufAt, s);
            scanned = true;
        }

        if (nametoken == 0 && simplename == 0 && nameSpaceSeparator == (char)lookahead) 
        {
            int nsgap = 1;
            bufAt++;
            advance();
            // Make second colon optional.
            if (nameSpaceSeparator == (char)lookahead) 
            { 
                nsgap++;
                bufAt++;
                advance();
            }
            if (scanned) // name space found
            {
                if (caseInsensitive)
                     n = new String(buf, bufStart, bufAt - bufStart - nsgap).toUpperCase();
                else n = new String(buf, bufStart, bufAt - bufStart - nsgap);
                Atom atom = Atom.create(n);
                if (DTD.isReservedNameSpace(atom)) 
                {
                    ns = atom;
                } else {
                    // check the current context first 
                    ns = current.findNameSpace(atom);
                    if (ns == null)
                    {
                        // check the global name space when the name space is not defined in current context
                        ns = dtd.findLongNameSpace(atom);
                        if (ns == null) 
                        {
//                          error("Name space not defined '" + n + "'");
                            ns = atom;
                            addNameSpace(ns,ns,false);
                        }
                    }
                }
            }
            bufStart = bufAt;
            bufAt = scanSimpleName(bufAt, s);
//            else 
//            {// report error and throw
//                error("Expecting namespace separator ':' instead of '" + (char)lookahead + "'");
//            }
        }    
        else 
        {
            if ((nametoken == 0 && simplename == 0) || inEntityRef > 0)
                ns = current.defaultNameSpace;
        }

        if ((nametoken == 0 && simplename == 0) || inEntityRef > 0)
            current.nameSpace = ns;
        if ((keyword > 0 && inEntityRef == 0) || ns == null) 
            name = Name.create(buf, bufStart, bufAt - bufStart);
        else 
            name = Name.create(new String(buf, bufStart, bufAt - bufStart), ns);        
        token = NAME; 
    }

    final char toUpperCase(char c)
    {
        if (nouppercase != 0)
            return c;
        else
        {
            if (c < 256)
            {
                return charupper[c];
            }
            else
            {
                return Character.toUpperCase(c);
            }
        }
    }

    final int scanSimpleName(int bufAt, String s) throws ParseException
    {
        boolean startname;

        if (lookahead < 256)
        {
            startname = (chartype[lookahead] & (FLETTER | FSTARTNAME)) != 0;
        }
        else
        {
            startname = Character.isLetter((char)lookahead) || lookahead == '_';
        }
        if (!startname)
        {
            if (!(nametoken > 0 && Character.isDigit((char)lookahead)))
            {
                // report error and throw
                error("Expecting " + s + " instead of '" + (char)lookahead + "'");
            }
        }
        // This is a flattened loop, to optimize loop speed.
        // Rather than doing a complex condition on each loop
        // iteration, the loop is duplicated 4 times for each
        // different condition.  This is a deliberate code-size
        // versus speed trade off since this loop is very critical
        if (nametoken == 0 && simplename == 0)
        {
            if (caseInsensitive)
            {
                buf[bufAt++]=toUpperCase((char)lookahead);
                advance();
                while (isNameChar((char)lookahead))
                {
                    buf[bufAt++] = toUpperCase((char)lookahead);
                    advance();
                }
            }
            else
            {
                buf[bufAt++]=(char)lookahead;
                advance();
                while (isNameChar((char)lookahead))
                {
                    buf[bufAt++] = (char)lookahead;
                    advance();
                }
            }
        } else {
            if (caseInsensitive)
            {
                buf[bufAt++]=toUpperCase((char)lookahead);
                advance();
                while (isNameChar((char)lookahead) ||
                    lookahead == nameSpaceSeparator )
                {
                    buf[bufAt++] = toUpperCase((char)lookahead);
                    advance();
                }
            }
            else
            {
                buf[bufAt++]=(char)lookahead;
                advance();
                while (isNameChar((char)lookahead) ||
                    lookahead == nameSpaceSeparator )
                {
                    buf[bufAt++] = (char)lookahead;
                    advance();
                }
            }
        }
        return bufAt;
    }

    /**
     * Scan text into a string
     */    
    final void scanText(int eref, int cref, boolean stripws) throws ParseException
    {
        charAt = 0;

        while (lookahead != -1 && lookahead != '<' && lookahead != breakText && (charAt + 1) < chars.length)
        {
            if (lookahead == cref)
            {
                if (seenWS) { chars[charAt++] = ' '; seenWS = false; }
                advance();
                if (lookahead == '#')
                {
                    scanCharRef();
                } 
                else if (isNameChar((char)lookahead)) 
                {
                    scanEntityRef(false);
                }
                else
                {
                    chars[charAt++] = (char)cref;
                }
            }
            else if (lookahead == eref)
            {
                if (seenWS) 
                { 
                    chars[charAt++] = ' '; seenWS = false; 
                }
                advance();
                if (isNameChar((char)lookahead))
                {
                    scanEntityRef(eref == '%');
                }
                else
                {
                    chars[charAt++] = (char)cref;
                }
            }
            else
            {
                if (stripws && isWhiteSpaceChar((char)lookahead))
                {
                    seenWS = true;
                }
                else
                {
                    if (seenWS) 
                    { 
                        chars[charAt++] = ' '; seenWS = false; 
                    }
                    chars[charAt++] = (char)lookahead;
                }
                advance();
            }
        }
        token = TEXT;
    }

    /** 
     * Process entity reference
     * If successful lookahead contains the next character after the entity.
     * @param par if this is a parametrized entity.
     */
    final Entity scanEntityRef(boolean par) throws ParseException
    {
        nouppercase++;
        inEntityRef++;
        scanName("entity ref");
        inEntityRef--;
        nouppercase--;
        if (lookahead != ';')
        {
            error("Entity reference syntax error " + name);
        }
        Entity en = dtd.findEntity(name);
        if (en == null)
        {
            String msg = "Missing entity '" + name + "'.";
            if (! loadexternal) 
            {
                msg += "\nPerhaps you need to change the loadexternal flag to 'true'.";
            }
            error(msg);
        }

        if (par != en.par)
        {
            if (par)
                error("Entity '" + name + "' is not a parameter entity.");
            else error("Entity '" + name + "' is a parameter entity.");
        }

        if (par) // in DTD
        {
            // BUG: We still haven't finished this yet.  
            // In the case where a parameter entity is used
            // inside an element declaration, for example,
            // we will forget this fact in the saved document. 

            if (! inTag) // add an entity reference node
            {
                addNewElement(Element.ENTITYREF, name, false, null);
            }
            if (en.getURL() == null) // parse entity text
            {
                push(en,name,Element.ENTITY);
                reader = en.getReader(reader);              
            }
            else // get external DTD
            {
                if (en.ndata != null)
                    error("Binary parameter entity " + name.toString() + "cannot be used in DTD");

                if (loadexternal)
                    loadDTD(en.getURL(), current.defaultNameSpace);
            }   
        }
        else  // in document 
        {
            // NOTE: in the following code, we pass the ElementDecl from
            // the current context to the new context, since we want to
            // validate the contents of the ENTITY relative to the parent
            // of the ENTITYREF.

            if (! inTag)
            {
                // Put current substring in new PCDATA node.
                addPCDATA();
                charAt = 0;
                // Now we need to store a special ENTITYREF
                // node in the tree to remember where the entities
                // are so we can save them back out.
                addNewElement(Element.ENTITYREF, name,true, null);
            }
            if (en.getLength() == -1) // special characters
            {
                // skip it since the special built in entities
                    // do not require parsing.
            }
            else {
                if (! en.parsed) 
                {
                    Context c = current;
                    if (c.parent != null)
                        c = c.parent;
                    if (en.getURL() == null) // parse entity text
                    {
                        en.parsed = true;
                        push(en,name,Element.ENTITY);
                        // here we have to link the entity context to the parent
                        // element context so we can do the validation correctly.
                        // See comments in addNewElement.
                        current.parent = c;
                        reader = en.getReader(reader);
                    }
                    else if (en.ndata == null && loadexternal)
                    {
                        en.parsed = true;
                        push(en, name,Element.ENTITY);
                        // here we have to link the entity context to the parent
                        // element context so we can do the validation correctly.
                        // See comments in addNewElement.
                        current.parent = c;
                        try 
                        {
                            URL u = new URL(url,en.getURL());
                            reader = new EntityReader(
                                u.openStream(),
                                reader.line, reader.column, reader, en);
                        } 
                        catch (Exception e) 
                        {
                            error("Cannot load external text entity: " + name.toString());
                        }
                    }
                }
            }
        }
        advance();
        return en;
    }

    /**
     *  Hax ::= [0-9a-fA-F]
     *  CharRef ::=  '&#' [0-9]+ ';' | '&#x' Hex+ ';'
     */
    final void scanCharRef() throws ParseException
    {
        int n = 0;

        if (lookahead == '#')
        {
            advance();
            if (lookahead == 'x' || lookahead == 'X') {
                advance();
                while (true)
                {
                    if (lookahead >= '0' && lookahead <= '9')
                    {
                        n = n * 16 + lookahead - '0';
                    }
                    else if (lookahead >= 'a' && lookahead <= 'f')
                    {
                        n = n * 16 + lookahead - 'a' + 10;
                    }
                    else if (lookahead >= 'A' && lookahead <= 'F')
                    {
                        n = n * 16 + lookahead - 'A' + 10;
                    }
                    else
                    {
                        break;
                    }
                    advance();
                }
            } else while (lookahead >= '0' && lookahead <= '9') {
                n = n * 10 + lookahead - '0';
                advance();
            }
        }

        if (lookahead != ';')
        {
            error("Bad character reference syntax. Expecting &#xx;");
        }
        else
        {
            chars[charAt++] = (char)n;
        }
        advance();
    }
    
    /**
     * Scan a quoted string
     *
     * @param q match the end
     * @param eref match entity reference start char (& or %) if not 0xffff
     * @param cref match character refence char (&# or &u) if not 0xffff
     */    
    final void scanString(int endChar, int eref, int cref, int breakChar) throws ParseException
    {
        charAt = 0;
        while (lookahead != -1 && lookahead != endChar)
        {
            if (lookahead == breakChar)
            {
                error("Illegal character in string " + (char)lookahead);
            }
            else if (lookahead == cref)
            {
                advance();
                if (lookahead == '#')
                {
                    scanCharRef();
                } 
                else if (isNameChar((char)lookahead)) 
                { 
                    if (expandNamedEntities) 
                    {
                        scanEntityRef(false);
                    } else {
                        chars[charAt++] = (char)cref;
                    }
                } 
                else 
                {
                    chars[charAt++] = (char)lookahead;
                }
            }
            else if (lookahead == eref)
            {
                advance();
                if (isNameChar((char)lookahead)) 
                {
                    boolean par = (eref == '%');
                    if (expandNamedEntities)  // par || 
                    {
                        scanEntityRef(par);
                    } else {
                        chars[charAt++] = (char)eref;
                    }
                } 
                else 
                {
                    chars[charAt++] = (char)lookahead;
                }
            }
            else
            {
                chars[charAt++] = (char)lookahead;
                advance();
            }
        }
        if (lookahead == endChar)
        {
            advance();
        }
        else
        {
            error("Unterminated string");
        }
        text = new String(chars, 0, charAt);
    }
    
    /**
     * scan URL string
     */    
    final String scanUrl() throws ParseException
    {
        parseToken(QUOTE, "Url");
        scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
        return text;
    }
        
    /**
     * expect token type t
     */    
    final void parseToken(int t, String s) throws ParseException
    {
        if (nextToken() != t)
        {
            error("Expected " + tokenString(t,s) + " instead of " + tokenString(token));
        }
    }

    /**
     * expect token type t
     */    
    final void checkToken(int t, String s) throws ParseException
    {
        if (token != t)
        {
            error("Expected " + tokenString(t,s) + " instead of " + tokenString(token));
        }
    }
    
    
    /**
     * expect keyword k
     */    
    final void parseKeyword(int k, String s) throws ParseException
    {
        keyword++;
        if (k == 0)
        {
            nextToken();
        }
        else
        {
            parseToken(k, s);
        }
        keyword--;        
    }

    /**
     * Parse names separated by given token and return the number of names
     * found.  Also, if the value argument is not null it returns the names
     * concatenated with a ' ' separator in this string object.
     */
    final int parseNames(Vector names, int separator, StringBuffer value) throws ParseException
    {
        int i = 0;
        bufAt = 0;
        if (value != null) nameappend++;
        while (nextToken() == NAME)
        {
            // Insert space to separate names for the returned
            // concatenated String value.
            if (i > 0)
            {
                buf[bufAt++] = ' ';
            }
            
            names.addElement(name);
            i++;
            if (separator != INVALIDTOKEN && nextToken() != OR)
                break;
        }
        if (value != null) {
            value.append(buf, 0, bufAt);
            nameappend--;
        }
        return i;
    }
    
    /**
     * expect XML document
     *
     * Document ::= Prolog Element Misc*
     */    
    final void parseDocument() throws ParseException
    {
        expandNamedEntities = true;
        internalSubset = false;
        seenWS = false;
        contextAt = 0;
        standAlone = false;
        validating = false;


        // The default is to strip white space.  
        // This default is also assumed in Document.save().
        // So changing the default would also require some changes
        // to the implementation of Document.save().
        newContext(root, null, Element.ELEMENT, false, null, null);
        parseProlog();
        parseRootElement();
        
        if (lookahead != -1)
        {
            nextToken();
            tryMisc();
            if (lookahead != -1)
                error("Expected comments, PI, or EOF instead of " + tokenString(token));
        }
        dtd.checkIDs();
    }
    
    void newContext(Element e, Name n, int type, boolean preserveWS, Atom nameSpace, Hashtable spaceTable)
    {
        if (contextAt == contexts.size())
        {
            current = new Context(e, n, type, preserveWS, nameSpace, spaceTable);
            contexts.addElement(current);
        }
        else
        {
            current = (Context)contexts.elementAt(contextAt);
            current.reset(e, n, type, preserveWS, nameSpace, spaceTable);
        }
    }

    final void push(Element e, Name n, int type)
    {
        contextAt++;
        // White space handling is inherited.
        newContext(e, n, type, current.preserveWS, current.nameSpace, current.spaceTable);
        return;
    }
    
    final void pop()
    {
        current = (Context)contexts.elementAt(--contextAt);
    }
    
    /**
     * expect prolog
     *
     * Prolog ::= XMLDecl Misc* DocTypeDecl? Misc*
     */    
    final void parseProlog() throws ParseException
    {
        if (lookahead != -1)
        {
            nextToken();
            if (token == PITAGSTART)
            {
                parseToken(NAME, "PI tag");
                if (name == nameXML)
                {
// 10/21/97 CJL - Disabled this check for CDF compatibility.
//                    if (current.lastWasWS)
//                        error("An XML declaration can only appear in the very beginning of the document.");
                    parseXMLDecl();
                }
                else  
                {
                    if (name == nameXMLNameSpace)
                        parseNameSpaceDecl(true, true);
                    else finishPI();
                }
                nextToken();
                firstLine = false;
            }           
        }
        tryMisc();
        tryDocTypeDecl();
        tryMisc();
    }


    /**
     * expect XML declaration
     *
     * XMLDecl = '<?XML' VersionInfo EncodingDecl? SDDecl? S? '?>'
     */
    final void parseXMLDecl() throws ParseException
    {
        Element xml = addNewElement(Element.PI, name,false, null);
        push(xml, name, Element.PI); // so that error reporting is correct.
        ElementDecl ed = current.ed;
        current.ed = XMLDecl;

        // check version information
        parseKeyword(VERSION, nameVERSION.toString());
        parseToken(EQ,"=");
        parseToken(QUOTE, "string");
        scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
        if (!text.equals("1.0"))
            error("Expected version 1.0 instead of " + text);
        factory.parsedAttribute(xml,nameVERSION, text);

        // check encoding information
        parseKeyword(0, "encoding or standalone");
        String encoding = null;
        if (token == ENCODING) {
            parseToken(EQ,"=");
            parseToken(QUOTE, "string");
            scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
            factory.parsedAttribute(xml,nameENCODING, text);
            encoding = text;
            parseKeyword(0, nameStandalone.toString());
        }

        // check Standalone
        if (token == STANDALONE) {
            parseToken(EQ,"=");
            parseToken(QUOTE, "string");
            scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
            if (caseInsensitive)
                text = text.toUpperCase();
            Name value = Name.create(text);
            if (value == nameYes)
            {
                standAlone = true;
            } 
            else if (value == nameNo)
            {
                standAlone = false;
            }
            else
            {
                error("Expected 'yes' or 'no' instead of " + value);
            }
            factory.parsedAttribute(xml, nameStandalone, value.toString());
            nextToken();
        }
        if (encoding != null)
        {
            try {
                xmlIn.setEncoding(encoding);
            } catch (IOException e) {
                error("Unsupported XML encoding: \"" + encoding + "\"" +
                       "\nLow level error: " + e.getMessage());
            }
        }
        if (token != PITAGEND) 
            error("Expected " + tokenString(PITAGEND) + " instead of " + tokenString(token));
        current.ed = ed;
        pop();
    }

    /**
     * check for misc elements
     *
     * Misc ::= Comment | PI | S
     */
    final void tryMisc() throws ParseException
    {
        for (;;)
        {
            switch (token)
            {
                case PITAGSTART:
                    parsePI(true);
                    break;
                case COMMENT:
                    parseComment();
                    break;
                default:
                    return;
            }
            if (lookahead != -1)
            {
                nextToken();
            }
            else
            {
                token = EOF;
                break;
            }
            firstLine = false;
        }
    }
     
    /**
     * check for document type declaration
     *
     * DocTypeDecl ::=  '<!DOCTYPE' S Name (S ExternalID)? S? ('[' internalsubset* ']' S?)? '>'
     */
    final void tryDocTypeDecl() throws ParseException
    {
        if (token == DECLTAGSTART)
        {
            firstLine = false;
            parseKeyword(DOCTYPE, "Doctype");
            Element dtdElement = addNewElement( Element.DTD, nameDOCTYPE, false, null);
            parseToken(NAME, "Doctype name");
            docType = name;
            dtd.docType = name;
            factory.parsedAttribute(dtdElement,nameNAME,docType);
            parseKeyword(0, "ExternalID");
            String url = null;
            switch(token)
            {
                case SYSTEM:
                    url = scanUrl();
                    factory.parsedAttribute(dtdElement,nameURL, url);
                    nextToken();
                    break;
                case PUBLIC:
                    parseKeyword(0, "String");
                    if (token == QUOTE) {
                        expandNamedEntities = false;
                        scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
                        expandNamedEntities = true;
                        factory.parsedAttribute(dtdElement,namePUBLICID,text);                         
                    }
                    else 
                        error("Expected " + tokenString(QUOTE) + " instead of " + tokenString(token));
                    url = scanUrl();
                    factory.parsedAttribute(dtdElement,nameURL, url);
                    nextToken();
                    break;
            }
            if (token == LEFTSQB)
            {
                // set up  context for [...]
                inTag = false;
                breakText = ']';
                internalSubset = true;
                // Push the dtd element so we can add to it.
                push(dtdElement, nameDOCTYPE, Element.DTD);
                parseInternalSubset();
                if (token != RIGHTSQB)
                {
                    error("Expected " + tokenString(RIGHTSQB));
                }
                // restore context 
                inTag = true;
                internalSubset = false;
                breakText = 0;
                pop();
                nextToken();
            }
            if (url != null && loadexternal)
            {
                loadDTD(url.toString(), null);
            }
            if (token != TAGEND)
            {
                error("Expected " + tokenString(TAGEND) + " instead of " + tokenString(token));
            }
            if (lookahead != -1)
            {
                nextToken();
            }
        }
    }
    

    public final void loadDTD(String urlStr, Atom nameSpace) throws ParseException
    {
        try {
            URL u = new URL(url, urlStr);
            Parser parser = new Parser();
            parser.dtd = this.dtd;
            parser.setURL(u);
            parser.setFactory(factory);
            parser.caseInsensitive = this.caseInsensitive;
            parser.loadexternal = this.loadexternal;
            Element root = factory.createElement(null,Element.ELEMENT, nameDOCTYPE,null);
            parser.newContext(root, null, Element.ELEMENT, false, nameSpace, current.spaceTable);
            parser.parseInternalSubset();
            validating = true;
        } catch (IOException e) {
            error("Couldn't find external DTD '" + urlStr + "'"); 
        }
    }

    final Element addNewElement(int type, Name name, boolean validate, String text) throws ParseException
    {
        // Make sure we don't add multiple whitespace nodes,
        // as would be the case while parsing ATTLISTS
        if (type == Element.WHITESPACE) {
            current.lastWasWS = true;
        } else {
            current.lastWasWS = false;
        }
        Element e = factory.createElement(current.e,type, name, text);

        // Entity has children, so we need to check the contents
        // of the entity, rather than the entityref itself.
        // For example, the following should parse correctly:
        //      <!DOCTYPE doc [
        //      <!ELEMENT doc (foo)>
        //      <!ELEMENT foo EMPTY>
        //      <!ENTITY  bar "<foo/>">
        //      ]>
        //      <doc>&bar;</doc>
        // In order to make this work, we link the entity Context to the
        // parent element so that when we add elements to the entity, we
        // can get back to the parent element for validation.

        if (validate && e != null)
        {
            Context c = current;
            if (current.parent != null)
                c = current.parent;
            if (c.ed != null)
            {
                c.ed.checkContent(c, e, this);
            }
        }
        return e;
    }

    /**
     * Parse DTD content
     */    
    final void parseInternalSubset() throws ParseException
    {
        substitution++;
        validating = true;  // we have a dtd then, so we should do validation.

        for (;;)
        {
            switch(nextToken())
            {
                case TEXT:
                    if (lookahead == '%') { // entity reference
                        advance();
                        scanEntityRef(true);
                    }
                    else 
                    {
                        error("Unexpected text in DTD.");
                        return;
                    }
                    break;
                case RIGHTSQB:
                    if (!internalSubset)
                        System.out.println("Illegal token in DTD: " + token);
                case EOF:
                    substitution--;
                    return;
                case PITAGSTART:
                    parsePI(true);
                    break;
                case COMMENT:
                    parseComment();
                    break;
                case DECLTAGSTART:
                    parseKeyword(0, "ENTITY|...");
                    switch (token)
                    {
                        case ENTITY:
                            parseEntityDecl();
                            break;
                        case ELEMENT:
                            parseElementDecl();
                            break;
                        case ATTLIST:
                            parseAttListDecl();
                            break;
                        case NOTATION:
                            parseNotation();
                            break;
                        default:
                            error("Unknown DTD keyword " + name);
                    }
                    break;
                case INCLUDETAGSTART:  
                    parseIncludeSection();
                    break;
                case IGNORETAGSTART:
                    parseIgnoreSection();
                    break;
                default:  
                    return;
            }
        }
    }
    
    /**
     * Parses conditional section with keyword INCLUDE
     */
    final void parseIncludeSection() throws ParseException
    {
        Element e = addNewElement(Element.INCLUDESECTION, conditionRef, false, null);
        push(e,conditionRef,Element.INCLUDESECTION);
        parseInternalSubset();
        pop();
    }

    /**
     * Parses conditional section with keyword IGNORE
     */
    final void parseIgnoreSection() throws ParseException
    {
        Element e = addNewElement(Element.IGNORESECTION, conditionRef, false, null);
        charAt = 0;
        push(e,conditionRef,Element.IGNORESECTION);
        parseIgnoreSectContent();

        // add contents as PCDATA, which includes the conditional section closing tag ']]>'
        if (charAt > 0)
        {
            addNewElement(Element.PCDATA, null, true, 
                new String(chars, 0, charAt));
            charAt = 0;
        }  
        pop();
    }


    /**
     *  Adds char into the chars buffer
     */
    final void addChar() throws ParseException
    {
        if (lookahead == -1)
            error("Unexpected EOF.");

        chars[charAt++] = (char)lookahead;
        if (charAt == chars.length)
        {
            addNewElement(Element.PCDATA, null, true,
                new String(chars, 0, charAt));
            charAt = 0;     
        }
        advance();
    }

    /**
     *  Checks whether CDEND ']]>' is the next token in Ignore conditional section
     */
    final void checkCDEND(boolean expected) throws ParseException
    {
        boolean yes = false;

        if (lookahead == ']')
        {
            addChar();
            if (lookahead == ']')
            {
                addChar();
                if (lookahead == '>')
                {
                    if (!expected)
                        error("Bad Ignore conditional section syntex. ']]>' is not allowed here.");
                    yes = true;
                    addChar();
                }
            }
        }

        if (!yes && expected)
            error("Bad Ignore conditional section syntex. Expected ']]>'.");
    }

    /**
     * Parses Ignore conditional section contents
     */
    final void parseIgnoreSectContent() throws ParseException
    {
        boolean stop = false;

        while (lookahead != ']')
        {
            switch (lookahead)
            {
                case '\'':  
                case '\"':  // SkipLit
                    int quote = lookahead;
                    addChar();
                    while (lookahead != quote)
                    {
                        checkCDEND(false);
                        addChar();
                    }
                    addChar(); 
                    break;
                case '<':
                    addChar();
                    switch (lookahead)
                    {
                        case '!':
                            addChar();
                            switch (lookahead)
                            {
                                case '-':  // comment
                                    addChar();
                                    if (lookahead != '-')
                                        error("Bad comment syntax. Expected '-'.");
                                    addChar();
                                    while (!stop)
                                    {
                                        if (lookahead == '-')
                                        {
                                            addChar();
                                            if (lookahead == '-')
                                            {
                                                addChar();
                                                if (lookahead == '>')
                                                {
                                                    addChar();
                                                    stop = true;
                                                }
                                                else
                                                {
                                                    error("Bad comment syntax. Expected '>'.");         
                                                }
                                            }
                                        }
                                        else
                                        {
                                            addChar();
                                        }
                                    }
                                    stop = false;
                                    break;
                                case '[':  // IgnoreSectContents
                                    addChar();
                                    parseIgnoreSectContent();
                                    break;
                                default:   // Single character
                                    addChar();
                                    break;
                            }
                            break;
                        case '?':  // PI
                            addChar();
                            while (!stop)
                            {
                                if (lookahead == '?')
                                {
                                    addChar();
                                    if (lookahead == '>')
                                    {
                                        addChar();
                                        stop = true;
                                    }
                                }
                                else
                                {
                                    addChar();
                                }
                            }
                            stop = false;
                            break;
                        default:   // Syntax error
                            error("Bad character in IGNORE conditional section.");
                    }
                    break;
                default:
                    addChar();
                    break;
            }
        }

        // checking closing tag ']]>'
        checkCDEND(true);
    }

    /**
     * Parse Entity declaration
     * EntityDecl ::= '<!ENTITY' S Name S EntityDef S? '>'
     *                | '<!ENTITY' S '%' S Name S EntityDef S? '>'
     */    
    final void parseEntityDecl() throws ParseException
    {
        boolean par = false;
        
        nouppercase++;
        if (nextToken() == PERCENT)
        {
            par = true;
            parseToken(NAME, "Entity name");
        }
        else
        {
            if (token != NAME)
            {
                error("Expected entity name instead of " + tokenString(token));
            }
        }
        nouppercase--;

        Entity en = dtd.findEntity(name);
        if (en != null)
        {
            System.err.println("Warning: Entity '" + name + "' already defined, using the first definition."); 
            // soak up the duplicate and throw it away.
            en = new Entity(name, par);
        }
        else
        {
            en = new Entity(name, par);
            dtd.addEntity(en);
            if (internalSubset) {
                // Save this entity in the <!DOCTYPE>
                // Element tree so we can save it back out.
                if (current.e != null)
                    current.e.addChild(en,null);
                current.lastWasWS = false;
            }    
        }

        // push so that we can give correct error message when an error is found in nextToken()
        // this is also necessary for NDATA to have name space
        push(en, name,Element.ENTITY);

        parseKeyword(0, "String or SYSTEM");
        if (token == PUBLIC)
        {
            parseKeyword(0, "String");
            if (token == QUOTE) {
                expandNamedEntities = false;
                scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
                expandNamedEntities = true;
                en.pubid = text; 
                token = SYSTEM; // assure to get url
            }
            else error("Expected " + tokenString(QUOTE) + " instead of " + tokenString(token));
        }

        switch (token)
        {
            case QUOTE:
                {
                    int line = reader.line;
                    int column = reader.column;
                    expandNamedEntities = false;
                    scanString(quote, '%', '&', 0xFFFF);
                    expandNamedEntities = true;
                    en.setText(text);   
                    en.setPosition(line, column);
                }
                nextToken();
                break;
            case SYSTEM:
                en.setURL(scanUrl());
                parseKeyword(0, "ndata");
                if (token == NDATA)
                {
                    parseToken(NAME, "ndata name");
                    Notation notationName = dtd.findNotation(name);
                    if (notationName == null)
                        error("Notation: " + name + " has not been declared yet");
                    en.setNDATA(name);
                    nextToken();
                }
                break;
            default:
                error("Expected " + tokenString(QUOTE) + " or " + tokenString(SYSTEM) + " instead of " + tokenString(token));
        }

        checkToken(TAGEND, ">");
        pop();
    }


    private void addNameSpace(Atom as, Atom href, boolean global) throws ParseException
    {
        if (as == null || href == null) {
            error("Name space syntax error.");
        }
        if (DTD.isReservedNameSpace(as))
            error(as.toString() + " is a reserved name space.");
        if (global) 
        {
            Atom aname = dtd.findShortNameSpace(href);
            if (aname != null)
            {
                if (aname != as)
                    error("Cannot give two short references '" + aname.toString() + "' and '" + as.toString() + "' to the same name space: '" + href.toString() + "'");
            }
            else {
                aname = dtd.findLongNameSpace(as);
                if (aname != null)
                    error("Short reference '" + as.toString() + "' is used by name space '" + aname.toString() + "'");
                dtd.addNameSpace(as, href);
            }
        }
        else current.addNameSpace(as, href);
    }

    /**
     * Parse notation
     * NotationDecl ::= '<!NOTATION' S Name S ExternalID S? '>'
     */    
    final void parseNotation() throws ParseException
    {
        parseToken(NAME, "Notation name");
                  
        Notation no = dtd.findNotation(name);
        if (no != null)
        {
            error("Notation already declared " + name);
        }

        no = new Notation(name);
        dtd.addNotation(no);
        if (internalSubset) {
            // Save this notation in the <!DOCTYPE>
            // Element tree so we can save it back out.
            if (current.e != null)
                current.e.addChild(no,null);
            current.lastWasWS = false;
        }    

        // needed to write correct error message
        push(no, name, Element.NOTATION);

        parseKeyword(0, "SYSTEM or PUBLIC");
        if (token != SYSTEM && token != PUBLIC)
        {
            error("Expected " + tokenString(SYSTEM) + " or " + tokenString(PUBLIC) + " instead of " + tokenString(token));
        }
        no.type = token;

        if (no.type == PUBLIC) {
            parseKeyword(0, "String");
            if (token == QUOTE) {
                expandNamedEntities = false;
                scanString(quote, 0xFFFF, 0xFFFF, 0xFFFF);
                expandNamedEntities = true;
                no.pubid = text; 
            }
            else error("Expected " + tokenString(QUOTE) + " instead of " + tokenString(token));
        }

        no.setURL(scanUrl());   
        
        parseToken(TAGEND, ">");
        pop();
    }

    final ElementDecl createElementDecl(Vector v) throws ParseException
    {

        if (token != NAME)
        {
            error("Expected " + tokenString(NAME) + " instead of " + tokenString(token));
        }
        if (dtd.findElementDecl(name) != null)
        {
            error("Element '" + name + "' already declared.");
        }
        ElementDecl ed = new ElementDecl(name);
        current.lastWasWS = false;
        dtd.addElementDecl(ed);
        v.addElement(ed);
        return ed;
    }
    
    /**
     * Parse element declaration
     */
    final void parseElementDecl() throws ParseException
    {
        Vector v = new Vector();
        ElementDecl ed;

        nextToken();
        ed = createElementDecl(v);
        if (internalSubset) {
            // Save this element in the <!DOCTYPE>
            // Element tree so we can save it back out.
            if (current.e != null)
                current.e.addChild(ed,null);
        }   
        // push to handle name space
        push(ed, name,Element.ELEMENTDECL);
        ed.parseModel(this);
        checkToken(TAGEND, ">");
        pop();
    }
    
    final ElementDecl findElementDecl(Vector v) throws ParseException
    {
        if (token != NAME)
        {
            error("Expected " + tokenString(NAME) + " instead of " + tokenString(token));
        }
        ElementDecl ed = dtd.findElementDecl(name);

        if (ed == null)
        {
            error("Missing Element declaration '" + name + "'");
        }
        v.addElement(ed);
        return ed;
    }
    
    /**
     * Parse element declaration
     * AttlistDecl ::= '<!ATTLIST' S Name AttDef+ S? '>'
     */
    final void parseAttListDecl() throws ParseException
    {
        Vector v = new Vector();
        ElementDecl ed;

        nextToken();
        ed = findElementDecl(v);
        // push to handle name space
        push(ed, name,Element.ELEMENTDECL);
        ed.parseAttList(this);
        checkToken(TAGEND, ">");
        pop();
    }


    private void reportMismatch(int state, String msg) throws ParseException
    {
        if (current.ed == null) // this should never happen unless the parser is in incorrect state
        {
            error("Content mismatch. Stopped at state " + state);
        }

        Vector els = current.ed.expectedElements(state);
        error(msg + "  Expected elements " + els);
    }


    /**
     * expect element content
     */    
    final void parseElement() throws ParseException
    {
        boolean empty = false;

        for (;;)
        {
            if (empty && token != CLOSETAGSTART)
                error("Expected " + tokenString(CLOSETAGSTART) + " instead of " + tokenString(token));
            empty = false;

            switch(token)
            {
                case TAGSTART:                 
                    scanName("element tag");

                    ElementDecl ed = null;

                    Element e = addNewElement(Element.ELEMENT, name, true,null);
                    push(e,name,Element.ELEMENT);

                    if (validating)
                    {
                        ed = dtd.findElementDecl(name);
                        if (ed != null)
                        {
                            if (ed.getContent().type == ContentModel.EMPTY)
                            {
                                empty = true;
                            }
                            ed.initContent(current, this);
                        }
                        else
                        {
                            error("Element '" + name + "' used but not declared in the DTD.");
                        }
                    }
                    else
                    {
                        // set matched so parsecontent won't complain
                        current.matched = true;
                    }

                    parseAttributes(e);
                    if (token == EMPTYTAGEND)
                    {
                        if (ed != null && !ed.acceptEmpty(current))
                        {
                            reportMismatch(0, ed.name.getName() + " cannot be empty.");
                        }
                        factory.parsed(current.e);
                        pop();
                        empty = false;
                    }
                    else if (token != TAGEND )
                    {
                        error("Expected " + tokenString(TAGEND) + " instead of " + tokenString(token));
                    }
                    else if (lookahead != '<' && empty)
                    {
                        error("Expected " + tokenString(TAGEND) + " instead of '" + (char)lookahead + "'");
                    }
//                    current.checkAllowText();
                    break;
                case TEXT:
                    parseText('&', '&');
                    break;
                case PITAGSTART:
                    parsePI(false);
                    break;
                case COMMENT:
                    parseComment();
                    break;
                case CDATATAGSTART:
                    parseCDATA();
                    break;
                case CLOSETAGSTART:
                    if (!current.matched) // error
                    {
                        reportMismatch(current.state, current.e.getTagName().getName() + " is not complete.");
                    }
                    if (! shortendtags || lookahead != '>') // allow </>
                    {
                        if (lookahead == '/')
                            advance();
                        else
                        {
                            // This little hack with the current context pointer
                            // makes sure that the namespace scope inside the tag
                            // is not applied to the end tag.  For example, if the
                            // namespace "foo" is defined to point to DTD "xyz.dtd", 
                            // then tag <foo::bar> is started, then the namespace
                            // "foo" is redefined inside it to point to some other
                            // DTD then the end tag is found </foo::bar>, then the
                            // namespace for this end tag must point to the xyz.dtd
                            // otherwise we will have a tag mismatch.  So we temporarily
                            // pop the context while scanning the end tag name.  We
                            // cannot do a full pop() here because the context is still
                            // needed after scanName, for example if an error occurs, and
                            // in the call to the factory->parsed method.
                            Context c = current;
                            current = (Context)contexts.elementAt(contextAt-1);                            scanName("element close tag");
                            current = c;
                            if (name != current.tagName)
                            {
                                error("Close tag " + name + " does not match start tag " + current.e.getTagName());
                            }
                        }
                    }

                    parseToken(TAGEND, ">");
                    factory.parsed(current.e);
                    pop();
                    break;
                case EOF:
                    if (contextAt == 1) 
                    {
                        error("Expected the end of root element instead of end of file.");
                        break;
                    }
                default:
                    error("Bad token in element content: " + tokenString(token));
            }
            if (contextAt == 0)
                break;
            nextToken();
        }
    }


    /**
     * parse root element
     */    
    final void parseRootElement() throws ParseException
    {
        if (token != TAGSTART)  // error
            error("Start of root element expected instead of " + tokenString(token));

        scanName("element tag");
        if (docType != null) {
            if (name != docType)
                error("Root element name must match the DOCTYPE name");
        }

        if (name == nameXMLNameSpace)
        {
            parseNameSpaceDecl(false, false);
            return;
        }


        ElementDecl ed = null;

        Element e = addNewElement(Element.ELEMENT, name, false,null);
        push(e,name,Element.ELEMENT);
        boolean empty = false;

        if (validating)
        {
            ed = dtd.findElementDecl(name);
            if (ed != null)
            {
                if (ed.getContent().type == ContentModel.EMPTY)
                {
                    empty = true;
                }
                ed.initContent(current, this);
            }
            else
            {
                error("Element '" + name + "' used but not declared in the DTD.");
            }
        }
        else
        {
            // set matched so parsecontent won't complain
            current.matched = true;
        }

        parseAttributes(e);
        if (token == EMPTYTAGEND)
        {
            if (ed != null && !ed.acceptEmpty(current))
                reportMismatch(0, "Root element " + e.getTagName().getName() + " cannot be empty.");
            empty = true;
        }
        else if (token != TAGEND)
        {
            if (e.getAttributes() == EnumWrapper.emptyEnumeration)
                error("No attribute is declared for element '" + e.getTagName() + "', expected " + tokenString(TAGEND));
            error("Expected " + tokenString(TAGEND) + " instead of " + tokenString(token));
        }
//        current.checkAllowText();
        if (empty)
        {
            pop();
            nextToken();
        }
        else {
            nextToken(); 
            parseElement();
        }
        return;
    }

    final void parseAttributes(Element e) throws ParseException
    {
        boolean ns = false;

        while (nextToken() == NAME)
        {
            Name currName = name;  // Because parseAttributes may reset name, currName
                                   // is needed to hold name and check for nameXMLSpace 

            // make sure xml-space belongs to the XML namespace.
            if (currName.getName().equals(nameXMLSpace.getName()))
            {
                currName = nameXMLSpace;
                ns = true;
            }

            if (e != null && e.getAttribute(currName) != null)
            {
                error("An attribute cannot appear more than once in the same start tag");
            }
            parseToken(EQ, "=");

            if (currName == nameXMLLang)
            {
                parseToken(QUOTE, "string");
                parseToken(NAME, "lang code");
                Name code = name;
                parseToken(QUOTE, "string");
                factory.parsedAttribute(e, currName, code.getName());
            } 
            else if (current.ed != null && e != null)
            {
                current.ed.parseAttribute(e, currName, this);
            }
            else 
            {
                parseToken(QUOTE, "string");
                scanString(quote, '&', '&', '<');
                factory.parsedAttribute(e, currName, text);
            }
        }

        if (ns)
        {
            Object attr = e.getAttribute(nameXMLSpace);
            if (attr == null) attr = e.getAttribute(nameXMLSpace2);
            if (attr != null)
            {
                String s = null;
                if (attr instanceof String) 
                {
                    s = (String)attr;
                } 
                else if (attr instanceof Atom)
                {
                    s = attr.toString();
                }
                else if (attr instanceof Name)
                {
                    s = ((Name)attr).getName().toString();   
                }

                if (s != null && s.equalsIgnoreCase("preserve")) 
                {
                    current.preserveWS = true;
                } 
                else if (s != null && s.equalsIgnoreCase("default")) 
                {
                    current.preserveWS = false;
                } 
                else 
                {
                    error("Invalid value '" + s + "' for xml-space attribute.");
                }
            }
        }

        if (current.ed != null)
            current.ed.checkAttributes(e, this);
    }


    final void parseContent(Element e) throws ParseException
    {
        while (nextToken() != CLOSETAGSTART)
        {
            switch (token)
            {
                case TEXT:
                    parseText('&', '&');
                    break;
                case TAGSTART:
                    parseElement();
                    break;
                case PITAGSTART:
                    parsePI(false);
                    break;
                case DECLTAGSTART:
                    parseElement();
                    break;
                case COMMENT:
                    parseComment();
                    break;
                case CDATATAGSTART:
                    parseCDATA();
                    break;
                default:
                    error("Bad token in element content: " + tokenString(token));
            }
        }        
        if (!current.matched)
        {
            error("Content mismatch, stopped at state " + current.state);
        }
        if (lookahead != TAGEND)
        {
            scanName("element close tag");
            if (name != current.e.getTagName())
            {
                error("Close tag mismatch: " + name + " instead of " + current.e.getTagName());
            }
        }
        parseToken(TAGEND, ">");
    }

    final Element parsePI(boolean global) throws ParseException
    {
        parseKeyword(0, "PI name");
        if (token == XML)
        {
            if (!firstLine)
                error("An XML declaration can only appear in the very beginning of the document.");
            else {
                parseXMLDecl();
                return null;
            }
        }
        else if (token == NAMESPACE)
            return parseNameSpaceDecl(global, true);
        else if (token != NAME)
        {
            error("Expecting PI name instead of " + tokenString(token));
        }

        return finishPI();
    }

    final Element parseNameSpaceDecl(boolean global, boolean PI) throws ParseException
    {
        Element e = addNewElement(Element.NAMESPACE, name, false,null);
        push(e,name,Element.NAMESPACE);
        parseAttributes(e);
        pop();

        if (PI && token != PITAGEND) 
        {
            error("Expected PI tag end '?>' instead of " + tokenString(token));
        }
        else if (!PI && token != EMPTYTAGEND)
        {
            error("Expected " + tokenString(EMPTYTAGEND) + " instead of " + tokenString(token));
        }

        Object asStr = e.getAttribute(nameXMLAS);
        Object hrefStr = e.getAttribute(nameXMLHREF);
        Object nsStr = e.getAttribute(nameXMLNS);

        if (asStr == null || nsStr == null) // href is now optional 
        {
            error("Missing attribute 'ns' or 'prefix'");          
        }

        Atom as, ns, href = null;
        if (caseInsensitive)
        {
            as = Atom.create(asStr.toString().toUpperCase());
            ns = Atom.create(nsStr.toString().toUpperCase());
            if (hrefStr != null) href = Atom.create(href.toString().toUpperCase());
        }
        else
        {
            as = Atom.create(asStr.toString());
            ns = Atom.create(nsStr.toString());
            if (hrefStr != null) href = Atom.create(href.toString());
        }
        if (DTD.isReservedNameSpace(as))
        {
            error(as.toString() + " is a reserved name space.");
        }

        addNameSpace(as, ns, global); // ns is the unique urn.

        if (loadexternal && href != null) { // load name space
            // if the dtd has not been loaded, load it
            if (dtd.findLoadedNameSpace(href) == null)
            {
                dtd.addLoadedNameSpace(href);
                loadDTD(href.toString(), href);
            }
        }
    
        return e;
    }

    final Element finishPI() throws ParseException
    {
        Element e = addNewElement(Element.PI, name, false,null);

        charAt = 0;
        boolean stop = false;
        while (lookahead != -1)
        {
            chars[charAt++] = (char)lookahead;
            if (lookahead == '?')
            {
                advance();
                if (lookahead == '>')
                {
                    charAt--;
                    stop = true;
                }
            }
            else
            {
                advance();
            }
            if (charAt == chars.length || stop)
            {
                push(e,name,Element.PI);
                addNewElement(Element.CDATA, null, false,
                    new String(chars, 0, charAt));
                pop();
                charAt = 0;
                if (stop)
                    break;
            }
        }

        parseToken(TAGEND, "PI end");
        return e;
    }

    final Element parseText(int eref, int cref) throws ParseException
    {
        scanText(eref, cref, ! current.preserveWS);
        return addPCDATA();
    }

    final Element addPCDATA() throws ParseException
    {
        if (charAt > 0 || seenWS) 
        {
            if (seenWS)
            {
                chars[charAt++] = ' ';
                seenWS = false;
            }
            text = new String(chars, 0, charAt);
            Element t = addNewElement(Element.PCDATA, null, true, text);
            return t;
        }
        seenWS = false; 
        return null;
    }

 
    /**
     * Parses comment <code> '<!--' comment '-->' </code>
     */
    final Element parseComment() throws ParseException
    {
        Element e = addNewElement(Element.COMMENT, nameComment, false, null);

        charAt = 0;
        boolean stop = false;
        while (lookahead != -1)
        {
            chars[charAt++] = (char)lookahead;
            if (lookahead == '-')
            {
                advance();
                if (lookahead == '-')
                {
                    advance();
                    if (lookahead == TAGEND)
                    {
                        charAt--;
                        stop = true;
                    }
                    else if (strict)
                    {
                        error("Bad comment syntax. Expected '>'.");
                    }
                    else
                    {
                        reader.push((char)lookahead);
                        lookahead = '-';
                    }
                }
            }
            else
            {
                advance();
            }
            if (charAt == chars.length || stop)
            {
                push(e,nameComment,Element.COMMENT);
                addNewElement(Element.CDATA, null, false,
                    new String(chars, 0, charAt));
                pop();
                charAt = 0;
                if (stop)
                    break;
            }

        }

        parseToken(TAGEND, "comment end");
        return e;
    }


    /**
     *  Parses CDATA <code> '<![CDATA[' data ']]>'  </code> 
     */
    final void parseCDATA() throws ParseException
    {
        charAt = 0;
        boolean stop = false;
        while (lookahead != -1)
        {
            chars[charAt++] = (char)lookahead;
            if (lookahead == ']')
            {
                advance();
                if (lookahead == ']')
                {
                    advance();
                    if (lookahead == TAGEND)
                    {
                        charAt--;
                        stop = true;
                    }
                    else
                    {
                        reader.push((char)lookahead); 
                        lookahead = ']';
                    }
                }
            }
            else
            {
                advance();
            }

            if (charAt == chars.length || stop)
            {
                addNewElement(Element.CDATA, nameCDATA, false,
                    new String(chars, 0, charAt));
                charAt = 0;
                if (stop)
                    break;
            }
        }

        parseToken(TAGEND, "CDATA end");
    }


    /**
     * Set url and open an input stream for it
     */
    final private void setURL(URL u) throws ParseException
    {
        url = u;         
        try
        {
           setInputStream( new BufferedInputStream(url.openStream()));
        }
        catch (IOException e) 
        {
            throw new ParseException("Error opening input stream for \"" + 
                url.toString() + "\": " + e.toString());
        }
    }

    final private void setInputStream(InputStream in) throws ParseException
    {
        xmlIn = new XMLInputStream(in);
        reader = new EntityReader(xmlIn, 1, 1, null, this);
        advance();
    }

    final private void setFactory(ElementFactory f) 
    {
        factory = f;
    }
    
    /**
     * Factory to create elements on the parse tree.
     */    
    ElementFactory factory;
    
    /**
     * DTD object.
     */    
    DTD dtd;      
    boolean validating; // whether we loaded any DTD's for validation.
    
    /**
     * Root of tree.
     */
    Element root;
    
    /**
     * Stack to keep track of contexts
     */
    Vector contexts = new Vector(16);
    int contextAt = 0;

    /**
     * Current element context.
     */
    Context current;
    
    /**
     * Inputstream used to read Unicode chars
     */    
    EntityReader reader;
    XMLInputStream xmlIn;

    /**
     * True if parsing inside tag
     */
    boolean inTag;

    /**
     * true when scanner is still collapsing white space.
     */
    boolean seenWS;
    
    /**
     * next character
     */    
    int lookahead;
    
    /**
     * quote char
     */    
    char quote;
    
    /**
     * chars collected up to 8K
     */    
    char chars[] = new char[8192];
    
    /**
     * char index into chars[]
     */    
    int charAt;
    
    /**
     * buf collected up to 8K
     */    
    char buf[] = new char[8192];
    
    /**
     * char index into buf[]
     */    
    int bufAt;

    /** 
     * counter to allow collecting multiple names into buf
     */
    int nameappend;
    
    /**
     * Token matching hashtable
     */    
    static Hashtable tokens;

    /**
     * token type
     */    
    int token;
    
    /**
     * counter to allow keyword checking
     */
    int keyword;

    /**
     * counter to disable name uppercasing
     */
    int nouppercase;

    /**
     * counter to allow parameter substition in token
     */
    int substitution;

    /**
     * break char for parseInternalSubset
     */
    int breakText;
    
    /**
     * switch to allow parsing name tokens in scanName
     */    
    int nametoken;

    /**
     * switch to allow parsing unqualified name in scanName
     */    
    int simplename;

    /**
     * switch to allow namespace when scan entity reference name in scanName
     */    
    int inEntityRef;

    /**
     * whether or not to expand named entities.
     */
    boolean expandNamedEntities;

    static boolean jdk11;

    /**
     * current name fetched
    */
    Name name;

    /**
     * current string or text fetched
    */
    String text;

    /**
      * document name string
      */
    URL url;

    /**
     * name in DOCTYPE tag
     */
    Name docType;

    /**
     * whether we are parsing internal subset
     */
    boolean internalSubset;

    /**
     * whether the document is caseInsensitive
     */
    boolean caseInsensitive;

    /**
     * whether the parser is parsing the first line of a document
     */
    boolean firstLine = true;

    /**
     * XML element declaration
     */
    static ElementDecl XMLDecl;

    /**
     * If the keyword of the latest conditional section is a parameter entity reference, this 
     * variable records the reference name; otherwise null.
     */
    Name conditionRef;

    /**
     * Whether standalone was specified in XML declaration.
     * (default is false).
     */ 
    boolean standAlone;

    /**
     * Whether to load external DTD's
     */
    boolean loadexternal;

    /**
     * Char type table
     */
    static int chartype[] = new int[256];

    /**
     * Char upper case table
     */
    static char charupper[] = new char[256];

    static final int FWHITESPACE    = 1;
    static final int FDIGIT         = 2;
    static final int FLETTER        = 4;
    static final int FMISCNAME      = 8;
    static final int FSTARTNAME     = 16;

    static final char nameSpaceSeparator = ':';

    /**
     * This flag specifies whether the parser should enforce strict
     * XML compliance rules or whether to allow some SGML like things
     * like SGML comment syntax.
     */
    static boolean strict = false;
	
	boolean shortendtags;

    /**
     * predefined names
     */
    static Name nameComment;
    static Name nameCDATA;
    static Name namePCDATA;
    static Name nameVERSION;
    static Name nameENCODING;
    static Name nameDOCTYPE;
    static Name nameXML;
    static Name nameStandalone;
    static Name nameYes;
    static Name nameNo;
    static Name nameURL;
    static Name namePUBLICID;
    static Name nameNAME;
    static Name nameXMLSpace;
    static Name nameXMLSpace2;
    static Name nameXMLAS;
    static Name nameXMLHREF;
    static Name nameXMLNS;
    static Name nameXMLNameSpace;
    static Atom atomXML;
    static Name nameXMLLang;

    static
    {
        nameComment = Name.create("--");
        nameCDATA = Name.create("[CDATA[");
        namePCDATA = Name.create("PCDATA");
        nameVERSION = Name.create("version");
        nameENCODING = Name.create("encoding");
        nameStandalone = Name.create("standalone");
        nameDOCTYPE = Name.create("DOCTYPE");
        nameXML = Name.create("xml");        
        nameYes = Name.create("yes");
        nameNo = Name.create("no");
        nameURL = Name.create("URL");
        namePUBLICID = Name.create("PUBLICID");
        nameNAME = Name.create("NAME");
        nameXMLSpace = Name.create("xml-space","xml");
        nameXMLSpace2 = Name.create("space","xml");
        nameXMLAS = Name.create("prefix", "xml");
        nameXMLHREF = Name.create("src", "xml");
        nameXMLNS = Name.create("ns", "xml");
        nameXMLNameSpace = Name.create("namespace", "xml");
        atomXML = Atom.create("xml");
        nameXMLLang = Name.create("lang","xml");

        //
        // '<?XML' VersionInfo EncodingDecl? SDDecl? S? '?>'
        XMLDecl = new ElementDecl(nameXML);
        //
        // VersionInfo ::= S 'version' Eq ('"1.0"' | "'1.0'")
        XMLDecl.addAttDef(new AttDef(nameVERSION, AttDef.CDATA, "1.0", AttDef.FIXED));
        //
        // S 'encoding' Eq QEncoding
        XMLDecl.addAttDef(new AttDef(nameENCODING, AttDef.CDATA, "UTF-8", AttDef.IMPLIED));

        // SDDecl ::=  S 'standalone' Eq "'" ('yes' | 'no') "'"  
        Vector an = new Vector(2);
        an.addElement(nameYes);
        an.addElement(nameNo);
        XMLDecl.addAttDef(new AttDef(nameStandalone, AttDef.ENUMERATION, (Name)an.elementAt(0), AttDef.IMPLIED, an));
        
        // add recognized names to the hashtable
        tokens = new Hashtable();
        tokens.put("DOCTYPE", new Integer(DOCTYPE));
        tokens.put("SYSTEM", new Integer(SYSTEM));
        tokens.put("PUBLIC", new Integer(PUBLIC));
        tokens.put("ENTITY", new Integer(ENTITY));
        tokens.put("ELEMENT", new Integer(ELEMENT));
        tokens.put("EMPTY", new Integer(EMPTY));
        tokens.put("ANY", new Integer(ANY));
        tokens.put("PCDATA", new Integer(PCDATA));
        tokens.put("ATTLIST", new Integer(ATTLIST));
        tokens.put("CDATA", new Integer(CDATA));
        tokens.put("ID", new Integer(ID));      
        tokens.put("IDREF", new Integer(IDREF));  
        tokens.put("IDREFS", new Integer(IDREFS));  
        tokens.put("ENTITY", new Integer(ENTITY));  
        tokens.put("ENTITIES", new Integer(ENTITIES));
        tokens.put("NMTOKEN", new Integer(NMTOKEN)); 
        tokens.put("NMTOKENS", new Integer(NMTOKENS));
        tokens.put("FIXED", new Integer(FIXED));
        tokens.put("REQUIRED", new Integer(REQUIRED));
        tokens.put("IMPLIED", new Integer(IMPLIED));
        tokens.put("NDATA", new Integer(NDATA));
        tokens.put("NOTATION", new Integer(NOTATION));
        tokens.put("INCLUDE", new Integer(INCLUDETAGSTART));
        tokens.put("IGNORE", new Integer(IGNORETAGSTART));
        tokens.put("namespace", new Integer(NAMESPACE));
        tokens.put("EXTENDS", new Integer(EXTENDS));
        tokens.put("IMPLEMENTS", new Integer(IMPLEMENTS));
        tokens.put("xml", new Integer(XML));
        tokens.put("version", new Integer(VERSION));
        tokens.put("encoding", new Integer(ENCODING));
        tokens.put("standalone", new Integer(STANDALONE));

        for (int i = 0; i < 256; i++) {
            char c = (char)i;
            chartype[i] = 0;
            if ((jdk11 && Character.isWhitespace(c)) ||
                (Character.isSpace(c) || c == 13))
                chartype[i] = FWHITESPACE;
            if (Character.isLetter(c))
                chartype[i] |= FLETTER;
            if (Character.isDigit(c))
                chartype[i] |= FDIGIT;

            charupper[i] = Character.toUpperCase(c);
        }
        chartype['.'] |= FMISCNAME;
        chartype['-'] |= FMISCNAME;
        chartype['_'] |= FMISCNAME | FSTARTNAME;
        chartype[0xb7] |= FMISCNAME; // Extender
    }
}    

