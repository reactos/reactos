/***
*fullpath.c -
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose: contains the function _fullpath which makes an absolute path out
*       of a relative path. i.e.  ..\pop\..\main.c => c:\src\main.c if the
*       current directory is c:\src\src
*
*******************************************************************************/

#include <stdio.h>
#include <direct.h>
#include <errno.h>
#include <stdlib.h>
#include <corecrt_internal_traits.h>
#include <windows.h>

/***
*_TCHAR *_fullpath( _TCHAR *buf, const _TCHAR *path, maxlen );
*
*Purpose:
*
*       _fullpath - combines the current directory with path to form
*       an absolute path. i.e. _fullpath takes care of .\ and ..\
*       in the path.
*
*       The result is placed in buf. If the length of the result
*       is greater than maxlen nullptr is returned, otherwise
*       the address of buf is returned.
*
*       If buf is nullptr then a buffer is malloc'ed and maxlen is
*       ignored. If there are no errors then the address of this
*       buffer is returned.
*
*       If path specifies a drive, the curent directory of this
*       drive is combined with path. If the drive is not valid
*       and _fullpath needs the current directory of this drive
*       then nullptr is returned.  If the current directory of this
*       non existant drive is not needed then a proper value is
*       returned.
*       For example:  path = "z:\\pop" does not need z:'s current
*       directory but path = "z:pop" does.
*
*
*
*Entry:
*       _TCHAR *buf  - pointer to a buffer maintained by the user;
*       _TCHAR *path - path to "add" to the current directory
*       int maxlen - length of the buffer pointed to by buf
*
*Exit:
*       Returns pointer to the buffer containing the absolute path
*       (same as buf if non-nullptr; otherwise, malloc is
*       used to allocate a buffer)
*
*Exceptions:
*
*******************************************************************************/

template <typename Character>
_Success_(return != 0)
static Character* __cdecl common_fullpath(
    _Out_writes_z_(max_count) Character* const user_buffer,
    Character const* const path,
    size_t           const max_count,
    int              const block_use,
    char const*      const file_name,
    int              const line_number
    ) throw()
{
    // These are referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(block_use);
    UNREFERENCED_PARAMETER(file_name);
    UNREFERENCED_PARAMETER(line_number);

    typedef __crt_char_traits<Character> traits;

    // If the path is empty, we have no work to do:
    if (path == nullptr || path[0] == '\0')
    {
#pragma warning(suppress:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036 Prefast does not understand perfect forwarding.
        return traits::tgetcwd(user_buffer, static_cast<int>(__min(max_count, INT_MAX)));
    }

    if (user_buffer != nullptr) {
        // Using user buffer. Fail if not enough space.
        __crt_no_alloc_win32_buffer<Character> buffer(user_buffer, max_count);
        if (!traits::get_full_path_name(path, buffer)) {
            return user_buffer;
        } else {
            return nullptr;
        }
    } else {
        // Always new memory suitable for debug mode and releasing to the user.
        __crt_public_win32_buffer<Character> buffer(
            __crt_win32_buffer_debug_info(block_use, file_name, line_number)
            );
        traits::get_full_path_name(path, buffer);
        return buffer.detach();
    }
}



extern "C" char* __cdecl _fullpath(
    char*       const user_buffer,
    char const* const path,
    size_t      const max_count
    )
{
    return common_fullpath(user_buffer, path, max_count, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" wchar_t* __cdecl _wfullpath(
    wchar_t*       const user_buffer,
    wchar_t const* const path,
    size_t         const max_count
    )
{
    return common_fullpath(user_buffer, path, max_count, _NORMAL_BLOCK, nullptr, 0);
}

#ifdef _DEBUG

#undef _fullpath_dbg
#undef _wfullpath_dbg

extern "C" char* __cdecl _fullpath_dbg(
    char*       const user_buffer,
    char const* const path,
    size_t      const max_count,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_fullpath(user_buffer, path, max_count, block_use, file_name, line_number);
}

extern "C" wchar_t* __cdecl _wfullpath_dbg(
    wchar_t*       const user_buffer,
    wchar_t const* const path,
    size_t         const max_count,
    int            const block_use,
    char const*    const file_name,
    int            const line_number
    )
{
    return common_fullpath(user_buffer, path, max_count, block_use, file_name, line_number);
}

#endif // _DEBUG
