/*
*******************************************************************************
* Copyright (C) 1997-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File FMTABLE.CPP
*
* Modification History:
*
*   Date        Name        Description
*   03/25/97    clhuang     Initial Implementation.
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"
#include "unicode/ustring.h"
#include "unicode/measure.h"
#include "unicode/curramt.h"
#include "cmemory.h"

// *****************************************************************************
// class Formattable
// *****************************************************************************

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Formattable)

//-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

// NOTE: As of 3.0, there are limitations to the UObject API.  It does
// not (yet) support cloning, operator=, nor operator==.  RTTI is also
// restricted in that subtype testing is not (yet) implemented.  To
// work around this, I implement some simple inlines here.  Later
// these can be modified or removed.  [alan]

// NOTE: These inlines assume that all fObjects are in fact instances
// of the Measure class, which is true as of 3.0.  [alan]

// Return TRUE if *a == *b.
static inline UBool objectEquals(const UObject* a, const UObject* b) {
    // LATER: return *a == *b;
    return *((const Measure*) a) == *((const Measure*) b);
}

// Return a clone of *a.
static inline UObject* objectClone(const UObject* a) {
    // LATER: return a->clone();
    return ((const Measure*) a)->clone();
}

// Return TRUE if *a is an instance of Measure.
static inline UBool instanceOfMeasure(const UObject* a) {
    // LATER: return a->instanceof(Measure::getStaticClassID());
    return a->getDynamicClassID() ==
        CurrencyAmount::getStaticClassID();
}

/**
 * Creates a new Formattable array and copies the values from the specified
 * original.
 * @param array the original array
 * @param count the original array count
 * @return the new Formattable array.
 */
static inline Formattable* createArrayCopy(const Formattable* array, int32_t count) {
    Formattable *result = new Formattable[count];
    for (int32_t i=0; i<count; ++i) result[i] = array[i]; // Don't memcpy!
    return result;
}

//-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

/**
 * Set 'ec' to 'err' only if 'ec' is not already set to a failing UErrorCode.
 */
static inline void setError(UErrorCode& ec, UErrorCode err) {
    if (U_SUCCESS(ec)) {
        ec = err;
    }
}

// -------------------------------------
// default constructor.
// Creates a formattable object with a long value 0.

Formattable::Formattable()
    :   UObject(), fType(kLong)
{
    fBogus.setToBogus();
    fValue.fInt64 = 0;
}

// -------------------------------------
// Creates a formattable object with a Date instance.

Formattable::Formattable(UDate date, ISDATE /*isDate*/)
    :   UObject(), fType(kDate)
{
    fBogus.setToBogus();
    fValue.fDate = date;
}

// -------------------------------------
// Creates a formattable object with a double value.

Formattable::Formattable(double value)
    :   UObject(), fType(kDouble)
{
    fBogus.setToBogus();
    fValue.fDouble = value;
}

// -------------------------------------
// Creates a formattable object with a long value.

Formattable::Formattable(int32_t value)
    :   UObject(), fType(kLong)
{
    fBogus.setToBogus();
    fValue.fInt64 = value;
}

// -------------------------------------
// Creates a formattable object with a long value.

Formattable::Formattable(int64_t value)
    :   UObject(), fType(kInt64)
{
    fBogus.setToBogus();
    fValue.fInt64 = value;
}

// -------------------------------------
// Creates a formattable object with a UnicodeString instance.

Formattable::Formattable(const UnicodeString& stringToCopy)
    :   UObject(), fType(kString)
{
    fBogus.setToBogus();
    fValue.fString = new UnicodeString(stringToCopy);
}

// -------------------------------------
// Creates a formattable object with a UnicodeString* value.
// (adopting symantics)

Formattable::Formattable(UnicodeString* stringToAdopt)
    :   UObject(), fType(kString)
{
    fBogus.setToBogus();
    fValue.fString = stringToAdopt;
}

Formattable::Formattable(UObject* objectToAdopt)
    :   UObject(), fType(kObject)
{
    fBogus.setToBogus();
    fValue.fObject = objectToAdopt;
}

// -------------------------------------

Formattable::Formattable(const Formattable* arrayToCopy, int32_t count)
    :   UObject(), fType(kArray)
{
    fBogus.setToBogus();
    fValue.fArrayAndCount.fArray = createArrayCopy(arrayToCopy, count);
    fValue.fArrayAndCount.fCount = count;
}

// -------------------------------------
// copy constructor

Formattable::Formattable(const Formattable &source)
    :   UObject(source), fType(kLong)
{
    fBogus.setToBogus();
    *this = source;
}

// -------------------------------------
// assignment operator

