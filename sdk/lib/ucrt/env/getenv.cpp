//
// getenv.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the getenv family of functions, which search the environment for a
// variable and return its value.
//
#include <corecrt_internal_traits.h>
#include <stdlib.h>
#include <string.h>



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// getenv() and _wgetenv()
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions search the environment for a variable with the given name.
// If such a variable is found, a pointer to its value is returned.  Otherwise,
// nullptr is returned.  Note that if the environment is access and manipulated
// from multiple threads, this function cannot be safely used:  the returned
// pointer may not be valid when the function returns.
template <typename Character>
static Character* __cdecl common_getenv_nolock(Character const* const name) throw()
{
    typedef __crt_char_traits<Character> traits;
    
    Character** const environment = traits::get_or_create_environment_nolock();
    if (environment == nullptr || name == nullptr)
        return nullptr;

    size_t const name_length = traits::tcslen(name);

    for (Character** current = environment; *current; ++current)
    {
        if (traits::tcslen(*current) <= name_length)
            continue;

        if (*(*current + name_length) != '=')
            continue;

        if (traits::tcsnicoll(*current, name, name_length) != 0)
            continue;

        // Internal consistency check:  The environment string should never use
        // a bigger buffer than _MAX_ENV.  See also the SetEnvironmentVariable
        // SDK function.
        _ASSERTE(traits::tcsnlen(*current + name_length + 1, _MAX_ENV) < _MAX_ENV);

        return *current + name_length + 1;
    }

    return nullptr;
}



template <typename Character>
static Character* __cdecl common_getenv(Character const* const name) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(name != nullptr,                            EINVAL, nullptr);
    _VALIDATE_RETURN(traits::tcsnlen(name, _MAX_ENV) < _MAX_ENV, EINVAL, nullptr);

    Character* result = 0;

    __acrt_lock(__acrt_environment_lock);
    __try
    {
        result = common_getenv_nolock(name);
    }
    __finally
    {
        __acrt_unlock(__acrt_environment_lock);
    }
    __endtry
    
    return result;
}

extern "C" char* __cdecl getenv(char const* const name)
{
    return common_getenv(name);
}

