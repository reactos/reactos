//
// mbrtoc16.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <errno.h>
#include <uchar.h>
#include <wchar.h>

using namespace __crt_mbstring;

namespace
{
    inline size_t begin_surrogate_state(char16_t* pc16, char32_t c32, size_t retval, mbstate_t* ps)
    {
        ps->_Wchar = c32;
        ps->_State = static_cast<decltype(ps->_State)>(-1);
        if (pc16)
        {
            *pc16 = static_cast<char16_t>((((c32 - 0x10000) & 0xfffff) >> 10) | 0xd800);
        }
        return retval;
    }

    inline size_t end_surrogate_state(char16_t* pc16, mbstate_t* ps)
    {
        if (pc16)
        {
            *pc16 = ((ps->_Wchar - 0x10000) & 0x03ff) | 0xdc00;
        }
        return reset_and_return(static_cast<size_t>(-3), ps);
    }

    inline bool is_surrogate_state(const mbstate_t* ps)
    {
        return ps->_State == static_cast<decltype(ps->_State)>(-1);
    }
}

extern "C" size_t __cdecl mbrtoc16(char16_t* pc16, const char* s, size_t n, mbstate_t* ps)
{
    // TODO: Bug 13307590 says this is always assuming UTF-8.
     __crt_cached_ptd_host ptd;
    return __mbrtoc16_utf8(pc16, s, n, ps, ptd);
}

size_t __cdecl __crt_mbstring::__mbrtoc16_utf8(char16_t* pc16, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    static mbstate_t internal_pst{};
    if (ps == nullptr)
    {
        ps = &internal_pst;
    }

    if (is_surrogate_state(ps))
    {
        return end_surrogate_state(pc16, ps);
    }

    char32_t c32;
    const size_t retval = __mbrtoc32_utf8(&c32, s, n, ps, ptd);
    if (!s || retval == INVALID || retval == INCOMPLETE)
    {
        return retval;
    }
    else if (c32 > 0x10ffff)
    {
        // Input is out of range for UTF-16
        return return_illegal_sequence(ps, ptd);
    }

    // Got a valid character
    if (c32 <= 0xffff)
    {
        if (pc16)
        {
            *pc16 = static_cast<char16_t>(c32);
        }
        return reset_and_return(retval, ps);
    }
    else
    {
        return begin_surrogate_state(pc16, c32, retval, ps);
    }
}
