/*
*******************************************************************************
* Copyright (C) 1997-2007, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File TIMEZONE.CPP
*
* Modification History:
*
*   Date        Name        Description
*   12/05/96    clhuang     Creation.
*   04/21/97    aliu        General clean-up and bug fixing.
*   05/08/97    aliu        Fixed Hashtable code per code review.
*   07/09/97    helena      Changed createInstance to createDefault.
*   07/29/97    aliu        Updated with all-new list of 96 UNIX-derived
*                           TimeZones.  Changed mechanism to load from static
*                           array rather than resource bundle.
*   07/07/1998  srl         Bugfixes from the Java side: UTC GMT CAT NST
*                           Added getDisplayName API
*                           going to add custom parsing.
*
*                           ISSUES:
*                               - should getDisplayName cache something?
*                               - should custom time zones be cached? [probably]
*  08/10/98     stephen     Brought getDisplayName() API in-line w/ conventions
*  08/19/98     stephen     Changed createTimeZone() to never return 0
*  09/02/98     stephen     Added getOffset(monthLen) and hasSameRules()
*  09/15/98     stephen     Added getStaticClassID()
*  02/22/99     stephen     Removed character literals for EBCDIC safety
*  05/04/99     stephen     Changed initDefault() for Mutex issues
*  07/12/99     helena      HPUX 11 CC Port.
*  12/03/99     aliu        Moved data out of static table into icudata.dll.
*                           Substantial rewrite of zone lookup, default zone, and
*                           available IDs code.  Misc. cleanup.
*********************************************************************************/

#include "unicode/utypes.h"
#include "unicode/ustring.h"

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
static char gStrBuf[256];
#define U_DEBUG_TZ_STR(x) u_austrncpy(gStrBuf,x,sizeof(gStrBuf)-1)
// must use double parens, i.e.:  U_DEBUG_TZ_MSG(("four is: %d",4));
#define U_DEBUG_TZ_MSG(x) {debug_tz_loc(__FILE__,__LINE__);debug_tz_msg x;}
#else
#define U_DEBUG_TZ_MSG(x)
#endif

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/smpdtfmt.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/ures.h"
#include "gregoimp.h"
#include "uresimp.h" // struct UResourceBundle
#include "olsontz.h"
#include "mutex.h"
#include "unicode/udata.h"
#include "ucln_in.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/strenum.h"
#include "uassert.h"

#define kZONEINFO "zoneinfo"
#define kREGIONS  "Regions"
#define kZONES    "Zones"
#define kRULES    "Rules"
#define kNAMES    "Names"
#define kDEFAULT  "Default"
#define kMAX_CUSTOM_HOUR    23
#define kMAX_CUSTOM_MIN     59
#define kMAX_CUSTOM_SEC     59
#define MINUS 0x002D
#define PLUS 0x002B
#define ZERO_DIGIT 0x0030

// Static data and constants

static const UChar         GMT_ID[] = {0x47, 0x4D, 0x54, 0x00}; /* "GMT" */
static const UChar         Z_STR[] = {0x7A, 0x00}; /* "z" */
static const UChar         ZZZZ_STR[] = {0x7A, 0x7A, 0x7A, 0x7A, 0x00}; /* "zzzz" */
static const int32_t       GMT_ID_LENGTH = 3;

static UMTX                             LOCK;
static UMTX                             TZSET_LOCK;
static U_NAMESPACE_QUALIFIER TimeZone*  DEFAULT_ZONE = NULL;
static U_NAMESPACE_QUALIFIER TimeZone*  _GMT = NULL; // cf. TimeZone::GMT

static char TZDATA_VERSION[16] = "";

#ifdef U_USE_TIMEZONE_OBSOLETE_2_8
static U_NAMESPACE_QUALIFIER UnicodeString* OLSON_IDS = 0;
#endif

