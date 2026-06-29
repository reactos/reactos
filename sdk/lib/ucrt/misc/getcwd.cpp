/***
*getcwd.cpp - get current working directory
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*       contains functions _getcwd, _getdcwd and _getcdrv for getting the
*       current working directory.  getcwd gets the c.w.d. for the default disk
*       drive, whereas _getdcwd allows one to get the c.w.d. for whatever disk
*       drive is specified. _getcdrv gets the current drive.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <direct.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>



// Tests whether the specified drive number is valid.  Returns zero if the drive
// does not exist; returns one if the drive exists.  Pass zero as an argument to
// check the default drive.
static int __cdecl is_valid_drive(unsigned const drive_number) throw()
{
    if (drive_number > 26)
    {
        _doserrno = ERROR_INVALID_DRIVE;
        _VALIDATE_RETURN(("Invalid Drive Index" ,0), EACCES, 0);
    }

    if (drive_number == 0)
        return 1;

#pragma warning(suppress:__WARNING_UNUSED_ASSIGNMENT) // 28931
    wchar_t const drive_letter   = static_cast<wchar_t>(L'A' + drive_number - 1);
    wchar_t const drive_string[] = { drive_letter, L':', L'\\', L'\0' };

    UINT const drive_type = GetDriveTypeW(drive_string);
    if (drive_type == DRIVE_UNKNOWN || drive_type == DRIVE_NO_ROOT_DIR)
        return 0;

    return 1;
}



/***
*_TSCHAR *_getcwd(pnbuf, maxlen) - get current working directory of default drive
*
*Purpose:
*       _getcwd gets the current working directory for the user,
*       placing it in the buffer pointed to by pnbuf. If the length
*       of the string exceeds the length of the buffer, maxlen,
*       then nullptr is returned. If pnbuf = nullptr, a buffer of at
*       least size maxlen is automatically allocated using
*       malloc() -- a pointer to which is returned by _getcwd().
*       An entry point "_getdcwd()" is defined which takes the above
*       parameters, plus a drive number.  "_getcwd()" is implemented
*       as a call to "_getdcwd()" with the default drive (0).
*
*       side effects: no global data is used or affected
*
*Entry:
*       _TSCHAR *pnbuf = pointer to a buffer maintained by the user;
*       int maxlen = length of the buffer pointed to by pnbuf;
*
*Exit:
*       Returns pointer to the buffer containing the c.w.d. name
*       (same as pnbuf if non-nullptr; otherwise, malloc is
*       used to allocate a buffer)
*
*Exceptions:
*
*******************************************************************************/

/***
*_TSCHAR *_getdcwd(drive, pnbuf, maxlen) - get c.w.d. for given drive
*
*Purpose:
*       _getdcwd gets the current working directory for the user,
*       placing it in the buffer pointed to by pnbuf. If the length
*       of the string exceeds the length of the buffer, maxlen,
*       then nullptr is returned. If pnbuf = nullptr, a buffer of at
*       least size maxlen is automatically allocated using
*       malloc() -- a pointer to which is returned by _getdcwd().
*
*       side effects: no global data is used or affected
*
*Entry:
*       int drive   - number of the drive being inquired about
*                     0 = default, 1 = 'a:', 2 = 'b:', etc.
*       _TSCHAR *pnbuf - pointer to a buffer maintained by the user;
*       int maxlen  - length of the buffer pointed to by pnbuf;
*
*Exit:
*       Returns pointer to the buffer containing the c.w.d. name
*       (same as pnbuf if non-nullptr; otherwise, malloc is
*       used to allocate a buffer)
*
*Exceptions:
*
*******************************************************************************/

template <typename Character>
_Success_(return != 0)
_Ret_z_
static Character* __cdecl common_getdcwd(
    int                                            drive_number,
    _Out_writes_opt_z_(max_count) Character*       user_buffer,
    int                                      const max_count,
    int                                      const block_use,
    _In_opt_z_ char const*                   const file_name,
    int                                      const line_number
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(max_count >= 0, EINVAL, nullptr);

    if (drive_number != 0)
    {
        // A drive other than the default drive was requested; make sure it
        // is a valid drive number:
        if (!is_valid_drive(drive_number))
        {
            _doserrno = ERROR_INVALID_DRIVE;
            _VALIDATE_RETURN(("Invalid Drive", 0), EACCES, nullptr);
        }
    }
    else
    {
        // Otherwise, get the drive number of the default drive:
        drive_number = _getdrive();
    }

    Character drive_string[4];
    if (drive_number != 0)
    {
        drive_string[0] = static_cast<Character>('A' - 1 + drive_number);
        drive_string[1] = ':';
        drive_string[2] = '.';
        drive_string[3] = '\0';
    }
    else
    {
        drive_string[0] = '.';
        drive_string[1] = '\0';
    }

    if (user_buffer == nullptr)
    {   // Always new memory suitable for debug mode and releasing to the user.
        __crt_public_win32_buffer<Character> buffer(
            __crt_win32_buffer_debug_info(block_use, file_name, line_number)
        );
        buffer.allocate(max_count);
        if (!traits::get_full_path_name(drive_string, buffer))
        {
            return buffer.detach();
        }
        return nullptr;
    }

    // Using user buffer. Fail if not enough space.
    _VALIDATE_RETURN(max_count > 0, EINVAL, nullptr);
    user_buffer[0] = '\0';

    __crt_no_alloc_win32_buffer<Character> buffer(user_buffer, max_count);
    if (!traits::get_full_path_name(drive_string, buffer))
    {
        return user_buffer;
    }
    return nullptr;
};



extern "C" char* __cdecl _getcwd(
    char* const user_buffer,
    int   const max_length
    )
{
    return common_getdcwd(0, user_buffer, max_length, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" wchar_t* __cdecl _wgetcwd(
    wchar_t* const user_buffer,
    int      const max_length
    )
{
    return common_getdcwd(0, user_buffer, max_length, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" char* __cdecl _getdcwd(
    int   const drive_number,
    char* const user_buffer,
    int   const max_length
    )
{
    return common_getdcwd(drive_number, user_buffer, max_length, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" wchar_t* __cdecl _wgetdcwd(
    int      const drive_number,
    wchar_t* const user_buffer,
    int      const max_length
    )
{
    return common_getdcwd(drive_number, user_buffer, max_length, _NORMAL_BLOCK, nullptr, 0);
}

#ifdef _DEBUG

#undef _getcwd_dbg
#undef _getdcwd_dbg

extern "C" char* __cdecl _getcwd_dbg(
    char*       const user_buffer,
    int         const max_length,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_getdcwd(0, user_buffer, max_length, block_use, file_name, line_number);
}

extern "C" wchar_t* __cdecl _wgetcwd_dbg(
    wchar_t*       const user_buffer,
    int            const max_length,
    int            const block_use,
    char const*    const file_name,
    int            const line_number
    )
{
    return common_getdcwd(0, user_buffer, max_length, block_use, file_name, line_number);
}

extern "C" char* __cdecl _getdcwd_dbg(
    int         const drive_number,
    char*       const user_buffer,
    int         const max_length,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_getdcwd(drive_number, user_buffer, max_length, block_use, file_name, line_number);
}

extern "C" wchar_t* __cdecl _wgetdcwd_dbg(
    int         const drive_number,
    wchar_t*    const user_buffer,
    int         const max_length,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_getdcwd(drive_number, user_buffer, max_length, block_use, file_name, line_number);
}

#endif // _DEBUG
