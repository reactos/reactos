//
// corecrt_internal_strtox.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file defines the core implementations of the numeric parsers that parse
// integer or floating point numbers stored as strings.  These general parsers
// operate on abstract character sources, which allow the functions to be used
// to parse both random access and nonseekable ranges of characters (to support
// both strtod-style functions and fscanf-style functions).
//
#pragma once

#include <corecrt_internal.h>
#include <corecrt_internal_big_integer.h>
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_ptd_propagation.h>
#include <corecrt_internal_stdio.h>
#include <ctype.h>
#include <fenv.h>
#include <locale.h>
#include <stdint.h>

// This header is temporarily mixed PTD-propagation and direct errno usage.
#pragma push_macro("_VALIDATE_RETURN_VOID")
#pragma push_macro("_VALIDATE_RETURN")
#pragma push_macro("_INVALID_PARAMETER")
#undef _VALIDATE_RETURN_VOID
#undef _VALIDATE_RETURN
#undef _INVALID_PARAMETER

#ifdef _DEBUG
    #define _INVALID_PARAMETER(expr) _invalid_parameter(expr, __FUNCTIONW__, __FILEW__, __LINE__, 0)
#else
    #define _INVALID_PARAMETER(expr) _invalid_parameter_noinfo()
#endif

#define _VALIDATE_RETURN(expr, errorcode, retexpr)                             \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            *_errno() = (errorcode);                                           \
            _INVALID_PARAMETER(_CRT_WIDE(#expr));                              \
            return (retexpr);                                                  \
        }                                                                      \
    }

