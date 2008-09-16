/*
**********************************************************************
*   Copyright (C) 1997-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File DIGITLST.CPP
*
* Modification History:
*
*   Date        Name        Description
*   03/21/97    clhuang     Converted from java.
*   03/21/97    clhuang     Implemented with new APIs.
*   03/27/97    helena      Updated to pass the simple test after code review.
*   03/31/97    aliu        Moved isLONG_MIN to here, and fixed it.
*   04/15/97    aliu        Changed MAX_COUNT to DBL_DIG.  Changed Digit to char.
*                           Reworked representation by replacing fDecimalAt
*                           with fExponent.
*   04/16/97    aliu        Rewrote set() and getDouble() to use sprintf/atof
*                           to do digit conversion.
*   09/09/97    aliu        Modified for exponential notation support.
*   08/02/98    stephen     Added nearest/even rounding
*                            Fixed bug in fitsIntoLong
******************************************************************************
*/

#include "digitlst.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/putil.h"
#include "cstring.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

// ***************************************************************************
// class DigitList
// This class handles the transcoding between numeric values and strings of
//  characters.  Only handles as non-negative numbers.
// ***************************************************************************

/**
 * This is the zero digit.  Array elements fDigits[i] have values from
 * kZero to kZero + 9.  Typically, this is '0'.
 */
#define kZero '0'

static char gDecimal = 0;

/* Only for 32 bit numbers. Ignore the negative sign. */
static const char LONG_MIN_REP[] = "2147483648";
static const char I64_MIN_REP[] = "9223372036854775808";

enum {
    LONG_MIN_REP_LENGTH = sizeof(LONG_MIN_REP) - 1, //Ignore the NULL at the end
    I64_MIN_REP_LENGTH = sizeof(I64_MIN_REP) - 1 //Ignore the NULL at the end
};

U_NAMESPACE_BEGIN


// -------------------------------------
// default constructor

DigitList::DigitList()
{
    fDigits = fDecimalDigits + 1;   // skip the decimal
    clear();
}

// -------------------------------------

DigitList::~DigitList()
{
}

// -------------------------------------
// copy constructor

DigitList::DigitList(const DigitList &other)
{
    fDigits = fDecimalDigits + 1;   // skip the decimal
    *this = other;
}

// -------------------------------------
// assignment operator

DigitList&
DigitList::operator=(const DigitList& other)
{
    if (this != &other)
    {
        fDecimalAt = other.fDecimalAt;
        fCount = other.fCount;
        fIsPositive = other.fIsPositive;
        fRoundingMode = other.fRoundingMode;
        uprv_strncpy(fDigits, other.fDigits, fCount);
    }
    return *this;
}

// -------------------------------------

UBool
DigitList::operator==(const DigitList& that) const
{
    return ((this == &that) ||
            (fDecimalAt == that.fDecimalAt &&
             fCount == that.fCount &&
             fIsPositive == that.fIsPositive &&
             fRoundingMode == that.fRoundingMode &&
             uprv_strncmp(fDigits, that.fDigits, fCount) == 0));
}

// -------------------------------------
// Resets the digit list; sets all the digits to zero.

void
DigitList::clear()
{
    fDecimalAt = 0;
    fCount = 0;
    fIsPositive = TRUE;
    fRoundingMode = DecimalFormat::kRoundHalfEven;

    // Don't bother initializing fDigits because fCount is 0.
}



// -------------------------------------

/**
 * Formats a number into a base 10 string representation, and NULL terminates it.
 * @param number The number to format
 * @param outputStr The string to output to
 * @param outputLen The maximum number of characters to put into outputStr
 *                  (including NULL).
 * @return the number of digits written, not including the sign.
 */
