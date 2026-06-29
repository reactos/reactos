//
// spawnvp.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the -vp and -vpe flavors of the _exec() and _spawn() functions.  See
// the comments in spawnv.cpp for details of the various flavors of these
// functions.
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>
#include <mbstring.h>



template <typename Character>
static intptr_t __cdecl common_spawnvp(
    int                     const mode,
    Character const*        const file_name,
    Character const* const* const arguments,
    Character const* const* const environment
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(file_name       != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(file_name[0]    != '\0',    EINVAL, -1);
    _VALIDATE_RETURN(arguments       != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments[0]    != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments[0][0] != '\0',    EINVAL, -1);

    __crt_errno_guard const guard_errno;

    // Attempt to perform the spawn with the given file name.  If this succeeds
    // and is a spawn operation (mode != _P_OVERLAY), then it returns a status
    // code other than -1 and we can just return it.  If this succeds and is an
    // exec operation (mode == _P_OVERLAY), then this call does not return.
    intptr_t const initial_result = traits::tspawnve(mode, file_name, arguments, environment);
    if (initial_result != -1)
        return initial_result;

    // If the spawn attempt failed, try to determine why.  ENOENT indicates that
    // the spawn operation itself failed, and there's nothing we can do:
    if (errno != ENOENT)
        return -1;

    // If the file name is an absolute or relative path, then we do not search
    // the %PATH%, so there is nothing left to do:
    if (traits::tcschr(file_name, '\\') != nullptr)
        return -1;
    
    if (traits::tcschr(file_name, '/') != nullptr)
        return -1;

    if (file_name[1] == ':')
        return -1;

    // Otherwise, the file name string just names an executable.  Let's try
    // appending it to each path in the %PATH% and see if we can find the
    // file to execute:
    Character const path_name[] = { 'P', 'A', 'T', 'H', '\0' };
    __crt_unique_heap_ptr<Character> path_value;
    if (_ERRCHECK_EINVAL(traits::tdupenv_s_crt(path_value.get_address_of(), nullptr, path_name)) != 0)
        return -1;

    if (!path_value)
        return -1;

    // This will be used to store alternative path names to the executable:
    __crt_unique_heap_ptr<Character> const owned_file_buffer(_calloc_crt_t(Character, _MAX_PATH));
    if (!owned_file_buffer)
        return -1;

    Character* file_buffer = owned_file_buffer.get();
    Character* path_state  = path_value.get();
    while ((path_state = traits::tgetpath(path_state, file_buffer, _MAX_PATH - 1)) != 0)
    {
        if (!*file_buffer)
            break;

        // Append a '\' if necessary:
        Character* const last_character_it = file_buffer + traits::tcslen(file_buffer) - 1;
        if (last_character_it != traits::tcsrchr(file_buffer, '\\') &&
            last_character_it != traits::tcsrchr(file_buffer, '/'))
        {
            Character const backslash_string[] =  { '\\', '\0' };
            _ERRCHECK(traits::tcscat_s(file_buffer, _MAX_PATH, backslash_string));
        }

        // Make sure that the buffer is sufficiently large to hold the file name
        // concatenated onto the end of this %PATH% component.  If it is not, we
        // return immediately (errno will still be set from the last call to
        // _spawnve().
        if (traits::tcslen(file_buffer) + traits::tcslen(file_name) >= _MAX_PATH)
            break;

        _ERRCHECK(traits::tcscat_s(file_buffer, _MAX_PATH, file_name));

        // Try the spawn again with the newly constructed path.  Like before, if
        // this succeeds and is a spawn operation, then a status is returned and
        // we can return immediately.  If this succeeds and is an exec operation,
        // the call does not return.
        errno = 0;
        intptr_t const result = traits::tspawnve(mode, file_buffer, arguments, environment);
        if (result != -1)
            return result;

        // Two classes of failures are acceptable here:  either the operation
        // failed because the spawn operation itself failed (e.g., if the file
        // could not be found)...
        if (errno == ENOENT || _doserrno == ERROR_NOT_READY)
            continue;

        // ...or the path is a UNC path...
        bool const is_unc_path_with_slashes =
            traits::tcschr(file_buffer,     '/') == file_buffer &&
            traits::tcschr(file_buffer + 1, '/') == file_buffer + 1;

        bool const is_unc_path_with_backslashes =
            traits::tcschr(file_buffer,     '\\') == file_buffer &&
            traits::tcschr(file_buffer + 1, '\\') == file_buffer + 1;

        if (is_unc_path_with_slashes || is_unc_path_with_backslashes)
            continue;

        // ...otherwise, we report the error back to the caller:
        break;
    }

    return -1;
}



extern "C" intptr_t __cdecl _execvp(
    char const*        const file_name,
    char const* const* const arguments
    )
{
    return common_spawnvp(_P_OVERLAY, file_name, arguments, static_cast<char const* const* const>(nullptr));
}

extern "C" intptr_t __cdecl _execvpe(
    char const*        const file_name,
    char const* const* const arguments,
    char const* const* const environment
    )
{
    return common_spawnvp(_P_OVERLAY, file_name, arguments, environment);
}

extern "C" intptr_t __cdecl _spawnvp(
    int                const mode,
    char const*        const file_name,
    char const* const* const arguments
    )
{
    return common_spawnvp(mode, file_name, arguments, static_cast<char const* const* const>(nullptr));
}

extern "C" intptr_t __cdecl _spawnvpe(
    int                const mode,
    char const*        const file_name,
    char const* const* const arguments,
    char const* const* const environment
    )
{
    return common_spawnvp(mode, file_name, arguments, environment);
}



extern "C" intptr_t __cdecl _wexecvp(
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments
    )
{
    return common_spawnvp(_P_OVERLAY, file_name, arguments, static_cast<wchar_t const* const* const>(nullptr));
}

extern "C" intptr_t __cdecl _wexecvpe(
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments,
    wchar_t const* const* const environment
    )
{
    return common_spawnvp(_P_OVERLAY, file_name, arguments, environment);
}

extern "C" intptr_t __cdecl _wspawnvp(
    int                   const mode,
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments
    )
{
    return common_spawnvp(mode, file_name, arguments, static_cast<wchar_t const* const* const>(nullptr));
}

extern "C" intptr_t __cdecl _wspawnvpe(
    int                   const mode,
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments,
    wchar_t const* const* const environment
    )
{
    return common_spawnvp(mode, file_name, arguments, environment);
}
