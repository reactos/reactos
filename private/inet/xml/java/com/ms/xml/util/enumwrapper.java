/*
 * @(#)Document.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.util;

import java.util.Enumeration;

public class EnumWrapper implements Enumeration
{
    public static EnumWrapper emptyEnumeration = new EnumWrapper(null);

    public EnumWrapper(Object o)
    {
        object = o;
        done = false;
    }

    public boolean hasMoreElements()
    {
        return (! done && object != null);
    }

    public Object nextElement()
    {
        if (! done) {
            done = true;
            return object;
        }
        return null;
    }

    boolean done;
    Object object;
}