#define _VALIDATE_RETURN_VOID(expr, errorcode)                                 \
    {                                                                          \
        int _Expr_val = !!(expr);                                              \
        _ASSERT_EXPR((_Expr_val), _CRT_WIDE(#expr));                           \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            *_errno() = (errorcode);                                           \
            _INVALID_PARAMETER(_CRT_WIDE(#expr));                              \
            return;                                                            \
        }                                                                      \
    }

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// String-to-Integer Conversion
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace __crt_strtox {

template <typename T> struct is_signed;
template <> struct is_signed<long              > { enum { value = true  }; };
template <> struct is_signed<long long         > { enum { value = true  }; };
template <> struct is_signed<unsigned long     > { enum { value = false }; };
template <> struct is_signed<unsigned long long> { enum { value = false }; };

template <typename T> struct make_signed;
template <> struct make_signed<long              > { using type = long;      };
template <> struct make_signed<long long         > { using type = long long; };
template <> struct make_signed<unsigned long     > { using type = long;      };
template <> struct make_signed<unsigned long long> { using type = long long; };

template <typename T> struct make_unsigned;
template <> struct make_unsigned<long              > { using type = unsigned long;      };
template <> struct make_unsigned<long long         > { using type = unsigned long long; };
template <> struct make_unsigned<unsigned long     > { using type = unsigned long;      };
template <> struct make_unsigned<unsigned long long> { using type = unsigned long long; };

// Converts a wide character to its corresponding digit.  Returns -1 on failure.
__forceinline int __cdecl wide_character_to_digit(wchar_t const c) throw()
{
    #define DIGIT_RANGE_TEST(zero) \
        if (c < zero)              \
            return -1;             \
                                   \
        if (c < zero + 10)         \
            return c - zero;

    DIGIT_RANGE_TEST(0x0030)        // 0030;DIGIT ZERO
    if (c < 0xFF10)                 // FF10;FULLWIDTH DIGIT ZERO
    {
        DIGIT_RANGE_TEST(0x0660)    // 0660;ARABIC-INDIC DIGIT ZERO
        DIGIT_RANGE_TEST(0x06F0)    // 06F0;EXTENDED ARABIC-INDIC DIGIT ZERO
        DIGIT_RANGE_TEST(0x0966)    // 0966;DEVANAGARI DIGIT ZERO
        DIGIT_RANGE_TEST(0x09E6)    // 09E6;BENGALI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0A66)    // 0A66;GURMUKHI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0AE6)    // 0AE6;GUJARATI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0B66)    // 0B66;ORIYA DIGIT ZERO
        DIGIT_RANGE_TEST(0x0C66)    // 0C66;TELUGU DIGIT ZERO
        DIGIT_RANGE_TEST(0x0CE6)    // 0CE6;KANNADA DIGIT ZERO
        DIGIT_RANGE_TEST(0x0D66)    // 0D66;MALAYALAM DIGIT ZERO
        DIGIT_RANGE_TEST(0x0E50)    // 0E50;THAI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0ED0)    // 0ED0;LAO DIGIT ZERO
        DIGIT_RANGE_TEST(0x0F20)    // 0F20;TIBETAN DIGIT ZERO
        DIGIT_RANGE_TEST(0x1040)    // 1040;MYANMAR DIGIT ZERO
        DIGIT_RANGE_TEST(0x17E0)    // 17E0;KHMER DIGIT ZERO
        DIGIT_RANGE_TEST(0x1810)    // 1810;MONGOLIAN DIGIT ZERO

        return -1;
    }

    #undef DIGIT_RANGE_TEST

    if (c < 0xFF10 + 10)            // FF10;FULLWIDTH DIGIT ZERO
        return c - 0xFF10;

    return -1;
}

__forceinline unsigned __cdecl parse_digit(char const c) throw()
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<unsigned>(c - '0');
    }

    if (c >= 'a' && c <= 'z')
    {
        return static_cast<unsigned>(c - 'a' + 10);
    }

    if (c >= 'A' && c <= 'Z')
    {
        return static_cast<unsigned>(c - 'A' + 10);
    }

    return static_cast<unsigned>(-1);
}

__forceinline unsigned __cdecl parse_digit(wchar_t const c) throw()
{
    int const value = wide_character_to_digit(c);
    if (value != -1)
        return static_cast<unsigned>(value);

    if (__ascii_iswalpha(c))
        return static_cast<unsigned>(__ascii_towupper(c) - 'A' + 10);

    return static_cast<unsigned>(-1);
}

// The digit and nondigit categories include 0-9, a-z, A-Z, and _.  They are not
// locale-dependent, so we do not call isalnum (which uses the current locale to
// test for alphabetic characters).
__forceinline bool __cdecl is_digit_or_nondigit(int const c) throw()
{
    if (c >= '0' && c <= '9')
        return true;

    if (c >= 'a' && c <= 'z')
        return true;

    if (c >= 'A' && c <= 'Z')
        return true;

    if (c == '_')
        return true;

    return false;
}

__forceinline bool __cdecl is_space(char const c, _locale_t const locale) throw()
{
    return _isspace_l(static_cast<int>(static_cast<unsigned char>(c)), locale) != 0;
}

__forceinline bool __cdecl is_space(wchar_t const c, _locale_t) throw()
{
    return iswspace(c) != 0;
}

inline long minimum_signed_value(unsigned long)    throw() { return LONG_MIN; }
inline long maximum_signed_value(unsigned long)    throw() { return LONG_MAX; }

inline __int64 minimum_signed_value(unsigned __int64) throw() { return _I64_MIN; }
inline __int64 maximum_signed_value(unsigned __int64) throw() { return _I64_MAX; }

enum : unsigned
{
    FL_SIGNED     = 0x01,
    FL_NEGATIVE   = 0x02,
    FL_OVERFLOW   = 0x04,
    FL_READ_DIGIT = 0x08
};

template <typename UnsignedInteger>
bool is_overflow_condition(unsigned const flags, UnsignedInteger const number) throw()
{
    if (flags & FL_OVERFLOW)
        return true;

    if (flags & FL_SIGNED)
    {
        if ((flags & FL_NEGATIVE) != 0 && number > static_cast<UnsignedInteger>(-minimum_signed_value(UnsignedInteger())))
            return true;

        if ((flags & FL_NEGATIVE) == 0 && number > static_cast<UnsignedInteger>(maximum_signed_value(UnsignedInteger())))
            return true;
    }

    return false;
}

template <typename UnsignedInteger, typename CharacterSource, bool TrimWhitespace = true>
UnsignedInteger __cdecl parse_integer(
    __crt_cached_ptd_host&   ptd,
    CharacterSource          source,
    int                      base,
    bool               const is_result_signed
    ) throw()
{
    static_assert(!__crt_strtox::is_signed<UnsignedInteger>::value, "UnsignedInteger must be unsigned");

    using char_type = typename CharacterSource::char_type;

    if (!source.validate())
        return 0;

    _UCRT_VALIDATE_RETURN(ptd, base == 0 || (2 <= base && base <= 36), EINVAL, 0);
    UnsignedInteger number{0}; // number is the accumulator

    auto const initial_state = source.save_state();


    char_type c{source.get()};

    if constexpr (TrimWhitespace)
    {
        const _locale_t loc = ptd.get_locale();
        while (is_space(c, loc))
        {
            c = source.get();
        }
    }

    unsigned flags{is_result_signed ? FL_SIGNED : 0};

    // Optional sign (+ or -):
    if (c == '-')
    {
        flags |= FL_NEGATIVE;
    }

    if (c == '-' || c == '+')
    {
        c = source.get();
    }

    // If the base is zero, we try to detect the base from the string prefix:
    if (base == 0 || base == 16)
    {
        if (parse_digit(c) != 0)
        {
            if (base == 0)
            {
                base = 10;
            }
        }
        else
        {
            char_type const next_c = source.get();
            if (next_c == 'x' || next_c == 'X')
            {
                if (base == 0)
                {
                    base = 16;
                }
                c = source.get();
            }
            else
            {
                if (base == 0)
                {
                    base = 8;
                }
                source.unget(next_c);
            }
        }
    }

    UnsignedInteger const max_pre_multiply_value = static_cast<UnsignedInteger>(-1) / base;

    for (;;)
    {
        unsigned const digit{parse_digit(c)};
        if (digit >= static_cast<unsigned>(base))
        {
            // This also handles the case where the digit could not
            // be parsed and parse_digit returns -1.
            break;
        }

        flags |= FL_READ_DIGIT;

        UnsignedInteger const number_after_multiply = number * base;
        UnsignedInteger const number_after_add = number_after_multiply + digit;

        // Avoid branching when setting overflow flag.
        flags |= FL_OVERFLOW * ((number > max_pre_multiply_value) | (number_after_add < number_after_multiply));

        number = number_after_add;

        c = source.get();
    }

    source.unget(c); // Return the pointer to the character that ended the scan

    // If we failed to read any digits, there's no number to be read:
    if ((flags & FL_READ_DIGIT) == 0)
    {
        source.restore_state(initial_state);
        return 0;
    }

    if (is_overflow_condition(flags, number))
    {
        ptd.get_errno().set(ERANGE);

        if ((flags & FL_SIGNED) == 0)
        {
            number = static_cast<UnsignedInteger>(-1);
        }
        else if (flags & FL_NEGATIVE)
        {
            return minimum_signed_value(UnsignedInteger());
        }
        else
        {
            return maximum_signed_value(UnsignedInteger());
        }
    }
    else if (flags & FL_NEGATIVE)
    {
        number = static_cast<UnsignedInteger>(-static_cast<typename make_signed<UnsignedInteger>::type>(number));
    }

    return number;
}

template <typename UnsignedInteger, typename CharacterSource>
UnsignedInteger __cdecl parse_integer(
    _locale_t       const locale,
    CharacterSource       source,
    int                   base,
    bool            const is_result_signed
    ) throw()
{
    __crt_cached_ptd_host ptd{locale};
    return parse_integer<UnsignedInteger>(ptd, static_cast<CharacterSource&&>(source), base, is_result_signed);
}

} // namespace __crt_strtox



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// String-to-Floating-Point Conversion
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef enum
{
    SLD_OK,

    SLD_NODIGITS,

    SLD_UNDERFLOW,
    SLD_OVERFLOW
} SLD_STATUS;

namespace __crt_strtox {

// This is the internal result type of an attempt to parse a floating point value
// from a string.  The SLD_STATUS type (above) is the type returned to callers of
// the top-level parse_floating_point.
enum class floating_point_parse_result
{
    decimal_digits,
    hexadecimal_digits,

    zero,
    infinity,
    qnan,
    snan,
    indeterminate,

    no_digits,
    underflow,
    overflow
};

enum
{
    maximum_temporary_decimal_exponent =  5200,
    minimum_temporary_decimal_exponent = -5200
};

// This type is used to hold a partially-parsed string representation of a
// floating point number.  The number is stored in the following form:
//
//     [sign] 0._mantissa * B^_exponent
//
// The _mantissa buffer stores the mantissa digits in big endian, binary coded
// decimal form.  The _mantissa_count stores the number of digits present in the
// _mantissa buffer.  The base B is not stored; it must be tracked separately.
// Note that the base of the mantissa digits may not be the same as B (e.g., for
// hexadecimal floating point hexadecimal, the mantissa digits are in base 16
// but the exponent is a base 2 exponent).
//
// We consider up to 768 decimal digits during conversion.  In most cases, we
// require nowhere near this many bits of precision to compute the correctly
// rounded binary floating point value for the input string.  768 bits gives us
// room to handle the exact decimal representation of the smallest denormal
// value, 2^-1074 (752 decimal digits after trimming zeroes) with a bit of slack
// space.
//
// NOTE:  The mantissa buffer count here must be kept in sync with the precision
// of the big_integer type, defined in <corecrt_internal_big_integer.h>.  See that file
// for details.
struct floating_point_string
{
    int32_t  _exponent;
    uint32_t _mantissa_count;
    uint8_t  _mantissa[768];
    bool     _is_negative;
};

// This type wraps a float or double.  It serves as a crude form of type erasure
// to allow us to avoid instantiating most of the parsing logic twice (once for
// float and once for double).
class floating_point_value
{
public:

    template <typename T>
    using traits = __acrt_floating_type_traits<T>;

    explicit floating_point_value(double* const value) throw()
        : _value(value), _is_double(true)
    {
        _ASSERTE(value != nullptr);
    }

    explicit floating_point_value(float* const value) throw()
        : _value(value), _is_double(false)
    {
        _ASSERTE(value != nullptr);
    }

    bool is_double() const throw()
    {
        return _is_double;
    }

    double& as_double() const throw()
    {
        _ASSERT_AND_INVOKE_WATSON(_is_double);
        return *static_cast<double*>(_value);
    }

    float& as_float() const throw()
    {
        _ASSERT_AND_INVOKE_WATSON(!_is_double);
        return *static_cast<float*>(_value);
    }

    int32_t mantissa_bits          () const throw() { return _is_double ? traits<double>::mantissa_bits           : traits<float>::mantissa_bits;           }
    int32_t exponent_bits          () const throw() { return _is_double ? traits<double>::exponent_bits           : traits<float>::exponent_bits;           }
    int32_t maximum_binary_exponent() const throw() { return _is_double ? traits<double>::maximum_binary_exponent : traits<float>::maximum_binary_exponent; }
    int32_t minimum_binary_exponent() const throw() { return _is_double ? traits<double>::minimum_binary_exponent : traits<float>::minimum_binary_exponent; }
    int32_t exponent_bias          () const throw() { return _is_double ? traits<double>::exponent_bias           : traits<float>::exponent_bias;           }

    uint64_t exponent_mask            () const throw() { return _is_double ? traits<double>::exponent_mask             : traits<float>::exponent_mask;             }
    uint64_t normal_mantissa_mask     () const throw() { return _is_double ? traits<double>::normal_mantissa_mask      : traits<float>::normal_mantissa_mask;      }
    uint64_t denormal_mantissa_mask   () const throw() { return _is_double ? traits<double>::denormal_mantissa_mask    : traits<float>::denormal_mantissa_mask;    }
    uint64_t special_nan_mantissa_mask() const throw() { return _is_double ? traits<double>::special_nan_mantissa_mask : traits<float>::special_nan_mantissa_mask; }

private:

    void*  _value;
    bool   _is_double;
};

// Stores a positive or negative zero into the result object
template <typename FloatingType>
void __cdecl assemble_floating_point_zero(bool const is_negative, FloatingType& result) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = is_negative ? 1 : 0;
    result_components._exponent = 0;
    result_components._mantissa = 0;
}

inline void __cdecl assemble_floating_point_zero(bool const is_negative, floating_point_value const& result) throw()
{
    if (result.is_double())
    {
        assemble_floating_point_zero(is_negative, result.as_double());
    }
    else
    {
        assemble_floating_point_zero(is_negative, result.as_float());
    }
}

// Stores a positive or negative infinity into the result object
template <typename FloatingType>
void __cdecl assemble_floating_point_infinity(bool const is_negative, FloatingType& result) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = is_negative ? 1 : 0;
    result_components._exponent = floating_traits::exponent_mask;
    result_components._mantissa = 0;
}