U_CDECL_BEGIN
static UBool U_CALLCONV timeZone_cleanup(void)
{
#ifdef U_USE_TIMEZONE_OBSOLETE_2_8
    delete []OLSON_IDS;
    OLSON_IDS = 0;
#endif

    delete DEFAULT_ZONE;
    DEFAULT_ZONE = NULL;

    delete _GMT;
    _GMT = NULL;

    uprv_memset(TZDATA_VERSION, 0, sizeof(TZDATA_VERSION));

    if (LOCK) {
        umtx_destroy(&LOCK);
        LOCK = NULL;
    }
    if (TZSET_LOCK) {
        umtx_destroy(&TZSET_LOCK);
        TZSET_LOCK = NULL;
    }

    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

/**
 * The Olson data is stored the "zoneinfo" resource bundle.
 * Sub-resources are organized into three ranges of data: Zones, final
 * rules, and country tables.  There is also a meta-data resource
 * which has 3 integers: The number of zones, rules, and countries,
 * respectively.  The country count includes the non-country 'Default'.
 */
static int32_t OLSON_ZONE_START = -1; // starting index of zones
static int32_t OLSON_ZONE_COUNT = 0;  // count of zones

/**
 * Given a pointer to an open "zoneinfo" resource, load up the Olson
 * meta-data. Return TRUE if successful.
 */
static UBool getOlsonMeta(const UResourceBundle* top) {
    if (OLSON_ZONE_START < 0) {
        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle res;
        ures_initStackObject(&res);
        ures_getByKey(top, kZONES, &res, &ec);
        if(U_SUCCESS(ec)) {
          OLSON_ZONE_COUNT = ures_getSize(&res);
          OLSON_ZONE_START = 0;
          U_DEBUG_TZ_MSG(("OZC%d OZS%d\n",OLSON_ZONE_COUNT, OLSON_ZONE_START));
        }
        ures_close(&res);
    }
    return (OLSON_ZONE_START >= 0);
}

/**
 * Load up the Olson meta-data. Return TRUE if successful.
 */
static UBool getOlsonMeta() {
    if (OLSON_ZONE_START < 0) {
        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        if (U_SUCCESS(ec)) {
          getOlsonMeta(top);
        }
        ures_close(top);
    }
    return (OLSON_ZONE_START >= 0);
}

static int32_t findInStringArray(UResourceBundle* array, const UnicodeString& id, UErrorCode &status)
{
    UnicodeString copy;
    const UChar *u;
    int32_t len;
    
    int32_t start = 0;
    int32_t limit = ures_getSize(array);
    int32_t mid;
    int32_t lastMid = INT32_MAX;
    if(U_FAILURE(status) || (limit < 1)) { 
        return -1;
    }
    U_DEBUG_TZ_MSG(("fisa: Looking for %s, between %d and %d\n", U_DEBUG_TZ_STR(UnicodeString(id).getTerminatedBuffer()), start, limit));
    
    for (;;) {
        mid = (int32_t)((start + limit) / 2);
        if (lastMid == mid) {   /* Have we moved? */
            break;  /* We haven't moved, and it wasn't found. */
        }
        lastMid = mid;
        u = ures_getStringByIndex(array, mid, &len, &status);
        if (U_FAILURE(status)) {
            break;
        }
        U_DEBUG_TZ_MSG(("tz: compare to %s, %d .. [%d] .. %d\n", U_DEBUG_TZ_STR(u), start, mid, limit));
        copy.setTo(TRUE, u, len);
        int r = id.compare(copy);
        if(r==0) {
            U_DEBUG_TZ_MSG(("fisa: found at %d\n", mid));
            return mid;
        } else if(r<0) {
            limit = mid;
        } else {
            start = mid;
        }
    }
    U_DEBUG_TZ_MSG(("fisa: not found\n"));
    return -1;
}

/**
 * Fetch a specific zone by name.  Replaces the getByKey call. 
 * @param top Top timezone resource
 * @param id Time zone ID
 * @param oldbundle Bundle for reuse (or NULL).   see 'ures_open()'
 * @return the zone's bundle if found, or undefined if error.  Reuses oldbundle.
 */
static UResourceBundle* getZoneByName(const UResourceBundle* top, const UnicodeString& id, UResourceBundle *oldbundle, UErrorCode& status) {
    // load the Rules object
    UResourceBundle *tmp = ures_getByKey(top, kNAMES, NULL, &status);
    
    // search for the string
    int32_t idx = findInStringArray(tmp, id, status);
    
    if((idx == -1) && U_SUCCESS(status)) {
        // not found 
        status = U_MISSING_RESOURCE_ERROR;
        //ures_close(oldbundle);
        //oldbundle = NULL;
    } else {
        U_DEBUG_TZ_MSG(("gzbn: oldbundle= size %d, type %d, %s\n", ures_getSize(tmp), ures_getType(tmp), u_errorName(status)));
        tmp = ures_getByKey(top, kZONES, tmp, &status); // get Zones object from top
        U_DEBUG_TZ_MSG(("gzbn: loaded ZONES, size %d, type %d, path %s %s\n", ures_getSize(tmp), ures_getType(tmp), ures_getPath(tmp), u_errorName(status)));
        oldbundle = ures_getByIndex(tmp, idx, oldbundle, &status); // get nth Zone object
        U_DEBUG_TZ_MSG(("gzbn: loaded z#%d, size %d, type %d, path %s, %s\n", idx, ures_getSize(oldbundle), ures_getType(oldbundle), ures_getPath(oldbundle),  u_errorName(status)));
    }
    ures_close(tmp);
    if(U_FAILURE(status)) { 
        //ures_close(oldbundle);
        return NULL;
    } else {
        return oldbundle;
    }
}


UResourceBundle* TimeZone::loadRule(const UResourceBundle* top, const UnicodeString& ruleid, UResourceBundle* oldbundle, UErrorCode& status) {
    char key[64];
    ruleid.extract(0, sizeof(key)-1, key, (int32_t)sizeof(key)-1, US_INV);
    U_DEBUG_TZ_MSG(("loadRule(%s)\n", key));
    UResourceBundle *r = ures_getByKey(top, kRULES, oldbundle, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> kRULES [%s]\n", key, u_errorName(status)));
    r = ures_getByKey(r, key, r, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> item [%s]\n", key, u_errorName(status)));
    return r;
}

/**
 * Given an ID, open the appropriate resource for the given time zone.
 * Dereference aliases if necessary.
 * @param id zone id
 * @param res resource, which must be ready for use (initialized but not open)
 * @param ec input-output error code
 * @return top-level resource bundle
 */
static UResourceBundle* openOlsonResource(const UnicodeString& id,
                                          UResourceBundle& res,
                                          UErrorCode& ec)
{
#if U_DEBUG_TZ
    char buf[128];
    id.extract(0, sizeof(buf)-1, buf, sizeof(buf), "");
#endif
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    U_DEBUG_TZ_MSG(("pre: res sz=%d\n", ures_getSize(&res)));
    /* &res = */ getZoneByName(top, id, &res, ec);
    // Dereference if this is an alias.  Docs say result should be 1
    // but it is 0 in 2.8 (?).
    U_DEBUG_TZ_MSG(("Loading zone '%s' (%s, size %d) - %s\n", buf, ures_getKey((UResourceBundle*)&res), ures_getSize(&res), u_errorName(ec)));
    if (ures_getSize(&res) <= 1 && getOlsonMeta(top)) {
        int32_t deref = ures_getInt(&res, &ec) + 0;
        U_DEBUG_TZ_MSG(("getInt: %s - type is %d\n", u_errorName(ec), ures_getType(&res)));
        UResourceBundle *ares = ures_getByKey(top, kZONES, NULL, &ec); // dereference Zones section
        ures_getByIndex(ares, deref, &res, &ec);
        ures_close(ares);
        U_DEBUG_TZ_MSG(("alias to #%d (%s) - %s\n", deref, "??", u_errorName(ec)));
    } else {
        U_DEBUG_TZ_MSG(("not an alias - size %d\n", ures_getSize(&res)));
    }
    U_DEBUG_TZ_MSG(("%s - final status is %s\n", buf, u_errorName(ec)));
    return top;
}

#ifdef U_USE_TIMEZONE_OBSOLETE_2_8

/**
 * Load all the ids from the "zoneinfo" resource bundle into a static
 * array that we hang onto.  This is _only_ used to implement the
 * deprecated createAvailableIDs() API.
 */
static UBool loadOlsonIDs() {
    if (OLSON_IDS != 0) {
        return TRUE;
    }

    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString* ids = 0;
    int32_t count = 0;
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    UResourceBundle *nres = ures_getByKey(top, kNAMES, NULL, &ec); // dereference Names section
    if (U_SUCCESS(ec)) {
        getOlsonMeta(top);
        int32_t start = 0;
        count = ures_getSize(nres);
        ids = new UnicodeString[(count > 0) ? count : 1];
        for (int32_t i=0; i<count; ++i) {
            int32_t idLen = 0;
            const UChar* id = ures_getStringByIndex(nres, i, &idLen, &ec);
            ids[i].fastCopyFrom(UnicodeString(TRUE, id, idLen));
            if (U_FAILURE(ec)) {
                break;
            }
        }
    }
    ures_close(nres);
    ures_close(top);

    if (U_FAILURE(ec)) {
        delete[] ids;
        return FALSE;
    }

    // Keep mutexed operations as short as possible by doing all
    // computations first, then doing pointer copies within the mutex.
    umtx_lock(&LOCK);
    if (OLSON_IDS == 0) {
        OLSON_IDS = ids;
        ids = 0;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
    umtx_unlock(&LOCK);

    // If another thread initialized the statics first, then delete
    // our unused data.
    delete[] ids;
    return TRUE;
}

#endif

// -------------------------------------

const TimeZone* U_EXPORT2
TimeZone::getGMT(void)
{
    UBool needsInit;
    UMTX_CHECK(&LOCK, (_GMT == NULL), needsInit);   /* This is here to prevent race conditions. */

    // Initialize _GMT independently of other static data; it should
    // be valid even if we can't load the time zone UDataMemory.
    if (needsInit) {
        umtx_lock(&LOCK);
        if (_GMT == 0) {
            _GMT = new SimpleTimeZone(0, UnicodeString(TRUE, GMT_ID, GMT_ID_LENGTH));
            ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        }
        umtx_unlock(&LOCK);
    }
    return _GMT;
}

// *****************************************************************************
// class TimeZone
// *****************************************************************************

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(TimeZone)

TimeZone::TimeZone()
    :   UObject(), fID()
{
}

// -------------------------------------

TimeZone::TimeZone(const UnicodeString &id)
    :   UObject(), fID(id)
{
}

// -------------------------------------

TimeZone::~TimeZone()
{
}

// -------------------------------------

TimeZone::TimeZone(const TimeZone &source)
    :   UObject(source), fID(source.fID)
{
}

// -------------------------------------

TimeZone &
TimeZone::operator=(const TimeZone &right)
{
    if (this != &right) fID = right.fID;
    return *this;
}

// -------------------------------------

UBool
TimeZone::operator==(const TimeZone& that) const
{
    return getDynamicClassID() == that.getDynamicClassID() &&
        fID == that.fID;
}

// -------------------------------------

TimeZone* U_EXPORT2
TimeZone::createTimeZone(const UnicodeString& ID)
{
    /* We first try to lookup the zone ID in our system list.  If this
     * fails, we try to parse it as a custom string GMT[+-]hh:mm.  If
     * all else fails, we return GMT, which is probably not what the
     * user wants, but at least is a functioning TimeZone object.
     *
     * We cannot return NULL, because that would break compatibility
     * with the JDK.
     */
    TimeZone* result = createSystemTimeZone(ID);

    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load system time zone with id - falling to custom"));
        result = createCustomTimeZone(ID);
    }
    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load time zone with id - falling to GMT"));
        result = getGMT()->clone();
    }
    return result;
}

/**
 * Lookup the given name in our system zone table.  If found,
 * instantiate a new zone of that name and return it.  If not
 * found, return 0.
 */
TimeZone*
TimeZone::createSystemTimeZone(const UnicodeString& id) {
    TimeZone* z = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("pre-err=%s\n", u_errorName(ec)));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    U_DEBUG_TZ_MSG(("post-err=%s\n", u_errorName(ec)));
    if (U_SUCCESS(ec)) {
        z = new OlsonTimeZone(top, &res, ec);
        if (z) {
          z->setID(id);
        } else {
          U_DEBUG_TZ_MSG(("cstz: olson time zone failed to initialize - err %s\n", u_errorName(ec)));
        }
    }
    ures_close(&res);
    ures_close(top);
    if (U_FAILURE(ec)) {
        U_DEBUG_TZ_MSG(("cstz: failed to create, err %s\n", u_errorName(ec)));
        delete z;
        z = 0;
    }
    return z;
}

// -------------------------------------

/**
 * Initialize DEFAULT_ZONE from the system default time zone.  The
 * caller should confirm that DEFAULT_ZONE is NULL before calling.
 * Upon return, DEFAULT_ZONE will not be NULL, unless operator new()
 * returns NULL.
 *
 * Must be called OUTSIDE mutex.
 */
void
TimeZone::initDefault()
{ 
    // We access system timezone data through TPlatformUtilities,
    // including tzset(), timezone, and tzname[].
    int32_t rawOffset = 0;
    const char *hostID;

    // First, try to create a system timezone, based
    // on the string ID in tzname[0].
    {
        // NOTE: Local mutex here. TimeZone mutex below
        // mutexed to avoid threading issues in the platform functions.
        // Some of the locale/timezone OS functions may not be thread safe,
        // so the intent is that any setting from anywhere within ICU
        // happens while the ICU mutex is held.
        // The operating system might actually use ICU to implement timezones.
        // So we may have ICU calling ICU here, like on AIX.
        // In order to prevent a double lock of a non-reentrant mutex in a
        // different part of ICU, we use TZSET_LOCK to allow only one instance
        // of ICU to query these thread unsafe OS functions at any given time.
        Mutex lock(&TZSET_LOCK);

        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        uprv_tzset(); // Initialize tz... system data
        
        // Get the timezone ID from the host.  This function should do
        // any required host-specific remapping; e.g., on Windows this
        // function maps the Date and Time control panel setting to an
        // ICU timezone ID.
        hostID = uprv_tzname(0);
        
        // Invert sign because UNIX semantics are backwards
        rawOffset = uprv_timezone() * -U_MILLIS_PER_SECOND;
    }

    UBool initialized;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE != NULL), initialized);
    if (initialized) {
        /* Hrmph? Either a race condition happened, or tzset initialized ICU. */
        return;
    }

    TimeZone* default_zone = NULL;

    /* Make sure that the string is NULL terminated to prevent BoundsChecker/Purify warnings. */
    UnicodeString hostStrID(hostID, -1, US_INV);
    hostStrID.append((UChar)0);
    hostStrID.truncate(hostStrID.length()-1);
    default_zone = createSystemTimeZone(hostStrID);

    int32_t hostIDLen = hostStrID.length();
    if (default_zone != NULL && rawOffset != default_zone->getRawOffset()
        && (3 <= hostIDLen && hostIDLen <= 4))
    {
        // Uh oh. This probably wasn't a good id.
        // It was probably an ambiguous abbreviation
        delete default_zone;
        default_zone = NULL;
    }

