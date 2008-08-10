/*
*******************************************************************************
* Copyright (C) 1997-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File DCFMTSYM.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/18/97    clhuang     Implemented with C++ APIs.
*   03/27/97    helena      Updated to pass the simple test after code review.
*   08/26/97    aliu        Added currency/intl currency symbol support.
*   07/20/98    stephen     Slightly modified initialization of monetarySeparator
********************************************************************************
*/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dcfmtsym.h"
#include "unicode/ures.h"
#include "unicode/decimfmt.h"
#include "unicode/ucurr.h"
#include "unicode/choicfmt.h"
#include "ucurrimp.h"
#include "cstring.h"
#include "locbased.h"
#include "uresimp.h"
// *****************************************************************************
// class DecimalFormatSymbols
// *****************************************************************************
 
U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DecimalFormatSymbols)

static const char gNumberElements[] = "NumberElements";

static const UChar INTL_CURRENCY_SYMBOL_STR[] = {0xa4, 0xa4, 0};

// -------------------------------------
// Initializes this with the decimal format symbols in the default locale.
 
DecimalFormatSymbols::DecimalFormatSymbols(UErrorCode& status)
    : UObject(),
    locale()
{
    initialize(locale, status, TRUE);
}
 
// -------------------------------------
// Initializes this with the decimal format symbols in the desired locale.
 
DecimalFormatSymbols::DecimalFormatSymbols(const Locale& loc, UErrorCode& status)
    : UObject(),
    locale(loc)
{
    initialize(locale, status);
}
 
// -------------------------------------

DecimalFormatSymbols::~DecimalFormatSymbols()
{
}

// -------------------------------------
// copy constructor

DecimalFormatSymbols::DecimalFormatSymbols(const DecimalFormatSymbols &source)
    : UObject(source)
{
    *this = source;
}

// -------------------------------------
// assignment operator

DecimalFormatSymbols&
DecimalFormatSymbols::operator=(const DecimalFormatSymbols& rhs)
{
    if (this != &rhs) {
        for(int32_t i = 0; i < (int32_t)kFormatSymbolCount; ++i) {
            // fastCopyFrom is safe, see docs on fSymbols
            fSymbols[(ENumberFormatSymbol)i].fastCopyFrom(rhs.fSymbols[(ENumberFormatSymbol)i]);
        }
        locale = rhs.locale;
        uprv_strcpy(validLocale, rhs.validLocale);
        uprv_strcpy(actualLocale, rhs.actualLocale);
    }
    return *this;
}

// -------------------------------------

UBool
DecimalFormatSymbols::operator==(const DecimalFormatSymbols& that) const
{
    if (this == &that) {
        return TRUE;
    }
    for(int32_t i = 0; i < (int32_t)kFormatSymbolCount; ++i) {
        if(fSymbols[(ENumberFormatSymbol)i] != that.fSymbols[(ENumberFormatSymbol)i]) {
            return FALSE;
        }
    }
    return locale == that.locale &&
        uprv_strcmp(validLocale, that.validLocale) == 0 &&
        uprv_strcmp(actualLocale, that.actualLocale) == 0;
}
 
// -------------------------------------
 
