/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include <core.hxx>
#pragma hdrstop

#include "core/lang/stringbuffer.hxx"

DEFINE_CLASS_MEMBERS(StringBuffer, _T("StringBuffer"), Base);


// Constructs a string buffer with no characters in it and an initial capacity 
// specified by the length argument. 
void
StringBuffer::init(int capacity)
{
    _chars = new (capacity) ATCHAR;
}

StringBuffer * StringBuffer::newStringBuffer(int capacity)
{
    return new StringBuffer(capacity);
}


// Appends the string representation of the char argument to this string buffer. 
StringBuffer *
StringBuffer::append(TCHAR ch)
{
    ensureCapacity(_length + 1);

    (*_chars)[_length] = ch;
    ++ _length;

    return this;
}


// Appends the string representation of the char array argument to this string 
// buffer. 
StringBuffer *
StringBuffer::append(ATCHAR * str)
{
    int len = str->length();
    
    ensureCapacity(_length + len);

    // BUGBUG GC danger on str
    _chars->simpleCopy(_length, len, str->getData());
    _length += len;

    return this;
}


// Appends the string representation of a subarray of the char array argument to 
// this string buffer. 
StringBuffer *
StringBuffer::append(ATCHAR * str, int offset, int len)
{
    ensureCapacity(_length + len);

    // BUGBUG GC danger on str
    _chars->simpleCopy(_length, len, str->getData());
    _length += len;

    return this;
}


// Appends the string to this string buffer. 
StringBuffer *
StringBuffer::append(String * str)
{
    ensureCapacity(_length + str->length());

    str->getChars(0, str->length(), _chars, _length);
    _length += str->length();

    return this;
}


void
StringBuffer::ensureCapacity(int minCapacity)
{
    ATCHAR * newChars;
    
    if (minCapacity > _chars->length())
    {
        if (minCapacity < 2 * _chars->length() + 2)
            minCapacity = 2 * _chars->length() + 2;

        copy(minCapacity);
    }
    else
    {
        aboutToChange(_chars->length());
    }
}


void
StringBuffer::getChars(int srcBegin, int srcEnd, ATCHAR *dst, int dstBegin)
{
    if (srcBegin < srcEnd)
    {
        if (0<=srcBegin && srcEnd<=_length &&
            0<=dstBegin && dstBegin+srcEnd-srcBegin <= dst->length())
        {
            dst->simpleCopy(dstBegin, srcEnd-srcBegin, _chars->getData() + srcBegin);
        }
        else
        {
            Exception::throwE(E_INVALIDARG); // Exception::StringIndexOutOfBoundsException);
        }
    }
}


// Converts to a string representing the data in this string buffer. 
String *
StringBuffer::toString()
{
    _copyBeforeChange = true;
    return String::newString(this);
}


///////////////   non-Java methods   //////////////////////////////////////////

StringBuffer *
StringBuffer::append(TCHAR * str)
{
    int len = _tcslen(str);
    
    ensureCapacity(_length + len);

    _chars->simpleCopy(_length, len, str);
    _length += len;

    return this;
}


StringBuffer *
StringBuffer::append(TCHAR * str, int offset, int len)
{
    ensureCapacity(_length + len);

    _chars->simpleCopy(_length, len, str + offset);
    _length += len;

    return this;
}


StringBuffer * 
StringBuffer::append(abyte * str, int offset, int len)
{
    Assert(len % sizeof(TCHAR) == 0);

    len /= sizeof(TCHAR);
    ensureCapacity(_length + len);

    // BUGBUG GC danger on str
    _chars->simpleCopy(_length, len, (const TCHAR *) str->getData() + offset);
    _length += len;

    return this;
}


// use a new copy of the buffer.  Called when we're about to change a buffer
// that's shared with a String.
void
StringBuffer::copy(int newCapacity)
{
    ATCHAR *newChars = new (newCapacity) ATCHAR;

    newChars->simpleCopy(0, _length, _chars->getData());
    _chars = newChars;
    _copyBeforeChange = false;
}
