/*
**********************************************************************
* Copyright (c) 2002-2006, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
*/
#ifndef _UCURR_H_
#define _UCURR_H_

#include "unicode/utypes.h"
#include "unicode/uenum.h"

/**
 * \file 
 * \brief C API: Encapsulates information about a currency.
 */

#if !UCONFIG_NO_FORMATTING

/**
 * The ucurr API encapsulates information about a currency, as defined by
 * ISO 4217.  A currency is represented by a 3-character string
 * containing its ISO 4217 code.  This API can return various data
 * necessary the proper display of a currency:
 *
 * <ul><li>A display symbol, for a specific locale
 * <li>The number of fraction digits to display
 * <li>A rounding increment
 * </ul>
 *
 * The <tt>DecimalFormat</tt> class uses these data to display
 * currencies.
 * @author Alan Liu
 * @since ICU 2.2
 */

/**
 * Finds a currency code for the given locale.
 * @param locale the locale for which to retrieve a currency code. 
 *               Currency can be specified by the "currency" keyword
 *               in which case it overrides the default currency code
 * @param buff   fill in buffer. Can be NULL for preflighting.
 * @param buffCapacity capacity of the fill in buffer. Can be 0 for
 *               preflighting. If it is non-zero, the buff parameter
 *               must not be NULL.
 * @param ec error code
 * @return length of the currency string. It should always be 3. If 0,
 *                currency couldn't be found or the input values are 
 *                invalid. 
 * @stable ICU 2.8
 */
U_STABLE int32_t U_EXPORT2
ucurr_forLocale(const char* locale,
                UChar* buff,
                int32_t buffCapacity,
                UErrorCode* ec);

/**
 * Selector constants for ucurr_getName().
 *
 * @see ucurr_getName
 * @stable ICU 2.6
 */
typedef enum UCurrNameStyle {
    /**
     * Selector for ucurr_getName indicating a symbolic name for a
     * currency, such as "$" for USD.
     * @stable ICU 2.6
     */
    UCURR_SYMBOL_NAME,

    /**
     * Selector for ucurr_getName indicating the long name for a
     * currency, such as "US Dollar" for USD.
     * @stable ICU 2.6
     */
    UCURR_LONG_NAME
} UCurrNameStyle;

#if !UCONFIG_NO_SERVICE
/**
 * @stable ICU 2.6
 */
typedef const void* UCurrRegistryKey;

/**
 * Register an (existing) ISO 4217 currency code for the given locale.
 * Only the country code and the two variants EURO and PRE_EURO are
 * recognized.
 * @param isoCode the three-letter ISO 4217 currency code
 * @param locale  the locale for which to register this currency code
 * @param status the in/out status code
 * @return a registry key that can be used to unregister this currency code, or NULL
 * if there was an error.
 * @stable ICU 2.6
 */
U_STABLE UCurrRegistryKey U_EXPORT2
ucurr_register(const UChar* isoCode, 
                   const char* locale,  
                   UErrorCode* status);
/**
 * Unregister the previously-registered currency definitions using the
 * URegistryKey returned from ucurr_register.  Key becomes invalid after
 * a successful call and should not be used again.  Any currency 
 * that might have been hidden by the original ucurr_register call is 
 * restored.
 * @param key the registry key returned by a previous call to ucurr_register
 * @param status the in/out status code, no special meanings are assigned
 * @return TRUE if the currency for this key was successfully unregistered
 * @stable ICU 2.6
 */
U_STABLE UBool U_EXPORT2
ucurr_unregister(UCurrRegistryKey key, UErrorCode* status);
#endif /* UCONFIG_NO_SERVICE */

/**
 * Returns the display name for the given currency in the
 * given locale.  For example, the display name for the USD
 * currency object in the en_US locale is "$".
 * @param currency null-terminated 3-letter ISO 4217 code
 * @param locale locale in which to display currency
 * @param nameStyle selector for which kind of name to return
 * @param isChoiceFormat fill-in set to TRUE if the returned value
 * is a ChoiceFormat pattern; otherwise it is a static string
 * @param len fill-in parameter to receive length of result
 * @param ec error code
 * @return pointer to display string of 'len' UChars.  If the resource
 * data contains no entry for 'currency', then 'currency' itself is
 * returned.  If *isChoiceFormat is TRUE, then the result is a
 * ChoiceFormat pattern.  Otherwise it is a static string.
 * @stable ICU 2.6
 */
U_STABLE const UChar* U_EXPORT2
ucurr_getName(const UChar* currency,
              const char* locale,
              UCurrNameStyle nameStyle,
              UBool* isChoiceFormat,
              int32_t* len,
              UErrorCode* ec);

/**
 * Returns the number of the number of fraction digits that should
 * be displayed for the given currency.
 * @param currency null-terminated 3-letter ISO 4217 code
 * @param ec input-output error code
 * @return a non-negative number of fraction digits to be
 * displayed, or 0 if there is an error
 * @stable ICU 3.0
 */
U_STABLE int32_t U_EXPORT2
ucurr_getDefaultFractionDigits(const UChar* currency,
                               UErrorCode* ec);

/**
 * Returns the rounding increment for the given currency, or 0.0 if no
 * rounding is done by the currency.
 * @param currency null-terminated 3-letter ISO 4217 code
 * @param ec input-output error code
 * @return the non-negative rounding increment, or 0.0 if none,
 * or 0.0 if there is an error
 * @stable ICU 3.0
 */
U_STABLE double U_EXPORT2
ucurr_getRoundingIncrement(const UChar* currency,
                           UErrorCode* ec);

/**
 * Selector constants for ucurr_openCurrencies().
 *
 * @see ucurr_openCurrencies
 * @stable ICU 3.2
 */
typedef enum UCurrCurrencyType {
    /**
     * Select all ISO-4217 currency codes.
     * @stable ICU 3.2
     */
    UCURR_ALL = INT32_MAX,
    /**
     * Select only ISO-4217 commonly used currency codes.
     * These currencies can be found in common use, and they usually have
     * bank notes or coins associated with the currency code.
     * This does not include fund codes, precious metals and other
     * various ISO-4217 codes limited to special financial products.
     * @stable ICU 3.2
     */
    UCURR_COMMON = 1,
    /**
     * Select ISO-4217 uncommon currency codes.
     * These codes respresent fund codes, precious metals and other
     * various ISO-4217 codes limited to special financial products.
     * A fund code is a monetary resource associated with a currency.
     * @stable ICU 3.2
     */
    UCURR_UNCOMMON = 2,
    /**
     * Select only deprecated ISO-4217 codes.
     * These codes are no longer in general public use.
     * @stable ICU 3.2
     */
    UCURR_DEPRECATED = 4,
    /**
     * Select only non-deprecated ISO-4217 codes.
     * These codes are in general public use.
     * @stable ICU 3.2
     */
    UCURR_NON_DEPRECATED = 8
} UCurrCurrencyType;

/**
 * Provides a UEnumeration object for listing ISO-4217 codes.
 * @param currType You can use one of several UCurrCurrencyType values for this
 *      variable. You can also | (or) them together to get a specific list of
 *      currencies. Most people will want to use the (UCURR_CURRENCY|UCURR_NON_DEPRECATED) value to
 *      get a list of current currencies.
 * @param pErrorCode Error code
 * @stable ICU 3.2
 */
U_STABLE UEnumeration * U_EXPORT2
ucurr_openISOCurrencies(uint32_t currType, UErrorCode *pErrorCode);


#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