#if 0
    // NOTE: As of ICU 2.8, we no longer have an offsets table, since
    // historical zones can change offset over time.  If we add
    // build-time heuristics to infer the "most frequent" raw offset
    // of a zone, we can build tables and institute defaults, as done
    // in ICU <= 2.6.

    // If we couldn't get the time zone ID from the host, use
    // the default host timezone offset.  Further refinements
    // to this include querying the host to determine if DST
    // is in use or not and possibly using the host locale to
    // select from multiple zones at a the same offset.  We
    // don't do any of this now, but we could easily add this.
    if (default_zone == NULL) {
        // Use the designated default in the time zone list that has the
        // appropriate GMT offset, if there is one.

        const OffsetIndex* index = INDEX_BY_OFFSET;

        for (;;) {
            if (index->gmtOffset > rawOffset) {
                // Went past our desired offset; no match found
                break;
            }
            if (index->gmtOffset == rawOffset) {
                // Found our desired offset
                default_zone = createSystemTimeZone(ZONE_IDS[index->defaultZone]);
                break;
            }
            // Compute the position of the next entry.  If the delta value
            // in this entry is zero, then there is no next entry.
            uint16_t delta = index->nextEntryDelta;
            if (delta == 0) {
                break;
            }
            index = (const OffsetIndex*)((int8_t*)index + delta);
        }
    }
