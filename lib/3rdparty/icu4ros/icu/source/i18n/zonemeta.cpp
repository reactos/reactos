/*
*******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "zonemeta.h"

#include "unicode/timezone.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"

#include "umutex.h"
#include "uvector.h"
#include "cmemory.h"
#include "gregoimp.h"
#include "cstring.h"
#include "ucln_in.h"

static UBool gZoneMetaInitialized = FALSE;

// Metazone mapping tables
static UMTX gZoneMetaLock = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gCanonicalMap = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gOlsonToMeta = NULL;
static U_NAMESPACE_QUALIFIER Hashtable *gMetaToOlson = NULL;

U_CDECL_BEGIN
/**
 * Cleanup callback func
 */
static UBool U_CALLCONV zoneMeta_cleanup(void)
{
     umtx_destroy(&gZoneMetaLock);

    if (gCanonicalMap != NULL) {
        delete gCanonicalMap;
        gCanonicalMap = NULL;
    }

    if (gOlsonToMeta != NULL) {
        delete gOlsonToMeta;
        gOlsonToMeta = NULL;
    }

    if (gMetaToOlson != NULL) {
        delete gMetaToOlson;
        gMetaToOlson = NULL;
    }

    gZoneMetaInitialized = FALSE;

    return TRUE;
}

/**
 * Deleter for UVector
 */
static void U_CALLCONV
deleteUVector(void *obj) {
   delete (U_NAMESPACE_QUALIFIER UVector*) obj;
}

/**
 * Deleter for CanonicalMapEntry
 */
static void U_CALLCONV
deleteCanonicalMapEntry(void *obj) {
    U_NAMESPACE_QUALIFIER CanonicalMapEntry *entry = (U_NAMESPACE_QUALIFIER CanonicalMapEntry*)obj;
    uprv_free(entry->id);
    uprv_free(entry);
}

/**
 * Deleter for OlsonToMetaMappingEntry
 */
static void U_CALLCONV
deleteOlsonToMetaMappingEntry(void *obj) {
    U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry *entry = (U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry*)obj;
    uprv_free(entry);
}

/**
 * Deleter for MetaToOlsonMappingEntry
 */
static void U_CALLCONV
deleteMetaToOlsonMappingEntry(void *obj) {
    U_NAMESPACE_QUALIFIER MetaToOlsonMappingEntry *entry = (U_NAMESPACE_QUALIFIER MetaToOlsonMappingEntry*)obj;
    uprv_free(entry->territory);
    uprv_free(entry);
}
U_CDECL_END

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX 128
static const char gZoneStringsTag[]     = "zoneStrings";
static const char gUseMetazoneTag[]     = "um";

static const char gSupplementalData[]   = "supplementalData";
static const char gMapTimezonesTag[]    = "mapTimezones";
static const char gMetazonesTag[]       = "metazones";
static const char gZoneFormattingTag[]  = "zoneFormatting";
static const char gTerritoryTag[]       = "territory";
static const char gAliasesTag[]         = "aliases";
static const char gMultizoneTag[]       = "multizone";

static const char gMetazoneInfo[]       = "metazoneInfo";
static const char gMetazoneMappings[]   = "metazoneMappings";

#define MZID_PREFIX_LEN 5
static const char gMetazoneIdPrefix[]   = "meta:";

static const UChar gWorld[] = {0x30, 0x30, 0x31, 0x00}; // "001"

#define ASCII_DIGIT(c) (((c)>=0x30 && (c)<=0x39) ? (c)-0x30 : -1)