void
DecimalFormatSymbols::initialize(const Locale& loc, UErrorCode& status,
                                 UBool useLastResortData)
{
    *validLocale = *actualLocale = 0;
    currPattern = NULL;
    if (U_FAILURE(status))
        return;
    
    const char* locStr = loc.getName();
    UResourceBundle *resource = ures_open((char *)0, locStr, &status);
    UResourceBundle *numberElementsRes = ures_getByKey(resource, gNumberElements, NULL, &status);
    if (U_FAILURE(status))
    {
        // Initializes with last resort data if necessary.
        if (useLastResortData)
        {
            status = U_USING_FALLBACK_WARNING;
            initialize();
        }
    }
    else {
        // Gets the number element array.
        int32_t numberElementsLength = ures_getSize(numberElementsRes);

        if (numberElementsLength > (int32_t)kFormatSymbolCount) {
            /* Warning: Invalid format. Array too large. */
            numberElementsLength = (int32_t)kFormatSymbolCount;
        }
        // If the array size is too small, something is wrong with the resource
        // bundle, returns the failure error code.
        if (numberElementsLength != 12 || U_FAILURE(status)) {
            status = U_INVALID_FORMAT_ERROR;
        }
        else {
            const UChar *numberElements[kFormatSymbolCount];
            int32_t numberElementsStrLen[kFormatSymbolCount];
            int32_t i = 0;
            for(i = 0; i<numberElementsLength; i++) {
                numberElements[i] = ures_getStringByIndex(numberElementsRes, i, &numberElementsStrLen[i], &status);
            }

            if (U_SUCCESS(status)) {
                initialize(numberElements, numberElementsStrLen, numberElementsLength);

                // Obtain currency data from the currency API.  This is strictly
                // for backward compatibility; we don't use DecimalFormatSymbols
                // for currency data anymore.
                UErrorCode internalStatus = U_ZERO_ERROR; // don't propagate failures out
                UChar curriso[4];
                UnicodeString tempStr;
                ucurr_forLocale(locStr, curriso, 4, &internalStatus);

                // Reuse numberElements[0] as a temporary buffer
                uprv_getStaticCurrencyName(curriso, locStr, tempStr, internalStatus);
                if (U_SUCCESS(internalStatus)) {
                    fSymbols[kIntlCurrencySymbol] = curriso;
                    fSymbols[kCurrencySymbol] = tempStr;
                }
                /* else use the default values. */
            }

            U_LOCALE_BASED(locBased, *this);
            locBased.setLocaleIDs(ures_getLocaleByType(numberElementsRes,
                                       ULOC_VALID_LOCALE, &status),
                                  ures_getLocaleByType(numberElementsRes,
                                       ULOC_ACTUAL_LOCALE, &status));
        }
        //load the currency data
        UChar ucc[4]={0}; //Currency Codes are always 3 chars long 
        int32_t uccLen = 4;
        const char* locName = loc.getName();
        uccLen = ucurr_forLocale(locName, ucc, uccLen, &status);
        if(U_SUCCESS(status) && uccLen > 0) {
            char cc[4]={0};
            u_UCharsToChars(ucc, cc, uccLen);
            /* An explicit currency was requested */
            UErrorCode localStatus = U_ZERO_ERROR;
            UResourceBundle *currency = ures_getByKeyWithFallback(resource, "Currencies", NULL, &localStatus);
            currency = ures_getByKeyWithFallback(currency, cc, currency, &localStatus);
            if(U_SUCCESS(localStatus) && ures_getSize(currency)>2) { // the length is 3 if more data is present
                currency = ures_getByIndex(currency, 2, currency, &localStatus);
                int32_t currPatternLen = 0;
                currPattern = ures_getStringByIndex(currency, (int32_t)0, &currPatternLen, &localStatus);
                UnicodeString decimalSep = ures_getStringByIndex(currency, (int32_t)1, NULL, &localStatus);
                UnicodeString groupingSep = ures_getStringByIndex(currency, (int32_t)2, NULL, &localStatus);
                if(U_SUCCESS(localStatus)){
                    fSymbols[kMonetaryGroupingSeparatorSymbol] = groupingSep;
                    fSymbols[kMonetarySeparatorSymbol] = decimalSep;
                    //pattern.setTo(TRUE, currPattern, currPatternLen);
                    status = localStatus;
                }
            }
            ures_close(currency);
            /* else An explicit currency was requested and is unknown or locale data is malformed. */
            /* ucurr_* API will get the correct value later on. */
        }else{
            // ignore the error if no currency
            status = U_ZERO_ERROR;
        }
    }
    ures_close(resource);
    ures_close(numberElementsRes);
}

// Initializes the DecimalFormatSymbol instance with the data obtained
// from ResourceBundle in the desired locale.