Formattable&
Formattable::operator=(const Formattable& source)
{
    if (this != &source)
    {
        // Disposes the current formattable value/setting.
        dispose();

        // Sets the correct data type for this value.
        fType = source.fType;
        switch (fType)
        {
        case kArray:
            // Sets each element in the array one by one and records the array count.
            fValue.fArrayAndCount.fCount = source.fValue.fArrayAndCount.fCount;
            fValue.fArrayAndCount.fArray = createArrayCopy(source.fValue.fArrayAndCount.fArray,
                                                           source.fValue.fArrayAndCount.fCount);
            break;
        case kString:
            // Sets the string value.
            fValue.fString = new UnicodeString(*source.fValue.fString);
            break;
        case kDouble:
            // Sets the double value.
            fValue.fDouble = source.fValue.fDouble;
            break;
        case kLong:
        case kInt64:
            // Sets the long value.
            fValue.fInt64 = source.fValue.fInt64;
            break;
        case kDate:
            // Sets the Date value.
            fValue.fDate = source.fValue.fDate;
            break;
        case kObject:
            fValue.fObject = objectClone(source.fValue.fObject);
            break;
        }
    }
    return *this;
}

// -------------------------------------

UBool
Formattable::operator==(const Formattable& that) const
{
    int32_t i;

    if (this == &that) return TRUE;

    // Returns FALSE if the data types are different.
    if (fType != that.fType) return FALSE;

    // Compares the actual data values.
    UBool equal = TRUE;
    switch (fType) {
    case kDate:
        equal = (fValue.fDate == that.fValue.fDate);
        break;
    case kDouble:
        equal = (fValue.fDouble == that.fValue.fDouble);
        break;
    case kLong:
    case kInt64:
        equal = (fValue.fInt64 == that.fValue.fInt64);
        break;
    case kString:
        equal = (*(fValue.fString) == *(that.fValue.fString));
        break;
    case kArray:
        if (fValue.fArrayAndCount.fCount != that.fValue.fArrayAndCount.fCount) {
            equal = FALSE;
            break;
        }
        // Checks each element for equality.
        for (i=0; i<fValue.fArrayAndCount.fCount; ++i) {
            if (fValue.fArrayAndCount.fArray[i] != that.fValue.fArrayAndCount.fArray[i]) {
                equal = FALSE;
                break;
            }
        }
        break;
    case kObject:
        equal = objectEquals(fValue.fObject, that.fValue.fObject);
        break;
    }

    return equal;
}

// -------------------------------------

Formattable::~Formattable()
{
    dispose();
}

// -------------------------------------

void Formattable::dispose()
{
    // Deletes the data value if necessary.
    switch (fType) {
    case kString:
        delete fValue.fString;
        break;
    case kArray:
        delete[] fValue.fArrayAndCount.fArray;
        break;
    case kObject:
        delete fValue.fObject;
        break;
    default:
        break;
    }
}

Formattable *
Formattable::clone() const {
    return new Formattable(*this);
}

// -------------------------------------
// Gets the data type of this Formattable object. 
Formattable::Type
Formattable::getType() const
{
    return fType;
}

UBool
Formattable::isNumeric() const {
    switch (fType) {
    case kDouble:
    case kLong:
    case kInt64:
        return TRUE;
    default:
        return FALSE;
    }
}

// -------------------------------------
int32_t
//Formattable::getLong(UErrorCode* status) const
Formattable::getLong(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
        return (int32_t)fValue.fInt64;
    case Formattable::kInt64: 
        if (fValue.fInt64 > INT32_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MAX;
        } else if (fValue.fInt64 < INT32_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MIN;
        } else {
            return (int32_t)fValue.fInt64;
        }
    case Formattable::kDouble:
        if (fValue.fDouble > INT32_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MAX;
        } else if (fValue.fDouble < INT32_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return INT32_MIN;
        } else {
            return (int32_t)fValue.fDouble; // loses fraction
        }
    case Formattable::kObject:
        // TODO Later replace this with instanceof call
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getLong(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}

// -------------------------------------
int64_t
Formattable::getInt64(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
    case Formattable::kInt64: 
        return fValue.fInt64;
    case Formattable::kDouble:
        if (fValue.fDouble > U_INT64_MAX) {
            status = U_INVALID_FORMAT_ERROR;
            return U_INT64_MAX;
        } else if (fValue.fDouble < U_INT64_MIN) {
            status = U_INVALID_FORMAT_ERROR;
            return U_INT64_MIN;
        } else {
            return (int64_t)fValue.fDouble;
        }
    case Formattable::kObject:
        // TODO Later replace this with instanceof call
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getInt64(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}

// -------------------------------------
double
Formattable::getDouble(UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return 0;
    }
        
    switch (fType) {
    case Formattable::kLong: 
    case Formattable::kInt64: // loses precision
        return (double)fValue.fInt64;
    case Formattable::kDouble:
        return fValue.fDouble;
    case Formattable::kObject:
        // TODO Later replace this with instanceof call
        if (instanceOfMeasure(fValue.fObject)) {
            return ((const Measure*) fValue.fObject)->
                getNumber().getDouble(status);
        }
    default: 
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }
}

const UObject*
Formattable::getObject() const {
    return (fType == kObject) ? fValue.fObject : NULL;
}

// -------------------------------------
// Sets the value to a double value d.

void
Formattable::setDouble(double d)
{
    dispose();
    fType = kDouble;
    fValue.fDouble = d;
}

// -------------------------------------
// Sets the value to a long value l.

void
Formattable::setLong(int32_t l)
{
    dispose();
    fType = kLong;
    fValue.fInt64 = l;
}

// -------------------------------------
// Sets the value to an int64 value ll.

void
Formattable::setInt64(int64_t ll)
{
    dispose();
    fType = kInt64;
    fValue.fInt64 = ll;
}

// -------------------------------------
// Sets the value to a Date instance d.

void
Formattable::setDate(UDate d)
{
    dispose();
    fType = kDate;
    fValue.fDate = d;
}

// -------------------------------------
// Sets the value to a string value stringToCopy.

void
Formattable::setString(const UnicodeString& stringToCopy)
{
    dispose();
    fType = kString;
    fValue.fString = new UnicodeString(stringToCopy);
}

// -------------------------------------
// Sets the value to an array of Formattable objects.

void
Formattable::setArray(const Formattable* array, int32_t count)
{
    dispose();
    fType = kArray;
    fValue.fArrayAndCount.fArray = createArrayCopy(array, count);
    fValue.fArrayAndCount.fCount = count;
}

// -------------------------------------
// Adopts the stringToAdopt value.

void
Formattable::adoptString(UnicodeString* stringToAdopt)
{
    dispose();
    fType = kString;
    fValue.fString = stringToAdopt;
}

// -------------------------------------
// Adopts the array value and its count.

void
Formattable::adoptArray(Formattable* array, int32_t count)
{
    dispose();
    fType = kArray;
    fValue.fArrayAndCount.fArray = array;
    fValue.fArrayAndCount.fCount = count;
}

void
Formattable::adoptObject(UObject* objectToAdopt) {
    dispose();
    fType = kObject;
    fValue.fObject = objectToAdopt;
}

// -------------------------------------
UnicodeString& 
Formattable::getString(UnicodeString& result, UErrorCode& status) const 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        result.setToBogus();
    } else {
        result = *fValue.fString;
    }
    return result;
}

