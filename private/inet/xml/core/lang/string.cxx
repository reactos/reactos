/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#ifndef _CORE_BASE_VMM
#include "core/base/vmm.hxx"
#endif

#ifndef _CORE_BASE_SLOT
#include "core/base/slot.hxx"
#endif

#include <shlwapi.h>

DEFINE_CLASS_MEMBERS_NEWINSTANCE(String, _T("String"), Base);


SRString String::s_nullString;
SRString String::s_emptyString;
SRString String::s_newLineString;

String *
String::nullString()
{
    return s_nullString;
}

String *
String::emptyString()
{
    return s_emptyString;
}

String *
String::newLineString()
{
    return s_newLineString;
}

void 
String::classInit()
{
    if (!String::s_nullString)
        String::s_nullString = String::newString(_T("null"));
    if (!String::s_emptyString)
        String::s_emptyString = String::newString(_T(""));
    if (!String::s_newLineString)
        String::s_newLineString = String::newString(_T("\r\n"));
}

/**
 * Init function
 */
void
String::init(const ATCHAR * a, int offset, int length)
{
    _offset = offset;
    _length = length;
    if (a == null)
        _chars = new (length) ATCHAR;
    else
        _chars = const_cast<ATCHAR *>(a);
}

////////////////  Private helper functions ////////////////////////////////////

void
String::init(const TCHAR * c, int offset, int length)
{
    _offset = 0;
    _length = length;
    _chars = new (_length) ATCHAR;
    if (length)
        memcpy( getDataNC(), c + offset, _length * sizeof(TCHAR) );
}

String * 
String::newString(const ATCHAR * value)
{
    return new String(value, 0, value->length());
}

String * 
String::newString(const ATCHAR * value, int offset, int count)
{
    return new String(value, offset, count);
}

String * 
String::newString(const String * value)
{
    return new String(value->_chars, value->_offset, value->_length);
}

// Allocates a new string that contains the sequence of characters currently 
// contained in the string buffer argument. 
String * 
String::newString(StringBuffer * buffer)
{
    String * s = new String((ATCHAR *)null, 0, buffer->length());
    buffer->getChars(0, s->_length, s->_chars, 0);
    return s;
}

String * 
String::newString(const TCHAR * c)
{
    if (c)
        return new String(c, 0, _tcslen(c));
    else
        return String::emptyString();
}


String * 
String::newString(const TCHAR * c, int offset, int length)
{
    return new String(c, offset, length);
}

String * 
String::newString(int c)
{
    TCHAR buffer[32];
    
    _itot(c, buffer, 10);
    return new String(buffer, 0, _tcslen(buffer));
}


String *
String::newString(TCHAR c)
{
	return new String(&c, 0, 1);
}


String * 
String::newString(const char * c)
{
    int len = strlen(c);
    
    int length = MultiByteToWideChar(CP_ACP, 0, c, len, NULL, 0);
    String * s = new String((ATCHAR *)null, 0, length);
    MultiByteToWideChar(CP_ACP, 0, c, len, s->getDataNC() + s->_offset, length);

    return s;
}


String *
String::copyString()
{
    return new String(_chars->getData() + _offset, 0, _length);
}


////////////////////////  Java methods  ///////////////////////////////////////

// Returns the character at the specified index. 
TCHAR
String::charAt(int index)
{
    if (0<=index  &&  index<_length)
    {
        return *(_chars->getData() + _offset + index);
    }
    else
    {
        Exception::throwE(E_INVALIDARG); // Exception::StringIndexOutOfBoundsException);
        return null;
    }
}


// Compares this string to the specified object. 
bool
String::equals(Object * anObject)
{
    if (!String::_getClass()->isInstance(anObject))
    {
        return false;
    }

    else
    {
        // BUGBUG use memcmp
        String * aString = ICAST_TO(String *, anObject);
        return (_length == aString->length() &&
                memcmp(getData() + _offset,
                       aString->getData() + aString->_offset,
                       _length * sizeof(TCHAR)) == 0
                );
    }
}