inline void __cdecl assemble_floating_point_infinity(bool const is_negative, floating_point_value const& result) throw()
{
    if (result.is_double())
    {
        assemble_floating_point_infinity(is_negative, result.as_double());
    }
    else
    {
        assemble_floating_point_infinity(is_negative, result.as_float());
    }
}

// Stores a positive or negative quiet NaN into the result object
template <typename FloatingType>
void __cdecl assemble_floating_point_qnan(bool const is_negative, FloatingType& result) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = is_negative ? 1 : 0;
    result_components._exponent = floating_traits::exponent_mask;
    result_components._mantissa = floating_traits::denormal_mantissa_mask;
}

inline void __cdecl assemble_floating_point_qnan(bool const is_negative, floating_point_value const& result) throw()
{
    if (result.is_double())
    {
        assemble_floating_point_qnan(is_negative, result.as_double());
    }
    else
    {
        assemble_floating_point_qnan(is_negative, result.as_float());
    }
}

// Stores a positive or negative signaling NaN into the result object
template <typename FloatingType>
void __cdecl assemble_floating_point_snan(bool const is_negative, FloatingType& result) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = is_negative ? 1 : 0;
    result_components._exponent = floating_traits::exponent_mask;
    result_components._mantissa = 1;
}

inline void __cdecl assemble_floating_point_snan(bool const is_negative, floating_point_value const& result) throw()
{
    if (result.is_double())
    {
        assemble_floating_point_snan(is_negative, result.as_double());
    }
    else
    {
        assemble_floating_point_snan(is_negative, result.as_float());
    }
}

// Stores an indeterminate into the result object (the indeterminate is "negative")
template <typename FloatingType>
void __cdecl assemble_floating_point_ind(FloatingType& result) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = 1;
    result_components._exponent = floating_traits::exponent_mask;
    result_components._mantissa = floating_traits::special_nan_mantissa_mask;
}

inline void __cdecl assemble_floating_point_ind(floating_point_value const& result) throw()
{
    if (result.is_double())
    {
        assemble_floating_point_ind(result.as_double());
    }
    else
    {
        assemble_floating_point_ind(result.as_float());
    }
}

// Determines whether a mantissa should be rounded up in the current rounding
// mode given [1] the value of the least significant bit of the mantissa, [2]
// the value of the next bit after the least significant bit (the "round" bit)
// and [3] whether any trailing bits after the round bit are set.
//
// The mantissa is treated as an unsigned integer magnitude; the sign is used
// only to compute the correct rounding direction for directional rounding modes.
//
// For this function, "round up" is defined as "increase the magnitude" of the
// mantissa.  (Note that this means that if we need to round a negative value to
// the next largest representable value, we return false, because the next
// largest representable value has a smaller magnitude.)
__forceinline bool __cdecl should_round_up(
    bool const is_negative,
    bool const lsb_bit,
    bool const round_bit,
    bool const has_tail_bits
    ) throw()
{
    // If there are no insignificant set bits, the value is exactly representable
    // and should not be rounded in any rounding mode:
    bool const is_exactly_representable = !round_bit && !has_tail_bits;
    if (is_exactly_representable)
    {
        return false;
    }

    // If there are insignificant set bits, we need to round according to the
    // current rounding mode.  For FE_TONEAREST, we need to handle two cases:
    // we round up if either [1] the value is slightly greater than the midpoint
    // between two exactly representable values or [2] the value is exactly the
    // midpoint between two exactly representable values and the greater of the
    // two is even (this is "round-to-even").
    switch (fegetround())
    {
    case FE_TONEAREST:  return round_bit && (has_tail_bits || lsb_bit);
    case FE_UPWARD:     return !is_negative;
    case FE_DOWNWARD:   return is_negative;
    case FE_TOWARDZERO: return false;
    }

    _ASSERTE(("unexpected rounding mode", false));
    return false;
}

// Computes value / 2^shift, then rounds the result according to the current
// rounding mode.  By the time we call this function, we will already have
// discarded most digits.  The caller must pass true for has_zero_tail if
// all discarded bits were zeroes.
__forceinline uint64_t __cdecl right_shift_with_rounding(
    bool     const is_negative,
    uint64_t const value,
    uint32_t const shift,
    bool     const has_zero_tail
    ) throw()
{
    // If we'd need to shift further than it is possible to shift, the answer
    // is always zero:
    if (shift >= sizeof(uint64_t) * CHAR_BIT)
    {
        return 0;
    }

    uint64_t const extra_bits_mask = (1ull << (shift - 1)) - 1;
    uint64_t const round_bit_mask  = (1ull << (shift - 1));
    uint64_t const lsb_bit_mask    =  1ull <<  shift;

    bool const lsb_bit   = (value & lsb_bit_mask)   != 0;
    bool const round_bit = (value & round_bit_mask) != 0;
    bool const tail_bits = !has_zero_tail || (value & extra_bits_mask) != 0;

    return (value >> shift) + should_round_up(is_negative, lsb_bit, round_bit, tail_bits);
}

// Converts the floating point value [sign] 0.mantissa * 2^exponent into the
// correct form for FloatingType and stores the result into the result object.
// The caller must ensure that the mantissa and exponent are correctly computed
// such that either [1] the most significant bit of the mantissa is in the
// correct position for the FloatingType, or [2] the exponent has been correctly
// adjusted to account for the shift of the mantissa that will be required.
//
// This function correctly handles range errors and stores a zero or infinity in
// the result object on underflow and overflow errors, respectively.  This
// function correctly forms denormal numbers when required.
//
// If the provided mantissa has more bits of precision than can be stored in the
// result object, the mantissa is rounded to the available precision.  Thus, if
// possible, the caller should provide a mantissa with at least one more bit of
// precision than is required, to ensure that the mantissa is correctly rounded.
// (The caller should not round the mantissa before calling this function.)
template <typename FloatingType>
SLD_STATUS __cdecl assemble_floating_point_value_t(
    bool         const  is_negative,
    int32_t      const  exponent,
    uint64_t     const  mantissa,
    FloatingType      & result
    ) throw()
{
    using floating_traits = __acrt_floating_type_traits<FloatingType>;
    using components_type = typename floating_traits::components_type;

    components_type& result_components = reinterpret_cast<components_type&>(result);
    result_components._sign     = is_negative;
    result_components._exponent = exponent + floating_traits::exponent_bias;
    result_components._mantissa = mantissa;
    return SLD_OK;
}

