/*
 * @(#)XMLOutputStream.java 1.0 6/10/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */ 
package com.ms.xml.util;
import com.ms.xml.parser.DTD;

import java.io.*;

/**
 * 
 * A writer specifically designed for dealing with XML, incluing
 * XML encoding as well as liitleendian files, and XML namespaces
 * and white space handling.
 *
 * @version 1.0, 6/10/97
 */
public class XMLOutputStream extends OutputStream
{       
    /**
     * The state enumerators
     */
    static final int OUTPUTSW    = 1;   // The different states that concern the write() 
    static final int UCS2        = 2;   // The UCS2_BOM implies a ByteOrderMark must be written.
    static final int UCS2_BOM    = 3;   // NOTE:  All encodings that can use OutputStreamWriter
    static final int ASCII       = 4;   //        fall into the OUTPUTSW state

    /**
     * Builds the XMLOutputStream.
     * It is created as a standard UTF-8 output stream.
     * A subsequent call to setEncoding will specify the correct
     * character encoding required.  XMLOutputStreams can be built
     * by using the createOutputStream method contained in XMLInputStream.
     */             
    public XMLOutputStream( OutputStream out)     
    {
        String version = System.getProperty("java.version");
        jdk11 = version.equals("1.1") ? true : false;
        outputStyle = DEFAULT;
        littleendian  = false;
        savingDTD = false;
        mixed = false;
        
        this.out      = out;

        newline = System.getProperty("line.separator");
        indent = 0;
        
        // We default immediately to UTF-8
        try {
            if (! jdk11)
               throw new IOException("Writers not supported in JDK 1.0");
            
            this.outsw = new OutputStreamWriter( out, "UTF8" );
            writeState = OUTPUTSW; 
            encoding   = "UTF-8";
        } catch( IOException e ) {
            // If there is an exception (there should never be) we can
            // just continue and treat file like ASCII text.
            this.outsw = null;
            writeState = ASCII;
            encoding   = "ASCII";
        }
    }

    /**
     * Pass-through of inherited OutputStream method.
     */
    public void flush() throws IOException
    {
        if( outsw != null )
            outsw.flush();
        else
            out.flush();    
    }

    /**
     * Pass-through of inherited OutputStream method.
     */
    public void close() throws IOException
    {
        if( outsw != null )
            outsw.close();
        else
            out.close();
    }

    /**
     * Defines the character encoding of the output stream.  
     */
    public void setEncoding( String encoding, 
                             boolean littleendian, 
                             boolean byteOrderMark ) throws IOException
    {
        outsw = null;
        String vmenc = encoding;

        if( encoding.equalsIgnoreCase( "UTF-8" ) )
        {                       
            vmenc = "UTF8";
        }
        else if( encoding.equalsIgnoreCase( "Shift_JIS" ) )
        {            
            vmenc = "SJIS";
        }
        else if( encoding.equalsIgnoreCase( "ISO-8859-1" ) )
        {         
            vmenc = "8859_1";
        }
        else if( encoding.equalsIgnoreCase( "ISO-10646-UCS-4" ) ||
                encoding.equalsIgnoreCase( "UCS-4" ))
        {  // UCS-4,  NOT YET SUPPORTED!
            throw new IOException( "UCS-4 not yet supported" );
        }
        else if( encoding.equalsIgnoreCase( "UCS-2" ) )
        {
            // We only set the byteOrderMark if the initialEncoding is UCS-2.  
            // Otherwise the flag is irrelant and we should ignore it.
            if( byteOrderMark )            
                writeState = UCS2_BOM;
            else
                writeState = UCS2;

            this.encoding  = "UCS-2";
                
            if( littleendian )
            {
                this.littleendian = true;
                out = new ByteSwapOutputStream( out );
            }
            return; // we're done ! UCS-2 is handled manually.
        }
        else
        {
            // If none of the conditionals equate to true, 
            // the inital writing is treated like ASCII text.
            writeState    = ASCII;
            this.encoding = "ASCII";
        }

        if (encoding.equalsIgnoreCase("ASCII"))
        {
            outsw = null;
        } 
        else
        {
            try {
                if (! jdk11)
                   throw new IOException("Writers not supported in JDK 1.0");
                outsw         = new OutputStreamWriter( out, vmenc );
                writeState    = OUTPUTSW;
                this.encoding = encoding;
            } catch( IOException e ) {
                // If there is an exception (there should never be) we can
                // just continue and treat file like ASCII text.
                throw new IOException( vmenc + " is not supported by your Java Virtual Machine." +
                    "  Try installing the latest VM from http://www.microsoft.com/java/download.htm");
            }            
        }
    }

