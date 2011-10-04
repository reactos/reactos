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
#ifndef OLSONTZ_H
#define OLSONTZ_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"

struct UResourceBundle;

U_NAMESPACE_BEGIN

class SimpleTimeZone;

/**
 * A time zone based on the Olson database.  Olson time zones change
 * behavior over time.  The raw offset, rules, presence or absence of
 * daylight savings time, and even the daylight savings amount can all
 * vary.
 *
 * This class uses a resource bundle named "zoneinfo".  Zoneinfo is a
 * table containing different kinds of resources.  In several places,
 * zones are referred to using integers.  A zone's integer is a number
 * from 0..n-1, where n is the number of zones, with the zones sorted
 * in lexicographic order.
 *
 * 1. Zones.  These have keys corresponding to the Olson IDs, e.g.,
 * "Asia/Shanghai".  Each resource describes the behavior of the given
 * zone.  Zones come in several formats, which are differentiated
 * based on length.
 *
 *  a. Alias (int, length 1).  An alias zone is an int resource.  The
 *  integer is the zone number of the target zone.  The key of this
 *  resource is an alternate name for the target zone.  Aliases
 *  represent Olson links and ICU compatibility IDs.
 *
 *  b. Simple zone (array, length 3).  The three subelements are:
 *
 *   i. An intvector of transitions.  These are given in epoch
 *   seconds.  This may be an empty invector (length 0).  If the
 *   transtions list is empty, then the zone's behavior is fixed and
 *   given by the offset list, which will contain exactly one pair.
 *   Otherwise each transtion indicates a time after which (inclusive)
 *   the associated offset pair is in effect.
 *
 *   ii. An intvector of offsets.  These are in pairs of raw offset /
 *   DST offset, in units of seconds.  There will be at least one pair
 *   (length >= 2 && length % 2 == 0).
 *
 *   iii. A binary resource.  This is of the same length as the
 *   transitions vector, so length may be zero.  Each unsigned byte
 *   corresponds to one transition, and has a value of 0..n-1, where n
 *   is the number of pairs in the offset vector.  This forms a map
 *   between transitions and offset pairs.
 *
 *  c. Simple zone with aliases (array, length 4).  This is like a
 *  simple zone, but also contains a fourth element:
 *
 *   iv. An intvector of aliases.  This list includes this zone
 *   itself, and lists all aliases of this zone.
 *
 *  d. Complex zone (array, length 5).  This is like a simple zone,
 *  but contains two more elements:
 *
 *   iv. A string, giving the name of a rule.  This is the "final
 *   rule", which governs the zone's behavior beginning in the "final
 *   year".  The rule ID is given without leading underscore, e.g.,
 *   "EU".
 *
 *   v. An intvector of length 2, containing the raw offset for the
 *   final rule (in seconds), and the final year.  The final rule
 *   takes effect for years >= the final year.
 *
 *  e. Complex zone with aliases (array, length 6).  This is like a
 *  complex zone, but also contains a sixth element:
 * 
 *   vi. An intvector of aliases.  This list includes this zone
 *   itself, and lists all aliases of this zone.
 *
 * 2. Rules.  These have keys corresponding to the Olson rule IDs,
 * with an underscore prepended, e.g., "_EU".  Each resource describes
 * the behavior of the given rule using an intvector, containing the
 * onset list, the cessation list, and the DST savings.  The onset and
 * cessation lists consist of the month, dowim, dow, time, and time
 * mode.  The end result is that the 11 integers describing the rule
 * can be passed directly into the SimpleTimeZone 13-argument
 * constructor (the other two arguments will be the raw offset, taken
 * from the complex zone element 5, and the ID string, which is not
 * used), with the times and the DST savings multiplied by 1000 to
 * scale from seconds to milliseconds.
 *
 * 3. Countries.  These have keys corresponding to the 2-letter ISO
 * country codes, with a percent sign prepended, e.g., "%US".  Each
 * resource is an intvector listing the zones associated with the
 * given country.  The special entry "%" corresponds to "no country",
 * that is, the category of zones assigned to no country in the Olson
 * DB.
 *
 * 4. Metadata.  Metadata is stored under the key "_".  It is an
 * intvector of length three containing the number of zones resources,
 * rule resources, and country resources.  For the purposes of this
 * count, the metadata entry itself is considered a rule resource,
 * since its key begins with an underscore.
 */
