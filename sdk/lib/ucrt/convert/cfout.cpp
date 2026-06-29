//
// cfout.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Floating point binary to decimal conversion routines
//
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_big_integer.h>
#include <fenv.h>
#include <string.h>



using namespace __crt_strtox;

namespace
{
    // Guard class for floating point control word modifications in the interrupt exception mask.
    class fp_control_word_guard
    {
    public:
        explicit fp_control_word_guard(unsigned int const mask = ~0u) : _mask(mask)
        {
            _controlfp_s(&_original_control_word, 0, 0);
        }

        fp_control_word_guard(unsigned int const new_control, unsigned int const mask) : _mask(mask)
        {
            unsigned int float_control;
            _controlfp_s(&_original_control_word, 0, 0);
            _controlfp_s(&float_control, new_control, _mask);
        }

        ~fp_control_word_guard()
        {
            unsigned int reset_cw;
            _controlfp_s(&reset_cw, _original_control_word, _mask);
        }

    private:
        unsigned int _original_control_word;
        unsigned int _mask;
    };

    class scoped_fp_state_reset
    {
    public:

        scoped_fp_state_reset() throw()
        {
            // The calls to feholdexcept and fesetenv are relatively expensive,
            // so we only call them if there are any unmasked exceptions.
            fegetenv(&_environment);
            if ((_environment._Fe_ctl & FE_ALL_EXCEPT) == FE_ALL_EXCEPT)
            {
                _requires_reset = false;
            }
            else
            {
                feholdexcept(&_environment);
                _requires_reset = true;
            }
        }

        ~scoped_fp_state_reset() throw()
        {
            if (_requires_reset)
            {
                fesetenv(&_environment);
            }
        }

    private:

        scoped_fp_state_reset(scoped_fp_state_reset const&);
        scoped_fp_state_reset& operator=(scoped_fp_state_reset const&);

        fenv_t _environment;
        bool   _requires_reset;
    };
}



