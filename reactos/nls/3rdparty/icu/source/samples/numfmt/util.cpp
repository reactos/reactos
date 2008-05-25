/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unistr.h"
#include "unicode/fmtable.h"
#include <stdio.h>
#include <stdlib.h>

enum {
    U_SPACE=0x20,
    U_DQUOTE=0x22,
    U_COMMA=0x2c,
    U_LEFT_SQUARE_BRACKET=0x5b,
    U_BACKSLASH=0x5c,
    U_RIGHT_SQUARE_BRACKET=0x5d,
    U_SMALL_U=0x75
};

// Verify that a UErrorCode is successful; exit(1) if not
void check(UErrorCode& status, const char* msg) {
    if (U_FAILURE(status)) {
        printf("ERROR: %s (%s)\n", u_errorName(status), msg);
        exit(1);
    }
    // printf("Ok: %s\n", msg);
}
                                                      
// Append a hex string to the target
static UnicodeString& appendHex(uint32_t number, 
                         int8_t digits, 
                         UnicodeString& target) {
    uint32_t digit;
    while (digits > 0) {
        digit = (number >> ((--digits) * 4)) & 0xF;
        target += (UChar)(digit < 10 ? 0x30 + digit : 0x41 - 10 + digit);
    }
    return target;
}

// Replace nonprintable characters with unicode escapes
UnicodeString escape(const UnicodeString &source) {
    int32_t i;
    UnicodeString target;
    target += (UChar)U_DQUOTE;
    for (i=0; i<source.length(); ++i) {
        UChar ch = source[i];
        if (ch < 0x09 || (ch > 0x0D && ch < 0x20) || ch > 0x7E) {
            (target += (UChar)U_BACKSLASH) += (UChar)U_SMALL_U;
            appendHex(ch, 4, target);
        } else {
            target += ch;
        }
    }
    target += (UChar)U_DQUOTE;
    return target;
}

// Print the given string to stdout using the UTF-8 converter
void uprintf(const UnicodeString &str) {
    char stackBuffer[100];
    char *buf = 0;

    int32_t bufLen = str.extract(0, 0x7fffffff, stackBuffer, sizeof(stackBuffer), "UTF-8");
    if(bufLen < sizeof(stackBuffer)) {
        buf = stackBuffer;
    } else {
        buf = new char[bufLen + 1];
        bufLen = str.extract(0, 0x7fffffff, buf, bufLen + 1, "UTF-8");
    }
    printf("%s", buf);
    if(buf != stackBuffer) {
        delete buf;
    }
}

// Create a display string for a formattable
UnicodeString formattableToString(const Formattable& f) {
    switch (f.getType()) {
    case Formattable::kDate:
        // TODO: Finish implementing this
        return UNICODE_STRING_SIMPLE("Formattable_DATE_TBD");
    case Formattable::kDouble:
        {
            char buf[256];
            sprintf(buf, "%gD", f.getDouble());
            return UnicodeString(buf, "");
        }
    case Formattable::kLong:
    case Formattable::kInt64:
        {
            char buf[256];
            sprintf(buf, "%ldL", f.getLong());
            return UnicodeString(buf, "");
        }
    case Formattable::kString:
        return UnicodeString((UChar)U_DQUOTE).append(f.getString()).append((UChar)U_DQUOTE);
    case Formattable::kArray:
        {
            int32_t i, count;
            const Formattable* array = f.getArray(count);
            UnicodeString result((UChar)U_LEFT_SQUARE_BRACKET);
            for (i=0; i<count; ++i) {
                if (i > 0) {
                    (result += (UChar)U_COMMA) += (UChar)U_SPACE;
                }
                result += formattableToString(array[i]);
            }
            result += (UChar)U_RIGHT_SQUARE_BRACKET;
            return result;
        }
    default:
        return UNICODE_STRING_SIMPLE("INVALID_Formattable");
    }
}