inline SLD_STATUS __cdecl assemble_floating_point_value(
    uint64_t             const  initial_mantissa,
    int32_t              const  initial_exponent,
    bool                 const  is_negative,
    bool                 const  has_zero_tail,
    floating_point_value const& result
    ) throw()
{
    // Assume that the number is representable as a normal value.  Compute the
    // number of bits by which we must adjust the mantissa to shift it into the
    // correct position, and compute the resulting base two exponent for the
    // normalized mantissa:
    uint32_t const initial_mantissa_bits = bit_scan_reverse(initial_mantissa);
    int32_t  const normal_mantissa_shift = static_cast<int32_t>(result.mantissa_bits() - initial_mantissa_bits);
    int32_t  const normal_exponent       = initial_exponent - normal_mantissa_shift;

    uint64_t mantissa = initial_mantissa;
    int32_t  exponent = normal_exponent;

    if (normal_exponent > result.maximum_binary_exponent())
    {
        // The exponent is too large to be represented by the floating point
        // type; report the overflow condition:
        assemble_floating_point_infinity(is_negative, result);
        return SLD_OVERFLOW;
    }
    else if (normal_exponent < result.minimum_binary_exponent())
    {
        // The exponent is too small to be represented by the floating point
        // type as a normal value, but it may be representable as a denormal
        // value.  Compute the number of bits by which we need to shift the
        // mantissa in order to form a denormal number.  (The subtraction of
        // an extra 1 is to account for the hidden bit of the mantissa that
        // is not available for use when representing a denormal.)
        int32_t const denormal_mantissa_shift =
            normal_mantissa_shift +
            normal_exponent +
            result.exponent_bias() -
            1;

        // Denormal values have an exponent of zero, so the debiased exponent is
        // the negation of the exponent bias:
        exponent = -result.exponent_bias();

        if (denormal_mantissa_shift < 0)
        {
            // Use two steps for right shifts:  for a shift of N bits, we first
            // shift by N-1 bits, then shift the last bit and use its value to
            // round the mantissa.
            mantissa = right_shift_with_rounding(is_negative, mantissa, -denormal_mantissa_shift, has_zero_tail);

            // If the mantissa is now zero, we have underflowed:
            if (mantissa == 0)
            {
                assemble_floating_point_zero(is_negative, result);
                return SLD_UNDERFLOW;
            }

            // When we round the mantissa, the result may be so large that the
            // number becomes a normal value.  For example, consider the single
            // precision case where the mantissa is 0x01ffffff and a right shift
            // of 2 is required to shift the value into position. We perform the
            // shift in two steps:  we shift by one bit, then we shift again and
            // round using the dropped bit.  The initial shift yields 0x00ffffff.
            // The rounding shift then yields 0x007fffff and because the least
            // significant bit was 1, we add 1 to this number to round it.  The
            // final result is 0x00800000.
            //
            // 0x00800000 is 24 bits, which is more than the 23 bits available
            // in the mantissa.  Thus, we have rounded our denormal number into
            // a normal number.
            //
            // We detect this case here and re-adjust the mantissa and exponent
            // appropriately, to form a normal number:
            if (mantissa > result.denormal_mantissa_mask())
            {
                // We add one to the denormal_mantissa_shift to account for the
                // hidden mantissa bit (we subtracted one to account for this bit
                // when we computed the denormal_mantissa_shift above).
                exponent =
                    initial_exponent -
                    (denormal_mantissa_shift + 1) -
                    normal_mantissa_shift;
            }
        }
        else
        {
            mantissa <<= denormal_mantissa_shift;
        }
    }
    else
    {
        if (normal_mantissa_shift < 0)
        {
            // Use two steps for right shifts:  for a shift of N bits, we first
            // shift by N-1 bits, then shift the last bit and use its value to
            // round the mantissa.
            mantissa = right_shift_with_rounding(is_negative, mantissa, -normal_mantissa_shift, has_zero_tail);

            // When we round the mantissa, it may produce a result that is too
            // large.  In this case, we divide the mantissa by two and increment
            // the exponent (this does not change the value).
            if (mantissa > result.normal_mantissa_mask())
            {
                mantissa >>= 1;
                ++exponent;

                // The increment of the exponent may have generated a value too
                // large to be represented.  In this case, report the overflow:
                if (exponent > result.maximum_binary_exponent())
                {
                    assemble_floating_point_infinity(is_negative, result);
                    return SLD_OVERFLOW;
                }
            }
        }
        else if (normal_mantissa_shift > 0)
        {
            mantissa <<= normal_mantissa_shift;
        }
    }

    // Unset the hidden bit in the mantissa and assemble the floating point value
    // from the computed components:
    mantissa &= result.denormal_mantissa_mask();

    return result.is_double()
        ? assemble_floating_point_value_t(is_negative, exponent, mantissa, result.as_double())
        : assemble_floating_point_value_t(is_negative, exponent, mantissa, result.as_float());
}

// This function is part of the fast track for integer floating point strings.
// It takes an integer and a sign and converts the value into its FloatingType
// representation, storing the result in the result object.  If the value is not
// representable, +/-infinity is stored and overflow is reported (since this
// function only deals with integers, underflow is impossible).
inline SLD_STATUS __cdecl assemble_floating_point_value_from_big_integer(
    big_integer          const& integer_value,
    uint32_t             const  integer_bits_of_precision,
    bool                 const  is_negative,
    bool                 const  has_nonzero_fractional_part,
    floating_point_value const& result
    ) throw()
{
    int32_t const base_exponent = result.mantissa_bits() - 1;

    // Very fast case:  If we have fewer than 64 bits of precision, we can just
    // take the two low order elements from the big_integer:
    if (integer_bits_of_precision <= 64)
    {
        int32_t const exponent = base_exponent;

        uint32_t const mantissa_low  = integer_value._used > 0 ? integer_value._data[0] : 0;
        uint32_t const mantissa_high = integer_value._used > 1 ? integer_value._data[1] : 0;
        uint64_t const mantissa =
            mantissa_low +
            (static_cast<uint64_t>(mantissa_high) << 32);

        return assemble_floating_point_value(mantissa, exponent, is_negative, !has_nonzero_fractional_part, result);
    }

    uint32_t const top_element_bits  = integer_bits_of_precision % 32;
    uint32_t const top_element_index = integer_bits_of_precision / 32;

    uint32_t const middle_element_index = top_element_index - 1;
    uint32_t const bottom_element_index = top_element_index - 2;

    // Pretty fast case:  If the top 64 bits occupy only two elements, we can
    // just combine those two elements:
    if (top_element_bits == 0)
    {
        int32_t const exponent = base_exponent + bottom_element_index * 32;

        uint64_t const mantissa =
            integer_value._data[bottom_element_index] +
            (static_cast<uint64_t>(integer_value._data[middle_element_index]) << 32);

        bool has_zero_tail = !has_nonzero_fractional_part;
        for (uint32_t i = 0; i != bottom_element_index; ++i)
        {
            has_zero_tail &= integer_value._data[i] == 0;
        }

        return assemble_floating_point_value(mantissa, exponent, is_negative, has_zero_tail, result);
    }

    // Not quite so fast case:  The top 64 bits span three elements in the big
    // integer.  Assemble the three pieces:
    uint32_t const top_element_mask  = (1u << top_element_bits) - 1;
    uint32_t const top_element_shift = 64 - top_element_bits; // Left

    uint32_t const middle_element_shift = top_element_shift - 32; // Left

    uint32_t const bottom_element_bits  = 32 - top_element_bits;
    uint32_t const bottom_element_mask  = ~top_element_mask;
    uint32_t const bottom_element_shift = 32 - bottom_element_bits; // Right

    int32_t const exponent = base_exponent + bottom_element_index * 32 + top_element_bits;

    uint64_t const mantissa =
        (static_cast<uint64_t>(integer_value._data[top_element_index] & top_element_mask)       << top_element_shift)    +
        (static_cast<uint64_t>(integer_value._data[middle_element_index])                       << middle_element_shift) +
        (static_cast<uint64_t>(integer_value._data[bottom_element_index] & bottom_element_mask) >> bottom_element_shift);

    bool has_zero_tail =
        !has_nonzero_fractional_part &&
        (integer_value._data[bottom_element_index] & top_element_mask) == 0;

    for (uint32_t i = 0; i != bottom_element_index; ++i)
    {
        has_zero_tail &= integer_value._data[i] == 0;
    }

    return assemble_floating_point_value(mantissa, exponent, is_negative, has_zero_tail, result);
}

// Accumulates the decimal digits in [first_digit, last_digit) into the result
// high precision integer.  This function assumes that no overflow will occur.
__forceinline void __cdecl accumulate_decimal_digits_into_big_integer(
    uint8_t const* const first_digit,
    uint8_t const* const last_digit,
    big_integer&         result
    ) throw()
{
    // We accumulate nine digit chunks, transforming the base ten string into
    // base one billion on the fly, allowing us to reduce the number of high
    // precision multiplication and addition operations by 8/9.
    uint32_t accumulator{};
    uint32_t accumulator_count{};
    for (uint8_t const* it = first_digit; it != last_digit; ++it)
    {
        if (accumulator_count == 9)
        {
            multiply(result, 1000 * 1000 * 1000);
            add(result, accumulator);

            accumulator       = 0;
            accumulator_count = 0;
        }

        accumulator *= 10;
        accumulator += *it;
        ++accumulator_count;
    }

    if (accumulator_count != 0)
    {
        multiply_by_power_of_ten(result, accumulator_count);
        add(result, accumulator);
    }
}