static int32_t
formatBase10(int64_t number, char *outputStr, int32_t outputLen)
{
    char buffer[MAX_DIGITS + 1];
    int32_t bufferLen;
    int32_t result;

    if (outputLen > MAX_DIGITS) {
        outputLen = MAX_DIGITS;     // Ignore NULL
    }
    else if (outputLen < 3) {
        return 0;                   // Not enough room
    }

    bufferLen = outputLen;

    if (number < 0) {   // Negative numbers are slightly larger than a postive
        buffer[bufferLen--] = (char)(-(number % 10) + kZero);
        number /= -10;
        *(outputStr++) = '-';
    }
    else {
        *(outputStr++) = '+';    // allow +0
    }
    while (bufferLen >= 0 && number) {      // Output the number
        buffer[bufferLen--] = (char)(number % 10 + kZero);
        number /= 10;
    }

    result = outputLen - bufferLen++;

    while (bufferLen <= outputLen) {     // Copy the number to output
        *(outputStr++) = buffer[bufferLen++];
    }
    *outputStr = 0;   // NULL terminate.
    return result;
}

/**
 * Currently, getDouble() depends on atof() to do its conversion.
 *
 * WARNING!!
 * This is an extremely costly function. ~1/2 of the conversion time
 * can be linked to this function.
 */
double
DigitList::getDouble() /*const*/
{
    double value;

    if (fCount == 0) {
        value = 0.0;
    }
    else {
        char* end = NULL;
        if (!gDecimal) {
            char rep[MAX_DIGITS];
            // For machines that decide to change the decimal on you,
            // and try to be too smart with localization.
            // This normally should be just a '.'.
            sprintf(rep, "%+1.1f", 1.0);
            gDecimal = rep[2];
        }

        *fDecimalDigits = gDecimal;
        *(fDigits+fCount) = 'e';    // add an e after the digits.
        formatBase10(fDecimalAt,
                     fDigits + fCount + 1,  // skip the 'e'
                     MAX_DEC_DIGITS - fCount - 3);  // skip the 'e' and '.'
        value = uprv_strtod(fDecimalDigits, &end);
    }

    return fIsPositive ? value : -value;
}

// -------------------------------------

/**
 * Make sure that fitsIntoLong() is called before calling this function.
 */
int32_t DigitList::getLong() /*const*/
{
    if (fCount == fDecimalAt) {
        int32_t value;

        fDigits[fCount] = 0;    // NULL terminate

        // This conversion is bad on 64-bit platforms when we want to
        // be able to return a 64-bit number [grhoten]
        *fDecimalDigits = fIsPositive ? '+' : '-';
        value = (int32_t)atol(fDecimalDigits);
        return value;
    }
    else {
        // This is 100% accurate in c++ because if we are representing
        // an integral value, we suffer nothing in the conversion to
        // double.  If we are to support 64-bit longs later, getLong()
        // must be rewritten. [LIU]
        return (int32_t)getDouble();
    }
}


/**
 * Make sure that fitsIntoInt64() is called before calling this function.
 */
int64_t DigitList::getInt64() /*const*/
{
    if (fCount == fDecimalAt) {
        uint64_t value;

        fDigits[fCount] = 0;    // NULL terminate

        // This conversion is bad on 64-bit platforms when we want to
        // be able to return a 64-bit number [grhoten]
        *fDecimalDigits = fIsPositive ? '+' : '-';

        // emulate a platform independent atoi64()
        value = 0;
        for (int i = 0; i < fCount; ++i) {
            int v = fDigits[i] - kZero;
            value = value * (uint64_t)10 + (uint64_t)v;
        }
        if (!fIsPositive) {
            value = ~value;
            value += 1;
        }
        int64_t svalue = (int64_t)value;
        return svalue;
    }
    else {
        // TODO: figure out best approach

        // This is 100% accurate in c++ because if we are representing
        // an integral value, we suffer nothing in the conversion to
        // double.  If we are to support 64-bit longs later, getLong()
        // must be rewritten. [LIU]
        return (int64_t)getDouble();
    }
}

/**
 * Return true if the number represented by this object can fit into
 * a long.
 */
