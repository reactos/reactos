/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/translit.h"
#include "unicode/rbt.h"
#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "unaccent.h"

// RuleBasedTransliterator rules to remove accents from characters
// so they can be displayed as ASCIIx
UnicodeString UNACCENT_RULES(
    "[\\u00C0-\\u00C5] > A;"
    "[\\u00C8-\\u00CB] > E;"
    "[\\u00CC-\\u00CF] > I;"
    "[\\u00E0-\\u00E5] > a;"
    "[\\u00E8-\\u00EB] > e;"
    "[\\u00EC-\\u00EF] > i;"
    );

int main(int argc, char **argv) {

    Calendar *cal;
    DateFormat *fmt;
    DateFormat *defFmt;
    Transliterator *greek_latin;
    Transliterator *rbtUnaccent;
    UErrorCode status = U_ZERO_ERROR;
    Locale greece("el", "GR");
    UnicodeString str, str2;

    // Create a calendar in the Greek locale
    cal = Calendar::createInstance(greece, status);
    check(status, "Calendar::createInstance");

    // Create a formatter
    fmt = DateFormat::createDateInstance(DateFormat::kFull, greece);
    fmt->setCalendar(*cal);

    // Create a default formatter
    defFmt = DateFormat::createDateInstance(DateFormat::kFull);
    defFmt->setCalendar(*cal);

    // Create a Greek-Latin Transliterator
    greek_latin = Transliterator::createInstance("Greek-Latin");
    if (greek_latin == 0) {
        printf("ERROR: Transliterator::createInstance() failed\n");
        exit(1);
    }

    // Create a custom Transliterator
    rbtUnaccent = new RuleBasedTransliterator("RBTUnaccent",
                                              UNACCENT_RULES,
                                              UTRANS_FORWARD,
                                              status);
    check(status, "RuleBasedTransliterator::ct");

    // Loop over various months
    for (int32_t month = Calendar::JANUARY;
         month <= Calendar::DECEMBER;
         ++month) {

        // Set the calendar to a date
        cal->clear();
        cal->set(1999, month, 4);
        
        // Format the date in default locale
        str.remove();
        defFmt->format(cal->getTime(status), str, status);
        check(status, "DateFormat::format");
        printf("Date: ");
        uprintf(escape(str));
        printf("\n");
        
        // Format the date for Greece
        str.remove();
        fmt->format(cal->getTime(status), str, status);
        check(status, "DateFormat::format");
        printf("Greek formatted date: ");
        uprintf(escape(str));
        printf("\n");
        
        // Transliterate result
        greek_latin->transliterate(str);
        printf("Transliterated via Greek-Latin: ");
        uprintf(escape(str));
        printf("\n");
        
        // Transliterate result
        str2 = str;
        rbtUnaccent->transliterate(str);
        printf("Transliterated via RBT unaccent: ");
        uprintf(escape(str));
        printf("\n\n");
    }

    // Clean up
    delete fmt;
    delete cal;
    delete greek_latin;
    delete rbtUnaccent;

    printf("Exiting successfully\n");
    return 0;
}