/*
 * Convert a date string used by metazone mappings to UDate.
 * The format used by CLDR metazone mapping is "yyyy-MM-dd HH:mm".
 */
 static UDate parseDate (const UChar *text, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    int32_t len = u_strlen(text);
    if (len != 16 && len != 10) {
        // It must be yyyy-MM-dd HH:mm (length 16) or yyyy-MM-dd (length 10)
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }

    int32_t year = 0, month = 0, day = 0, hour = 0, min = 0, n;
    int32_t idx;

    // "yyyy" (0 - 3)
    for (idx = 0; idx <= 3 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            year = 10*year + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "MM" (5 - 6)
    for (idx = 5; idx <= 6 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            month = 10*month + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "dd" (8 - 9)
    for (idx = 8; idx <= 9 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT(text[idx]);
        if (n >= 0) {
            day = 10*day + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    if (len == 16) {
        // "HH" (11 - 12)
        for (idx = 11; idx <= 12 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT(text[idx]);
            if (n >= 0) {
                hour = 10*hour + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
        // "mm" (14 - 15)
        for (idx = 14; idx <= 15 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT(text[idx]);
            if (n >= 0) {
                min = 10*min + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
    }

    if (U_SUCCESS(status)) {
        UDate date = Grego::fieldsToDay(year, month - 1, day) * U_MILLIS_PER_DAY
            + hour * U_MILLIS_PER_HOUR + min * U_MILLIS_PER_MINUTE;
        return date;
    }
    return 0;
}

 /*
 * Initialize global objects
 */
void
ZoneMeta::initialize(void) {
    UBool initialized;
    UMTX_CHECK(&gZoneMetaLock, gZoneMetaInitialized, initialized);
    if (initialized) {
        return;
    }

    // Initialize hash tables
    Hashtable *tmpCanonicalMap = createCanonicalMap();
    Hashtable *tmpOlsonToMeta = createOlsonToMetaMap();
    if (tmpOlsonToMeta == NULL) {
        // With ICU 3.8 data
        tmpOlsonToMeta = createOlsonToMetaMapOld();
    }
    Hashtable *tmpMetaToOlson = createMetaToOlsonMap();

    umtx_lock(&gZoneMetaLock);
    if (gZoneMetaInitialized) {
        // Another thread already created mappings
        delete tmpCanonicalMap;
        delete tmpOlsonToMeta;
        delete tmpMetaToOlson;
    } else {
        gZoneMetaInitialized = TRUE;
        gCanonicalMap = tmpCanonicalMap;
        gOlsonToMeta = tmpOlsonToMeta;
        gMetaToOlson = tmpMetaToOlson;
        ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
    }
    umtx_unlock(&gZoneMetaLock);
}

Hashtable*
ZoneMeta::createCanonicalMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *canonicalMap = NULL;
    UResourceBundle *zoneFormatting = NULL;
    UResourceBundle *tzitem = NULL;
    UResourceBundle *aliases = NULL;

    canonicalMap = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    canonicalMap->setValueDeleter(deleteCanonicalMapEntry);

    zoneFormatting = ures_openDirect(NULL, gSupplementalData, &status);
    zoneFormatting = ures_getByKey(zoneFormatting, gZoneFormattingTag, zoneFormatting, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    while (ures_hasNext(zoneFormatting)) {
        tzitem = ures_getNextResource(zoneFormatting, tzitem, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            continue;
        }
        if (ures_getType(tzitem) != URES_TABLE) {
            continue;
        }

        int32_t territoryLen;
        const UChar *territory = ures_getStringByKey(tzitem, gTerritoryTag, &territoryLen, &status);
        if (U_FAILURE(status)) {
            territory = NULL;
            status = U_ZERO_ERROR;
        }

        int32_t tzidLen = 0;
        char tzid[ZID_KEY_MAX];
        const char *tzkey = ures_getKey(tzitem);
        uprv_strcpy(tzid, tzkey);
        // Replace ':' with '/'
        char *p = tzid;
        while (*p) {
            if (*p == ':') {
                *p = '/';
            }
            p++;
            tzidLen++;
        }

        // Create canonical map entry
        CanonicalMapEntry *entry = (CanonicalMapEntry*)uprv_malloc(sizeof(CanonicalMapEntry));
        if (entry == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto error_cleanup;
        }
        entry->id = (UChar*)uprv_malloc((tzidLen + 1) * sizeof(UChar));
        if (entry->id == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(entry);
            goto error_cleanup;
        }
        u_charsToUChars(tzid, entry->id, tzidLen + 1);

        if (territory == NULL || u_strcmp(territory, gWorld) == 0) {
            entry->country = NULL;
        } else {
            entry->country = territory;
        }

        // Put this entry to the table
        canonicalMap->put(UnicodeString(entry->id), entry, status);
        if (U_FAILURE(status)) {
            deleteCanonicalMapEntry(entry);
            goto error_cleanup;
        }

        // Get aliases
        aliases = ures_getByKey(tzitem, gAliasesTag, aliases, &status);
        if (U_FAILURE(status)) {
            // No aliases
            status = U_ZERO_ERROR;
            continue;
        }

        while (ures_hasNext(aliases)) {
            const UChar* alias = ures_getNextString(aliases, NULL, NULL, &status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }
            // Create canonical map entry for this alias
            entry = (CanonicalMapEntry*)uprv_malloc(sizeof(CanonicalMapEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto error_cleanup;
            }
            entry->id = (UChar*)uprv_malloc((tzidLen + 1) * sizeof(UChar));
            if (entry->id == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                uprv_free(entry);
                goto error_cleanup;
            }
            u_charsToUChars(tzid, entry->id, tzidLen + 1);

            if (territory  == NULL || u_strcmp(territory, gWorld) == 0) {
                entry->country = NULL;
            } else {
                entry->country = territory;
            }
            canonicalMap->put(UnicodeString(alias), entry, status);
            if (U_FAILURE(status)) {
                deleteCanonicalMapEntry(entry);
                goto error_cleanup;
            }
        }
    }

normal_cleanup:
    ures_close(aliases);
    ures_close(tzitem);
    ures_close(zoneFormatting);
    return canonicalMap;

error_cleanup:
    delete canonicalMap;
    canonicalMap = NULL;

    goto normal_cleanup;
}

/*
 * Creating Olson tzid to metazone mappings from resource (3.8.1 and beyond)
 */
Hashtable*
ZoneMeta::createOlsonToMetaMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *olsonToMeta = NULL;
    UResourceBundle *metazoneMappings = NULL;
    UResourceBundle *zoneItem = NULL;
    UResourceBundle *mz = NULL;
    StringEnumeration *tzids = NULL;

    olsonToMeta = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    olsonToMeta->setValueDeleter(deleteUVector);

    // Read metazone mappings from metazoneInfo bundle
    metazoneMappings = ures_openDirect(NULL, gMetazoneInfo, &status);
    metazoneMappings = ures_getByKey(metazoneMappings, gMetazoneMappings, metazoneMappings, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    // Walk through all canonical tzids
    char zidkey[ZID_KEY_MAX];

    tzids = TimeZone::createEnumeration();
    const UnicodeString *tzid;
    while ((tzid = tzids->snext(status))) {
        if (U_FAILURE(status)) {
            goto error_cleanup;
        }
        // We may skip aliases, because the bundle
        // contains only canonical IDs.  For now, try
        // all of them.
        tzid->extract(0, tzid->length(), zidkey, sizeof(zidkey), US_INV);
        zidkey[sizeof(zidkey)-1] = 0; // NULL terminate just in case.

        // Replace '/' with ':'
        UBool foundSep = FALSE;
        char *p = zidkey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
                foundSep = TRUE;
            }
            p++;
        }
        if (!foundSep) {
            // A valid time zone key has at least one separator
            continue;
        }

        zoneItem = ures_getByKey(metazoneMappings, zidkey, zoneItem, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            continue;
        }

        UVector *mzMappings = NULL;
        while (ures_hasNext(zoneItem)) {
            mz = ures_getNextResource(zoneItem, mz, &status);
            const UChar *mz_name = ures_getStringByIndex(mz, 0, NULL, &status);
            const UChar *mz_from = ures_getStringByIndex(mz, 1, NULL, &status);
            const UChar *mz_to   = ures_getStringByIndex(mz, 2, NULL, &status);

            if(U_FAILURE(status)){
                status = U_ZERO_ERROR;
                continue;
            }
            // We do not want to use SimpleDateformat to parse boundary dates,
            // because this code could be triggered by the initialization code
            // used by SimpleDateFormat.
            UDate from = parseDate(mz_from, status);
            UDate to = parseDate(mz_to, status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }

            OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            entry->mzid = mz_name;
            entry->from = from;
            entry->to = to;

            if (mzMappings == NULL) {
                mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                if (U_FAILURE(status)) {
                    delete mzMappings;
                    deleteOlsonToMetaMappingEntry(entry);
                    uprv_free(entry);
                    break;
                }
            }

            mzMappings->addElement(entry, status);
            if (U_FAILURE(status)) {
                break;
            }
        }

        if (U_FAILURE(status)) {
            if (mzMappings != NULL) {
                delete mzMappings;
            }
            goto error_cleanup;
        }
        if (mzMappings != NULL) {
            olsonToMeta->put(*tzid, mzMappings, status);
            if (U_FAILURE(status)) {
                delete mzMappings;
                goto error_cleanup;
            }
        }
    }

normal_cleanup:
    if (tzids != NULL) {
        delete tzids;
    }
    ures_close(zoneItem);
    ures_close(mz);
    ures_close(metazoneMappings);
    return olsonToMeta;

error_cleanup:
    if (olsonToMeta != NULL) {
        delete olsonToMeta;
        olsonToMeta = NULL;
    }
    goto normal_cleanup;
}

/*
 * Creating Olson tzid to metazone mappings from ICU resource (3.8)
 */
Hashtable*
ZoneMeta::createOlsonToMetaMapOld(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *olsonToMeta = NULL;
    UResourceBundle *zoneStringsArray = NULL;
    UResourceBundle *mz = NULL;
    UResourceBundle *zoneItem = NULL;
    UResourceBundle *useMZ = NULL;
    StringEnumeration *tzids = NULL;

    olsonToMeta = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    olsonToMeta->setValueDeleter(deleteUVector);

    // Read metazone mappings from root bundle
    zoneStringsArray = ures_openDirect(NULL, "", &status);
    zoneStringsArray = ures_getByKey(zoneStringsArray, gZoneStringsTag, zoneStringsArray, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    // Walk through all canonical tzids
    char zidkey[ZID_KEY_MAX];

    tzids = TimeZone::createEnumeration();
    const UnicodeString *tzid;
    while ((tzid = tzids->snext(status))) {
        if (U_FAILURE(status)) {
            goto error_cleanup;
        }
        // We may skip aliases, because the bundle
        // contains only canonical IDs.  For now, try
        // all of them.
        tzid->extract(0, tzid->length(), zidkey, sizeof(zidkey), US_INV);
        zidkey[sizeof(zidkey)-1] = 0; // NULL terminate just in case.

        // Replace '/' with ':'
        UBool foundSep = FALSE;
        char *p = zidkey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
                foundSep = TRUE;
            }
            p++;
        }
        if (!foundSep) {
            // A valid time zone key has at least one separator
            continue;
        }

        zoneItem = ures_getByKey(zoneStringsArray, zidkey, zoneItem, &status);
        useMZ = ures_getByKey(zoneItem, gUseMetazoneTag, useMZ, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            continue;
        }

        UVector *mzMappings = NULL;
        while (ures_hasNext(useMZ)) {
            mz = ures_getNextResource(useMZ, mz, &status);
            const UChar *mz_name = ures_getStringByIndex(mz, 0, NULL, &status);
            const UChar *mz_from = ures_getStringByIndex(mz, 1, NULL, &status);
            const UChar *mz_to   = ures_getStringByIndex(mz, 2, NULL, &status);

            if(U_FAILURE(status)){
                status = U_ZERO_ERROR;
                continue;
            }
            // We do not want to use SimpleDateformat to parse boundary dates,
            // because this code could be triggered by the initialization code
            // used by SimpleDateFormat.
            UDate from = parseDate(mz_from, status);
            UDate to = parseDate(mz_to, status);
            if (U_FAILURE(status)) {
                status = U_ZERO_ERROR;
                continue;
            }

            OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
            if (entry == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            entry->mzid = mz_name;
            entry->from = from;
            entry->to = to;

            if (mzMappings == NULL) {
                mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                if (U_FAILURE(status)) {
                    delete mzMappings;
                    deleteOlsonToMetaMappingEntry(entry);
                    uprv_free(entry);
                    break;
                }
            }

            mzMappings->addElement(entry, status);
            if (U_FAILURE(status)) {
                break;
            }
        }

        if (U_FAILURE(status)) {
            if (mzMappings != NULL) {
                delete mzMappings;
            }
            goto error_cleanup;
        }
        if (mzMappings != NULL) {
            olsonToMeta->put(*tzid, mzMappings, status);
            if (U_FAILURE(status)) {
                delete mzMappings;
                goto error_cleanup;
            }
        }
    }

normal_cleanup:
    if (tzids != NULL) {
        delete tzids;
    }
    ures_close(zoneItem);
    ures_close(useMZ);
    ures_close(mz);
    ures_close(zoneStringsArray);
    return olsonToMeta;

error_cleanup:
    if (olsonToMeta != NULL) {
        delete olsonToMeta;
    }
    goto normal_cleanup;
}

Hashtable*
ZoneMeta::createMetaToOlsonMap(void) {
    UErrorCode status = U_ZERO_ERROR;

    Hashtable *metaToOlson = NULL;
    UResourceBundle *metazones = NULL;
    UResourceBundle *mz = NULL;

    metaToOlson = new Hashtable(uhash_compareUnicodeString, NULL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    metaToOlson->setValueDeleter(deleteUVector);

    metazones = ures_openDirect(NULL, gSupplementalData, &status);
    metazones = ures_getByKey(metazones, gMapTimezonesTag, metazones, &status);
    metazones = ures_getByKey(metazones, gMetazonesTag, metazones, &status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }

    while (ures_hasNext(metazones)) {
        mz = ures_getNextResource(metazones, mz, &status);
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            continue;
        }
        const char *mzkey = ures_getKey(mz);
        if (uprv_strncmp(mzkey, gMetazoneIdPrefix, MZID_PREFIX_LEN) == 0) {
            const char *mzid = mzkey + MZID_PREFIX_LEN;
            const char *territory = uprv_strrchr(mzid, '_');
            int32_t mzidLen = 0;
            int32_t territoryLen = 0;
            if (territory) {
                mzidLen = territory - mzid;
                territory++;
                territoryLen = uprv_strlen(territory);
            }
            if (mzidLen > 0 && territoryLen > 0) {
                int32_t tzidLen;
                const UChar *tzid = ures_getStringByIndex(mz, 0, &tzidLen, &status);
                if (U_SUCCESS(status)) {
                    // Create MetaToOlsonMappingEntry
                    MetaToOlsonMappingEntry *entry = (MetaToOlsonMappingEntry*)uprv_malloc(sizeof(MetaToOlsonMappingEntry));
                    if (entry == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        goto error_cleanup;
                    }
                    entry->id = tzid;
                    entry->territory = (UChar*)uprv_malloc((territoryLen + 1) * sizeof(UChar));
                    if (entry->territory == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        uprv_free(entry);
                        goto error_cleanup;
                    }
                    u_charsToUChars(territory, entry->territory, territoryLen + 1);

                    // Check if mapping entries for metazone is already available
                    UnicodeString mzidStr(mzid, mzidLen, US_INV);
                    UVector *tzMappings = (UVector*)metaToOlson->get(mzidStr);
                    if (tzMappings == NULL) {
                        // Create new UVector and put it into the hashtable
                        tzMappings = new UVector(deleteMetaToOlsonMappingEntry, NULL, status);
                        metaToOlson->put(mzidStr, tzMappings, status);
                        if (U_FAILURE(status)) {
                            if (tzMappings != NULL) {
                                delete tzMappings;
                            }
                            deleteMetaToOlsonMappingEntry(entry);
                            goto error_cleanup;
                        }
                    }
                    tzMappings->addElement(entry, status);
                    if (U_FAILURE(status)) {
                        goto error_cleanup;
                    }
                } else {
                    status = U_ZERO_ERROR;
                }
            }
        }
    }

normal_cleanup:
    ures_close(mz);
    ures_close(metazones);
    return metaToOlson;

error_cleanup:
    if (metaToOlson != NULL) {
        delete metaToOlson;
    }
    goto normal_cleanup;
}

UnicodeString&
ZoneMeta::getCanonicalID(const UnicodeString &tzid, UnicodeString &canonicalID) {
    const CanonicalMapEntry *entry = getCanonicalInfo(tzid);
    if (entry != NULL) {
        canonicalID.setTo(entry->id);
    } else {
        // Use the input tzid
        canonicalID.setTo(tzid);
    }
    return canonicalID;
}

UnicodeString&
ZoneMeta::getCanonicalCountry(const UnicodeString &tzid, UnicodeString &canonicalCountry) {
    const CanonicalMapEntry *entry = getCanonicalInfo(tzid);
    if (entry != NULL && entry->country != NULL) {
        canonicalCountry.setTo(entry->country);
    } else {
        // Use the input tzid
        canonicalCountry.remove();
    }
    return canonicalCountry;
}

const CanonicalMapEntry*
ZoneMeta::getCanonicalInfo(const UnicodeString &tzid) {
    initialize();
    CanonicalMapEntry *entry = NULL;
    UnicodeString canonicalOlsonId;
    TimeZone::getOlsonCanonicalID(tzid, canonicalOlsonId);
    if (!canonicalOlsonId.isEmpty()) {
        if (gCanonicalMap != NULL) {
            entry = (CanonicalMapEntry*)gCanonicalMap->get(tzid);
        }
    }
    return entry;
}

UnicodeString&
ZoneMeta::getSingleCountry(const UnicodeString &tzid, UnicodeString &country) {
    UErrorCode status = U_ZERO_ERROR;

    // Get canonical country for the zone
    getCanonicalCountry(tzid, country);

    if (!country.isEmpty()) { 
        UResourceBundle *supplementalDataBundle = ures_openDirect(NULL, gSupplementalData, &status);
        UResourceBundle *zoneFormatting = ures_getByKey(supplementalDataBundle, gZoneFormattingTag, NULL, &status);
        UResourceBundle *multizone = ures_getByKey(zoneFormatting, gMultizoneTag, NULL, &status);

        if (U_SUCCESS(status)) {
            while (ures_hasNext(multizone)) {
                int32_t len;
                const UChar* multizoneCountry = ures_getNextString(multizone, &len, NULL, &status);
                if (country.compare(multizoneCountry, len) == 0) {
                    // Included in the multizone country list
                    country.remove();
                    break;
                }
            }
        }

        ures_close(multizone);
        ures_close(zoneFormatting);
        ures_close(supplementalDataBundle);
    }

    return country;
}

UnicodeString&
ZoneMeta::getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result) {
    UBool isSet = FALSE;
    const UVector *mappings = getMetazoneMappings(tzid);
    if (mappings != NULL) {
        for (int32_t i = 0; i < mappings->size(); i++) {
            OlsonToMetaMappingEntry *mzm = (OlsonToMetaMappingEntry*)mappings->elementAt(i);
            if (mzm->from <= date && mzm->to > date) {
                result.setTo(mzm->mzid, -1);
                isSet = TRUE;
                break;
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}

const UVector*
ZoneMeta::getMetazoneMappings(const UnicodeString &tzid) {
    initialize();
    const UVector *result = NULL;
    if (gOlsonToMeta != NULL) {
        result = (UVector*)gOlsonToMeta->get(tzid);
    }
    return result;
}

UnicodeString&
ZoneMeta::getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result) {
    initialize();
    UBool isSet = FALSE;
    if (gMetaToOlson != NULL) {
        UVector *mappings = (UVector*)gMetaToOlson->get(mzid);
        if (mappings != NULL) {
            // Find a preferred time zone for the given region.
            for (int32_t i = 0; i < mappings->size(); i++) {
                MetaToOlsonMappingEntry *olsonmap = (MetaToOlsonMappingEntry*)mappings->elementAt(i);
                if (region.compare(olsonmap->territory, -1) == 0) {
                    result.setTo(olsonmap->id);
                    isSet = TRUE;
                    break;
                } else if (u_strcmp(olsonmap->territory, gWorld) == 0) {
                    result.setTo(olsonmap->id);
                    isSet = TRUE;
                }
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
