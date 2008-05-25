/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/numfmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "unicode/uclean.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

extern "C" void capi();
void cppapi();

static void
showCurrencyFormatting(UBool useICU26API);

int main(int argc, char **argv) {
    printf("%s output is in UTF-8\n", argv[0]);

    printf("C++ API\n");
    cppapi();

    printf("C API\n");
    capi();

    showCurrencyFormatting(FALSE);
    showCurrencyFormatting(TRUE);

    u_cleanup();    // Release any additional storage held by ICU.  

    printf("Exiting successfully\n");
    return 0;
}

/**
 * Sample code for the C++ API to NumberFormat.
 */
void cppapi() {
    Locale us("en", "US");
    UErrorCode status = U_ZERO_ERROR;
    
    // Create a number formatter for the US locale
    NumberFormat *fmt = NumberFormat::createInstance(us, status);
    check(status, "NumberFormat::createInstance");

    // Parse a string.  The string uses the digits '0' through '9'
    // and the decimal separator '.', standard in the US locale
    UnicodeString str("9876543210.123");
    Formattable result;
    fmt->parse(str, result, status);
    check(status, "NumberFormat::parse");

    printf("NumberFormat::parse(\""); // Display the result
    uprintf(str);
    printf("\") => ");
    uprintf(formattableToString(result));
    printf("\n");

    // Take the number parsed above, and use the formatter to
    // format it.
    str.remove(); // format() will APPEND to this string
    fmt->format(result, str, status);
    check(status, "NumberFormat::format");

    printf("NumberFormat::format("); // Display the result
    uprintf(formattableToString(result));
    printf(") => \"");
    uprintf(str);
    printf("\"\n");

    delete fmt; // Release the storage used by the formatter
    
}

// currency formatting ----------------------------------------------------- ***

/*
 * Set a currency on a NumberFormat with pre-ICU 2.6 APIs.
 * This is a "hack" that will not work properly for all cases because
 * only ICU 2.6 introduced a more complete framework and data for this.
 *
 * @param nf The NumberFormat on which to set the currency; takes effect on
 *           currency-formatting NumberFormat instances.
 *           This must actually be a DecimalFormat instance.
 *           The display style of the output is controlled by nf (its pattern,
 *           usually from the display locale ID used to create this instance)
 *           while the currency symbol and number of decimals are set for
 *           the currency.
 * @param currency The 3-letter ISO 4217 currency code, NUL-terminated.
 * @param errorCode ICU error code, must pass U_SUCCESS() on input.
 */
static void
setNumberFormatCurrency_2_4(NumberFormat &nf, const char *currency, UErrorCode &errorCode) {
    // argument checking
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(currency==NULL || strlen(currency)!=3) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // check that the formatter is a DecimalFormat instance
    // necessary because we will cast to the DecimalFormat subclass to set
    // the currency symbol
    if(nf.getDynamicClassID()!=DecimalFormat::getStaticClassID()) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // map the currency code to a locale ID
    // only the currencies in this array are supported
    // it would be possible to map to a locale ID, instantiate a currency
    // formatter for that and copy its values, but that would be slower,
    // and we have to hardcode something here anyway
    static const struct {
        // ISO currency ID
        const char *currency;

        // fractionDigits==minimumFractionDigits==maximumFractionDigits
        // for these currencies
        int32_t fractionDigits;

        /*
         * Set the rounding increment to 0 if it is implied with the number of
         * fraction digits. Setting an explicit rounding increment makes
         * number formatting slower.
         * In other words, set it to something other than 0 only for unusual
         * cases like "nickel rounding" (0.05) when the increment differs from
         * 10^(-maximumFractionDigits).
         */
        double roundingIncrement;

        // Unicode string with the desired currency display symbol or name
        UChar symbol[16];
    } currencyMap[]={
        { "USD", 2, 0.0, { 0x24, 0 } },
        { "GBP", 2, 0.0, { 0xa3, 0 } },
        { "EUR", 2, 0.0, { 0x20ac, 0 } },
        { "JPY", 0, 0.0, { 0xa5, 0 } }
    };

    int32_t i;

    for(i=0; i<LENGTHOF(currencyMap); ++i) {
        if(strcmp(currency, currencyMap[i].currency)==0) {
            break;
        }
    }
    if(i==LENGTHOF(currencyMap)) {
        // a more specific error code would be useful in a real application
        errorCode=U_UNSUPPORTED_ERROR;
        return;
    }

    // set the currency-related data into the caller's formatter

    nf.setMinimumFractionDigits(currencyMap[i].fractionDigits);
    nf.setMaximumFractionDigits(currencyMap[i].fractionDigits);

    DecimalFormat &dnf=(DecimalFormat &)nf;
    dnf.setRoundingIncrement(currencyMap[i].roundingIncrement);

    DecimalFormatSymbols symbols(*dnf.getDecimalFormatSymbols());
    symbols.setSymbol(DecimalFormatSymbols::kCurrencySymbol, currencyMap[i].symbol);
    dnf.setDecimalFormatSymbols(symbols); // do not adopt symbols: Jitterbug 2889
}

