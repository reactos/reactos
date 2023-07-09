/*
 * @(#)XMLInputStream.java 1.0 6/10/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
package com.ms.xml.util;

import java.io.*;
import java.net.*;

/**
 * 
 * A Reader specifically for dealing with different encoding formats 
 * as well as liitleendian files.
 *
 * @version 1.0, 6/10/97
 */
public class XMLInputStream extends InputStream
{
    /**
     * The state enumerators
     */    
    static final int INPUTSR     = 1;   // The different states that concern the read() 
    static final int UCS2        = 2;   // The POP states imply that there are characters
    static final int ASCII       = 3;   // stored in the next[] stack.  There are separate
    static final int INPUTSR_POP = 4;   // states for POP in order to speed up the read().
    static final int UCS2_POP    = 5;   // There is still room for future additions, such
    static final int ASCII_POP   = 6;   // as a UCS-4 state.  
                                        // NOTE:  All encodings that can use InputStreamReader
                                        //        fall into the INPUTSR state
    /**
     * input buffer size on windows platforms
     */ 
    static final int SIZE = 1024;       // input buffer size

    /**
     * encoding numbers
     */
    static final int INTUTF8   = 0;
    static final int INTASCII  = 1;
    static final int INTUCS2   = 2;
    static final int INTUCS4   = 3;
    static final int INTEBCDIC = 4;
    static final int INT1252 = 5;


    /**
     * Builds the XMLInputStream.
	 * This constructor is intended to be called on Windows to speed up input
	 * speed. The decoding is done by IXMLStream
	 * 
	 * Note:
	 * This constructor relies on CXMLStream class in xmlurlstream.dll. If 
	 * xmlurlstream.dll or xmlurlstream.tlb is not properly registered on the 
	 * system, or the encoding of the input stream cannot be handled,
	 * this constructor throws an IOException
     */
    public XMLInputStream(URL url) throws IOException
    {
        try 
		{
			Class clazz = Class.forName("com.ms.xml.xmlstream.XMLStream");
            xmlis = (XMLStreamReader)clazz.newInstance();
            intEncoding = xmlis.open(url.toString());
        }
		catch (Throwable e) 
		{
            throw new IOException("Error opening input stream for \"" + 
                    url.toString() + "\": " + e.toString());
        }

        onWindows = true;
        switch (intEncoding)
        {
            case INTUTF8:    encoding = "UTF-8";  break;
            case INTASCII:   encoding = "ASCII";  break;
            case INTUCS2:    encoding = "UCS-2";  break;
            case INTUCS4:    encoding = "UCS-4";  break;
            case INTEBCDIC:  encoding = "EBCDIC"; break;
            case INT1252:   encoding = "windows-1252"; break;
            default:
                throw new IOException("Error opening input stream for \"" + 
                    url.toString() + "\"");
        }
    }


