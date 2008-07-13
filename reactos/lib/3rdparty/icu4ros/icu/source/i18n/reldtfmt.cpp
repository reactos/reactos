/*
*******************************************************************************
* Copyright (C) 2007, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

//#define DEBUG_RELDTFMT

#include <stdio.h>
#include <stdlib.h>

#include "reldtfmt.h"
#include "unicode/msgfmt.h"

#include "gregoimp.h" // for CalendarData
#include "cmemory.h"

U_NAMESPACE_BEGIN


/**
 * An array of URelativeString structs is used to store the resource data loaded out of the bundle.
 */
struct URelativeString {
    int32_t offset;         /** offset of this item, such as, the relative date **/
    int32_t len;            /** length of the string **/
    const UChar* string;    /** string, or NULL if not set **/
};


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RelativeDateFormat)

RelativeDateFormat::RelativeDateFormat(const RelativeDateFormat& other) :
DateFormat(other), fDateFormat(NULL), fTimeFormat(NULL), fCombinedFormat(NULL),
fDateStyle(other.fDateStyle), fTimeStyle(other.fTimeStyle), fLocale(other.fLocale),
fDayMin(other.fDayMin), fDayMax(other.fDayMax),
fDatesLen(other.fDatesLen), fDates(NULL)
{
    if(other.fDateFormat != NULL) {
        fDateFormat = (DateFormat*)other.fDateFormat->clone();
    } else {
        fDateFormat = NULL;
    }
    if (fDatesLen > 0) {
        fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);
        uprv_memcpy(fDates, other.fDates, sizeof(fDates[0])*fDatesLen);
    }
    //fCalendar = other.fCalendar->clone();
/*    
    if(other.fTimeFormat != NULL) {
        fTimeFormat = (DateFormat*)other.fTimeFormat->clone();
    } else {
        fTimeFormat = NULL;
    }
*/
}

RelativeDateFormat::RelativeDateFormat( UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const Locale& locale, UErrorCode& status)
 : DateFormat(), fDateFormat(NULL), fTimeFormat(NULL), fCombinedFormat(NULL),
fDateStyle(dateStyle), fTimeStyle(timeStyle), fLocale(locale), fDatesLen(0), fDates(NULL)
 {
    if(U_FAILURE(status) ) {
        return;
    }
    
    if(fDateStyle != UDAT_NONE) {
        EStyle newStyle = (EStyle)(fDateStyle & ~UDAT_RELATIVE);
        // Create a DateFormat in the non-relative style requested.
        fDateFormat = createDateInstance(newStyle, locale);
    }
    if(fTimeStyle != UDAT_NONE) {
        // don't support time style, for now
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    // Initialize the parent fCalendar, so that parse() works correctly.
    initializeCalendar(NULL, locale, status);
    loadDates(status);
}

RelativeDateFormat::~RelativeDateFormat() {
    delete fDateFormat;
    delete fTimeFormat;
    delete fCombinedFormat;
    uprv_free(fDates);
}


Format* RelativeDateFormat::clone(void) const {
    return new RelativeDateFormat(*this);
}

UBool RelativeDateFormat::operator==(const Format& other) const {
    if(DateFormat::operator==(other)) {
        // DateFormat::operator== guarantees following cast is safe
        RelativeDateFormat* that = (RelativeDateFormat*)&other;
        return (fDateStyle==that->fDateStyle   &&
                fTimeStyle==that->fTimeStyle   &&
                fLocale==that->fLocale);
    }
    return FALSE;
}

UnicodeString& RelativeDateFormat::format(  Calendar& cal,
                                UnicodeString& appendTo,
                                FieldPosition& pos) const {
                                
    UErrorCode status = U_ZERO_ERROR;
    
    // calculate the difference, in days, between 'cal' and now.
    int dayDiff = dayDifference(cal, status);

    // look up string
    int32_t len;
    const UChar *theString = getStringForDay(dayDiff, len, status);
    
    if(U_FAILURE(status) || (theString==NULL)) {
        // didn't find it. Fall through to the fDateFormat 
        if(fDateFormat != NULL) {
            return fDateFormat->format(cal,appendTo,pos);
        } else {
            return appendTo; // no op
        }
    } else {
        // found a relative string
        return appendTo.append(theString, len);
    }
}



UnicodeString&
RelativeDateFormat::format(const Formattable& obj, 
                         UnicodeString& appendTo, 
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    // this is just here to get around the hiding problem
    // (the previous format() override would hide the version of
    // format() on DateFormat that this function correspond to, so we
    // have to redefine it here)
    return DateFormat::format(obj, appendTo, pos, status);
}


void RelativeDateFormat::parse( const UnicodeString& text,
                    Calendar& cal,
                    ParsePosition& pos) const {

    // Can the fDateFormat parse it?
    if(fDateFormat != NULL) {
        ParsePosition aPos(pos);
        fDateFormat->parse(text,cal,aPos);
        if((aPos.getIndex() != pos.getIndex()) && 
            (aPos.getErrorIndex()==-1)) {
                pos=aPos; // copy the sub parse
                return; // parsed subfmt OK
        }
    }
    
    // Linear search the relative strings
    for(int n=0;n<fDatesLen;n++) {
        if(fDates[n].string != NULL &&
            (0==text.compare(pos.getIndex(),
                         fDates[n].len,
                         fDates[n].string))) {
            UErrorCode status = U_ZERO_ERROR;
            
            // Set the calendar to now+offset
            cal.setTime(Calendar::getNow(),status);
            cal.add(UCAL_DATE,fDates[n].offset, status);
            
            if(U_FAILURE(status)) { 
                // failure in setting calendar fields
                pos.setErrorIndex(pos.getIndex()+fDates[n].len);
            } else {
                pos.setIndex(pos.getIndex()+fDates[n].len);
            }
            return;
        }
    }
    
    // parse failed
}

UDate
RelativeDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    // redefined here because the other parse() function hides this function's
    // cunterpart on DateFormat
    return DateFormat::parse(text, pos);
}