#endif

    // Construct a fixed standard zone with the host's ID
    // and raw offset.
    if (default_zone == NULL) {
        default_zone = new SimpleTimeZone(rawOffset, hostStrID);
    }

    // If we _still_ don't have a time zone, use GMT.
    if (default_zone == NULL) {
        default_zone = getGMT()->clone();
    }

    // If DEFAULT_ZONE is still NULL, set it up.
    umtx_lock(&LOCK);
    if (DEFAULT_ZONE == NULL) {
        DEFAULT_ZONE = default_zone;
        default_zone = NULL;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
    umtx_unlock(&LOCK);

    delete default_zone;
}

// -------------------------------------

TimeZone* U_EXPORT2
TimeZone::createDefault()
{
    /* This is here to prevent race conditions. */
    UBool needsInit;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE == NULL), needsInit);
    if (needsInit) {
        initDefault();
    }

    Mutex lock(&LOCK); // In case adoptDefault is called
    return (DEFAULT_ZONE != NULL) ? DEFAULT_ZONE->clone() : NULL;
}

// -------------------------------------

void U_EXPORT2
TimeZone::adoptDefault(TimeZone* zone)
{
    if (zone != NULL)
    {
        TimeZone* old = NULL;

        umtx_init(&LOCK);   /* This is here to prevent race conditions. */
        umtx_lock(&LOCK);
        old = DEFAULT_ZONE;
        DEFAULT_ZONE = zone;
        umtx_unlock(&LOCK);

        delete old;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
}
// -------------------------------------

void U_EXPORT2
TimeZone::setDefault(const TimeZone& zone)
{
    adoptDefault(zone.clone());
}

//----------------------------------------------------------------------

/**
 * This is the default implementation for subclasses that do not
 * override this method.  This implementation calls through to the
 * 8-argument getOffset() method after suitable computations, and
 * correctly adjusts GMT millis to local millis when necessary.
 */
void TimeZone::getOffset(UDate date, UBool local, int32_t& rawOffset,
                         int32_t& dstOffset, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }

    rawOffset = getRawOffset();
    if (!local) {
        date += rawOffset; // now in local standard millis
    }

    // When local == TRUE, date might not be in local standard
    // millis.  getOffset taking 7 parameters used here assume
    // the given time in day is local standard time.
    // At STD->DST transition, there is a range of time which
    // does not exist.  When 'date' is in this time range
    // (and local == TRUE), this method interprets the specified
    // local time as DST.  At DST->STD transition, there is a
    // range of time which occurs twice.  In this case, this
    // method interprets the specified local time as STD.
    // To support the behavior above, we need to call getOffset
    // (with 7 args) twice when local == true and DST is
    // detected in the initial call.
    for (int32_t pass=0; ; ++pass) {
        int32_t year, month, dom, dow;
        double day = uprv_floor(date / U_MILLIS_PER_DAY);
        int32_t millis = (int32_t) (date - day * U_MILLIS_PER_DAY);

        Grego::dayToFields(day, year, month, dom, dow);

        dstOffset = getOffset(GregorianCalendar::AD, year, month, dom,
                              (uint8_t) dow, millis,
                              Grego::monthLength(year, month),
                              ec) - rawOffset;

        // Recompute if local==TRUE, dstOffset!=0.
        if (pass!=0 || !local || dstOffset == 0) {
            break;
        }
        // adjust to local standard millis
        date -= dstOffset;
    }
}

