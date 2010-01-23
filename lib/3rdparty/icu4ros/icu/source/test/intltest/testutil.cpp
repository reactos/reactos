/*
**********************************************************************
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/

#include "unicode/unistr.h"
#include "testutil.h"

static const UChar HEX[16]={48,49,50,51,52,53,54,55,56,57,65,66,67,68,69,70};

UnicodeString TestUtility::hex(UChar ch) {
    UnicodeString buf;
    buf.append(HEX[0xF&(ch>>12)]);
    buf.append(HEX[0xF&(ch>>8)]);
    buf.append(HEX[0xF&(ch>>4)]);
    buf.append(HEX[0xF&ch]);
    return buf;
}

UnicodeString TestUtility::hex(const UnicodeString& s) {
    return hex(s, 44 /*,*/);
}

UnicodeString TestUtility::hex(const UnicodeString& s, UChar sep) {
    if (s.length() == 0) return "";
    UnicodeString result = hex(s.charAt(0));
    for (int32_t i = 1; i < s.length(); ++i) {
        result.append(sep);
        result.append(hex(s.charAt(i)));
    }
    return result;
}