UBool
DigitList::fitsIntoLong(UBool ignoreNegativeZero) /*const*/
{
    // Figure out if the result will fit in a long.  We have to
    // first look for nonzero digits after the decimal point;
    // then check the size.

    // Trim trailing zeros after the decimal point. This does not change
    // the represented value.
    while (fCount > fDecimalAt && fCount > 0 && fDigits[fCount - 1] == kZero)
        --fCount;

    if (fCount == 0) {
        // Positive zero fits into a long, but negative zero can only
        // be represented as a double. - bug 4162852
        return fIsPositive || ignoreNegativeZero;
    }

    // If the digit list represents a double or this number is too
    // big for a long.
    if (fDecimalAt < fCount || fDecimalAt > LONG_MIN_REP_LENGTH)
        return FALSE;

    // If number is small enough to fit in a long
    if (fDecimalAt < LONG_MIN_REP_LENGTH)
        return TRUE;

    // At this point we have fDecimalAt == fCount, and fCount == LONG_MIN_REP_LENGTH.
    // The number will overflow if it is larger than LONG_MAX
    // or smaller than LONG_MIN.
    for (int32_t i=0; i<fCount; ++i)
    {
        char dig = fDigits[i],
             max = LONG_MIN_REP[i];
        if (dig > max)
            return FALSE;
        if (dig < max)
            return TRUE;
    }

    // At this point the first count digits match.  If fDecimalAt is less
    // than count, then the remaining digits are zero, and we return true.
    if (fCount < fDecimalAt)
        return TRUE;

    // Now we have a representation of Long.MIN_VALUE, without the leading
    // negative sign.  If this represents a positive value, then it does
    // not fit; otherwise it fits.
    return !fIsPositive;
}

/**
 * Return true if the number represented by this object can fit into
 * a long.
 */
UBool
DigitList::fitsIntoInt64(UBool ignoreNegativeZero) /*const*/
{
    // Figure out if the result will fit in a long.  We have to
    // first look for nonzero digits after the decimal point;
    // then check the size.

    // Trim trailing zeros after the decimal point. This does not change
    // the represented value.
    while (fCount > fDecimalAt && fCount > 0 && fDigits[fCount - 1] == kZero)
        --fCount;

    if (fCount == 0) {
        // Positive zero fits into a long, but negative zero can only
        // be represented as a double. - bug 4162852
        return fIsPositive || ignoreNegativeZero;
    }

    // If the digit list represents a double or this number is too
    // big for a long.
    if (fDecimalAt < fCount || fDecimalAt > I64_MIN_REP_LENGTH)
        return FALSE;

    // If number is small enough to fit in an int64
    if (fDecimalAt < I64_MIN_REP_LENGTH)
        return TRUE;

    // At this point we have fDecimalAt == fCount, and fCount == INT64_MIN_REP_LENGTH.
    // The number will overflow if it is larger than U_INT64_MAX
    // or smaller than U_INT64_MIN.
    for (int32_t i=0; i<fCount; ++i)
    {
        char dig = fDigits[i],
             max = I64_MIN_REP[i];
        if (dig > max)
            return FALSE;
        if (dig < max)
            return TRUE;
    }

    // At this point the first count digits match.  If fDecimalAt is less
    // than count, then the remaining digits are zero, and we return true.
    if (fCount < fDecimalAt)
        return TRUE;

    // Now we have a representation of INT64_MIN_VALUE, without the leading
    // negative sign.  If this represents a positive value, then it does
    // not fit; otherwise it fits.
    return !fIsPositive;
}


// -------------------------------------

void
DigitList::set(int32_t source, int32_t maximumDigits)
{
    set((int64_t)source, maximumDigits);
}

// -------------------------------------
/**
 * @param maximumDigits The maximum digits to be generated.  If zero,
 * there is no maximum -- generate all digits.
 */
void
DigitList::set(int64_t source, int32_t maximumDigits)
{
    fCount = fDecimalAt = formatBase10(source, fDecimalDigits, MAX_DIGITS);

    fIsPositive = (*fDecimalDigits == '+');

    // Don't copy trailing zeros
    while (fCount > 1 && fDigits[fCount - 1] == kZero)
        --fCount;

    if(maximumDigits > 0)
        round(maximumDigits);
}

/**
 * Set the digit list to a representation of the given double value.
 * This method supports both fixed-point and exponential notation.
 * @param source Value to be converted; must not be Inf, -Inf, Nan,
 * or a value <= 0.
 * @param maximumDigits The most fractional or total digits which should
 * be converted.  If total digits, and the value is zero, then
 * there is no maximum -- generate all digits.
 * @param fixedPoint If true, then maximumDigits is the maximum
 * fractional digits to be converted.  If false, total digits.
 */