// -------------------------------------

// New available IDs API as of ICU 2.4.  Uses StringEnumeration API.

class TZEnumeration : public StringEnumeration {
private:

    // Map into to zones.  Our results are zone[map[i]] for
    // i=0..len-1, where zone[i] is the i-th Olson zone.  If map==NULL
    // then our results are zone[i] for i=0..len-1.  Len will be zero
    // iff the zone data could not be loaded.
    int32_t* map;
    int32_t  len;
    int32_t  pos;

    UBool getID(int32_t i) {
        UErrorCode ec = U_ZERO_ERROR;
        int32_t idLen = 0;
        const UChar* id = NULL;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        top = ures_getByKey(top, kNAMES, top, &ec); // dereference Zones section
        id = ures_getStringByIndex(top, i, &idLen, &ec);
        if(U_FAILURE(ec)) {
            unistr.truncate(0);
        }
        else {
            unistr.fastCopyFrom(UnicodeString(TRUE, id, idLen));
        }
        ures_close(top);
        return U_SUCCESS(ec);
    }

public:
    TZEnumeration() : map(NULL), len(0), pos(0) {
        if (getOlsonMeta()) {
            len = OLSON_ZONE_COUNT;
        }
    }

    TZEnumeration(int32_t rawOffset) : map(NULL), len(0), pos(0) {
        if (!getOlsonMeta()) {
            return;
        }

        // Allocate more space than we'll need.  The end of the array will
        // be blank.
        map = (int32_t*)uprv_malloc(OLSON_ZONE_COUNT * sizeof(int32_t));
        if (map == 0) {
            return;
        }

        uprv_memset(map, 0, sizeof(int32_t) * OLSON_ZONE_COUNT);

        UnicodeString s;
        for (int32_t i=0; i<OLSON_ZONE_COUNT; ++i) {
            if (getID(i)) {
                // This is VERY inefficient.
                TimeZone* z = TimeZone::createTimeZone(unistr);
                // Make sure we get back the ID we wanted (if the ID is
                // invalid we get back GMT).
                if (z != 0 && z->getID(s) == unistr &&
                    z->getRawOffset() == rawOffset) {
                    map[len++] = i;
                }
                delete z;
            }
        }
    }

    TZEnumeration(const char* country) : map(NULL), len(0), pos(0) {
        if (!getOlsonMeta()) {
            return;
        }

        char key[] = {0, 0, 0, 0,0, 0, 0,0, 0, 0,0}; // e.g., "US", or "Default" for no country
        if (country)  {
          uprv_strncat(key, country, 2);
        } else {
          uprv_strcpy(key, kDEFAULT);
        }

        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        top = ures_getByKey(top, kREGIONS, top, &ec); // dereference 'Regions' section
        if (U_SUCCESS(ec)) {
            UResourceBundle res;
            ures_initStackObject(&res);
            ures_getByKey(top, key, &res, &ec);
            // The list of zones is a list of integers, from 0..n-1,
            // where n is the total number of system zones.
            const int32_t* v = ures_getIntVector(&res, &len, &ec);
            if (U_SUCCESS(ec)) {
                U_ASSERT(len > 0);
                map = (int32_t*)uprv_malloc(sizeof(int32_t) * len);
                if (map != 0) {
                    for (uint16_t i=0; i<len; ++i) {
                        U_ASSERT(v[i] >= 0 && v[i] < OLSON_ZONE_COUNT);
                        map[i] = v[i];
                    }
                }
            } else {
              U_DEBUG_TZ_MSG(("Failed to load tz for region %s: %s\n", country, u_errorName(ec)));
            }
            ures_close(&res);
        }
        ures_close(top);
    }

  TZEnumeration(const TZEnumeration &other) : StringEnumeration(), map(NULL), len(0), pos(0) {
        if(other.len > 0) {
            if(other.map != NULL) {
                map = (int32_t *)uprv_malloc(other.len * sizeof(int32_t));
                if(map != NULL) {
                    len = other.len;
                    uprv_memcpy(map, other.map, len * sizeof(int32_t));
                    pos = other.pos;
                }
            } else {
                len = other.len;
                pos = other.pos;
            }
        }
    }