//  compare(String)
// Compares this string to the specified object.
int
String::compare(String * strThat)
{
    return compare(strThat->getWCHARPtr(), strThat->length());
}

//  compare(TCHAR *, int)
// Compares this string to the specified string.
// BUGBUG combine with String::equals(Object * anObject)
int 
String::compare(const TCHAR * pwch, int cch)
{
    int cchThis = length();
    int cchThat = (cch < 0) ? _tcslen(pwch) : cch;

    // compare strings up to the length of the shortest
    // BUGBUG this function needs to support locale-based compares (StrCmpN does not)
    return StrCmpN( getData(), pwch, ((cchThis <= cchThat) ? cchThis : cchThat) );
}



// Compares this String to another String 
bool
String::equalsIgnoreCase(String * aString)
{
    // BUGBUG use shlwapi
    return (_length == aString->length() &&
            _tcsnicmp(getData() + _offset,
                      aString->getData() + aString->_offset,
                      _length) == 0
            );
}


// Copies characters from this string into the destination character array. 
void
String::getChars(int srcBegin, int srcEnd, ATCHAR * dst, int dstBegin) const
{
    if (srcBegin < srcEnd)
    {
        if (0<=srcBegin && srcEnd<=_length &&
            0<=dstBegin && dstBegin+srcEnd-srcBegin <= dst->length())
        {
            dst->simpleCopy(dstBegin, srcEnd - srcBegin,
                            getData() + _offset + srcBegin);
        }
        else
        {
            Exception::throwE(E_INVALIDARG); // Exception::StringIndexOutOfBoundsException);
        }
    }
}


// Returns a hashcode for this string. 
int
String::hashCode()
{
    return hashCode(getData() + _offset, _length);
}


// Returns the index within this string of the first occurrence of the specified 
// character, starting the search at the specified index. 
int
String::indexOf(int ch, int fromIndex)
{
    int i;
    const TCHAR *s;

    // BUGBUG is there a _tcs... function for this?  (or shlwapi)
    if (fromIndex < 0)
        fromIndex = 0;
    
    s = getData() + _offset + fromIndex;

    for (i=fromIndex; i<_length; ++i, ++s)
    {
        if (*s == ch)
            break;
    }

    return (i<_length) ? i : -1;
}


// Returns the index within this string of the last occurrence of the specified 
// character, searching backward starting at the specified index. 
int
String::lastIndexOf(int ch, int fromIndex)
{
    int i;
    const TCHAR *s;

    if (fromIndex >= _length)
        fromIndex = _length - 1;
    
    s = getData() + _offset + fromIndex;

    for (i=fromIndex; i>=0; --i, --s)
    {
        if (*s == ch)
            break;
    }

    return (i>=0) ? i : -1;
}


// Returns a new string resulting from replacing all occurrences of oldChar in 
// this string with newChar. 
String *
String::replace(TCHAR oldChar, TCHAR newChar)
{
    int i;
    String *result = String::newString(getData() + _offset, 0, _length);   // copies
    TCHAR *s = result->getDataNC();

    for (i=0; i<_length; ++i, ++s)
    {
        if (*s == oldChar)
        {
            *s = newChar;
        }
    }

    return result;
}


// Tests if this string starts with the specified prefix. 
bool
String::startsWith(String * prefix, int toffset)
{
    return (0<=toffset && toffset + prefix->length() <= length() &&
            memcmp(getData() + _offset + toffset,
                     prefix->getData() + prefix->_offset,
                     prefix->length() * sizeof(TCHAR)) == 0
            );
}


// Returns a new string that is a substring of this string. 
String *
String::substring(int beginIndex, int endIndex)
{
    String *result;

    if (0<=beginIndex && beginIndex<=endIndex && endIndex<=_length)
    {
        result = String::newString(_chars, 
                            _offset + beginIndex,       // shared
                            endIndex - beginIndex);
    }
    else
    {
        Exception::throwE(E_INVALIDARG); // Exception::StringIndexOutOfBoundsException);
        result = null;
    }

    return result;
}


