//
// gcvt.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the _gcvt functions, which convert a floating point value to a narrow
// character string.  It attempts to produce 'precision' significant digits in
// the Fortran F format if possible, otherwise the E format.  Trailing zeroes may
// be suppressed.  The _s-suffixed function returns zero on success; an error
// code on failure.  If the buffer is too small, that is an error.
//
#include <corecrt_internal.h>
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_ptd_propagation.h>
#include <corecrt_internal_securecrt.h>
#include <corecrt_stdio_config.h>
#include <locale.h>
#include <stdlib.h>



static errno_t __cdecl _gcvt_s_internal(
    char*              const buffer,
    size_t             const buffer_count,
    double             const value,
    int                const precision,
    __crt_cached_ptd_host&   ptd
    )
{
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, buffer != nullptr, EINVAL);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, buffer_count > 0,  EINVAL);
    _RESET_STRING(buffer, buffer_count);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, static_cast<size_t>(precision) < buffer_count, ERANGE);
    // Additional validation will be performed in the fp_format functions.

    char const decimal_point = *ptd.get_locale()->locinfo->lconv->decimal_point;

    // We only call __acrt_fltout in order to parse the correct exponent value (strflt.decpt).
    // Therefore, we don't want to generate any digits, so we pass a buffer size only large
    // enough to hold the inf, nan, or ind string to prevent failure.

    size_t const restricted_count = 7; // "1#SNAN" + 1 null terminator
    char result_string[restricted_count];

    _strflt strflt{};

    __acrt_fltout(
        reinterpret_cast<_CRT_DOUBLE const&>(value),
        precision,
        __acrt_precision_style::fixed,
        &strflt,
        result_string,
        restricted_count);

    int const magnitude = strflt.decpt - 1;

    // Output the result according to the Fortran G format as outlined in the
    // Fortran language specification.
    if (magnitude < -1 || magnitude > precision - 1)
    {
        // Ew.d where d = precision
        char scratch_buffer[_CVTBUFSIZE + 1];
        errno_t const e = __acrt_fp_format(
            &value,
            buffer,
            buffer_count,
            scratch_buffer,
            _countof(scratch_buffer),
            'e',
            precision - 1,
            _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS,
            __acrt_rounding_mode::legacy,
            ptd);

        if (e != 0)
        {
            return ptd.get_errno().set(e);
        }
    }
    else
    {
        // Fw.d where d = precision-string->decpt
        char scratch_buffer[_CVTBUFSIZE + 1];
        errno_t const e = __acrt_fp_format(
            &value,
            buffer,
            buffer_count,
            scratch_buffer,
            _countof(scratch_buffer),
            'f',
            precision - strflt.decpt,
            _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS,
            __acrt_rounding_mode::legacy,
            ptd);

        if (e != 0)
        {
            return ptd.get_errno().set(e);
        }
    }

    // Remove the trailing zeroes before the exponent; we don't need to check
    // for buffer_count:
    char* p = buffer;
    while (*p && *p != decimal_point)
    {
        ++p;
    }

    if (*p == '\0')
    {
        return 0;
    }

    ++p;

    while (*p && *p != 'e')
    {
        ++p;
    }

    char* stop = p;
    --p;

    while (*p == '0')
    {
        --p;
    }

    while ((*++p = *stop++) != '\0') { }

    return 0;
}

extern "C" errno_t __cdecl _gcvt_s(
    char*  const buffer,
    size_t const buffer_count,
    double const value,
    int    const precision
    )
{
    __crt_cached_ptd_host ptd;
    return _gcvt_s_internal(buffer, buffer_count, value, precision, ptd);
}

extern "C" char* __cdecl _gcvt(
    double const value,
    int    const precision,
    char*  const buffer
    )
{
    errno_t const e = _gcvt_s(buffer, _CRT_UNBOUNDED_BUFFER_SIZE, value, precision);
    if (e != 0)
    {
        return nullptr;
    }

    return buffer;
}