class OlsonTimeZone: public BasicTimeZone {
 public:
    /**
     * Construct from a resource bundle.
     * @param top the top-level zoneinfo resource bundle.  This is used
     * to lookup the rule that `res' may refer to, if there is one.
     * @param res the resource bundle of the zone to be constructed
     * @param ec input-output error code
     */
    OlsonTimeZone(const UResourceBundle* top,
                  const UResourceBundle* res, UErrorCode& ec);

    /**
     * Copy constructor
     */
    OlsonTimeZone(const OlsonTimeZone& other);

    /**
     * Destructor
     */
    virtual ~OlsonTimeZone();

    /**
     * Assignment operator
     */
    OlsonTimeZone& operator=(const OlsonTimeZone& other);

    /**
     * Returns true if the two TimeZone objects are equal.
     */
    virtual UBool operator==(const TimeZone& other) const;

    /**
     * TimeZone API.
     */
    virtual TimeZone* clone() const;

    /**
     * TimeZone API.
     */
    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

    /**
     * TimeZone API.
     */
    virtual UClassID getDynamicClassID() const;
    
    /**
     * TimeZone API.  Do not call this; prefer getOffset(UDate,...).
     */
    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month,
                              int32_t day, uint8_t dayOfWeek,
                              int32_t millis, UErrorCode& ec) const;

    /**
     * TimeZone API.  Do not call this; prefer getOffset(UDate,...).
     */
    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month,
                              int32_t day, uint8_t dayOfWeek,
                              int32_t millis, int32_t monthLength,
                              UErrorCode& ec) const;

    /**
     * TimeZone API.
     */
    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                   int32_t& dstOffset, UErrorCode& ec) const;

    /**
     * BasicTimeZone API.
     */
    virtual void getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
        int32_t& rawoff, int32_t& dstoff, UErrorCode& ec) /*const*/;

    /**
     * TimeZone API.  This method has no effect since objects of this
     * class are quasi-immutable (the base class allows the ID to be
     * changed).
     */
    virtual void setRawOffset(int32_t offsetMillis);

    /**
     * TimeZone API.  For a historical zone, the raw offset can change
     * over time, so this API is not useful.  In order to approximate
     * expected behavior, this method returns the raw offset for the
     * current moment in time.
     */
    virtual int32_t getRawOffset() const;

    /**
     * TimeZone API.  For a historical zone, whether DST is used or
     * not varies over time.  In order to approximate expected
     * behavior, this method returns TRUE if DST is observed at any
     * point in the current year.
     */
    virtual UBool useDaylightTime() const;

    /**
     * TimeZone API.
     */
    virtual UBool inDaylightTime(UDate date, UErrorCode& ec) const;

    /**
     * TimeZone API.
     */
    virtual int32_t getDSTSavings() const;

    /**
     * TimeZone API.  Also comare historic transitions.
     */
    virtual UBool hasSameRules(const TimeZone& other) const;

    /**
     * BasicTimeZone API.
     * Gets the first time zone transition after the base time.
     * @param base      The base time.
     * @param inclusive Whether the base time is inclusive or not.
     * @param result    Receives the first transition after the base time.
     * @return  TRUE if the transition is found.
     */
    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/;

    /**
     * BasicTimeZone API.
     * Gets the most recent time zone transition before the base time.
     * @param base      The base time.
     * @param inclusive Whether the base time is inclusive or not.
     * @param result    Receives the most recent transition before the base time.
     * @return  TRUE if the transition is found.
     */
    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) /*const*/;

    /**
     * BasicTimeZone API.
     * Returns the number of <code>TimeZoneRule</code>s which represents time transitions,
     * for this time zone, that is, all <code>TimeZoneRule</code>s for this time zone except
     * <code>InitialTimeZoneRule</code>.  The return value range is 0 or any positive value.
     * @param status    Receives error status code.
     * @return The number of <code>TimeZoneRule</code>s representing time transitions.
     */
    virtual int32_t countTransitionRules(UErrorCode& status) /*const*/;

    /**
     * Gets the <code>InitialTimeZoneRule</code> and the set of <code>TimeZoneRule</code>
     * which represent time transitions for this time zone.  On successful return,
     * the argument initial points to non-NULL <code>InitialTimeZoneRule</code> and
     * the array trsrules is filled with 0 or multiple <code>TimeZoneRule</code>
     * instances up to the size specified by trscount.  The results are referencing the
     * rule instance held by this time zone instance.  Therefore, after this time zone
     * is destructed, they are no longer available.
     * @param initial       Receives the initial timezone rule
     * @param trsrules      Receives the timezone transition rules
     * @param trscount      On input, specify the size of the array 'transitions' receiving
     *                      the timezone transition rules.  On output, actual number of
     *                      rules filled in the array will be set.
     * @param status        Receives error status code.
     * @draft ICU 3.8
     */
    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status) /*const*/;

