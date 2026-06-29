//
// cvt.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions for formatting floating point values with the %a, %e, %f, and %g
// printf format specifiers.
//
#include <corecrt_internal.h>
#include <corecrt_internal_ptd_propagation.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <corecrt_internal_fltintrn.h>
#include <fenv.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

// The C String pointed to by string is shifted distance bytes to the right.
// If distance is negative, the string is shifted to the left.
// The C String pointed to by string and all shifting operations must be
// contained within buffer_base or buffer_count.
static void __cdecl shift_bytes(
    _Maybe_unsafe_(_Inout_updates_z_, buffer_count)          char * const buffer_base,
    _In_                                                     size_t const buffer_count,
    _In_range_(buffer_base, buffer_base + buffer_count)      char*  const string,
    _In_                                                     int    const distance
    ) throw()
{
    UNREFERENCED_PARAMETER(buffer_base);
    UNREFERENCED_PARAMETER(buffer_count);

    if (distance != 0)
    {
        memmove(string + distance, string, strlen(string) + 1);
    }
}


//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// NaN and Infinity Formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_Success_(return == 0)
static errno_t __cdecl fp_format_nan_or_infinity(
    _In_                                                __acrt_fp_class const classification,
    _In_range_(0,1)                                     bool            const is_negative,
    _Maybe_unsafe_(_Out_writes_z_, result_buffer_count) char*                 result_buffer,
    _In_range_(1,SIZE_MAX)                              size_t                result_buffer_count,
    _In_range_(0,1)                                     bool            const use_capitals
    ) throw()
{
    using floating_traits = __acrt_floating_type_traits<double>;
    using components_type = floating_traits::components_type;

    // Ensure that we have sufficient space to store at least the basic three-
    // character INF or NAN string, plus the minus sign if required:
    if (result_buffer_count < _countof("INF") + is_negative)
    {
        *result_buffer = '\0';
        return ENOMEM;
    }

    if (is_negative)
    {
        *result_buffer++ = '-';
        *result_buffer   = '\0';
        if (result_buffer_count != _CRT_UNBOUNDED_BUFFER_SIZE)
        {
            --result_buffer_count;
        }
    }

    static char const* const strings[][4] =
    {
        { "INF",       "INF", "inf",       "inf" }, // Infinity
        { "NAN",       "NAN", "nan",       "nan" }, // Quiet NAN
        { "NAN(SNAN)", "NAN", "nan(snan)", "nan" }, // Signaling NAN
        { "NAN(IND)",  "NAN", "nan(ind)",  "nan" }, // Indeterminate
    };

    uint32_t const row    = static_cast<uint32_t>(classification) - 1;
    uint32_t const column = use_capitals ? 0 : 2;

    bool const long_string_will_fit = result_buffer_count > strlen(strings[row][column]);
    _ERRCHECK(strcpy_s(
        result_buffer,
        result_buffer_count,
        strings[row][column + !long_string_will_fit]));
    return 0;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// %e formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions handle the formatting of floating point values in the %e
// printf format.  This format has the form [-]d.ddde(+/-)ddd, where there will
// be 'precision' digits following the decimal point.  If the precision is less
// than or equal to zero, no decimal point will appear.  The low order digit is
// rounded.  If 'capitals' is true, then the exponent will appear as E(+/-)ddd.
_Success_(return == 0)
static errno_t fp_format_e_internal(
    _Maybe_unsafe_(_Inout_updates_z_, result_buffer_count) char*              const result_buffer,
    _In_fits_precision_(precision)                         size_t             const result_buffer_count,
    _In_                                                   int                const precision,
    _In_                                                   bool               const capitals,
    _In_                                                   unsigned           const min_exponent_digits,
    _In_                                                   STRFLT             const pflt,
    _In_                                                   bool               const g_fmt,
    _Inout_                                                __crt_cached_ptd_host&   ptd
    ) throw()
{
    // The max length if calculated like this:
    // 3 = sign + first digit + decimal point
    // precision = decimal digits
    // 5 = exponent letter (e or E), exponent sign, three digits exponent
    // 1 = extra space for rounding
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, result_buffer_count > static_cast<size_t>(3 + (precision > 0 ? precision : 0) + 5 + 1), ERANGE);

    // Place the output in the buffer and round.  Leave space in the buffer
    // for the '-' sign (if any) and the decimal point (if any):
    if (g_fmt)
    {
        // Shift it right one place if nec. for decimal point:
        char* const p = result_buffer + (pflt->sign == '-');
        shift_bytes(result_buffer, result_buffer_count, p, precision > 0);
    }

    // Now fix the number up to be in e format:
    char* p = result_buffer;

    // Put in negative sign if needed:
    if (pflt->sign == '-')
        *p++ = '-';

    // Put in decimal point if needed.  Copy the first digit to the place left
    // for it and put the decimal point in its place:
    if (precision > 0)
    {
        *p = *(p + 1);
        *++p = *ptd.get_locale()->locinfo->lconv->decimal_point;
    }

    // Find the end of the string, attach the exponent field and save the
    // exponent position:
    p = p + precision + (g_fmt ? 0 : 1);
    _ERRCHECK(strcpy_s(
        p,
        result_buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE
            ? result_buffer_count
            : result_buffer_count - (p - result_buffer), "e+000"));
    char* exponentpos = p + 2;

    // Adjust exponent indicator according to capitals flag and increment
    // pointer to point to exponent sign:
    if (capitals)
        *p = 'E';

    ++p;

    // If mantissa is zero, then the number is 0 and we are done; otherwise
    // adjust the exponent sign (if necessary) and value:
    if (*pflt->mantissa != '0')
    {
        // Check to see if exponent is negative; if so adjust exponent sign and
        // exponent value:
        int exp = pflt->decpt - 1;
        if (exp < 0)
        {
            exp = -exp;
            *p = '-';
        }

        ++p;

        if (exp >= 100)
        {
            *p += static_cast<char>(exp / 100);
            exp %= 100;
        }

        ++p;

        if (exp >= 10)
        {
            *p += static_cast<char>(exp / 10);
            exp %= 10;
        }

        *++p += static_cast<char>(exp);
    }

    if (min_exponent_digits == 2)
    {
        // If possible, reduce the exponent to two digits:
        if (*exponentpos == '0')
        {
            memmove(exponentpos, exponentpos + 1, 3);
        }
    }

    return 0;
}

