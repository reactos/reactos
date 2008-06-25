/*
**********************************************************************
* Copyright (c) 2003-2007, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: July 21 2003
* Since: ICU 2.8
**********************************************************************
*/

#include "olsontz.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/simpletz.h"
#include "unicode/gregocal.h"
#include "gregoimp.h"
#include "cmemory.h"
#include "uassert.h"
#include "uvector.h"
#include <float.h> // DBL_MAX

#ifdef U_DEBUG_TZ
# include <stdio.h>
# include "uresimp.h" // for debugging

static void debug_tz_loc(const char *f, int32_t l)
{
  fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_tz_msg(const char *pat, ...)
{
  va_list ap;
  va_start(ap, pat);
  vfprintf(stderr, pat, ap);
  fflush(stderr);
}
// must use double parens, i.e.:  U_DEBUG_TZ_MSG(("four is: %d",4));
#define U_DEBUG_TZ_MSG(x) {debug_tz_loc(__FILE__,__LINE__);debug_tz_msg x;}
#else
#define U_DEBUG_TZ_MSG(x)
#endif

U_NAMESPACE_BEGIN

#define SECONDS_PER_DAY (24*60*60)

static const int32_t ZEROS[] = {0,0};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(OlsonTimeZone)

/**
 * Default constructor.  Creates a time zone with an empty ID and
 * a fixed GMT offset of zero.
 */
/*OlsonTimeZone::OlsonTimeZone() : finalYear(INT32_MAX), finalMillis(DBL_MAX), finalZone(0), transitionRulesInitialized(FALSE) {
    clearTransitionRules();
    constructEmpty();
}*/

/**
 * Construct a GMT+0 zone with no transitions.  This is done when a
 * constructor fails so the resultant object is well-behaved.
 */
void OlsonTimeZone::constructEmpty() {
    transitionCount = 0;
    typeCount = 1;
    transitionTimes = typeOffsets = ZEROS;
    typeData = (const uint8_t*) ZEROS;
}

/**
 * Construct from a resource bundle
 * @param top the top-level zoneinfo resource bundle.  This is used
 * to lookup the rule that `res' may refer to, if there is one.
 * @param res the resource bundle of the zone to be constructed
 * @param ec input-output error code
 */
OlsonTimeZone::OlsonTimeZone(const UResourceBundle* top,
                             const UResourceBundle* res,
                             UErrorCode& ec) :
  finalYear(INT32_MAX), finalMillis(DBL_MAX), finalZone(0), transitionRulesInitialized(FALSE)
{
    clearTransitionRules();
    U_DEBUG_TZ_MSG(("OlsonTimeZone(%s)\n", ures_getKey((UResourceBundle*)res)));
    if ((top == NULL || res == NULL) && U_SUCCESS(ec)) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }
    if (U_SUCCESS(ec)) {
        // TODO -- clean up -- Doesn't work if res points to an alias
        //        // TODO remove nonconst casts below when ures_* API is fixed
        //        setID(ures_getKey((UResourceBundle*) res)); // cast away const

        // Size 1 is an alias TO another zone (int)
        // HOWEVER, the caller should dereference this and never pass it in to us
        // Size 3 is a purely historical zone (no final rules)
        // Size 4 is like size 3, but with an alias list at the end
        // Size 5 is a hybrid zone, with historical and final elements
        // Size 6 is like size 5, but with an alias list at the end
        int32_t size = ures_getSize(res);
        if (size < 3 || size > 6) {
            ec = U_INVALID_FORMAT_ERROR;
        }

        // Transitions list may be empty
        int32_t i;
        UResourceBundle* r = ures_getByIndex(res, 0, NULL, &ec);
        transitionTimes = ures_getIntVector(r, &i, &ec);
        if ((i<0 || i>0x7FFF) && U_SUCCESS(ec)) {
            ec = U_INVALID_FORMAT_ERROR;
        }
        transitionCount = (int16_t) i;
        
        // Type offsets list must be of even size, with size >= 2
        r = ures_getByIndex(res, 1, r, &ec);
        typeOffsets = ures_getIntVector(r, &i, &ec);
        if ((i<2 || i>0x7FFE || ((i&1)!=0)) && U_SUCCESS(ec)) {
            ec = U_INVALID_FORMAT_ERROR;
        }
        typeCount = (int16_t) i >> 1;

        // Type data must be of the same size as the transitions list        
        r = ures_getByIndex(res, 2, r, &ec);
        int32_t len;
        typeData = ures_getBinary(r, &len, &ec);
        ures_close(r);
        if (len != transitionCount && U_SUCCESS(ec)) {
            ec = U_INVALID_FORMAT_ERROR;
        }

#if defined (U_DEBUG_TZ)
        U_DEBUG_TZ_MSG(("OlsonTimeZone(%s) - size = %d, typecount %d transitioncount %d - err %s\n", ures_getKey((UResourceBundle*)res), size, typeCount,  transitionCount, u_errorName(ec)));
        if(U_SUCCESS(ec)) {
          int32_t jj;
          for(jj=0;jj<transitionCount;jj++) {
            int32_t year, month, dom, dow;
            double millis=0;
            double days = Math::floorDivide(((double)transitionTimes[jj])*1000.0, (double)U_MILLIS_PER_DAY, millis);
            
            Grego::dayToFields(days, year, month, dom, dow);
            U_DEBUG_TZ_MSG(("   Transition %d:  time %d (%04d.%02d.%02d+%.1fh), typedata%d\n", jj, transitionTimes[jj],
                            year, month+1, dom, (millis/kOneHour), typeData[jj]));
//            U_DEBUG_TZ_MSG(("     offset%d\n", typeOffsets[jj]));
            int16_t f = jj;
            f <<= 1;
            U_DEBUG_TZ_MSG(("     offsets[%d+%d]=(%d+%d)=(%d==%d)\n", (int)f,(int)f+1,(int)typeOffsets[f],(int)typeOffsets[f+1],(int)zoneOffset(jj),
                (int)typeOffsets[f]+(int)typeOffsets[f+1]));
          }
        }
#endif

        // Process final rule and data, if any
        if (size >= 5) {
            int32_t ruleidLen = 0;
            const UChar* idUStr = ures_getStringByIndex(res, 3, &ruleidLen, &ec);
            UnicodeString ruleid(TRUE, idUStr, ruleidLen);
            r = ures_getByIndex(res, 4, NULL, &ec);
            const int32_t* data = ures_getIntVector(r, &len, &ec);
#if defined U_DEBUG_TZ
            const char *rKey = ures_getKey(r);
            const char *zKey = ures_getKey((UResourceBundle*)res);
#endif
            ures_close(r);
            if (U_SUCCESS(ec)) {
                if (data != 0 && len == 2) {
                    int32_t rawOffset = data[0] * U_MILLIS_PER_SECOND;
                    // Subtract one from the actual final year; we
                    // actually store final year - 1, and compare
                    // using > rather than >=.  This allows us to use
                    // INT32_MAX as an exclusive upper limit for all
                    // years, including INT32_MAX.
                    U_ASSERT(data[1] > INT32_MIN);
                    finalYear = data[1] - 1;
                    // Also compute the millis for Jan 1, 0:00 GMT of the
                    // finalYear.  This reduces runtime computations.
                    finalMillis = Grego::fieldsToDay(data[1], 0, 1) * U_MILLIS_PER_DAY;
                    U_DEBUG_TZ_MSG(("zone%s|%s: {%d,%d}, finalYear%d, finalMillis%.1lf\n",
                                    zKey,rKey, data[0], data[1], finalYear, finalMillis));
                    r = TimeZone::loadRule(top, ruleid, NULL, ec);
                    if (U_SUCCESS(ec)) {
                        // 3, 1, -1, 7200, 0, 9, -31, -1, 7200, 0, 3600
                        data = ures_getIntVector(r, &len, &ec);
                        if (U_SUCCESS(ec) && len == 11) {
                            UnicodeString emptyStr;
                            U_DEBUG_TZ_MSG(("zone%s, rule%s: {%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}\n", zKey, ures_getKey(r), 
                                          data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]));
                            finalZone = new SimpleTimeZone(rawOffset, emptyStr,
                                (int8_t)data[0], (int8_t)data[1], (int8_t)data[2],
                                data[3] * U_MILLIS_PER_SECOND,
                                (SimpleTimeZone::TimeMode) data[4],
                                (int8_t)data[5], (int8_t)data[6], (int8_t)data[7],
                                data[8] * U_MILLIS_PER_SECOND,
                                (SimpleTimeZone::TimeMode) data[9],
                                data[10] * U_MILLIS_PER_SECOND, ec);
                        } else {
                            ec = U_INVALID_FORMAT_ERROR;
                        }
                    }
                    ures_close(r);
                } else {
                    ec = U_INVALID_FORMAT_ERROR;
                }
            }
        }
    }

    if (U_FAILURE(ec)) {
        constructEmpty();
    }
}