/*
 * Set a currency on a NumberFormat with ICU 2.6 APIs.
 *
 * @param nf The NumberFormat on which to set the currency; takes effect on
 *           currency-formatting NumberFormat instances.
 *           The display style of the output is controlled by nf (its pattern,
 *           usually from the display locale ID used to create this instance)
 *           while the currency symbol and number of decimals are set for
 *           the currency.
 * @param currency The 3-letter ISO 4217 currency code, NUL-terminated.
 * @param errorCode ICU error code, must pass U_SUCCESS() on input.
 */
static void
setNumberFormatCurrency_2_6(NumberFormat &nf, const char *currency, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(currency==NULL || strlen(currency)!=3) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // invariant-character conversion to UChars (see utypes.h and putil.h)
    UChar uCurrency[4];
    u_charsToUChars(currency, uCurrency, 4);

    // set the currency
    // in ICU 3.0 this API (which was @draft ICU 2.6) gained a UErrorCode& argument
#if (U_ICU_VERSION_MAJOR_NUM < 3)
    nf.setCurrency(uCurrency);
#else
    nf.setCurrency(uCurrency, errorCode);
#endif
}

static const char *const
sampleLocaleIDs[]={
    // use locale IDs complete with country code to be sure to
    // pick up number/currency format patterns
    "en_US", "en_GB", "de_DE", "ja_JP", "fr_FR", "hi_IN"
};

static const char *const
sampleCurrencies[]={
    "USD", "GBP", "EUR", "JPY"
};

static void
showCurrencyFormatting(UBool useICU26API) {
    NumberFormat *nf;
    int32_t i, j;

    UnicodeString output;

    UErrorCode errorCode;

    // TODO: Using printf() here assumes that the runtime encoding is ASCII-friendly
    // and can therefore be mixed with UTF-8

    for(i=0; i<LENGTHOF(sampleLocaleIDs); ++i) {
        printf("show currency formatting (method for %s) in the locale \"%s\"\n",
                useICU26API ? "ICU 2.6" : "before ICU 2.6",
                sampleLocaleIDs[i]);

        // get a currency formatter for this locale ID
        errorCode=U_ZERO_ERROR;
        nf=NumberFormat::createCurrencyInstance(sampleLocaleIDs[i], errorCode);
        if(U_FAILURE(errorCode)) {
            printf("NumberFormat::createCurrencyInstance(%s) failed - %s\n",
                    sampleLocaleIDs[i], u_errorName(errorCode));
            continue;
        }

        for(j=0; j<LENGTHOF(sampleCurrencies); ++j) {
            printf("  - format currency \"%s\": ", sampleCurrencies[j]);

            // set the actual currency to be formatted
            if(useICU26API) {
                setNumberFormatCurrency_2_6(*nf, sampleCurrencies[j], errorCode);
            } else {
                setNumberFormatCurrency_2_4(*nf, sampleCurrencies[j], errorCode);
            }
            if(U_FAILURE(errorCode)) {
                printf("setNumberFormatCurrency(%s) failed - %s\n",
                        sampleCurrencies[j], u_errorName(errorCode));
                continue;
            }

            // output=formatted currency value
            output.remove();
            nf->format(12345678.93, output);
            output+=(UChar)0x0a; // '\n'
            uprintf(output);
        }
    }
}
