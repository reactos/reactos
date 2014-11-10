/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         CRT: implementation of __[w]splitpath[_s]
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <precomp.h>
#include <tchar.h>
#include <mbctype.h>

#if IS_SECAPI
#define _FAILURE -1
#define _SUCCESS 0

_Check_return_wat_
_CRTIMP_ALTERNATIVE
errno_t
__cdecl
_tsplitpath_x(
    _In_z_ const _TCHAR* path,
    _Out_writes_opt_z_(drive_size) _TCHAR* drive,
    _In_ size_t drive_size,
    _Out_writes_opt_z_(dir_size) _TCHAR* dir,
    _In_ size_t dir_size,
    _Out_writes_opt_z_(fname_size) _TCHAR* fname,
    _In_ size_t fname_size,
    _Out_writes_opt_z_(ext_size) _TCHAR* ext,
    _In_ size_t ext_size)
#else
#define _FAILURE
#define _SUCCESS

_CRT_INSECURE_DEPRECATE(_splitpath_s)
_CRTIMP
void
__cdecl
_tsplitpath_x(
    _In_z_ const _TCHAR* path,
    _Pre_maybenull_ _Post_z_ _TCHAR* drive,
    _Pre_maybenull_ _Post_z_ _TCHAR* dir,
    _Pre_maybenull_ _Post_z_ _TCHAR* fname,
    _Pre_maybenull_ _Post_z_ _TCHAR* ext)
#endif
{
    const _TCHAR *src, *dir_start, *file_start = 0, *ext_start = 0;
    size_t count;
#if !IS_SECAPI
    const size_t drive_size = INT_MAX, dir_size = INT_MAX,
        fname_size = INT_MAX, ext_size = INT_MAX;
#endif

#if IS_SECAPI
    /* Validate parameters */
    if (MSVCRT_CHECK_PMT((path == NULL) ||
                         ((drive != NULL) && (drive_size == 0)) ||
                         ((dir != NULL) &&  (dir_size == 0)) ||
                         ((fname != NULL) && (fname_size == 0)) ||
                         ((ext != NULL) && (ext_size == 0))))
    {
        errno = EINVAL;
        return -1;
    }
#endif

    /* Truncate all output strings */
    if (drive) drive[0] = '\0';
    if (dir) dir[0] = '\0';
    if (fname) fname[0] = '\0';
    if (ext) ext[0] = '\0';

#if WINVER >= 0x600
    /* Check parameter */
    if (!path)
    {
#ifndef _LIBCNT_
        _set_errno(EINVAL);
#endif
        return _FAILURE;
    }
#endif

    _Analysis_assume_(path != 0);

#if WINVER == 0x600
    /* Skip '\\?\' prefix */
    if ((path[0] == '\\') && (path[1] == '\\') &&
        (path[2] == '?') && (path[3] == '\\')) path += 4;
#endif

    if (path[0] == '\0') return _FAILURE;

    /* Check if we have a drive letter (only 1 char supported) */
    if (path[1] == ':')
    {
        if (drive && (drive_size >= 3))
        {
            drive[0] = path[0];
            drive[1] = ':';
            drive[2] = '\0';
        }
        path += 2;
    }

    /* Scan the rest of the string */
    dir_start = path;
    while (*path != '\0')
    {
#if !defined(_UNICODE) && !defined(_LIBCNT_)
        /* Check for multibyte lead bytes */
        if (_ismbblead((unsigned char)*path))
        {
            /* Check for unexpected end of string */
            if (path[1] == 0) break;

            /* Skip the lead byte and the following byte */
            path += 2;
            continue;
        }
#endif
        /* Remember last path separator and last dot */
        if ((*path == '\\') || (*path == '/')) file_start = path + 1;
        if (*path == '.') ext_start = path;
        path++;
    }

    /* Check if we got a file name / extension */
    if (!file_start)
        file_start = dir_start;
    if (!ext_start || (ext_start < file_start))
        ext_start = path;

    if (dir)
    {
        src = dir_start;
        count = dir_size - 1;
        while ((src < file_start) && count--) *dir++ = *src++;
        *dir = '\0';
    }

    if (fname)
    {
        src = file_start;
        count = fname_size - 1;
        while (src < ext_start && count--) *fname++ = *src++;
        *fname = '\0';
    }

    if (ext)
    {
        src = ext_start;
        count = ext_size - 1;
        while (*src != '\0' && count--) *ext++ = *src++;
        *ext = '\0';
    }

    return _SUCCESS;
}