/**
 * Copy constructor
 */
OlsonTimeZone::OlsonTimeZone(const OlsonTimeZone& other) :
    BasicTimeZone(other), finalZone(0) {
    *this = other;
}

/**
 * Assignment operator
 */
OlsonTimeZone& OlsonTimeZone::operator=(const OlsonTimeZone& other) {
    transitionCount = other.transitionCount;
    typeCount = other.typeCount;
    transitionTimes = other.transitionTimes;
    typeOffsets = other.typeOffsets;
    typeData = other.typeData;
    finalYear = other.finalYear;
    finalMillis = other.finalMillis;
    delete finalZone;
    finalZone = (other.finalZone != 0) ?
        (SimpleTimeZone*) other.finalZone->clone() : 0;
    clearTransitionRules();
    return *this;
}

/**
 * Destructor
 */
OlsonTimeZone::~OlsonTimeZone() {
    deleteTransitionRules();
    delete finalZone;
}

/**
 * Returns true if the two TimeZone objects are equal.
 */
UBool OlsonTimeZone::operator==(const TimeZone& other) const {
    return ((this == &other) ||
            (getDynamicClassID() == other.getDynamicClassID() &&
            TimeZone::operator==(other) &&
            hasSameRules(other)));
}

/**
 * TimeZone API.
 */