void
DecimalFormatSymbols::initialize(const UChar** numberElements, int32_t *numberElementsStrLen, int32_t numberElementsLength)
{
    static const int32_t TYPE_MAPPING[][2] = {
        {kDecimalSeparatorSymbol, 0},
        {kGroupingSeparatorSymbol, 1},
        {kPatternSeparatorSymbol, 2},
        {kPercentSymbol, 3},
        {kZeroDigitSymbol, 4},
        {kDigitSymbol, 5},
        {kMinusSignSymbol, 6},
        {kExponentialSymbol, 7},
        {kPerMillSymbol, 8},
        {kInfinitySymbol, 9},
        {kNaNSymbol, 10},
        {kPlusSignSymbol, 11},
        {kMonetarySeparatorSymbol, 0}
    };
    int32_t idx;

    for (idx = 0; idx < (int32_t)(sizeof(TYPE_MAPPING)/sizeof(TYPE_MAPPING[0])); idx++) {
        if (TYPE_MAPPING[idx][1] < numberElementsLength) {
            fSymbols[TYPE_MAPPING[idx][0]].setTo(TRUE, numberElements[TYPE_MAPPING[idx][1]], numberElementsStrLen[TYPE_MAPPING[idx][1]]);
        }
    }

    // Default values until it's set later on.
    fSymbols[kCurrencySymbol] = (UChar)0xa4;            // 'OX' currency symbol
    fSymbols[kIntlCurrencySymbol] = INTL_CURRENCY_SYMBOL_STR;
    // TODO: read from locale data, if this makes it into CLDR
    fSymbols[kSignificantDigitSymbol] = (UChar)0x0040;  // '@' significant digit
    fSymbols[kPadEscapeSymbol] = (UChar)0x002a; // TODO: '*' Hard coded for now; get from resource later
    fSymbols[kMonetaryGroupingSeparatorSymbol] = fSymbols[kGroupingSeparatorSymbol];
}

// initialize with default values
void
DecimalFormatSymbols::initialize() {
    /*
     * These strings used to be in static arrays, but the HP/UX aCC compiler
     * cannot initialize a static array with class constructors.
     *  markus 2000may25
     */
    fSymbols[kDecimalSeparatorSymbol] = (UChar)0x2e;    // '.' decimal separator
    fSymbols[kGroupingSeparatorSymbol].remove();        //     group (thousands) separator
    fSymbols[kPatternSeparatorSymbol] = (UChar)0x3b;    // ';' pattern separator
    fSymbols[kPercentSymbol] = (UChar)0x25;             // '%' percent sign
    fSymbols[kZeroDigitSymbol] = (UChar)0x30;           // '0' native 0 digit
    fSymbols[kDigitSymbol] = (UChar)0x23;               // '#' pattern digit
    fSymbols[kPlusSignSymbol] = (UChar)0x002b;          // '+' plus sign
    fSymbols[kMinusSignSymbol] = (UChar)0x2d;           // '-' minus sign
    fSymbols[kCurrencySymbol] = (UChar)0xa4;            // 'OX' currency symbol
    fSymbols[kIntlCurrencySymbol] = INTL_CURRENCY_SYMBOL_STR;
    fSymbols[kMonetarySeparatorSymbol] = (UChar)0x2e;   // '.' monetary decimal separator
    fSymbols[kExponentialSymbol] = (UChar)0x45;         // 'E' exponential
    fSymbols[kPerMillSymbol] = (UChar)0x2030;           // '%o' per mill
    fSymbols[kPadEscapeSymbol] = (UChar)0x2a;           // '*' pad escape symbol
    fSymbols[kInfinitySymbol] = (UChar)0x221e;          // 'oo' infinite
    fSymbols[kNaNSymbol] = (UChar)0xfffd;               // SUB NaN
    fSymbols[kSignificantDigitSymbol] = (UChar)0x0040;  // '@' significant digit
}

Locale 
DecimalFormatSymbols::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