// Converts this String to lowercase. 
String *
String::toLowerCase()
{
    TCHAR *s = getDataNC() + _offset;
    int i;
    String *result;

    // special case:  I'm already in lower case
    for (i=0; i<_length; ++i, ++s)
    {
        if (*s != Character::toLowerCase(*s))
            break;
    }

    if (i == _length)
        return this;

    // general case:  copy myself and convert the characters
    result = String::newString(getData(), _offset, _length);
    s = result->getDataNC();

    for (i=0; i<_length; ++i, ++s)
    {
        *s = Character::toLowerCase(*s);
    }

    return result;
}


// Converts this string to uppercase. 
String *
String::toUpperCase()
{
    TCHAR *s = getDataNC() + _offset;
    int i;
    String *result;

    // special case:  I'm already in upper case
    for (i=0; i<_length; ++i, ++s)
    {
        if (*s != Character::toUpperCase(*s))
            break;
    }

    if (i == _length)
        return this;

    // general case:  copy myself and convert the characters
    result = String::newString(getData(), _offset, _length);
    s = result->getDataNC();

    for (i=0; i<_length; ++i, ++s)
    {
        *s = Character::toUpperCase(*s);
    }

    return result;
}


////////////////////////  non-Java methods  ///////////////////////////////////

String * __cdecl
String::add(String *s1, ...)
{
    String *s;
    int len = 0;
    va_list arglist;
    ATCHAR *buffer;

    // calculate the length of the result
    va_start(arglist, s1);
    for (s = s1;  s;  s = va_arg(arglist, String *))
    {
        len += s->length();
    }
    va_end(arglist);

    // create a buffer of the desired length and fill it in
    buffer = new (len) ATCHAR;
    len = 0;
    va_start(arglist, s1);
    for (s = s1;  s;  s = va_arg(arglist, String *))
    {
        s->getChars(0, s->length(), buffer, len);
        len += s->length();
    }
    va_end(arglist);

    // finally, return the resulting string
    return String::newString(buffer);
}


bool
String::equals(const TCHAR *s)
{
    long l = _tcslen( s);
    return s != null &&
           _length == l &&
           memcmp(getData() + _offset, s, _length * sizeof(TCHAR)) == 0;
}


bool
String::equals(const TCHAR *s, int length)
{
    return s != null && length == _length &&
           memcmp(getData() + _offset, s, length * sizeof(TCHAR)) == 0;
}


bool
String::equalsIgnoreCase(const TCHAR *s)
{
    // BUGBUG shlwapi
    long l = _tcslen( s);
    return s != null &&
           _length == l &&
           _tcsnicmp(getData() + _offset, s, _length) == 0;
}


BSTR
String::getBSTR() const
{
    BSTR bstr = SysAllocStringLen(getData() + _offset, _length);
    return bstr;
}


int
String::hashCode(const TCHAR * s, int length)
{
    int result = 0;

    for (; length>0; --length, ++s)
    {
        result = result * 113 + (*s);
    }

    return result;
}


String * 
String::trim() 
{
    int srcBegin, srcEnd;
    const TCHAR *str = getData() + _offset;

    for (srcEnd=_length; srcEnd>0; --srcEnd)
    {
        if (!Character::isWhitespace(str[srcEnd-1]))
            break;
    }
    
    for (srcBegin=0; srcBegin<srcEnd; ++srcBegin)
    {
        if (!Character::isWhitespace(str[srcBegin]))
            break;
    }

    if (srcBegin > 0 || srcEnd < _length)
        return substring(srcBegin, srcEnd);
    else
        return this;
}


bool 
String::isWhitespace() 
{
    const WCHAR * pchStart = getWCHARPtr();
    const WCHAR * pchEnd = SkipWhiteSpace(pchStart, _length);
    return (((int)(pchEnd - pchStart)) == _length);
}


String * String::valueOf(TCHAR c)
{
    ATCHAR * data = new (1) ATCHAR;
    (*data)[0] = c;
    return String::newString(data, 0, 1);
}


