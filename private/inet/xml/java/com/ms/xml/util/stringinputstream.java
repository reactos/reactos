/*
 * @(#)XMLInputStream.java 1.0 6/10/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
package com.ms.xml.util;

import java.io.*;

public class StringInputStream extends InputStream
{

	public StringInputStream(String text)
	{
		size = text.length();
		index = 0;
		buf = text;
	}

    public int read() throws IOException
	{
		if (index < size)
			return buf.charAt(index++);
		return -1;
	}

	int size;
	int index;
	String buf;
}
