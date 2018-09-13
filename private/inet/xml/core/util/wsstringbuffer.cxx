/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include <core.hxx>
#pragma hdrstop

#include "core/util/wsstringbuffer.hxx"

DEFINE_CLASS_MEMBERS(WSStringBuffer, _T("WSStringBuffer"), Base);

#define MAYBE_INSERT_WHITESPACE  \
if (_fCachedWS && !_fLeading) \
{\
    _append(L' ');\
}\
_fCachedWS = false;\
_fLeading = false;


// Constructs a string buffer with no characters in it and an initial capacity 
// specified by the length argument. 
void
WSStringBuffer::init(int capacity)
{
    _chars = new (capacity) ATCHAR;
    _fCachedWS = false;
    _fLeading = true;
}

WSStringBuffer * WSStringBuffer::newWSStringBuffer(int capacity)
{
    return new WSStringBuffer(capacity);
}


bool
WSStringBuffer::_collapsingAppend(TCHAR ch)
{
    if (!Character::isWhitespace(ch))
    {
        MAYBE_INSERT_WHITESPACE
        _append( ch);
    }
    else
        _fCachedWS = true;
    return !_fCachedWS;
}

WSStringBuffer *
WSStringBuffer::append(TCHAR ch, WSMODE eMode)
{
    ensureCapacity(_length + 1);

    switch (eMode)
    {
    case WS_COLLAPSE:
        if (_collapsingAppend( ch))
            _trimPoint = _length;
        break;

    case WS_PRESERVE:
    case WS_TRIM:
        {
            bool fTrimWS = false;
            if (WS_TRIM == eMode)
                fTrimWS = Character::isWhitespace(ch);
            if (_fLeading && fTrimWS)
                break;
            MAYBE_INSERT_WHITESPACE;
            _append( ch);
            if (!fTrimWS)
                _trimPoint = _length;
        }
        break;
    }

    return this;
}


// Appends the tchar to this string buffer. 
WSStringBuffer *
WSStringBuffer::append(const TCHAR * pch, int length, WSMODE eMode)
{
    Assert(pch || !length);
    int trimLen = length;
    ensureCapacity(_length + length + 1);

    switch (eMode)
    {
    case WS_COLLAPSE:
        if (pch)
        {
            int i = length;
            while (i-- > 0)
                _collapsingAppend( *pch++);
            _trimPoint = _length;
        }
        break;

    case WS_TRIM:
        if (_fLeading)
        {
            while (length && Character::isWhitespace(*pch))
            {
                ++pch;
                --length;
            }
            if (!length)
                break;
        }
        // keep track of tail trim
        {
            const TCHAR * tpch = pch + length;
            while (Character::isWhitespace(*(--tpch)))
                ;
            trimLen = (int)(tpch - pch) + 1;
            if (0 == trimLen)
            {
                _fCachedWS = true;
                break;
            }
        }
        // Fall Through !!
    case WS_PRESERVE:
        {
            MAYBE_INSERT_WHITESPACE;
            if (length)
            {
                _chars->simpleCopy(_length, length, pch);
                if (trimLen)
                    _trimPoint = _length + trimLen;
                _length += length;
            }
        }
        break;
    }

    return this;
}


// Appends the string to this string buffer. 
WSStringBuffer *
WSStringBuffer::append(const String * str, WSMODE eMode)
{
    if (str)
    {
        return append((const TCHAR *)str->getWCHARPtr(), str->length(), eMode);
    }

    return this;
}


// Appends the atchar to this string buffer. 
WSStringBuffer *
WSStringBuffer::append(const ATCHAR * chars, WSMODE eMode)
{
    if (chars)
    {
        // BUGBUG GC danger on str
        append(chars->getData(), chars->length(), eMode);
    }

    return this;
}


void
WSStringBuffer::ensureCapacity(int minCapacity)
{
    ATCHAR * newChars;

    if (_fCachedWS)
        minCapacity++;

    if (minCapacity > _chars->length())
    {
        if (minCapacity < 2 * _chars->length() + 2)
            minCapacity = 2 * _chars->length() + 2;

        copy(minCapacity);
    }
}


void
WSStringBuffer::copy(int newCapacity)
{
    ATCHAR *newChars = new (newCapacity) ATCHAR;
    if (_length)
        newChars->simpleCopy(0, _length, _chars->getData());
    _chars = newChars;
}


// Converts to a string representing the data in this string buffer. 
String *
WSStringBuffer::toString()
{
    String * pString = String::newString(_chars, 0, _trimPoint);
    _length = 0;
    _trimPoint = 0;
    _chars = null;
    return pString;
}

//  normalize() 
//  Convert newlines into spaces (fIE4 is true) or 0xA.
//  if fIE4 is true, also convert \t into space 
WSStringBuffer *
WSStringBuffer::normalize(bool fIE4)
{
    if (_length > 0)
    {
        TCHAR *pchOld = const_cast<TCHAR *>(_chars->getData());
        TCHAR *pchCurrent = pchOld, *pchNew = pchOld, *pchStart = pchOld;
        TCHAR ch, chLast = 0, chTarget = fIE4 ? ' ' : 0xA;
        int i, l = 0, k, t = 0;
        bool fMove = false;

        for (i = 0; i < _trimPoint; i++)
        {
            ch = *pchCurrent;
            switch (ch)
            {
            case 0xA:
                if (chLast == 0xD)
                {
                    k = (int)(pchCurrent - pchOld);
                    if (fMove && k)
                        ::memmove(pchNew, pchOld, k * sizeof(TCHAR));                        
                    pchNew += k;
                    pchOld = pchCurrent + 1;
                    fMove = true;
                    l++; // count the characters shrunk
                    t++;
                    break;
                }
                // fall through

            case 0xD:
                *pchCurrent = chTarget;
                break;

            case '\t':
                if (fIE4)
                    *pchCurrent = ' ';
                break;
                
            case ' ':
                break;

            default:
                t = 0;
            }
            chLast = ch;
            pchCurrent++;
        }

        if (fMove && pchOld != pchCurrent)
        {
            k = (int)(pchCurrent - pchOld);
            if (k)
                ::memmove(pchNew, pchOld, k * sizeof(TCHAR));  
        }

        _trimPoint -= l;
        *(pchStart + _trimPoint) = 0;
    }

    return this;
} 
