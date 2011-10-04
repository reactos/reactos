/*
*******************************************************************************
*   Copyright (C) 1996-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/udat.h"

#include "unicode/uloc.h"
#include "unicode/datefmt.h"
#include "unicode/timezone.h"
#include "unicode/smpdtfmt.h"
#include "unicode/fieldpos.h"
#include "unicode/parsepos.h"
#include "unicode/calendar.h"
#include "unicode/numfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/ustring.h"
#include "cpputils.h"

U_NAMESPACE_USE

U_CAPI UDateFormat* U_EXPORT2
udat_open(UDateFormatStyle  timeStyle,
          UDateFormatStyle  dateStyle,
          const char        *locale,
          const UChar       *tzID,
          int32_t           tzIDLength,
          const UChar       *pattern,
          int32_t           patternLength,
          UErrorCode        *status)
{
    DateFormat *fmt;
    if(U_FAILURE(*status)) {
        return 0;
    }
    if(timeStyle != UDAT_IGNORE) {
        if(locale == 0) {
            fmt = DateFormat::createDateTimeInstance((DateFormat::EStyle)dateStyle,
                (DateFormat::EStyle)timeStyle);
        }
        else {
            fmt = DateFormat::createDateTimeInstance((DateFormat::EStyle)dateStyle,
                (DateFormat::EStyle)timeStyle,
                Locale(locale));
        }
    }
    else {
        UnicodeString pat((UBool)(patternLength == -1), pattern, patternLength);

        if(locale == 0) {
            fmt = new SimpleDateFormat(pat, *status);
        }
        else {
            fmt = new SimpleDateFormat(pat, Locale(locale), *status);
        }
    }

    if(fmt == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    if(tzID != 0) {
        TimeZone *zone = TimeZone::createTimeZone(UnicodeString((UBool)(tzIDLength == -1), tzID, tzIDLength));
        if(zone == 0) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            delete fmt;
            return 0;
        }
        fmt->adoptTimeZone(zone);
    }

    return (UDateFormat*)fmt;
}


U_CAPI void U_EXPORT2
udat_close(UDateFormat* format)
{
    delete (DateFormat*)format;
}

U_CAPI UDateFormat* U_EXPORT2
udat_clone(const UDateFormat *fmt,
       UErrorCode *status)
{
    if(U_FAILURE(*status)) return 0;

    Format *res = ((SimpleDateFormat*)fmt)->clone();

    if(res == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    return (UDateFormat*) res;
}

U_CAPI int32_t U_EXPORT2
udat_format(    const    UDateFormat*    format,
        UDate           dateToFormat,
        UChar*          result,
        int32_t         resultLength,
        UFieldPosition* position,
        UErrorCode*     status)
{
    if(U_FAILURE(*status)) return -1;

    UnicodeString res;
    if(!(result==NULL && resultLength==0)) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer
        res.setTo(result, 0, resultLength);
    }

    FieldPosition fp;

    if(position != 0)
        fp.setField(position->field);

    ((DateFormat*)format)->format(dateToFormat, res, fp);

    if(position != 0) {
        position->beginIndex = fp.getBeginIndex();
        position->endIndex = fp.getEndIndex();
    }

    return res.extract(result, resultLength, *status);
}

U_CAPI UDate U_EXPORT2
udat_parse(    const    UDateFormat*        format,
        const    UChar*          text,
        int32_t         textLength,
        int32_t         *parsePos,
        UErrorCode      *status)
{
    if(U_FAILURE(*status)) return (UDate)0;

    const UnicodeString src((UBool)(textLength == -1), text, textLength);
    ParsePosition pp;
    int32_t stackParsePos = 0;
    UDate res;

    if(parsePos == NULL) {
        parsePos = &stackParsePos;
    }

    pp.setIndex(*parsePos);

    res = ((DateFormat*)format)->parse(src, pp);

    if(pp.getErrorIndex() == -1)
        *parsePos = pp.getIndex();
    else {
        *parsePos = pp.getErrorIndex();
        *status = U_PARSE_ERROR;
    }

    return res;
}

U_CAPI void U_EXPORT2
udat_parseCalendar(const    UDateFormat*    format,
                            UCalendar*      calendar,
                   const    UChar*          text,
                            int32_t         textLength,
                            int32_t         *parsePos,
                            UErrorCode      *status)
{
    if(U_FAILURE(*status)) return;

    const UnicodeString src((UBool)(textLength == -1), text, textLength);
    ParsePosition pp;

    if(parsePos != 0)
        pp.setIndex(*parsePos);

    ((DateFormat*)format)->parse(src, *(Calendar*)calendar, pp);

    if(parsePos != 0) {
        if(pp.getErrorIndex() == -1)
            *parsePos = pp.getIndex();
        else {
            *parsePos = pp.getErrorIndex();
            *status = U_PARSE_ERROR;
        }
    }
}

U_CAPI UBool U_EXPORT2
udat_isLenient(const UDateFormat* fmt)
{
    return ((DateFormat*)fmt)->isLenient();
}

U_CAPI void U_EXPORT2
udat_setLenient(    UDateFormat*    fmt,
            UBool          isLenient)
{
    ((DateFormat*)fmt)->setLenient(isLenient);
}

U_CAPI const UCalendar* U_EXPORT2
udat_getCalendar(const UDateFormat* fmt)
{
    return (const UCalendar*) ((DateFormat*)fmt)->getCalendar();
}

U_CAPI void U_EXPORT2
udat_setCalendar(UDateFormat*    fmt,
                 const   UCalendar*      calendarToSet)
{
    ((DateFormat*)fmt)->setCalendar(*((Calendar*)calendarToSet));
}

U_CAPI const UNumberFormat* U_EXPORT2
udat_getNumberFormat(const UDateFormat* fmt)
{
    return (const UNumberFormat*) ((DateFormat*)fmt)->getNumberFormat();
}

U_CAPI void U_EXPORT2
udat_setNumberFormat(UDateFormat*    fmt,
                     const   UNumberFormat*  numberFormatToSet)
{
    ((DateFormat*)fmt)->setNumberFormat(*((NumberFormat*)numberFormatToSet));
}

U_CAPI const char* U_EXPORT2
udat_getAvailable(int32_t index)
{
    return uloc_getAvailable(index);
}

U_CAPI int32_t U_EXPORT2
udat_countAvailable()
{
    return uloc_countAvailable();
}

U_CAPI UDate U_EXPORT2
udat_get2DigitYearStart(    const   UDateFormat     *fmt,
                        UErrorCode      *status)
{
    if(U_FAILURE(*status)) return (UDate)0;
    return ((SimpleDateFormat*)fmt)->get2DigitYearStart(*status);
}

U_CAPI void U_EXPORT2
udat_set2DigitYearStart(    UDateFormat     *fmt,
                        UDate           d,
                        UErrorCode      *status)
{
    if(U_FAILURE(*status)) return;
    ((SimpleDateFormat*)fmt)->set2DigitYearStart(d, *status);
}

U_CAPI int32_t U_EXPORT2
udat_toPattern(    const   UDateFormat     *fmt,
        UBool          localized,
        UChar           *result,
        int32_t         resultLength,
        UErrorCode      *status)
{
    if(U_FAILURE(*status)) return -1;

    UnicodeString res;
    if(!(result==NULL && resultLength==0)) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer
        res.setTo(result, 0, resultLength);
    }

    if(localized)
        ((SimpleDateFormat*)fmt)->toLocalizedPattern(res, *status);
    else
        ((SimpleDateFormat*)fmt)->toPattern(res);

    return res.extract(result, resultLength, *status);
}

// TBD: should this take an UErrorCode?
U_CAPI void U_EXPORT2
udat_applyPattern(  UDateFormat     *format,
                    UBool          localized,
                    const   UChar           *pattern,
                    int32_t         patternLength)
{
    const UnicodeString pat((UBool)(patternLength == -1), pattern, patternLength);
    UErrorCode status = U_ZERO_ERROR;

    if(localized)
        ((SimpleDateFormat*)format)->applyLocalizedPattern(pat, status);
    else
        ((SimpleDateFormat*)format)->applyPattern(pat);
}

U_CAPI int32_t U_EXPORT2
udat_getSymbols(const   UDateFormat     *fmt,
                UDateFormatSymbolType   type,
                int32_t                 index,
                UChar                   *result,
                int32_t                 resultLength,
                UErrorCode              *status)
{
    if(U_FAILURE(*status)) return -1;

    const DateFormatSymbols *syms = 
        ((SimpleDateFormat*)fmt)->getDateFormatSymbols();
    int32_t count;
    const UnicodeString *res = NULL;

    switch(type) {
    case UDAT_ERAS:
        res = syms->getEras(count);
        break;

    case UDAT_ERA_NAMES:
        res = syms->getEraNames(count);
        break;

    case UDAT_MONTHS:
        res = syms->getMonths(count);
        break;

    case UDAT_SHORT_MONTHS:
        res = syms->getShortMonths(count);
        break;

    case UDAT_WEEKDAYS:
        res = syms->getWeekdays(count);
        break;

    case UDAT_SHORT_WEEKDAYS:
        res = syms->getShortWeekdays(count);
        break;

    case UDAT_AM_PMS:
        res = syms->getAmPmStrings(count);
        break;

    case UDAT_LOCALIZED_CHARS:
        {
            UnicodeString res1;
            if(!(result==NULL && resultLength==0)) {
                // NULL destination for pure preflighting: empty dummy string
                // otherwise, alias the destination buffer
                res1.setTo(result, 0, resultLength);
            }
            syms->getLocalPatternChars(res1);
            return res1.extract(result, resultLength, *status);
        }

    case UDAT_NARROW_MONTHS:
        res = syms->getMonths(count, DateFormatSymbols::FORMAT, DateFormatSymbols::NARROW);
        break;

    case UDAT_NARROW_WEEKDAYS:
        res = syms->getWeekdays(count, DateFormatSymbols::FORMAT, DateFormatSymbols::NARROW);
        break;

    case UDAT_STANDALONE_MONTHS:
        res = syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_MONTHS:
        res = syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_NARROW_MONTHS:
        res = syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::NARROW);
        break;

    case UDAT_STANDALONE_WEEKDAYS:
        res = syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_WEEKDAYS:
        res = syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_NARROW_WEEKDAYS:
        res = syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::NARROW);
        break;

    case UDAT_QUARTERS:
        res = syms->getQuarters(count, DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE);
        break;

    case UDAT_SHORT_QUARTERS:
        res = syms->getQuarters(count, DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_QUARTERS:
        res = syms->getQuarters(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_QUARTERS:
        res = syms->getQuarters(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    }

    if(index < count) {
        return res[index].extract(result, resultLength, *status);
    }
    return 0;
}

U_CAPI int32_t U_EXPORT2
udat_countSymbols(    const    UDateFormat                *fmt,
            UDateFormatSymbolType    type)
{
    const DateFormatSymbols *syms = 
        ((SimpleDateFormat*)fmt)->getDateFormatSymbols();
    int32_t count = 0;

    switch(type) {
    case UDAT_ERAS:
        syms->getEras(count);
        break;

    case UDAT_MONTHS:
        syms->getMonths(count);
        break;

    case UDAT_SHORT_MONTHS:
        syms->getShortMonths(count);
        break;

    case UDAT_WEEKDAYS:
        syms->getWeekdays(count);
        break;

    case UDAT_SHORT_WEEKDAYS:
        syms->getShortWeekdays(count);
        break;

    case UDAT_AM_PMS:
        syms->getAmPmStrings(count);
        break;

    case UDAT_LOCALIZED_CHARS:
        count = 1;
        break;

    case UDAT_ERA_NAMES:
        syms->getEraNames(count);
        break;

    case UDAT_NARROW_MONTHS:
        syms->getMonths(count, DateFormatSymbols::FORMAT, DateFormatSymbols::NARROW);
        break;

    case UDAT_NARROW_WEEKDAYS:
        syms->getWeekdays(count, DateFormatSymbols::FORMAT, DateFormatSymbols::NARROW);
        break;

    case UDAT_STANDALONE_MONTHS:
        syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_MONTHS:
        syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_NARROW_MONTHS:
        syms->getMonths(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::NARROW);
        break;

    case UDAT_STANDALONE_WEEKDAYS:
        syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_WEEKDAYS:
        syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_NARROW_WEEKDAYS:
        syms->getWeekdays(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::NARROW);
        break;

    case UDAT_QUARTERS:
        syms->getQuarters(count, DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE);
        break;

    case UDAT_SHORT_QUARTERS:
        syms->getQuarters(count, DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED);
        break;

    case UDAT_STANDALONE_QUARTERS:
        syms->getQuarters(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;

    case UDAT_STANDALONE_SHORT_QUARTERS:
        syms->getQuarters(count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;

    }

    return count;
}

U_NAMESPACE_BEGIN

/*
 * This DateFormatSymbolsSingleSetter class is a friend of DateFormatSymbols
 * solely for the purpose of avoiding to clone the array of strings
 * just to modify one of them and then setting all of them back.
 * For example, the old code looked like this:
 *  case UDAT_MONTHS:
 *    res = syms->getMonths(count);
 *    array = new UnicodeString[count];
 *    if(array == 0) {
 *      *status = U_MEMORY_ALLOCATION_ERROR;
 *      return;
 *    }
 *    uprv_arrayCopy(res, array, count);
 *    if(index < count)
 *      array[index] = val;
 *    syms->setMonths(array, count);
 *    break;
 *
 * Even worse, the old code actually cloned the entire DateFormatSymbols object,
 * cloned one value array, changed one value, and then made the SimpleDateFormat
 * replace its DateFormatSymbols object with the new one.
 *
 * markus 2002-oct-14
 */
