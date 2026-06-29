/***
*chdir.c - change directory
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file has the _chdir() function - change current directory.
*
*******************************************************************************/

#include <direct.h>
#include <corecrt_internal_traits.h>
#include <corecrt_internal_win32_buffer.h>
#include <malloc.h>
#include <stdlib.h>

/***
*int _chdir(path) - change current directory
*
*Purpose:
*       Changes the current working directory to that given in path.
*
*Entry:
*       _TSCHAR *path - directory to change to
*
*Exit:
*       returns 0 if successful,
*       returns -1 and sets errno if unsuccessful
*
*Exceptions:
*
*******************************************************************************/
template <typename Character>
_Success_(return == 0)
static int __cdecl set_cwd_environment_variable(_In_z_ Character const* const path) throw()
{
    typedef __crt_char_traits<Character> traits;

    // If the path is a UNC name, no need to update:
    if ((path[0] == '\\' || path[0] == '/') && path[0] == path[1])
        return 0;

#pragma warning(suppress:28931) // unused assignment of variable drive_letter
    Character const drive_letter = static_cast<Character>(toupper(static_cast<char>(path[0])));
    Character const name[] = { '=', drive_letter, ':', '\0' };

    if (traits::set_environment_variable(name, path))
        return 0;

    __acrt_errno_map_os_error(GetLastError());
    return -1;
}



template <typename Character>
_Success_(return == 0)
static int __cdecl common_chdir(_In_z_ Character const* const path) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_CLEAR_OSSERR_RETURN(path != nullptr, EINVAL, -1);

    if (!traits::set_current_directory(path))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    // If the new current directory path is not a UNC path, we must update the
    // OS environment variable specifying the current full current directory,
    // build the environment variable string, and call SetEnvironmentVariable().
    // We need to do this because the SetCurrentDirectory() API does not update
    // the environment variables, and other functions (fullpath, spawn, etc.)
    // need them to be set.
    //
    // If associated with a 'drive', the current directory should have the
    // form of the example below:
    //
    //     D:\nt\private\mytests
    //
    // So that the environment variable should be of the form:
    //
    //     =D:=D:\nt\private\mytests

    Character buffer_initial_storage[MAX_PATH + 1];
    __crt_internal_win32_buffer<Character> current_directory_buffer(buffer_initial_storage);

    errno_t const err = traits::get_current_directory(current_directory_buffer);

    if (err != 0) {
        // Appropriate error already set
        return -1;
    }

    return set_cwd_environment_variable(current_directory_buffer.data());
}



extern "C" int __cdecl _chdir(char const* const path)
{
    return common_chdir(path);
}

extern "C" int __cdecl _wchdir(wchar_t const* const path)
{
    return common_chdir(path);
}