// This function converts a finite, positive floating point value into its
// decimal representation. The decimal mantissa and exponent are returned
// via the out parameters and the return value affirms if there were trailing
// digits. If the value is zero, negative, infinity, or nan, the behavior is
// undefined.
//
// This function is based on the digit generation algorithm described in the
// paper "Printing floating point numbers quickly and accurately," by Robert G.
// Burger and R. Kent Dybvig, from ACM SIGPLAN 1996 Conference on Programming
// Language Design and Implementation, June 1996.  This function deviates from
// that algorithm in its termination conditions:  the Burger-Dybvig algorithm
// terminates when it runs out of significant digits; this function continues
// generating digits, even if they are insignificant (this allows us to print
// large, exactly representable values exactly, e.g. large powers of two).
//
// This function stops generating digits when the first of the following
// conditions is true:  [1] the mantissa buffer is exhausted, [2] sufficient
// digits have been generated for a %f specifier with the requested precision,
// or [3] all remaining digits are known to be zero.
template <typename FloatingType>
__forceinline static __acrt_has_trailing_digits __cdecl convert_to_fos_high_precision(
    FloatingType           const value,
    uint32_t               const precision,
    __acrt_precision_style const precision_style,
    int*                   const exponent,
    char*                  const mantissa_buffer,
    size_t                 const mantissa_buffer_count
    ) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    _ASSERTE(mantissa_buffer_count > 0);

    components_type const& value_components = reinterpret_cast<components_type const&>(value);

    // Special handling is required for denormal values:  because the implicit
    // high order bit is a zero, not a one, we need to [1] not set that bit when
    // we expand the mantissa, and [2] increment the exponent to account for the
    // extra shift.
    bool const is_denormal = value_components._exponent == 0;

    uint64_t const mantissa_adjustment = is_denormal
        ? 0
        : static_cast<uint64_t>(1) << (floating_traits::mantissa_bits - 1);

    int32_t const exponent_adjustment = is_denormal
        ? 2
        : 1;

    // f and e are the unbiased mantissa and exponent, respectively.  (Where one-
    // -letter variable names are used, they match the same variables from the
    // Burger-Dybvig paper.)
    uint64_t const f = value_components._mantissa + mantissa_adjustment;
    int32_t  const e =
        static_cast<int32_t>(value_components._exponent) -
        floating_traits::exponent_bias -
        floating_traits::mantissa_bits +
        exponent_adjustment;

    // k is the decimal exponent, such that the resulting decimal number is of
    // the form 0.mmmmm * 10^k, where mmmm are the mantissa digits we generate.
    // Note that the use of log10 here may not give the correct result:  it may
    // be off by one, e.g. as is the case for powers of ten (log10(100) is two,
    // but the correct value of k for 100 is 3).  We detect off-by-one errors
    // later and correct for them.
    int32_t k = static_cast<int32_t>(ceil(log10(value)));
    if (k == INT32_MAX || k == INT32_MIN)
    {
        _ASSERTE(("unexpected input value; log10 failed", 0));
        k = 0;
    }

    // The floating point number is represented as a fraction, r / s.  The
    // initialization of these two values is as described in the Burger-Dybvig
    // paper.
    big_integer r = make_big_integer(f);
    big_integer s{};

    if (e >= 0)
    {
        if (r != make_big_integer_power_of_two(floating_traits::mantissa_bits - 1))
        {
            shift_left(r, e + 1);   // f * b^e * 2
            s = make_big_integer(2); // 2
        }
        else
        {
            shift_left(r, e + 2);   // f * b^(e+1) * 2
            s = make_big_integer(4); // b * 2
        }
    }
    else
    {
        if (e == floating_traits::minimum_binary_exponent ||
            r != make_big_integer_power_of_two(floating_traits::mantissa_bits - 1))
        {
            shift_left(r, 1);                          // f * 2
            s = make_big_integer_power_of_two(-e + 1); // b^-e * 2
        }
        else
        {
            shift_left(r, 2);                          // f * b * 2
            s = make_big_integer_power_of_two(-e + 2); // b^(-e+1) * 2
        }
    }

    if (k >= 0)
    {
        multiply_by_power_of_ten(s, k);
    }
    else
    {
        multiply_by_power_of_ten(r, -k);
    }

    char* mantissa_it = mantissa_buffer;

    // Perform a trial digit generation to handle off-by-one errors in the
    // computation of 'k':  There are three possible cases, which we handle
    // in turn:
    multiply(r, 10);
    uint32_t const initial_digit = static_cast<uint32_t>(divide(r, s));

    // If the initial digit was computed as 10, k is too small.  We increment k
    // and adjust s as if it had been computed with the correct k above.  We
    // then treat the digit as a one, which is what it would have been extracted
    // as had k, r, and s been correct to begin with.
    if (initial_digit == 10)
    {
        ++k;
        *mantissa_it++ = '1';
        multiply(s, 10);
    }
    // If the initial digit is zero, k is too large.  We decrement k and ignore
    // the zero that we read (the next digit that we read will be the "real"
    // initial digit.
    else if (initial_digit == 0)
    {
        --k;
    }
    // Otherwise, k was correct and the digit we read was the actual initial
    // digit of the number; just store it.
    else
    {
        *mantissa_it++ = static_cast<char>('0' + initial_digit);
    }

    *exponent = k; // k is now correct; store it for our caller

    // convert_to_fos_high_precision() generates digits assuming we're formatting with %f
    // When %e is the format specifier, adding the exponent to the number of required digits
    // is not needed.
    uint32_t const required_digits = k >= 0 && precision <= INT_MAX && precision_style == __acrt_precision_style::fixed
        ? k + precision
        : precision;

    char* const mantissa_last = mantissa_buffer + __min(mantissa_buffer_count - 1, required_digits);

    // We must track whether there are any non-zero digits that were not written to the mantissa,
    // since just checking whether 'r' is zero is insufficient for knowing whether there are any
    // remaining zeros after the generated digits.
    bool unwritten_nonzero_digits_in_chunk = false;
    for (;;)
    {
        if (mantissa_it == mantissa_last)
        {
            break;
        }

        if (is_zero(r))
        {
            break;
        }

        // To reduce the number of expensive high precision division operations,
        // we generate multiple digits per iteration.  Our quotient type is a
        // uint32_t, and the largest power of ten representable by a uint32_t is
        // 10^9, so we generate nine digits per iteration.
        uint32_t const digits_per_iteration            = 9;
        uint32_t const digits_per_iteration_multiplier = 1000 * 1000 * 1000;

        multiply(r, digits_per_iteration_multiplier);
        uint32_t quotient = static_cast<uint32_t>(divide(r, s));

        _ASSERTE(quotient < digits_per_iteration_multiplier);

        // Decompose the quotient into its nine component digits by repeatedly
        // dividing by ten.  This generates digits in reverse order.
        #pragma warning(suppress: 6293) // For-loop counts down from minimum
        for (uint32_t i = digits_per_iteration - 1; i != static_cast<uint32_t>(-1); --i)
        {
            char const d = static_cast<char>('0' + quotient % 10);
            quotient /= 10;

            // We may not have room in the mantissa buffer for all of the digits.
            // Ignore the ones for which we do not have room.
            // Last value in mantissa must be null terminator, account for one place after digit generation.
            if (static_cast<uint32_t>(mantissa_last - mantissa_it) <= i)
            {
                if (d != '0')
                {
                    unwritten_nonzero_digits_in_chunk = true;
                }

                continue;
            }

            mantissa_it[i] = d;
        }

        mantissa_it += __min(digits_per_iteration, mantissa_last - mantissa_it);
    }

    *mantissa_it = '\0';

    // To detect whether there are zeros after the generated digits for the purposes of rounding,
    // we must check whether 'r' is zero, but also whether there were any non-zero numbers that were
    // not written to the mantissa in the last evaluated chunk.
    bool const all_zeros_after_chunk = is_zero(r);

    if (all_zeros_after_chunk && !unwritten_nonzero_digits_in_chunk)
    {
        return __acrt_has_trailing_digits::no_trailing;
    }

    return __acrt_has_trailing_digits::trailing;
}