// -------------------------------------
const UnicodeString& 
Formattable::getString(UErrorCode& status) const 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        return *getBogus();
    }
    return *fValue.fString;
}

// -------------------------------------
UnicodeString& 
Formattable::getString(UErrorCode& status) 
{
    if (fType != kString) {
        setError(status, U_INVALID_FORMAT_ERROR);
        return *getBogus();
    }
    return *fValue.fString;
}

// -------------------------------------
const Formattable* 
Formattable::getArray(int32_t& count, UErrorCode& status) const 
{
    if (fType != kArray) {
        setError(status, U_INVALID_FORMAT_ERROR);
        count = 0;
        return NULL;
    }
    count = fValue.fArrayAndCount.fCount; 
    return fValue.fArrayAndCount.fArray;
}

// -------------------------------------
// Gets the bogus string, ensures mondo bogosity.

UnicodeString*
Formattable::getBogus() const 
{
    return (UnicodeString*)&fBogus; /* cast away const :-( */
}

#if 0
//----------------------------------------------------
// console I/O
//----------------------------------------------------
#ifdef _DEBUG

#if U_IOSTREAM_SOURCE >= 199711
#include <iostream>
using namespace std;
#elif U_IOSTREAM_SOURCE >= 198506
#include <iostream.h>
#endif

#include "unicode/datefmt.h"
#include "unistrm.h"

class FormattableStreamer /* not : public UObject because all methods are static */ {
public:
    static void streamOut(ostream& stream, const Formattable& obj);

private:
    FormattableStreamer() {} // private - forbid instantiation
};

// This is for debugging purposes only.  This will send a displayable
// form of the Formattable object to the output stream.

void
FormattableStreamer::streamOut(ostream& stream, const Formattable& obj)
{
    static DateFormat *defDateFormat = 0;

    UnicodeString buffer;
    switch(obj.getType()) {
        case Formattable::kDate : 
            // Creates a DateFormat instance for formatting the
            // Date instance.
            if (defDateFormat == 0) {
                defDateFormat = DateFormat::createInstance();
            }
            defDateFormat->format(obj.getDate(), buffer);
            stream << buffer;
            break;
        case Formattable::kDouble :
            // Output the double as is.
            stream << obj.getDouble() << 'D';
            break;
        case Formattable::kLong :
            // Output the double as is.
            stream << obj.getLong() << 'L';
            break;
        case Formattable::kString:
            // Output the double as is.  Please see UnicodeString console
            // I/O routine for more details.
            stream << '"' << obj.getString(buffer) << '"';
            break;
        case Formattable::kArray:
            int32_t i, count;
            const Formattable* array;
            array = obj.getArray(count);
            stream << '[';
            // Recursively calling the console I/O routine for each element in the array.
            for (i=0; i<count; ++i) {
                FormattableStreamer::streamOut(stream, array[i]);
                stream << ( (i==(count-1)) ? "" : ", " );
            }
            stream << ']';
            break;
        default:
            // Not a recognizable Formattable object.
            stream << "INVALID_Formattable";
    }
    stream.flush();
}
#endif

#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