    virtual ~TZEnumeration() {
        uprv_free(map);
    }

    virtual StringEnumeration *clone() const {
        return new TZEnumeration(*this);
    }

    virtual int32_t count(UErrorCode& status) const {
        return U_FAILURE(status) ? 0 : len;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        if (U_SUCCESS(status) && pos < len) {
            getID((map == 0) ? pos : map[pos]);
            ++pos;
            return &unistr;
        }
        return 0;
    }

    virtual void reset(UErrorCode& /*status*/) {
        pos = 0;
    }

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TZEnumeration)

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration() {
    return new TZEnumeration();
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(int32_t rawOffset) {
    return new TZEnumeration(rawOffset);
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(const char* country) {
    return new TZEnumeration(country);
}

// -------------------------------------

#ifdef U_USE_TIMEZONE_OBSOLETE_2_8

const UnicodeString** 
TimeZone::createAvailableIDs(int32_t rawOffset, int32_t& numIDs)
{
    // We are creating a new array to existing UnicodeString pointers.
    // The caller will delete the array when done, but not the pointers
    // in the array.

    numIDs = 0;
    if (!loadOlsonIDs()) {
        return 0;
    }

    // Allocate more space than we'll need.  The end of the array will
    // be blank.
    const UnicodeString** ids =
        (const UnicodeString** )uprv_malloc(OLSON_ZONE_COUNT * sizeof(UnicodeString *));
    if (ids == 0) {
        return 0;
    }

    uprv_memset(ids, 0, sizeof(UnicodeString*) * OLSON_ZONE_COUNT);

    UnicodeString s;
    for (int32_t i=0; i<OLSON_ZONE_COUNT; ++i) {
        // This is VERY inefficient.
        TimeZone* z = TimeZone::createTimeZone(OLSON_IDS[i]);
        // Make sure we get back the ID we wanted (if the ID is
        // invalid we get back GMT).
        if (z != 0 && z->getID(s) == OLSON_IDS[i] &&
            z->getRawOffset() == rawOffset) {
            ids[numIDs++] = &OLSON_IDS[i]; // [sic]
        }
        delete z;
    }

    return ids;
}

// -------------------------------------

const UnicodeString** 
TimeZone::createAvailableIDs(const char* country, int32_t& numIDs) {

    // We are creating a new array to existing UnicodeString pointers.
    // The caller will delete the array when done, but not the pointers
    // in the array.

    numIDs = 0;
    if (!loadOlsonIDs()) {
        return 0;
    }
    
    char key[] = { 0, 0, 0,0, 0, 0,0, 0, 0 }; // e.g., "US", or "Default" for non-country zones
    if (country) { 
      uprv_strncat(key, country, 2);
    } else {
      uprv_strcpy(key, kDEFAULT);
    }

    const UnicodeString** ids = 0;

    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    UResourceBundle *ares = ures_getByKey(top, kREGIONS, NULL, &ec); // dereference Regions section
    if (U_SUCCESS(ec)) {
        getOlsonMeta(top);
        UResourceBundle res;
        ures_initStackObject(&res);
        ures_getByKey(ares, key, &res, &ec);
        U_DEBUG_TZ_MSG(("caI: on %s, err %s\n", country, u_errorName(ec)));
        if (U_SUCCESS(ec)) {
            /* The list of zones is a list of integers, from 0..n-1,
             * where n is the total number of system zones.  The
             * numbering corresponds exactly to the ordering of
             * OLSON_IDS.
             */
            const int32_t* v = ures_getIntVector(&res, &numIDs, &ec);
            ids = (const UnicodeString**)
                uprv_malloc(numIDs * sizeof(UnicodeString*));
            if (ids == 0) {
                numIDs = 0;
            } else {
                for (int32_t i=0; i<numIDs; ++i) {
                    ids[i] = &OLSON_IDS[v[i]]; // [sic]
                }
            }
        }
        ures_close(&res);
    }
    ures_close(ares);
    ures_close(top);

    return ids;
}

// -------------------------------------

const UnicodeString** 
TimeZone::createAvailableIDs(int32_t& numIDs)
{
    // We are creating a new array to existing UnicodeString pointers.
    // The caller will delete the array when done, but not the pointers
    // in the array.
    numIDs = 0;
    if (!loadOlsonIDs()) {
        return 0;
    }
    
    const UnicodeString** ids =
        (const UnicodeString** )uprv_malloc(OLSON_ZONE_COUNT * sizeof(UnicodeString *));
    if (ids != 0) {
        numIDs = OLSON_ZONE_COUNT;
        for (int32_t i=0; i<numIDs; ++i) {
            ids[i] = &OLSON_IDS[i];
        }
    }

    return ids;
}

#endif

// ---------------------------------------

int32_t U_EXPORT2
TimeZone::countEquivalentIDs(const UnicodeString& id) {
    int32_t result = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("countEquivalentIDs..\n"));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    if (U_SUCCESS(ec)) {
        int32_t size = ures_getSize(&res);
        U_DEBUG_TZ_MSG(("cEI: success (size %d, key %s)..\n", size, ures_getKey(&res)));
        if (size == 4 || size == 6) {
            UResourceBundle r;
            ures_initStackObject(&r);
            ures_getByIndex(&res, size-1, &r, &ec);
            //result = ures_getSize(&r); // doesn't work
            ures_getIntVector(&r, &result, &ec);
            U_DEBUG_TZ_MSG(("ceI: result %d, err %s\n", result, u_errorName(ec)));
            ures_close(&r);
        }
    } else {
      U_DEBUG_TZ_MSG(("cEI: fail, %s\n", u_errorName(ec)));
    }
    ures_close(&res);
    ures_close(top);
    return result;
}

// ---------------------------------------

const UnicodeString U_EXPORT2
TimeZone::getEquivalentID(const UnicodeString& id, int32_t index) {
    U_DEBUG_TZ_MSG(("gEI(%d)\n", index));
    UnicodeString result;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    UResourceBundle *top = openOlsonResource(id, res, ec);
    int32_t zone = -1;
    if (U_SUCCESS(ec)) {
        int32_t size = ures_getSize(&res);
        if (size == 4 || size == 6) {
            UResourceBundle r;
            ures_initStackObject(&r);
            ures_getByIndex(&res, size-1, &r, &ec);
            const int32_t* v = ures_getIntVector(&r, &size, &ec);
            if (index >= 0 && index < size && getOlsonMeta()) {
                zone = v[index];
            }
            ures_close(&r);
        }
    }
    ures_close(&res);
    if (zone >= 0) {
        UResourceBundle *ares = ures_getByKey(top, kNAMES, NULL, &ec); // dereference Zones section
        if (U_SUCCESS(ec)) {
            int32_t idLen = 0;
            const UChar* id = ures_getStringByIndex(ares, zone, &idLen, &ec);
            result.fastCopyFrom(UnicodeString(TRUE, id, idLen));
            U_DEBUG_TZ_MSG(("gei(%d) -> %d, len%d, %s\n", index, zone, result.length(), u_errorName(ec)));
        }
        ures_close(ares);
    }
    ures_close(top);
#if defined(U_DEBUG_TZ)
    if(result.length() ==0) {
      U_DEBUG_TZ_MSG(("equiv [__, #%d] -> 0 (%s)\n", index, u_errorName(ec)));
    }
#endif
    return result;
}

// ---------------------------------------

UnicodeString&
TimeZone::getOlsonCanonicalID(const UnicodeString &id, UnicodeString &canonical) {
    UErrorCode ec = U_ZERO_ERROR;
    canonical.remove();
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    UResourceBundle *res = getZoneByName(top, id, NULL, ec);
    if (U_SUCCESS(ec)) {
        if (ures_getSize(res) == 1) {
            int32_t deref = ures_getInt(res, &ec);
            UResourceBundle *nres = ures_getByKey(top, kNAMES, NULL, &ec); // dereference Names section
            int32_t len;
            const UChar* tmp = ures_getStringByIndex(nres, deref, &len, &ec);
            if (U_SUCCESS(ec)) {
                canonical.setTo(tmp, len);
            }
            ures_close(nres);
        } else {
            canonical.setTo(id);
        }
    }
    ures_close(res);
    ures_close(top);
    return canonical;
}

// ---------------------------------------


UnicodeString&
TimeZone::getDisplayName(UnicodeString& result) const
{
    return getDisplayName(FALSE,LONG,Locale::getDefault(), result);
}

UnicodeString&
TimeZone::getDisplayName(const Locale& locale, UnicodeString& result) const
{
    return getDisplayName(FALSE, LONG, locale, result);
}

UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, UnicodeString& result)  const
{
    return getDisplayName(daylight,style, Locale::getDefault(), result);
}
//--------------------------------------
int32_t 
TimeZone::getDSTSavings()const {
    if (useDaylightTime()) {
        return 3600000;
    }
    return 0;
}
//---------------------------------------
UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, const Locale& locale, UnicodeString& result) const
{
    // SRL TODO: cache the SDF, just like java.
    UErrorCode status = U_ZERO_ERROR;
#ifdef U_DEBUG_TZ
    char buf[128];
    fID.extract(0, sizeof(buf)-1, buf, sizeof(buf), "");
#endif
    SimpleDateFormat format(style == LONG ? ZZZZ_STR : Z_STR,locale,status);
    U_DEBUG_TZ_MSG(("getDisplayName(%s)\n", buf));
    if(!U_SUCCESS(status))
    {
#ifdef U_DEBUG_TZ
      char buf2[128];
      result.extract(0, sizeof(buf2)-1, buf2, sizeof(buf2), "");
      U_DEBUG_TZ_MSG(("getDisplayName(%s) -> %s\n", buf, buf2));
#endif
      return result.remove();
    }

    // Create a new SimpleTimeZone as a stand-in for this zone; the
    // stand-in will have no DST, or all DST, but the same ID and offset,
    // and hence the same display name.
    // We don't cache these because they're small and cheap to create.
    UnicodeString tempID;
    SimpleTimeZone *tz = NULL;
    if(daylight  && useDaylightTime()){
        // For the pure-DST zone, we use JANUARY and DECEMBER        
        int savings = getDSTSavings();
        tz = new SimpleTimeZone(getRawOffset(), getID(tempID),
                                UCAL_JANUARY, 1, 0, 0,
                                UCAL_FEBRUARY, 1, 0, 0,
                                savings, status);
    }else{
        tz = new SimpleTimeZone(getRawOffset(), getID(tempID));
    }
    format.applyPattern(style == LONG ? ZZZZ_STR : Z_STR);
    Calendar *myCalendar = (Calendar*)format.getCalendar();
    myCalendar->setTimeZone(*tz); // copy
    
    delete tz;

    FieldPosition pos(FieldPosition::DONT_CARE);
    return format.format(UDate(864000000L), result, pos); // Must use a valid date here.
}


