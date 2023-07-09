/*
 * @(#)StringHashtable.java 1.0 11/10/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
package com.ms.xml.util;

class Entry
{
	int		hash;
	String	key;
	Object	value;
	Entry	next;

	Entry(String key, Object value, int hash)
	{
		this.key = key;
		this.value = value;
		this.hash = hash;
	}
}

/**
 * This simple hashtable uses strings as the keys.
 *
 * @version 1.0, 6/3/97
 */
public class StringHashtable 
{
	Entry[]	entries;

    /**
     * Construct empty hashtable.
     */
	public StringHashtable()
	{
		this(13);
	}

    /**
     * Construct empty hashtable.
     */
	public StringHashtable(int size)
	{
		this.entries = new Entry[size];
	}

    /**
     * Add object to the hashtable.
     */
    public Object put(String key, Object value)
    {
		int hash = key.hashCode();
		int index = ((hash & 0x7FFFFFFF) % entries.length);
		for (Entry entry = entries[index]; entry != null; entry = entry.next)
		{
			if (entry.hash == hash && entry.key.equals(key))
			{
				Object o = entry.value;
				entry.value = value;
				return o;
			}
		}
		Entry entry = new Entry(key, value, hash);
		entry.next = entries[index];
		entries[index] = entry;
		return null;
    }

    /**
     * Get a value from the hashtable.
     */
    public Object get(String key)
    {
		int hash = key.hashCode();
		int index = ((hash & 0x7FFFFFFF) % entries.length);
		for (Entry entry = entries[index]; entry != null; entry = entry.next)
		{
			if (entry.hash == hash && entry.key.equals(key))
			{
				return entry.value;
			}
			
		}
		return null;
	}

    /**
     * Get a value from the hashtable.
     */
    public Object get(char chars[], int offset, int length)
    {
		// calculate hashcode the same way as the String class...
		int hash = 0;
		int off = offset;

		if (length < 16) 
		{
			for (int i = length; i > 0; i--) 
			{
				hash = (hash * 37) + chars[off++];
			}
		} 
		else 
		{
			// only sample some characters
			int skip = length / 8;
			for (int i = length; i > 0; i -= skip, off += skip) 
			{
				hash = (hash * 39) + chars[off];
			}
		}
		int index = ((hash & 0x7FFFFFFF) % entries.length);
		for (Entry entry = entries[index]; entry != null; entry = entry.next)
		{
nextEntry:	{
				if (entry.hash == hash)
				{
					String key = entry.key;
					if (key.length() == length) 
					{
						while(--length >= 0)
						{
							if (chars[offset + length] != key.charAt(length))
								break nextEntry;
						}
						return entry.value;
					}
				}
			}
		}
		return null;
	}
}    


