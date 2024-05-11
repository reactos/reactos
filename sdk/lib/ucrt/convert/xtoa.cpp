//
// _ctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions for converting integers to strings.
//
#include <corecrt_internal.h>
#include <corecrt_internal_securecrt.h>
#include <limits.h>
#include <stdlib.h>

#pragma warning(disable:__WARNING_NOT_SATISFIED) // 28020 Prefast thinks that 18446744073709551615 < 1.
#pragma warning(disable:__WARNING_RANGE_PRECONDITION_VIOLATION) // 26060 Prefast thinks that 18446744073709551615 < 1.

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Common Conversion Implementation
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace
{
    template <typename T> struct make_signed;
    template <> struct make_signed<unsigned long>    { typedef long    type; };
    template <> struct make_signed<unsigned __int64> { typedef __int64 type; };
}



template <typename UnsignedInteger, typename Character>
_Success_(return == 0)
static errno_t __cdecl common_xtox(
                                    UnsignedInteger const   original_value,
    _Out_writes_z_(buffer_count)    Character*      const   buffer,
    _When_(is_negative == true, _In_range_(>=, 2)) _In_range_(>=, 1)
                                    size_t          const   buffer_count,
                                    unsigned        const   radix,
                                    bool            const   is_negative
    ) throw()
{
// OACR isn't able to track that p stays within the bounds of [buffer, buffer + buffer_count) so manually verified and disabled warning
#pragma warning(push)
#pragma warning(disable:26014)
    Character* p      = buffer; // pointer to traverse the string
    size_t     length = 0;      // current length of string

    UnsignedInteger remaining_value = original_value;;

    if (is_negative)
    {
        *p++ = '-';
        ++length;

        remaining_value = static_cast<UnsignedInteger>(
            -static_cast<typename make_signed<UnsignedInteger>::type>(remaining_value)
        );
    }

    Character* first_digit = p;

    do
    {
        unsigned const digit = static_cast<unsigned>(remaining_value % radix);
        remaining_value /= radix;

        // Convert to ASCII and store:
        if (digit > 9)
        {
            *p++ = static_cast<Character>(digit - 10 + 'a');
        }
        else
        {
            *p++ = static_cast<Character>(digit + '0');
        }

        ++length;
    }
    while (remaining_value > 0 && length < buffer_count);

    if (length >= buffer_count)
    {
        buffer[0] = '\0';
        _VALIDATE_RETURN_ERRCODE(length < buffer_count, ERANGE);
    }

    // We now have the digits of the number in the buffer, but in reverse order.
    // Reverse the order, but first terminate the string:
    *p-- = '\0';

    do
    {
        Character const t = *p;
        *p = *first_digit;
        *first_digit = t;
        --p;
        ++first_digit;
    }
    while (first_digit < p);

    return 0;
#pragma warning(pop)
}

template <typename UnsignedInteger, typename Character>
_Success_(return == 0)
static errno_t __cdecl common_xtox_s(
                                    UnsignedInteger const   value,
    _Out_writes_z_(buffer_count)    Character*      const   buffer,
                                    size_t          const   buffer_count,
                                    unsigned        const   radix,
                                    bool            const   is_negative
    ) throw()
{
    _VALIDATE_RETURN_ERRCODE(buffer != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(buffer_count > 0,  EINVAL);
    _RESET_STRING(buffer, buffer_count);
    _VALIDATE_RETURN_ERRCODE(buffer_count > static_cast<size_t>(is_negative ? 2 : 1), ERANGE);
    _VALIDATE_RETURN_ERRCODE(2 <= radix && radix <= 36, EINVAL);

    return common_xtox(value, buffer, buffer_count, radix, is_negative);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// 32-bit Integers => Narrow Strings
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" errno_t __cdecl _itoa_s(
    int    const value,
    char*  const buffer,
    size_t const buffer_count,
    int    const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned long>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ltoa_s(
    long   const value,
    char*  const buffer,
    size_t const buffer_count,
    int    const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned long>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ultoa_s(
    unsigned long   const value,
    char*           const buffer,
    size_t          const buffer_count,
    int             const radix
    )
{
    return common_xtox_s(value, buffer, buffer_count, radix, false);
}



extern "C" char* __cdecl _itoa(
    int   const value,
    char* const buffer,
    int   const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned long>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" char* __cdecl _ltoa(
    long  const value,
    char* const buffer,
    int   const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned long>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" char* __cdecl _ultoa(
    unsigned long const value,
    char*         const buffer,
    int           const radix
    )
{
    common_xtox(value, buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, false);
    return buffer;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// 64-bit Integers => Narrow Strings
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" errno_t __cdecl _i64toa_s(
    __int64 const value,
    char*   const buffer,
    size_t  const buffer_count,
    int     const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned __int64>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ui64toa_s(
    unsigned __int64 const value,
    char*            const buffer,
    size_t           const buffer_count,
    int              const radix
    )
{
    return common_xtox_s(value, buffer, buffer_count, radix, false);
}



extern "C" char* __cdecl _i64toa(
    __int64 const value,
    char*   const buffer,
    int     const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned __int64>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" char* __cdecl _ui64toa(
    unsigned __int64 const value,
    char*            const buffer,
    int              const radix
    )
{
    common_xtox(value, buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, false);
    return buffer;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// 32-bit Integers => Wide Strings
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" errno_t __cdecl _itow_s(
    int      const value,
    wchar_t* const buffer,
    size_t   const buffer_count,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned long>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ltow_s(
    long     const value,
    wchar_t* const buffer,
    size_t   const buffer_count,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned long>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ultow_s(
    unsigned long const value,
    wchar_t*      const buffer,
    size_t        const buffer_count,
    int           const radix
    )
{
    return common_xtox_s(value, buffer, buffer_count, radix, false);
}



extern "C" wchar_t* __cdecl _itow(
    int      const value,
    wchar_t* const buffer,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned long>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" wchar_t* __cdecl _ltow(
    long     const value,
    wchar_t* const buffer,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned long>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" wchar_t* __cdecl _ultow(
    unsigned long const value,
    wchar_t*      const buffer,
    int           const radix
    )
{
    common_xtox(value, buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, false);
    return buffer;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// 64-bit Integers => Wide Strings
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" errno_t __cdecl _i64tow_s(
    __int64  const value,
    wchar_t* const buffer,
    size_t   const buffer_count,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    return common_xtox_s(static_cast<unsigned __int64>(value), buffer, buffer_count, radix, is_negative);
}

extern "C" errno_t __cdecl _ui64tow_s(
    unsigned __int64 const value,
    wchar_t*         const buffer,
    size_t           const buffer_count,
    int              const radix
    )
{
    return common_xtox_s(value, buffer, buffer_count, radix, false);
}



extern "C" wchar_t* __cdecl _i64tow(
    __int64  const value,
    wchar_t* const buffer,
    int      const radix
    )
{
    bool const is_negative = radix == 10 && value < 0;
    common_xtox(static_cast<unsigned __int64>(value), buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, is_negative);
    return buffer;
}

extern "C" wchar_t* __cdecl _ui64tow(
    unsigned __int64 const value,
    wchar_t*         const buffer,
    int              const radix
    )
{
    common_xtox(value, buffer, _CRT_UNBOUNDED_BUFFER_SIZE, radix, false);
    return buffer;
}
