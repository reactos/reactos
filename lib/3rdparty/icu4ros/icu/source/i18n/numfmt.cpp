/*
*******************************************************************************
* Copyright (C) 1997-2007, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File NUMFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/18/97    clhuang     Implemented with C++ APIs.
*   04/17/97    aliu        Enlarged MAX_INTEGER_DIGITS to fully accomodate the
*                           largest double, by default.
*                           Changed DigitCount to int per code review.
*    07/20/98    stephen        Changed operator== to check for grouping
*                            Changed setMaxIntegerDigits per Java implementation.
*                            Changed setMinIntegerDigits per Java implementation.
*                            Changed setMinFractionDigits per Java implementation.
*                            Changed setMaxFractionDigits per Java implementation.
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/ustring.h"
#include "unicode/ucurr.h"
#include "unicode/curramt.h"
#include "winnmfmt.h"
#include "uresimp.h"
#include "uhash.h"
#include "cmemory.h"
#include "servloc.h"
#include "ucln_in.h"
#include "cstring.h"
#include "putilimp.h"
#include <float.h>

//#define FMT_DEBUG

#ifdef FMT_DEBUG
#include <stdio.h>
static void debugout(UnicodeString s) {
    char buf[2000];
    s.extract((int32_t) 0, s.length(), buf);
    printf("%s", buf);
}
#define debug(x) printf("%s", x);
#else
#define debugout(x)
#define debug(x)
#endif

// If no number pattern can be located for a locale, this is the last
// resort.
static const UChar gLastResortDecimalPat[] = {
    0x23, 0x30, 0x2E, 0x23, 0x23, 0x23, 0x3B, 0x2D, 0x23, 0x30, 0x2E, 0x23, 0x23, 0x23, 0 /* "#0.###;-#0.###" */
};
static const UChar gLastResortCurrencyPat[] = {
    0x24, 0x23, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x28, 0x24, 0x23, 0x30, 0x2E, 0x30, 0x30, 0x29, 0 /* "$#0.00;($#0.00)" */
};
static const UChar gLastResortPercentPat[] = {
    0x23, 0x30, 0x25, 0 /* "#0%" */
};
static const UChar gLastResortScientificPat[] = {
    0x23, 0x45, 0x30, 0 /* "#E0" */
};

// If the maximum base 10 exponent were 4, then the largest number would
// be 99,999 which has 5 digits.
// On IEEE754 systems gMaxIntegerDigits is 308 + possible denormalized 15 digits + rounding digit
static const int32_t gMaxIntegerDigits = DBL_MAX_10_EXP + DBL_DIG + 1;
static const int32_t gMinIntegerDigits = 127;

static const UChar * const gLastResortNumberPatterns[] =
{
    gLastResortDecimalPat,
    gLastResortCurrencyPat,
    gLastResortPercentPat,
    gLastResortScientificPat
};

// *****************************************************************************
// class NumberFormat
// *****************************************************************************

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(NumberFormat)

#if !UCONFIG_NO_SERVICE
// -------------------------------------
// SimpleNumberFormatFactory implementation
NumberFormatFactory::~NumberFormatFactory() {}
SimpleNumberFormatFactory::SimpleNumberFormatFactory(const Locale& locale, UBool visible)
    : _visible(visible)
{
    LocaleUtility::initNameFromLocale(locale, _id);
}

SimpleNumberFormatFactory::~SimpleNumberFormatFactory() {}

UBool SimpleNumberFormatFactory::visible(void) const {
    return _visible;
}

const UnicodeString *
SimpleNumberFormatFactory::getSupportedIDs(int32_t &count, UErrorCode& status) const 
{
    if (U_SUCCESS(status)) {
        count = 1;
        return &_id;
    }
    count = 0;
    return NULL;
}
#endif /* #if !UCONFIG_NO_SERVICE */

// -------------------------------------
// default constructor
NumberFormat::NumberFormat()
:   fGroupingUsed(TRUE),
    fMaxIntegerDigits(gMaxIntegerDigits),
    fMinIntegerDigits(1),
    fMaxFractionDigits(3), // invariant, >= minFractionDigits
    fMinFractionDigits(0),
    fParseIntegerOnly(FALSE)
{
    fCurrency[0] = 0;
}

