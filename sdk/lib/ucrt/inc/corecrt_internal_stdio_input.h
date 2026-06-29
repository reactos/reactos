//
// corecrt_internal_stdio_input.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file defines the core implementation of the formatted output functions,
// including scanf and its many variants (sscanf, fscanf, etc.).
//
#include <conio.h>
#include <ctype.h>
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_strtox.h>
#include <locale.h>
#include <stdarg.h>



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Input Adapters
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The actual read operations are different depending on whether the source is a
// stream or the console.  We handle these differences via input adapters.  The
// stream and console I/O functions pass an adapter to the core input function,
// and that function calls various read members of the adapter to perform read
// operations.
namespace __crt_stdio_input {

template <typename Character>
class console_input_adapter
{
public:

    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using int_type           = typename traits::int_type;

    console_input_adapter() throw()
        : _characters_read{0}
    {
    }

    bool validate() const throw()
    {
        return true;
    }

    int_type get() throw()
    {
        int_type const c{traits::gettche_nolock()};
        if (c != traits::eof)
            ++_characters_read;

        return c;
    }

    void unget(int_type const c) throw()
    {
        if (c == traits::eof)
            return;

        --_characters_read;
        traits::ungettch_nolock(c);
    }

    size_t characters_read() const throw()
    {
        return _characters_read;
    }

private:

    size_t _characters_read;
};

template <typename Character>
class stream_input_adapter
{
public:

    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using int_type           = typename traits::int_type;

    stream_input_adapter(FILE* const public_stream) throw()
        : _stream         {public_stream},
          _characters_read{0            }
    {
    }

    bool validate() const throw()
    {
        _VALIDATE_RETURN(_stream.valid(), EINVAL, false);

        return traits::validate_stream_is_ansi_if_required(_stream.public_stream());
    }

    int_type get() throw()
    {
        int_type const c{traits::getc_nolock(_stream.public_stream())};
        if (c != traits::eof)
            ++_characters_read;

        return c;
    }

    void unget(int_type const c) throw()
    {
        if (c == traits::eof)
            return;

        --_characters_read;
        traits::ungettc_nolock(c, _stream.public_stream());
    }

    size_t characters_read() const throw()
    {
         return _characters_read;
    }

private:

    __crt_stdio_stream _stream;
    size_t             _characters_read;
};

template <typename Character>
class string_input_adapter
{
public:

    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using unsigned_char_type = typename traits::unsigned_char_type;
    using int_type           = typename traits::int_type;

    string_input_adapter(
        char_type const* const string,
        size_t           const length
        ) throw()
        : _first{string}, _last{string + length}, _it{string}
    {
    }

    bool validate() throw()
    {
        _VALIDATE_RETURN(_it != nullptr, EINVAL, false);
        _VALIDATE_RETURN(_it <= _last,   EINVAL, false);
        return true;
    }

    int_type get() throw()
    {
        if (_it == _last)
            return traits::eof;

        // If we are processing narrow characters, the character value may be
        // negative.  In this case, its value will have been sign extended in
        // the conversion to int_type.  Mask the sign extension bits for
        // compatibility with the other input adapters:
        return static_cast<unsigned_char_type>(*_it++);
    }

    void unget(int_type const c) throw()
    {
        if (_it == _first)
            return;

        if (_it == _last && c == traits::eof)
            return;

        --_it;
    }

    size_t characters_read() const throw()
    {
        return static_cast<size_t>(_it - _first);
    }

private:

