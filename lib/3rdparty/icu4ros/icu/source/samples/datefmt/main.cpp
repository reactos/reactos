/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

/**
 * If the ID supplied to TimeZone is not a valid system ID,
 * TimeZone::createTimeZone() will return a GMT zone object.  In order
 * to detect this error, we check the ID of the returned zone against
 * the ID we requested.  If they don't match, we fail with an error.
 */
TimeZone* createZone(const UnicodeString& id) {
    UnicodeString str;
    TimeZone* zone = TimeZone::createTimeZone(id);
    if (zone->getID(str) != id) {
        delete zone;
        printf("Error: TimeZone::createTimeZone(");
        uprintf(id);
        printf(") returned zone with ID ");
        uprintf(str);
        printf("\n");
        exit(1);
    }
    return zone;
}

int main(int argc, char **argv) {

    Calendar *cal;
    TimeZone *zone;
    DateFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    UDate date;

    // The languages in which we will display the date
    static char* LANGUAGE[] = {
        "en", "de", "fr"
    };
    static const int32_t N_LANGUAGE = sizeof(LANGUAGE)/sizeof(LANGUAGE[0]);

    // The time zones in which we will display the time
    static char* TIMEZONE[] = {
        "America/Los_Angeles",
        "America/New_York",
        "Europe/Paris",
        "Europe/Berlin"
    };
    static const int32_t N_TIMEZONE = sizeof(TIMEZONE)/sizeof(TIMEZONE[0]);

    // Create a calendar
    cal = Calendar::createInstance(status);
    check(status, "Calendar::createInstance");
    zone = createZone("GMT"); // Create a GMT zone
    cal->adoptTimeZone(zone);
    cal->clear();
    cal->set(1999, Calendar::JUNE, 4);
    date = cal->getTime(status);
    check(status, "Calendar::getTime");

    for (int32_t i=0; i<N_LANGUAGE; ++i) {
        Locale loc(LANGUAGE[i]);

        // Create a formatter for DATE and TIME
        fmt = DateFormat::createDateTimeInstance(
                                DateFormat::kFull, DateFormat::kFull, loc);

        for (int32_t j=0; j<N_TIMEZONE; ++j) {

            cal->adoptTimeZone(createZone(TIMEZONE[j]));
            fmt->setCalendar(*cal);

            // Format the date
            str.remove();
            fmt->format(date, str, status);
            
            // Display the formatted date string
            printf("Date (%s, %s): ", LANGUAGE[i], TIMEZONE[j]);
            uprintf(escape(str));
            printf("\n\n");
        }

        delete fmt;
    }

    printf("Exiting successfully\n");
    return 0;
}
