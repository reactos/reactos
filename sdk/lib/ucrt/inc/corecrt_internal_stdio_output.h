//
// corecrt_internal_stdio_output.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file defines the core implementation of the formatted output functions,
// including printf and its many variants (sprintf, fprintf, etc.).
//
#include <conio.h>
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_strtox.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>

#include <corecrt_internal_ptd_propagation.h>

namespace __crt_stdio_output {



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Argument Handling
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These provide a very basic encapsulation over a va_list.  The 'read' function
// reads the next argument of type T from the varargs list and updates the
// va_list, just like va_arg does.  The 'peek' function returns the next argument
// of type T, but does not modify the va_list.
#if defined(__GNUC__) || defined(__clang__)
template<typename T> struct _va_arg_promoted_tye { using type = T; };
template<> struct _va_arg_promoted_tye<signed char> { using type = int; };
template<> struct _va_arg_promoted_tye<unsigned char> { using type = int; };
template<> struct _va_arg_promoted_tye<wchar_t> { using type = int; };
template<> struct _va_arg_promoted_tye<short int> { using type = int; };
template<> struct _va_arg_promoted_tye<short unsigned int> { using type = int; };
#endif

template <typename T>
T read_va_arg(va_list& arglist) throw()
{
#if defined(__GNUC__) || defined(__clang__)
    return (T)(va_arg(arglist, typename _va_arg_promoted_tye<T>::type));
#else
    return va_arg(arglist, T);
#endif
}

template <typename T>
T peek_va_arg(va_list arglist) throw()
{
#if defined(__GNUC__) || defined(__clang__)
    return (T)(va_arg(arglist, typename _va_arg_promoted_tye<T>::type));
#else
    return va_arg(arglist, T);
#endif
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Output Adapters
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The actual write operations are different depending on whether the target is
// a stream or the console.  We handle these differences via output adapters.
// The stream and console I/O functions pass an adapter to the core output
// function, and that function calls the various write members of the adapter to
// perform the write operations.
template <typename Character, typename Derived>
class output_adapter_common
{
public:
    void write_character(Character const c, int* const count_written, __crt_cached_ptd_host& ptd) const throw()
    {
        if (static_cast<Derived const*>(this)->write_character_without_count_update(c, ptd))
        {
            ++*count_written;
        }
        else
        {
            *count_written = -1;
        }
    }

protected:
    void write_string_impl(
        Character const*   const string,
        int                const length,
        int*               const count_written,
        __crt_cached_ptd_host&   ptd
        ) const throw()
    {
        auto const reset_errno = ptd.get_errno().create_guard();

        Character const* const string_last{string + length};
        for (Character const* it{string}; it != string_last; ++it)
        {
            if (static_cast<Derived const*>(this)->write_character_without_count_update(*it, ptd))
            {
                ++*count_written;
            }
            else
            {
                // Non-standard extension: EILSEQ errors are recoverable.
                // Standard behavior when we've encountered an 'illegal sequence' error
                // (i.e. EILSEQ) is to set 'errno' to EILSEQ and return -1.
                // Instead, we write '?' and continue writing the string.
                if (!ptd.get_errno().check(EILSEQ))
                {
                    // *printf returns the number of characters written
                    // set count written to -1 to indicate an error occurred
                    *count_written = -1;
                    break;
                }

                write_character('?', count_written, ptd);
            }
        }
    }
};

template <typename Character>
class console_output_adapter
    : public output_adapter_common<Character, console_output_adapter<Character>>
{
#ifndef _MSC_VER // For retarded compilers!
    using oac_base = output_adapter_common<Character, console_output_adapter<Character>>;
    using oac_base::write_string_impl;
#endif
public:
    typedef __acrt_stdio_char_traits<Character> char_traits;

    bool validate(__crt_cached_ptd_host&) const throw()
    {
        return true;
    }

    bool write_character_without_count_update(Character const c, __crt_cached_ptd_host& ptd) const throw()
    {
        return char_traits::puttch_nolock_internal(c, ptd) != char_traits::eof;
    }

    void write_string(
        Character const*   const string,
        int                const length,
        int*               const count_written,
        __crt_cached_ptd_host&   ptd
        ) const throw()
    {
        write_string_impl(string, length, count_written, ptd);
    }
};



template <typename Character>
class stream_output_adapter
    : public output_adapter_common<Character, stream_output_adapter<Character>>
{
#ifndef _MSC_VER // For retarded compilers!
    using oac_base = output_adapter_common<Character, stream_output_adapter<Character>>;
    using oac_base::write_string_impl;
#endif
public:
    typedef __acrt_stdio_char_traits<Character> char_traits;

    stream_output_adapter(FILE* const public_stream) throw()
        : _stream{public_stream}
    {
    }

    bool validate(__crt_cached_ptd_host& ptd) const throw()
    {
        _UCRT_VALIDATE_RETURN(ptd, _stream.valid(), EINVAL, false);

        return char_traits::validate_stream_is_ansi_if_required(_stream.public_stream());
    }

    bool write_character_without_count_update(Character const c, __crt_cached_ptd_host& ptd) const throw()
    {
        if (_stream.is_string_backed() && _stream->_base == nullptr)
        {
            return true;
        }

        return char_traits::puttc_nolock_internal(c, _stream.public_stream(), ptd) != char_traits::eof;
    }

    void write_string(
        Character const*   const string,
        int                const length,
        int*               const count_written,
        __crt_cached_ptd_host&   ptd
        ) const throw()
    {
        if (_stream.is_string_backed() && _stream->_base == nullptr)
        {
            *count_written += length;
            return;
        }

        write_string_impl(string, length, count_written, ptd);
    }

private:

    __crt_stdio_stream _stream;
};



template <typename Character>
struct string_output_adapter_context
{
    Character* _buffer;
    size_t     _buffer_count;
    size_t     _buffer_used;
    bool       _continue_count;
};

template <typename Character>
class string_output_adapter
{
public:

    typedef __acrt_stdio_char_traits<Character>      char_traits;
    typedef string_output_adapter_context<Character> context_type;

    string_output_adapter(context_type* const context) throw()
        : _context(context)
    {
    }

    bool validate(__crt_cached_ptd_host& ptd) const throw()
    {
        _UCRT_VALIDATE_RETURN(ptd, _context != nullptr, EINVAL, false);
        return true;
    }

    __forceinline bool write_character(Character const c, int* const count_written, __crt_cached_ptd_host&) const throw()
    {
        if (_context->_buffer_used == _context->_buffer_count)
        {
            if (_context->_continue_count)
            {
                ++*count_written;
            }
            else
            {
                *count_written = -1;
            }

            return _context->_continue_count;
        }

        ++*count_written;
        ++_context->_buffer_used;
        *_context->_buffer++ = c;
        return true;
    }

    void write_string(
        Character const*   const string,
        int                const length,
        int*               const count_written,
        __crt_cached_ptd_host&   ptd
        ) const throw()
    {
        // This function does not perform any operations that might reset errno,
        // so we don't need to use __crt_errno_guard as we do in other output
        // adapters.
        UNREFERENCED_PARAMETER(ptd);

        if (length == 0)
        {
            return;
        }

        if (_context->_buffer_used == _context->_buffer_count)
        {
            if (_context->_continue_count)
            {
                *count_written += length;
            }
            else
            {
                *count_written = -1;
            }

            return;
        }

        size_t const space_available  = _context->_buffer_count - _context->_buffer_used;
        size_t const elements_to_copy = __min(space_available, static_cast<size_t>(length));

        // Performance note:  This is hot code.  Profiling has shown the extra
        // validation done by memcpy_s to be quite expensive.  The validation is
        // unnecessary, so we call memcpy directly here:
        memcpy(
            _context->_buffer,
            string,
            elements_to_copy * sizeof(Character));

        _context->_buffer      += elements_to_copy;
        _context->_buffer_used += elements_to_copy;

        if (_context->_continue_count)
        {
            *count_written += length;
        }
        else if (elements_to_copy != static_cast<size_t>(length))
        {
            *count_written = -1;
        }
        else
        {
            *count_written += static_cast<int>(elements_to_copy);
        }
    }

private:

    context_type* _context;
};



template <typename OutputAdapter, typename Character>
__forceinline void write_multiple_characters(
    OutputAdapter      const& adapter,
    Character          const  c,
    int                const  count,
    int*               const  count_written,
    __crt_cached_ptd_host&    ptd
    ) throw()
{
    for (int i{0}; i < count; ++i)
    {
        adapter.write_character(c, count_written, ptd);
        if (*count_written == -1)
            break;
    }
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Formatting Buffer
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This type encapsulates the buffer to be used when formatting numbers.  It
// contains as a data member a large, statically-sized buffer, which is used
// at first.  If a larger buffer is required a new, larger buffer is dynamically
// allocated.
class formatting_buffer
{
public:

    enum
    {
        member_buffer_size = 1024,
    };

    static_assert(member_buffer_size >= (_CVTBUFSIZE + 6) * 2, "Buffer is too small");

    formatting_buffer() throw()
        : _dynamic_buffer_size{0}
    {
    }

    template <typename T>
    bool ensure_buffer_is_big_enough(size_t const count, __crt_cached_ptd_host& ptd) throw()
    {
        constexpr size_t max_count = SIZE_MAX / sizeof(T) / 2; // avoid runtime division
        _UCRT_VALIDATE_RETURN_NOEXC(ptd, max_count >= count, ENOMEM, false);

        size_t const required_size{count * sizeof(T) * 2};

        // Once we allocate a dynamic buffer, we no longer use the member buffer
        if (!_dynamic_buffer && required_size <= member_buffer_size) {
            return true;
        }

        if (required_size <= _dynamic_buffer_size) {
            return true;
        }

        __crt_unique_heap_ptr<char> new_buffer{_malloc_crt_t(char, required_size)};
        if (!new_buffer) {
            return false;
        }

        _dynamic_buffer      = static_cast<__crt_unique_heap_ptr<char>&&>(new_buffer);
        _dynamic_buffer_size = required_size;
        return true;
    }

    template <typename T>
    T* data() throw()
    {
        if (!_dynamic_buffer)
            return reinterpret_cast<T*>(_member_buffer);

        return reinterpret_cast<T*>(_dynamic_buffer.get());
    }

    template <typename T>
    T* scratch_data() throw()
    {
        if (!_dynamic_buffer)
            return reinterpret_cast<T*>(_member_buffer) + count<T>();

        return reinterpret_cast<T*>(_dynamic_buffer.get()) + count<T>();
    }

    template <typename T>
    size_t count() const throw()
    {
        if (!_dynamic_buffer)
            return member_buffer_size / sizeof(T) / 2;

        return _dynamic_buffer_size / sizeof(T) / 2;
    }

    template <typename T>
    size_t scratch_count() const throw()
    {
        return count<T>();
    }

private:

    char _member_buffer[member_buffer_size];

    size_t                      _dynamic_buffer_size;
    __crt_unique_heap_ptr<char> _dynamic_buffer;
};



// This function forces a decimal point in floating point output.  It is called
// if '#' flag is given and precision is 0, so we know the number has no '.' in
// its current representation.  We insert the '.' and move everything back one
// position until '\0' is seen.  This function updates the buffer in place.
inline void __cdecl force_decimal_point(_Inout_z_ char* buffer, _locale_t const locale) throw()
{
    if (_tolower_fast_internal(static_cast<unsigned char>(*buffer), locale) != 'e')
    {
        do
        {
            ++buffer;
        }
        while (_isdigit_fast_internal(static_cast<unsigned char>(*buffer), locale));
    }

    // Check if the buffer is in hexadecimal format (cfr %a or %A and fp_format_a):
    if (_tolower_fast_internal(*buffer, locale) == 'x')
    {
        // The buffer is in the form: [-]0xhP+d, and buffer points to the 'x':
        // we want to put the decimal point after the h digit: [-]0xh.P+d
        buffer += 2; // REVIEW This can skip terminal nul?
    }

    char holdchar = *buffer;

    *buffer++ = *locale->locinfo->lconv->decimal_point;

    do
    {
        char const nextchar = *buffer;
        *buffer  = holdchar;
        holdchar = nextchar;
    }
    while (*buffer++);
}



// This function removes trailing zeroes (after the '.') from a floating point
// number.  This function is called only when formatting in the %g mode when
// there is no '#' flag and precision is nonzero.  This function updates the
// buffer in-place.
//
// This function changes the contents of the buffer from:
//     [-] digit [digit...] [ . [digits...] [0...] ] [(exponent part)]
// to:
//     [-] digit [digit...] [ . digit [digits...] ] [(exponent part)]
// or:
//     [-] digit [digit...] [(exponent part)]
inline void __cdecl crop_zeroes(_Inout_z_ char* buffer, _locale_t const locale) throw()
{
    char const decimal_point = *locale->locinfo->lconv->decimal_point;

    while (*buffer && *buffer != decimal_point)
        ++buffer;

    if (*buffer++)
    {
        while (*buffer && *buffer != 'e' && *buffer != 'E')
            ++buffer;

        char* stop = buffer--;

        while (*buffer == '0')
            --buffer;

        if (*buffer == decimal_point)
            --buffer;

        while((*++buffer = *stop++) != '\0') { }
    }
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// States and the State Transition Tables
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These tables hold the state transition data for the format processor.  The
// lower nibble of each byte is the character class of the character.  The upper
// nibble of each byte gives the next state to enter.

// The states of the format parsing state machine.  These are encoded into the
// transition tables and are referenced throughout the format processor.
enum class state : unsigned char
{
    normal,    // Normal state; outputting literal chars
    percent,   // Just read '%'
    flag,      // Just read flag character
    width,     // Just read width specifier
    dot,       // Just read '.'
    precision, // Just read precision specifier
    size,      // Just read size specifier
    type,      // Just read type specifier
    invalid    // Invalid format
};

// Manifest constants to represent the different types of characters that may
// be encountered in the format string.  These are not referenced anywhere in
// the source anymore, but their values are encoded into the state table, so
// we have retained the definition for reference.
enum class character_type : unsigned char
{
    other,   // character with no special meaning
    percent, // '%'
    dot,     // '.'
    star,    // '*'
    zero,    // '0'
    digit,   // '1'..'9'
    flag,    // ' ', '+', '-', '#'
    size,    // 'h', 'l', 'L', 'N', 'F', 'w'
    type     // type specifying character
};

struct state_transition_pair
{
    state          next_state;
    character_type current_class;
};

template <typename T, size_t Size>
class spectre_mitigated_lookup_table
{
public:

    static size_t const mask = Size - 1;
    static_assert((Size & mask) == 0, "Size must be a power of two.");

    // Instead of using an lfence mitigation, we can fill the table to a power of two,
    // then bitwise-and all values used to index into the array.
    #if defined(_MSC_VER) && !defined(__clang__)
    __declspec(spectre(nomitigation))
    #endif
    T const& operator[](size_t const index) const
    {
        return m_array[index & mask];
    }

    T m_array[Size];
};

using printf_state_transition_table = spectre_mitigated_lookup_table<state_transition_pair, 128>;

// Note, the state transition and character type are unrelated data - they just occupy the same table.
// This table skips the 'invalid' state.
extern __declspec(selectany) printf_state_transition_table const standard_lookup_table_spectre
{
    /* prev state -> cur char ->    new state                   character type */
    /* normal     -> other    -> */ state::normal,    /* ' ' */ character_type::flag,
    /* percent    -> other    -> */ state::normal,    /* '!' */ character_type::other,
    /* flag       -> other    -> */ state::normal,    /* '"' */ character_type::other,
    /* width      -> other    -> */ state::normal,    /* '#' */ character_type::flag,
    /* dot        -> other    -> */ state::normal,    /* '$' */ character_type::other,
    /* precision  -> other    -> */ state::normal,    /* '%' */ character_type::percent,
    /* size       -> other    -> */ state::normal,    /* '&' */ character_type::other,
    /* type       -> other    -> */ state::normal,    /* ''' */ character_type::other,
    /* normal     -> percent  -> */ state::percent,   /* '(' */ character_type::other,
    /* percent    -> percent  -> */ state::normal,    /* ')' */ character_type::other,
    /* flag       -> percent  -> */ state::normal,    /* '*' */ character_type::star,
    /* width      -> percent  -> */ state::normal,    /* '+' */ character_type::flag,
    /* dot        -> percent  -> */ state::normal,    /* ',' */ character_type::other,
    /* precision  -> percent  -> */ state::normal,    /* '-' */ character_type::flag,
    /* size       -> percent  -> */ state::normal,    /* '.' */ character_type::dot,
    /* type       -> percent  -> */ state::percent,   /* '/' */ character_type::other,
    /* normal     -> dot      -> */ state::normal,    /* '0' */ character_type::zero,
    /* percent    -> dot      -> */ state::dot,       /* '1' */ character_type::digit,
    /* flag       -> dot      -> */ state::dot,       /* '2' */ character_type::digit,
    /* width      -> dot      -> */ state::dot,       /* '3' */ character_type::digit,
    /* dot        -> dot      -> */ state::normal,    /* '4' */ character_type::digit,
    /* precision  -> dot      -> */ state::normal,    /* '5' */ character_type::digit,
    /* size       -> dot      -> */ state::normal,    /* '6' */ character_type::digit,
    /* type       -> dot      -> */ state::normal,    /* '7' */ character_type::digit,
    /* normal     -> star     -> */ state::normal,    /* '8' */ character_type::digit,
    /* percent    -> star     -> */ state::width,     /* '9' */ character_type::digit,
    /* flag       -> star     -> */ state::width,     /* ':' */ character_type::other,
    /* width      -> star     -> */ state::normal,    /* ';' */ character_type::other,
    /* dot        -> star     -> */ state::precision, /* '<' */ character_type::other,
    /* precision  -> star     -> */ state::normal,    /* '=' */ character_type::other,
    /* size       -> star     -> */ state::normal,    /* '>' */ character_type::other,
    /* type       -> star     -> */ state::normal,    /* '?' */ character_type::other,
    /* normal     -> zero     -> */ state::normal,    /* '@' */ character_type::other,
    /* percent    -> zero     -> */ state::flag,      /* 'A' */ character_type::type,
    /* flag       -> zero     -> */ state::flag,      /* 'B' */ character_type::other,
    /* width      -> zero     -> */ state::width,     /* 'C' */ character_type::type,
    /* dot        -> zero     -> */ state::precision, /* 'D' */ character_type::other,
    /* precision  -> zero     -> */ state::precision, /* 'E' */ character_type::type,
    /* size       -> zero     -> */ state::normal,    /* 'F' */ character_type::size,
    /* type       -> zero     -> */ state::normal,    /* 'G' */ character_type::type,
    /* normal     -> digit    -> */ state::normal,    /* 'H' */ character_type::other,
    /* percent    -> digit    -> */ state::width,     /* 'I' */ character_type::size,
    /* flag       -> digit    -> */ state::width,     /* 'J' */ character_type::other,
    /* width      -> digit    -> */ state::width,     /* 'K' */ character_type::other,
    /* dot        -> digit    -> */ state::precision, /* 'L' */ character_type::size,
    /* precision  -> digit    -> */ state::precision, /* 'M' */ character_type::other,
    /* size       -> digit    -> */ state::normal,    /* 'N' */ character_type::size,
    /* type       -> digit    -> */ state::normal,    /* 'O' */ character_type::other,
    /* normal     -> flag     -> */ state::normal,    /* 'P' */ character_type::other,
    /* percent    -> flag     -> */ state::flag,      /* 'Q' */ character_type::other,
    /* flag       -> flag     -> */ state::flag,      /* 'R' */ character_type::other,
    /* width      -> flag     -> */ state::normal,    /* 'S' */ character_type::type,
    /* dot        -> flag     -> */ state::normal,    /* 'T' */ character_type::size,
    /* precision  -> flag     -> */ state::normal,    /* 'U' */ character_type::other,
    /* size       -> flag     -> */ state::normal,    /* 'V' */ character_type::other,
    /* type       -> flag     -> */ state::normal,    /* 'W' */ character_type::other,
    /* normal     -> size     -> */ state::normal,    /* 'X' */ character_type::type,
    /* percent    -> size     -> */ state::size,      /* 'Y' */ character_type::other,
    /* flag       -> size     -> */ state::size,      /* 'Z' */ character_type::type,
    /* width      -> size     -> */ state::size,      /* '[' */ character_type::other,
    /* dot        -> size     -> */ state::size,      /* '\' */ character_type::other,
    /* precision  -> size     -> */ state::size,      /* ']' */ character_type::other,
    /* size       -> size     -> */ state::size,      /* '^' */ character_type::other,
    /* type       -> size     -> */ state::normal,    /* '_' */ character_type::other,
    /* normal     -> type     -> */ state::normal,    /* '`' */ character_type::other,
    /* percent    -> type     -> */ state::type,      /* 'a' */ character_type::type,
    /* flag       -> type     -> */ state::type,      /* 'b' */ character_type::other,
    /* width      -> type     -> */ state::type,      /* 'c' */ character_type::type,
    /* dot        -> type     -> */ state::type,      /* 'd' */ character_type::type,
    /* precision  -> type     -> */ state::type,      /* 'e' */ character_type::type,
    /* size       -> type     -> */ state::type,      /* 'f' */ character_type::type,
    /* type       -> type     -> */ state::normal,    /* 'g' */ character_type::type,
    /* unused                    */ state::normal,    /* 'h' */ character_type::size,
    /* unused                    */ state::normal,    /* 'i' */ character_type::type,
    /* unused                    */ state::normal,    /* 'j' */ character_type::size,
    /* unused                    */ state::normal,    /* 'k' */ character_type::other,
    /* unused                    */ state::normal,    /* 'l' */ character_type::size,
    /* unused                    */ state::normal,    /* 'm' */ character_type::other,
    /* unused                    */ state::normal,    /* 'n' */ character_type::type,
    /* unused                    */ state::normal,    /* 'o' */ character_type::type,
    /* unused                    */ state::normal,    /* 'p' */ character_type::type,
    /* unused                    */ state::normal,    /* 'q' */ character_type::other,
    /* unused                    */ state::normal,    /* 'r' */ character_type::other,
    /* unused                    */ state::normal,    /* 's' */ character_type::type,
    /* unused                    */ state::normal,    /* 't' */ character_type::size,
    /* unused                    */ state::normal,    /* 'u' */ character_type::type,
    /* unused                    */ state::normal,    /* 'v' */ character_type::other,
    /* unused                    */ state::normal,    /* 'w' */ character_type::size,
    /* unused                    */ state::normal,    /* 'x' */ character_type::type,
    /* unused                    */ state::normal,    /* 'y' */ character_type::other,
    /* unused                    */ state::normal,    /* 'z' */ character_type::size
};

// Note, the state transition and character type are unrelated data - they just occupy the same table.
extern __declspec(selectany) printf_state_transition_table const format_validation_lookup_table_spectre
{
    /* prev state -> cur char ->    new state                   character type */
    /* normal     -> other    -> */ state::normal,    /* ' ' */ character_type::flag,
    /* percent    -> other    -> */ state::invalid,   /* '!' */ character_type::other,
    /* flag       -> other    -> */ state::invalid,   /* '"' */ character_type::other,
    /* width      -> other    -> */ state::invalid,   /* '#' */ character_type::flag,
    /* dot        -> other    -> */ state::invalid,   /* '$' */ character_type::other,
    /* precision  -> other    -> */ state::invalid,   /* '%' */ character_type::percent,
    /* size       -> other    -> */ state::invalid,   /* '&' */ character_type::other,
    /* type       -> other    -> */ state::normal,    /* ''' */ character_type::other,
    /* invalid    -> other    -> */ state::normal,    /* '(' */ character_type::other,
    /* normal     -> percent  -> */ state::percent,   /* ')' */ character_type::other,
    /* percent    -> percent  -> */ state::normal,    /* '*' */ character_type::star,
    /* flag       -> percent  -> */ state::invalid,   /* '+' */ character_type::flag,
    /* width      -> percent  -> */ state::invalid,   /* ',' */ character_type::other,
    /* dot        -> percent  -> */ state::invalid,   /* '-' */ character_type::flag,
    /* precision  -> percent  -> */ state::invalid,   /* '.' */ character_type::dot,
    /* size       -> percent  -> */ state::invalid,   /* '/' */ character_type::other,
    /* type       -> percent  -> */ state::percent,   /* '0' */ character_type::zero,
    /* invalid    -> percent  -> */ state::normal,    /* '1' */ character_type::digit,
    /* normal     -> dot      -> */ state::normal,    /* '2' */ character_type::digit,
    /* percent    -> dot      -> */ state::dot,       /* '3' */ character_type::digit,
    /* flag       -> dot      -> */ state::dot,       /* '4' */ character_type::digit,
    /* width      -> dot      -> */ state::dot,       /* '5' */ character_type::digit,
    /* dot        -> dot      -> */ state::invalid,   /* '6' */ character_type::digit,
    /* precision  -> dot      -> */ state::invalid,   /* '7' */ character_type::digit,
    /* size       -> dot      -> */ state::invalid,   /* '8' */ character_type::digit,
    /* type       -> dot      -> */ state::normal,    /* '9' */ character_type::digit,
    /* invalid    -> dot      -> */ state::normal,    /* ':' */ character_type::other,
    /* normal     -> star     -> */ state::normal,    /* ';' */ character_type::other,
    /* percent    -> star     -> */ state::width,     /* '<' */ character_type::other,
    /* flag       -> star     -> */ state::width,     /* '=' */ character_type::other,
    /* width      -> star     -> */ state::invalid,   /* '>' */ character_type::other,
    /* dot        -> star     -> */ state::precision, /* '?' */ character_type::other,
    /* precision  -> star     -> */ state::invalid,   /* '@' */ character_type::other,
    /* size       -> star     -> */ state::invalid,   /* 'A' */ character_type::type,
    /* type       -> star     -> */ state::normal,    /* 'B' */ character_type::other,
    /* invalid    -> star     -> */ state::normal,    /* 'C' */ character_type::type,
    /* normal     -> zero     -> */ state::normal,    /* 'D' */ character_type::other,
    /* percent    -> zero     -> */ state::flag,      /* 'E' */ character_type::type,
    /* flag       -> zero     -> */ state::flag,      /* 'F' */ character_type::size,
    /* width      -> zero     -> */ state::width,     /* 'G' */ character_type::type,
    /* dot        -> zero     -> */ state::precision, /* 'H' */ character_type::other,
    /* precision  -> zero     -> */ state::precision, /* 'I' */ character_type::size,
    /* size       -> zero     -> */ state::invalid,   /* 'J' */ character_type::other,
    /* type       -> zero     -> */ state::normal,    /* 'K' */ character_type::other,
    /* invalid    -> zero     -> */ state::normal,    /* 'L' */ character_type::size,
    /* normal     -> digit    -> */ state::normal,    /* 'M' */ character_type::other,
    /* percent    -> digit    -> */ state::width,     /* 'N' */ character_type::size,
    /* flag       -> digit    -> */ state::width,     /* 'O' */ character_type::other,
    /* width      -> digit    -> */ state::width,     /* 'P' */ character_type::other,
    /* dot        -> digit    -> */ state::precision, /* 'Q' */ character_type::other,
    /* precision  -> digit    -> */ state::precision, /* 'R' */ character_type::other,
    /* size       -> digit    -> */ state::invalid,   /* 'S' */ character_type::type,
    /* type       -> digit    -> */ state::normal,    /* 'T' */ character_type::size,
    /* invalid    -> digit    -> */ state::normal,    /* 'U' */ character_type::other,
    /* normal     -> flag     -> */ state::normal,    /* 'V' */ character_type::other,
    /* percent    -> flag     -> */ state::flag,      /* 'W' */ character_type::other,
    /* flag       -> flag     -> */ state::flag,      /* 'X' */ character_type::type,
    /* width      -> flag     -> */ state::invalid,   /* 'Y' */ character_type::other,
    /* dot        -> flag     -> */ state::invalid,   /* 'Z' */ character_type::type,
    /* precision  -> flag     -> */ state::invalid,   /* '[' */ character_type::other,
    /* size       -> flag     -> */ state::invalid,   /* '\' */ character_type::other,
    /* type       -> flag     -> */ state::normal,    /* ']' */ character_type::other,
    /* invalid    -> flag     -> */ state::normal,    /* '^' */ character_type::other,
    /* normal     -> size     -> */ state::normal,    /* '_' */ character_type::other,
    /* percent    -> size     -> */ state::size,      /* '`' */ character_type::other,
    /* flag       -> size     -> */ state::size,      /* 'a' */ character_type::type,
    /* width      -> size     -> */ state::size,      /* 'b' */ character_type::other,
    /* dot        -> size     -> */ state::size,      /* 'c' */ character_type::type,
    /* precision  -> size     -> */ state::size,      /* 'd' */ character_type::type,
    /* size       -> size     -> */ state::size,      /* 'e' */ character_type::type,
    /* type       -> size     -> */ state::normal,    /* 'f' */ character_type::type,
    /* invalid    -> size     -> */ state::normal,    /* 'g' */ character_type::type,
    /* normal     -> type     -> */ state::normal,    /* 'h' */ character_type::size,
    /* percent    -> type     -> */ state::type,      /* 'i' */ character_type::type,
    /* flag       -> type     -> */ state::type,      /* 'j' */ character_type::size,
    /* width      -> type     -> */ state::type,      /* 'k' */ character_type::other,
    /* dot        -> type     -> */ state::type,      /* 'l' */ character_type::size,
    /* precision  -> type     -> */ state::type,      /* 'm' */ character_type::other,
    /* size       -> type     -> */ state::type,      /* 'n' */ character_type::other,
    /* type       -> type     -> */ state::normal,    /* 'o' */ character_type::type,
    /* invalid    -> type     -> */ state::normal,    /* 'p' */ character_type::type,
    /* unused                    */ state::normal,    /* 'q' */ character_type::other,
    /* unused                    */ state::normal,    /* 'r' */ character_type::other,
    /* unused                    */ state::normal,    /* 's' */ character_type::type,
    /* unused                    */ state::normal,    /* 't' */ character_type::size,
    /* unused                    */ state::normal,    /* 'u' */ character_type::type,
    /* unused                    */ state::normal,    /* 'v' */ character_type::other,
    /* unused                    */ state::normal,    /* 'w' */ character_type::size,
    /* unused                    */ state::normal,    /* 'x' */ character_type::type,
    /* unused                    */ state::normal,    /* 'y' */ character_type::other,
    /* unused                    */ state::normal,    /* 'z' */ character_type::size
};

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Flags
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
enum FLAG : unsigned
{
    FL_SIGN       = 0x01, // Put plus or minus in front
    FL_SIGNSP     = 0x02, // Put space or minus in front
    FL_LEFT       = 0x04, // Left justify
    FL_LEADZERO   = 0x08, // Pad with leading zeros
    FL_SIGNED     = 0x10, // Signed data given
    FL_ALTERNATE  = 0x20, // Alternate form requested
    FL_NEGATIVE   = 0x40, // Value is negative
    FL_FORCEOCTAL = 0x80, // Force leading '0' for octals
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
    I,
    I32,
    I64,
    w,
    T,
    enumerator_count
};

inline size_t __cdecl to_integer_size(length_modifier const length) throw()
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
    case length_modifier::I:    return sizeof(void*    );
    case length_modifier::I32:  return sizeof(int32_t  );
    case length_modifier::I64:  return sizeof(int64_t  );
    default:                    return 0;
    }
}

template <typename Character>
bool __cdecl is_wide_character_specifier(
    uint64_t        const options,
    Character       const format_type,
    length_modifier const length
    ) throw()
{
    UNREFERENCED_PARAMETER(options);

    // If a length specifier was used, use that width:
    switch (length)
    {
    case length_modifier::l: return true;
    case length_modifier::w: return true;
    case length_modifier::h: return false;
    }

    if (length == length_modifier::T)
    {
        return sizeof(Character) == sizeof(wchar_t);
    }

    bool const is_naturally_wide{
        sizeof(Character) == sizeof(wchar_t) &&
        (options & _CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS) != 0
    };

    bool const is_natural_width{
        format_type == 'c' ||
        format_type == 's'
    };

    return is_naturally_wide == is_natural_width;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Core Base Classes
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// The format processor consists of a set of base classes that form a single
// inheritance hierarchy that looks like so:
//
//  * common_data
//    * output_adapter_data
//      * standard_base                   [1]
//        * format_validation_base        [2]
//          * positional_parameter_base   [3]
//
// There is a single class template, output_processor, which may be instantiated
// to derive from any of the three bottom-most classes in the aforementioned
// base class hierarchy (these are marked with the numbers 1-3). When it derives
// from standard_base, it provides the standard formatted output functionality
// as defined in the C Standard Library specification.  When it derives from the
// format_validation_base, it provides that same functionality, but also checks
// the format string for validity, and invokes the invalid parameter handler if
// the format string is determined to be invalid.  Finally, if it derives from
// the positional_parameter_base, it supports positional parameters.
//
// These three derivable bases allow us to implement the three forms of public
// functions using a common implementation:  the standard, unsuffixed functions,
// the format validating _s-suffixed functions, and the _p-suffixed functions
// that permit positional parameter usage.
//
// There are no virtual functions used here:  each of the three functionality
// bases "overrides" base class functionality by hiding it.  We must therefore
// be cautious to ensure that functions are called in such a way that the right
// implementation is called.
template <typename Character>
class common_data
{
protected:
    common_data(__crt_cached_ptd_host& ptd)
        : _options           {0            },
          _ptd               {ptd          },
          _format_it         {nullptr      },
          _valist_it         {nullptr      },
          _characters_written{0            },
          _state             {state::normal},
          _flags             {0            },
          _field_width       {0            },
          _precision         {0            },
          _suppress_output   {false        },
          _format_char       {'\0'         },
          _string_length     {0            },
          _string_is_wide    {false        }
    {
    }

    uint64_t                        _options;

    // We cache a reference to the PTD and updated locale information
    // to avoid having to query thread-local storage for every character
    // write that we perform.
    __crt_cached_ptd_host&          _ptd;

    // These two iterators control the formatting operation.  The format iterator
    // iterates over the format string, and the va_list argument pointer iterates
    // over the varargs arguments.
    Character const*                _format_it;
    va_list                         _valist_it;

    // This stores the number of characters that have been written so far.  It is
    // initialized to zero and is incremented as characters are written.  If an
    // I/O error occurs, it is set to -1 to indicate the I/O failure.
    int                             _characters_written;

    // These represent the state for the current format specifier.  The suppress
    // output flag is set when output should be suppressed for the current format
    // specifier (note that this is distinct from the global suppression that we
    // use during the first pass of positional parameter handling).
    state                           _state;
    unsigned                        _flags;
    int                             _field_width;
    int                             _precision;
    length_modifier                 _length;
    bool                            _suppress_output;

    // This is the character from the format string that was used to compute the
    // current state.  We need to store this separately because we advance the
    // format string iterator at various points during processing.
    Character                       _format_char;

    // These pointers are used in various places to point to strings that either
    // [1] are being formatted into, or [2] contain formatted data that is ready
    // to be printed.  At any given time, we may have either a narrow string or
    // a wide string, but never both.  The string length is the length of which-
    // -ever string is currently present.  The wide flag is set if the wide string
    // is currently in use.
    union
    {
        char*                       _narrow_string;
        wchar_t*                    _wide_string;
    };

    char*&    tchar_string(char   ) throw() { return _narrow_string; }
    wchar_t*& tchar_string(wchar_t) throw() { return _wide_string;   }

    Character*& tchar_string() throw() { return tchar_string(Character()); }

    int                             _string_length;
    bool                            _string_is_wide;

    // The formatting buffer.  This buffer is used to store the result of various
    // formatting operations--notably, numbers get formatted into strings in this
    // buffer.
    formatting_buffer               _buffer;
};

// This data base is split out from the common data base only so that we can
// more easily value-initialize all of the members of the common data base.
template <typename Character, typename OutputAdapter>
class output_adapter_data
    : protected common_data<Character>
{
protected:
#ifndef _MSC_VER // For retarded compilers!
    using common_data<Character>::_options;
    using common_data<Character>::_format_it;
    using common_data<Character>::_valist_it;
#endif
    output_adapter_data(
        OutputAdapter      const& output_adapter,
        uint64_t           const  options,
        Character const*   const  format,
        __crt_cached_ptd_host&    ptd,
        va_list            const  arglist
        ) throw()
        : common_data<Character>{ptd},
          _output_adapter(output_adapter)
    {
        // We initialize several base class data members here, so that we can
        // value initialize the entire base class before we get to this point.
        _options   = options;
        _format_it = format;
        _valist_it = arglist;
    }

    OutputAdapter _output_adapter;
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// standard_base
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This is the base class with which the format processor is instantiated to
// provide "standard" printf formatting.
template <typename Character, typename OutputAdapter>
class standard_base
    : protected output_adapter_data<Character, OutputAdapter>
{
protected:
#ifndef _MSC_VER // For retarded compilers!
    using common_data_base = typename output_adapter_data<Character, OutputAdapter>::template common_data<Character>;
    using common_data_base::_valist_it;
    using common_data_base::_field_width;
    using common_data_base::_precision;
#endif
    template <typename... Ts>
    standard_base(Ts&&... arguments) throw()
        : output_adapter_data<Character, OutputAdapter>{arguments...     },
          _current_pass      {pass::not_started}
    {
    }

    bool advance_to_next_pass() throw()
    {
        _current_pass = static_cast<pass>(static_cast<unsigned>(_current_pass) + 1);
        return _current_pass != pass::finished;
    }

    __forceinline bool validate_and_update_state_at_end_of_format_string() const throw()
    {
        // No validation is performed in the standard output implementation:
        return true;
    }

    bool should_format() throw()
    {
        return true;
    }

    template <typename RequestedParameterType, typename ActualParameterType>
    bool extract_argument_from_va_list(ActualParameterType& result) throw()
    {
        result = static_cast<ActualParameterType>(read_va_arg<RequestedParameterType>(_valist_it));

        return true;
    }

    bool update_field_width() throw()
    {
        _field_width = read_va_arg<int>(_valist_it);
        return true;
    }

    bool update_precision() throw()
    {
        _precision = read_va_arg<int>(_valist_it);
        return true;
    }

    bool validate_state_for_type_case_a() const throw()
    {
        return true;
    }

    bool should_skip_normal_state_processing() throw()
    {
        return false;
    }

    bool validate_and_update_state_at_beginning_of_format_character() throw()
    {
        return true;
    }

    bool should_skip_type_state_output() const throw()
    {
        return false;
    }

    static unsigned state_count() throw()
    {
        return static_cast<unsigned>(state::type) + 1;
    }

    static printf_state_transition_table const& state_transition_table() throw()
    {
        return standard_lookup_table_spectre;
    }

private:

    // In the standard format string processing, there is only one state, in
    // which we both evaluate the format specifiers and format the parameters.
    enum class pass : unsigned
    {
        not_started,
        output,
        finished
    };

    pass _current_pass;
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// format_validation_base
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This is the base class with which the format processor is instantiated to
// provide "standard" printf formatting with additional validation of the format
// string.
template <typename Character, typename OutputAdapter>
class format_validation_base
    : protected standard_base<Character, OutputAdapter>
{
protected:
#ifndef _MSC_VER // For retarded compilers!
    using common_data_base = typename standard_base<Character, OutputAdapter>::template output_adapter_data<Character, OutputAdapter>::template common_data<Character>;
    using common_data_base::_ptd;
    using common_data_base::_state;
#endif
    template <typename... Ts>
    format_validation_base(Ts&&... arguments) throw()
        : standard_base<Character, OutputAdapter>{arguments...}
    {
    }

    __forceinline bool validate_and_update_state_at_end_of_format_string() throw()
    {
        // When we reach the end of the format string, we ensure that the format
        // string is not incomplete.  I.e., when we are finished, the lsat thing
        // that we should have encountered is a regular character to be written
        // or a type specifier.  Otherwise, the format string was incomplete.
        _UCRT_VALIDATE_RETURN(_ptd, _state == state::normal || _state == state::type, EINVAL, false);

        return true;
    }

    static unsigned state_count() throw()
    {
        return static_cast<unsigned>(state::invalid) + 1;
    }

    static printf_state_transition_table const& state_transition_table() throw()
    {
        return format_validation_lookup_table_spectre;
    }
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// positional_parameter_base
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This is the base class with which the format processor is instantiated to
// provide support for the formatted output functions that permit positional
// parameter references in the format string.  Note that when this base is used,
// it also pulls in the format validation functionality.
template <typename Character, typename OutputAdapter>
class positional_parameter_base
    : protected format_validation_base<Character, OutputAdapter>
{
protected:
#if defined(__GNUC__) || defined(__clang__) // For retarded compilers!
    using common_data_base = typename format_validation_base<Character, OutputAdapter>::template standard_base<Character, OutputAdapter>::template output_adapter_data<Character, OutputAdapter>::template common_data<Character>;
    using output_adapter_data = typename format_validation_base<Character, OutputAdapter>::template standard_base<Character, OutputAdapter>::template output_adapter_data<Character, OutputAdapter>;
    using common_data_base::_format_it;
    using common_data_base::_ptd;
    using common_data_base::_field_width;
    using common_data_base::_precision;
    using common_data_base::_format_char;
    using common_data_base::_valist_it;
    using common_data_base::_length;
    using common_data_base::_state;
    using common_data_base::_options;
#endif

    typedef positional_parameter_base    self_type;
    typedef format_validation_base<Character, OutputAdapter>       base_type;
    typedef __crt_char_traits<Character> char_traits;

    template <typename... Ts>
    positional_parameter_base(Ts&&... arguments) throw()
        : format_validation_base<Character, OutputAdapter>{arguments...     },
          _current_pass         {pass::not_started},
          _format_mode          {mode::unknown    },
          _format               {_format_it       },
          _type_index           {-1               },
          _maximum_index        {-1               }
    {
        // Note that we do not zero-initialize the parameter data table until
        // the first positional parameter is encountered in the format string.
    }

    bool advance_to_next_pass() throw()
    {
        _current_pass = static_cast<pass>(static_cast<unsigned>(_current_pass) + 1);
        if (_current_pass == pass::finished)
            return false;

        // If we are in the output pass but the format string is a non-positional,
        // ordinary format string, then we do not need a second pass:
        if (_current_pass == pass::output && _format_mode == mode::nonpositional)
            return false;

        // All characters before the first format specifier are output in the
        // first pass.  We reset the format mode to 'unknown' to ensure that
        // they are not output again during the second pass.
        _format_mode = mode::unknown;

        _maximum_index = -1;
        _type_index    = -1;

        _field_width = 0;
        _precision   = 0;
        _format_it   = _format;

        return true;
    }

    bool validate_and_update_state_at_end_of_format_string() throw()
    {
        if (!base_type::validate_and_update_state_at_end_of_format_string())
            return false;

        if (_format_mode != mode::positional || _current_pass != pass::position_scan)
            return true;

        // At the end of the first pass, we have the types filled into the
        // arg_type member for each positional parameter.  We now need to get
        // the argument pointer for each positional parameter and store it
        // into the arg_ptr member.
        parameter_data* const first{_parameters};
        parameter_data* const last {_parameters + _maximum_index + 1};
        for (parameter_data* it{first}; it != last; ++it)
        {
            it->_valist_it = _valist_it;

            switch (it->_actual_type)
            {
            case parameter_type::int32:   read_va_arg<int        >(_valist_it); break;
            case parameter_type::int64:   read_va_arg<__int64    >(_valist_it); break;
            case parameter_type::pointer: read_va_arg<void*      >(_valist_it); break;
            case parameter_type::real64:  read_va_arg<_CRT_DOUBLE>(_valist_it); break;

            default:
                // We should never reach this point:
                _UCRT_VALIDATE_RETURN(_ptd, ("Missing position in the format string", 0), EINVAL, false);
                break;
            }
        }

        return true;
    }

    bool should_format() throw()
    {
        return _current_pass != pass::position_scan || _format_mode == mode::nonpositional;
    }

    template <typename RequestedParameterType, typename ActualParameterType>
    bool extract_argument_from_va_list(ActualParameterType& result) throw()
    {
        if (_format_mode == mode::nonpositional)
        {
            return base_type::template extract_argument_from_va_list<RequestedParameterType>(result);
        }

        _UCRT_VALIDATE_RETURN(_ptd, _type_index >= 0 && _type_index < _ARGMAX, EINVAL, false);

        if (_current_pass == pass::position_scan)
        {
            return validate_and_store_parameter_data(
                _parameters[_type_index],
                get_parameter_type(RequestedParameterType()),
                _format_char,
                _length
            );
        }
        else
        {
            result = static_cast<ActualParameterType>(peek_va_arg<RequestedParameterType>(_parameters[_type_index]._valist_it));
            return true;
        }
    }

    bool update_field_width() throw()
    {
        if (_format_mode == mode::nonpositional)
        {
            return base_type::update_field_width();
        }

        Character* end_pointer{nullptr};
        int const width_index{_tcstol_internal(_ptd, _format_it, &end_pointer, 10) - 1};
        _format_it = end_pointer + 1;

        if (_current_pass == pass::position_scan)
        {
            _UCRT_VALIDATE_RETURN(_ptd, width_index >= 0 && *end_pointer == '$' && width_index < _ARGMAX, EINVAL, false);

            _maximum_index = width_index > _maximum_index
                ? width_index
                : _maximum_index;

            return validate_and_store_parameter_data(
                _parameters[width_index],
                parameter_type::int32,
                _format_char,
                _length
            );
        }
        else
        {
            _field_width = peek_va_arg<int>(_parameters[width_index]._valist_it);
        }

        return true;
    }

    bool update_precision() throw()
    {
        if (_format_mode == mode::nonpositional)
        {
            return base_type::update_precision();
        }

        Character* end_pointer{nullptr};
        int const precision_index{_tcstol_internal(_ptd, _format_it, &end_pointer, 10) - 1};
        _format_it = end_pointer + 1;

        if (_current_pass == pass::position_scan)
        {
            _UCRT_VALIDATE_RETURN(_ptd, precision_index >= 0 && *end_pointer == '$' && precision_index < _ARGMAX, EINVAL, false);

            _maximum_index = precision_index > _maximum_index
                ? precision_index
                : _maximum_index;

            return validate_and_store_parameter_data(
                _parameters[precision_index],
                parameter_type::int32,
                _format_char,
                _length
            );
        }
        else
        {
            _precision = peek_va_arg<int>(_parameters[precision_index]._valist_it);
        }

        return true;
    }

    bool validate_state_for_type_case_a() throw()
    {
        if (_format_mode == mode::positional && _current_pass == pass::position_scan)
        {
            _UCRT_VALIDATE_RETURN(_ptd, _type_index >= 0 && _type_index < _ARGMAX, EINVAL, false);
            return validate_and_store_parameter_data(
                _parameters[_type_index],
                parameter_type::real64,
                _format_char,
                _length
            );
        }

        return true;
    }

    bool should_skip_normal_state_processing() throw()
    {
        if (_current_pass == pass::position_scan && _format_mode == mode::positional)
            return true;

        if (_current_pass == pass::output && _format_mode == mode::unknown)
            return true;

        // We do not output during the first pass if we have already come across
        // a positional format specifier.  All characters before the first format
        // specifier are output in the first pass.  We need to check the format
        // type during the second pass to ensure that they are not output a second
        // time.
        return false;
    }

    bool validate_and_update_state_at_beginning_of_format_character() throw()
    {
        // We're looking for a format specifier, so we'll have just seen a '%'
        // and the next character is not a '%':
        if (_state != state::percent || *_format_it == '%')
            return true;

        // When we encounter the first format specifier, we determine whether
        // the format string is a positional format string or a standard format
        // string.
        if (_format_mode == mode::unknown)
        {
            Character* end_pointer{nullptr};

            // Only digits are permitted between the % and the $ in the positional format specifier.
            if (*_format_it < '0' || *_format_it > '9')
            {
                _format_mode = mode::nonpositional;
            }
            else if (_tcstol_internal(_ptd, _format_it, &end_pointer, 10) > 0 && *end_pointer == '$')
            {
                if (_current_pass == pass::position_scan)
                {
                    memset(_parameters, 0, sizeof(_parameters));
                }

                _format_mode = mode::positional;
            }
            else
            {
                _format_mode = mode::nonpositional;
            }
        }

        if (_format_mode != mode::positional)
        {
            return true;
        }

        Character* end_pointer{nullptr};
        _type_index = _tcstol_internal(_ptd, _format_it, &end_pointer, 10) - 1;
        _format_it = end_pointer + 1;

        if (_current_pass != pass::position_scan)
            return true;

        // We do not re-perform the type validations during the second pass...
        _UCRT_VALIDATE_RETURN(_ptd, _type_index >= 0 && *end_pointer == '$' && _type_index < _ARGMAX, EINVAL, false);

        _maximum_index = _type_index > _maximum_index
            ? _type_index
            : _maximum_index;

        return true;
    }

    bool should_skip_type_state_output() const throw()
    {
        return _format_mode == mode::positional && _current_pass == pass::position_scan;
    }

private:

    // Positional parameter processing occurs in two passes.  In the first pass,
    // we scan the format string to accumulate type information for the
    // positional parameters.  If the format string is a standard format string
    // (that does not use positional parameters), then this pass also does the
    // output.  Otherwise, if the format string uses positional parameters,
    // the actual output operation takes place in the second pass, after we've
    // aggregated all of the information about the positional parameters.
    enum class pass : unsigned
    {
        not_started,
        position_scan,
        output,
        finished
    };

    // These represent the two modes of formatting.  The processor starts off in
    // the unknown mode and can transition to either nonpositional (indicating
    // standard format string processing) or positional mode.
    enum class mode : unsigned
    {
        unknown,
        nonpositional,
        positional
    };

    // These represent the different types of parameters that may be present in
    // the argument list (remember that integral types narrower than int and
    // real types narrower than double are promoted for the varargs call).
    enum class parameter_type : unsigned
    {
        unused,
        int32,
        int64,
        pointer,
        real64
    };

    // This represents a parameter in the varargs list, along with information
    // we have about the parameter from the format string.  We store an array
    // of these structures internally, and the structures are updated in several
    // steps.  They start off uninitialized, with the "unused" actual type.  If
    // the format string uses positional parameters, then the actual type, format
    // type, and flags are updated during the positional scan of the format
    // string.  At the end of that scan, we iterate over the parameters and the
    // varargs array to fill in the argptr for each of the positional parameters.
    struct parameter_data
    {
        parameter_type  _actual_type;
        Character       _format_type;

        va_list         _valist_it;
        length_modifier _length;
    };

    // These provide transformations from the source parameter types to their
    // underlying representation enumerators (from the parameter_type enum).
    template <typename T>
    static parameter_type __cdecl get_parameter_type(T*              ) throw() { return parameter_type::pointer; }
    static parameter_type __cdecl get_parameter_type(short           ) throw() { return parameter_type::int32;   }
    static parameter_type __cdecl get_parameter_type(unsigned short  ) throw() { return parameter_type::int32;   }
    static parameter_type __cdecl get_parameter_type(wchar_t         ) throw() { return parameter_type::int32;   }
    static parameter_type __cdecl get_parameter_type(int             ) throw() { return parameter_type::int32;   }
    static parameter_type __cdecl get_parameter_type(unsigned int    ) throw() { return parameter_type::int32;   }
    static parameter_type __cdecl get_parameter_type(__int64         ) throw() { return parameter_type::int64;   }
    static parameter_type __cdecl get_parameter_type(unsigned __int64) throw() { return parameter_type::int64;   }
    static parameter_type __cdecl get_parameter_type(_CRT_DOUBLE     ) throw() { return parameter_type::real64;  }

    // With positional parameters, a given parameter may be used multiple times
    // in a format string.  All appearances of a given parameter must be
    // consistent (they must match in type and width).  The first time a given
    // parameter appears, it is accepted as-is.  Each subsequent time that it
    // reappears, this function is called to ensure that the reappearance is
    // consistent with the previously processed appearance.
    //
    // Returns true if the reappearance is consistent (and valid).  Returns false
    // otherwise.  This function does not invoke invalid parameter handling.
    bool is_positional_parameter_reappearance_consistent(
        parameter_data  const& parameter,
        parameter_type  const  actual_type,
        Character       const  format_type,
        length_modifier const  length
        ) throw()
    {
        // Pointer format specifiers are exclusive; a parameter that previously
        // appeared as a pointer may not reappear as a non-pointer, and vice-
        // versa.
        bool const old_is_pointer{is_pointer_specifier(parameter._format_type)};
        bool const new_is_pointer{is_pointer_specifier(format_type)};
        if (old_is_pointer || new_is_pointer)
        {
            return old_is_pointer == new_is_pointer;
        }

        // String format specifiers are exclusive, just like pointer specifiers.
        // We must also ensure that the two appearances match in string type:
        // either both must be wide or both must be narrow.
        bool const old_is_string{is_string_specifier(parameter._format_type)};
        bool const new_is_string{is_string_specifier(format_type)};

        bool const old_is_character{is_character_specifier(parameter._format_type)};
        bool const new_is_character{is_character_specifier(format_type)};
        if (old_is_string || new_is_string || old_is_character || new_is_character)
        {
            if (old_is_string != new_is_string || old_is_character != new_is_character)
                return false;

            bool const old_is_wide{is_wide_character_specifier(_options, parameter._format_type, parameter._length)};
            bool const new_is_wide{is_wide_character_specifier(_options, format_type, length)};
            if (old_is_wide != new_is_wide)
                return false;

            return true;
        }

        // Numeric specifiers are exclusive:  either both appearances must be
        // numeric specifiers or neither appearance may be a numeric specifier.
        // Additionally, if both appearances are numeric specifiers, they must
        // both have the same width.
        bool const old_is_integral{is_integral_specifier(parameter._format_type)};
        bool const new_is_integral{is_integral_specifier(format_type)};
        if (old_is_integral || new_is_integral)
        {
            if (old_is_integral != new_is_integral)
                return false;

            if ((parameter._length == length_modifier::I) != (length == length_modifier::I))
                return false;

            return to_integer_size(parameter._length) == to_integer_size(length);
        }

        return parameter._actual_type == actual_type;
    }

    bool validate_and_store_parameter_data(
        parameter_data       & parameter,
        parameter_type  const  actual_type,
        Character       const  format_type,
        length_modifier const  length
        ) throw()
    {
        if (parameter._actual_type == parameter_type::unused)
        {
            parameter._actual_type = actual_type;
            parameter._format_type = format_type;
            parameter._length      = length;
        }
        else
        {
            _UCRT_VALIDATE_RETURN(_ptd, is_positional_parameter_reappearance_consistent(
                parameter, actual_type, format_type, length
            ), EINVAL, false);
        }

        return true;
    }

    template <typename Character2>
    static bool __cdecl is_pointer_specifier(Character2 const specifier) throw()
    {
        return specifier == 'p';
    }

    template <typename Character2>
    static bool __cdecl is_string_specifier(Character2 const specifier) throw()
    {
        return specifier == 's' || specifier == 'S';
    }

    template <typename Character2>
    static bool __cdecl is_character_specifier(Character2 const specifier) throw()
    {
        return specifier == 'c' || specifier == 'C';
    }

    template <typename Character2>
    static bool __cdecl is_integral_specifier(Character2 const specifier) throw()
    {
        return specifier == 'd' || specifier == 'i' || specifier == 'o'
            || specifier == 'u' || specifier == 'x' || specifier == 'X'
            || specifier == '*';
    }

    pass _current_pass;
    mode _format_mode;

    // The original format pointer, which allows us to reset the format iterator
    // between the two passes used in positional parameter processing.
    Character const* _format;

    // The array of positional parameters used in the format string.
    parameter_data _parameters[_ARGMAX];

    // These are indices into the _parameters array.  They hold the maximum
    // positional parameter index that has been seen so far and the index of the
    // current format specifier.  Until a positional parameter is encountered in
    // the format string, these have the value -1.
    int _maximum_index;
    int _type_index;
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// output_processor
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// This is the main output processor class, which performs the format string
// processing and formats the varargs arguments appropriately.  It delegates to
// the provided ProcessorBase for certain parts of the functionality.  It is
// expected that the ProcessorBase argument is one of the base class types
// defined above.
template <typename Character, typename OutputAdapter, typename ProcessorBase>
class output_processor
    : private ProcessorBase
{
public:
#ifndef _MSC_VER // For retarded compilers!
    using ProcessorBase::advance_to_next_pass;
    using ProcessorBase::validate_and_update_state_at_beginning_of_format_character;
    using ProcessorBase::validate_and_update_state_at_end_of_format_string;
    using ProcessorBase::should_skip_normal_state_processing;
    using ProcessorBase::update_field_width;
    using ProcessorBase::should_format;
    using ProcessorBase::update_precision;
    using ProcessorBase::should_skip_type_state_output;
    using ProcessorBase::validate_state_for_type_case_a;
    using ProcessorBase::tchar_string;
    using ProcessorBase::state_transition_table;
    using ProcessorBase::state_count;
    using oad_base = typename ProcessorBase::output_adapter_data;
    using oad_base::_output_adapter;
    using common_data_base = typename ProcessorBase::common_data_base;
    using common_data_base::_string_length;
    using common_data_base::_ptd;
    using common_data_base::_format_it;
    using common_data_base::_state;
    using common_data_base::_format_char;
    using common_data_base::_characters_written;
    using common_data_base::_string_is_wide;
    using common_data_base::_field_width;
    using common_data_base::_suppress_output;
    using common_data_base::_flags;
    using common_data_base::_precision;
    using common_data_base::_length;
    using common_data_base::_options;
    using common_data_base::_buffer;
    using common_data_base::_narrow_string;
    using common_data_base::_wide_string;
#endif

    typedef __acrt_stdio_char_traits<Character> char_traits;

    output_processor(
        OutputAdapter      const& output_adapter,
        uint64_t           const  options,
        Character const*   const  format,
        __crt_cached_ptd_host&    ptd,
        va_list            const  arglist
        ) throw()
        : ProcessorBase{output_adapter, options, format, ptd, arglist}
    {
    }

    // After construction, this function is called to evaluate the formatted
    // output operation.  This function must be called exactly once, immediately
    // after constructing the object.
    int process() throw()
    {
        if (!_output_adapter.validate(_ptd))
        {
            return -1;
        }

        _UCRT_VALIDATE_RETURN(_ptd, _format_it != nullptr, EINVAL, -1);

        while (advance_to_next_pass())
        {
            // At the start of each pass, we have no buffered string and we are
            // in the normal state:
            _string_length = 0;
            _state         = state::normal;

            // Iterate over the format string until we reach the end, encounter
            // an I/O error, or fail due to some other error:
            while ((_format_char = *_format_it++) != '\0' && _characters_written >= 0)
            {
                _state = find_next_state(_format_char, _state);

                if (!validate_and_update_state_at_beginning_of_format_character())
                {
                    return -1;
                }

                if (_state >= state::invalid)
                {
                    _UCRT_VALIDATE_RETURN(_ptd, ("Incorrect format specifier", 0), EINVAL, -1);
                }

                bool result = false;

                switch (_state)
                {
                case state::normal:    result = state_case_normal   (); break;
                case state::percent:   result = state_case_percent  (); break;
                case state::flag:      result = state_case_flag     (); break;
                case state::width:     result = state_case_width    (); break;
                case state::dot:       result = state_case_dot      (); break;
                case state::precision: result = state_case_precision(); break;
                case state::size:      result = state_case_size     (); break;
                case state::type:      result = state_case_type     (); break;
                }

                // If the state-specific operation failed, return immediately.
                // The individual state cases are responsible for invoking the
                // invalid parameter handler if the failure is due to an invalid
                // parameter.
                if (!result)
                    return -1;
            }

            if (!validate_and_update_state_at_end_of_format_string())
                return -1;
        }

        return _characters_written;
    }

private:

    // The normal state is entered when a character that is not part of a format
    // specifier is encountered in the format string.  We simply write the
    // character.  There are four parts to the normal state:  the state_case_normal
    // function is called when the state is entered.  It tests whether the actual
    // output operation should take place.  The state_case_normal_common function
    // performs the actual output operation and is called from elsewhere in this
    // class.
    __forceinline bool state_case_normal() throw()
    {
        if (should_skip_normal_state_processing())
            return true;

        _UCRT_VALIDATE_RETURN(_ptd, state_case_normal_common(), EINVAL, false);

        return true;
    }

    __forceinline bool state_case_normal_common() throw()
    {
        if (!state_case_normal_tchar(Character()))
            return false;

        _output_adapter.write_character(_format_char, &_characters_written, _ptd);
        return true;
    }

    __forceinline bool state_case_normal_tchar(char) throw()
    {
        _string_is_wide = false;

        if (__acrt_isleadbyte_l_noupdate(_format_char, _ptd.get_locale()))
        {
            _output_adapter.write_character(_format_char, &_characters_written, _ptd);
            _format_char = *_format_it++;

            // Ensure that we do not fall off the end of the format string:
            _UCRT_VALIDATE_RETURN(_ptd, _format_char != '\0', EINVAL, false);
        }

        return true;
    }

    __forceinline bool state_case_normal_tchar(wchar_t) throw()
    {
        _string_is_wide = true;
        return true;
    }

    // We enter the percent state when we read a '%' from the format string.  The
    // percent sign begins a format specifier, so we reset our internal state for
    // the new format specifier.
    __forceinline bool state_case_percent() throw()
    {
        _field_width     =  0;
        _suppress_output =  false;
        _flags           =  0;
        _precision       = -1;
        _length          =  length_modifier::none;
        _string_is_wide  =  false;

        return true;
    }

    // We enter the flag state when we are reading a format specifier and we
    // encounter one of the optional flags.  We update our state to account for
    // the flag.
    __forceinline bool state_case_flag() throw()
    {
        // Set the flag based on which flag character:
        switch (_format_char)
        {
        case '-': set_flag(FL_LEFT     ); break; // '-' => left justify
        case '+': set_flag(FL_SIGN     ); break; // '+' => force sign indicator
        case ' ': set_flag(FL_SIGNSP   ); break; // ' ' => force sign or space
        case '#': set_flag(FL_ALTERNATE); break; // '#' => alternate form
        case '0': set_flag(FL_LEADZERO ); break; // '0' => pad with leading zeros
        }

        return true;
    }

    // Parses an integer from the format string.  It is expected in this function
    // that it is called _after_ the format string iterator has been advanced to
    // the next character, so it starts parsing from _format_it - 1 (i.e., from
    // the character currently being processed, a copy of which is stored in
    // the _format_char data member).
    bool parse_int_from_format_string(int* const result) throw()
    {
        auto const reset_errno = _ptd.get_errno().create_guard();

        Character* end{};
        *result = static_cast<int>(_tcstol_internal(_ptd,
            _format_it - 1,
            &end,
            10));

        if (_ptd.get_errno().check(ERANGE))
        {
            return false;
        }

        if (end < _format_it)
        {
            return false;
        }

        _format_it = end;
        return true;
    }

    // We enter the width state when we are reading a format specifier and we
    // encounter either an asterisk (indicating the width should be read from
    // the varargs) or a digit (indicating that we are in the process of reading
    // the width from the format string.
    __forceinline bool state_case_width() throw()
    {
        if (_format_char != '*')
        {
            return parse_int_from_format_string(&_field_width);
        }

        // If the format character is an asterisk, we read the width from the
        // varargs.  If we read a negative value, we treat it as the '-' flag
        // followed by a positive width (per the C Standard Library spec).
        if (!update_field_width())
            return false;

        if (!should_format())
            return true;

        if (_field_width < 0)
        {
            set_flag(FL_LEFT);
            _field_width = -_field_width;
        }

        return true;
    }

    // We enter the dot state when we read a '.' from the format string.  This
    // '.' introduces the precision part of the format specifier.
    __forceinline bool state_case_dot() throw()
    {
        // Reset the precision to zero.  If the dot is not followed by a number,
        // it means a precision of zero, not the default precision (per the C
        // Standard Library specification).  (Note:  We represent the default
        // precision with -1.)
        _precision = 0;

        return true;
    }

    // We enter the precision state after we read a ',' from the format string.
    // At this point, we read the precision, in a manner similar to how we read
    // the width.
    __forceinline bool state_case_precision() throw()
    {
        if (_format_char != '*')
        {
            return parse_int_from_format_string(&_precision);
        }

        // If the format character is an asterisk, we read the width from the
        // varargs.  If we read a negative value, we treat it as indicating the
        // default precision.
        if (!update_precision())
            return false;

        if (!should_format())
            return true;

        if (_precision < 0)
            _precision = -1;

        return true;
    }

    // We enter the size state when we have read one of the size characters from
    // the format string.
    bool state_case_size() throw()
    {
        if (_format_char == 'F')
        {
            // We hand the 'F' character as a length modifier because the length
            // modifier occurs before the type specifier.  If we find an 'F' and
            // we are in the legacy compatibility mode that supports the 'F' length
            // modifier, we just ignore it (it has no meaning).  Otherwise we are
            // not in compatibility mode so we switch out to the type case to handle
            // the 'F' as a %F format specifier:
            if ((_options & _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY) == 0)
            {
                _state = state::type;
                return state_case_type();
            }

            return true;
        }

        if (_format_char == 'N')
        {
            // If we find an 'N' and we are in the legacy compatibility mode that
            // supports the 'N' length modifier, we just ignore it (it has no
            // meaning).  Otherwise, we are not in compatibility mode, so we
            // invoke the invalid parameter handler and return failure.
            if ((_options & _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY) == 0)
            {
                _state = state::invalid;
#pragma warning(suppress: __WARNING_IGNOREDBYCOMMA) // 6319 comma operator
                _UCRT_VALIDATE_RETURN(_ptd, ("N length modifier not specifier", false), EINVAL, false);
                return false;
            }

            return true;
        }

        _UCRT_VALIDATE_RETURN(_ptd, _length == length_modifier::none, EINVAL, false);

        // We just read a size specifier; set the flags based on it:
        switch (_format_char)
        {
        case 'h':
        {
            if (*_format_it == 'h')
            {
                ++_format_it;
                _length = length_modifier::hh;
            }
            else
            {
                _length = length_modifier::h;
            }

            return true;
        }

        case 'I':
        {
            // The I32, I64, and I length modifiers are Microsoft extensions.

            if (*_format_it == '3' && *(_format_it + 1) == '2')
            {
                _format_it += 2;
                _length = length_modifier::I32;
            }
            else if (*_format_it == '6' && *(_format_it + 1) == '4')
            {
                _format_it += 2;
                _length = length_modifier::I64;
            }
            else if (*_format_it == 'd' ||
                     *_format_it == 'i' ||
                     *_format_it == 'o' ||
                     *_format_it == 'u' ||
                     *_format_it == 'x' ||
                     *_format_it == 'X')
            {
                // If we support positional parameters, then %I without a following
                // 32 or 64 is platform-dependent:
                _length = length_modifier::I;
            }

            return true;
        }

        case 'l':
        {
            if (*_format_it == 'l')
            {
                ++_format_it;
                _length = length_modifier::ll;
            }
            else
            {
                _length = length_modifier::l;
            }

            return true;
        }

        case 'L':
        {
            _length = length_modifier::L;
            return true;
        }

        case 'j':
        {
            _length = length_modifier::j;
            return true;
        }

        case 't':
        {
            _length = length_modifier::t;
            return true;
        }

        case 'z':
        {
            _length = length_modifier::z;
            return true;
        }

        case 'w':
        {
            _length = length_modifier::w;
            return true;
        }

        case 'T':
        {
            _length = length_modifier::T;
            return true;
        }
        }

        return true;
    }

    // We enter the type case when we read the type part of the format specifier.
    // At this point, we have read the entire format specifier and we can extract
    // the value to be formatted from the varargs, format it, then output it.
    // This state is broken up into subfunctions, one per type category.
    bool state_case_type() throw()
    {
        // Each of the subfunctions is responsible for [1] extracting the next
        // argument from the varargs, [2] formatting that argument into the
        // internal buffer, and [3] updating the string state variables with
        // the correct data for the format operation (either _narrow_string or
        // _wide_string must be set correctly, the _string_length must be set
        // correctly, and _string_is_wide must be true if the wide string
        // should be used.
        bool result{false};
        switch (_format_char)
        {
        // Individual character output:
        case 'C':
        case 'c': result = type_case_c(); break;

        // String output:
        case 'Z': result = type_case_Z(); break;
        case 'S':
        case 's': result = type_case_s(); break;

        // Floating-point output:
        case 'A':
        case 'E':
        case 'F':
        case 'G':
        case 'a':
        case 'e':
        case 'f':
        case 'g': result = type_case_a(); break;

        // Integer and pointer output:
        case 'd':
        case 'i': result = type_case_d(); break;
        case 'u': result = type_case_u(); break;
        case 'o': result = type_case_o(); break;
        case 'X': result = type_case_X(); break;
        case 'x': result = type_case_x(); break;
        case 'p': result = type_case_p(); break;

        // State reporting (no output):
        case 'n': result = type_case_n(); break;
        }

        // If the case-specific logic failed, return immediately.  The case-
        // specific function is responsible for invoking the invalid parameter
        // handler if it needs to do so.
        if (!result)
            return false;

        // Check to see whether the output operation should be skipped (we skip
        // the output part e.g. during the positional scan pass when positional
        // formatting is used).
        if (should_skip_type_state_output())
            return true;

        // At this point, we've completed the bulk of the formatting operation
        // and the string is ready to be printed.  We now justify the string,
        // pre-pend any required prefix and leading zeroes, then print it.  Well,
        // unless output is suppressed, that is... :-)
        if (_suppress_output)
            return true;

        // Compute the prefix, if one is required...
        Character  prefix[3]{};
        size_t     prefix_length{0};

        if (has_flag(FL_SIGNED))
        {
            if (has_flag(FL_NEGATIVE))
            {
                prefix[prefix_length++] = '-';
            }
            else if (has_flag(FL_SIGN))
            {
                prefix[prefix_length++] = '+';
            }
            else if (has_flag(FL_SIGNSP))
            {
                prefix[prefix_length++] = ' ';
            }
        }

        bool const print_integer_0x{(_format_char == 'x' || _format_char == 'X') && has_flag(FL_ALTERNATE)};
        bool const print_floating_point_0x{_format_char == 'a' || _format_char == 'A'};

        if (print_integer_0x || print_floating_point_0x)
        {
            prefix[prefix_length++] = '0';
            prefix[prefix_length++] = adjust_hexit('x' - 'a' + '9' + 1, _format_char == 'X' || _format_char == 'A');
        }

        // Compute the amount of padding required to get to the desired field
        // width, then output the left padding, prefix, leading zeroes, the
        // string, and right padding, in that order.
        int const padding = static_cast<int>(_field_width - _string_length - prefix_length);

        if (!has_flag(FL_LEFT | FL_LEADZERO))
        {
            // Left-pad with spaces
            write_multiple_characters(_output_adapter, ' ', padding, &_characters_written, _ptd);
        }

        // Write the prefix
        _output_adapter.write_string(prefix, static_cast<int>(prefix_length), &_characters_written, _ptd);

        if (has_flag(FL_LEADZERO) && !has_flag(FL_LEFT))
        {
            // Write leading zeroes
            write_multiple_characters(_output_adapter, '0', padding, &_characters_written, _ptd);
        }

        // Write the string
        write_stored_string_tchar(Character());

        if (_characters_written >= 0 && has_flag(FL_LEFT))
        {
            // Right-pad with spaces
            write_multiple_characters(_output_adapter, ' ', padding, &_characters_written, _ptd);
        }

        return true;
    }

    // Individual character output:  The 'C' and 'c' format specifiers output an
    // individual wide or narrow character, respectively.  They both delegate to
    // the type_case_c_tchar() overloads, which handle the output appropriately
    // for the character type of the output adapter.
    bool type_case_c() throw()
    {
        return type_case_c_tchar(Character());
    }

    bool type_case_c_tchar(char) throw()
    {
        // If the character is a wide character, we translate it to multibyte
        // to be output, storing the multibyte string in the internal buffer:
        if (is_wide_character_specifier(_options, _format_char, _length))
        {
            wchar_t wide_character{};
            if (!this->template extract_argument_from_va_list<wchar_t>(wide_character))
            {
                return false;
            }

            if (!should_format())
            {
                return true;
            }

            // Convert to multibyte.  If the conversion fails, we suppress the
            // output operation but we do not fail the entire operation:
            errno_t const status{_wctomb_internal(&_string_length, _buffer.template data<char>(), _buffer.template count<char>(), wide_character, _ptd)};
            if (status != 0)
            {
                _suppress_output = true;
            }
        }
        // If the character is a narrow character, we can just write it directly
        // to the output, as-is.
        else
        {
            if (!this->template extract_argument_from_va_list<unsigned short>(_buffer.template data<char>()[0]))
            {
                return false;
            }

            if (!should_format())
            {
                return true;
            }

            _string_length = 1;
        }

        _narrow_string = _buffer.template data<char>();
        return true;
    }

    bool type_case_c_tchar(wchar_t) throw()
    {
        // If the output adapter accepts wide characters, then we must transform
        // the character into a wide character to be output.
        _string_is_wide = true;

        wchar_t wide_character{};
        if (!this->template extract_argument_from_va_list<wchar_t>(wide_character))
            return false;

        if (!should_format())
            return true;

        if (!is_wide_character_specifier(_options, _format_char, _length))
        {
            // If the character is actually a multibyte character, then we must
            // transform it into the equivalent wide character.  If the translation
            // is unsuccessful, we ignore this character but do not fail the entire
            // output operation.
            char const local_buffer[2]{ static_cast<char>(wide_character & 0x00ff), '\0' };
            int const mbc_length{_mbtowc_internal(
                _buffer.template data<wchar_t>(),
                local_buffer,
                _ptd.get_locale()->locinfo->_public._locale_mb_cur_max,
                _ptd
                )};
            if (mbc_length < 0)
            {
                _suppress_output = true;
            }
        }
        else
        {
            _buffer.template data<wchar_t>()[0] = wide_character;
        }

        _wide_string   = _buffer.template data<wchar_t>();
        _string_length = 1;
        return true;
    }

    // String Output:  The Z, S, and s format specifiers output a string.  The Z
    // format specifier is an extension, used to print a Windows SDK ANSI_STRING
    // or UNICODE_STRING counted string.  The S and s format specifiers output a
    // wide or narrow C string, respectively.  If a null pointer is passed, we
    // replace it with a special sentinel string, with the contents "(null)".
    bool type_case_Z() throw()
    {
        // This matches the representation of the Windows SDK types ANSI_STRING
        // and UNICODE_STRING, which represent a counted string.
        struct ansi_string
        {
            unsigned short _length;
            unsigned short _maximum_length;
            char*          _buffer;
        };

        ansi_string* string{};
        if (!this->template extract_argument_from_va_list<ansi_string*>(string))
            return false;

        if (!should_format())
            return true;

        if (!string || string->_buffer == nullptr)
        {
            _narrow_string  = narrow_null_string();
            _string_length  = static_cast<int>(strlen(_narrow_string));
            _string_is_wide = false;
        }
        else if (is_wide_character_specifier(_options, _format_char, _length))
        {
            _wide_string    = reinterpret_cast<wchar_t*>(string->_buffer);
            _string_length  = string->_length / static_cast<int>(sizeof(wchar_t));
            _string_is_wide = true;
        }
        else
        {
            _narrow_string  = string->_buffer;
            _string_length  = string->_length;
            _string_is_wide = false;
        }

        return true;
    }

    bool type_case_s() throw()
    {
        // If this format specifier has the default precision, then the entire
        // string is output.  If a precision is given, then we output the minimum
        // of the length of the C string and the given precision.  Note that the
        // string needs not be null-terminated if a precision is given, so we
        // cannot call strlen to compute the length of the string.
        if (!this->template extract_argument_from_va_list<char*>(_narrow_string))
            return false;

        if (!should_format())
            return true;

        int const maximum_length{(_precision == -1) ? INT_MAX : _precision};

        if (is_wide_character_specifier(_options, _format_char, _length))
        {
            if (!_wide_string)
                _wide_string = wide_null_string();

            _string_is_wide = true;
            _string_length  = static_cast<int>(wcsnlen(_wide_string, maximum_length));
        }
        else
        {
            if (!_narrow_string)
                _narrow_string = narrow_null_string();

            _string_length = type_case_s_compute_narrow_string_length(maximum_length, Character());
        }

        return true;
    }

    // We have two different implementations of the 's' type case, to handle
    // narrow and wide strings, since computation of the length is subtly
    // different depending on whether we are outputting narrow or wide
    // characters.  These functions just update the string state appropriately
    // for the string that has just been read from the varargs.
    int type_case_s_compute_narrow_string_length(int const maximum_length, char) throw()
    {
        return static_cast<int>(strnlen(_narrow_string, maximum_length));
    }

    int type_case_s_compute_narrow_string_length(int const maximum_length, wchar_t) throw()
    {
        _locale_t locale = _ptd.get_locale();
        int string_length{0};

        for (char const* p{_narrow_string}; string_length < maximum_length && *p; ++string_length)
        {
            if (__acrt_isleadbyte_l_noupdate(static_cast<unsigned char>(*p), locale))
            {
                ++p;
            }

            ++p;
        }

        return string_length;
    }

    // Floating-point output:  The A case handles the A, E, F, and G format specifiers.
    // The a case handles the a, e, f, and g format specifiers.  A capital format
    // specifier causes us to output capital hexits, whereas a lowercase format
    // specifier causes us to output lowercase hexits.
    bool type_case_a() throw()
    {
        // The double type is signed:
        set_flag(FL_SIGNED);

        if (!validate_state_for_type_case_a())
            return false;

        if (!should_format())
            return true;

        // First, we need to compute the actual precision to use, limited by
        // both the maximum precision and the size of the buffer that we can
        // allocate.
        if (_precision < 0)
        {
            // The default precision depends on the format specifier used.  For
            // %e, %f, and %g, C specifies that the default precision is 6.  For
            // %a, C specifies that "if the precision is missing and FLT_RADIX
            // is a power of 2, then the precision is sufficient for an exact
            // representation of the value" (C11 7.21.6.1/8).
            //
            // The 64-bit double has 53 bits of precision.  When printing in
            // hexadecimal form, we print one bit of precision to the left of the
            // radix point and the remaining 52 bits of precision to the right.
            // Thus, the default precision is 13 (13 * 4 == 52).
            if (_format_char == 'a' || _format_char == 'A')
            {
                _precision = 13;
            }
            else
            {
                _precision = 6;
            }
        }
        else if (_precision == 0 && (_format_char == 'g' || _format_char == 'G'))
        {
            _precision = 1; // Per C Standard Library specification.
        }

        if (!_buffer.template ensure_buffer_is_big_enough<char>(_CVTBUFSIZE + _precision, _ptd))
        {
            // If we fail to enlarge the buffer, cap precision so that the
            // statically-sized buffer may be used for the formatting:
            _precision = static_cast<int>(_buffer.template count<char>() - _CVTBUFSIZE);
        }

        _narrow_string = _buffer.template data<char>();

        // Note that we separately handle the FORMAT_POSSCAN_PASS above.
        _CRT_DOUBLE tmp{};
        if (!this->template extract_argument_from_va_list<_CRT_DOUBLE>(tmp))
        {
            return false;
        }

        // Format the number into the buffer:
        __acrt_fp_format(
            &tmp.x,
            _buffer.template data<char>(),
            _buffer.template count<char>(),
            _buffer.template scratch_data<char>(),
            _buffer.template scratch_count<char>(),
            static_cast<char>(_format_char),
            _precision,
            _options,
            __acrt_rounding_mode::standard,
            _ptd);

        // If the precision is zero but the '#' flag is part of the specifier,
        // we force a decimal point:
        if (has_flag(FL_ALTERNATE) && _precision == 0)
        {
            force_decimal_point(_narrow_string, _ptd.get_locale());
        }

        // The 'g' format specifier indicates that zeroes should be cropped
        // unless the '#' flag is part of the specifier.
        if ((_format_char == 'g' || _format_char == 'G') && !has_flag(FL_ALTERNATE))
        {
            crop_zeroes(_narrow_string, _ptd.get_locale());
        }

        // If the result was negative, we save the '-' for later and advance past
        // the negative sign (we handle the '-' separately, in code shared with
        // the integer formatting, to correctly handle flags).
        if (*_narrow_string == '-')
        {
            set_flag(FL_NEGATIVE);
            ++_narrow_string;
        }

        // If the result was a special infinity or a nan string, suppress output
        // of the "0x" prefix by treating the special string as just a string:
        if (*_narrow_string == 'i' || *_narrow_string == 'I' ||
            *_narrow_string == 'n' || *_narrow_string == 'N')
        {
            unset_flag(FL_LEADZERO); // padded with spaces, not zeros.
            _format_char = 's';
        }

        _string_length = static_cast<int>(strlen(_narrow_string));

        return true;
    }

    // Integer output:  These functions handle the formatting of integer values.
    // There are a number of format specifiers that handle integer formatting;
    // we handle them separately, but they all end up calling the common function
    // type_case_integer(), defined last.
    bool type_case_d() throw()
    {
        set_flag(FL_SIGNED);

        return type_case_integer<10>();
    }

    bool type_case_u() throw()
    {
        return type_case_integer<10>();
    }

    bool type_case_o() throw()
    {
        // If the alternate flag is set, we force a leading 0:
        if (has_flag(FL_ALTERNATE))
            set_flag(FL_FORCEOCTAL);

        return type_case_integer<8>();
    }

    bool type_case_X() throw()
    {
        return type_case_integer<16>(true);
    }

    bool type_case_x() throw()
    {
        return type_case_integer<16>();
    }

    // The 'p' format specifier writes a pointer, which is simply treated as a
    // hexadecimal integer with lowercase hexits.
    bool type_case_p() throw()
    {
        // We force the precision to be 2 * sizeof(void*), which is the number
        // of hexits required to represent the pointer, so that it is zero-
        // padded.
        _precision = 2 * sizeof(void*);

        // Ensure that we read a 32-bit integer on 32-bit architectures, and
        // a 64-bit integer on 64-bit platforms:
        _length = sizeof(void*) == 4
            ? length_modifier::I32
            : length_modifier::I64;

        return type_case_integer<16>(true);
    }

    // This is the first half of the common integer formatting routine.  It
    // extracts the integer of the specified type from the varargs and does
    // pre-processing common to all integer processing.
    template <unsigned Radix>
    bool type_case_integer(bool const capital_hexits = false) throw()
    {
        size_t const integer_size = to_integer_size(_length);

        // First, extract the argument of the required type from the varargs:
        __int64 original_number  {};
        bool    extraction_result{};
        switch (integer_size)
        {
        case sizeof(int8_t):
            extraction_result = has_flag(FL_SIGNED)
                ? this->template extract_argument_from_va_list<int8_t >(original_number)
                : this->template extract_argument_from_va_list<uint8_t>(original_number);
            break;
        case sizeof(int16_t):
            extraction_result = has_flag(FL_SIGNED)
                ? this->template extract_argument_from_va_list<int16_t >(original_number)
                : this->template extract_argument_from_va_list<uint16_t>(original_number);
            break;
        case sizeof(int32_t):
            extraction_result = has_flag(FL_SIGNED)
                ? this->template extract_argument_from_va_list<int32_t >(original_number)
                : this->template extract_argument_from_va_list<uint32_t>(original_number);
            break;
        case sizeof(int64_t):
            extraction_result = has_flag(FL_SIGNED)
                ? this->template extract_argument_from_va_list<int64_t >(original_number)
                : this->template extract_argument_from_va_list<uint64_t>(original_number);
            break;
        default:
            _UCRT_VALIDATE_RETURN(_ptd, ("Invalid integer length modifier", 0), EINVAL, false);
            break;
        }

        if (!extraction_result)
            return false;

        // If we're not formatting, then we're done; we just needed to read the
        // argument from the varargs.
        if (!should_format())
            return true;

        // Check the sign of the number.  If it is negative, convert it to
        // positive for formatting.  We'll handle the minus sign later (after
        // we return from this function).
        unsigned __int64 number{};

        if (has_flag(FL_SIGNED) && original_number < 0)
        {
            number = static_cast<unsigned __int64>(-original_number);
            set_flag(FL_NEGATIVE);
        }
        else
        {
            number = static_cast<unsigned __int64>(original_number);
        }

        // Check the precision to see if the default precision was specified.  If
        // a non-default precision was specified, we turn off the zero flag, per
        // the C Standard Library specification.
        if (_precision < 0)
        {
            _precision = 1; // Default precision
        }
        else
        {
            unset_flag(FL_LEADZERO);
            _buffer.template ensure_buffer_is_big_enough<Character>(_precision, _ptd);
        }

        // If the number is zero, we do not want to print the hex prefix ("0x"),
        // even if it was requested:
        if (number == 0)
        {
            unset_flag(FL_ALTERNATE);
        }

        _string_is_wide = sizeof(Character) == sizeof(wchar_t);

        if (integer_size == sizeof(int64_t))
        {
            type_case_integer_parse_into_buffer<uint64_t, Radix>(number, capital_hexits);
        }
        else
        {
            type_case_integer_parse_into_buffer<uint32_t, Radix>(static_cast<uint32_t>(number), capital_hexits);
        }

        // If the FORCEOCTAL flag is set, then we output a leading zero, unless
        // the formatted string already has a leading zero:
        if (has_flag(FL_FORCEOCTAL) && (_string_length == 0 || tchar_string()[0] != '0'))
        {
            *--tchar_string() = '0';
            ++_string_length;
        }

        return true;
    }

    // This is the second half of the common integer formatting routine.  It
    // handles the actual formatting of the number.  This logic has been split
    // out from the first part so that we only use 64-bit arithmetic when
    // absolutely required (on x86, 64-bit division is Slow-with-a-capital-S).
    template <typename UnsignedInteger, unsigned Radix>
    void type_case_integer_parse_into_buffer(
        UnsignedInteger       number,
        bool            const capital_hexits
        ) throw()
    {
        // Format the number into the formatting buffer.  Note that we format the
        // buffer at the end of the formatting buffer, which allows us to perform
        // the formatting from least to greatest magnitude, which maps well to
        // the math.
        Character* const last_digit{_buffer.template data<Character>() + _buffer.template count<Character>() - 1};

        Character*& string_pointer = tchar_string();

        string_pointer = last_digit;
        while (_precision > 0 || number != 0)
        {
            --_precision;

            Character digit{static_cast<Character>(number % Radix + '0')};
            number /= Radix;

            // If the digit is greater than 9, we need to convert it to the
            // corresponding letter hexit in the required case:
            if (digit > '9')
            {
                digit = adjust_hexit(digit, capital_hexits);
            }

            *string_pointer-- = static_cast<char>(digit);
        }

        _string_length = static_cast<int>(last_digit - string_pointer);
        ++string_pointer;
    }

    // The 'n' type specifier is special:  We read a short* or int* from the
    // varargs and write the number of characters that we've written so far to
    // the pointed-to integer.
    bool type_case_n() throw()
    {
        void* p{nullptr};
        if (!this->template extract_argument_from_va_list<void*>(p))
            return false;

        if (!should_format())
            return true;

        if (!_get_printf_count_output())
        {
            _UCRT_VALIDATE_RETURN(_ptd, ("'n' format specifier disabled", 0), EINVAL, false);
            return false; // Unreachable
        }

        switch (to_integer_size(_length))
        {
        case sizeof(int8_t):  *static_cast<int8_t *>(p) = static_cast<int8_t >(_characters_written);   break;
        case sizeof(int16_t): *static_cast<int16_t*>(p) = static_cast<int16_t>(_characters_written);   break;
        case sizeof(int32_t): *static_cast<int32_t*>(p) = static_cast<int32_t>(_characters_written);   break;
        case sizeof(int64_t): *static_cast<int64_t*>(p) = static_cast<int64_t>(_characters_written);   break;
        default:              _UCRT_VALIDATE_RETURN(_ptd, ("Invalid integer length modifier", 0), EINVAL, false); break;
        }

        // This format specifier never corresponds to an output operation:
        _suppress_output = true;
        return true;
    }

    // After we have completed the formatting of the string to be output, we
    // perform the output operation, which is handled by these two functions.
    __forceinline bool write_stored_string_tchar(char) throw()
    {
        if (!_string_is_wide || _string_length <= 0)
        {
            _output_adapter.write_string(_narrow_string, _string_length, &_characters_written, _ptd);
        }
        else
        {
            wchar_t* p{_wide_string};
            for (int i{0}; i != _string_length; ++i)
            {
                char local_buffer[MB_LEN_MAX + 1];

                int mbc_length{0};
                errno_t const status{_wctomb_internal(&mbc_length, local_buffer, _countof(local_buffer), *p++, _ptd)};
                if (status != 0 || mbc_length == 0)
                {
                    _characters_written = -1;
                    return true;
                }

                _output_adapter.write_string(local_buffer, mbc_length, &_characters_written, _ptd);
            }
        }

        return true;
    }

    __forceinline bool write_stored_string_tchar(wchar_t) throw()
    {
        if (_string_is_wide || _string_length <= 0)
        {
            _output_adapter.write_string(_wide_string, _string_length, &_characters_written, _ptd);
        }
        else
        {
            _locale_t locale_ptr = _ptd.get_locale();
            char* p{_narrow_string};
            for (int i{0}; i != _string_length; ++i)
            {
                wchar_t wide_character{};
                int mbc_length{_mbtowc_internal(&wide_character, p, locale_ptr->locinfo->_public._locale_mb_cur_max, _ptd)};

                if (mbc_length <= 0)
                {
                    _characters_written = -1;
                    return true;
                }

                _output_adapter.write_character(wide_character, &_characters_written, _ptd);
                p += mbc_length;
            }
        }

        return true;
    }

    // These functions are utilities for working with the flags, and performing
    // state transitions.
    bool has_flag  (unsigned const f) const throw() { return (_flags & f) != 0; }
    void set_flag  (unsigned const f)       throw() { _flags |= f;              }
    void unset_flag(unsigned const f)       throw() { _flags &= ~f;             }

    state find_next_state(Character const c, state const previous_state) const throw()
    {
        auto const& lookup_table = state_transition_table();

        unsigned const current_class = static_cast<unsigned>((c < ' ' || c > 'z')
            ? character_type::other
            : static_cast<character_type>(lookup_table[c - ' '].current_class));

        auto const index = current_class * state_count() + static_cast<unsigned>(previous_state);
        return static_cast<state>(lookup_table[index].next_state);
    }

    // Adjusts an out-of-range hexit character to be a lowercase or capital
    // letter.  This function is called when formatting a number as hexadecimal
    // and a hexit is above 9.  The normal formatting will not correctly handle
    // this case.  Here, we adjust it by the required offset to yield a letter,
    // in the range [A, F], as either a lowercase or capital letter.
    static char __cdecl adjust_hexit(int const value, bool const capitalize) throw()
    {
        int const base  {capitalize ? 'A' : 'a'};
        int const offset{base - '9' - 1        };

        return static_cast<char>(offset + value);
    }

    // When a null pointer is passed, we print this string as a placeholder.
    static char   * __cdecl narrow_null_string() throw() { return  "(null)"; }
    static wchar_t* __cdecl wide_null_string  () throw() { return L"(null)"; }
};



} // namespace __crt_stdio_output
