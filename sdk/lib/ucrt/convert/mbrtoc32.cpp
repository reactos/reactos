//
// mbrtoc32.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <errno.h>
#include <stdint.h>
#include <uchar.h>

using namespace __crt_mbstring;

extern "C" size_t __cdecl mbrtoc32(char32_t* pc32, const char* s, size_t n, mbstate_t* ps)
{
    // TODO: Bug 13307590 says this is always assuming UTF-8.
    __crt_cached_ptd_host ptd;
    return __mbrtoc32_utf8(pc32, s, n, ps, ptd);
}

size_t __cdecl __crt_mbstring::__mbrtoc32_utf8(char32_t* pc32, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    const char* begin = s;
    static mbstate_t internal_pst{};
    if (ps == nullptr)
    {
        ps = &internal_pst;
    }

    if (!s)
    {
        s = "";
        n = 1;
        pc32 = nullptr;
    }

    if (n == 0)
    {
        return INCOMPLETE;
    }

    // Retrieve the first byte from the string, or from the previous state
    uint8_t length;
    uint8_t bytes_needed;
    char32_t c32;
    const bool init_state = (ps->_State == 0);
    if (init_state)
    {
        const uint8_t first_byte = static_cast<uint8_t>(*s++);

        // Optimize for ASCII if in initial state
        if ((first_byte & 0x80) == 0)
        {
            if (pc32 != nullptr)
            {
                *pc32 = first_byte;
            }
            return first_byte != '\0' ? 1 : 0;
        }

        if ((first_byte & 0xe0) == 0xc0)
        {
            length = 2;
        }
        else if ((first_byte & 0xf0) == 0xe0)
        {
            length = 3;
        }
        else if ((first_byte & 0xf8) == 0xf0)
        {
            length = 4;
        }
        else
        {
            return return_illegal_sequence(ps, ptd);
        }
        bytes_needed = length;
        // Mask out the length bits
        c32 = first_byte & ((1 << (7 - length)) - 1);
    }
    else
    {
        c32 = ps->_Wchar;
        length = static_cast<uint8_t>(ps->_Byte);
        bytes_needed = static_cast<uint8_t>(ps->_State);

        // Make sure we don't have some sort of invalid/corrupted state.
        // Any input that left behind state would have been more than one byte long
        // and the first byte should have been processed already.
        if (length < 2 || length > 4 || bytes_needed < 1 || bytes_needed >= length)
        {
            return return_illegal_sequence(ps, ptd);
        }
    }

    // Don't read more bytes than we're allowed
    if (bytes_needed < n)
    {
        n = bytes_needed;
    }

    // We've already read the first byte.
    // All remaining bytes should be continuation bytes
    while (static_cast<size_t>(s - begin) < n)
    {
        uint8_t current_byte = static_cast<uint8_t>(*s++);
        if ((current_byte & 0xc0) != 0x80)
        {
            // Not a continuation character
            return return_illegal_sequence(ps, ptd);
        }
        c32 = (c32 << 6) | (current_byte & 0x3f);
    }

    if (n < bytes_needed)
    {
        // Store state and return incomplete
        auto bytes_remaining = static_cast<uint8_t>(bytes_needed - n);
        static_assert(sizeof(mbstate_t::_Wchar) >= sizeof(char32_t), "mbstate_t has broken mbrtoc32");
        ps->_Wchar = c32;
        ps->_Byte = length;
        ps->_State = bytes_remaining;
        return INCOMPLETE;
    }

    if ((0xd800 <= c32 && c32 <= 0xdfff) || (0x10ffff < c32))
    {
        // Invalid code point (surrogate or out of range)
        return return_illegal_sequence(ps, ptd);
    }

    constexpr char32_t min_legal[3]{ 0x80, 0x800, 0x10000 };
    if (c32 < min_legal[length - 2])
    {
        // Overlong encoding
        return return_illegal_sequence(ps, ptd);
    }

    // Success! Store results
    if (pc32 != nullptr)
    {
        *pc32 = c32;
    }

    return reset_and_return(c32 == U'\0' ? 0 : bytes_needed, ps);
}

