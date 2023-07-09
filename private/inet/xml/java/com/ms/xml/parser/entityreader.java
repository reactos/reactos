/*
 * @(#)EntityReader.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import java.io.*;

/**
 * This is a wrapper class on entities to provide transparent
 * inclusion of entity text
 *
 * @version 1.0, 6/3/97
 */
class EntityReader 
{
    EntityReader(InputStream in, int line, int column, EntityReader prev, Object owner)
    {        
        this.line = line;
        this.column = column;
        this.prev = prev;
        this.owner = owner;
        this.pos = -1;
        this.input = in;
    }

    public int read() throws ParseException
    {
        int c =  readChar();

        // Proper end-of-line handling.

        if (c == 0xd || c == 0xa)
        {
            if (c == 0xd)
            {
                int d = readChar();
                if (d != 0xa)
                {
                    // Hmmm.  not the usual 0xd, 0xa pair.
                    // Put the d back
                    push((char)d);
                }
                c = 0xa; // normalize to 0xa
            }
            line++;
            column = 1;
        }
        else
        {
            column++;
        }
        return c;
    }

    int readChar() throws ParseException
    {
        int c;
        if( pos >= 0 )
        {
            c = next[pos--];
        }
        else
        {
            try
            {
                c = input.read();
            }
            catch( IOException e )
            {
                String s = owner.toString();
                c = -1;
                throw new ParseException( "Error reading " + s + " at (" + line + "," + column + ") : " + e,
                    line, column, owner);
            }
        }
        return c;
    }

    void push(char next) throws ParseException
    {
        if (pos == 1) {
            throw new ParseException("Error unreading '" + next + "' at (" + line + "," + column + ") : ",
                    line, column, owner);
        }
        this.next[++pos] = next;
    }

    /**
     * line number
     */    
    int line;
    
    /**
     * character pos
     */    
    int column;

    /**
     * previous reader to go back to when this is done
     */
    EntityReader prev;
    
    /**
     * Entity reading from
     */    
    Object owner;

    /**
     * For limited character push-back support.
     */
    int next[] = new int[3];
    int pos;

    /**
     * The input stream for reading characters
     */
    InputStream input;
}