// -------------------------------------

NumberFormat::~NumberFormat()
{
}

// -------------------------------------
// copy constructor

NumberFormat::NumberFormat(const NumberFormat &source)
:   Format(source)
{
    *this = source;
}

// -------------------------------------
// assignment operator

NumberFormat&
NumberFormat::operator=(const NumberFormat& rhs)
{
    if (this != &rhs)
    {
        fGroupingUsed = rhs.fGroupingUsed;
        fMaxIntegerDigits = rhs.fMaxIntegerDigits;
        fMinIntegerDigits = rhs.fMinIntegerDigits;
        fMaxFractionDigits = rhs.fMaxFractionDigits;
        fMinFractionDigits = rhs.fMinFractionDigits;
        fParseIntegerOnly = rhs.fParseIntegerOnly;
        u_strncpy(fCurrency, rhs.fCurrency, 4);
    }
    return *this;
}

// -------------------------------------

UBool
NumberFormat::operator==(const Format& that) const
{
    // Format::operator== guarantees this cast is safe
    NumberFormat* other = (NumberFormat*)&that;

#ifdef FMT_DEBUG
    // This code makes it easy to determine why two format objects that should
    // be equal aren't.
    UBool first = TRUE;
    if (!Format::operator==(that)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Format::!=");
    }
    if (!(fMaxIntegerDigits == other->fMaxIntegerDigits &&
          fMinIntegerDigits == other->fMinIntegerDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Integer digits !=");
    }
    if (!(fMaxFractionDigits == other->fMaxFractionDigits &&
          fMinFractionDigits == other->fMinFractionDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Fraction digits !=");
    }
    if (!(fGroupingUsed == other->fGroupingUsed)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fGroupingUsed != ");
    }
    if (!(fParseIntegerOnly == other->fParseIntegerOnly)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fParseIntegerOnly != ");
    }
    if (!(u_strcmp(fCurrency, other->fCurrency) == 0)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fCurrency !=");
    }
    if (!first) { printf(" ]"); }    
#endif

    return ((this == &that) ||
            ((Format::operator==(that) &&
              fMaxIntegerDigits == other->fMaxIntegerDigits &&
              fMinIntegerDigits == other->fMinIntegerDigits &&
              fMaxFractionDigits == other->fMaxFractionDigits &&
              fMinFractionDigits == other->fMinFractionDigits &&
              fGroupingUsed == other->fGroupingUsed &&
              fParseIntegerOnly == other->fParseIntegerOnly &&
              u_strcmp(fCurrency, other->fCurrency) == 0)));
}

// -------------------------------------x
// Formats the number object and save the format
// result in the toAppendTo string buffer.

UnicodeString&
NumberFormat::format(const Formattable& obj,
                        UnicodeString& appendTo,
                        FieldPosition& pos,
                        UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    NumberFormat* nonconst = (NumberFormat*) this;
    const Formattable* n = &obj;

    UChar save[4];
    UBool setCurr = FALSE;
    const UObject* o = obj.getObject(); // most commonly o==NULL
    if (o != NULL &&
        o->getDynamicClassID() == CurrencyAmount::getStaticClassID()) {
        // getISOCurrency() returns a pointer to internal storage, so we
        // copy it to retain it across the call to setCurrency().
        const CurrencyAmount* amt = (const CurrencyAmount*) o;
        const UChar* curr = amt->getISOCurrency();
        u_strcpy(save, getCurrency());
        setCurr = (u_strcmp(curr, save) != 0);
        if (setCurr) {
            nonconst->setCurrency(curr, status);
        }
        n = &amt->getNumber();
    }

    switch (n->getType()) {
    case Formattable::kDouble:
        format(n->getDouble(), appendTo, pos);
        break;
    case Formattable::kLong:
        format(n->getLong(), appendTo, pos);
        break;
    case Formattable::kInt64:
        format(n->getInt64(), appendTo, pos);
        break;
    default:
        status = U_INVALID_FORMAT_ERROR;
        break;
    }

    if (setCurr) {
        UErrorCode ok = U_ZERO_ERROR;
        nonconst->setCurrency(save, ok); // always restore currency
    }
    return appendTo;
}

// -------------------------------------

UnicodeString& 
NumberFormat::format(int64_t number,
                     UnicodeString& appendTo,
                     FieldPosition& pos) const
{
    // default so we don't introduce a new abstract method
    return format((int32_t)number, appendTo, pos);
}

// -------------------------------------
// Parses the string and save the result object as well
// as the final parsed position.

void
NumberFormat::parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const
{
    parse(source, result, parse_pos);
}

// -------------------------------------
// Formats a double number and save the result in a string.

UnicodeString&
NumberFormat::format(double number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}

// -------------------------------------
// Formats a long number and save the result in a string.

UnicodeString&
NumberFormat::format(int32_t number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}

// -------------------------------------
// Formats a long number and save the result in a string.

UnicodeString&
NumberFormat::format(int64_t number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}

// -------------------------------------
// Parses the text and save the result object.  If the returned
// parse position is 0, that means the parsing failed, the status
// code needs to be set to failure.  Ignores the returned parse
// position, otherwise.

void
NumberFormat::parse(const UnicodeString& text,
                        Formattable& result,
                        UErrorCode& status) const
{
    if (U_FAILURE(status)) return;

    ParsePosition parsePosition(0);
    parse(text, result, parsePosition);
    if (parsePosition.getIndex() == 0) {
        status = U_INVALID_FORMAT_ERROR;
    }
}

Formattable& NumberFormat::parseCurrency(const UnicodeString& text,
                                         Formattable& result,
                                         ParsePosition& pos) const {
    // Default implementation only -- subclasses should override
    int32_t start = pos.getIndex();
    parse(text, result, pos);
    if (pos.getIndex() != start) {
        UChar curr[4];
        UErrorCode ec = U_ZERO_ERROR;
        getEffectiveCurrency(curr, ec);
        if (U_SUCCESS(ec)) {
            Formattable n(result);
            result.adoptObject(new CurrencyAmount(n, curr, ec));
            if (U_FAILURE(ec)) {
                pos.setIndex(start); // indicate failure
            }
        }
    }
    return result;
}

// -------------------------------------
// Sets to only parse integers.

void
NumberFormat::setParseIntegerOnly(UBool value)
{
    fParseIntegerOnly = value;
}

// -------------------------------------
// Create a number style NumberFormat instance with the default locale.

NumberFormat* U_EXPORT2
NumberFormat::createInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), kNumberStyle, status);
}