    char_type const* _first;
    char_type const* _last;
    char_type const* _it;
};



// Eats whitespace characters from an input adapter.  When it returns, the next
// call to get() on the input adapter will be either a non-whitespace character
// or will be EOF.
template <template <typename> class InputAdapter, typename Character>
typename __acrt_stdio_char_traits<Character>::int_type __cdecl skip_whitespace(
    InputAdapter<Character>&       adapter,
    _locale_t                const locale
    )
{
    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using unsigned_char_type = typename traits::unsigned_char_type;
    using int_type           = typename traits::int_type;

    int_type c;

    do
    {
        c = adapter.get();

        if (c == traits::eof)
            break;
    }
    while (__crt_strtox::is_space(static_cast<char_type>(c), locale));

    return c;
}

} // namespace __crt_stdio_input



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Scanset Buffer
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace __crt_stdio_input {



// The scanset buffer provides storage for a scanset (a set of characters to be
// matched).  We need a bitfield with one bit per character, so for char we can
// use a member buffer but for wchar_t we need to dynamically allocate a buffer
// on first use to avoid excessive stack usage.
template <size_t CharacterSize>
class scanset_storage;

template <>
class scanset_storage<sizeof(char)>
{
public:

    scanset_storage()
        : _buffer{}
    {
    }

    unsigned char* data() const throw() { return _buffer;     }
    size_t         size() const throw() { return buffer_size; }

private:

    enum : size_t { buffer_size = (static_cast<size_t>(UCHAR_MAX) + 1) / CHAR_BIT };

    mutable unsigned char _buffer[buffer_size];
};

template <>
class scanset_storage<sizeof(wchar_t)>
{
public:

    unsigned char* data() const throw()
    {
        if (!_buffer)
            _buffer = _calloc_crt_t(unsigned char, buffer_size);

        return _buffer.get();
    }

    size_t size() const throw()
    {
        return buffer_size;
    }

private:

    enum : size_t { buffer_size = (static_cast<size_t>(WCHAR_MAX) + 1) / CHAR_BIT };

    mutable __crt_unique_heap_ptr<unsigned char> _buffer;
};

template <typename UnsignedCharacter>
class scanset_buffer
{
public:

    bool is_usable() throw()
    {
        return _storage.data() != nullptr;
    }

    void set(UnsignedCharacter const c) throw()
    {
        _storage.data()[c / CHAR_BIT] |= 1 << (c % CHAR_BIT);
    }

    bool test(UnsignedCharacter const c) const throw()
    {
        return (_storage.data()[c / CHAR_BIT] & 1 << (c % CHAR_BIT)) != 0;
    }

    void reset() throw()
    {
        unsigned char* const first{_storage.data()};
        if (!first)
            return;

        ::memset(first, 0, _storage.size());
    }

    void invert() throw()
    {
        unsigned char* const first{_storage.data()        };
        unsigned char* const last {first + _storage.size()};

        for (unsigned char* it{first}; it != last; ++it)
        {
            *it ^= static_cast<unsigned char>(-1);
        }
    }

private:

    scanset_storage<sizeof(UnsignedCharacter)> _storage;
};

} // namespace __crt_stdio_input



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Format String Parser
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
namespace __crt_stdio_input {

enum class format_directive_kind
{
    unknown_error,
    end_of_string,
    whitespace,
    literal_character,
    conversion_specifier
};

enum class length_modifier
{
    none,
    hh,
    h,
    l,
    ll,
    j,
    z,
    t,
    L,
    I32,
    I64,
    T,
    enumerator_count
};

inline size_t __cdecl to_integer_length(length_modifier const length) throw()
{
    switch (length)
    {
    case length_modifier::none: return sizeof(int      );
    case length_modifier::hh:   return sizeof(char     );
    case length_modifier::h:    return sizeof(short    );
    case length_modifier::l:    return sizeof(long     );
    case length_modifier::ll:   return sizeof(long long);
    case length_modifier::j:    return sizeof(intmax_t );
    case length_modifier::z:    return sizeof(size_t   );
    case length_modifier::t:    return sizeof(ptrdiff_t);
    case length_modifier::I32:  return sizeof(int32_t  );
    case length_modifier::I64:  return sizeof(int64_t  );

    default:
        _ASSERTE(("Unexpected length specifier", false));
        return 0;
    }
}

inline size_t __cdecl to_floating_point_length(length_modifier const length) throw()
{
    switch (length)
    {
    case length_modifier::none: return sizeof(float      );
    case length_modifier::l:    return sizeof(double     );
    case length_modifier::L:    return sizeof(long double);

    default:
        _ASSERTE(("Unexpected length specifier", false));
        return 0;
    }
}

enum class conversion_mode
{
    character,
    string,

    signed_unknown,
    signed_decimal,
    unsigned_octal,
    unsigned_decimal,
    unsigned_hexadecimal,

    floating_point,

    scanset,

    report_character_count,

    enumerator_count
};

inline bool __cdecl is_length_valid(conversion_mode const mode, length_modifier const length) throw()
{
    static unsigned char const constraints
        [static_cast<size_t>(conversion_mode::enumerator_count)]
        [static_cast<size_t>(length_modifier::enumerator_count)]
    {
                                     /* none hh   h    l    ll   j    z    t    L    I32  I64  T */
        /* character              */ {  1  , 0  , 1  , 1  , 1  , 0  , 0  , 0  , 1  , 0  , 0  , 1  },
        /* string                 */ {  1  , 0  , 1  , 1  , 1  , 0  , 0  , 0  , 1  , 0  , 0  , 1  },
        /* signed decimal         */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  },
        /* signed unknown         */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  },
        /* unsigned octal         */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  },
        /* unsigned decimal       */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  },
        /* unsigned hexadecimal   */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  },
        /* floating point         */ {  1  , 0  , 0  , 1  , 0  , 0  , 0  , 0  , 1  , 0  , 0  , 0  },
        /* scanset                */ {  1  , 0  , 0  , 1  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 1  },
        /* report character count */ {  1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 0  , 1  , 1  , 0  }
    };

    return constraints[static_cast<size_t>(mode)][static_cast<size_t>(length)] != 0;
}

