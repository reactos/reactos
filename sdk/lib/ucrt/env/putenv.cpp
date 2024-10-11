//
// putenv.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the putenv() family of functions, which add, replace, or remove an
// environment variable from the current process environment.
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <stdlib.h>
#include <string.h>



// These functions test to see if the other environment exists
static bool other_environment_exists(wchar_t) throw() { return _environ_table.value() != nullptr; }
static bool other_environment_exists(char)    throw() { return _wenviron_table.value() != nullptr; }



// This function computes the required size of the buffer that will store a
// "name=value" string in the other environment.
template <typename Character>
static size_t compute_required_transform_buffer_count(
    Character const* const name,
    Character const* const value
    ) throw()
{
    // Compute the amount of space required for the transformation:
    size_t const name_count_required = __crt_compute_required_transform_buffer_count(CP_ACP, name);
    _VALIDATE_RETURN_NOEXC(name_count_required != 0, EILSEQ, false);

    if (!value)
        return name_count_required;

    size_t const value_count_required = __crt_compute_required_transform_buffer_count(CP_ACP, value);
    _VALIDATE_RETURN_NOEXC(value_count_required != 0, EILSEQ, false);

    // Note that each count includes space a the null terminator.  Since we'll
    // only be storing one terminator in the buffer, the space for the other
    // terminator will be used to store the '=' between the name and the value.
    return name_count_required + value_count_required;
}



// Constructs an environment string of the form "name=value" from a {name, value}
// pair.  Returns a pointer to the resulting string.  The string is dynamically
// allocated and must be freed by the caller (via the CRT free).
template <typename Character>
static Character* create_environment_string(
    Character const* const name,
    Character const* const value
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    if (value)
    {
        size_t const name_length  = traits::tcsnlen(name,  _MAX_ENV);
        size_t const value_length = traits::tcsnlen(value, _MAX_ENV);

        _VALIDATE_RETURN(name_length  < _MAX_ENV, EINVAL, nullptr);
        _VALIDATE_RETURN(value_length < _MAX_ENV, EINVAL, nullptr);

        // We add two to the length:  one for the '=' and one for the terminator
        size_t const buffer_count = name_length + 1 + value_length + 1;

        __crt_unique_heap_ptr<Character> buffer(_calloc_crt_t(Character, buffer_count));
        if (!buffer)
            return nullptr;

        traits::tcscpy_s(buffer.get(), buffer_count, name);
        buffer.get()[name_length] = '=';
        traits::tcscpy_s(buffer.get() + name_length + 1, value_length + 1, value);

        return buffer.detach();
    }
    else
    {
        Character const* const equal_sign_it = traits::tcschr(name, '=');
        if (equal_sign_it)
        {
            // Validate the length of both the name and the value:
            _VALIDATE_RETURN(equal_sign_it - name < _MAX_ENV,                         EINVAL, nullptr);
            _VALIDATE_RETURN(traits::tcsnlen(equal_sign_it + 1, _MAX_ENV) < _MAX_ENV, EINVAL, nullptr);
        }

        size_t const buffer_count = traits::tcslen(name) + 1;

        __crt_unique_heap_ptr<Character> buffer(_calloc_crt_t(Character, buffer_count));
        if (!buffer)
            return nullptr;

        traits::tcscpy_s(buffer.get(), buffer_count, name);

        return buffer.detach();
    }
}



// Converts the {name, value} pair to the other kind of string (wchar_t => char,
// char => wchar_t) and updates the other environment.
template <typename Character>
static bool __cdecl set_variable_in_other_environment(
    Character const* const name,
    Character const* const value
    ) throw()
{
    typedef __crt_char_traits<Character>       traits;
    typedef typename traits::other_char_type   other_char_type;
    typedef __crt_char_traits<other_char_type> other_traits;

    size_t const buffer_count = compute_required_transform_buffer_count(name, value);

    __crt_unique_heap_ptr<other_char_type> buffer(_calloc_crt_t(other_char_type, buffer_count));
    if (!buffer)
        return false;

    size_t const name_written_count = __crt_transform_string(CP_ACP, name, buffer.get(), buffer_count);
    _VALIDATE_RETURN_NOEXC(name_written_count != 0, EILSEQ, false);

    if (value)
    {
        // Overwrite the null terminator with an '=':
        buffer.get()[name_written_count - 1] = '=';

        size_t const value_written_count = __crt_transform_string(
            CP_ACP,
            value,
            buffer.get() + name_written_count,
            buffer_count - name_written_count);
        _VALIDATE_RETURN_NOEXC(value_written_count != 0, EILSEQ, false);
    }

    return other_traits::set_variable_in_environment_nolock(buffer.detach(), 0) == 0;
}



// Adds, replaces, or removes a variable in the current environment.  For the
// functions that take a {name, value} pair, the name is the name of the variable
// and the value is the value it is to be given.  For the functions that just
// take and option, the option is of the form "name=value".
//
// If the value is an empty string and the name names an existing environment
// variable, that variable is removed from the environment.  If the value is
// a nonempty string and the name names an existing environment variable, the
// variable is updated to have the new value.  If the value is a nonempty string
// and the name does not name an existing environment variable, a new variable
// is added to the environment.
//
// If the required environment does not yet exist, it is created from the other
// environment.  If both environments exist, the modifications are made to both
// of them so that they are kept in sync.
//
// Returns 0 on success; -1 on failure.
template <typename Character>
static int __cdecl common_putenv_nolock(
    Character const* const name,
    Character const* const value
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    // Ensure that the environment is initialized:
    if (!_environ_table.value() && !_wenviron_table.value())
        return -1;

    // At startup, we obtain the "native" flavor of environment strings from the
    // operating system.  So, a "main" program has _environ set and a "wmain"
    // program has _wenviron set.  Only when the user gets or puts the "other"
    // flavor do we convert it.
    _VALIDATE_RETURN(name != nullptr, EINVAL, -1);

    __crt_unique_heap_ptr<Character> new_option(create_environment_string(name, value));
    if (!new_option)
        return -1;

    if (traits::set_variable_in_environment_nolock(new_option.detach(), 1) != 0)
        return -1;

    // See if the "other" environment type exists; if it doesn't, we're done.
    // Otherwise, put the new option into the other environment as well.
    if (!other_environment_exists(Character()))
        return 0;

    if (!set_variable_in_other_environment(name, value))
        return -1;

    return 0;
}



template <typename Character>
static int __cdecl common_putenv(
    Character const* const name,
    Character const* const value
    ) throw()
{
    int status = 0;

    __acrt_lock(__acrt_environment_lock);
    __try
    {
        status = common_putenv_nolock(name, value);
    }
    __finally
    {
        __acrt_unlock(__acrt_environment_lock);
    }
    __endtry

    return status;
}



extern "C" int __cdecl _putenv(char const* const option)
{
    return common_putenv(option, static_cast<char const*>(nullptr));
}

extern "C" int __cdecl _wputenv(wchar_t const* const option)
{
    return common_putenv(option, static_cast<wchar_t const*>(nullptr));
}



extern "C" errno_t __cdecl _putenv_s(char const* const name, char const* const value)
{
    _VALIDATE_RETURN_ERRCODE(value != nullptr, EINVAL);
    return common_putenv(name, value) == 0 ? 0 : errno;
}

extern "C" errno_t __cdecl _wputenv_s(wchar_t const* const name, wchar_t const* const value)
{
    _VALIDATE_RETURN_ERRCODE(value != nullptr, EINVAL);
    return common_putenv(name, value) == 0 ? 0 : errno;
}