// -------------------------------------
// Create a number style NumberFormat instance with the inLocale locale.

NumberFormat* U_EXPORT2
NumberFormat::createInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, kNumberStyle, status);
}

// -------------------------------------
// Create a currency style NumberFormat instance with the default locale.

NumberFormat* U_EXPORT2
NumberFormat::createCurrencyInstance(UErrorCode& status)
{
    return createCurrencyInstance(Locale::getDefault(),  status);
}

// -------------------------------------
// Create a currency style NumberFormat instance with the inLocale locale.

NumberFormat* U_EXPORT2
NumberFormat::createCurrencyInstance(const Locale& inLocale, UErrorCode& status)
{  
    return createInstance(inLocale, kCurrencyStyle, status);
}

// -------------------------------------
// Create a percent style NumberFormat instance with the default locale.

NumberFormat* U_EXPORT2
NumberFormat::createPercentInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), kPercentStyle, status);
}

// -------------------------------------
// Create a percent style NumberFormat instance with the inLocale locale.

NumberFormat* U_EXPORT2
NumberFormat::createPercentInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, kPercentStyle, status);
}

// -------------------------------------
// Create a scientific style NumberFormat instance with the default locale.

NumberFormat* U_EXPORT2
NumberFormat::createScientificInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), kScientificStyle, status);
}

// -------------------------------------
// Create a scientific style NumberFormat instance with the inLocale locale.

NumberFormat* U_EXPORT2
NumberFormat::createScientificInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, kScientificStyle, status);
}

// -------------------------------------

const Locale* U_EXPORT2
NumberFormat::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}

// ------------------------------------------
//
// Registration
//
//-------------------------------------------

#if !UCONFIG_NO_SERVICE
static ICULocaleService* gService = NULL;

/**
 * Release all static memory held by numberformat.  
 */