UDate
RelativeDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    // redefined here because the other parse() function hides this function's
    // counterpart on DateFormat
    return DateFormat::parse(text, status);
}


const UChar *RelativeDateFormat::getStringForDay(int32_t day, int32_t &len, UErrorCode &status) const {
    if(U_FAILURE(status)) {
        return NULL;
    }
    
    // Is it outside the resource bundle's range?
    if(day < fDayMin || day > fDayMax) {
        return NULL; // don't have it.
    }
    
    // Linear search the held strings
    for(int n=0;n<fDatesLen;n++) {
        if(fDates[n].offset == day) {
            len = fDates[n].len;
            return fDates[n].string;
        }
    }
    
    return NULL;  // not found.
}

void RelativeDateFormat::loadDates(UErrorCode &status) {
    CalendarData calData(fLocale, "gregorian", status);
    UResourceBundle *strings = calData.getByKey3("fields", "day", "relative", status);
    // set up min/max 
    fDayMin=-1;
    fDayMax=1;

    if(U_FAILURE(status)) {
        fDatesLen=0;
        return;
    }

    fDatesLen = ures_getSize(strings);
    fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);

    // Load in each item into the array...
    int n = 0;

    UResourceBundle *subString = NULL;
    
    while(ures_hasNext(strings) && U_SUCCESS(status)) {  // iterate over items
        subString = ures_getNextResource(strings, subString, &status);
        
        if(U_FAILURE(status) || (subString==NULL)) break;
        
        // key = offset #
        const char *key = ures_getKey(subString);
        
        // load the string and length
        int32_t aLen;
        const UChar* aString = ures_getString(subString, &aLen, &status);
        
        if(U_FAILURE(status) || aString == NULL) break;

        // calculate the offset
        int32_t offset = atoi(key);
        
        // set min/max
        if(offset < fDayMin) {
            fDayMin = offset;
        }
        if(offset > fDayMax) {
            fDayMax = offset;
        }
        
        // copy the string pointer
        fDates[n].offset = offset;
        fDates[n].string = aString;
        fDates[n].len = aLen; 

        n++;
    }
    ures_close(subString);
    
    // the fDates[] array could be sorted here, for direct access.
}


// this should to be in DateFormat, instead it was copied from SimpleDateFormat.

Calendar*
RelativeDateFormat::initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status)
{
    if(!U_FAILURE(status)) {
        fCalendar = Calendar::createInstance(adoptZone?adoptZone:TimeZone::createDefault(), locale, status);
    }
    if (U_SUCCESS(status) && fCalendar == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return fCalendar;
}

int32_t RelativeDateFormat::dayDifference(Calendar &cal, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return 0;
    }
    // TODO: Cache the nowCal to avoid heap allocs?
    Calendar *nowCal = cal.clone();
    nowCal->setTime(Calendar::getNow(), status);
    int32_t dayDiff = nowCal->fieldDifference(cal.getTime(status), Calendar::DATE, status);
    delete nowCal;
    return dayDiff;
}

U_NAMESPACE_END

#endif

