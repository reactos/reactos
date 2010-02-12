/*
*******************************************************************************
*   Copyright (C) 1996-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  ucol_res.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* Description:
* This file contains dependencies that the collation run-time doesn't normally
* need. This mainly contains resource bundle usage and collation meta information
*
* Modification history
* Date        Name      Comments
* 1996-1999   various members of ICU team maintained C API for collation framework
* 02/16/2001  synwee    Added internal method getPrevSpecialCE
* 03/01/2001  synwee    Added maxexpansion functionality.
* 03/16/2001  weiv      Collation framework is rewritten in C and made UCA compliant
* 12/08/2004  grhoten   Split part of ucol.cpp into ucol_res.cpp
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION
#include "unicode/uloc.h"
#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/caniter.h"
#include "unicode/ustring.h"

#include "ucol_bld.h"
#include "ucol_imp.h"
#include "ucol_tok.h"
#include "ucol_elm.h"
#include "uresimp.h"
#include "ustr_imp.h"
#include "cstring.h"
#include "umutex.h"
#include "ustrenum.h"
#include "putilimp.h"
#include "utracimp.h"
#include "cmemory.h"

U_NAMESPACE_USE

U_CDECL_BEGIN
static void U_CALLCONV
ucol_prv_closeResources(UCollator *coll) {
    if(coll->rb != NULL) { /* pointing to read-only memory */
        ures_close(coll->rb);
    }
    if(coll->elements != NULL) {
        ures_close(coll->elements);
    }
}
U_CDECL_END

/****************************************************************************/
/* Following are the open/close functions                                   */
/*                                                                          */
/****************************************************************************/
static UCollator*
tryOpeningFromRules(UResourceBundle *collElem, UErrorCode *status) {
    int32_t rulesLen = 0;
    const UChar *rules = ures_getStringByKey(collElem, "Sequence", &rulesLen, status);
    return ucol_openRules(rules, rulesLen, UCOL_DEFAULT, UCOL_DEFAULT, NULL, status);

}


// API in ucol_imp.h

