//
// strerror.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the strerror() family of functions, which convert an errno error code
// to a string representation.  Note that there are legacy implementations of
// these functions, prefixed with underscores, in _strerr.cpp.  The functions in
// this file are implemented per the C Standard Library specification.
//
#include <corecrt_internal.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>



// These functions return the error string to be used when we are unable to return
// the actual error string.
_Ret_z_
static char* get_failure_string(char) throw()
{
    return const_cast<char*>("Visual C++ CRT: Not enough memory to complete call to strerror.");
}

_Ret_z_
static wchar_t* get_failure_string(wchar_t) throw()
{
    return const_cast<wchar_t*>(L"Visual C++ CRT: Not enough memory to complete call to strerror.");
}



// These functions return a reference to the thread-local pointer to the buffer
// to be used to store the error string.
static char*& get_ptd_buffer(__acrt_ptd* const ptd, char) throw()
{
    return ptd->_strerror_buffer;
}

static wchar_t*& get_ptd_buffer(__acrt_ptd* const ptd, wchar_t) throw()
{
    return ptd->_wcserror_buffer;
}



// These functions copy the narrow string into the provided buffer.
_Success_(return == 0)
static errno_t copy_string_into_buffer(
    _In_reads_or_z_(max_count)      char const* const   string,
    _Out_writes_z_(buffer_count)    char*       const   buffer,
                                    size_t      const   buffer_count,
                                    size_t      const   max_count
    ) throw()
{
    return strncpy_s(buffer, buffer_count, string, max_count);
}

_Success_(return == 0)
static errno_t copy_string_into_buffer(
    _In_reads_or_z_(max_count) char const* const string,
    _Out_writes_z_(buffer_count) wchar_t* const buffer,
    size_t      const buffer_count,
    size_t      const max_count
    ) throw()
{
    return mbstowcs_s(nullptr, buffer, buffer_count, string, max_count);
}



// Maps an error number to an error message string.  The error number should be
// one of the errno values.  The string is valid until the next call (on this
// thread) to one of the strerror functions.  The CRT owns the string.
template <typename Character>
_Ret_z_
static Character* __cdecl common_strerror(int const error_number)
{
    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (!ptd)
        return get_failure_string(Character());

    Character*& buffer = get_ptd_buffer(ptd, Character());
    if (!buffer)
        buffer = _calloc_crt_t(Character, strerror_buffer_count).detach();

    if (!buffer)
        return get_failure_string(Character());

    _ERRCHECK(copy_string_into_buffer(_get_sys_err_msg(error_number), buffer, strerror_buffer_count, strerror_buffer_count - 1));
    return buffer;
}

extern "C" char* __cdecl strerror(int const error_number)
{
    return common_strerror<char>(error_number);
}

extern "C" wchar_t* __cdecl _wcserror(int const error_number)
{
    return common_strerror<wchar_t>(error_number);
}



// Maps an error number to an error message string.  The error number should be
// one of the errno values.  The string is copied into the provided buffer.  On
// success or truncation, 0 is returned.  Otherwise, an error code is returned.
template <typename Character>
static errno_t __cdecl common_strerror_s(
    _Out_writes_z_(buffer_count)    Character* const    buffer,
                                    size_t     const    buffer_count,
                                    int        const    error_number
    )
{
    _VALIDATE_RETURN_ERRCODE(buffer != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(buffer_count > 0,  EINVAL);

    errno_t const result = _ERRCHECK_EINVAL_ERANGE(copy_string_into_buffer(
        _get_sys_err_msg(error_number),
        buffer,
        buffer_count,
        _TRUNCATE));

    return result == STRUNCATE ? 0 : result;
}

extern "C" errno_t __cdecl strerror_s(
    char*  const buffer,
    size_t const buffer_count,
    int    const error_number
    )
{
    return common_strerror_s(buffer, buffer_count, error_number);
}

extern "C" errno_t __cdecl _wcserror_s(
    wchar_t* const buffer,
    size_t   const buffer_count,
    int      const error_number
    )
{
    return common_strerror_s(buffer, buffer_count, error_number);
}