_Success_(return == 0)
static errno_t __cdecl fp_format_e(
    _In_                                                   double const*        const argument,
    _Maybe_unsafe_(_Inout_updates_z_, result_buffer_count) char*                const result_buffer,
    _In_fits_precision_(precision)                         size_t               const result_buffer_count,
    _Out_writes_(scratch_buffer_count)                     char*                const scratch_buffer,
    _In_                                                   size_t               const scratch_buffer_count,
    _In_                                                   int                  const precision,
    _In_                                                   bool                 const capitals,
    _In_                                                   unsigned             const min_exponent_digits,
    _In_                                                   __acrt_rounding_mode const rounding_mode,
    _Inout_                                                __crt_cached_ptd_host&     ptd
    ) throw()
{
    // The precision passed to __acrt_fltout is the number of fractional digits.
    // To ensure that we get enough digits, we require a total of precision + 1 digits,
    // to account for the digit placed to the left of the decimal point when all digits are fractional.
    _strflt strflt;

    __acrt_has_trailing_digits const trailing_digits = __acrt_fltout(
        *reinterpret_cast<_CRT_DOUBLE const*>(argument),
        precision + 1,
        __acrt_precision_style::scientific,
        &strflt,
        scratch_buffer,
        scratch_buffer_count
        );

    errno_t const e = __acrt_fp_strflt_to_string(
        result_buffer + (strflt.sign == '-') + (precision > 0),
        (result_buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE
            ? result_buffer_count
            : result_buffer_count - (strflt.sign == '-') - (precision > 0)),
        precision + 1,
        &strflt,
        trailing_digits,
        rounding_mode,
        ptd);

    if (e != 0)
    {
        result_buffer[0] = '\0';
        return e;
    }

    return fp_format_e_internal(result_buffer, result_buffer_count, precision, capitals, min_exponent_digits, &strflt, false, ptd);
}