// The core floating point string parser for decimal strings.  After a subject
// string is parsed and converted into a floating_point_string object, if the
// subject string was determined to be a decimal string, the object is passed to
// this function.  This function converts the decimal real value to floating
// point.
inline SLD_STATUS __cdecl convert_decimal_string_to_floating_type_common(
    floating_point_string const& data,
    floating_point_value  const& result
    ) throw()
{
    // To generate an N bit mantissa we require N + 1 bits of precision.  The
    // extra bit is used to correctly round the mantissa (if there are fewer bits
    // than this available, then that's totally okay; in that case we use what we
    // have and we don't need to round).
    uint32_t const required_bits_of_precision = result.mantissa_bits() + 1;

    // The input is of the form 0.mantissa x 10^exponent, where 'mantissa' are
    // the decimal digits of the mantissa and 'exponent' is the decimal exponent.
    // We decompose the mantissa into two parts: an integer part and a fractional
    // part.  If the exponent is positive, then the integer part consists of the
    // first 'exponent' digits, or all present digits if there are fewer digits.
    // If the exponent is zero or negative, then the integer part is empty.  In
    // either case, the remaining digits form the fractional part of the mantissa.
    uint32_t       const positive_exponent      = static_cast<uint32_t>(__max(0, data._exponent));
    uint32_t       const integer_digits_present = __min(positive_exponent, data._mantissa_count);
    uint32_t       const integer_digits_missing = positive_exponent - integer_digits_present;
    uint8_t const* const integer_first          = data._mantissa;
    uint8_t const* const integer_last           = data._mantissa + integer_digits_present;

    uint8_t const* const fractional_first          = integer_last;
    uint8_t const* const fractional_last           = data._mantissa + data._mantissa_count;
    uint32_t       const fractional_digits_present = static_cast<uint32_t>(fractional_last - fractional_first);

    // First, we accumulate the integer part of the mantissa into a big_integer:
    big_integer integer_value{};
    accumulate_decimal_digits_into_big_integer(integer_first, integer_last, integer_value);

    if (integer_digits_missing > 0)
    {
        if (!multiply_by_power_of_ten(integer_value, integer_digits_missing))
        {
            assemble_floating_point_infinity(data._is_negative, result);
            return SLD_OVERFLOW;
        }
    }

    // At this point, the integer_value contains the value of the integer part
    // of the mantissa.  If either [1] this number has more than the required
    // number of bits of precision or [2] the mantissa has no fractional part,
    // then we can assemble the result immediately:
    uint32_t const integer_bits_of_precision = bit_scan_reverse(integer_value);
    if (integer_bits_of_precision >= required_bits_of_precision ||
        fractional_digits_present == 0)
    {
        return assemble_floating_point_value_from_big_integer(
            integer_value,
            integer_bits_of_precision,
            data._is_negative,
            fractional_digits_present != 0,
            result);
    }

    // Otherwise, we did not get enough bits of precision from the integer part,
    // and the mantissa has a fractional part.  We parse the fractional part of
    // the mantsisa to obtain more bits of precision.  To do this, we convert
    // the fractional part into an actual fraction N/M, where the numerator N is
    // computed from the digits of the fractional part, and the denominator M is
    // computed as the power of 10 such that N/M is equal to the value of the
    // fractional part of the mantissa.
    big_integer fractional_numerator{};
    accumulate_decimal_digits_into_big_integer(fractional_first, fractional_last, fractional_numerator);

    uint32_t const fractional_denominator_exponent = data._exponent < 0
        ? fractional_digits_present + static_cast<uint32_t>(-data._exponent)
        : fractional_digits_present;

    big_integer fractional_denominator = make_big_integer(1);
    if (!multiply_by_power_of_ten(fractional_denominator, fractional_denominator_exponent))
    {
        // If there were any digits in the integer part, it is impossible to
        // underflow (because the exponent cannot possibly be small enough),
        // so if we underflow here it is a true underflow and we return zero.
        assemble_floating_point_zero(data._is_negative, result);
        return SLD_UNDERFLOW;
    }

    // Because we are using only the fractional part of the mantissa here, the
    // numerator is guaranteed to be smaller than the denominator.  We normalize
    // the fraction such that the most significant bit of the numerator is in
    // the same position as the most significant bit in the denominator.  This
    // ensures that when we later shift the numerator N bits to the left, we
    // will produce N bits of precision.
    uint32_t const fractional_numerator_bits   = bit_scan_reverse(fractional_numerator);
    uint32_t const fractional_denominator_bits = bit_scan_reverse(fractional_denominator);

    uint32_t const fractional_shift = fractional_denominator_bits > fractional_numerator_bits
        ? fractional_denominator_bits - fractional_numerator_bits
        : 0;

    if (fractional_shift > 0)
    {
        shift_left(fractional_numerator, fractional_shift);
    }

    uint32_t const required_fractional_bits_of_precision =
        required_bits_of_precision -
        integer_bits_of_precision;

    uint32_t remaining_bits_of_precision_required = required_fractional_bits_of_precision;
    if (integer_bits_of_precision > 0)
    {
        // If the fractional part of the mantissa provides no bits of precision
        // and cannot affect rounding, we can just take whatever bits we got from
        // the integer part of the mantissa.  This is the case for numbers like
        // 5.0000000000000000000001, where the significant digits of the fractional
        // part start so far to the right that they do not affect the floating
        // point representation.
        //
        // If the fractional shift is exactly equal to the number of bits of
        // precision that we require, then no fractional bits will be part of the
        // result, but the result may affect rounding.  This is e.g. the case for
        // large, odd integers with a fractional part greater than or equal to .5.
        // Thus, we need to do the division to correctl round the result.
        if (fractional_shift > remaining_bits_of_precision_required)
        {
            return assemble_floating_point_value_from_big_integer(
                integer_value,
                integer_bits_of_precision,
                data._is_negative,
                fractional_digits_present != 0,
                result);
        }

        remaining_bits_of_precision_required -= fractional_shift;
    }

    // If there was no integer part of the mantissa, we will need to compute the
    // exponent from the fractional part.  The fractional exponent is the power
    // of two by which we must multiply the fractional part to move it into the
    // range [1.0, 2.0).  This will either be the same as the shift we computed
    // earlier, or one greater than that shift:
    uint32_t const fractional_exponent = fractional_numerator < fractional_denominator
        ? fractional_shift + 1
        : fractional_shift;

    shift_left(fractional_numerator, remaining_bits_of_precision_required);
    uint64_t fractional_mantissa = divide(fractional_numerator, fractional_denominator);

    bool has_zero_tail = fractional_numerator._used == 0;

    // We may have produced more bits of precision than were required.  Check,
    // and remove any "extra" bits:
    uint32_t const fractional_mantissa_bits = bit_scan_reverse(fractional_mantissa);
    if (fractional_mantissa_bits > required_fractional_bits_of_precision)
    {
        uint32_t const shift = fractional_mantissa_bits - required_fractional_bits_of_precision;
        has_zero_tail         = has_zero_tail && (fractional_mantissa & ((1ull << shift) - 1)) == 0;
        fractional_mantissa >>= shift;
    }

    // Compose the mantissa from the integer and fractional parts:
    uint32_t const integer_mantissa_low  = integer_value._used > 0 ? integer_value._data[0] : 0;
    uint32_t const integer_mantissa_high = integer_value._used > 1 ? integer_value._data[1] : 0;
    uint64_t const integer_mantissa =
        integer_mantissa_low +
        (static_cast<uint64_t>(integer_mantissa_high) << 32);

    uint64_t const complete_mantissa =
        (integer_mantissa << required_fractional_bits_of_precision) +
        fractional_mantissa;

    // Compute the final exponent:
    // * If the mantissa had an integer part, then the exponent is one less than
    //   the number of bits we obtained from the integer part.  (It's one less
    //   because we are converting to the form 1.11111, with one 1 to the left
    //   of the decimal point.)
    // * If the mantissa had no integer part, then the exponent is the fractional
    //   exponent that we computed.
    // Then, in both cases, we subtract an additional one from the exponent, to
    // account for the fact that we've generated an extra bit of precision, for
    // use in rounding.
    int32_t const final_exponent = integer_bits_of_precision > 0
        ? integer_bits_of_precision                          - 2
        : -static_cast<int32_t>(fractional_exponent)         - 1;

    return assemble_floating_point_value(complete_mantissa, final_exponent, data._is_negative, has_zero_tail, result);
}