    /**
     * writes a character to the stream according to the current writeState.
     * There is an extra state for a UCS-2 requiring a ByteOrderMark so as to
     * avoid an extra conditional in every UCS2 write.  The ByteOrderMark will only
     * be written once at the beginning of the file.
     */
    public void write( int c ) throws IOException
    {
        switch( writeState )
        {
            case OUTPUTSW:
                outsw.write( c );       
                break;
            case UCS2:
                {
                    int byte1, byte2;

                    byte1 = c >> 8;
                    byte2 = c & 0xff;

                    out.write(byte1);  
                    out.write(byte2);                    
                }
                break;
            case UCS2_BOM:   // We need to put a ByteOrderMark to immitate original file.
                {
                    int byte1, byte2;

                    writeState    = UCS2;  // We now go to a normal UCS-2 state.

                    out.write( 0xfe );
                    out.write( 0xff );
    
                    byte1 = c >> 8;
                    byte2 = c & 0xff;

                    out.write(byte1);  
                    out.write(byte2);                    
                }
                break;
            case ASCII:
            default:
                out.write( c );
                break;
        }
    }

    /**
     * This method writes out the fully qualified name, using
     * the appropriate short name syntax. For example: "foo:bar".
     * @param n the name being written
     * @param ns the name space which defines the context
     * in which the name was defined.
     */
    public void writeQualifiedName(Name n, Atom ns) throws IOException
    {
		Atom sns = n.getNameSpace();
		Atom shortName = sns;
		if (sns == ns)
		{
			writeChars(n.getName());
			return;
		}
        if (sns != null) 
        {
		    if (dtd != null && ! dtd.isReservedNameSpace(sns))
		    {
			    shortName = nameSpaceContext.findNameSpace(sns);
			    if (shortName == null)
				    shortName = dtd.findShortNameSpace(sns);
                if (shortName == null)
                    shortName = sns;
		    }
            writeChars(shortName.toString() + ":" + n.getName());
        } 
        else
        {
			if (ns == null)
                writeChars(n.getName());
			else writeChars(":" + n.getName());
        }
    }

    /**
     * Write the given string.
     */
    public void writeChars( String str ) throws IOException 
    {
        int strLen = str.length();

        for( int i = 0; i<strLen; i++ )
        {
            char ch = str.charAt( i );
            if (ch == 0xa)
            {
                int nlen = newline.length();
                for (int j = 0; j < nlen; j++)
                    this.write( newline.charAt(j) );
            }
            else
            {
                this.write( ch );
            }
        }
    }        

    /**
     * Write out the string with quotes around it.
     * This method uses the quotes that are appropriate for the string.
     * I.E. if the string contains a ' then it uses ", & vice versa.
     */
    public void writeQuotedString( String str ) throws IOException 
    {
        char quote = '"';
        if (str.indexOf('"') >= 0 && str.indexOf('\'') < 0) {
            quote = '\'';
        }
        write(quote); 

        int strLen = str.length();
        for( int i = 0; i<strLen; i++ )
        {
            int ch = str.charAt( i ) ;
            if (ch == quote) {
                if (quote == '"')
                    writeChars("&quot;");
                else
                    writeChars("&apos;");
            } else {
                this.write(ch);
            }
        }

        write(quote);
    }        

    /**
     * Write a new line or do nothing if output style is COMPACT.
     */
    public void writeNewLine() throws IOException
    {
        if (outputStyle == PRETTY && ! mixed) {
            int nlen = newline.length();
            for (int j = 0; j < nlen; j++)
                this.write( newline.charAt(j) );
        }
    }

    /**
     * Set the relative indenting level.  Eg indent(+1) or indent(-1).
     * The indent level controls what writeIndent writes.
     */
    public void addIndent(int offset) {
        indent += offset;
    }

    /**
     * Write the appropriate indent - given current indent level,
     * or do nothing if output style is COMPACT.
     */
    public void writeIndent() throws IOException
    {
        if (outputStyle == PRETTY && ! mixed)  {
            for (int i = 0; i < indent; i++)
                this.write('\t');
        }
    }

    public static int DEFAULT = 0; // default
    public static int PRETTY = 1; // pretty output
    public static int COMPACT = 2; // no new lines or tabs.

    /**
     * Set the output style (PRETTY or COMPACT).
     */
    public void setOutputStyle(int style)
    {
        outputStyle = style;
    }

    /**
     * Return the current output style.
     */
    public int getOutputStyle()
    {
        return outputStyle;
    }

    /**
     * The stream readers
     */
    private OutputStream       out;
    private OutputStreamWriter outsw;

    /**
     * The system newline character
     */
    String newline;

    /**
     * Character encoding state
     */
    boolean      littleendian;
    String       encoding;
    private int  writeState;

    /**
     * output styling.
     */
    int outputStyle;
    boolean jdk11;
    public boolean mixed;

    private int     indent;

    /**
     * needed for saving in correct format
     */
    public DTD dtd;

	/**
	 * needed for saving in correct format
	 */
	public NameSpaceContext nameSpaceContext = new NameSpaceContext();

    /**
     * whether we are in the scope of saving a DTD or not
     */
    public boolean savingDTD;

}