U_CFUNC UCollator*
ucol_open_internal(const char *loc,
                   UErrorCode *status)
{
    const UCollator* UCA = ucol_initUCA(status);

    /* New version */
    if(U_FAILURE(*status)) return 0;



    UCollator *result = NULL;
    UResourceBundle *b = ures_open(U_ICUDATA_COLL, loc, status);

    /* we try to find stuff from keyword */
    UResourceBundle *collations = ures_getByKey(b, "collations", NULL, status);
    UResourceBundle *collElem = NULL;
    char keyBuffer[256];
    // if there is a keyword, we pick it up and try to get elements
    if(!uloc_getKeywordValue(loc, "collation", keyBuffer, 256, status)) {
        // no keyword. we try to find the default setting, which will give us the keyword value
        UErrorCode intStatus = U_ZERO_ERROR;
        // finding default value does not affect collation fallback status
        UResourceBundle *defaultColl = ures_getByKeyWithFallback(collations, "default", NULL, &intStatus);
        if(U_SUCCESS(intStatus)) {
            int32_t defaultKeyLen = 0;
            const UChar *defaultKey = ures_getString(defaultColl, &defaultKeyLen, &intStatus);
            u_UCharsToChars(defaultKey, keyBuffer, defaultKeyLen);
            keyBuffer[defaultKeyLen] = 0;
        } else {
            *status = U_INTERNAL_PROGRAM_ERROR;
            return NULL;
        }
        ures_close(defaultColl);
    }
    collElem = ures_getByKeyWithFallback(collations, keyBuffer, collElem, status);

    UResourceBundle *binary = NULL;

    if(*status == U_MISSING_RESOURCE_ERROR) { /* We didn't find the tailoring data, we fallback to the UCA */
        *status = U_USING_DEFAULT_WARNING;
        result = ucol_initCollator(UCA->image, result, UCA, status);
        // if we use UCA, real locale is root
        result->rb = ures_open(U_ICUDATA_COLL, "", status);
        result->elements = ures_open(U_ICUDATA_COLL, "", status);
        if(U_FAILURE(*status)) {
            goto clean;
        }
        ures_close(b);
        result->hasRealData = FALSE;
    } else if(U_SUCCESS(*status)) {
        int32_t len = 0;
        UErrorCode binaryStatus = U_ZERO_ERROR;

        binary = ures_getByKey(collElem, "%%CollationBin", NULL, &binaryStatus);

        if(binaryStatus == U_MISSING_RESOURCE_ERROR) { /* we didn't find the binary image, we should use the rules */
            binary = NULL;
            result = tryOpeningFromRules(collElem, status);
            if(U_FAILURE(*status)) {
                goto clean;
            }
        } else if(U_SUCCESS(*status)) { /* otherwise, we'll pick a collation data that exists */
            const uint8_t *inData = ures_getBinary(binary, &len, status);
            UCATableHeader *colData = (UCATableHeader *)inData;
            if(uprv_memcmp(colData->UCAVersion, UCA->image->UCAVersion, sizeof(UVersionInfo)) != 0 ||
                uprv_memcmp(colData->UCDVersion, UCA->image->UCDVersion, sizeof(UVersionInfo)) != 0 ||
                colData->version[0] != UCOL_BUILDER_VERSION)
            {
                *status = U_DIFFERENT_UCA_VERSION;
                result = tryOpeningFromRules(collElem, status);
            } else {
                if(U_FAILURE(*status)){
                    goto clean;
                }
                if((uint32_t)len > (paddedsize(sizeof(UCATableHeader)) + paddedsize(sizeof(UColOptionSet)))) {
                    result = ucol_initCollator((const UCATableHeader *)inData, result, UCA, status);
                    if(U_FAILURE(*status)){
                        goto clean;
                    }
                    result->hasRealData = TRUE;
                } else {
                    result = ucol_initCollator(UCA->image, result, UCA, status);
                    ucol_setOptionsFromHeader(result, (UColOptionSet *)(inData+((const UCATableHeader *)inData)->options), status);
                    if(U_FAILURE(*status)){
                        goto clean;
                    }
                    result->hasRealData = FALSE;
                }
                result->freeImageOnClose = FALSE;
            }
        }
        result->rb = b;
        result->elements = collElem;
        len = 0;
        binaryStatus = U_ZERO_ERROR;
        result->rules = ures_getStringByKey(result->elements, "Sequence", &len, &binaryStatus);
        result->rulesLength = len;
        result->freeRulesOnClose = FALSE;
    } else { /* There is another error, and we're just gonna clean up */
        goto clean;
    }

    result->validLocale = NULL; // default is to use rb info

    if(loc == NULL) {
        loc = ures_getLocale(result->rb, status);
    }
    result->requestedLocale = (char *)uprv_malloc((uprv_strlen(loc)+1)*sizeof(char));
    /* test for NULL */
    if (result->requestedLocale == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        goto clean;
    }
    uprv_strcpy(result->requestedLocale, loc);

    ures_close(binary);
    ures_close(collations); //??? we have to decide on that. Probably affects something :)
    result->resCleaner = ucol_prv_closeResources;
    return result;

clean:
    ures_close(b);
    ures_close(collElem);
    ures_close(collations);
    ures_close(binary);
    return NULL;
}

U_CAPI UCollator*
ucol_open(const char *loc,
          UErrorCode *status)
{
    U_NAMESPACE_USE

    UTRACE_ENTRY_OC(UTRACE_UCOL_OPEN);
    UTRACE_DATA1(UTRACE_INFO, "locale = \"%s\"", loc);
    UCollator *result = NULL;

    u_init(status);
#if !UCONFIG_NO_SERVICE
    result = Collator::createUCollator(loc, status);
    if (result == NULL)
#endif
    {
        result = ucol_open_internal(loc, status);
    }
    UTRACE_EXIT_PTR_STATUS(result, *status);
    return result;
}