template <typename FloatingType>
SLD_STATUS __cdecl convert_decimal_string_to_floating_type(
    floating_point_string const& data,
    FloatingType               & result
    ) throw()
{
    return convert_decimal_string_to_floating_type_common(data, floating_point_value(&result));
}

inline SLD_STATUS __cdecl convert_hexadecimal_string_to_floating_type_common(
    floating_point_string const& data,
    floating_point_value  const& result
    ) throw()
{
    uint64_t mantissa = 0;
    int32_t  exponent = data._exponent + result.mantissa_bits() - 1;

    // Accumulate bits into the mantissa buffer
    uint8_t const* const mantissa_last = data._mantissa + data._mantissa_count;
    uint8_t const*       mantissa_it   = data._mantissa;
    while (mantissa_it != mantissa_last && mantissa <= result.normal_mantissa_mask())
    {
        mantissa *= 16;
        mantissa += *mantissa_it++;
        exponent -=  4; // The exponent is in binary; log2(16) == 4
    }

    bool has_zero_tail = true;
    while (mantissa_it != mantissa_last && has_zero_tail)
    {
        has_zero_tail = has_zero_tail && *mantissa_it++ == 0;
    }

    return assemble_floating_point_value(mantissa, exponent, data._is_negative, has_zero_tail, result);
}

template <typename FloatingType>
SLD_STATUS __cdecl convert_hexadecimal_string_to_floating_type(
    floating_point_string const& data,
    FloatingType               & result
    ) throw()
{
    return convert_hexadecimal_string_to_floating_type_common(data, floating_point_value(&result));
}

// The high precision conversion algorithm defined above supports only float and
// double.  It does not support the 10- and 12-byte extended precision types.
// These types are supported only for legacy reasons, so we use the old, low-
// precision algorithm for these types, and do so by overloading the main
// conversion and assembly functions for _LDBL12.
void __cdecl assemble_floating_point_zero    (bool is_negative, _LDBL12& result) throw();
void __cdecl assemble_floating_point_infinity(bool is_negative, _LDBL12& result) throw();
void __cdecl assemble_floating_point_qnan    (bool is_negative, _LDBL12& result) throw();
void __cdecl assemble_floating_point_snan    (bool is_negative, _LDBL12& result) throw();
void __cdecl assemble_floating_point_ind     (_LDBL12& result) throw();

SLD_STATUS __cdecl convert_decimal_string_to_floating_type(
    floating_point_string const& data,
    _LDBL12                    & result
    ) throw();

SLD_STATUS __cdecl convert_hexadecimal_string_to_floating_type(
    floating_point_string const& data,
    _LDBL12                    & result
    ) throw();

template <typename Character, typename CharacterSource>
bool __cdecl parse_next_characters_from_source(
    Character const* const uppercase,
    Character const* const lowercase,
    size_t           const count,
    Character&             c,
    CharacterSource&       source
    ) throw()
{
    for (size_t i = 0; i != count; ++i)
    {
        if (c != uppercase[i] && c != lowercase[i])
        {
            return false;
        }

        c = source.get();
    }

    return true;
}

template <typename Character, typename CharacterSource>
bool __cdecl parse_floating_point_possible_nan_is_snan(
    Character&       c,
    CharacterSource& source
    ) throw()
{
    static Character const uppercase[] = { 'S', 'N', 'A', 'N', ')' };
    static Character const lowercase[] = { 's', 'n', 'a', 'n', ')' };
    return parse_next_characters_from_source(uppercase, lowercase, _countof(uppercase), c, source);
}

template <typename Character, typename CharacterSource>
bool __cdecl parse_floating_point_possible_nan_is_ind(
    Character&       c,
    CharacterSource& source
    ) throw()
{
    static Character const uppercase[] = { 'I', 'N', 'D', ')' };
    static Character const lowercase[] = { 'i', 'n', 'd', ')' };
    return parse_next_characters_from_source(uppercase, lowercase, _countof(uppercase), c, source);
}

template <typename Character, typename CharacterSource, typename StoredState>
floating_point_parse_result __cdecl parse_floating_point_possible_infinity(
    Character&            c,
    CharacterSource&      source,
    StoredState           stored_state
    ) throw()
{
    using char_type = typename CharacterSource::char_type;

    auto restore_state = [&]()
    {
        source.unget(c);
        c = '\0';
        return source.restore_state(stored_state);
    };

    static char_type const inf_uppercase[] = { 'I', 'N', 'F' };
    static char_type const inf_lowercase[] = { 'i', 'n', 'f' };
    if (!parse_next_characters_from_source(inf_uppercase, inf_lowercase, _countof(inf_uppercase), c, source))
    {
        return restore_state(), floating_point_parse_result::no_digits;
    }

    source.unget(c);
    stored_state = source.save_state();
    c = source.get();

    static char_type const inity_uppercase[] = { 'I', 'N', 'I', 'T', 'Y' };
    static char_type const inity_lowercase[] = { 'i', 'n', 'i', 't', 'y' };
    if (!parse_next_characters_from_source(inity_uppercase, inity_lowercase, _countof(inity_uppercase), c, source))
    {
        return restore_state()
            ? floating_point_parse_result::infinity
            : floating_point_parse_result::no_digits;
    }

    source.unget(c);
    return floating_point_parse_result::infinity;
}

template <typename Character, typename CharacterSource, typename StoredState>
floating_point_parse_result __cdecl parse_floating_point_possible_nan(
    Character&       c,
    CharacterSource& source,
    StoredState      stored_state
    ) throw()
{
    using char_type = typename CharacterSource::char_type;

    auto restore_state = [&]()
    {
        source.unget(c);
        c = '\0';
        return source.restore_state(stored_state);
    };

    static char_type const uppercase[] = { 'N', 'A', 'N' };
    static char_type const lowercase[] = { 'n', 'a', 'n' };
    if (!parse_next_characters_from_source(uppercase, lowercase, _countof(uppercase), c, source))
    {
        return restore_state(), floating_point_parse_result::no_digits;
    }

    source.unget(c);
    stored_state = source.save_state();
    c = source.get();

    if (c != '(')
    {
        return restore_state()
            ? floating_point_parse_result::qnan
            : floating_point_parse_result::no_digits;
    }

    c = source.get(); // Advance past the left parenthesis

    // After we've parsed a left parenthesis, test to see whether the parenthesized
    // string represents a signaling NaN "(SNAN)" or an indeterminate "(IND)".  If
    // so, we return the corresponding kind of NaN:
    if (parse_floating_point_possible_nan_is_snan(c, source))
    {
        source.unget(c);
        return floating_point_parse_result::snan;
    }

    if (parse_floating_point_possible_nan_is_ind(c, source))
    {
        source.unget(c);
        return floating_point_parse_result::indeterminate;
    }

    // Otherwise, we didn't match one of the two special parenthesized strings.
    // Keep eating chracters until we come across the right parenthesis or the
    // end of the character sequence:
    while (c != ')' && c != '\0')
    {
        if (!is_digit_or_nondigit(c))
        {
            return restore_state()
                ? floating_point_parse_result::qnan
                : floating_point_parse_result::no_digits;
        }

        c = source.get();
    }

    if (c != ')')
    {
        return restore_state()
            ? floating_point_parse_result::qnan
            : floating_point_parse_result::no_digits;
    }

    return floating_point_parse_result::qnan;
}