/**
 * Parse a custom time zone identifier and return a corresponding zone.
 * @param id a string of the form GMT[+-]hh:mm, GMT[+-]hhmm, or
 * GMT[+-]hh.
 * @return a newly created SimpleTimeZone with the given offset and
 * no Daylight Savings Time, or null if the id cannot be parsed.
*/
TimeZone*
TimeZone::createCustomTimeZone(const UnicodeString& id)
{
    static const int32_t         kParseFailed = -99999;

    NumberFormat* numberFormat = 0;
    
    UnicodeString idUppercase = id;
    idUppercase.toUpper();

    if (id.length() > GMT_ID_LENGTH &&
        idUppercase.startsWith(GMT_ID))
    {
        ParsePosition pos(GMT_ID_LENGTH);
        UBool negative = FALSE;
        int32_t hour = 0;
        int32_t min = 0;
        int32_t sec = 0;

        if (id[pos.getIndex()] == MINUS /*'-'*/)
            negative = TRUE;
        else if (id[pos.getIndex()] != PLUS /*'+'*/)
            return NULL;
        pos.setIndex(pos.getIndex() + 1);

        UErrorCode success = U_ZERO_ERROR;
        numberFormat = NumberFormat::createInstance(success);
        if(U_FAILURE(success)){
            return NULL;
        }
        numberFormat->setParseIntegerOnly(TRUE);

    
        // Look for either hh:mm, hhmm, or hh
        int32_t start = pos.getIndex();
        
        Formattable n(kParseFailed);

        numberFormat->parse(id, n, pos);
        if (pos.getIndex() == start) {
            delete numberFormat;
            return NULL;
        }
        hour = n.getLong();

        if (pos.getIndex() < id.length()) {
            if (pos.getIndex() - start > 2
                || id[pos.getIndex()] != 0x003A /*':'*/) {
                delete numberFormat;
                return NULL;
            }
            // hh:mm
            pos.setIndex(pos.getIndex() + 1);
            int32_t oldPos = pos.getIndex();
            n.setLong(kParseFailed);
            numberFormat->parse(id, n, pos);
            if ((pos.getIndex() - oldPos) != 2) {
                // must be 2 digits
                delete numberFormat;
                return NULL;
            }
            min = n.getLong();
            if (pos.getIndex() < id.length()) {
                if (id[pos.getIndex()] != 0x003A /*':'*/) {
                    delete numberFormat;
                    return NULL;
                }
                // [:ss]
                pos.setIndex(pos.getIndex() + 1);
                oldPos = pos.getIndex();
                n.setLong(kParseFailed);
                numberFormat->parse(id, n, pos);
                if (pos.getIndex() != id.length()
                        || (pos.getIndex() - oldPos) != 2) {
                    delete numberFormat;
                    return NULL;
                }
                sec = n.getLong();
            }
        } else {
            // Supported formats are below -
            //
            // HHmmss
            // Hmmss
            // HHmm
            // Hmm
            // HH
            // H

            int32_t length = pos.getIndex() - start;
            if (length <= 0 || 6 < length) {
                // invalid length
                delete numberFormat;
                return NULL;
            }
            switch (length) {
                case 1:
                case 2:
                    // already set to hour
                    break;
                case 3:
                case 4:
                    min = hour % 100;
                    hour /= 100;
                    break;
                case 5:
                case 6:
                    sec = hour % 100;
                    min = (hour/100) % 100;
                    hour /= 10000;
                    break;
            }
        }

        delete numberFormat;

        if (hour > kMAX_CUSTOM_HOUR || min > kMAX_CUSTOM_MIN || sec > kMAX_CUSTOM_SEC) {
            return 0;
        }

        // Create time zone ID - GMT[+|-]hhmm[ss]
        UnicodeString tzID(GMT_ID);
        if (hour | min | sec) {
            if (negative) {
                tzID += (UChar)MINUS;
            } else {
                tzID += (UChar)PLUS;
            }

            if (hour < 10) {
                tzID += (UChar)ZERO_DIGIT;
            } else {
                tzID += (UChar)(ZERO_DIGIT + hour/10);
            }
            tzID += (UChar)(ZERO_DIGIT + hour%10);

            if (min < 10) {
                tzID += (UChar)ZERO_DIGIT;
            } else {
                tzID += (UChar)(ZERO_DIGIT + min/10);
            }
            tzID += (UChar)(ZERO_DIGIT + min%10);

            if (sec) {
                if (sec < 10) {
                    tzID += (UChar)ZERO_DIGIT;
                } else {
                    tzID += (UChar)(ZERO_DIGIT + sec/10);
                }
                tzID += (UChar)(ZERO_DIGIT + sec%10);
            }
        }

        int32_t offset = ((hour * 60 + min) * 60 + sec) * 1000;
        if (negative) {
            offset = -offset;
        }
        return new SimpleTimeZone(offset, tzID);
    }
    return NULL;
}


UBool 
TimeZone::hasSameRules(const TimeZone& other) const
{
    return (getRawOffset() == other.getRawOffset() && 
            useDaylightTime() == other.useDaylightTime());
}

const char*
TimeZone::getTZDataVersion(UErrorCode& status)
{
    /* This is here to prevent race conditions. */
    UBool needsInit;
    UMTX_CHECK(&LOCK, (TZDATA_VERSION[0] == 0), needsInit);
    if (needsInit) {
        int32_t len = sizeof(TZDATA_VERSION);
        UResourceBundle *bundle = ures_openDirect(NULL, "zoneinfo", &status);
        const UChar *tzver = ures_getStringByKey(bundle, "TZVersion",
            &len, &status);

        umtx_lock(&LOCK);
        if (U_SUCCESS(status)) {
            u_UCharsToChars(tzver, TZDATA_VERSION, len+1);
            ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        }
        umtx_unlock(&LOCK);

        ures_close(bundle);
    }
    if (U_FAILURE(status)) {
        return NULL;
    }
    return (const char*)TZDATA_VERSION;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