String * String::valueOf(int i)
{
    TCHAR buf[32];
    _itot(i, buf, 10);
    return String::newString(buf);
}


ATCHAR *
String::toCharArray() const
{
    ATCHAR *result = new (_length) ATCHAR;

    getChars(0, _length, result, 0);
    return result;
}


const ATCHAR *
String::getCharArray() const
{
    const ATCHAR * chars;
    if (_offset == 0 && _length == _chars->length())
        chars = _chars;
    else
        chars = toCharArray();
    return chars;
}


ATCHAR *
String::toCharArrayZ() const
{
    ATCHAR *result = new (_length + 1) ATCHAR;

    getChars(0, _length, result, 0);
    (*result)[_length] = 0;
    return result;
}

const WCHAR * 
String::getWCHARPtr() const
{ 
    return _chars ? (_chars->getData() + _offset) : NULL; 
}

WCHAR * 
String::getWCHARBuffer() const
{ 
    WCHAR * buffer = new WCHAR[_length + 1];
    if (_length)
        memcpy((void *)buffer, (void *)getWCHARPtr(), sizeof(WCHAR)*_length);
    buffer[_length] = 0;
    return buffer; 
}

const char * AsciiText::_acnull = "null";

AsciiText::AsciiText(Base * obj) 
{
    if (obj == null)
    {
        _pac = (char *)_acnull;
        _uLen = 4;
    }
    else
    {
        // initialize pointer to null
        _pac = null;

        String * s = obj->toString();
        // addref it so it doesn't go away while looking at the data...
        s->AddRef();
        const TCHAR * pc = s->getData() + s->_offset;

        _uLen = WideCharToMultiByte(CP_ACP, 0,
                                    pc, s->_length,
                                    NULL, 0, NULL, NULL);
        if (_uLen < LENGTH(_acbuff) - 1)
        {
            _pac = _acbuff;
        }
        else
        {
            _pac = new_ne char [_uLen + 1];
        }
        if (_pac)
        {
            WideCharToMultiByte(CP_ACP, 0,
                                pc, s->_length,
                                _pac, _uLen, NULL, NULL);
            _pac [_uLen] = 0;
        }
        s->Release();
        // if allocation failed throw here after releasing the string object
        if (!_pac)
            Exception::throwEOutOfMemory();
    }
}


AsciiText::~AsciiText()
{
    if (_pac != _acnull && _pac != _acbuff)
        delete [] _pac;
}


AString *
String::split(TCHAR splitchar)
{
    AString *result;
    int count;
    int i, j;
    
    // Count the number of strings
    count = 1;
    i = indexOf(splitchar);
    while (i > 0)
    {
        ++count;
        i = indexOf(splitchar, i+1);
    }

    // Allocate array and populate
    result = new (count) AString;
    i = indexOf(splitchar);
    j = 0;
    count = 0;
    while (i > 0)
    {
        (*result)[count++] = substring(j, i);
        j = i+1;
        i = indexOf(splitchar, j);
    }
    (*result)[count] = substring(j, length());
    
    return result;
}


String*
String::join(AString* array, const TCHAR joinchar, int start, int end)
{
    int len = 0;
    int i;
    ATCHAR * buf;
    
    if (start == -1) start = 0;
    if (end == -1) end = array->length();

    // Count string lengths .
    for (i = start; i < end; ++i)
    {
        String* s = (*array)[i];
        len += s->length();
        if (i < end-1)
            ++len;
    }

    // allocate buffer and fill it.
    buf = new (len) ATCHAR;
    len = 0;
    for (i = start; i < end; i++)
    {
        String* s = (*array)[i];
        s->getChars(0, s->length(), buf, len);
        len += s->length();
        if (i < end-1)
            (*buf)[len++] = joinchar;
    }
    return String::newString(buf);
}

void 
String::copyData(TCHAR * pchBuf, int iBuf)
{
    int i = _length > iBuf ? iBuf : _length;

    memcpy(pchBuf, _chars->getData() + _offset, i * sizeof(TCHAR));
}