private:
    /**
     * Default constructor.  Creates a time zone with an empty ID and
     * a fixed GMT offset of zero.
     */
    OlsonTimeZone();

private:

    void constructEmpty();

    void getHistoricalOffset(UDate date, UBool local,
        int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
        int32_t& rawoff, int32_t& dstoff) const;

    int32_t zoneOffset(int16_t index) const;
    int32_t rawOffset(int16_t index) const;
    int32_t dstOffset(int16_t index) const;

    /**
     * Number of transitions, 0..~370
     */
    int16_t transitionCount;

    /**
     * Number of types, 1..255
     */
    int16_t typeCount;

    /**
     * Time of each transition in seconds from 1970 epoch.
     * Length is transitionCount int32_t's.
     */
    const int32_t *transitionTimes; // alias into res; do not delete

    /**
     * Offset from GMT in seconds for each type.
     * Length is typeCount int32_t's.
     */
    const int32_t *typeOffsets; // alias into res; do not delete

    /**
     * Type description data, consisting of transitionCount uint8_t
     * type indices (from 0..typeCount-1).
     * Length is transitionCount int8_t's.
     */
    const uint8_t *typeData; // alias into res; do not delete

    /**
     * The last year for which the transitions data are to be used
     * rather than the finalZone.  If there is no finalZone, then this
     * is set to INT32_MAX.  NOTE: This corresponds to the year _before_
     * the one indicated by finalMillis.
     */
    int32_t finalYear;

    /**
     * The millis for the start of the first year for which finalZone
     * is to be used, or DBL_MAX if finalZone is 0.  NOTE: This is
     * 0:00 GMT Jan 1, <finalYear + 1> (not <finalMillis>).
     */
    double finalMillis;

    /**
     * A SimpleTimeZone that governs the behavior for years > finalYear.
     * If and only if finalYear == INT32_MAX then finalZone == 0.
     */
    SimpleTimeZone *finalZone; // owned, may be NULL

    /* BasicTimeZone support */
    void clearTransitionRules(void);
    void deleteTransitionRules(void);
    void initTransitionRules(UErrorCode& status);

    InitialTimeZoneRule *initialRule;
    TimeZoneTransition  *firstTZTransition;
    int16_t             firstTZTransitionIdx;
    TimeZoneTransition  *firstFinalTZTransition;
    TimeArrayTimeZoneRule   **historicRules;
    int16_t             historicRuleCount;
    SimpleTimeZone      *finalZoneWithStartYear; // hack
    UBool               transitionRulesInitialized;
};

inline int32_t
OlsonTimeZone::zoneOffset(int16_t index) const {
    index <<= 1;
    return typeOffsets[index] + typeOffsets[index+1];
}

inline int32_t
OlsonTimeZone::rawOffset(int16_t index) const {
    return typeOffsets[(uint32_t)(index << 1)];
}

inline int32_t
OlsonTimeZone::dstOffset(int16_t index) const {
    return typeOffsets[(uint32_t)((index << 1) + 1)];
}

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING
#endif // OLSONTZ_H

//eof