void
DigitList::set(double source, int32_t maximumDigits, UBool fixedPoint)
{
    // for now, simple implementation; later, do proper IEEE stuff
    char rep[MAX_DIGITS + 8]; // Extra space for '+', '.', e+NNN, and '\0' (actually +8 is enough)
    char *digitPtr      = fDigits;
    char *repPtr        = rep + 2;  // +2 to skip the sign and decimal
    int32_t exponent    = 0;

    fIsPositive = !uprv_isNegative(source);    // Allow +0 and -0

    // Generate a representation of the form /[+-][0-9]+e[+-][0-9]+/
    sprintf(rep, "%+1.*e", MAX_DBL_DIGITS - 1, source);
    fDecimalAt  = 0;
    rep[2]      = rep[1];    // remove decimal

    while (*repPtr == kZero) {
        repPtr++;
        fDecimalAt--;   // account for leading zeros
    }

    while (*repPtr != 'e') {
        *(digitPtr++) = *(repPtr++);
    }
    fCount = MAX_DBL_DIGITS + fDecimalAt;

    // Parse an exponent of the form /[eE][+-][0-9]+/
    UBool negExp = (*(++repPtr) == '-');
    while (*(++repPtr) != 0) {
        exponent = 10*exponent + *repPtr - kZero;
    }
    if (negExp) {
        exponent = -exponent;
    }
    fDecimalAt += exponent + 1; // +1 for decimal removal

    // The negative of the exponent represents the number of leading
    // zeros between the decimal and the first non-zero digit, for
    // a value < 0.1 (e.g., for 0.00123, -decimalAt == 2).  If this
    // is more than the maximum fraction digits, then we have an underflow
    // for the printed representation.
    if (fixedPoint && -fDecimalAt >= maximumDigits)
    {
        // If we round 0.0009 to 3 fractional digits, then we have to
        // create a new one digit in the least significant location.
        if (-fDecimalAt == maximumDigits && shouldRoundUp(0)) {
            fCount = 1;
            ++fDecimalAt;
            fDigits[0] = (char)'1';
        } else {
            // Handle an underflow to zero when we round something like
            // 0.0009 to 2 fractional digits.
            fCount = 0;
        }
        return;
    }


    // Eliminate digits beyond maximum digits to be displayed.
    // Round up if appropriate.  Do NOT round in the special
    // case where maximumDigits == 0 and fixedPoint is FALSE.
    if (fixedPoint || (0 < maximumDigits && maximumDigits < fCount)) {
        round(fixedPoint ? (maximumDigits + fDecimalAt) : maximumDigits);
    }
    else {
        // Eliminate trailing zeros.
        while (fCount > 1 && fDigits[fCount - 1] == kZero)
            --fCount;
    }
}

// -------------------------------------

/**
 * Round the representation to the given number of digits.
 * @param maximumDigits The maximum number of digits to be shown.
 * Upon return, count will be less than or equal to maximumDigits.
 */
void
DigitList::round(int32_t maximumDigits)
{
    // Eliminate digits beyond maximum digits to be displayed.
    // Round up if appropriate.
    if (maximumDigits >= 0 && maximumDigits < fCount)
    {
        if (shouldRoundUp(maximumDigits)) {
            // Rounding up involved incrementing digits from LSD to MSD.
            // In most cases this is simple, but in a worst case situation
            // (9999..99) we have to adjust the decimalAt value.
            while (--maximumDigits >= 0 && ++fDigits[maximumDigits] > '9')
                ;

            if (maximumDigits < 0)
            {
                // We have all 9's, so we increment to a single digit
                // of one and adjust the exponent.
                fDigits[0] = (char) '1';
                ++fDecimalAt;
                maximumDigits = 1; // Adjust the count
            }
            else
            {
                ++maximumDigits; // Increment for use as count
            }
        }
        fCount = maximumDigits;
    }

    // Eliminate trailing zeros.
    while (fCount > 1 && fDigits[fCount-1] == kZero) {
        --fCount;
    }
}