TimeZone* OlsonTimeZone::clone() const {
    return new OlsonTimeZone(*this);
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getOffset(uint8_t era, int32_t year, int32_t month,
                                 int32_t dom, uint8_t dow,
                                 int32_t millis, UErrorCode& ec) const {
    if (month < UCAL_JANUARY || month > UCAL_DECEMBER) {
        if (U_SUCCESS(ec)) {
            ec = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return 0;
    } else {
        return getOffset(era, year, month, dom, dow, millis,
                         Grego::monthLength(year, month),
                         ec);
    }
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getOffset(uint8_t era, int32_t year, int32_t month,
                                 int32_t dom, uint8_t dow,
                                 int32_t millis, int32_t monthLength,
                                 UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return 0;
    }

    if ((era != GregorianCalendar::AD && era != GregorianCalendar::BC)
        || month < UCAL_JANUARY
        || month > UCAL_DECEMBER
        || dom < 1
        || dom > monthLength
        || dow < UCAL_SUNDAY
        || dow > UCAL_SATURDAY
        || millis < 0
        || millis >= U_MILLIS_PER_DAY
        || monthLength < 28
        || monthLength > 31) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if (era == GregorianCalendar::BC) {
        year = -year;
    }

    if (year > finalYear) { // [sic] >, not >=; see above
        U_ASSERT(finalZone != 0);
        return finalZone->getOffset(era, year, month, dom, dow,
                                    millis, monthLength, ec);
    }

    // Compute local epoch millis from input fields
    UDate date = (UDate)(Grego::fieldsToDay(year, month, dom) * U_MILLIS_PER_DAY + millis);
    int32_t rawoff, dstoff;
    getHistoricalOffset(date, TRUE, kDaylight, kStandard, rawoff, dstoff);
    return rawoff + dstoff;
}

/**
 * TimeZone API.
 */
void OlsonTimeZone::getOffset(UDate date, UBool local, int32_t& rawoff,
                              int32_t& dstoff, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }
    // The check against finalMillis will suffice most of the time, except
    // for the case in which finalMillis == DBL_MAX, date == DBL_MAX,
    // and finalZone == 0.  For this case we add "&& finalZone != 0".
    if (date >= finalMillis && finalZone != 0) {
        finalZone->getOffset(date, local, rawoff, dstoff, ec);
    } else {
        getHistoricalOffset(date, local, kFormer, kLatter, rawoff, dstoff);
    }
}

void
OlsonTimeZone::getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
                                  int32_t& rawoff, int32_t& dstoff, UErrorCode& ec) /*const*/ {
    if (U_FAILURE(ec)) {
        return;
    }
    if (date >= finalMillis && finalZone != 0) {
        finalZone->getOffsetFromLocal(date, nonExistingTimeOpt, duplicatedTimeOpt, rawoff, dstoff, ec);
    } else {
        getHistoricalOffset(date, TRUE, nonExistingTimeOpt, duplicatedTimeOpt, rawoff, dstoff);
    }
}


