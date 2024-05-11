//
// c32rtomb.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <stdint.h>
#include <uchar.h>

using namespace __crt_mbstring;

extern "C" size_t __cdecl c32rtomb(char* s, char32_t c32, mbstate_t* ps)
{
    // TODO: Bug 13307590 says this is always assuming UTF-8.
    __crt_cached_ptd_host ptd;
    return __c32rtomb_utf8(s, c32, ps, ptd);
}

size_t __cdecl __crt_mbstring::__c32rtomb_utf8(char* s, char32_t c32, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    if (!s)
    {
        // Equivalent to c32rtomb(buf, U'\0', ps) for some internal buffer buf
        *ps = {};
        return 1;
    }

    if (c32 == U'\0')
    {
        *s = '\0';
        *ps = {};
        return 1;
    }

    // Fast path for ASCII
    if ((c32 & ~0x7f) == 0)
    {
        *s = static_cast<char>(c32);
        return 1;
    }

    // Figure out how many trail bytes we need
    size_t trail_bytes;
    uint8_t lead_byte;
    if ((c32 & ~0x7ff) == 0)
    {
        trail_bytes = 1;
        lead_byte = 0xc0;
    }
    else if ((c32 & ~0xffff) == 0)
    {
        // high/low surrogates are only valid in UTF-16 encoded data
        if (0xd800 <= c32 && c32 <= 0xdfff)
        {
            return return_illegal_sequence(ps, ptd);
        }
        trail_bytes = 2;
        lead_byte = 0xe0;
    }
    else if ((c32 & ~0x001fffff) == 0)
    {
        // Unicode's max code point is 0x10ffff
        if (0x10ffff < c32)
        {
            return return_illegal_sequence(ps, ptd);
        }
        trail_bytes = 3;
        lead_byte = 0xf0;
    }
    else
    {
        return return_illegal_sequence(ps, ptd);
    }
    _ASSERTE(1 <= trail_bytes && trail_bytes <= 3);

    // Put six bits into each of the trail bytes
    // Lowest bits are in the last UTF-8 byte.
    // Filling back to front.
    for (size_t i = trail_bytes; i > 0; --i)
    {
        s[i] = (c32 & 0x3f) | 0x80;
        c32 >>= 6;
    }

    // The first byte needs the upper (trail_bytes + 1) bits to store the length
    // And the lower (7 - trail_bytes) to store the upper bits of the code point
    _ASSERTE(c32 < (1u << (7 - trail_bytes)));
    s[0] = static_cast<uint8_t>(c32) | lead_byte;

    return reset_and_return(trail_bytes + 1, ps);
}