class DateFormatSymbolsSingleSetter /* not : public UObject because all methods are static */ {
public:
    static void
        setSymbol(UnicodeString *array, int32_t count, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        if(array!=NULL) {
            if(index>=count) {
                errorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            } else if(value==NULL) {
                errorCode=U_ILLEGAL_ARGUMENT_ERROR;
            } else {
                array[index].setTo(value, valueLength);
            }
        }
    }

    static void
        setEra(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fEras, syms->fErasCount, index, value, valueLength, errorCode);
    }

    static void
        setEraName(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fEraNames, syms->fEraNamesCount, index, value, valueLength, errorCode);
    }

    static void
        setMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fMonths, syms->fMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setShortMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fShortMonths, syms->fShortMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setNarrowMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fNarrowMonths, syms->fNarrowMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneMonths, syms->fStandaloneMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneShortMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneShortMonths, syms->fStandaloneShortMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneNarrowMonth(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneNarrowMonths, syms->fStandaloneNarrowMonthsCount, index, value, valueLength, errorCode);
    }

    static void
        setWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fWeekdays, syms->fWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setShortWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fShortWeekdays, syms->fShortWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setNarrowWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fNarrowWeekdays, syms->fNarrowWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneWeekdays, syms->fStandaloneWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneShortWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneShortWeekdays, syms->fStandaloneShortWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneNarrowWeekday(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneNarrowWeekdays, syms->fStandaloneNarrowWeekdaysCount, index, value, valueLength, errorCode);
    }

    static void
        setQuarter(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fQuarters, syms->fQuartersCount, index, value, valueLength, errorCode);
    }

    static void
        setShortQuarter(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fShortQuarters, syms->fShortQuartersCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneQuarter(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneQuarters, syms->fStandaloneQuartersCount, index, value, valueLength, errorCode);
    }

    static void
        setStandaloneShortQuarter(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fStandaloneShortQuarters, syms->fStandaloneShortQuartersCount, index, value, valueLength, errorCode);
    }

    static void
        setAmPm(DateFormatSymbols *syms, int32_t index,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(syms->fAmPms, syms->fAmPmsCount, index, value, valueLength, errorCode);
    }

    static void
        setLocalPatternChars(DateFormatSymbols *syms,
        const UChar *value, int32_t valueLength, UErrorCode &errorCode)
    {
        setSymbol(&syms->fLocalPatternChars, 1, 0, value, valueLength, errorCode);
    }
};

