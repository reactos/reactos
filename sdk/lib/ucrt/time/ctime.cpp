//
// ctime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The ctime() family of functions, which convert a time_t into a string.
//
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_time.h>



// Converts a time stored as a time_t into a string of the form:
//
//     Tue May  1 14:25:03 1984
//
// Returns zero on success; returns an error code and sets errno on failure.  The
// buffer is always null terminated as long as it is at least one character in
// length.
template <typename TimeType, typename Character>
static errno_t __cdecl common_ctime_s(
    _Out_writes_z_(size_in_chars) _Post_readable_size_(26)  Character*      const   buffer,
    _In_range_(>=,26)                                       size_t          const   size_in_chars,
                                                            TimeType const* const   time_t_value
    ) throw()
{
    typedef __crt_time_traits<TimeType, Character> time_traits;

    _VALIDATE_RETURN_ERRCODE(buffer != nullptr && size_in_chars > 0, EINVAL)

    _RESET_STRING(buffer, size_in_chars);

    _VALIDATE_RETURN_ERRCODE      (size_in_chars >= 26,     EINVAL)
    _VALIDATE_RETURN_ERRCODE      (time_t_value != nullptr, EINVAL)
    _VALIDATE_RETURN_ERRCODE_NOEXC(*time_t_value >= 0,      EINVAL)

    tm tm_value;
    errno_t const status = time_traits::localtime_s(&tm_value, time_t_value);
    if (status != 0)
        return status;

    return time_traits::tasctime_s(buffer, size_in_chars, &tm_value);
}

extern "C" errno_t __cdecl _ctime32_s(
    char*             const buffer,
    size_t            const size_in_chars,
    __time32_t const* const time_t_value
    )
{
    return common_ctime_s(buffer, size_in_chars, time_t_value);
}

extern "C" errno_t __cdecl _wctime32_s(
    wchar_t*          const buffer,
    size_t            const size_in_chars,
    __time32_t const* const time_t_value
    )
{
    return common_ctime_s(buffer, size_in_chars, time_t_value);
}

extern "C" errno_t __cdecl _ctime64_s(
    char*             const buffer,
    size_t            const size_in_chars,
    __time64_t const* const time_t_value
    )
{
    return common_ctime_s(buffer, size_in_chars, time_t_value);
}

extern "C" errno_t __cdecl _wctime64_s(
    wchar_t*          const buffer,
    size_t            const size_in_chars,
    __time64_t const* const time_t_value
    )
{
    return common_ctime_s(buffer, size_in_chars, time_t_value);
}



// Converts a time stored as a time_t into a string of the form, just as the
// secure _s version (defined above) does.  Returns a pointer to a thread-local
// buffer containing the resulting time string.
template <typename TimeType, typename Character>
_Ret_writes_z_(26)
_Success_(return != 0)
static Character* __cdecl common_ctime(
    TimeType const* const time_t_value
    ) throw()
{
    typedef __crt_time_traits<TimeType, Character> time_traits;

    _VALIDATE_RETURN      (time_t_value != nullptr, EINVAL, nullptr)
    _VALIDATE_RETURN_NOEXC(*time_t_value >= 0,      EINVAL, nullptr)

    tm tm_value;
    errno_t const status = time_traits::localtime_s(&tm_value, time_t_value);
    if (status != 0)
        return nullptr;

    return time_traits::tasctime(&tm_value);
}

extern "C" char* __cdecl _ctime32(__time32_t const* const time_t_value)
{
    return common_ctime<__time32_t, char>(time_t_value);
}

extern "C" wchar_t* __cdecl _wctime32(__time32_t const* const time_t_value)
{
    return common_ctime<__time32_t, wchar_t>(time_t_value);
}

extern "C" char* __cdecl _ctime64(__time64_t const* const time_t_value)
{
    return common_ctime<__time64_t, char>(time_t_value);
}

extern "C" wchar_t* __cdecl _wctime64(__time64_t const* const time_t_value)
{
    return common_ctime<__time64_t, wchar_t>(time_t_value);
}
