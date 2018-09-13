/*
 * @(#)StringInputStream.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS(StringInputStream, _T("StringInputStream"), InputStream);

StringInputStream * 
StringInputStream::newStringInputStream(String * text)
{
    StringInputStream * s = new StringInputStream();
    s->size = text->length();
    s->index = 0;
    s->buf = text;
    return s;
}

int StringInputStream::read() // throws IOException
{
    if (index < size)
        return buf->charAt(index++);
    return -1;
}

int StringInputStream::read(abyte * b, int off, int len) 
{ 
    if (len <= 4)
        return 0;

    int i = 0;

    // first return unicode byte order mark
    if (index == 0)
    {
        (*b)[i+off] = 0xFE; // high byte first
        len--;
        i++;
        (*b)[i+off] = 0xFF; // lo byte next
        len--;
        i++;
    }

    while (i < len)
    {
        int ch = read();
        if (ch == -1)
            break;
        (*b)[i+off] = (byte)(ch >> 8); 
        i++;
        (*b)[i+off] = (byte)ch;
        i++;
    }
    return i; 
};