template <typename CharacterSource>
floating_point_parse_result __cdecl parse_floating_point_from_source(
    _locale_t             const  locale,
    CharacterSource            & source,
    floating_point_string      & fp_string
    ) throw()
{
    using char_type = typename CharacterSource::char_type;

    if (!source.validate())
    {
        return floating_point_parse_result::no_digits;
    }

    auto stored_state = source.save_state();
    char_type c{source.get()};

    auto restore_state = [&]()
    {
        source.unget(c);
        c = '\0';
        return source.restore_state(stored_state);
    };

    // Skip past any leading whitespace:
    while (is_space(c, locale))
    {
        c = source.get();
    }

    // Check for the optional plus or minus sign:
    fp_string._is_negative = c == '-';
    if (c == '-' || c == '+')
    {
        c = source.get();
    }

    // Handle special cases "INF" and "INFINITY" (these are the only accepted
    // character sequences that start with 'I'):
    if (c == 'I' || c == 'i')
    {
        return parse_floating_point_possible_infinity(c, source, stored_state);
    }

    // Handle special cases "NAN" and "NAN(...)" (these are the only accepted
    // character sequences that start with 'N'):
    if (c == 'N' || c == 'n')
    {
        return parse_floating_point_possible_nan(c, source, stored_state);
    }

    // Check for optional "0x" or "0X" hexadecimal base prefix:
    bool is_hexadecimal{false};
    if (c == '0')
    {
        auto const next_stored_state = source.save_state();

        char_type const next_c{source.get()};
        if (next_c == 'x' || next_c == 'X')
        {
            is_hexadecimal = true;
            c = source.get();

            // If we match the hexadecimal base prefix we update the state to
            // reflect that we consumed the leading zero to handle the case
            // where a valid mantissa does not follow the base prefix.  In this
            // case, the "0x" string is treated as a decimal zero subject ("0")
            // followed by a final string starting with the "x".
            stored_state = next_stored_state;
        }
        else
        {
            source.unget(next_c);
        }
    }

    uint8_t*       mantissa_first{fp_string._mantissa};
    uint8_t* const mantissa_last {fp_string._mantissa + _countof(fp_string._mantissa)};
    uint8_t*       mantissa_it   {fp_string._mantissa};

    // The exponent adjustment holds the number of digits in the mantissa buffer
    // that appeared before the radix point.  It is positive for number strings
    // with an integer component and negative for number strings without.
    int exponent_adjustment{0};

    // Track whether we've seen any digits, so we know whether we've successfully
    // parsed something:
    bool found_digits{false};

    // Skip past any leading zeroes in the mantissa:
    while (c == '0')
    {
        found_digits = true;
        c = source.get();
    }

    // Scan the integer part of the mantissa:
    for (; ; c = source.get())
    {
        unsigned const max_digit_value{is_hexadecimal ? 0xfu : 9u};

        unsigned const digit_value{parse_digit(c)};
        if (digit_value > max_digit_value)
        {
            break;
        }

        found_digits = true;
        if (mantissa_it != mantissa_last)
        {
            *mantissa_it++ = static_cast<uint8_t>(digit_value);
        }

        ++exponent_adjustment;
    }

    // If a radix point is present, scan the fractional part of the mantissa:
    char const radix_point{*locale->locinfo->lconv->decimal_point};
    if (c == radix_point)
    {
        c = source.get();

        // If we haven't yet scanned any nonzero digits, continue skipping over
        // zeroes, updating the exponent adjustment to account for the zeroes
        // we are skipping:
        if (mantissa_it == mantissa_first)
        {
            while (c == '0')
            {
                found_digits = true;
                --exponent_adjustment;
                c = source.get();
            }
        }

        for (; ; c = source.get())
        {
            unsigned const max_digit_value{is_hexadecimal ? 0xfu : 9u};

            unsigned const digit_value{parse_digit(c)};
            if (digit_value > max_digit_value)
                break;

            found_digits = true;
            if (mantissa_it != mantissa_last)
            {
                *mantissa_it++ = static_cast<uint8_t>(digit_value);
            }
        }
    }

    if (!found_digits)
    {
        // We failed to parse any digits, so attempt to restore back to the last
        // good terminal state.  This may fail if we are reading from a stream,
        // we read a hexadecimal base prefix ("0x"), but we did not find any digits
        // following the base prefix.
        if (!restore_state())
        {
            return floating_point_parse_result::no_digits;
        }

        // If a hexadecimal base prefix was present ("0x"), then the string is a
        // valid input:  the "0" is the subject sequence and the "x" is the first
        // character of the final string.  Otherwise, the string is not a valid
        // input.
        if (is_hexadecimal)
        {
            return floating_point_parse_result::zero;
        }
        else
        {
            return floating_point_parse_result::no_digits;
        }
    }

    source.unget(c);
    stored_state = source.save_state();
    c = source.get();

    // Check for the optional 'e' or 'p' exponent introducer:
    bool has_exponent{false};
    switch (c)
    {
    case 'e':
    case 'E':
        has_exponent = !is_hexadecimal;
        break;

    case 'p':
    case 'P':
        has_exponent = is_hexadecimal;
        break;
    }

    // If there was an exponent introducer, scan the exponent:
    int exponent{0};
    if (has_exponent)
    {
        c = source.get(); // Skip past exponent introducer character

        // Check for the optional plus or minus sign:
        bool const exponent_is_negative{c == '-'};
        if (c == '+' || c == '-')
        {
            c = source.get();
        }

        bool has_exponent_digits{false};

        while (c == '0')
        {
            has_exponent_digits = true;
            c = source.get();
        }

        for (; ; c = source.get())
        {
            unsigned const digit_value{parse_digit(c)};
            if (digit_value >= 10)
            {
                break;
            }

            has_exponent_digits = true;
            exponent = exponent * 10 + digit_value;
            if (exponent > maximum_temporary_decimal_exponent)
            {
                exponent = maximum_temporary_decimal_exponent + 1;
                break;
            }
        }

        // If the exponent is too large, skip over the remaining exponent digits
        // so we can correctly update the end pointer:
        while (parse_digit(c) < 10)
        {
            c = source.get();
        }

        if (exponent_is_negative)
        {
            exponent = -exponent;
        }

        // If no exponent digits were present, attempt to restore the last good
        // terminal state.
        if (!has_exponent_digits)
        {
            if (restore_state())
            {
                // The call to restore_state will have ungotten the exponent
                // introducer.  Re-get this character, to restore us to the
                // state we had before we entered the exponent parsing block.
                c = source.get();
            }
            else
            {
                return floating_point_parse_result::no_digits;
            }
        }
    }

    // Unget the last character that we read that terminated input.  After this
    // point, we must not use the source, c, or stored_state.
    source.unget(c);

    // Remove trailing zeroes from mantissa:
    while (mantissa_it != mantissa_first && *(mantissa_it - 1) == 0)
    {
        --mantissa_it;
    }

    // If the mantissa buffer is empty, the mantissa was composed of all zeroes
    // (so the mantissa is 0).  All such strings have the value zero, regardless
    // what the exponent is (because 0 x b^n == 0 for all b and n).  We can return
    // now.  Note that we defer this check until after we scan the exponent, so
    // that we can correctly update end_ptr to point past the end of the exponent.
    if (mantissa_it == mantissa_first)
    {
        return floating_point_parse_result::zero;
    }

    // Before we adjust the exponent, handle the case where we detected a wildly
    // out of range exponent during parsing and clamped the value:
    if (exponent > maximum_temporary_decimal_exponent)
    {
        return floating_point_parse_result::overflow;
    }

    if (exponent < minimum_temporary_decimal_exponent)
    {
        return floating_point_parse_result::underflow;
    }

    // In hexadecimal floating constants, the exponent is a base 2 exponent.  The
    // exponent adjustment computed during parsing has the same base as the
    // mantissa (so, 16 for hexadecimal floating constants).  We therefore need to
    // scale the base 16 multiplier to base 2 by multiplying by log2(16):
    int const exponent_adjustment_multiplier{is_hexadecimal ? 4 : 1};

    exponent += exponent_adjustment * exponent_adjustment_multiplier;

    // Verify that after adjustment the exponent isn't wildly out of range (if
    // it is, it isn't representable in any supported floating point format).
    if (exponent > maximum_temporary_decimal_exponent)
    {
        return floating_point_parse_result::overflow;
    }

    if (exponent < minimum_temporary_decimal_exponent)
    {
        return floating_point_parse_result::underflow;
    }

    fp_string._exponent       = exponent;
    fp_string._mantissa_count = static_cast<uint32_t>(mantissa_it - mantissa_first);

    return is_hexadecimal
        ? floating_point_parse_result::hexadecimal_digits
        : floating_point_parse_result::decimal_digits;
}

