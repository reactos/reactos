/*
 * @(#)ByteSwapInputStream.java 1.0 6/5/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.io.*;

/**
 * 
 * This is a class that extends InputStream.
 * The read method switches byte order.
 * It is used for dealing with littleendian format.
 *
 * @version 1.0, 6/5/97
 */
class ByteSwapInputStream extends InputStream
{
    public ByteSwapInputStream( InputStream in )
    {
        this.in = in;
        this.byte1 = -2;
    }

    public int read() throws IOException
    {   
        if( byte1 == -2 )
        {
            byte1 = in.read();                

            if ( byte1 == -1 ) {            
                return -1;                
            }

            return in.read();                
        }                  
        else
        {        
            int temp = byte1;
            byte1 = -2;
            return temp;
        }
    }        

    /**
     * The input stream for reading characters
     */
    private InputStream in;

    /**
     * byte used for byteswapping purposes...
     */
    private int byte1;
}