U_NAMESPACE_END

U_CAPI void U_EXPORT2
udat_setSymbols(    UDateFormat             *format,
            UDateFormatSymbolType   type,
            int32_t                 index,
            UChar                   *value,
            int32_t                 valueLength,
            UErrorCode              *status)
{

    if(U_FAILURE(*status)) return;

    DateFormatSymbols *syms = (DateFormatSymbols *)((SimpleDateFormat *)format)->getDateFormatSymbols();

    switch(type) {
    case UDAT_ERAS:
        DateFormatSymbolsSingleSetter::setEra(syms, index, value, valueLength, *status);
        break;

    case UDAT_ERA_NAMES:
        DateFormatSymbolsSingleSetter::setEraName(syms, index, value, valueLength, *status);
        break;

    case UDAT_MONTHS:
        DateFormatSymbolsSingleSetter::setMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_SHORT_MONTHS:
        DateFormatSymbolsSingleSetter::setShortMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_NARROW_MONTHS:
        DateFormatSymbolsSingleSetter::setNarrowMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_MONTHS:
        DateFormatSymbolsSingleSetter::setStandaloneMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_SHORT_MONTHS:
        DateFormatSymbolsSingleSetter::setStandaloneShortMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_NARROW_MONTHS:
        DateFormatSymbolsSingleSetter::setStandaloneNarrowMonth(syms, index, value, valueLength, *status);
        break;

    case UDAT_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_SHORT_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setShortWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_NARROW_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setNarrowWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setStandaloneWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_SHORT_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setStandaloneShortWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_NARROW_WEEKDAYS:
        DateFormatSymbolsSingleSetter::setStandaloneNarrowWeekday(syms, index, value, valueLength, *status);
        break;

    case UDAT_QUARTERS:
        DateFormatSymbolsSingleSetter::setQuarter(syms, index, value, valueLength, *status);
        break;

    case UDAT_SHORT_QUARTERS:
        DateFormatSymbolsSingleSetter::setShortQuarter(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_QUARTERS:
        DateFormatSymbolsSingleSetter::setStandaloneQuarter(syms, index, value, valueLength, *status);
        break;

    case UDAT_STANDALONE_SHORT_QUARTERS:
        DateFormatSymbolsSingleSetter::setStandaloneShortQuarter(syms, index, value, valueLength, *status);
        break;

    case UDAT_AM_PMS:
        DateFormatSymbolsSingleSetter::setAmPm(syms, index, value, valueLength, *status);
        break;

    case UDAT_LOCALIZED_CHARS:
        DateFormatSymbolsSingleSetter::setLocalPatternChars(syms, value, valueLength, *status);
        break;

    default:
        *status = U_UNSUPPORTED_ERROR;
        break;
        
    }
}

U_CAPI const char* U_EXPORT2
udat_getLocaleByType(const UDateFormat *fmt,
                     ULocDataLocaleType type,
                     UErrorCode* status)
{
    if (fmt == NULL) {
        if (U_SUCCESS(*status)) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return NULL;
    }
    return ((Format*)fmt)->getLocaleID(type, *status);
}
#endif /* #if !UCONFIG_NO_FORMATTING */