U_CAPI UCollator* U_EXPORT2
ucol_openRules( const UChar        *rules,
               int32_t            rulesLength,
               UColAttributeValue normalizationMode,
               UCollationStrength strength,
               UParseError        *parseError,
               UErrorCode         *status)
{
    UColTokenParser src;
    UColAttributeValue norm;
    UParseError tErr;

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    u_init(status);
    if (U_FAILURE(*status)) {
        return NULL;
    }

    if(rules == NULL || rulesLength < -1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(rulesLength == -1) {
        rulesLength = u_strlen(rules);
    }

    if(parseError == NULL){
        parseError = &tErr;
    }

    switch(normalizationMode) {
    case UCOL_OFF:
    case UCOL_ON:
    case UCOL_DEFAULT:
        norm = normalizationMode;
        break;
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UCollator *UCA = ucol_initUCA(status);

    if(U_FAILURE(*status)){
        return NULL;
    }

    ucol_tok_initTokenList(&src, rules, rulesLength, UCA, status);
    ucol_tok_assembleTokenList(&src,parseError, status);

    if(U_FAILURE(*status)) {
        /* if status is U_ILLEGAL_ARGUMENT_ERROR, src->current points at the offending option */
        /* if status is U_INVALID_FORMAT_ERROR, src->current points after the problematic part of the rules */
        /* so something might be done here... or on lower level */
#ifdef UCOL_DEBUG
        if(*status == U_ILLEGAL_ARGUMENT_ERROR) {
            fprintf(stderr, "bad option starting at offset %i\n", src.current-src.source);
        } else {
            fprintf(stderr, "invalid rule just before offset %i\n", src.current-src.source);
        }
#endif
        ucol_tok_closeTokenList(&src);
        return NULL;
    }
    UCollator *result = NULL;
    UCATableHeader *table = NULL;

    if(src.resultLen > 0 || src.removeSet != NULL) { /* we have a set of rules, let's make something of it */
        /* also, if we wanted to remove some contractions, we should make a tailoring */
        table = ucol_assembleTailoringTable(&src, status);
        if(U_SUCCESS(*status)) {
            // builder version
            table->version[0] = UCOL_BUILDER_VERSION;
            // no tailoring information on this level
            table->version[1] = table->version[2] = table->version[3] = 0;
            // set UCD version
            u_getUnicodeVersion(table->UCDVersion);
            // set UCA version
            uprv_memcpy(table->UCAVersion, UCA->image->UCAVersion, sizeof(UVersionInfo));
            result = ucol_initCollator(table, 0, UCA, status);
            result->hasRealData = TRUE;
            result->freeImageOnClose = TRUE;
        }
    } else { /* no rules, but no error either */
        // must be only options
        // We will init the collator from UCA
        result = ucol_initCollator(UCA->image, 0, UCA, status);
        // And set only the options
        UColOptionSet *opts = (UColOptionSet *)uprv_malloc(sizeof(UColOptionSet));
        /* test for NULL */
        if (opts == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(opts, src.opts, sizeof(UColOptionSet));
        ucol_setOptionsFromHeader(result, opts, status);
        result->freeOptionsOnClose = TRUE;
        result->hasRealData = FALSE;
        result->freeImageOnClose = FALSE;
    }

    if(U_SUCCESS(*status)) {
        UChar *newRules;
        result->dataVersion[0] = UCOL_BUILDER_VERSION;
        if(rulesLength > 0) {
            newRules = (UChar *)uprv_malloc((rulesLength+1)*U_SIZEOF_UCHAR);
            /* test for NULL */
            if (newRules == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(newRules, rules, rulesLength*U_SIZEOF_UCHAR);
            newRules[rulesLength]=0;
            result->rules = newRules;
            result->rulesLength = rulesLength;
            result->freeRulesOnClose = TRUE;
        }
        result->rb = NULL;
        result->elements = NULL;
        result->validLocale = NULL;
        result->requestedLocale = NULL;
        ucol_setAttribute(result, UCOL_STRENGTH, strength, status);
        ucol_setAttribute(result, UCOL_NORMALIZATION_MODE, norm, status);
    } else {
cleanup:
        if(result != NULL) {
            ucol_close(result);
        } else {
            if(table != NULL) {
                uprv_free(table);
            }
        }
        result = NULL;
    }

    ucol_tok_closeTokenList(&src);

    return result;
}

U_CAPI int32_t U_EXPORT2
ucol_getRulesEx(const UCollator *coll, UColRuleOption delta, UChar *buffer, int32_t bufferLen) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;
    int32_t UCAlen = 0;
    const UChar* ucaRules = 0;
    const UChar *rules = ucol_getRules(coll, &len);
    if(delta == UCOL_FULL_RULES) {
        /* take the UCA rules and append real rules at the end */
        /* UCA rules will be probably coming from the root RB */
        ucaRules = ures_getStringByKey(coll->rb,"UCARules",&UCAlen,&status);
        /*
        UResourceBundle* cresb = ures_getByKeyWithFallback(coll->rb, "collations", NULL, &status);
        UResourceBundle*  uca = ures_getByKeyWithFallback(cresb, "UCA", NULL, &status);
        ucaRules = ures_getStringByKey(uca,"Sequence",&UCAlen,&status);
        ures_close(uca);
        ures_close(cresb);
        */
    }
    if(U_FAILURE(status)) {
        return 0;
    }
    if(buffer!=0 && bufferLen>0){
        *buffer=0;
        if(UCAlen > 0) {
            u_memcpy(buffer, ucaRules, uprv_min(UCAlen, bufferLen));
        }
        if(len > 0 && bufferLen > UCAlen) {
            u_memcpy(buffer+UCAlen, rules, uprv_min(len, bufferLen-UCAlen));
        }
    }
    return u_terminateUChars(buffer, bufferLen, len+UCAlen, &status);
}

static const UChar _NUL = 0;

U_CAPI const UChar* U_EXPORT2
ucol_getRules(    const    UCollator       *coll,
              int32_t            *length)
{
    if(coll->rules != NULL) {
        *length = coll->rulesLength;
        return coll->rules;
    }
    else {
        *length = 0;
        return &_NUL;
    }
}

U_CAPI UBool U_EXPORT2
ucol_equals(const UCollator *source, const UCollator *target) {
    UErrorCode status = U_ZERO_ERROR;
    // if pointers are equal, collators are equal
    if(source == target) {
        return TRUE;
    }
    int32_t i = 0, j = 0;
    // if any of attributes are different, collators are not equal
    for(i = 0; i < UCOL_ATTRIBUTE_COUNT; i++) {
        if(ucol_getAttribute(source, (UColAttribute)i, &status) != ucol_getAttribute(target, (UColAttribute)i, &status) || U_FAILURE(status)) {
            return FALSE;
        }
    }

    int32_t sourceRulesLen = 0, targetRulesLen = 0;
    const UChar *sourceRules = ucol_getRules(source, &sourceRulesLen);
    const UChar *targetRules = ucol_getRules(target, &targetRulesLen);

    if(sourceRulesLen == targetRulesLen && u_strncmp(sourceRules, targetRules, sourceRulesLen) == 0) {
        // all the attributes are equal and the rules are equal - collators are equal
        return(TRUE);
    }
    // hard part, need to construct tree from rules and see if they yield the same tailoring
    UBool result = TRUE;
    UParseError parseError;
    UColTokenParser sourceParser, targetParser;
    int32_t sourceListLen = 0, targetListLen = 0;
    ucol_tok_initTokenList(&sourceParser, sourceRules, sourceRulesLen, source->UCA, &status);
    ucol_tok_initTokenList(&targetParser, targetRules, targetRulesLen, target->UCA, &status);
    sourceListLen = ucol_tok_assembleTokenList(&sourceParser, &parseError, &status);
    targetListLen = ucol_tok_assembleTokenList(&targetParser, &parseError, &status);

    if(sourceListLen != targetListLen) {
        // different number of resets
        result = FALSE;
    } else {
        UColToken *sourceReset = NULL, *targetReset = NULL;
        UChar *sourceResetString = NULL, *targetResetString = NULL;
        int32_t sourceStringLen = 0, targetStringLen = 0;
        for(i = 0; i < sourceListLen; i++) {
            sourceReset = sourceParser.lh[i].reset;
            sourceResetString = sourceParser.source+(sourceReset->source & 0xFFFFFF);
            sourceStringLen = sourceReset->source >> 24;
            for(j = 0; j < sourceListLen; j++) {
                targetReset = targetParser.lh[j].reset;
                targetResetString = targetParser.source+(targetReset->source & 0xFFFFFF);
                targetStringLen = targetReset->source >> 24;
                if(sourceStringLen == targetStringLen && (u_strncmp(sourceResetString, targetResetString, sourceStringLen) == 0)) {
                    sourceReset = sourceParser.lh[i].first;
                    targetReset = targetParser.lh[j].first;
                    while(sourceReset != NULL && targetReset != NULL) {
                        sourceResetString = sourceParser.source+(sourceReset->source & 0xFFFFFF);
                        sourceStringLen = sourceReset->source >> 24;
                        targetResetString = targetParser.source+(targetReset->source & 0xFFFFFF);
                        targetStringLen = targetReset->source >> 24;
                        if(sourceStringLen != targetStringLen || (u_strncmp(sourceResetString, targetResetString, sourceStringLen) != 0)) {
                            result = FALSE;
                            goto returnResult;
                        }
                        // probably also need to check the expansions
                        if(sourceReset->expansion) {
                            if(!targetReset->expansion) {
                                result = FALSE;
                                goto returnResult;
                            } else {
                                // compare expansions
                                sourceResetString = sourceParser.source+(sourceReset->expansion& 0xFFFFFF);
                                sourceStringLen = sourceReset->expansion >> 24;
                                targetResetString = targetParser.source+(targetReset->expansion & 0xFFFFFF);
                                targetStringLen = targetReset->expansion >> 24;
                                if(sourceStringLen != targetStringLen || (u_strncmp(sourceResetString, targetResetString, sourceStringLen) != 0)) {
                                    result = FALSE;
                                    goto returnResult;
                                }
                            }
                        } else {
                            if(targetReset->expansion) {
                                result = FALSE;
                                goto returnResult;
                            }
                        }
                        sourceReset = sourceReset->next;
                        targetReset = targetReset->next;
                    }
                    if(sourceReset != targetReset) { // at least one is not NULL
                        // there are more tailored elements in one list
                        result = FALSE;
                        goto returnResult;
                    }


                    break;
                }
            }
            // couldn't find the reset anchor, so the collators are not equal
            if(j == sourceListLen) {
                result = FALSE;
                goto returnResult;
            }
        }
    }

returnResult:
    ucol_tok_closeTokenList(&sourceParser);
    ucol_tok_closeTokenList(&targetParser);
    return result;

}

U_CAPI int32_t U_EXPORT2
ucol_getDisplayName(    const    char        *objLoc,
                    const    char        *dispLoc,
                    UChar             *result,
                    int32_t         resultLength,
                    UErrorCode        *status)
{
    U_NAMESPACE_USE

    if(U_FAILURE(*status)) return -1;
    UnicodeString dst;
    if(!(result==NULL && resultLength==0)) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer
        dst.setTo(result, 0, resultLength);
    }
    Collator::getDisplayName(Locale(objLoc), Locale(dispLoc), dst);
    return dst.extract(result, resultLength, *status);
}

U_CAPI const char* U_EXPORT2
ucol_getAvailable(int32_t index)
{
    int32_t count = 0;
    const Locale *loc = Collator::getAvailableLocales(count);
    if (loc != NULL && index < count) {
        return loc[index].getName();
    }
    return NULL;
}

U_CAPI int32_t U_EXPORT2
ucol_countAvailable()
{
    int32_t count = 0;
    Collator::getAvailableLocales(count);
    return count;
}

#if !UCONFIG_NO_SERVICE
U_CAPI UEnumeration* U_EXPORT2
ucol_openAvailableLocales(UErrorCode *status) {
    U_NAMESPACE_USE

    // This is a wrapper over Collator::getAvailableLocales()
    if (U_FAILURE(*status)) {
        return NULL;
    }
    StringEnumeration *s = Collator::getAvailableLocales();
    if (s == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return uenum_openStringEnumeration(s, status);
}
#endif

// Note: KEYWORDS[0] != RESOURCE_NAME - alan

static const char* RESOURCE_NAME = "collations";

static const char* KEYWORDS[] = { "collation" };

#define KEYWORD_COUNT (sizeof(KEYWORDS)/sizeof(KEYWORDS[0]))

U_CAPI UEnumeration* U_EXPORT2
ucol_getKeywords(UErrorCode *status) {
    UEnumeration *result = NULL;
    if (U_SUCCESS(*status)) {
        return uenum_openCharStringsEnumeration(KEYWORDS, KEYWORD_COUNT, status);
    }
    return result;
}

U_CAPI UEnumeration* U_EXPORT2
ucol_getKeywordValues(const char *keyword, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    // hard-coded to accept exactly one collation keyword
    // modify if additional collation keyword is added later
    if (keyword==NULL || uprv_strcmp(keyword, KEYWORDS[0])!=0)
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    return ures_getKeywordValues(U_ICUDATA_COLL, RESOURCE_NAME, status);
}

U_CAPI int32_t U_EXPORT2
ucol_getFunctionalEquivalent(char* result, int32_t resultCapacity,
                             const char* keyword, const char* locale,
                             UBool* isAvailable, UErrorCode* status)
{
    // N.B.: Resource name is "collations" but keyword is "collation"
    return ures_getFunctionalEquivalent(result, resultCapacity, U_ICUDATA_COLL,
        "collations", keyword, locale,
        isAvailable, TRUE, status);
}

/* returns the locale name the collation data comes from */
U_CAPI const char * U_EXPORT2
ucol_getLocale(const UCollator *coll, ULocDataLocaleType type, UErrorCode *status) {
    return ucol_getLocaleByType(coll, type, status);
}

U_CAPI const char * U_EXPORT2
ucol_getLocaleByType(const UCollator *coll, ULocDataLocaleType type, UErrorCode *status) {
    const char *result = NULL;
    if(status == NULL || U_FAILURE(*status)) {
        return NULL;
    }
    UTRACE_ENTRY(UTRACE_UCOL_GETLOCALE);
    UTRACE_DATA1(UTRACE_INFO, "coll=%p", coll);

    switch(type) {
  case ULOC_ACTUAL_LOCALE:
      // validLocale is set only if service registration has explicitly set the
      // requested and valid locales.  if this is the case, the actual locale
      // is considered to be the valid locale.
      if (coll->validLocale != NULL) {
          result = coll->validLocale;
      } else if(coll->elements != NULL) {
          result = ures_getLocale(coll->elements, status);
      }
      break;
  case ULOC_VALID_LOCALE:
      if (coll->validLocale != NULL) {
          result = coll->validLocale;
      } else if(coll->rb != NULL) {
          result = ures_getLocale(coll->rb, status);
      }
      break;
  case ULOC_REQUESTED_LOCALE:
      result = coll->requestedLocale;
      break;
  default:
      *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    UTRACE_DATA1(UTRACE_INFO, "result = %s", result);
    UTRACE_EXIT_STATUS(*status);
    return result;
}

U_CFUNC void U_EXPORT2
ucol_setReqValidLocales(UCollator *coll, char *requestedLocaleToAdopt, char *validLocaleToAdopt)
{
    if (coll) {
        if (coll->validLocale) {
            uprv_free(coll->validLocale);
        }
        coll->validLocale = validLocaleToAdopt;
        if (coll->requestedLocale) { // should always have
            uprv_free(coll->requestedLocale);
        }
        coll->requestedLocale = requestedLocaleToAdopt;
    }
}

U_CAPI USet * U_EXPORT2
ucol_getTailoredSet(const UCollator *coll, UErrorCode *status)
{
    U_NAMESPACE_USE

    if(status == NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(coll == NULL || coll->UCA == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    UParseError parseError;
    UColTokenParser src;
    int32_t rulesLen = 0;
    const UChar *rules = ucol_getRules(coll, &rulesLen);
    UBool startOfRules = TRUE;
    // we internally use the C++ class, for the following reasons:
    // 1. we need to utilize canonical iterator, which is a C++ only class
    // 2. canonical iterator returns UnicodeStrings - USet cannot take them
    // 3. USet is internally really UnicodeSet, C is just a wrapper
    UnicodeSet *tailored = new UnicodeSet();
    UnicodeString pattern;
    UnicodeString empty;
    CanonicalIterator it(empty, *status);


    // The idea is to tokenize the rule set. For each non-reset token,
    // we add all the canonicaly equivalent FCD sequences
    ucol_tok_initTokenList(&src, rules, rulesLen, coll->UCA, status);
    while (ucol_tok_parseNextToken(&src, startOfRules, &parseError, status) != NULL) {
        startOfRules = FALSE;
        if(src.parsedToken.strength != UCOL_TOK_RESET) {
            const UChar *stuff = src.source+(src.parsedToken.charsOffset);
            it.setSource(UnicodeString(stuff, src.parsedToken.charsLen), *status);
            pattern = it.next();
            while(!pattern.isBogus()) {
                if(Normalizer::quickCheck(pattern, UNORM_FCD, *status) != UNORM_NO) {
                    tailored->add(pattern);
                }
                pattern = it.next();
            }
        }
    }
    ucol_tok_closeTokenList(&src);
    return (USet *)tailored;
}

#endif /* #if !UCONFIG_NO_COLLATION */