template <typename FloatingType>
SLD_STATUS __cdecl parse_floating_point_write_result(
    floating_point_parse_result const  parse_result,
    floating_point_string       const& fp_string,
    FloatingType*               const  result
    ) throw()
{
    switch (parse_result)
    {
    case floating_point_parse_result::decimal_digits:     return convert_decimal_string_to_floating_type    (fp_string, *result);
    case floating_point_parse_result::hexadecimal_digits: return convert_hexadecimal_string_to_floating_type(fp_string, *result);

    case floating_point_parse_result::zero:          assemble_floating_point_zero    (fp_string._is_negative, *result); return SLD_OK;
    case floating_point_parse_result::infinity:      assemble_floating_point_infinity(fp_string._is_negative, *result); return SLD_OK;
    case floating_point_parse_result::qnan:          assemble_floating_point_qnan    (fp_string._is_negative, *result); return SLD_OK;
    case floating_point_parse_result::snan:          assemble_floating_point_snan    (fp_string._is_negative, *result); return SLD_OK;
    case floating_point_parse_result::indeterminate: assemble_floating_point_ind     (                        *result); return SLD_OK;

    case floating_point_parse_result::no_digits:     assemble_floating_point_zero    (false,                  *result); return SLD_NODIGITS;
    case floating_point_parse_result::underflow:     assemble_floating_point_zero    (fp_string._is_negative, *result); return SLD_UNDERFLOW;
    case floating_point_parse_result::overflow:      assemble_floating_point_infinity(fp_string._is_negative, *result); return SLD_OVERFLOW;

    default:
        // Unreachable
        _ASSERTE(false);
        return SLD_NODIGITS;
    }
}

template <typename CharacterSource, typename FloatingType>
SLD_STATUS __cdecl parse_floating_point(
    _locale_t       const locale,
    CharacterSource       source,
    FloatingType*   const result
    ) throw()
{
    using char_type = typename CharacterSource::char_type;

    _VALIDATE_RETURN(result != nullptr, EINVAL, SLD_NODIGITS);
    _VALIDATE_RETURN(locale != nullptr, EINVAL, SLD_NODIGITS);

    // PERFORMANCE NOTE:  fp_string is intentionally left uninitialized.  Zero-
    // initialization is quite expensive and is unnecessary.  The benefit of not
    // zero-initializing is greatest for short inputs.
    floating_point_string fp_string;

    floating_point_parse_result const parse_result = parse_floating_point_from_source(locale, source, fp_string);

    return parse_floating_point_write_result(parse_result, fp_string, result);
}

} // namespace __crt_strtox



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Character Sources
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace __crt_strtox {

template <typename Character>
class c_string_character_source
{
public:

    typedef Character char_type;

    c_string_character_source(
        Character const*  const string,
        Character const** const end
        ) throw()
        : _p{string}, _end{end}
    {
        if (end)
        {
            *end = string;
        }
    }

    c_string_character_source(c_string_character_source&& other) throw()
        : _p{other._p}, _end{other._end}
    {
        other._p   = nullptr;
        other._end = nullptr;
    }

    c_string_character_source& operator=(c_string_character_source&& other) throw()
    {
        _p   = other._p;
        _end = other._end;

        other._p   = nullptr;
        other._end = nullptr;
        return *this;
    }

    ~c_string_character_source()
    {
        if (_end)
        {
            *_end = _p;
        }
    }

    bool validate() const throw()
    {
        _VALIDATE_RETURN(_p != nullptr, EINVAL, false);
        return true;
    }

    Character get() throw()
    {
        return *_p++;
    }

    void unget(Character const c) throw()
    {
        --_p;
        _VALIDATE_RETURN_VOID(c == '\0' || *_p == c, EINVAL);
    }

    Character const* save_state() const throw()
    {
        return _p;
    }

    bool restore_state(Character const* const state) throw()
    {
        _p = state;
        return true;
    }

private:

    c_string_character_source(c_string_character_source const&) = delete;
    c_string_character_source& operator=(c_string_character_source const&) = delete;

    Character const*  _p;
    Character const** _end;
};

template <typename Character, typename EndPointer>
c_string_character_source<Character> __cdecl make_c_string_character_source(
    Character const* const string,
    EndPointer       const end
    ) throw()
{
    return c_string_character_source<Character>(string, (Character const**)(end));
}

template <typename Integer, typename Character, typename EndPointer>
__forceinline Integer __cdecl parse_integer_from_string(
    Character const* const string,
    EndPointer       const end,
    int              const base,
    _locale_t        const locale
    ) throw()
{
    __crt_cached_ptd_host ptd{locale};

    return static_cast<Integer>(parse_integer<typename make_unsigned<Integer>::type>(
        ptd,
        make_c_string_character_source(string, end),
        base,
        is_signed<Integer>::value));
}

template <typename InputAdapter>
class input_adapter_character_source
{
public:

    typedef typename InputAdapter::char_type    char_type;
    typedef __acrt_stdio_char_traits<char_type> traits;

    input_adapter_character_source(
        InputAdapter* const input_adapter,
        uint64_t      const width,
        bool*         const succeeded
        ) throw()
        : _input_adapter{input_adapter},
          _max_get_count{width        },
          _get_count    {0            },
          _succeeded    {succeeded    }
    {
        if (succeeded)
            *succeeded = true;
    }

    input_adapter_character_source(input_adapter_character_source&& other) throw()
        : _input_adapter{other._input_adapter},
          _max_get_count{other._max_get_count},
          _get_count    {other._get_count    },
          _succeeded    {other._succeeded    }
    {
        other._input_adapter = nullptr;
        other._succeeded     = nullptr;
    }

    input_adapter_character_source& operator=(input_adapter_character_source&& other) throw()
    {
        _input_adapter = other._input_adapter;
        _max_get_count = other._max_get_count;
        _get_count     = other._get_count;
        _succeeded     = other._succeeded;

        other._input_adapter = nullptr;
        other._succeeded     = nullptr;
        return *this;
    }

    ~input_adapter_character_source()
    {
        // If no characters were consumed, report as a failure.  This occurs if
        // a matching failure occurs on the first character (if it occurs on a
        // subsequent character, then the attempt to restore state will have
        // failed and will set the failure flag).
        if (_succeeded != nullptr && _get_count == 0)
        {
            *_succeeded = false;
        }
    }

    bool validate() const throw()
    {
        _VALIDATE_RETURN(_input_adapter != nullptr, EINVAL, false);
        _VALIDATE_RETURN(_succeeded     != nullptr, EINVAL, false);
        return true;
    }

    char_type get() throw()
    {
        ++_get_count;

        if (_max_get_count != 0 && _get_count > _max_get_count)
            return '\0';

        auto c = _input_adapter->get();
        if (c == traits::eof)
            return '\0';

        return static_cast<char_type>(c);
    }

    void unget(char_type const c) throw()
    {
        --_get_count;

        if (_max_get_count != 0 && _get_count > _max_get_count)
            return;

        if (c == '\0' || c == traits::eof)
            return;

        _input_adapter->unget(c);
    }

    uint64_t save_state() const throw()
    {
        return _get_count;
    }

    bool restore_state(uint64_t const get_count) throw()
    {
        if (get_count != _get_count)
        {
            *_succeeded = false;
            return false;
        }

        return true;
    }

private:

    input_adapter_character_source(input_adapter_character_source const&) = delete;
    input_adapter_character_source& operator=(input_adapter_character_source const&) = delete;

    InputAdapter* _input_adapter;
    uint64_t      _max_get_count;
    uint64_t      _get_count;
    bool*         _succeeded;
};

template <typename InputAdapter>
input_adapter_character_source<InputAdapter> __cdecl make_input_adapter_character_source(
    InputAdapter* const input_adapter,
    uint64_t      const width,
    bool*         const succeeded
    ) throw()
{
    return input_adapter_character_source<InputAdapter>{input_adapter, width, succeeded};
}

} // namespace __crt_strtox

// Internal-only variations of the above functions

// Note this is different from a usual tcstol call in that whitespace is not
// trimmed in order to avoid acquiring the global locale for code paths that
// do not require that functionality.
template <typename Character, typename EndPointer, bool TrimWhitespace = false>
__forceinline long __cdecl _tcstol_internal(
    __crt_cached_ptd_host&   ptd,
    Character const*   const string,
    EndPointer         const end,
    int                const base
    ) throw()
{
    return static_cast<long>(__crt_strtox::parse_integer<unsigned long, __crt_strtox::c_string_character_source<Character>, TrimWhitespace>(
        ptd,
        __crt_strtox::make_c_string_character_source(string, end),
        base,
        true // long is signed
        ));
}

#pragma pop_macro("_INVALID_PARAMETER")
#pragma pop_macro("_VALIDATE_RETURN")
#pragma pop_macro("_VALIDATE_RETURN_VOID")