template <size_t N> struct uintn_t_impl;
template <> struct uintn_t_impl<4> { typedef uint32_t type; };
template <> struct uintn_t_impl<8> { typedef uint64_t type; };

template <size_t N>
using uintn_t = typename uintn_t_impl<N>::type;

template <typename Character>
class format_string_parser
{
public:

    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using unsigned_char_type = typename traits::unsigned_char_type;
    using int_type           = typename traits::int_type;

    format_string_parser(
        _In_   uint64_t                  const options,
        _In_z_ unsigned_char_type const* const format_it
        ) throw()
        : _options   {options  },
          _format_it {format_it},
          _error_code{0        }
    {
        reset_token_state();
    }

    bool validate() const throw()
    {
        _VALIDATE_RETURN(_format_it != nullptr, EINVAL, false);
        return true;
    }

    bool advance() throw()
    {
        if (_error_code != 0)
        {
            return false;
        }

        reset_token_state();

        if (*_format_it == '\0')
        {
            _kind = format_directive_kind::end_of_string;
            return false;
        }

        if (traits::istspace(*_format_it))
        {
            _kind = format_directive_kind::whitespace;

            while (traits::istspace(*_format_it))
                ++_format_it;

            return true;
        }

        if (_format_it[0] != '%' || _format_it[1] == '%')
        {
            _kind = format_directive_kind::literal_character;
            _literal_character_lead = *_format_it;
            _format_it += _format_it[0] != '%' ? 1 : 2;

            return scan_optional_literal_character_trail_bytes_tchar(char_type());
        }

        _kind = format_directive_kind::conversion_specifier;
        ++_format_it; // Advance past the %

        scan_optional_assignment_suppressor();

        if (!scan_optional_field_width())
        {
            return false;
        }

        scan_optional_length_modifier();
        scan_optional_wide_modifier();

        if (!scan_conversion_specifier())
        {
            return false;
        }

        if (!is_length_valid(_mode, _length))
        {
            reset_token_state_for_error(EINVAL);
            return false;
        }

        return true;
    }

    errno_t error_code() const throw()
    {
        return _error_code;
    }

    format_directive_kind kind() const throw()
    {
        return _kind;
    }

    unsigned_char_type literal_character_lead() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::literal_character);
        return _literal_character_lead;
    }

    unsigned_char_type literal_character_trail() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::literal_character);
        return _literal_character_trail;
    }

    bool suppress_assignment() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::conversion_specifier);
        return _suppress_assignment;
    }

    uint64_t width() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::conversion_specifier);
        return _width;
    }

    size_t length() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::conversion_specifier);
        switch (_mode)
        {
        case conversion_mode::character:
        case conversion_mode::string:
        case conversion_mode::scanset:
            return _is_wide ? sizeof(wchar_t) : sizeof(char);

        case conversion_mode::signed_unknown:
        case conversion_mode::signed_decimal:
        case conversion_mode::unsigned_octal:
        case conversion_mode::unsigned_decimal:
        case conversion_mode::unsigned_hexadecimal:
        case conversion_mode::report_character_count:
            return to_integer_length(_length);

        case conversion_mode::floating_point:
            return to_floating_point_length(_length);
        }

        return 0; // Unreachable
    }

    conversion_mode mode() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::conversion_specifier);
        return _mode;
    }

    scanset_buffer<unsigned_char_type> const& scanset() const throw()
    {
        _ASSERTE(_kind == format_directive_kind::conversion_specifier && _mode == conversion_mode::scanset);
        return _scanset;
    }

