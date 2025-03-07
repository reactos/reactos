//
// _fptostr.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the internal function that writes a mantissa (in a STRFLT) into a
// buffer.  This function puts all of the digits into the buffer, handles
// rounding based on the requested precision (digits), and updates the decimal
// point position in the STRFLT object.  Note that this function does not change
// the mantissa field of the STRFLT, so callers of this function may rely on it
// being unmodified.
//
#include <corecrt_internal.h>
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_ptd_propagation.h>
#include <fenv.h>
#include <string.h>
#include <stddef.h>

// __acrt_has_trailing_digits::no_trailing isn't indicative of how to round if we're rounding to a point before the end of the generated digits.
// __acrt_has_trailing_digits::no_trailing only says if there are any digits after the mantisa we got.
// We need to ensure the remaining generated digits are all zero before choosing rounds-to-even behavior intended for exactly representable floating point numbers.
static bool check_trailing(char const * mantissa_it, __acrt_has_trailing_digits const trailing_digits)
{
    if (trailing_digits == __acrt_has_trailing_digits::trailing)
    {
        return true;
    }

    while (*mantissa_it == '0')
    {
        mantissa_it++;
    }

    if (*mantissa_it != '\0')
    {
        return true;
    }

    return false;
}

static bool should_round_up(
    char const *               const mantissa_base,
    char const *               const mantissa_it,
    int                        const sign,
    __acrt_has_trailing_digits const trailing_digits,
    __acrt_rounding_mode       const rounding_mode
)
{
    if (rounding_mode == __acrt_rounding_mode::legacy)
    {
        return *mantissa_it >= '5';
    }

    int const round_mode = fegetround();

    if (round_mode == FE_TONEAREST)
    {
        if (*mantissa_it > '5')
        {
            return true;
        }

        if (*mantissa_it < '5')
        {
            return false;
        }

        // If there are trailing digits we are in a scenario like this .5000000001 and should round up
        if (check_trailing(mantissa_it + 1, trailing_digits))
        {
            return true;
        }

        // At this point, the number is exactly representable and we are rounding 5.
        // In this case, IEEE 754 states to round towards the nearest even number.
        // Therefore: if the previous digit is odd, we round up (1.5 -> 2).
        //            if the previous digit is even, we round down (2.5 -> 2).

        // If there is no preceding digit, it is considered zero, so round down.
        if (mantissa_it == mantissa_base)
        {
            return false;
        }

        // If the previous digit is odd, we should round up to the closest even.
        return *(mantissa_it - 1) % 2;
    }

    if (round_mode == FE_UPWARD)
    {
        return check_trailing(mantissa_it, trailing_digits) && sign != '-';
    }

    if (round_mode == FE_DOWNWARD)
    {
        return check_trailing(mantissa_it, trailing_digits) && sign == '-';
    }

    return false;
}

extern "C" errno_t __cdecl __acrt_fp_strflt_to_string(
    char*                        const buffer,
    size_t                       const buffer_count,
    int                                digits,
    STRFLT                       const pflt,
    __acrt_has_trailing_digits   const trailing_digits,
    __acrt_rounding_mode         const rounding_mode,
    __crt_cached_ptd_host&             ptd
)
{
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, buffer != nullptr, EINVAL);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, buffer_count > 0,  EINVAL);
    buffer[0] = '\0';

    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, buffer_count > static_cast<size_t>((digits > 0 ? digits : 0) + 1), ERANGE);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, pflt != nullptr, EINVAL);

    char* buffer_it           = buffer;
    char* const mantissa_base = pflt->mantissa;
    char* mantissa_it         = pflt->mantissa;

    // The buffer will contain 'digits' decimal digits plus an optional overflow
    // digit for the rounding.

    // Initialize the first digit in the buffer to '0' (Note: not '\0') and set
    // the pointer to the second digit of the buffer.  The first digit is used
    // to handle overflow on rounding (e.g. 9.999... becomes 10.000...), which
    // requires a carry into the first digit.
    *buffer_it++ = '0';

    // Copy the digits of the value into the buffer (with '0' padding) and
    // insert the null terminator:
    while (digits > 0)
    {
        *buffer_it++ = *mantissa_it ? *mantissa_it++ : '0';
        --digits;
    }

    *buffer_it = '\0';

    // Do any rounding which may be needed.  Note:  if digits < 0, we don't do
    // any rounding because in this case, the rounding occurs in a digit which
    // will not be output because of the precision requested.
    if (digits >= 0 && should_round_up(mantissa_base, mantissa_it, pflt->sign, trailing_digits, rounding_mode))
    {
        buffer_it--;

        while (*buffer_it == '9')
        {
            *buffer_it-- = '0';
        }

        *buffer_it += 1;
    }

    if (*buffer == '1')
    {
        // The rounding caused overflow into the leading digit (e.g. 9.999...
        // became 10.000...), so increment the decimal point position by 1:
        pflt->decpt++;
    }
    else
    {
        // Move the entire string to the left one digit to remove the unused
        // overflow digit:
        memmove(buffer, buffer + 1, strlen(buffer + 1) + 1);
    }

    return 0;
}
