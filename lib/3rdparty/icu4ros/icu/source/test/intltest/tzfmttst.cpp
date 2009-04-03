/*
*******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tzfmttst.h"

#include "unicode/timezone.h"
#include "unicode/simpletz.h"
#include "unicode/calendar.h"
#include "unicode/strenum.h"
#include "unicode/smpdtfmt.h"
#include "unicode/uchar.h"
#include "unicode/basictz.h"
#include "cstring.h"
#include "zonemeta.h"

#define DEBUG_ALL 0

static const char* PATTERNS[] = {"z", "zzzz", "Z", "ZZZZ", "v", "vvvv", "V", "VVVV"};
static const int NUM_PATTERNS = sizeof(PATTERNS)/sizeof(const char*);

void
TimeZoneFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) {
        logln("TestSuite TimeZoneFormatTest");
    }
    switch (index) {
        TESTCASE(0, TestTimeZoneRoundTrip);
        TESTCASE(1, TestTimeRoundTrip);
        default: name = ""; break;
    }
}

void
TimeZoneFormatTest::TestTimeZoneRoundTrip(void) {
    UErrorCode status = U_ZERO_ERROR;

    SimpleTimeZone unknownZone(-31415, (UnicodeString)"Etc/Unknown");
    int32_t badDstOffset = -1234;
    int32_t badZoneOffset = -2345;

    int32_t testDateData[][3] = {
        {2007, 1, 15},
        {2007, 6, 15},
        {1990, 1, 15},
        {1990, 6, 15},
        {1960, 1, 15},
        {1960, 6, 15},
    };

    Calendar *cal = Calendar::createInstance(TimeZone::createTimeZone((UnicodeString)"UTC"), status);
    if (U_FAILURE(status)) {
        errln("Calendar::createInstance failed");
        return;
    }

    // Set up rule equivalency test range
    UDate low, high;
    cal->set(1900, UCAL_JANUARY, 1);
    low = cal->getTime(status);
    cal->set(2040, UCAL_JANUARY, 1);
    high = cal->getTime(status);
    if (U_FAILURE(status)) {
        errln("getTime failed");
        return;
    }

    // Set up test dates
    UDate DATES[(sizeof(testDateData)/sizeof(int32_t))/3];
    const int32_t nDates = (sizeof(testDateData)/sizeof(int32_t))/3;
    cal->clear();
    for (int32_t i = 0; i < nDates; i++) {
        cal->set(testDateData[i][0], testDateData[i][1], testDateData[i][2]);
        DATES[i] = cal->getTime(status);
        if (U_FAILURE(status)) {
            errln("getTime failed");
            return;
        }
    }

    // Set up test locales
    const Locale locales1[] = {
        Locale("en_US")
    };
    const Locale locales2[] = {
        Locale("en_US"),
        Locale("en"),
        Locale("en_CA"),
        Locale("fr"),
        Locale("zh_Hant")
    };

    const Locale *LOCALES;
    int32_t nLocales;
    if (DEBUG_ALL) {
        LOCALES = Locale::getAvailableLocales(nLocales);
    } else if (quick) {
        LOCALES = locales1;
        nLocales = sizeof(locales1)/sizeof(Locale);
    } else {
        LOCALES = locales2;
        nLocales = sizeof(locales2)/sizeof(Locale);
    }

    StringEnumeration *tzids = TimeZone::createEnumeration();
    if (U_FAILURE(status)) {
        errln("tzids->count failed");
        return;
    }

    int32_t inRaw, inDst;
    int32_t outRaw, outDst;

    // Run the roundtrip test
    for (int32_t locidx = 0; locidx < nLocales; locidx++) {
        for (int32_t patidx = 0; patidx < NUM_PATTERNS; patidx++) {

            //DEBUG static const char* PATTERNS[] = {"z", "zzzz", "Z", "ZZZZ", "v", "vvvv", "V", "VVVV"};
            //if (patidx != 1) continue;

            SimpleDateFormat *sdf = new SimpleDateFormat((UnicodeString)PATTERNS[patidx], LOCALES[locidx], status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"new SimpleDateFormat failed for pattern " +
                    PATTERNS[patidx] + " for locale " + LOCALES[locidx].getName());
                status = U_ZERO_ERROR;
                continue;
            }

            tzids->reset(status);
            const UnicodeString *tzid;
            while ((tzid = tzids->snext(status))) {
                TimeZone *tz = TimeZone::createTimeZone(*tzid);

                for (int32_t datidx = 0; datidx < nDates; datidx++) {
                    UnicodeString tzstr;
                    FieldPosition fpos(0);
                    // Format
                    sdf->setTimeZone(*tz);
                    sdf->format(DATES[datidx], tzstr, fpos);

                    // Before parse, set unknown zone to SimpleDateFormat instance
                    // just for making sure that it does not depends on the time zone
                    // originally set.
                    sdf->setTimeZone(unknownZone);

                    // Parse
                    ParsePosition pos(0);
                    Calendar *outcal = Calendar::createInstance(unknownZone, status);
                    if (U_FAILURE(status)) {
                        errln("Failed to create an instance of calendar for receiving parse result.");
                        status = U_ZERO_ERROR;
                        continue;
                    }
                    outcal->set(UCAL_DST_OFFSET, badDstOffset);
                    outcal->set(UCAL_ZONE_OFFSET, badZoneOffset);

                    sdf->parse(tzstr, *outcal, pos);

                    // Check the result
                    const TimeZone &outtz = outcal->getTimeZone();
                    UnicodeString outtzid;
                    outtz.getID(outtzid);

                    tz->getOffset(DATES[datidx], false, inRaw, inDst, status);
                    if (U_FAILURE(status)) {
                        errln((UnicodeString)"Failed to get offsets from time zone" + *tzid);
                        status = U_ZERO_ERROR;
                    }
                    outtz.getOffset(DATES[datidx], false, outRaw, outDst, status);
                    if (U_FAILURE(status)) {
                        errln((UnicodeString)"Failed to get offsets from time zone" + outtzid);
                        status = U_ZERO_ERROR;
                    }

                    // Check if localized GMT format or RFC format is used.
                    int32_t numDigits = 0;
                    for (int n = 0; n < tzstr.length(); n++) {
                        if (u_isdigit(tzstr.charAt(n))) {
                            numDigits++;
                        }
                    }
                    if (numDigits >= 4) {
                        // Localized GMT or RFC: total offset (raw + dst) must be preserved.
                        int32_t inOffset = inRaw + inDst;
                        int32_t outOffset = outRaw + outDst;
                        if (inOffset != outOffset) {
                            errln((UnicodeString)"Offset round trip failed; tz=" + *tzid
                                + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                + ", time=" + DATES[datidx] + ", str=" + tzstr
                                + ", inOffset=" + inOffset + ", outOffset=" + outOffset);
                        }
                    } else if (uprv_strcmp(PATTERNS[patidx], "z") == 0 || uprv_strcmp(PATTERNS[patidx], "zzzz") == 0
                            || uprv_strcmp(PATTERNS[patidx], "v") == 0 || uprv_strcmp(PATTERNS[patidx], "vvvv") == 0
                            || uprv_strcmp(PATTERNS[patidx], "V") == 0) {
                        // Specific or generic: raw offset must be preserved.
                        if (inRaw != outRaw) {
                            errln((UnicodeString)"Raw offset round trip failed; tz=" + *tzid
                                + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                + ", time=" + DATES[datidx] + ", str=" + tzstr
                                + ", inRawOffset=" + inRaw + ", outRawOffset=" + outRaw);
                        }
                    } else { // "VVVV"
                        // Location: time zone rule must be preserved.
                        UnicodeString canonical;
                        ZoneMeta::getCanonicalID(*tzid, canonical);
                        if (outtzid != canonical) {
                            // Canonical ID did not match - check the rules
                            if (!((BasicTimeZone*)&outtz)->hasEquivalentTransitions((BasicTimeZone&)*tz, low, high, TRUE, status)) {
                                errln("Canonical round trip failed; tz=" + *tzid
                                    + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                    + ", time=" + DATES[datidx] + ", str=" + tzstr
                                    + ", outtz=" + outtzid);
                            }
                            if (U_FAILURE(status)) {
                                errln("hasEquivalentTransitions failed");
                                status = U_ZERO_ERROR;
                            }
                        }
                    }
                    delete outcal;
                }
                delete tz;
            }
            delete sdf;
        }
    }
    delete cal;
    delete tzids;
}

void
TimeZoneFormatTest::TestTimeRoundTrip(void) {
    UErrorCode status = U_ZERO_ERROR;

    Calendar *cal = Calendar::createInstance(TimeZone::createTimeZone((UnicodeString)"UTC"), status);
    if (U_FAILURE(status)) {
        errln("Calendar::createInstance failed");
        return;
    }

    UDate START_TIME, END_TIME;

    if (DEBUG_ALL) {
        cal->set(1900, UCAL_JANUARY, 1);
    } else {
        cal->set(1965, UCAL_JANUARY, 1);
    }
    START_TIME = cal->getTime(status);

    cal->set(2015, UCAL_JANUARY, 1);
    END_TIME = cal->getTime(status);
    if (U_FAILURE(status)) {
        errln("getTime failed");
        return;
    }

    // Whether each pattern is ambiguous at DST->STD local time overlap
    UBool AMBIGUOUS_DST_DECESSION[] = {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE};
    // Whether each pattern is ambiguous at STD->STD/DST->DST local time overlap
    UBool AMBIGUOUS_NEGATIVE_SHIFT[] = {TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE};

    UnicodeString BASEPATTERN("yyyy-MM-dd'T'HH:mm:ss.SSS");

    // timer for performance analysis
    UDate timer;
    UDate times[NUM_PATTERNS];
    for (int32_t i = 0; i < NUM_PATTERNS; i++) {
        times[i] = 0;
    }

    UBool REALLY_VERBOSE = FALSE;

    // Set up test locales
    const Locale locales1[] = {
        Locale("en_US")
    };
    const Locale locales2[] = {
        Locale("en_US"),
        Locale("en"),
        Locale("de_DE"),
        Locale("es_ES"),
        Locale("fr_FR"),
        Locale("it_IT"),
        Locale("ja_JP"),
        Locale("ko_KR"),
        Locale("pt_BR"),
        Locale("zh_Hans_CN"),
        Locale("zh_Hant_TW")
    };

    const Locale *LOCALES;
    int32_t nLocales;
    if (DEBUG_ALL) {
        LOCALES = Locale::getAvailableLocales(nLocales);
    } else if (quick) {
        LOCALES = locales1;
        nLocales = sizeof(locales1)/sizeof(Locale);
    } else {
        LOCALES = locales2;
        nLocales = sizeof(locales2)/sizeof(Locale);
    }

    StringEnumeration *tzids = TimeZone::createEnumeration();
    if (U_FAILURE(status)) {
        errln("tzids->count failed");
        return;
    }

    int32_t testCounts = 0;
    UDate testTimes[4];
    UBool expectedRoundTrip[4];
    int32_t testLen = 0;

    for (int32_t locidx = 0; locidx < nLocales; locidx++) {
        logln((UnicodeString)"Locale: " + LOCALES[locidx].getName());

        for (int32_t patidx = 0; patidx < NUM_PATTERNS; patidx++) {
            logln((UnicodeString)"    pattern: " + PATTERNS[patidx]);

            //DEBUG static const char* PATTERNS[] = {"z", "zzzz", "Z", "ZZZZ", "v", "vvvv", "V", "VVVV"};
            //if (patidx != 1) continue;

            UnicodeString pattern(BASEPATTERN);
            pattern.append(" ").append(PATTERNS[patidx]);

            SimpleDateFormat *sdf = new SimpleDateFormat(pattern, LOCALES[locidx], status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"new SimpleDateFormat failed for pattern " +
                    pattern + " for locale " + LOCALES[locidx].getName());
                status = U_ZERO_ERROR;
                continue;
            }

            tzids->reset(status);
            const UnicodeString *tzid;

            timer = Calendar::getNow();

            while ((tzid = tzids->snext(status))) {
                UnicodeString canonical;
                ZoneMeta::getCanonicalID(*tzid, canonical);
                if (*tzid != canonical) {
                    // Skip aliases
                    continue;
                }
                BasicTimeZone *tz = (BasicTimeZone*)TimeZone::createTimeZone(*tzid);
                sdf->setTimeZone(*tz);

                UDate t = START_TIME;
                TimeZoneTransition tzt;
                UBool tztAvail = FALSE;
                UBool middle = TRUE;

                while (t < END_TIME) {
                    if (!tztAvail) {
                        testTimes[0] = t;
                        expectedRoundTrip[0] = TRUE;
                        testLen = 1;
                    } else {
                        int32_t fromOffset = tzt.getFrom()->getRawOffset() + tzt.getFrom()->getDSTSavings();
                        int32_t toOffset = tzt.getTo()->getRawOffset() + tzt.getTo()->getDSTSavings();
                        int32_t delta = toOffset - fromOffset;
                        if (delta < 0) {
                            UBool isDstDecession = tzt.getFrom()->getDSTSavings() > 0 && tzt.getTo()->getDSTSavings() == 0;
                            testTimes[0] = t + delta - 1;
                            expectedRoundTrip[0] = TRUE;
                            testTimes[1] = t + delta;
                            expectedRoundTrip[1] = isDstDecession ?
                                    !AMBIGUOUS_DST_DECESSION[patidx] : !AMBIGUOUS_NEGATIVE_SHIFT[patidx];
                            testTimes[2] = t - 1;
                            expectedRoundTrip[2] = isDstDecession ?
                                    !AMBIGUOUS_DST_DECESSION[patidx] : !AMBIGUOUS_NEGATIVE_SHIFT[patidx];
                            testTimes[3] = t;
                            expectedRoundTrip[3] = TRUE;
                            testLen = 4;
                        } else {
                            testTimes[0] = t - 1;
                            expectedRoundTrip[0] = TRUE;
                            testTimes[1] = t;
                            expectedRoundTrip[1] = TRUE;
                            testLen = 2;
                        }
                    }
                    for (int32_t testidx = 0; testidx < testLen; testidx++) {
                        if (quick) {
                            // reduce regular test time
                            if (!expectedRoundTrip[testidx]) {
                                continue;
                            }
                        }
                        testCounts++;

                        UnicodeString text;
                        FieldPosition fpos(0);
                        sdf->format(testTimes[testidx], text, fpos);

                        UDate parsedDate = sdf->parse(text, status);
                        if (U_FAILURE(status)) {
                            errln((UnicodeString)"Failed to parse " + text);
                            status = U_ZERO_ERROR;
                            continue;
                        }
                        if (parsedDate != testTimes[testidx]) {
                            UnicodeString msg = (UnicodeString)"Time round trip failed for "
                                + "tzid=" + *tzid
                                + ", locale=" + LOCALES[locidx].getName()
                                + ", pattern=" + PATTERNS[patidx]
                                + ", text=" + text
                                + ", time=" + testTimes[testidx]
                                + ", restime=" + parsedDate
                                + ", diff=" + (parsedDate - testTimes[testidx]);
                            if (expectedRoundTrip[testidx]) {
                                errln((UnicodeString)"FAIL: " + msg);
                            } else if (REALLY_VERBOSE) {
                                logln(msg);
                            }
                        }
                    }
                    tztAvail = tz->getNextTransition(t, FALSE, tzt);
                    if (!tztAvail) {
                        break;
                    }
                    if (middle) {
                        // Test the date in the middle of two transitions.
                        t += (int64_t)((tzt.getTime() - t)/2);
                        middle = FALSE;
                        tztAvail = FALSE;
                    } else {
                        t = tzt.getTime();
                    }
                }
                delete tz;
            }
            times[patidx] += (Calendar::getNow() - timer);
            delete sdf;
        }
    }
    UDate total = 0;
    logln("### Elapsed time by patterns ###");
    for (int32_t i = 0; i < NUM_PATTERNS; i++) {
        logln(UnicodeString("") + times[i] + "ms (" + PATTERNS[i] + ")");
        total += times[i];
    }
    logln((UnicodeString)"Total: " + total + "ms");
    logln((UnicodeString)"Iteration: " + testCounts);

    delete cal;
    delete tzids;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