extern "C" wchar_t* __cdecl _wgetenv(wchar_t const* const name)
{
    return common_getenv(name);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// getenv_s() and _wgetenv_s()
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions search the environment for a variable with the given name.
// If such a variable is found, its value is copied into the provided buffer.
// The number of characters in the value (including the null terminator) is
// stored in '*required_count'.  Returns 0 on success; returns ERANGE if the
// provided buffer is too small; otherwise returns an error code on failure.
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_getenv_s_nolock(
                                    size_t*             const   required_count,
    _Out_writes_z_(buffer_count)    Character*          const   buffer,
                                    size_t              const   buffer_count,
                                    Character const*    const   name
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN_ERRCODE(required_count != nullptr, EINVAL);
    *required_count = 0;

    _VALIDATE_RETURN_ERRCODE(
        (buffer != nullptr && buffer_count >  0) ||
        (buffer == nullptr && buffer_count == 0), EINVAL);

    if (buffer)
        buffer[0] = '\0';

    Character const* const value = common_getenv_nolock(name);
    if (!value)
        return 0;

    *required_count = traits::tcslen(value) + 1;
    if (buffer_count == 0)
        return 0;

    // The buffer is too small; we return an error code and the caller can have
    // the opportunity to try again with a larger buffer:
    if (*required_count > buffer_count)
        return ERANGE;

    _ERRCHECK(traits::tcscpy_s(buffer, buffer_count, value));
    return 0;
}

template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_getenv_s(
                                    size_t*             const   required_count,
    _Out_writes_z_(buffer_count)    Character*          const   buffer,
                                    size_t              const   buffer_count,
                                    Character const*    const   name
    ) throw()
{
    errno_t status = 0;

    __acrt_lock(__acrt_environment_lock);
    __try
    {
        status = common_getenv_s_nolock(required_count, buffer, buffer_count, name);
    }
    __finally
    {
        __acrt_unlock(__acrt_environment_lock);
    }
    __endtry
    
    return status;
}

extern "C" errno_t __cdecl getenv_s(
    size_t*     const required_count,
    char*       const buffer,
    size_t      const buffer_count,
    char const* const name
    )
{
    return common_getenv_s(required_count, buffer, buffer_count, name);
}

extern "C" errno_t __cdecl _wgetenv_s(
    size_t*        const required_count,
    wchar_t*       const buffer,
    size_t         const buffer_count,
    wchar_t const* const name
    )
{
    return common_getenv_s(required_count, buffer, buffer_count, name);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// _dupenv_s() and _wdupenv_s()
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions search the environment for a variable with the given name.
// If a variable is found, a buffer is allocated using the public malloc to hold
// the value of the variable. The value is copied into the buffer and the buffer
// is returned to the caller via 'buffer_pointer' and 'buffer_count'. The caller
// is responsible for freeing the buffer.
//
// Returns zero on success; returns an error code on failure.  Note that if a
// variable with the specified name is not found, that is still considered a
// success; in this case, the 'value' is an empty string ('*buffer_count' will
// be zero and '*buffer_pointer' will be nullptr).
template <typename Character>
static errno_t __cdecl common_dupenv_s_nolock(
    _Outptr_result_buffer_maybenull_(*buffer_count) _Outptr_result_maybenull_z_ Character**         const   buffer_pointer,
    _Out_opt_                                                                   size_t*             const   buffer_count,
                                                                                Character const*    const   name,
                                                                                int                 const   block_use,
                                                                                char const*         const   file_name,
                                                                                int                 const   line_number
    ) throw()
{
    // These are referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(block_use);
    UNREFERENCED_PARAMETER(file_name);
    UNREFERENCED_PARAMETER(line_number);

    typedef __crt_char_traits<Character> traits;

     _VALIDATE_RETURN_ERRCODE(buffer_pointer != nullptr, EINVAL);
    *buffer_pointer = nullptr;

    if (buffer_count != nullptr)
        *buffer_count = 0;

    _VALIDATE_RETURN_ERRCODE(name != nullptr, EINVAL);

    Character const* const value = common_getenv_nolock(name);
    if (value == nullptr)
        return 0;

    size_t const value_count = traits::tcslen(value) + 1;

    *buffer_pointer = static_cast<Character*>(_calloc_dbg(
        value_count,
        sizeof(Character),
        block_use,
        file_name,
        line_number));
    _VALIDATE_RETURN_ERRCODE_NOEXC(*buffer_pointer != nullptr, ENOMEM);

    _ERRCHECK(traits::tcscpy_s(*buffer_pointer, value_count, value));
    if (buffer_count != nullptr)
        *buffer_count = value_count;

    return 0;
}

template <typename Character>
_Success_(return != 0)
static errno_t __cdecl common_dupenv_s(
    _Outptr_result_buffer_maybenull_(*buffer_count) _Outptr_result_maybenull_z_ Character**         const   buffer_pointer,
    _Out_opt_                                                                   size_t*             const   buffer_count,
                                                                                Character const*    const   name,
                                                                                int                 const   block_use,
                                                                                char const*         const   file_name,
                                                                                int                 const   line_number
    ) throw()
{
    errno_t status = 0;

    __acrt_lock(__acrt_environment_lock);
    __try
    {
        status = common_dupenv_s_nolock(
            buffer_pointer,
            buffer_count,
            name,
            block_use,
            file_name,
            line_number);
    }
    __finally
    {
        __acrt_unlock(__acrt_environment_lock);
    }
    __endtry
    
    return status;
}

extern "C" errno_t __cdecl _dupenv_s(
    char**      const buffer_pointer,
    size_t*     const buffer_count,
    char const* const name
    )
{
    return common_dupenv_s(buffer_pointer, buffer_count, name, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" errno_t __cdecl _wdupenv_s(
    wchar_t**      const buffer_pointer,
    size_t*        const buffer_count,
    wchar_t const* const name
    )
{
    return common_dupenv_s(buffer_pointer, buffer_count, name, _NORMAL_BLOCK, nullptr, 0);
}

#ifdef _DEBUG

#undef _dupenv_s_dbg
#undef _wdupenv_s_dbg

extern "C" errno_t __cdecl _dupenv_s_dbg(
    char**      const buffer_pointer,
    size_t*     const buffer_count,
    char const* const name,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_dupenv_s(buffer_pointer, buffer_count, name, block_use, file_name, line_number);
}

extern "C" errno_t __cdecl _wdupenv_s_dbg(
    wchar_t**      const buffer_pointer,
    size_t*        const buffer_count,
    wchar_t const* const name,
    int            const block_use,
    char const*    const file_name,
    int            const line_number
    )
{
    return common_dupenv_s(buffer_pointer, buffer_count, name, block_use, file_name, line_number);
}

#endif // _DEBUG