U_CDECL_BEGIN
static UBool U_CALLCONV numfmt_cleanup(void) {
    if (gService) {
        delete gService;
        gService = NULL;
    }
    return TRUE;
}
U_CDECL_END

// -------------------------------------

class ICUNumberFormatFactory : public ICUResourceBundleFactory {
protected:
    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* /* service */, UErrorCode& status) const {
        // !!! kind is not an EStyles, need to determine how to handle this
        return NumberFormat::makeInstance(loc, (NumberFormat::EStyles)kind, status);
    }
};

// -------------------------------------

class NFFactory : public LocaleKeyFactory {
private:
    NumberFormatFactory* _delegate;
    Hashtable* _ids;

public:
    NFFactory(NumberFormatFactory* delegate) 
        : LocaleKeyFactory(delegate->visible() ? VISIBLE : INVISIBLE)
        , _delegate(delegate)
        , _ids(NULL)
    {
    }

    virtual ~NFFactory()
    {
        delete _delegate;
        delete _ids;
    }

    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const
    {
        if (handlesKey(key, status)) {
            const LocaleKey& lkey = (const LocaleKey&)key;
            Locale loc;
            lkey.canonicalLocale(loc);
            int32_t kind = lkey.kind();

            UObject* result = _delegate->createFormat(loc, (UNumberFormatStyle)(kind+1));
            if (result == NULL) {
                result = service->getKey((ICUServiceKey&)key /* cast away const */, NULL, this, status);
            }
            return result;
        }
        return NULL;
    }

protected:
    /**
     * Return the set of ids that this factory supports (visible or 
     * otherwise).  This can be called often and might need to be
     * cached if it is expensive to create.
     */
    virtual const Hashtable* getSupportedIDs(UErrorCode& status) const
    {
        if (U_SUCCESS(status)) {
            if (!_ids) {
                int32_t count = 0;
                const UnicodeString * const idlist = _delegate->getSupportedIDs(count, status);
                ((NFFactory*)this)->_ids = new Hashtable(status); /* cast away const */
                if (_ids) {
                    for (int i = 0; i < count; ++i) {
                        _ids->put(idlist[i], (void*)this, status);
                    }
                }
            }
            return _ids;
        }
        return NULL;
    }
};

class ICUNumberFormatService : public ICULocaleService {
public:
    ICUNumberFormatService()
        : ICULocaleService(UNICODE_STRING_SIMPLE("Number Format"))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new ICUNumberFormatFactory(), status);
    }

    virtual UObject* cloneInstance(UObject* instance) const {
        return ((NumberFormat*)instance)->clone();
    }

    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* /* actualID */, UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        int32_t kind = lkey.kind();
        Locale loc;
        lkey.currentLocale(loc);
        return NumberFormat::makeInstance(loc, (NumberFormat::EStyles)kind, status);
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

// -------------------------------------

static ICULocaleService* 
getNumberFormatService(void)
{
    UBool needInit;
    UMTX_CHECK(NULL, (UBool)(gService == NULL), needInit);
    if (needInit) {
        ICULocaleService * newservice = new ICUNumberFormatService();
        if (newservice) {
            umtx_lock(NULL);
            if (gService == NULL) {
                gService = newservice;
                newservice = NULL;
            }
            umtx_unlock(NULL);
        }
        if (newservice) {
            delete newservice;
        } else {
            // we won the contention, this thread can register cleanup.
            ucln_i18n_registerCleanup(UCLN_I18N_NUMFMT, numfmt_cleanup);
        }
    }
    return gService;
}

// -------------------------------------

URegistryKey U_EXPORT2
NumberFormat::registerFactory(NumberFormatFactory* toAdopt, UErrorCode& status)
{
  ICULocaleService *service = getNumberFormatService();
  if (service) {
    return service->registerFactory(new NFFactory(toAdopt), status);
  }
  status = U_MEMORY_ALLOCATION_ERROR;
  return NULL;
}

// -------------------------------------

UBool U_EXPORT2
NumberFormat::unregister(URegistryKey key, UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        UBool haveService;
        UMTX_CHECK(NULL, gService != NULL, haveService);
        if (haveService) {
            return gService->unregister(key, status);
        }
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return FALSE;
}