    /**
     * Builds the XMLInputStream.
     * Reads the first four bytes of the InputStream in order to make a guess
     * as to the character encoding of the file.
     * Assumes that the document is following the XML standard and that
     * any non-UTF-8 file will begin with a <?XML> tag.
     */
    public XMLInputStream(InputStream in)
    {
        String version = System.getProperty("java.version");
        jdk11 = version.equals("1.1") ? true : false;

        littleendian   = false;
        caseInsensitive = false;

        byteOrderMark  = false;
        encoding       = "UTF-8";    // Default encoding

        this.in        = in;
        this.insr      = null;

        readState      = ASCII;

        boolean setDefault = false;

        try
        {
            char c1, c2, c3, c4;

            c1 = (char)in.read();
            c2 = (char)in.read();
            c3 = (char)in.read();
            c4 = (char)in.read();
            if( c1 == 0xFE && c2 == 0xFF && c3 == 0x00 && c4 == 0x3C )
            {
                // UCS-2, big-endian
                littleendian  = false;
                byteOrderMark = true;
                readState     = UCS2_POP;
                encoding      = "UCS-2";
            }
            else if( c1 == 0xFF && c2 == 0xFE && c3 == 0x3C && c4 == 0x00 )
            {
                // UCS-2, little-endian
                littleendian  = true;
                byteOrderMark = true;
                readState     = UCS2_POP;
                encoding      = "UCS-2";

                this.in       = new ByteSwapInputStream( in );
            }
            else if( c1 == 0x00 && c2 == 0x3C && c3 == 0x00 && c4 == 0x3F )
            {
                // UCS-2, big-endian, no Byte Order Mark
                littleendian = false;
                readState    = UCS2_POP;
                encoding     = "UCS-2";
            }
            else if( c1 == 0x3C && c2 == 0x00 && c3 == 0x3F && c4 == 0x00 )
            {
                // UCS-2, little-endian, no Byte Order Mark
                littleendian = true;
                readState    = UCS2_POP;
                encoding     = "UCS-2";

                this.in      = new ByteSwapInputStream( in );             
            }
            else if( c1 == 0x3C && c2 == 0x3F && 
                Character.toUpperCase(c3) == 0x58 && 
                Character.toUpperCase(c4) == 0x4D )
            {
                // UTF-8, ISO 646, ASCII, some part of ISO 8859, Shift-JIS, EUC,
                // or any other encoding that ensures that ASCII has normal positions
                readState = ASCII_POP;
                encoding  = "ASCII";
            }
            else if( c1 == 0x00 && c2 == 0x00 && c3 == 0x00 && c4 == 0x3C )
            {
                // UCS-4, big-endian machine (1234 order)
                readState = ASCII_POP;  // Until UCS-4 is implemented
                encoding  = "UCS-4";
            }
            else if( c1 == 0x3C && c2 == 0x00 && c3 == 0x00 && c4 == 0x00 )
            {
                // UCS-4, little-endian machine (4321 order)
                readState = ASCII_POP;  // Until UCS-4 is implemented
                encoding  = "UCS-4";
            }
            else if( c1 == 0x00 && c2 == 0x00 && c3 == 0x3C && c4 == 0x00 )
            {
                // UCS-4, unusual octet order (2143 order)
                readState = ASCII_POP;  // Until UCS-4 is implemented
                encoding  = "UCS-4";
            }
            else if( c1 == 0x00 && c2 == 0x3C && c3 == 0x00 && c4 == 0x00 )
            {
                // UCS-4, unusual octet order (3412 order)
                readState = ASCII_POP;  // Until UCS-4 is implemented
                encoding  = "UCS-4";
            }
            else if( c1 == 0x4C && c2 == 0x6F && c3 == 0xE7 && c4 == 0xD4 )
            {
                // EBCDIC - We do NOT support this!
                readState = ASCII_POP;  // Until EBCDIC is implemented
                encoding  = "EBCDIC";
            }
            else
            {
                // UTF-8 without an <?XML> tag (assuming data is not corrupt)
                setDefault = true;
            }

            if( !encoding.equals( "UCS-2" ) )
            {
                push(c4);
                push(c3);
                push(c2);
                push(c1);
            }
            else
            {
                if( littleendian )
                {
                    push(c3);
                    push(c4);
                    if( !byteOrderMark )
                    {
                        push(c1);
                        push(c2);        
                    }
                }
                else
                {
                    push(c4);
                    push(c3);
                    if( !byteOrderMark )
                    {
                        push(c2);
                        push(c1);        
                    }
                }
            }
        }
        catch (IOException e) 
        {
            // Can't do lookahead, so use default UTF-8
            setDefault = true;
        } 

        pos = -1;

        if( setDefault )
        {
            try 
			{  
                if (! jdk11)
                    throw new IOException("Readers not supported in JDK 1.0");
                // guess that the <?xml encoding=...?> tag will be read in
                // less that 4096 bytes.
                if (! in.markSupported()) 
				{
                    in = new BufferedInputStream(in);
                }
                in.mark(4096);
                insr      = new InputStreamReader( in, "UTF8" );
                readState = INPUTSR_POP;                    
                encoding  = "UTF-8";
            } 
			catch( IOException e2 ) 
			{
                // If there is an exception we can
                // just continue and treat file like ASCII text.
                readState = ASCII_POP;   
                encoding = "ASCII";
            }          
        }
    }