private:

    bool scan_optional_literal_character_trail_bytes_tchar(char) throw()
    {
        if (isleadbyte(_literal_character_lead) == 0)
        {
            return true;
        }

        // If we need a trail byte and we're at the end of the format string,
        // the format string is malformed:
        if (*_format_it == '\0')
        {
            reset_token_state_for_error(EILSEQ);
            return false;
        }

        _literal_character_trail = *_format_it;
        ++_format_it;
        return true;
    }

    bool scan_optional_literal_character_trail_bytes_tchar(wchar_t) throw()
    {
        return true; // There are no trail bytes for wide character strings
    }

    void scan_optional_assignment_suppressor() throw()
    {
        if (*_format_it != '*')
            return;

        _suppress_assignment = true;
        ++_format_it;
        return;
    }

    bool scan_optional_field_width() throw()
    {
        if (__crt_strtox::parse_digit(static_cast<char_type>(*_format_it)) > 9)
            return true;

        unsigned_char_type* width_end{};
        uint64_t const width{traits::tcstoull(
            reinterpret_cast<char_type const*>(_format_it),
            reinterpret_cast<char_type**>(&width_end),
            10)};

        if (width == 0 || width_end == _format_it)
        {
            reset_token_state_for_error(EINVAL);
            return false;
        }

        _width     = width;
        _format_it = width_end;
        return true;
    }

    void scan_optional_length_modifier() throw()
    {
        switch (*_format_it)
        {
        case 'h':
        {
            if (_format_it[1] == 'h')
            {
                _format_it += 2; // Advance past "hh"
                _length = length_modifier::hh;
            }
            else
            {
                _format_it += 1; // Advance past "h"
                _length = length_modifier::h;
            }

            return;
        }

        case 'I':
        {
            // The I32, I64, and I length modifiers are Microsoft extensions.

            if (_format_it[1] == '3' && _format_it[2] == '2')
            {
                _format_it += 3; // Advance past "I32"
                _length = length_modifier::I32;
                return;
            }

            if (_format_it[1] == '6' && _format_it[2] == '4')
            {
                _format_it += 3; // Advance past "I64"
                _length = length_modifier::I64;
                return;
            }

            // Some disambiguation is required for the standalone I length
            // modifier.  If the I is followed by a d, i, o, x, or X character,
            // then we treat it as I32 on 32-bit platforms and I64 on 64-bit
            // platforms.  Otherwise, we do not treat it as a length modifier
            // and instead treat it as a conversion specifier (equivalent to the
            // lowercase i).
            if (_format_it[1] == 'd' ||
                _format_it[1] == 'i' ||
                _format_it[1] == 'o' ||
                _format_it[1] == 'u' ||
                _format_it[1] == 'x' ||
                _format_it[1] == 'X')
            {
                ++_format_it; // Advance past "I"
                _length = sizeof(void*) == 4
                    ? length_modifier::I32
                    : length_modifier::I64;
                return;
            }

            return;
        }

        case 'l':
        {
            if (_format_it[1] == 'l')
            {
                _format_it += 2; // Advance past "ll"
                _length = length_modifier::ll;
            }
            else
            {
                _format_it += 1; // Advance past "l"
                _length = length_modifier::l;
            }

            return;
        }

        case 'L':
        {
            ++_format_it; // Advance past "L"
            _length = length_modifier::L;
            return;
        }

        case 'j':
        {
            ++_format_it; // Advance past "j"
            _length = length_modifier::j;
            return;
        }

        case 't':
        {
            ++_format_it; // Advance past "t"
            _length = length_modifier::t;
            return;
        }

        case 'z':
        {
            ++_format_it; // Advance past "z"
            _length = length_modifier::z;
            return;
        }

        case 'T':
        {
            ++_format_it; // Advance past "T"
            _length = length_modifier::T;
            return;
        }
        }
    }

    void scan_optional_wide_modifier() throw()
    {
        if (*_format_it == 'w')
        {
            ++_format_it; // Advance past "w"
            _is_wide = true;
            return;
        }

        if (should_default_to_wide(*_format_it))
        {
            _is_wide = true;
            return;
        }
    }

    bool should_default_to_wide(unsigned char const c) throw()
    {
        return c == 'C' || c == 'S';
    }

    bool should_default_to_wide(wchar_t const c) throw()
    {
        if (c == 'C' || c == 'S')
            return false;

        if (_length == length_modifier::T)
            return true;

        return (_options & _CRT_INTERNAL_SCANF_LEGACY_WIDE_SPECIFIERS) != 0;
    }

    void set_wide_for_c_s_or_scanset() throw()
    {
        if (_length == length_modifier::h)
            _is_wide = false;

        if (_length == length_modifier::l  ||
            _length == length_modifier::ll ||
            _length == length_modifier::L)
            _is_wide = true;
    }

    bool scan_conversion_specifier() throw()
    {
        switch (*_format_it)
        {
        case 'C':
        case 'c':
        {
            // If no width was specified, use a default width of one character:
            if (_width == 0)
                _width = 1;

            set_wide_for_c_s_or_scanset();

            _mode = conversion_mode::character;
            ++_format_it;
            return true;
        }

        case 'S':
        case 's':
        {
            set_wide_for_c_s_or_scanset();

            _mode = conversion_mode::string;
            ++_format_it;
            return true;
        }

        case 'i':
        case 'I':
        {
            _mode = conversion_mode::signed_unknown;
            ++_format_it;
            return true;
        }

        case 'd':
        {
            _mode = conversion_mode::signed_decimal;
            ++_format_it;
            return true;
        }

        case 'u':
        {
            _mode = conversion_mode::unsigned_decimal;
            ++_format_it;
            return true;
        }

        case 'X':
        case 'x':
        {
            _mode = conversion_mode::unsigned_hexadecimal;
            ++_format_it;
            return true;
        }

        case 'p':
        {
            _length = sizeof(void*) == 4
                ? length_modifier::I32
                : length_modifier::I64;
            _mode = conversion_mode::unsigned_hexadecimal;
            ++_format_it;
            return true;
        }

        case 'o':
        {
            _mode = conversion_mode::unsigned_octal;
            ++_format_it;
            return true;
        }

        case 'A':
        case 'a':
        case 'E':
        case 'e':
        case 'F':
        case 'f':
        case 'G':
        case 'g':
        {
            _mode = conversion_mode::floating_point;
            ++_format_it;
            return true;
        }

        case '[':
        {
            set_wide_for_c_s_or_scanset();

            _mode = conversion_mode::scanset;
            ++_format_it;
            return scan_scanset_range();
        }

        case 'n':
        {
            _mode = conversion_mode::report_character_count;
            ++_format_it;
            return true;
        }

        default:
        {
            reset_token_state_for_error(EINVAL);
            return false;
        }
        }
    }

    bool scan_scanset_range() throw()
    {
        if (!_scanset.is_usable())
        {
            reset_token_state_for_error(ENOMEM);
            return false;
        }

        _scanset.reset();

        bool const is_reject_set{*_format_it == '^'};
        if (is_reject_set)
        {
            ++_format_it;
        }

        if (*_format_it == ']')
        {
            ++_format_it;
            _scanset.set(static_cast<unsigned_char_type>(']'));
        }

        unsigned_char_type const* const first{_format_it};
        unsigned_char_type const* last_range_end = nullptr;
        for (; *_format_it != ']' && *_format_it != '\0'; ++_format_it)
        {
            // If the current character is not a hyphen, if its the end of a range,
            // or if it's the first or last character in the scanset, treat it as a
            // literal character and just add it to the table:
            if (*_format_it != '-' || _format_it - 1 == last_range_end || _format_it == first || _format_it[1] == ']')
            {
                _scanset.set(*_format_it);
            }
            // Otherwise, we're pointing to a hyphen that falls between two other
            // characters, so this is a range to be matched (e.g. [a-z]).  Toggle
            // the bits for each character in the range:
            else
            {
                unsigned_char_type lower_bound{_format_it[-1]};
                unsigned_char_type upper_bound{_format_it[ 1]};
                last_range_end = _format_it + 1;

                // We support ranges in both directions ([a-z] and [z-a]).  We
                // can handle both simultaneously by transforming [z-a] into [a-z]:
                if (lower_bound > upper_bound)
                {
                    unsigned_char_type const c{lower_bound};
                    lower_bound = upper_bound;
                    upper_bound = c;
                }

                // Convert [lower_bound, upper_bound] into [lower_bound, upper_bound):
                ++upper_bound;

                for (unsigned_char_type c{lower_bound}; c != upper_bound; ++c)
                {
                    _scanset.set(c);
                }
            }
        }

        if (*_format_it == '\0')
        {
            reset_token_state_for_error(EINVAL);
            return false;
        }

        if (is_reject_set)
        {
            _scanset.invert();
        }

        ++_format_it; // Advance past ']'
        return true;
    }

    void reset_token_state() throw()
    {
        // Note that we never reset the error code; in theory, we should never
        // be trying to use the parser at all once an error is encountered.
        _kind                    = format_directive_kind::unknown_error;
        _literal_character_lead  = '\0';
        _literal_character_trail = '\0';
        _suppress_assignment     = false;
        _width                   = 0;
        _length                  = length_modifier::none;
        _is_wide                 = false;
        _mode                    = conversion_mode();
    }

    void reset_token_state_for_error(errno_t const error_code) throw()
    {
        reset_token_state();
        _error_code = error_code;
    }

    uint64_t                           _options;
    unsigned_char_type const*          _format_it;
    errno_t                            _error_code;

    format_directive_kind              _kind;

    unsigned_char_type                 _literal_character_lead;
    unsigned_char_type                 _literal_character_trail;

    bool                               _suppress_assignment;
    uint64_t                           _width;
    length_modifier                    _length;
    bool                               _is_wide;
    conversion_mode                    _mode;
    scanset_buffer<unsigned_char_type> _scanset;
};

} // namespace __crt_stdio_input



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Input Processor
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The input processor constructs a format string parser, iterates over the
// format string, and performs the appropriate action for each directive.
namespace __crt_stdio_input {

template <typename Character, typename InputAdapter>
class input_processor
{
public:

    using traits             = __acrt_stdio_char_traits<Character>;
    using char_type          = Character;
    using unsigned_char_type = typename traits::unsigned_char_type;
    using int_type           = typename traits::int_type;

    input_processor(
        InputAdapter     const& input_adapter,
        uint64_t         const  options,
        char_type const* const  format,
        _locale_t        const  locale,
        va_list          const  arglist
        ) throw()
        : _options                        {options                                                     },
          _input_adapter                  {input_adapter                                               },
          _format_parser                  {options, reinterpret_cast<unsigned_char_type const*>(format)},
          _locale                         {locale                                                      },
          _valist                         {arglist                                                     },
          _receiving_arguments_assigned   {0                                                           }
    {
    }

    int process() throw()
    {
        if (!_input_adapter.validate())
            return EOF;

        if (!_format_parser.validate())
            return EOF;

        while (_format_parser.advance())
        {
            if (!process_state())
                break;
        }

        // Return value is number of receiving arguments assigned,
        // or EOF if read failure occurs before the first receiving argument was assigned.
        int result{static_cast<int>(_receiving_arguments_assigned)};

        // If we haven't reached the 'end_of_string' parse, then a read error occurred.
        if (_receiving_arguments_assigned == 0 && _format_parser.kind() != format_directive_kind::end_of_string)
        {
            int_type const c{_input_adapter.get()};
            if (c == traits::eof)
                result = EOF;

            _input_adapter.unget(c);
        }

        if (secure_buffers())
        {
            _VALIDATE_RETURN(_format_parser.error_code() == 0, _format_parser.error_code(), result);
        }

        return result;
    }

private:

    bool process_state() throw()
    {
        switch (_format_parser.kind())
        {
        case format_directive_kind::whitespace:
        {
            return process_whitespace();
        }

        case format_directive_kind::literal_character:
        {
            return process_literal_character();
        }

        case format_directive_kind::conversion_specifier:
        {
            bool const result{process_conversion_specifier()};
            // %n and suppressed conversion specifiers are not considered receiving arguments
            if (result
                && _format_parser.mode() != conversion_mode::report_character_count
                && !_format_parser.suppress_assignment())
            {
                ++_receiving_arguments_assigned;
            }

            return result;
        }
        }

        return false;
    }

    bool process_whitespace() throw()
    {
        _input_adapter.unget(skip_whitespace(_input_adapter, _locale));
        return true;
    }

    bool process_literal_character() throw()
    {
        int_type const c{_input_adapter.get()};
        if (c == traits::eof)
        {
            return false;
        }

        if (c != _format_parser.literal_character_lead())
        {
            _input_adapter.unget(c);
            return false;
        }

        return process_literal_character_tchar(static_cast<char_type>(c));
    }

    bool process_literal_character_tchar(char const initial_character) throw()
    {
        if (isleadbyte(static_cast<unsigned char>(initial_character)) == 0)
        {
            return true;
        }

        int_type const c{_input_adapter.get()};
        if (c != _format_parser.literal_character_trail())
        {
            _input_adapter.unget(c);
            _input_adapter.unget(initial_character);
            return false;
        }

        return true;
    }

    bool process_literal_character_tchar(wchar_t) throw()
    {
        return true; // No-op for Unicode
    }

    bool process_conversion_specifier() throw()
    {
        switch (_format_parser.mode())
        {
        case conversion_mode::character:              return process_string_specifier(conversion_mode::character);
        case conversion_mode::string:                 return process_string_specifier(conversion_mode::string);
        case conversion_mode::scanset:                return process_string_specifier(conversion_mode::scanset);

        case conversion_mode::signed_unknown:         return process_integer_specifier( 0,  true);
        case conversion_mode::signed_decimal:         return process_integer_specifier(10,  true);
        case conversion_mode::unsigned_octal:         return process_integer_specifier( 8, false);
        case conversion_mode::unsigned_decimal:       return process_integer_specifier(10, false);
        case conversion_mode::unsigned_hexadecimal:   return process_integer_specifier(16, false);

        case conversion_mode::floating_point:         return process_floating_point_specifier();

        case conversion_mode::report_character_count: return process_character_count_specifier();
        }

        return false;
    }

    bool process_string_specifier(conversion_mode const mode) throw()
    {
        if (mode == conversion_mode::string)
        {
            process_whitespace();
        }

        switch (_format_parser.length())
        {
        case sizeof(char):    return process_string_specifier_tchar(mode, char());
        case sizeof(wchar_t): return process_string_specifier_tchar(mode, wchar_t());
        default:              return false;
        }
    }

