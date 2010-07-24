/*
**********************************************************************
*   Copyright (c) 2001-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/19/2001  aliu        Creation.
**********************************************************************
*/

#ifndef CHARSTRING_H
#define CHARSTRING_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "cmemory.h"

//--------------------------------------------------------------------
// class CharString
//
// This is a tiny wrapper class that is used internally to make a
// UnicodeString look like a const char*.  It can be allocated on the
// stack.  It only creates a heap buffer if it needs to.
//--------------------------------------------------------------------

U_NAMESPACE_BEGIN

class U_COMMON_API CharString : public UMemory {
public:

#if !UCONFIG_NO_CONVERSION
    // Constructor
    //     @param  str    The unicode string to be converted to char *
    //     @param  codepage   The char * code page.  ""   for invariant conversion.
    //                                               NULL for default code page.
//    inline CharString(const UnicodeString& str, const char *codepage);
#endif

    inline CharString(const UnicodeString& str);
    inline ~CharString();
    inline operator const char*() const { return ptr; }

private:
    char buf[128];
    char* ptr;

    CharString(const CharString &other); // forbid copying of this class
    CharString &operator=(const CharString &other); // forbid copying of this class
};

#if !UCONFIG_NO_CONVERSION

// PLEASE DON'T USE THIS FUNCTION.
// We don't want the static dependency on conversion or the performance hit that comes from a codepage conversion.
/*
inline CharString::CharString(const UnicodeString& str, const char *codepage) {
    int32_t    len;
    ptr = buf;
    len = str.extract(0, 0x7FFFFFFF, buf ,sizeof(buf)-1, codepage);
    if (len >= (int32_t)(sizeof(buf)-1)) {
        ptr = (char *)uprv_malloc(len+1);
        str.extract(0, 0x7FFFFFFF, ptr, len+1, codepage);
    }
}*/

#endif

inline CharString::CharString(const UnicodeString& str) {
    int32_t    len;
    ptr = buf;
    len = str.extract(0, 0x7FFFFFFF, buf, (int32_t)(sizeof(buf)-1), US_INV);
    if (len >= (int32_t)(sizeof(buf)-1)) {
        ptr = (char *)uprv_malloc(len+1);
        str.extract(0, 0x7FFFFFFF, ptr, len+1, US_INV);
    }
}

inline CharString::~CharString() {
    if (ptr != buf) {
        uprv_free(ptr);
    }
}

U_NAMESPACE_END

#endif
//eof