// -------------------------------------
StringEnumeration* U_EXPORT2
NumberFormat::getAvailableLocales(void)
{
  ICULocaleService *service = getNumberFormatService();
  if (service) {
    return service->getAvailableLocales();
  }
  return NULL; // no way to return error condition
}
#endif /* UCONFIG_NO_SERVICE */
// -------------------------------------

NumberFormat* U_EXPORT2
NumberFormat::createInstance(const Locale& loc, EStyles kind, UErrorCode& status)
{
#if !UCONFIG_NO_SERVICE
    UBool haveService;
    UMTX_CHECK(NULL, gService != NULL, haveService);
    if (haveService) {
        return (NumberFormat*)gService->get(loc, kind, status);
    }
    else
#endif
    {
        return makeInstance(loc, kind, status);
    }
}


// -------------------------------------
// Checks if the thousand/10 thousand grouping is used in the
// NumberFormat instance.

UBool
NumberFormat::isGroupingUsed() const
{
    return fGroupingUsed;
}

// -------------------------------------
// Sets to use the thousand/10 thousand grouping in the
// NumberFormat instance.

void
NumberFormat::setGroupingUsed(UBool newValue)
{
    fGroupingUsed = newValue;
}

// -------------------------------------
// Gets the maximum number of digits for the integral part for
// this NumberFormat instance.

int32_t NumberFormat::getMaximumIntegerDigits() const
{
    return fMaxIntegerDigits;
}

// -------------------------------------
// Sets the maximum number of digits for the integral part for
// this NumberFormat instance.

void
NumberFormat::setMaximumIntegerDigits(int32_t newValue)
{
    fMaxIntegerDigits = uprv_max(0, uprv_min(newValue, gMaxIntegerDigits));
    if(fMinIntegerDigits > fMaxIntegerDigits)
        fMinIntegerDigits = fMaxIntegerDigits;
}

// -------------------------------------
// Gets the minimum number of digits for the integral part for
// this NumberFormat instance.

int32_t
NumberFormat::getMinimumIntegerDigits() const
{
    return fMinIntegerDigits;
}

// -------------------------------------
// Sets the minimum number of digits for the integral part for
// this NumberFormat instance.

void
NumberFormat::setMinimumIntegerDigits(int32_t newValue)
{
    fMinIntegerDigits = uprv_max(0, uprv_min(newValue, gMinIntegerDigits));
    if(fMinIntegerDigits > fMaxIntegerDigits)
        fMaxIntegerDigits = fMinIntegerDigits;
}

// -------------------------------------
// Gets the maximum number of digits for the fractional part for
// this NumberFormat instance.

int32_t
NumberFormat::getMaximumFractionDigits() const
{
    return fMaxFractionDigits;
}

// -------------------------------------
// Sets the maximum number of digits for the fractional part for
// this NumberFormat instance.

void
NumberFormat::setMaximumFractionDigits(int32_t newValue)
{
    fMaxFractionDigits = uprv_max(0, uprv_min(newValue, gMaxIntegerDigits));
    if(fMaxFractionDigits < fMinFractionDigits)
        fMinFractionDigits = fMaxFractionDigits;
}

// -------------------------------------
// Gets the minimum number of digits for the fractional part for
// this NumberFormat instance.

int32_t
NumberFormat::getMinimumFractionDigits() const
{
    return fMinFractionDigits;
}

// -------------------------------------
// Sets the minimum number of digits for the fractional part for
// this NumberFormat instance.

void
NumberFormat::setMinimumFractionDigits(int32_t newValue)
{
    fMinFractionDigits = uprv_max(0, uprv_min(newValue, gMinIntegerDigits));
    if (fMaxFractionDigits < fMinFractionDigits)
        fMaxFractionDigits = fMinFractionDigits;
}

// -------------------------------------

void NumberFormat::setCurrency(const UChar* theCurrency, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return;
    }
    if (theCurrency) {
        u_strncpy(fCurrency, theCurrency, 3);
        fCurrency[3] = 0;
    } else {
        fCurrency[0] = 0;
    }
}

const UChar* NumberFormat::getCurrency() const {
    return fCurrency;
}