    template <typename BufferCharacter>
    bool process_string_specifier_tchar(conversion_mode const mode, BufferCharacter) throw()
    {
        BufferCharacter _UNALIGNED* buffer{nullptr};
        if (!_format_parser.suppress_assignment())
        {
            buffer = va_arg(_valist, BufferCharacter _UNALIGNED*);
            _VALIDATE_RETURN(buffer != nullptr, EINVAL, false);
        }

        size_t const buffer_count{buffer != nullptr && secure_buffers()
            ? va_arg(_valist, unsigned)
            : _CRT_UNBOUNDED_BUFFER_SIZE
        };

        if (buffer_count == 0)
        {
            if (_options & _CRT_INTERNAL_SCANF_LEGACY_MSVCRT_COMPATIBILITY)
            {
                // For legacy compatibility:  in the old implementation, we failed
                // to unget the last character read if the buffer had a size of zero
                _input_adapter.get();

                // Additionally, we wrote a terminator to the buffer, even though
                // the caller said the buffer had zero elements
                buffer[0] = '\0';
            }

            errno = ENOMEM;
            return false;
        }

        uint64_t const width{_format_parser.width()};

        BufferCharacter _UNALIGNED* buffer_pointer  {buffer      };
        size_t                      buffer_remaining{buffer_count};

        if (mode != conversion_mode::character && buffer_remaining != _CRT_UNBOUNDED_BUFFER_SIZE)
        {
            --buffer_remaining; // Leave room for terminator when scanning strings
        }

        uint64_t width_consumed{0};
        for (; width == 0 || width_consumed != width; ++width_consumed)
        {
            int_type c{_input_adapter.get()};

            if (!is_character_allowed_in_string(mode, c))
            {
                _input_adapter.unget(c);
                break;
            }

            if (_format_parser.suppress_assignment())
            {
                continue;
            }

            if (buffer_remaining == 0)
            {
                reset_buffer(buffer, buffer_count);
                errno = ENOMEM;
                return false;
            }

            if (!write_character(buffer, buffer_count, buffer_pointer, buffer_remaining, static_cast<char_type>(c)))
            {
                break;
            }
        }

        if (width_consumed == 0)
        {
            return false;
        }

        // The %c conversion specifier "matches a sequence of characters of exactly
        // the number specified by the field width."  The legacy behavior was to
        // match a sequence of up to that many characters.  If legacy mode was not
        // requested, fail if we did not match the requested number of characters:
        if (mode == conversion_mode::character &&
            width_consumed != width &&
            (_options & _CRT_INTERNAL_SCANF_LEGACY_MSVCRT_COMPATIBILITY) == 0)
        {
            return false;
        }

        if (_format_parser.suppress_assignment())
        {
            return true;
        }

        if (mode != conversion_mode::character)
        {
            *buffer_pointer = '\0';
            fill_buffer(buffer, buffer_count, buffer_remaining);
        }

        return true;
    }

    // There are four overloads of write_character to handle writing both narrow
    // and wide characters into either a narrow or a wide buffer.
    bool write_character(
        char*  const buffer,
        size_t const buffer_count,
        char*&       buffer_pointer,
        size_t&      buffer_remaining,
        char   const c
        ) throw()
    {
        UNREFERENCED_PARAMETER(buffer);
        UNREFERENCED_PARAMETER(buffer_count);

        *buffer_pointer++ = c;
        --buffer_remaining;
        return true;
    }

    bool write_character(
        char*   const buffer,
        size_t  const buffer_count,
        char*&        buffer_pointer,
        size_t&       buffer_remaining,
        wchar_t const c
        ) throw()
    {
        if (buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE)
        {
            int narrow_count{0};
            if (_ERRCHECK_EINVAL_ERANGE(wctomb_s(&narrow_count, buffer_pointer, MB_LEN_MAX, c)) == 0)
            {
                buffer_pointer   += narrow_count;
                buffer_remaining -= narrow_count;
            }
        }
        else
        {
            int narrow_count{0};
            if (wctomb_s(&narrow_count, buffer_pointer, buffer_remaining, c) == ERANGE)
            {
                reset_buffer(buffer, buffer_count);
                return false;
            }

            if (narrow_count > 0)
            {
                buffer_pointer   += narrow_count;
                buffer_remaining -= narrow_count;
            }
        }

        return true;
    }

    bool write_character(
        wchar_t _UNALIGNED* const buffer,
        size_t              const buffer_count,
        wchar_t _UNALIGNED*&      buffer_pointer,
        size_t&                   buffer_remaining,
        char                const c
        ) throw()
    {
        UNREFERENCED_PARAMETER(buffer);
        UNREFERENCED_PARAMETER(buffer_count);

        char narrow_temp[2]{c, '\0'};
        if (isleadbyte(static_cast<unsigned char>(c)))
        {
            narrow_temp[1] = static_cast<char>(_input_adapter.get());
        }

        wchar_t wide_temp{'?'};

        _mbtowc_l(&wide_temp, narrow_temp, _locale->locinfo->_public._locale_mb_cur_max, _locale);

        *buffer_pointer++ = c;
        --buffer_remaining;
        return true;
    }

    bool write_character(
        wchar_t _UNALIGNED* const buffer,
        size_t              const buffer_count,
        wchar_t _UNALIGNED*&      buffer_pointer,
        size_t&                   buffer_remaining,
        wchar_t             const c
        ) throw()
    {
        UNREFERENCED_PARAMETER(buffer);
        UNREFERENCED_PARAMETER(buffer_count);

        *buffer_pointer++ = c;
        --buffer_remaining;
        return true;
    }

