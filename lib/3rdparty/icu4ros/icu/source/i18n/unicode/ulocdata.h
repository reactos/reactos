/*
******************************************************************************
*                                                                            *
* Copyright (C) 2003-2007, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  ulocdata.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003Oct21
*   created by: Ram Viswanadha
*/

#ifndef __ULOCDATA_H__
#define __ULOCDATA_H__

#include "unicode/ures.h"
#include "unicode/uloc.h"
#include "unicode/uset.h"

/**
 * \file
 * \brief C API: Provides access to locale data. 
 */

/** Forward declaration of the ULocaleData structure. @stable ICU 3.6 */
struct ULocaleData;

/** A locale data object. @stable ICU 3.6 */
typedef struct ULocaleData ULocaleData;



/** The possible types of exemplar character sets.
  * @stable ICU 3.4
  */
typedef enum ULocaleDataExemplarSetType  {
     ULOCDATA_ES_STANDARD=0,      /* Basic set */
     ULOCDATA_ES_AUXILIARY=1,     /* Auxiliary set */
     ULOCDATA_ES_COUNT=2
} ULocaleDataExemplarSetType;

/** The possible types of delimiters.
  * @stable ICU 3.4
  */
typedef enum ULocaleDataDelimiterType {
#ifndef U_HIDE_DRAFT_API 
    ULOCDATA_QUOTATION_START = 0,     /* Quotation start */
     ULOCDATA_QUOTATION_END = 1,       /* Quotation end */
     ULOCDATA_ALT_QUOTATION_START = 2, /* Alternate quotation start */
     ULOCDATA_ALT_QUOTATION_END = 3,   /* Alternate quotation end */
#endif
     ULOCDATA_DELIMITER_COUNT = 4
} ULocaleDataDelimiterType;

/**
 * Opens a locale data object for the given locale
 *
 * @param localeID  Specifies the locale associated with this locale
 *                  data object.
 * @param status    Pointer to error status code.
 * @stable ICU 3.4
 */
U_STABLE ULocaleData* U_EXPORT2 
ulocdata_open(const char *localeID, UErrorCode *status);

/**
 * Closes a locale data object.
 *
 * @param uld       The locale data object to close
 * @stable ICU 3.4
 */
U_STABLE void U_EXPORT2 
ulocdata_close(ULocaleData *uld);

/**
 * Sets the "no Substitute" attribute of the locale data
 * object.  If true, then any methods associated with the
 * locale data object will return null when there is no
 * data available for that method, given the locale ID
 * supplied to ulocdata_open().
 *
 * @param uld       The locale data object to set.
 * @param setting   Value of the "no substitute" attribute.
 * @stable ICU 3.4
 */
U_STABLE void U_EXPORT2 
ulocdata_setNoSubstitute(ULocaleData *uld, UBool setting);

/**
 * Retrieves the current "no Substitute" value of the locale data
 * object.  If true, then any methods associated with the
 * locale data object will return null when there is no
 * data available for that method, given the locale ID
 * supplied to ulocdata_open().
 *
 * @param uld       Pointer to the The locale data object to set.
 * @return UBool    Value of the "no substitute" attribute.
 * @stable ICU 3.4
 */
U_STABLE UBool U_EXPORT2 
ulocdata_getNoSubstitute(ULocaleData *uld);

/**
 * Returns the set of exemplar characters for a locale.
 *
 * @param uld       Pointer to the locale data object from which the 
 *                  exemplar character set is to be retrieved.
 * @param fillIn    Pointer to a USet object to receive the 
 *                  exemplar character set for the given locale.  Previous
 *                  contents of fillIn are lost.  <em>If fillIn is NULL,
 *                  then a new USet is created and returned.  The caller
 *                  owns the result and must dispose of it by calling
 *                  uset_close.</em>
 * @param options   Bitmask for options to apply to the exemplar pattern.
 *                  Specify zero to retrieve the exemplar set as it is
 *                  defined in the locale data.  Specify
 *                  USET_CASE_INSENSITIVE to retrieve a case-folded
 *                  exemplar set.  See uset_applyPattern for a complete
 *                  list of valid options.  The USET_IGNORE_SPACE bit is
 *                  always set, regardless of the value of 'options'.
 * @param extype    Specifies the type of exemplar set to be retrieved.
 * @param status    Pointer to an input-output error code value;
 *                  must not be NULL.
 * @return USet*    Either fillIn, or if fillIn is NULL, a pointer to
 *                  a newly-allocated USet that the user must close.
 * @stable ICU 3.4
 */
U_STABLE USet* U_EXPORT2 
ulocdata_getExemplarSet(ULocaleData *uld, USet *fillIn, 
                        uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode *status);

/**
 * Returns one of the delimiter strings associated with a locale.
 *
 * @param uld           Pointer to the locale data object from which the 
 *                      delimiter string is to be retrieved.
 * @param type          the type of delimiter to be retrieved.
 * @param result        A pointer to a buffer to receive the result.
 * @param resultLength  The maximum size of result.
 * @param status        Pointer to an error code value
 * @return int32_t      The total buffer size needed; if greater than resultLength,
 *                      the output was truncated.
 * @stable ICU 3.4
 */
U_STABLE int32_t U_EXPORT2 
ulocdata_getDelimiter(ULocaleData *uld, ULocaleDataDelimiterType type, UChar *result, int32_t resultLength, UErrorCode *status);

/**
 * Enumeration for representing the measurement systems.
 * @stable ICU 2.8
 */
typedef enum UMeasurementSystem {
    UMS_SI,     /** Measurement system specified by SI otherwise known as Metric system. */
    UMS_US,     /** Measurement system followed in the United States of America. */ 
    UMS_LIMIT
} UMeasurementSystem;

/**
 * Returns the measurement system used in the locale specified by the localeID.
 * Please note that this API will change in ICU 3.6 and will use an ulocdata object.
 *
 * @param localeID      The id of the locale for which the measurement system to be retrieved.
 * @param status        Must be a valid pointer to an error code value,
 *                      which must not indicate a failure before the function call.
 * @return UMeasurementSystem the measurement system used in the locale.
 * @stable ICU 2.8
 */
U_STABLE UMeasurementSystem U_EXPORT2
ulocdata_getMeasurementSystem(const char *localeID, UErrorCode *status);

/**
 * Returns the element gives the normal business letter size, and customary units. 
 * The units for the numbers are always in <em>milli-meters</em>.
 * For US since 8.5 and 11 do not yeild an integral value when converted to milli-meters,
 * the values are rounded off.
 * So for A4 size paper the height and width are 297 mm and 210 mm repectively, 
 * and for US letter size the height and width are 279 mm and 216 mm respectively.
 * Please note that this API will change in ICU 3.6 and will use an ulocdata object.
 *
 * @param localeID      The id of the locale for which the paper size information to be retrieved.
 * @param height        A pointer to int to recieve the height information.
 * @param width         A pointer to int to recieve the width information.
 * @param status        Must be a valid pointer to an error code value,
 *                      which must not indicate a failure before the function call.
 * @stable ICU 2.8
 */
U_STABLE void U_EXPORT2
ulocdata_getPaperSize(const char *localeID, int32_t *height, int32_t *width, UErrorCode *status);

#endif
