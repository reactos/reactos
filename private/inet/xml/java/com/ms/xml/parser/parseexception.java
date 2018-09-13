/*
 * @(#)ParseException.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */

package com.ms.xml.parser;

import java.lang.Exception;

/**
* This class signals that a parsing exception of some sort has occurred. 
*
 * @version 1.0, 6/3/97
 */
public class ParseException extends Exception 
{
    /**
     * Constructs a <code>ParseException</code> exception with no detail message. 
     */
    public ParseException() 
    {
    	super();
    }

    /**
     * Constructs a <code>ParseException</code> exception with a specified 
      * message. 
      * @param s The detail message.
      * 
     */
    public ParseException(String s) 
    {
        super(s);
        line = column = 0;
        owner = null;
    }

    /**
     * Constructs a <code>ParseException</code> exception with detail about
     * what occurred.
     *
     * @param   s       The detail message.
     * @param   line    The line number of the input where the error was found.
     * @param   column  The position on the line.
     * @param   owner   The context in which the error was encountered.
     * This is either an <code>Entity</code> object or a <code>Parser</code> 
      * object.
     */
    public ParseException(String s, int line, int column, Object owner) 
    {
    	super(s);
        this.line = line;
        this.column = column;
        this.owner = owner;
    }

    /**
     * The line where the error was found.
     */
    public int line;

    /**
     * The position on the line where the error was found.
     */
    public int column;

    /**
     * The parser context in which the error occurred.
     */
    public Object owner;
}
