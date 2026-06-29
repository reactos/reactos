//
// asctime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The asctime() family of functions, which convert a tm struct into a string.
//
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_time.h>

#define _ASCBUFSIZE 26



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// asctime_s and _wasctime_s
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template <typename Character>
static Character* __cdecl common_asctime_s_write_value(
    _Out_writes_(2) Character* p,
    int                  const value,
    bool                 const zero_fill
    ) throw()
{
    if (value >= 10 || zero_fill)
    {
        *p++ = static_cast<Character>('0' + value / 10);
    }
    else
    {
        *p++ = ' ';
    }

    *p++ = static_cast<Character>('0' + value % 10);
    return p;
}



// Converts a time structure (tm) into an ASCII string.  The string is always
// exactly 26 characters long, in the form Tue May  1 02:34:55 1984\n\0.  The
// buffer 'size_in_chars' must be at least 26.  The string is generated into the
// buffer.  On success, zero is returned and the buffer contains the time string;
// on failure, an error code is returned and the contents of the buffer are
// indeterminate.
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_asctime_s(
    _Out_writes_z_(size_in_chars) _Post_readable_size_(_ASCBUFSIZE) Character* const buffer,
    _In_range_(>=, _ASCBUFSIZE)   size_t                                       const size_in_chars,
    tm const*                                                                  const tm_value
    ) throw()
{
    _VALIDATE_RETURN_ERRCODE(
        buffer != nullptr && size_in_chars > 0,
        EINVAL
    )

    _RESET_STRING(buffer, size_in_chars);

    _VALIDATE_RETURN_ERRCODE(size_in_chars >= _ASCBUFSIZE, EINVAL)
    _VALIDATE_RETURN_ERRCODE(tm_value != nullptr,          EINVAL)
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_year >= 0,       EINVAL)

    // Month, hour, minute, and second are zero-based
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_mon  >= 0 && tm_value->tm_mon  <= 11, EINVAL)
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_hour >= 0 && tm_value->tm_hour <= 23, EINVAL)
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_min  >= 0 && tm_value->tm_min  <= 59, EINVAL)
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_sec  >= 0 && tm_value->tm_sec  <= 60, EINVAL) // including leap second
    _VALIDATE_RETURN_ERRCODE(tm_value->tm_wday >= 0 && tm_value->tm_wday <=  6, EINVAL)

    _VALIDATE_RETURN_ERRCODE(__crt_time_is_day_valid(tm_value->tm_year, tm_value->tm_mon, tm_value->tm_mday), EINVAL)

    Character* buffer_it = buffer;

    // Copy the day name into the buffer:
    char const* const day_first = __dnames + tm_value->tm_wday * 3;
    char const* const day_last  = day_first + 3;
    for (char const* day_it = day_first; day_it != day_last; ++day_it)
        *buffer_it++ = static_cast<Character>(*day_it);

    *buffer_it++ = static_cast<Character>(' ');

    // Copy the month name into the buffer:
    char const* const month_first = __mnames + tm_value->tm_mon * 3;
    char const* const month_last  = month_first + 3;
    for (char const* month_it = month_first; month_it != month_last; ++month_it)
        *buffer_it++ = static_cast<Character>(*month_it);

    *buffer_it++ = static_cast<Character>(' ');

    // Copy the day of the month (1 - 31) into the buffer:
    buffer_it = common_asctime_s_write_value(buffer_it, tm_value->tm_mday, false);
    *buffer_it++ = static_cast<Character>(' ');

    // Copy the time into the buffer in HH:MM:SS form:
    buffer_it = common_asctime_s_write_value(buffer_it, tm_value->tm_hour, true);
    *buffer_it++ = static_cast<Character>(':');
    buffer_it = common_asctime_s_write_value(buffer_it, tm_value->tm_min, true);
    *buffer_it++ = static_cast<Character>(':');
    buffer_it = common_asctime_s_write_value(buffer_it, tm_value->tm_sec, true);
    *buffer_it++ = static_cast<Character>(' ');

    // Copy the four-digit year into the buffer:
    buffer_it = common_asctime_s_write_value(buffer_it, __crt_get_century(tm_value->tm_year), true);
    buffer_it = common_asctime_s_write_value(buffer_it, __crt_get_2digit_year(tm_value->tm_year), true);

    // And that's it...
    *buffer_it++ = static_cast<Character>('\n');
    *buffer_it++ = static_cast<Character>('\0');

    return 0;
}

extern "C" errno_t __cdecl asctime_s(
    char*     const buffer,
    size_t    const size_in_chars,
    tm const* const tm_value
    )
{
    return common_asctime_s(buffer, size_in_chars, tm_value);
}

extern "C" errno_t __cdecl _wasctime_s(
    wchar_t*  const buffer,
    size_t    const size_in_chars,
    tm const* const tm_value
    )
{
    return common_asctime_s(buffer, size_in_chars, tm_value);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// asctime and _wasctime
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Utility functions used by common_asctime to get the per-thread actime buffer.
static char** common_asctime_get_ptd_buffer(char) throw()
{
    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (ptd == nullptr)
        return nullptr;

    return &ptd->_asctime_buffer;
}

static wchar_t** common_asctime_get_ptd_buffer(wchar_t) throw()
{
    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (ptd == nullptr)
        return nullptr;

    return &ptd->_wasctime_buffer;
}

// Converts a time structure (tm) into an ASCII string.  The string is always
// exactly 26 characters long, in the form Tue May  1 02:34:55 1984\n\0.
// The return value is a pointer to a per-thread buffer containing the
// generated time string.
template <typename Character>
_Success_(return != 0)
_Ret_writes_z_(26)
static Character* __cdecl common_asctime(tm const* const tm_value) throw()
{
    static Character static_buffer[_ASCBUFSIZE];

    Character** ptd_buffer_address = common_asctime_get_ptd_buffer(Character());
    if (ptd_buffer_address != nullptr && *ptd_buffer_address == nullptr)
    {
        *ptd_buffer_address = _calloc_crt_t(Character, _ASCBUFSIZE).detach();
    }

    Character* const buffer = ptd_buffer_address != nullptr && *ptd_buffer_address != nullptr
        ? *ptd_buffer_address
        : static_buffer;

    errno_t const status = common_asctime_s(buffer, _ASCBUFSIZE, tm_value);
    if (status != 0)
        return nullptr;

    return buffer;
}

extern "C" char* __cdecl asctime(tm const* const tm_value)
{
    return common_asctime<char>(tm_value);
}

extern "C" wchar_t* __cdecl _wasctime(tm const* const tm_value)
{
    return common_asctime<wchar_t>(tm_value);
}
