/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unistr.h"
#include "unicode/fmtable.h"

// Verify that a UErrorCode is successful; exit(1) if not
void check(UErrorCode& status, const char* msg);

// Replace nonprintable characters with unicode escapes
UnicodeString escape(const UnicodeString &source);

// Print the given string to stdout
void uprintf(const UnicodeString &str);

// Create a display string for a formattable
UnicodeString formattableToString(const Formattable& f);
