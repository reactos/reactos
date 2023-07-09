/*
 * @(#)ByteSwapOutputStream.java 1.0 6/5/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.io.*;

/**
 * 
 * This is a class that extends OutputStream.
 * It swap bytes order and is intended to create Littleendian format.
 *
 * @version 1.0, 6/9/97
 */
class ByteSwapOutputStream extends OutputStream
{
    public ByteSwapOutputStream( OutputStream out )
    {
        this.out = out;
        this.byte1 = -2;

    }

    public void write( int c ) throws IOException
    {
        if( byte1 == -2 )
        {
            byte1 = c;
        }
        else
        {
            out.write( c );
            out.write( byte1 );
            byte1 = -2;
        }
    }

    public void close() throws IOException
    {
        if( byte1 != -2 )
        {
            out.write( 0x00 );
            out.write( byte1 );
        }
    }

    /**
     * The input stream for reading characters
     */
    private OutputStream out;

    /**
     * byte used for buffering up writes and swapping order.
     */
    private int byte1;
}