/**
 * TimeZone API.
 */
void OlsonTimeZone::setRawOffset(int32_t /*offsetMillis*/) {
    // We don't support this operation, since OlsonTimeZones are
    // immutable (except for the ID, which is in the base class).

    // Nothing to do!
}

/**
 * TimeZone API.
 */
int32_t OlsonTimeZone::getRawOffset() const {
    UErrorCode ec = U_ZERO_ERROR;
    int32_t raw, dst;
    getOffset((double) uprv_getUTCtime() * U_MILLIS_PER_SECOND,
              FALSE, raw, dst, ec);
    return raw;
}

#if defined U_DEBUG_TZ
void printTime(double ms) {
            int32_t year, month, dom, dow;
            double millis=0;
            double days = Math::floorDivide(((double)ms), (double)U_MILLIS_PER_DAY, millis);
            
            Grego::dayToFields(days, year, month, dom, dow);
            U_DEBUG_TZ_MSG(("   getHistoricalOffset:  time %.1f (%04d.%02d.%02d+%.1fh)\n", ms,
                            year, month+1, dom, (millis/kOneHour)));
    }
#endif

void
OlsonTimeZone::getHistoricalOffset(UDate date, UBool local,
                                   int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
                                   int32_t& rawoff, int32_t& dstoff) const {
    U_DEBUG_TZ_MSG(("getHistoricalOffset(%.1f, %s, %d, %d, raw, dst)\n",
        date, local?"T":"F", NonExistingTimeOpt, DuplicatedTimeOpt));
#if defined U_DEBUG_TZ
        printTime(date*1000.0);
#endif
    if (transitionCount != 0) {
        double sec = uprv_floor(date / U_MILLIS_PER_SECOND);
        // Linear search from the end is the fastest approach, since
        // most lookups will happen at/near the end.
        int16_t i;
        for (i = transitionCount - 1; i > 0; --i) {
            int32_t transition = transitionTimes[i];

            if (local) {
                int32_t offsetBefore = zoneOffset(typeData[i-1]);
                UBool dstBefore = dstOffset(typeData[i-1]) != 0;

                int32_t offsetAfter = zoneOffset(typeData[i]);
                UBool dstAfter = dstOffset(typeData[i]) != 0;

                UBool dstToStd = dstBefore && !dstAfter;
                UBool stdToDst = !dstBefore && dstAfter;
                
                if (offsetAfter - offsetBefore >= 0) {
                    // Positive transition, which makes a non-existing local time range
                    if (((NonExistingTimeOpt & kStdDstMask) == kStandard && dstToStd)
                            || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
                        transition += offsetBefore;
                    } else if (((NonExistingTimeOpt & kStdDstMask) == kStandard && stdToDst)
                            || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
                        transition += offsetAfter;
                    } else if ((NonExistingTimeOpt & kFormerLatterMask) == kLatter) {
                        transition += offsetBefore;
                    } else {
                        // Interprets the time with rule before the transition,
                        // default for non-existing time range
                        transition += offsetAfter;
                    }
                } else {
                    // Negative transition, which makes a duplicated local time range
                    if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && dstToStd)
                            || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
                        transition += offsetAfter;
                    } else if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && stdToDst)
                            || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
                        transition += offsetBefore;
                    } else if ((DuplicatedTimeOpt & kFormerLatterMask) == kFormer) {
                        transition += offsetBefore;
                    } else {
                        // Interprets the time with rule after the transition,
                        // default for duplicated local time range
                        transition += offsetAfter;
                    }
                }
            }
            if (sec >= transition) {
                U_DEBUG_TZ_MSG(("Found@%d: time=%.1f, localtransition=%d (orig %d) dz %d\n", i, sec, transition, transitionTimes[i],
                    zoneOffset(typeData[i-1])));
#if defined U_DEBUG_TZ
                printTime(transition*1000.0);
                printTime(transitionTimes[i]*1000.0);
#endif
                break;
            } else {
                U_DEBUG_TZ_MSG(("miss@%d: time=%.1f, localtransition=%d (orig %d) dz %d\n", i, sec, transition, transitionTimes[i],
                    zoneOffset(typeData[i-1])));
#if defined U_DEBUG_TZ
                printTime(transition*1000.0);
                printTime(transitionTimes[i]*1000.0);
#endif
            }
        }

        U_ASSERT(i>=0 && i<transitionCount);

        // Check invariants for GMT times; if these pass for GMT times
        // the local logic should be working too.
        U_ASSERT(local || sec < transitionTimes[0] || sec >= transitionTimes[i]);
        U_ASSERT(local || i == transitionCount-1 || sec < transitionTimes[i+1]);

        U_DEBUG_TZ_MSG(("getHistoricalOffset(%.1f, %s, %d, %d, raw, dst) - trans %d\n",
            date, local?"T":"F", NonExistingTimeOpt, DuplicatedTimeOpt, i));

        // Since ICU tzdata 2007c, the first transition data is actually not a
        // transition, but used for representing the initial offset.  So the code
        // below works even if i == 0.
        int16_t index = typeData[i];
        rawoff = rawOffset(index) * U_MILLIS_PER_SECOND;
        dstoff = dstOffset(index) * U_MILLIS_PER_SECOND;
    } else {
        // No transitions, single pair of offsets only
        rawoff = rawOffset(0) * U_MILLIS_PER_SECOND;
        dstoff = dstOffset(0) * U_MILLIS_PER_SECOND;
    }
    U_DEBUG_TZ_MSG(("getHistoricalOffset(%.1f, %s, %d, %d, raw, dst) - raw=%d, dst=%d\n",
        date, local?"T":"F", NonExistingTimeOpt, DuplicatedTimeOpt, rawoff, dstoff));
}

