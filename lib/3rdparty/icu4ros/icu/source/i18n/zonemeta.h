/*
*******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef ZONEMETA_H
#define ZONEMETA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "hash.h"

U_NAMESPACE_BEGIN

typedef struct CanonicalMapEntry {
    UChar *id;
    const UChar *country; // const because it's a reference to a resource bundle string.
} CanonicalMapEntry;

typedef struct OlsonToMetaMappingEntry {
    const UChar *mzid; // const because it's a reference to a resource bundle string.
    UDate from;
    UDate to;
} OlsonToMetaMappingEntry;

typedef struct MetaToOlsonMappingEntry {
    const UChar *id; // const because it's a reference to a resource bundle string.
    UChar *territory;
} MetaToOlsonMappingEntry;

class UVector;

class U_I18N_API ZoneMeta {
public:
    /**
     * Return the canonical id for this tzid, which might be the id itself.
     * If there is no canonical id for it, return the passed-in id.
     */
    static UnicodeString& getCanonicalID(const UnicodeString &tzid, UnicodeString &canonicalID);

    /**
     * Return the canonical country code for this tzid.  If we have none, or if the time zone
     * is not associated with a country, return null.
     */
    static UnicodeString& getCanonicalCountry(const UnicodeString &tzid, UnicodeString &canonicalCountry);

    /**
     * Return the country code if this is a 'single' time zone that can fallback to just
     * the country, otherwise return empty string.  (Note, one must also check the locale data
     * to see that there is a localization for the country in order to implement
     * tr#35 appendix J step 5.)
     */
    static UnicodeString& getSingleCountry(const UnicodeString &tzid, UnicodeString &country);

    /**
     * Returns a CLDR metazone ID for the given Olson tzid and time.
     */
    static UnicodeString& getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result);
    /**
     * Returns an Olson ID for the ginve metazone and region
     */
    static UnicodeString& getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result);

    static const UVector* getMetazoneMappings(const UnicodeString &tzid);

private:
    static void initialize(void);

    static const CanonicalMapEntry* getCanonicalInfo(const UnicodeString &tzid);

    static Hashtable* createCanonicalMap(void);
    static Hashtable* createOlsonToMetaMapOld(void);
    static Hashtable* createOlsonToMetaMap(void);
    static Hashtable* createMetaToOlsonMap(void);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
#endif // ZONEMETA_H