/**
 * Return true if truncating the representation to the given number
 * of digits will result in an increment to the last digit.  This
 * method implements the requested rounding mode.
 * [bnf]
 * @param maximumDigits the number of digits to keep, from 0 to
 * <code>count-1</code>.  If 0, then all digits are rounded away, and
 * this method returns true if a one should be generated (e.g., formatting
 * 0.09 with "#.#").
 * @return true if digit <code>maximumDigits-1</code> should be
 * incremented
 */
UBool DigitList::shouldRoundUp(int32_t maximumDigits) const {
    int i = 0;
    if (fRoundingMode == DecimalFormat::kRoundDown ||
        fRoundingMode == DecimalFormat::kRoundFloor   &&  fIsPositive ||
        fRoundingMode == DecimalFormat::kRoundCeiling && !fIsPositive) {
        return FALSE;
    }

    if (fRoundingMode == DecimalFormat::kRoundHalfEven ||
        fRoundingMode == DecimalFormat::kRoundHalfDown ||
        fRoundingMode == DecimalFormat::kRoundHalfUp) {
        if (fDigits[maximumDigits] == '5' ) {
            for (i=maximumDigits+1; i<fCount; ++i) {
                if (fDigits[i] != kZero) {
                    return TRUE;
                }
            }
            switch (fRoundingMode) {
            case DecimalFormat::kRoundHalfEven:
            default:
                // Implement IEEE half-even rounding
                return maximumDigits > 0 && (fDigits[maximumDigits-1] % 2 != 0);
            case DecimalFormat::kRoundHalfDown:
                return FALSE;
            case DecimalFormat::kRoundHalfUp:
                return TRUE;
            }
        }
        return (fDigits[maximumDigits] > '5');
    }

    U_ASSERT(fRoundingMode == DecimalFormat::kRoundUp ||
             fRoundingMode == DecimalFormat::kRoundFloor   && !fIsPositive ||
             fRoundingMode == DecimalFormat::kRoundCeiling &&  fIsPositive);

     for (i=maximumDigits; i<fCount; ++i) {
         if (fDigits[i] != kZero) {
             return TRUE;
         }
     }
     return false;
}

// -------------------------------------

// In the Java implementation, we need a separate set(long) because 64-bit longs
// have too much precision to fit into a 64-bit double.  In C++, longs can just
// be passed to set(double) as long as they are 32 bits in size.  We currently
// don't implement 64-bit longs in C++, although the code below would work for
// that with slight modifications. [LIU]
/*
void
DigitList::set(long source)
{
    // handle the special case of zero using a standard exponent of 0.
    // mathematically, the exponent can be any value.
    if (source == 0)
    {
        fcount = 0;
        fDecimalAt = 0;
        return;
    }

    // we don't accept negative numbers, with the exception of long_min.
    // long_min is treated specially by being represented as long_max+1,
    // which is actually an impossible signed long value, so there is no
    // ambiguity.  we do this for convenience, so digitlist can easily
    // represent the digits of a long.
    bool islongmin = (source == long_min);
    if (islongmin)
    {
        source = -(source + 1); // that is, long_max
        islongmin = true;
    }
    sprintf(fdigits, "%d", source);

    // now we need to compute the exponent.  it's easy in this case; it's
    // just the same as the count.  e.g., 0.123 * 10^3 = 123.
    fcount = strlen(fdigits);
    fDecimalAt = fcount;

    // here's how we represent long_max + 1.  note that we always know
    // that the last digit of long_max will not be 9, because long_max
    // is of the form (2^n)-1.
    if (islongmin)
        ++fdigits[fcount-1];

    // finally, we trim off trailing zeros.  we don't alter fDecimalAt,
    // so this has no effect on the represented value.  we know the first
    // digit is non-zero (see code above), so we only have to check down
    // to fdigits[1].
    while (fcount > 1 && fdigits[fcount-1] == kzero)
        --fcount;
}
*/

/**
 * Return true if this object represents the value zero.  Anything with
 * no digits, or all zero digits, is zero, regardless of fDecimalAt.
 */
UBool
DigitList::isZero() const
{
    for (int32_t i=0; i<fCount; ++i)
        if (fDigits[i] != kZero)
            return FALSE;
    return TRUE;
}

U_NAMESPACE_END
#endif // #if !UCONFIG_NO_FORMATTING

//eof