/**
 * TimeZone API.
 */
UBool OlsonTimeZone::useDaylightTime() const {
    // If DST was observed in 1942 (for example) but has never been
    // observed from 1943 to the present, most clients will expect
    // this method to return FALSE.  This method determines whether
    // DST is in use in the current year (at any point in the year)
    // and returns TRUE if so.

    int32_t days = (int32_t)Math::floorDivide(uprv_getUTCtime(), (double)U_MILLIS_PER_DAY); // epoch days

    int32_t year, month, dom, dow;
    
    Grego::dayToFields(days, year, month, dom, dow);

    if (year > finalYear) { // [sic] >, not >=; see above
        U_ASSERT(finalZone != 0 && finalZone->useDaylightTime());
        return TRUE;
    }

    // Find start of this year, and start of next year
    int32_t start = (int32_t) Grego::fieldsToDay(year, 0, 1) * SECONDS_PER_DAY;    
    int32_t limit = (int32_t) Grego::fieldsToDay(year+1, 0, 1) * SECONDS_PER_DAY;    

    // Return TRUE if DST is observed at any time during the current
    // year.
    for (int16_t i=0; i<transitionCount; ++i) {
        if (transitionTimes[i] >= limit) {
            break;
        }
        if (transitionTimes[i] >= start &&
            dstOffset(typeData[i]) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}
int32_t 
OlsonTimeZone::getDSTSavings() const{
    if(finalZone!=NULL){
        return finalZone->getDSTSavings();
    }
    return TimeZone::getDSTSavings();
}
/**
 * TimeZone API.
 */
UBool OlsonTimeZone::inDaylightTime(UDate date, UErrorCode& ec) const {
    int32_t raw, dst;
    getOffset(date, FALSE, raw, dst, ec);
    return dst != 0;
}

UBool
OlsonTimeZone::hasSameRules(const TimeZone &other) const {
    if (this == &other) {
        return TRUE;
    }
    if (other.getDynamicClassID() != OlsonTimeZone::getStaticClassID()) {
        return FALSE;
    }
    const OlsonTimeZone* z = (const OlsonTimeZone*) &other;

    // [sic] pointer comparison: typeData points into
    // memory-mapped or DLL space, so if two zones have the same
    // pointer, they are equal.
    if (typeData == z->typeData) {
        return TRUE;
    }
    
     // If the pointers are not equal, the zones may still
     // be equal if their rules and transitions are equal
    return
        (finalYear == z->finalYear &&
          // Don't compare finalMillis; if finalYear is ==, so is finalMillis
          ((finalZone == 0 && z->finalZone == 0) ||
            (finalZone != 0 && z->finalZone != 0 && *finalZone == *z->finalZone)) &&
          
          transitionCount == z->transitionCount &&
          typeCount == z->typeCount &&
          uprv_memcmp(transitionTimes, z->transitionTimes,
                      sizeof(transitionTimes[0]) * transitionCount) == 0 &&
          uprv_memcmp(typeOffsets, z->typeOffsets,
                      (sizeof(typeOffsets[0]) * typeCount) << 1) == 0 &&
          uprv_memcmp(typeData, z->typeData,
                      (sizeof(typeData[0]) * typeCount)) == 0);
}

void
OlsonTimeZone::clearTransitionRules(void) {
    initialRule = NULL;
    firstTZTransition = NULL;
    firstFinalTZTransition = NULL;
    historicRules = NULL;
    historicRuleCount = 0;
    finalZoneWithStartYear = NULL;
    firstTZTransitionIdx = 0;
    transitionRulesInitialized = FALSE;
}

void
OlsonTimeZone::deleteTransitionRules(void) {
    if (initialRule != NULL) {
        delete initialRule;
    }
    if (firstTZTransition != NULL) {
        delete firstTZTransition;
    }
    if (firstFinalTZTransition != NULL) {
        delete firstFinalTZTransition;
    }
    if (finalZoneWithStartYear != NULL) {
        delete finalZoneWithStartYear;
    }
    if (historicRules != NULL) {
        for (int i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                delete historicRules[i];
            }
        }
        uprv_free(historicRules);
    }
    clearTransitionRules();
}

void
OlsonTimeZone::initTransitionRules(UErrorCode& status) {
    if(U_FAILURE(status)) {
        return;
    }
    if (transitionRulesInitialized) {
        return;
    }
    deleteTransitionRules();
    UnicodeString tzid;
    getID(tzid);

    UnicodeString stdName = tzid + UNICODE_STRING_SIMPLE("(STD)");
    UnicodeString dstName = tzid + UNICODE_STRING_SIMPLE("(DST)");

    int32_t raw, dst;
    if (transitionCount > 0) {
        int16_t transitionIdx, typeIdx;

        // Note: Since 2007c, the very first transition data is a dummy entry
        //       added for resolving a offset calculation problem.

        // Create initial rule
        typeIdx = (int16_t)typeData[0]; // initial type
        raw = rawOffset(typeIdx) * U_MILLIS_PER_SECOND;
        dst = dstOffset(typeIdx) * U_MILLIS_PER_SECOND;
        initialRule = new InitialTimeZoneRule((dst == 0 ? stdName : dstName), raw, dst);

        firstTZTransitionIdx = 0;
        for (transitionIdx = 1; transitionIdx < transitionCount; transitionIdx++) {
            firstTZTransitionIdx++;
            if (typeIdx != (int16_t)typeData[transitionIdx]) {
                break;
            }
        }
        if (transitionIdx == transitionCount) {
            // Actually no transitions...
        } else {
            // Build historic rule array
            UDate* times = (UDate*)uprv_malloc(sizeof(UDate)*transitionCount); /* large enough to store all transition times */
            if (times == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                deleteTransitionRules();
                return;
            }
            for (typeIdx = 0; typeIdx < typeCount; typeIdx++) {
                // Gather all start times for each pair of offsets
                int32_t nTimes = 0;
                for (transitionIdx = firstTZTransitionIdx; transitionIdx < transitionCount; transitionIdx++) {
                    if (typeIdx == (int16_t)typeData[transitionIdx]) {
                        UDate tt = ((UDate)transitionTimes[transitionIdx]) * U_MILLIS_PER_SECOND;
                        if (tt < finalMillis) {
                            // Exclude transitions after finalMillis
                            times[nTimes++] = tt;
                        }
                    }
                }
                if (nTimes > 0) {
                    // Create a TimeArrayTimeZoneRule
                    raw = rawOffset(typeIdx) * U_MILLIS_PER_SECOND;
                    dst = dstOffset(typeIdx) * U_MILLIS_PER_SECOND;
                    if (historicRules == NULL) {
                        historicRuleCount = typeCount;
                        historicRules = (TimeArrayTimeZoneRule**)uprv_malloc(sizeof(TimeArrayTimeZoneRule*)*historicRuleCount);
                        if (historicRules == NULL) {
                            status = U_MEMORY_ALLOCATION_ERROR;
                            deleteTransitionRules();
                            uprv_free(times);
                            return;
                        }
                        for (int i = 0; i < historicRuleCount; i++) {
                            // Initialize TimeArrayTimeZoneRule pointers as NULL
                            historicRules[i] = NULL;
                        }
                    }
                    historicRules[typeIdx] = new TimeArrayTimeZoneRule((dst == 0 ? stdName : dstName),
                        raw, dst, times, nTimes, DateTimeRule::UTC_TIME);
                }
            }
            uprv_free(times);

            // Create initial transition
            typeIdx = (int16_t)typeData[firstTZTransitionIdx];
            firstTZTransition = new TimeZoneTransition(((UDate)transitionTimes[firstTZTransitionIdx]) * U_MILLIS_PER_SECOND,
                    *initialRule, *historicRules[typeIdx]);
        }
    }
    if (initialRule == NULL) {
        // No historic transitions
        raw = rawOffset(0) * U_MILLIS_PER_SECOND;
        dst = dstOffset(0) * U_MILLIS_PER_SECOND;
        initialRule = new InitialTimeZoneRule((dst == 0 ? stdName : dstName), raw, dst);
    }
    if (finalZone != NULL) {
        // Get the first occurence of final rule starts
        UDate startTime = (UDate)finalMillis;
        TimeZoneRule *firstFinalRule = NULL;
        if (finalZone->useDaylightTime()) {
            /*
             * Note: When an OlsonTimeZone is constructed, we should set the final year
             * as the start year of finalZone.  However, the bounday condition used for
             * getting offset from finalZone has some problems.  So setting the start year
             * in the finalZone will cause a problem.  For now, we do not set the valid
             * start year when the construction time and create a clone and set the
             * start year when extracting rules.
             */
            finalZoneWithStartYear = (SimpleTimeZone*)finalZone->clone();
            // finalYear is 1 year before the actual final year.
            // See the comment in the construction method.
            finalZoneWithStartYear->setStartYear(finalYear + 1);

            TimeZoneTransition tzt;
            finalZoneWithStartYear->getNextTransition(startTime, false, tzt);
            firstFinalRule  = tzt.getTo()->clone();
            startTime = tzt.getTime();
        } else {
            finalZoneWithStartYear = (SimpleTimeZone*)finalZone->clone();
            finalZone->getID(tzid);
            firstFinalRule = new TimeArrayTimeZoneRule(tzid,
                finalZone->getRawOffset(), 0, &startTime, 1, DateTimeRule::UTC_TIME);
        }
        TimeZoneRule *prevRule = NULL;
        if (transitionCount > 0) {
            prevRule = historicRules[typeData[transitionCount - 1]];
        }
        if (prevRule == NULL) {
            // No historic transitions, but only finalZone available
            prevRule = initialRule;
        }
        firstFinalTZTransition = new TimeZoneTransition();
        firstFinalTZTransition->setTime(startTime);
        firstFinalTZTransition->adoptFrom(prevRule->clone());
        firstFinalTZTransition->adoptTo(firstFinalRule);
    }
    transitionRulesInitialized = TRUE;
}

UBool
OlsonTimeZone::getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/ {
    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    if (finalZone != NULL) {
        if (inclusive && base == firstFinalTZTransition->getTime()) {
            result = *firstFinalTZTransition;
            return TRUE;
        } else if (base >= firstFinalTZTransition->getTime()) {
            if (finalZone->useDaylightTime()) {
                //return finalZone->getNextTransition(base, inclusive, result);
                return finalZoneWithStartYear->getNextTransition(base, inclusive, result);
            } else {
                // No more transitions
                return FALSE;
            }
        }
    }
    if (historicRules != NULL) {
        // Find a historical transition
        int16_t ttidx = transitionCount - 1;
        for (; ttidx >= firstTZTransitionIdx; ttidx--) {
            UDate t = ((UDate)transitionTimes[ttidx]) * U_MILLIS_PER_SECOND;
            if (base > t || (!inclusive && base == t)) {
                break;
            }
        }
        if (ttidx == transitionCount - 1)  {
            if (firstFinalTZTransition != NULL) {
                result = *firstFinalTZTransition;
                return TRUE;
            } else {
                return FALSE;
            }
        } else if (ttidx < firstTZTransitionIdx) {
            result = *firstTZTransition;
            return TRUE;
        } else {
            // Create a TimeZoneTransition
            TimeZoneRule *to = historicRules[typeData[ttidx + 1]];
            TimeZoneRule *from = historicRules[typeData[ttidx]];
            UDate startTime = ((UDate)transitionTimes[ttidx+1]) * U_MILLIS_PER_SECOND;

            // The transitions loaded from zoneinfo.res may contain non-transition data
            UnicodeString fromName, toName;
            from->getName(fromName);
            to->getName(toName);
            if (fromName == toName && from->getRawOffset() == to->getRawOffset()
                    && from->getDSTSavings() == to->getDSTSavings()) {
                return getNextTransition(startTime, false, result);
            }
            result.setTime(startTime);
            result.adoptFrom(from->clone());
            result.adoptTo(to->clone());
            return TRUE;
        }
    }
    return FALSE;
}

UBool
OlsonTimeZone::getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/ {
    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    if (finalZone != NULL) {
        if (inclusive && base == firstFinalTZTransition->getTime()) {
            result = *firstFinalTZTransition;
            return TRUE;
        } else if (base > firstFinalTZTransition->getTime()) {
            if (finalZone->useDaylightTime()) {
                //return finalZone->getPreviousTransition(base, inclusive, result);
                return finalZoneWithStartYear->getPreviousTransition(base, inclusive, result);
            } else {
                result = *firstFinalTZTransition;
                return TRUE;
            }                
        }
    }

    if (historicRules != NULL) {
        // Find a historical transition
        int16_t ttidx = transitionCount - 1;
        for (; ttidx >= firstTZTransitionIdx; ttidx--) {
            UDate t = ((UDate)transitionTimes[ttidx]) * U_MILLIS_PER_SECOND;
            if (base > t || (inclusive && base == t)) {
                break;
            }
        }
        if (ttidx < firstTZTransitionIdx) {
            // No more transitions
            return FALSE;
        } else if (ttidx == firstTZTransitionIdx) {
            result = *firstTZTransition;
            return TRUE;
        } else {
            // Create a TimeZoneTransition
            TimeZoneRule *to = historicRules[typeData[ttidx]];
            TimeZoneRule *from = historicRules[typeData[ttidx-1]];
            UDate startTime = ((UDate)transitionTimes[ttidx]) * U_MILLIS_PER_SECOND;

            // The transitions loaded from zoneinfo.res may contain non-transition data
            UnicodeString fromName, toName;
            from->getName(fromName);
            to->getName(toName);
            if (fromName == toName && from->getRawOffset() == to->getRawOffset()
                    && from->getDSTSavings() == to->getDSTSavings()) {
                return getPreviousTransition(startTime, false, result);
            }
            result.setTime(startTime);
            result.adoptFrom(from->clone());
            result.adoptTo(to->clone());
            return TRUE;
        }
    }
    return FALSE;
}

int32_t
OlsonTimeZone::countTransitionRules(UErrorCode& status) /*const*/ {
    if (U_FAILURE(status)) {
        return 0;
    }
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return 0;
    }

    int32_t count = 0;
    if (historicRules != NULL) {
        // historicRules may contain null entries when original zoneinfo data
        // includes non transition data.
        for (int32_t i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                count++;
            }
        }
    }
    if (finalZone != NULL) {
        if (finalZone->useDaylightTime()) {
            count += 2;
        } else {
            count++;
        }
    }
    return count;
}

void
OlsonTimeZone::getTimeZoneRules(const InitialTimeZoneRule*& initial,
                                const TimeZoneRule* trsrules[],
                                int32_t& trscount,
                                UErrorCode& status) /*const*/ {
    if (U_FAILURE(status)) {
        return;
    }
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return;
    }

    // Initial rule
    initial = initialRule;

    // Transition rules
    int32_t cnt = 0;
    if (historicRules != NULL && trscount > cnt) {
        // historicRules may contain null entries when original zoneinfo data
        // includes non transition data.
        for (int32_t i = 0; i < historicRuleCount; i++) {
            if (historicRules[i] != NULL) {
                trsrules[cnt++] = historicRules[i];
                if (cnt >= trscount) {
                    break;
                }
            }
        }
    }
    if (finalZoneWithStartYear != NULL && trscount > cnt) {
        const InitialTimeZoneRule *tmpini;
        int32_t tmpcnt = trscount - cnt;
        finalZoneWithStartYear->getTimeZoneRules(tmpini, &trsrules[cnt], tmpcnt, status);
        if (U_FAILURE(status)) {
            return;
        }
        cnt += tmpcnt;
    }
    // Set the result length
    trscount = cnt;
}

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING

//eof
