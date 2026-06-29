//
// makepath.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the makepath family of functions, which compose a path string.
//
#include <corecrt_internal_securecrt.h>
#include <mbstring.h>
#include <stdlib.h>



static char const* previous_character(char const* const first, char const* const current) throw()
{
    return reinterpret_cast<char const*>(_mbsdec(
        reinterpret_cast<unsigned char const*>(first),
        reinterpret_cast<unsigned char const*>(current)));
}

static wchar_t const* previous_character(wchar_t const*, wchar_t const* const current) throw()
{
    return current - 1;
}



template <typename Character>
static errno_t __cdecl cleanup_after_error(
    _Out_writes_z_(count) Character* const buffer,
    size_t                           const count) throw()
{
    // This is referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(count);

    _RESET_STRING(buffer, count);
    _RETURN_BUFFER_TOO_SMALL(buffer, count);
    return EINVAL; // This is unreachable
}



// These functions compose a path string from its component parts.  They
// concatenate the drive, directory, file_name, and extension into the provided
// result_buffer.  For the functions that do not have a buffer count parameter,
// the buffer is assumed to be large enough to hold however many characters are
// required.
//
// The drive may or may not contain a ':'.  The directory may or may not contain
// a leading or trailing '/' or '\'.  The extension may or may not contain a
// leading '.'.
template <typename Character>
static errno_t __cdecl common_makepath_s(
    _Out_writes_z_(result_count) Character* const result_buffer,
    _In_       size_t           const result_count,
    _In_opt_z_ Character const* const drive,
    _In_opt_z_ Character const* const directory,
    _In_opt_z_ Character const* const file_name,
    _In_opt_z_ Character const* const extension
    ) throw()
{
    _VALIDATE_STRING(result_buffer, result_count);

    Character*       result_it  = result_buffer;

    // For the non-secure makepath functions, result_count is _CRT_UNBOUNDED_BUFFER_SIZE.
    // In this case, we do not want to perform arithmetic with the result_count.  Instead,
    // we set the result_end to nullptr:  result_it will never be a null pointer,
    // and when we need to perform arithmetic with result_end we can check it for
    // null first.
    Character* const result_end = result_count != _CRT_UNBOUNDED_BUFFER_SIZE
        ? result_buffer + result_count
        : nullptr;

    CRT_WARNING_DISABLE_PUSH(26015, "Silence prefast about overflow - covered by result_end for secure callers")

    // Copy the drive:
    if (drive && drive[0] != '\0')
    {
        if (result_end != nullptr && result_end - result_it < 2)
            return cleanup_after_error(result_buffer, result_count);

        *result_it++ = *drive;
        *result_it++ = ':';
    }

    // Copy the directory:
    if (directory && directory[0] != '\0')
    {
        Character const* source_it = directory;
        while (*source_it != '\0')
        {
            if ((result_end != nullptr) && (result_it >= result_end))
                return cleanup_after_error(result_buffer, result_count);

            *result_it++ = *source_it++;
        }

        // Write a trailing backslash if there isn't one:
        source_it = previous_character(directory, source_it);
        if (*source_it != '/' && *source_it != '\\')
        {
            if ((result_end != nullptr) && (result_it >= result_end))
                return cleanup_after_error(result_buffer, result_count);

            *result_it++ = '\\';
        }
    }

    // Copy the file name:
    if (file_name)
    {
        Character const* source_it = file_name;
        while (*source_it != '\0')
        {
            if ((result_end != nullptr) && (result_it >= result_end))
                return cleanup_after_error(result_buffer, result_count);

            *result_it++ = *source_it++;
        }
    }

    // Copy the extension:
    if (extension)
    {
        // Add a '.' if one is required:
        if (extension[0] != '\0' && extension[0] != '.')
        {
            if ((result_end != nullptr) && (result_it >= result_end))
                return cleanup_after_error(result_buffer, result_count);

            *result_it++ = '.';
        }

        Character const* source_it = extension;
        while (*source_it != '\0')
        {
            if ((result_end != nullptr) && (result_it >= result_end))
                return cleanup_after_error(result_buffer, result_count);

            *result_it++ = *source_it++;
        }
    }

    // Copy the null terminator:
    if ((result_end != nullptr) && (result_it >= result_end))
        return cleanup_after_error(result_buffer, result_count);

    *result_it++ = '\0';

    CRT_WARNING_POP

    _FILL_STRING(result_buffer, result_count, result_it - result_buffer);
    return 0;
}



extern "C" void __cdecl _makepath(
    char*       const result_buffer,
    char const* const drive,
    char const* const directory,
    char const* const file_name,
    char const* const extension
    )
{
    _makepath_s(result_buffer, _CRT_UNBOUNDED_BUFFER_SIZE, drive, directory, file_name, extension);
}

extern "C" void __cdecl _wmakepath(
    wchar_t*       const result_buffer,
    wchar_t const* const drive,
    wchar_t const* const directory,
    wchar_t const* const file_name,
    wchar_t const* const extension
    )
{
    _wmakepath_s(result_buffer, _CRT_UNBOUNDED_BUFFER_SIZE, drive, directory, file_name, extension);
}



extern "C" errno_t __cdecl _makepath_s(
    char*       const result_buffer,
    size_t      const result_count,
    char const* const drive,
    char const* const directory,
    char const* const file_name,
    char const* const extension
    )
{
    return common_makepath_s(result_buffer, result_count, drive, directory, file_name, extension);
}

extern "C" errno_t __cdecl _wmakepath_s(
    wchar_t*       const result_buffer,
    size_t         const result_count,
    wchar_t const* const drive,
    wchar_t const* const directory,
    wchar_t const* const file_name,
    wchar_t const* const extension
    )
{
    return common_makepath_s(result_buffer, result_count, drive, directory, file_name, extension);
}
