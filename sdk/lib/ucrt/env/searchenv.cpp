//
// searchenv.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the searchenv() family of functions, which search for a file in the
// paths contained in an environment variable (like %PATH%).
//
#include <direct.h>
#include <corecrt_internal_traits.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036 Prefast doesn't understand perfect forwarding so annotations are lost.
#pragma warning(disable:__WARNING_MISSING_ZERO_TERMINATION2) // 6054 Prefast doesn't understand perfect forwarding so annotations are lost.

// These functions search for a file in the paths in an environment variable.
// The environment variable named 'environment_variable' is retrieved from the
// environment.  It is expected to contain a semicolon-delimited sequence of
// directories, similar to %PATH%.  The file name is concatenated onto the end
// of each directory string in sequence and its existence is tested for.  When
// a file is found, the search stops and the buffer is filled with the full path
// to the file.  If the file is not found for whatever reason, a zero-length
// string is placed in the buffer and an error code is returned.
template <typename Character>
static errno_t __cdecl common_searchenv_s(
    _In_z_                       Character const* const file_name,
    _In_z_                       Character const* const environment_variable,
    _Out_writes_z_(result_count) Character*       const result_buffer,
    _In_                         size_t           const result_count
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN_ERRCODE(result_buffer != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(result_count  >  0,       EINVAL);
    if (!file_name)
    {
        result_buffer[0] = '\0';
        _VALIDATE_RETURN_ERRCODE(file_name != nullptr, EINVAL);
    }

    // Special case:  If the file name is an empty string, we'll just return an
    // empty result_buffer and set errno:
    if (file_name[0] == '\0')
    {
        result_buffer[0] = '\0';
        return errno = ENOENT;
    }

    errno_t saved_errno = errno;
    int const access_result = traits::taccess_s(file_name, 0);
    errno = saved_errno;

    // If the file name exists, convert it to a fully qualified result_buffer name:
    if (access_result == 0)
    {
        if (traits::tfullpath(result_buffer, file_name, result_count) == nullptr)
        {
            result_buffer[0] = '\0';
            return errno; // fullpath will set errno
        }

        return 0;
    }

    Character* path_string = nullptr;
    if (_ERRCHECK_EINVAL(traits::tdupenv_s_crt(&path_string, nullptr, environment_variable)) != 0 || path_string == nullptr)
    {   
        result_buffer[0] = '\0';
        return errno = ENOENT; // The environment variable doesn't exist
    }

    __crt_unique_heap_ptr<Character> const path_string_cleanup(path_string);

    size_t const file_name_length = traits::tcslen(file_name);
    
    size_t const local_path_count = _MAX_PATH + 4;
    Character local_path_buffer[local_path_count];
    
    size_t     path_count  = local_path_count;
    Character* path_buffer = local_path_buffer;
    if (file_name_length >= result_count)
    {
        // The local buffer is not large enough; dynamically allocate a new
        // buffer.  We add two to the size to account for a trailing slash
        // that we may need to add and for the null terminator:
        path_count  = traits::tcslen(path_string) + file_name_length + 2;
        path_buffer = _calloc_crt_t(Character, path_count).detach(); // We'll retake ownership below
        if (!path_buffer)
        {
            result_buffer[0] = '\0';
            return errno = ENOMEM;
        }
    }

    __crt_unique_heap_ptr<Character> path_buffer_cleanup(path_buffer == local_path_buffer
        ? nullptr
        : path_buffer);

    saved_errno = errno;
    while (path_string)
    {
        Character* const previous_path_string = path_string;
        path_string = traits::tgetpath(path_string, path_buffer, path_count - file_name_length - 1);
        if (!path_string && path_buffer == local_path_buffer && errno == ERANGE)
        {
            // If the getpath operation failed because the buffer was not large
            // enough, try allocating a larger buffer:
            size_t const required_count = traits::tcslen(previous_path_string) + file_name_length + 2;

            path_buffer_cleanup = _calloc_crt_t(Character, required_count);
            if (!path_buffer_cleanup)
            {
                result_buffer[0] = '\0';
                return errno = ENOMEM;
            }

            path_count  = required_count;
            path_buffer = path_buffer_cleanup.get();

            path_string = traits::tgetpath(previous_path_string, path_buffer, path_count - file_name_length);
        }

        if (!path_string || path_buffer[0] == '\0')
        {
            result_buffer[0] = '\0';
            return errno = ENOENT;
        }

        // The result_buffer now holds a non-empty path name from path_string
        // and we know that the buffer is large enough to hold the concatenation
        // of the path with the file name (if not, the call to getpath would
        // have failed.  So, we concatenate the path and file names:
        size_t path_length = traits::tcslen(path_buffer);
        Character* path_it = path_buffer + path_length;
        
        // Add a trailing '\' if one is required:
        Character const last_character = *(path_it - 1);
        if (last_character != '/' && last_character != '\\' && last_character != ':')
        {
            *path_it++ = '\\';
            ++path_length;
        }

        // The path_it now points to the character following the trailing '\',
        // '/', or ':'; this is where we copy the file name:
        _ERRCHECK(traits::tcscpy_s(path_it, path_count - path_length, file_name));

        // If we can't access the file at this path, it isn't a match:
        if (traits::taccess_s(path_buffer, 0) != 0)
            continue;

        // Otherwise, we can access the file, and we copy the full path into the
        // caller-provided buffer:
        if (path_length + file_name_length + 1 > result_count)
        {
            result_buffer[0] = '\0';
            return errno = ERANGE;
        }

        errno = saved_errno;

        _ERRCHECK(traits::tcscpy_s(result_buffer, result_count, path_buffer));
        return 0;
    }

    // If we get here, we must have tried every path in the environment and not
    // found the file name:
    result_buffer[0] = '\0';
    return errno = ENOENT;
}



extern "C" errno_t __cdecl _searchenv_s(
    char const* const file_name,
    char const* const environment_variable,
    char*       const result_buffer,
    size_t      const result_count
    )
{
    return common_searchenv_s(file_name, environment_variable, result_buffer, result_count);
}

extern "C" errno_t __cdecl _wsearchenv_s(
    wchar_t const* const file_name,
    wchar_t const* const environment_variable,
    wchar_t*       const result_buffer,
    size_t         const result_count
    )
{
    return common_searchenv_s(file_name, environment_variable, result_buffer, result_count);
}



extern "C" void __cdecl _searchenv(
    char const* const file_name,
    char const* const environment_variable,
    char*       const result_buffer
    )
{
    common_searchenv_s(file_name, environment_variable, result_buffer, _MAX_PATH);
}

extern "C" void __cdecl _wsearchenv(
    wchar_t const* const file_name,
    wchar_t const* const environment_variable,
    wchar_t*       const result_buffer
    )
{
    common_searchenv_s(file_name, environment_variable, result_buffer, _MAX_PATH);
}