    private void push(char next)
    {
        if (index == 3) 
		{
            System.exit(0);
        }
        this.next[++index] = next;
    }

    /**
     * Returns the next unicode char in the stream.  The read done 
     * depends on the current readState.  POP states imply that there
     * are characters that have been pushed onto the next[] stack.
     */
    public int read() throws IOException
    {
        // On windows 
        if (onWindows)
        {
            int len = 0;
            if (index >= size)
            {
                if (eof)
                    return -1;
                try 
                {
                    len = xmlis.read(buffer, SIZE);
                }
                catch (Exception e) 
				{
//                    System.out.println("Unexpected error: " + e.toString());
                    return -1;
                }
                
                if (len <= 0)
                    return -1;
                if (len < SIZE)
                    eof = true;
                size = len;
                index = 0;
            }
            int rc = buffer[index++];
            if (rc == 0)
            {
                String err = "Stream error: unexpected null";
                throw new IOException(err);
            }
            return rc;
        }

        // On other platform
        switch( readState )
        {
            case INPUTSR:
                pos++; 
                return insr.read();
            case ASCII:
                return in.read();
            case UCS2:            
                {
                    int b1, b2;

                    b1 = in.read();
    
                    if( b1 == -1 )
                        return -1;
        
                    b2 = in.read();

                    return ((b1 << 8) | b2);                         
                }              
            case INPUTSR_POP:
                if (index >= 0) 
				{
                    return next[index--];
                }
				else 
				{
                    readState = INPUTSR;   
                    return read();
                }
            case UCS2_POP:        
                {
                    int b1, b2;

                    if (index >= 0) 
					{
                        b1 = next[index--];
                    } 
					else 
					{
                        readState = UCS2;
                        b1 = in.read();
                    }
                
                    if( b1 == -1 )
                        return -1;
    
                    if (index >= 0) 
					{
                        b2 = next[index--];
                    } 
					else 
					{
                        readState = UCS2;
                        b2 = in.read();
                    }

                    return ((b1 << 8) | b2);              
                }        
            case ASCII_POP:
            default:
                if (index >= 0) 
				{
                    return next[index--];
                } 
				else 
				{
                    readState = ASCII;
                    return in.read();
                }
        }
    }
    

