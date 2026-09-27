/*
 * PROJECT:     xml2sdb
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Define mapping of all shim database types to xml
 * COPYRIGHT:   Copyright 2026 William Kent <wjk011@gmail.com>
 */

#pragma once

#include <cwchar>
#include <string>
#include <typedefs.h>

// Starting in libc++ 17.x, there is no longer a generic template for
// std::char_traits. Comments in libc++'s <__string/char_traits.h> indicate
// that if you want to use std::string with a different character type, you need
// to specialize it yourself. That is exactly what we do here.
//
// Note this header should only be used with libc++, as AFAIK other C++ standard libraries
// do not deprecate char_traits like libc++ does, and it references libc++ implementation details.

#if _LIBCPP_VERSION < 170000
#error This file is only intended to be used with libc++ 17.0.0 or later.
#endif

namespace std
{
    template<>
    struct char_traits<WCHAR> : __char_traits_base<WCHAR, wint_t, static_cast<WCHAR>(WEOF)>
    {
        [[__nodiscard__]] static int compare(const char_type* s1, const char_type* s2, size_t n) _NOEXCEPT {
            if (n == 0)
                return 0;

            for (; n; --n, ++s1, ++s2) {
                if (*s1 < *s2) return -1;
                if (*s2 < *s1) return 1;
            }

            return 0;
        }

        [[__nodiscard__]] static size_t length(const char_type* s) {
            size_t n = 0;
            while (*s != '\0') {
                n++;
                s++;
            }
            return n;
        }

        [[__nodiscard__]] static const char_type* find(const char_type* s, size_t n, const char_type& a) _NOEXCEPT {
            for (; n; --n) {
                if (*s == a) return s;
                s++;
            }

            return NULL;
        }
    };
}