static bool fe_to_nearest(double const* const argument, unsigned __int64 const mask, short const maskpos)
{
    using floating_traits = __acrt_floating_type_traits<double>;
    using components_type = floating_traits::components_type;
    components_type const* const components = reinterpret_cast<components_type const*>(argument);

    unsigned short digit = static_cast<unsigned short>((components->_mantissa & mask) >> maskpos);

    if (digit > 8)
    {
        return true;
    }

    if (digit < 8)
    {
        return false;
    }

    unsigned __int64 const roundBitsMask = (static_cast<unsigned __int64>(1) << maskpos) - 1;
    if (components->_mantissa & roundBitsMask)
    {
        return true;
    }

    //if we still have digits to the left to compare
    if (maskpos != DBL_MANT_DIG - 5)
    {
        // We divide the mantisa by 16 to move the digits to the right, after that we apply the mask
        // to get the digit at the left.
        digit = static_cast<unsigned short>(((components->_mantissa / 16) & mask) >> maskpos);
    }
    else
    {
        digit = components->_exponent == 0 ? 0 : 1;
    }

    return digit % 2 == 1;
}

static bool should_round_up(double const* const argument, unsigned __int64 const mask, short const maskpos, __acrt_rounding_mode const rounding_mode)
{
    using floating_traits = __acrt_floating_type_traits<double>;
    using components_type = floating_traits::components_type;
    components_type const* const components = reinterpret_cast<components_type const*>(argument);

    unsigned short const digit = static_cast<unsigned short>((components->_mantissa & mask) >> maskpos);

    if (rounding_mode == __acrt_rounding_mode::legacy)
    {
        return digit >= 8;
    }
    int const round_mode = fegetround();

    if (round_mode == FE_TONEAREST)
    {
        return fe_to_nearest(argument, mask, maskpos);
    }

    if (round_mode == FE_UPWARD)
    {
        return digit != 0 && !components->_sign;
    }

    if (round_mode == FE_DOWNWARD)
    {
        return digit != 0 && components->_sign;
    }

    return false;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// %a formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions handle the formatting of floating point values in the %a
// printf format.  This format has the form [-]0xh.hhhhp(+/-)d, where there will
// be 'precision' hex digits following the decimal point.  If the precision is
// less than or equal to zero, no decimal point will appear.  If 'capitals' is
// true, then the number will appear as [-]0xH.HHHHP(+/-)d.
_Success_(return == 0)
static errno_t __cdecl fp_format_a(
    _In_                                                   double const*        const argument,
    _Maybe_unsafe_(_Inout_updates_z_, result_buffer_count) char*                      result_buffer,
    _In_fits_precision_(precision)                         size_t               const result_buffer_count,
    _Out_writes_(scratch_buffer_count)                     char*                const scratch_buffer,
    _In_                                                   size_t               const scratch_buffer_count,
    _In_                                                   int                        precision,
    _In_                                                   bool                 const capitals,
    _In_                                                   unsigned             const min_exponent_digits,
    _In_                                                   __acrt_rounding_mode const rounding_mode,
    _Inout_                                                __crt_cached_ptd_host&     ptd
    )
{
    using floating_traits = __acrt_floating_type_traits<double>;
    using components_type = floating_traits::components_type;

    if (precision < 0)
    {
        precision = 0;
    }

    result_buffer[0] = '\0';

    // the constraint for the size of buffer is:
    // 1 (sign)
    // 4 ("0xh.")
    // precision (decimal digits)
    // 6 ("p+0000")
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, result_buffer_count > static_cast<size_t>(1 + 4 + precision + 6), ERANGE);

    // Let __acrt_fp_format_e handle the special cases like SNAN, etc.:
    components_type const* const components = reinterpret_cast<components_type const*>(argument);
    if (components->_exponent == floating_traits::exponent_mask)
    {
        errno_t const e = fp_format_e(
            argument,
            result_buffer,
            result_buffer_count,
            scratch_buffer,
            scratch_buffer_count,
            precision,
            false,
            min_exponent_digits,
            rounding_mode,
            ptd);

        if (e != 0)
        {
            // An error occurred
            result_buffer[0] = '\0';
            return e;
        }

        // Substitute the e with p:
        char* p = strrchr(result_buffer, 'e');
        if (p)
        {
            *p = capitals ? 'P' : 'p';

            // Trim the exponent (which is 0) to only one digit; skip the
            // exponent sign and the first digit and put the terminating 0:
            p += 3;
            *p = 0;
        }
        return e;
    }

    // Sign:
    if (components->_sign)
    {
        *result_buffer++ = '-';
    }

    int const hexadd = (capitals ? 'A' : 'a') - '9' - 1;

    // Leading digit (and set the debias):
    unsigned __int64 debias = floating_traits::exponent_bias;
    if (components->_exponent == 0)
    {
        *result_buffer++ = '0';
        if (components->_mantissa == 0)
        {
            // Zero:
            debias = 0;
        }
        else
        {
            // Denormal:
            debias--;
        }
    }
    else
    {
        *result_buffer++ = '1';
    }

    // Decimal point (save the position in pos):
    char* pos = result_buffer++;
    if (precision == 0)
    {
        // If precision is 0, then we don't have to print the decimal point:
        // we mark this putting 0 instead of the decimal point itself
        *pos = 0;
    }
    else
    {
        *pos = *ptd.get_locale()->locinfo->lconv->decimal_point;
    }

    // Mantissa:
    if (components->_mantissa > 0)
    {
        // Print 4 bits at a time, and skip the initial zeroes
        // Prepare the mask to read the first 4 bits
        short maskpos = (floating_traits::mantissa_bits - 1) - 4;

        unsigned __int64 mask = 0xf;
        mask <<= maskpos;

        while (maskpos >= 0 && precision > 0)
        {
            unsigned short digit = static_cast<unsigned short>((components->_mantissa & mask) >> maskpos);
            digit += '0';
            if (digit > '9')
            {
                digit += static_cast<unsigned short>(hexadd);
            }
            *result_buffer++ = static_cast<char>(digit);
            mask >>= 4;
            maskpos -= 4;
            --precision;
        }

        // Round the mantissa:
        if (maskpos >= 0)
        {
            if (should_round_up(argument, mask, maskpos, rounding_mode))
            {
                char* p = result_buffer;
                --p;
                // If the last digit is 'f', we need to add one to the previous
                // digit, too; pos is the position of the decimal point
                while (*p == 'f' || *p == 'F')
                {
                    *p-- = '0';
                }
                // If we reached the decimal point, it means we are rounding
                // something like 0x0.fffff so this will become 0x1.00000 :
                if (p != pos)
                {
                    if (*p == '9')
                    {
                        *p += static_cast<char>(1 + hexadd);
                    }
                    else
                    {
                        *p += 1;
                    }
                }
                else // p == pos
                {
                    // Skip the decimal point:
                    --p;

                    // The first digit is always 0 or 1, so we don't need to
                    // add hexadd:
                    *p += 1;
                }
            }
        }
    }

    // Add the final zeroes, if needed:
    for (; precision > 0; --precision)
    {
        *result_buffer++ = '0';
    }

    // Move back the buffer pointer if there is no decimal point:
    if (*pos == 0)
    {
        result_buffer = pos;
    }

    // Exponent:
    *result_buffer++ = capitals ? 'P' : 'p';
    __int64 exponent = components->_exponent - debias;
    if (exponent >= 0)
    {
        *result_buffer++ = '+';
    }
    else
    {
        *result_buffer++ = '-';
        exponent = -exponent;
    }
    // Save the position in pos and write a '0':
    pos = result_buffer;
    *pos = '0';

    if (exponent >= 1000)
    {
        *result_buffer++ = '0' + static_cast<char>(exponent / 1000);
        exponent %= 1000;
    }
    if (result_buffer != pos || exponent >= 100)
    {
        *result_buffer++ = '0' + static_cast<char>(exponent / 100);
        exponent %= 100;
    }
    if (result_buffer != pos || exponent >= 10)
    {
        *result_buffer++ = '0' + static_cast<char>(exponent / 10);
        exponent %= 10;
    }

    *result_buffer++ = '0' + static_cast<char>(exponent);

    // Terminate the string:
    *result_buffer = '\0';

    return 0;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// %f formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions handle the formatting of floating point values in the %f
// printf format.  This format has the form [-]ddddd.ddddd, where there will be
// precision digits following the decimal point.  If precision is less than or
// equal to zero, no decimal point will appear.  The low order digit is rounded.
_Success_(return == 0)
static errno_t fp_format_f_internal(
    _Pre_z_ _Maybe_unsafe_(_Inout_updates_z_, buffer_count) char*              const buffer,
    _In_fits_precision_(precision)                          size_t             const buffer_count,
    _In_                                                    int                const precision,
    _In_                                                    STRFLT             const pflt,
    _In_                                                    bool               const g_fmt,
    _Inout_                                                 __crt_cached_ptd_host&   ptd
    ) throw()
{
    int const g_magnitude = pflt->decpt - 1;

    // Place the output in the user's buffer and round.  Save space for the
    // minus sign now if it will be needed:
    if (g_fmt && g_magnitude == precision)
    {
        char* const p = g_magnitude + buffer + (pflt->sign == '-');
        p[0] = '0';
        p[1] = '\0';

        // Allows for extra place-holding '0' in the exponent == precision case
        // of the %g format.
    }

    // Now fix up the number to be in the correct %f format:
    char* p = buffer;

    // Put in a negative sign, if necessary:
    if (pflt->sign == '-')
        *p++ = '-';

    // Insert a leading zero for purely fractional values and position ourselves
    // at the correct spot for inserting the decimal point:
    if (pflt->decpt <= 0)
    {
        // In the specific scenario of the 0 when using g format this would cause
        // to have an extra zero at the end
        bool const is_zero_pflt = pflt->decpt == 0 && *pflt->mantissa == '0';
        if (!g_fmt || !is_zero_pflt) {
            shift_bytes(buffer, buffer_count, p, 1);
        }
        *p++ = '0';
    }
    else
    {
        p += pflt->decpt;
    }

    // Put a decimal point if required, and add any needed zero padding:
    if (precision > 0)
    {
        shift_bytes(buffer, buffer_count, p, 1);
        *p++ = *ptd.get_locale()->locinfo->lconv->decimal_point;

        // If the value is less than 1 then we may need to put zeroes out in
        // front of the first non-zero digit of the mantissa:
        if (pflt->decpt < 0)
        {
            int const computed_precision = (g_fmt || -pflt->decpt < precision)
                ? -pflt->decpt
                : precision;

            shift_bytes(buffer, buffer_count, p, computed_precision);
            memset(p, '0', computed_precision);
        }
    }

    return 0;
}

_Success_(return == 0)
static errno_t __cdecl fp_format_f(
    _In_                                                   double const*        const argument,
    _Maybe_unsafe_(_Inout_updates_z_, result_buffer_count) char*                const result_buffer,
    _In_fits_precision_(precision)                         size_t               const result_buffer_count,
    _Out_writes_(scratch_buffer_count)                     char*                const scratch_buffer,
    _In_                                                   size_t               const scratch_buffer_count,
    _In_                                                   int                  const precision,
    _In_                                                   __acrt_rounding_mode const rounding_mode,
    _Inout_                                                __crt_cached_ptd_host&     ptd
    ) throw()
{
    _strflt strflt{};
    __acrt_has_trailing_digits const trailing_digits = __acrt_fltout(
        *reinterpret_cast<_CRT_DOUBLE const*>(argument),
        precision,
        __acrt_precision_style::fixed,
        &strflt,
        scratch_buffer,
        scratch_buffer_count
        );

    errno_t const e = __acrt_fp_strflt_to_string(
        result_buffer + (strflt.sign == '-'),
        (result_buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE ? result_buffer_count : result_buffer_count - (strflt.sign == '-')),
        precision + strflt.decpt,
        &strflt,
        trailing_digits,
        rounding_mode,
        ptd);

    if (e != 0)
    {
        result_buffer[0] = '\0';
        return e;
    }

    return fp_format_f_internal(result_buffer, result_buffer_count, precision, &strflt, false, ptd);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// %g formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions handle the formatting of floating point values in the %g
// printf format.  The form used depends on the value converted.  The printf %e
// form will be used if the magnitude of the value is less than -4 or is greater
// than 'precision', otherwise %f will be used.  The 'precision' always specifies
// the number of digits following the decimal point.  The low order digit is
// appropriately rounded.
_Success_(return == 0)
static errno_t __cdecl fp_format_g(
    _In_                                                   double const*        const argument,
    _Maybe_unsafe_(_Inout_updates_z_, result_buffer_count) char*                const result_buffer,
    _In_fits_precision_(precision)                         size_t               const result_buffer_count,
    _Out_writes_(scratch_buffer_count)                     char*                const scratch_buffer,
    _In_                                                   size_t               const scratch_buffer_count,
    _In_                                                   int                  const precision,
    _In_                                                   bool                 const capitals,
    _In_                                                   unsigned             const min_exponent_digits,
    _In_                                                   __acrt_rounding_mode const rounding_mode,
    _Inout_                                                __crt_cached_ptd_host&   ptd
    ) throw()
{
    _strflt strflt{};

    // Generate digits as though we will use %f formatting, then decide based on the result
    // whether to use %f or %e formatting. %f always requires more generated digits than %e,
    // so generating them all now will avoid generating more later (generation isn't resumable).
    __acrt_has_trailing_digits const trailing_digits = __acrt_fltout(
        *reinterpret_cast<_CRT_DOUBLE const*>(argument),
        precision,
        __acrt_precision_style::fixed,
        &strflt,
        scratch_buffer,
        scratch_buffer_count
        );

    size_t const minus_sign_length = strflt.sign == '-' ? 1 : 0;

    int g_magnitude = strflt.decpt - 1;
    char* p = result_buffer + minus_sign_length;

    size_t const buffer_count_for_fptostr = result_buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE
        ? result_buffer_count
        : result_buffer_count - minus_sign_length;

    errno_t const fptostr_result = __acrt_fp_strflt_to_string(p, buffer_count_for_fptostr, precision, &strflt, trailing_digits, rounding_mode, ptd);
    if (fptostr_result != 0)
    {
        result_buffer[0] = '\0';
        return fptostr_result;
    }

    bool const g_round_expansion = g_magnitude < (strflt.decpt - 1);

    // Compute the magnitude of value:
    g_magnitude = strflt.decpt - 1;

    // Convert value to the C Language g format:
    if (g_magnitude < -4 || g_magnitude >= precision) // Use e format
    {
        // We can ignore the round expansion flag here:  the extra digit will be
        // overwritten by "e+xxx".
        return fp_format_e_internal(result_buffer, result_buffer_count, precision, capitals, min_exponent_digits, &strflt, true, ptd);
    }
    else // Use f format
    {
        if (g_round_expansion)
        {
            // Throw away extra final digit from expansion:
            while (*p++)
            {
                // Iterate to the end of the string
            }

            *(p - 2) = '\0';
        }

        return fp_format_f_internal(result_buffer, result_buffer_count, precision, &strflt, true, ptd);
    }
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Format Dispatch
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The main floating point formatting dispatch function.  This function just
// looks at the 'format' character, calls the right formatting function, and
// returns the result.  The other parameters are passed on to the selected
// formatting function and are used as described in the documentation for
// those functions.
extern "C" errno_t __cdecl __acrt_fp_format(
    double const*        const value,
    char*                const result_buffer,
    size_t               const result_buffer_count,
    char*                const scratch_buffer,
    size_t               const scratch_buffer_count,
    int                  const format,
    int                  const precision,
    uint64_t             const options,
    __acrt_rounding_mode       rounding_mode,
    __crt_cached_ptd_host&     ptd
    )
{
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, result_buffer != nullptr,  EINVAL);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, result_buffer_count > 0,   EINVAL);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, scratch_buffer != nullptr, EINVAL);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, scratch_buffer_count > 0,  EINVAL);

    bool const use_capitals = format == 'A' || format == 'E' || format == 'F' || format == 'G';

    // Detect special cases (NaNs and infinities) and handle them specially.
    // Note that the underlying __acrt_fltout function will also handle these
    // special cases, but it does so using the legacy strings (e.g. 1.#INF).
    // Our special handling here uses the C99 strings (e.g. INF).
    if ((options & _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY) == 0)
    {
        __acrt_fp_class const classification = __acrt_fp_classify(*value);
        if (classification != __acrt_fp_class::finite)
        {
            return fp_format_nan_or_infinity(
                classification,
                __acrt_fp_is_negative(*value),
                result_buffer,
                result_buffer_count,
                use_capitals);
        }
    }

    unsigned const min_exponent_digits = (options & _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS) != 0 ? 3 : 2;
    if ((options & _CRT_INTERNAL_PRINTF_STANDARD_ROUNDING) == 0) {
        rounding_mode = __acrt_rounding_mode::legacy;
    }

    switch (format)
    {
    case 'a':
    case 'A':
        return fp_format_a(value, result_buffer, result_buffer_count, scratch_buffer, scratch_buffer_count, precision, use_capitals, min_exponent_digits, rounding_mode, ptd);

    case 'e':
    case 'E':
        return fp_format_e(value, result_buffer, result_buffer_count, scratch_buffer, scratch_buffer_count, precision, use_capitals, min_exponent_digits, rounding_mode, ptd);

    case 'f':
    case 'F':
        return fp_format_f(value, result_buffer, result_buffer_count, scratch_buffer, scratch_buffer_count, precision, rounding_mode, ptd);

    default:
        _ASSERTE(("Unsupported format specifier", 0));
    case 'g':
    case 'G':
        return fp_format_g(value, result_buffer, result_buffer_count, scratch_buffer, scratch_buffer_count, precision, use_capitals, min_exponent_digits, rounding_mode, ptd);
    }
}