void NumberFormat::getEffectiveCurrency(UChar* result, UErrorCode& ec) const {
    const UChar* c = getCurrency();
    if (*c != 0) {
        u_strncpy(result, c, 3);
        result[3] = 0;
    } else {
        const char* loc = getLocaleID(ULOC_VALID_LOCALE, ec);
        if (loc == NULL) {
            loc = uloc_getDefault();
        }
        ucurr_forLocale(loc, result, 4, &ec);
    }
}

// -------------------------------------
// Creates the NumberFormat instance of the specified style (number, currency,
// or percent) for the desired locale.

NumberFormat*
NumberFormat::makeInstance(const Locale& desiredLocale,
                           EStyles style,
                           UErrorCode& status)
{
    if (U_FAILURE(status)) return NULL;

    if (style < 0 || style >= kStyleCount) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

#ifdef U_WINDOWS
    char buffer[8];
    int32_t count = desiredLocale.getKeywordValue("compat", buffer, sizeof(buffer), status);

    // if the locale has "@compat=host", create a host-specific NumberFormat
    if (count > 0 && uprv_strcmp(buffer, "host") == 0) {
        Win32NumberFormat *f = NULL;
        UBool curr = TRUE;

        switch (style) {
        case kNumberStyle:
            curr = FALSE;
            // fall-through

        case kCurrencyStyle:
            f = new Win32NumberFormat(desiredLocale, curr, status);

            if (U_SUCCESS(status)) {
                return f;
            }

            delete f;
            break;
            
        default:
            break;
        }
    }
#endif

    NumberFormat* f = NULL;
    DecimalFormatSymbols* symbolsToAdopt = NULL;
    UnicodeString pattern;
    UResourceBundle *resource = ures_open((char *)0, desiredLocale.getName(), &status);
    UResourceBundle *numberPatterns = ures_getByKey(resource, DecimalFormat::fgNumberPatterns, NULL, &status);

    if (U_FAILURE(status)) {
        // We don't appear to have resource data available -- use the last-resort data
        status = U_USING_FALLBACK_WARNING;
        // When the data is unavailable, and locale isn't passed in, last resort data is used.
        symbolsToAdopt = new DecimalFormatSymbols(status);

        // Creates a DecimalFormat instance with the last resort number patterns.
        pattern.setTo(TRUE, gLastResortNumberPatterns[style], -1);
    }
    else {
        // If not all the styled patterns exists for the NumberFormat in this locale,
        // sets the status code to failure and returns nil.
        if (ures_getSize(numberPatterns) < (int32_t)(sizeof(gLastResortNumberPatterns)/sizeof(gLastResortNumberPatterns[0]))) {
            status = U_INVALID_FORMAT_ERROR;
            goto cleanup;
        }

        // Loads the decimal symbols of the desired locale.
        symbolsToAdopt = new DecimalFormatSymbols(desiredLocale, status);

        int32_t patLen = 0;
        const UChar *patResStr = ures_getStringByIndex(numberPatterns, (int32_t)style, &patLen, &status);
        // Creates the specified decimal format style of the desired locale.
        pattern.setTo(TRUE, patResStr, patLen);
    }
    if (U_FAILURE(status) || symbolsToAdopt == NULL) {
        goto cleanup;
    }
    if(style==kCurrencyStyle){
        const UChar* currPattern = symbolsToAdopt->getCurrencyPattern();
        if(currPattern!=NULL){
            pattern.setTo(currPattern, u_strlen(currPattern));
        }
    }
    f = new DecimalFormat(pattern, symbolsToAdopt, status);
    if (U_FAILURE(status) || f == NULL) {
        goto cleanup;
    }

    f->setLocaleIDs(ures_getLocaleByType(numberPatterns, ULOC_VALID_LOCALE, &status),
                    ures_getLocaleByType(numberPatterns, ULOC_ACTUAL_LOCALE, &status));
cleanup:
    ures_close(numberPatterns);
    ures_close(resource);
    if (U_FAILURE(status)) {
        /* If f exists, then it will delete the symbols */
        if (f==NULL) {
            delete symbolsToAdopt;
        }
        else {
            delete f;
        }
        return NULL;
    }
    if (f == NULL || symbolsToAdopt == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        f = NULL;
    }
    return f;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
