/*
 * PROJECT:     GCC c++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     __throw_out_of_range_fmt implementation
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdexcept>
#include <cstdarg>
#include <cstring>
#include <malloc.h>

namespace std {

class out_of_range_error : public exception
{
    char* m_msg;
public:
    explicit out_of_range_error(const char* msg) noexcept
    {
        size_t len = strlen(msg) + 1;
        m_msg = (char*)malloc(len);
        strcpy(m_msg, msg);
    }
    virtual ~out_of_range_error() { free(m_msg); };
    virtual const char* what() const noexcept { return m_msg; }
};

void __throw_out_of_range_fmt(const char *format, ...)
{
    char buffer[1024];
    va_list argptr;

    va_start(argptr, format);
    _vsnprintf(buffer, sizeof(buffer), format, argptr);
    buffer[sizeof(buffer) - 1] = 0;
    va_end(argptr);

    throw out_of_range_error(buffer);
}

}  // namespace std