extern "C" __acrt_has_trailing_digits __cdecl __acrt_fltout(
    _CRT_DOUBLE                  value,
    unsigned               const precision,
    __acrt_precision_style const precision_style,
    STRFLT                 const flt,
    char*                  const result,
    size_t                 const result_count
    )
{
    using floating_traits = __acrt_floating_type_traits<double>;
    using components_type = floating_traits::components_type;

    scoped_fp_state_reset const reset_fp_state;

    components_type& components = reinterpret_cast<components_type&>(value);

    flt->sign     = components._sign == 1 ? '-' : ' ';
    flt->mantissa = result;

    unsigned int float_control;
    _controlfp_s(&float_control, 0, 0);
    bool const value_is_zero = components._exponent == 0 && (components._mantissa == 0 || float_control & _DN_FLUSH);
    if (value_is_zero)
    {
        flt->decpt = 0;
        _ERRCHECK(strcpy_s(result, result_count, "0"));
        return __acrt_has_trailing_digits::no_trailing;
    }

    // Handle special cases:
    __acrt_fp_class const classification = __acrt_fp_classify(value.x);
    if (classification != __acrt_fp_class::finite)
    {
        flt->decpt = 1;
    }

    switch (classification)
    {
    case __acrt_fp_class::infinity:      _ERRCHECK(strcpy_s(result, result_count, "1#INF" )); return __acrt_has_trailing_digits::trailing;
    case __acrt_fp_class::quiet_nan:     _ERRCHECK(strcpy_s(result, result_count, "1#QNAN")); return __acrt_has_trailing_digits::no_trailing;
    case __acrt_fp_class::signaling_nan: _ERRCHECK(strcpy_s(result, result_count, "1#SNAN")); return __acrt_has_trailing_digits::no_trailing;
    case __acrt_fp_class::indeterminate: _ERRCHECK(strcpy_s(result, result_count, "1#IND" )); return __acrt_has_trailing_digits::no_trailing;
    }

    // Make the number positive before we pass it to the digit generator:
    components._sign = 0;

    // The digit generator produces a truncated sequence of digits.  To allow
    // our caller to correctly round the mantissa, we need to generate an extra
    // digit.
    fp_control_word_guard const fpc(_MCW_EM, _MCW_EM);
    return convert_to_fos_high_precision(value.x, precision + 1, precision_style, &flt->decpt, result, result_count);
}