    template <typename BufferCharacter>
    static void fill_buffer(
        BufferCharacter _UNALIGNED* const buffer,
        size_t                      const buffer_count,
        size_t                      const buffer_remaining
        ) throw()
    {
        UNREFERENCED_PARAMETER(buffer);
        UNREFERENCED_PARAMETER(buffer_remaining);

        if (buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE)
            return;

        _FILL_STRING(buffer, buffer_count, buffer_count - buffer_remaining);
    }

    template <typename BufferCharacter>
    static void reset_buffer(
        BufferCharacter _UNALIGNED* const buffer,
        size_t                      const buffer_count
        ) throw()
    {
        UNREFERENCED_PARAMETER(buffer);

        if (buffer_count == _CRT_UNBOUNDED_BUFFER_SIZE)
            return;

        _RESET_STRING(buffer, buffer_count);
    }

    bool is_character_allowed_in_string(conversion_mode const mode, int_type const c) const throw()
    {
        if (c == traits::eof)
            return false;

        switch (mode)
        {
        case conversion_mode::character:
        {
            return true;
        }

        case conversion_mode::string:
        {
            if (c >= '\t' && c <= '\r')
                return false;

            if (c == ' ')
                return false;

            return true;
        }

        case conversion_mode::scanset:
        {
            if (!_format_parser.scanset().test(static_cast<unsigned_char_type>(c)))
                return false;

            return true;
        }
        }

        return false;
    }

    bool process_integer_specifier(unsigned base, bool const is_signed) throw()
    {
        process_whitespace();

        bool succeeded{false};
        uint64_t const number{__crt_strtox::parse_integer<uint64_t>(
            _locale,
            __crt_strtox::make_input_adapter_character_source(&_input_adapter, _format_parser.width(), &succeeded),
            base,
            is_signed)};

        if (!succeeded)
            return false;

        if (_format_parser.suppress_assignment())
            return true;

        return write_integer(number);
    }

    template <typename FloatingType>
    bool process_floating_point_specifier_t() throw()
    {
        bool succeeded{false};
        FloatingType value{};
        SLD_STATUS const status{__crt_strtox::parse_floating_point(
            _locale,
            __crt_strtox::make_input_adapter_character_source(&_input_adapter, _format_parser.width(), &succeeded),
            &value)};

        if (!succeeded || status == SLD_NODIGITS)
            return false;

        if (_format_parser.suppress_assignment())
            return true;

        return write_floating_point(value);
    }

    bool process_floating_point_specifier() throw()
    {
        process_whitespace();

        switch (_format_parser.length())
        {
        case sizeof(float):  return process_floating_point_specifier_t<float >();
        case sizeof(double): return process_floating_point_specifier_t<double>();
        default:             return false;
        }
    }

    bool process_character_count_specifier() throw()
    {
        if (_format_parser.suppress_assignment())
            return true;

        return write_integer(_input_adapter.characters_read());
    }

    bool write_integer(uint64_t const value) throw()
    {
        void* const result_pointer{va_arg(_valist, void*)};
        _VALIDATE_RETURN(result_pointer != nullptr, EINVAL, false);

        switch (_format_parser.length())
        {
        case sizeof(uint8_t ): *static_cast<uint8_t *>(result_pointer) = static_cast<uint8_t >(value); return true;
        case sizeof(uint16_t): *static_cast<uint16_t*>(result_pointer) = static_cast<uint16_t>(value); return true;
        case sizeof(uint32_t): *static_cast<uint32_t*>(result_pointer) = static_cast<uint32_t>(value); return true;
        case sizeof(uint64_t): *static_cast<uint64_t*>(result_pointer) = static_cast<uint64_t>(value); return true;
        default:
            _ASSERTE(("Unexpected length specifier", false));
            return false;
        }
    }

    template <typename FloatingType>
    bool write_floating_point(FloatingType const& value) throw()
    {
        using integer_type = uintn_t<sizeof(FloatingType)>;

        void* const result_pointer{va_arg(_valist, void*)};
        _VALIDATE_RETURN(result_pointer != nullptr, EINVAL, false);

        _ASSERTE(sizeof(FloatingType) == _format_parser.length());

        // We write through the pointer as if it were an integer pointer, to
        // avoid floating point instructions that cause information loss for
        // special NaNs (signaling NaNs, indeterminates):
        *static_cast<integer_type*>(result_pointer) = reinterpret_cast<integer_type const&>(value);
        return true;
    }

private:

    bool secure_buffers() const throw()
    {
        return (_options & _CRT_INTERNAL_SCANF_SECURECRT) != 0;
    }

    uint64_t                        _options;
    InputAdapter                    _input_adapter;
    format_string_parser<char_type> _format_parser;
    _locale_t                       _locale;
    va_list                         _valist;
    size_t                          _receiving_arguments_assigned;
};

} // namespace __crt_stdio_input