    /**
     * Defines the character encoding of the stream.  The new character encoding
     * must agree with the encoding determined by the constructer.  setEncoding
     * is used to clarify between encodings that are not fully determinable 
     * through the first four bytes in a stream and not to change the encoding.
     * This method must be called within 4096 reads() after construction.
     */
    public void setEncoding( String encoding ) throws IOException
    {
        insr = null;
        String encvm; // Java VM's version of encoding.
        int newEncoding = 0;

        if( encoding.equalsIgnoreCase( "ISO-10646-UCS-2" ) ||
            encoding.equalsIgnoreCase( "UCS-2" ) )
        {         
            if( !this.encoding.equalsIgnoreCase( "UCS-2" ) )
                throw new IOException( "Illegal Change of Encoding" );

            readState = UCS2;
            this.encoding = "UCS-2";
            return;
        }
        else if( encoding.equalsIgnoreCase( "Shift_JIS" ) )
        {
            if (onWindows)
                throw new IOException( "SJIS not yet supported" );
            encvm = "SJIS";
        }
        else if( encoding.equalsIgnoreCase( "ISO-8859-1" ) )
        {
            if (onWindows)
                throw new IOException( "8859_1 not yet supported" );
            encvm = "8859_1";
        }
        else if( encoding.equalsIgnoreCase( "ISO-10646-UCS-4" ) )
        {  // UCS-4 NOT YET SUPPORTED!
            throw new IOException( "UCS-4 not yet supported" );
        }
        else if( encoding.equalsIgnoreCase( "UTF-8" ) )
        {  
            encvm = "UTF8";
            newEncoding = INTUTF8; 
        }
        else
        {
            if (onWindows)
            {
                if (encoding.equals("windows-1252"))
                    newEncoding = INT1252 ;
                else
                    throw new IOException( encoding + " not yet supported" );
            }
            encvm = encoding; // try passing through to VM...
        }

        if( !this.encoding.equalsIgnoreCase( "ASCII" ) &&
            !this.encoding.equalsIgnoreCase( "UTF-8" ) )
            throw new IOException( "Illegal Change of Encoding" );
 
        if (onWindows)
        {
            if (intEncoding != newEncoding)
            {
                xmlis.setEncoding(newEncoding, index); 
                index = size = 0;
                eof = false; // since we have more to read now.
            }
            return;
        }
            
        if (this.encoding.equalsIgnoreCase( "ASCII" )) 
        {
            insr = null;
            readState = ASCII_POP; 
        } 
		else 
		{
            if (jdk11) {
                if (pos != -1) 
                {
                    in.reset();     // This fixes a nasty bug in that InputStreamReaders
                    in.skip(pos+1);   // now buffer their input.
                }
                insr = new InputStreamReader( in, encvm );
                readState = INPUTSR;                
                this.encoding = encoding;
            } 
			else 
			{
               throw new IOException( encvm + " is not supported by your Java Virtual Machine." +
                    "  Try installing the latest VM from http://www.microsoft.com/java/download.htm");           
            }
        }
    }

    
    /**
     * Creates a new XMLOutputStream with the proper initial state.
     * XMLOutputStreams should always be created through this method
     * if the output stream is to mimic this input stream.
     */
    public XMLOutputStream createOutputStream( OutputStream out)
    {
        XMLOutputStream xmlOut = new XMLOutputStream( out );
        try {
            xmlOut.setEncoding( encoding, 
                            littleendian, 
                            byteOrderMark );
        } catch (IOException e ) {
            // Hmm.  This should never happen because we already
            // successfully created the input stream.
        }
        return xmlOut;
    }

    /**
     * Close the stream and release system resources.
     */
    public void close() throws IOException
    {
        if (xmlis != null)
            xmlis.close();
        else if (insr != null)
            insr.close();
        else if (in != null)
            in.close();
    }

    /**
     * Character pushed back into the stream.
     * Need to be able push back four characters for auto-encoding
     * detection support.
     */
    private int next[] = new int[4];
    private int index = -1;

    /**
     * The stream readers
     */
    private InputStream       in;
    private InputStreamReader insr;

    /**
     * We remember the current position in the input stream so that
     * we can re-scan input after doing a reset() when setEncoding 
     * is called.  We have to do a mark()/reset() on setEncoding because
     * the first UTF8 InputStreamReader may buffer the input !
     */
    private int pos = 0; 

    /**
     * Character encoding state
     */
    private String  encoding;        
    private boolean littleendian;    // file littleendian (only applies to UCS-2 encoded files)
    private boolean byteOrderMark;   // byteOrderMark at the beginning of file (UCS-2)
    private int     readState;
    private boolean jdk11;

    /**
     * Instance varibales for Windows platforms
     */
    private XMLStreamReader xmlis  = null;    // input stream pointer
    private int intEncoding   = -1;           // encoding of input stream
    private int[] buffer = new int[SIZE];     // input buffer
    private int size  = -1;                   // bytes in buffer 
    private boolean eof = false;              // whether the end of input stream has been reached
    private boolean onWindows = false;        // whether this program is running on a windows machine

    /**
     * Whether the document to be parsed caseInsensitive.
     * (if so, all names are folded to upper case).
     * Default is 'false'.
     */ 
    public boolean caseInsensitive;
}
