/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         CRT: implementation of _splitpath / _wsplitpath
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
void _tsplitpath(const _TCHAR* path, _TCHAR* drive, _TCHAR* dir, _TCHAR* fname, _TCHAR* ext)
{
    const _TCHAR *src, *dir_start, *file_start = 0, *ext_start = 0;

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
        return;
    }
#endif

#if WINVER == 0x600
    /* Skip '\\?\' prefix */
    if ((path[0] == '\\') && (path[1] == '\\') &&
        (path[2] == '?') && (path[3] == '\\')) path += 4;
#endif

    if (path[0] == '\0') return;

    /* Check if we have a drive letter (only 1 char supported) */
    if (path[1] == ':')
    {
        if (drive)
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
        /* Remember last path seperator and last dot */
        if ((*path == '\\') || (*path == '/')) file_start = path + 1;
        if (*path == '.') ext_start = path;
        path++;
    }

    /* Check if we got a file name / extension */
    if (!file_start)
        file_start = dir_start;
    if (!ext_start || ext_start < file_start)
        ext_start = path;

    if (dir)
    {
        src = dir_start;
        while (src < file_start) *dir++ = *src++;
        *dir = '\0';
    }

    if (fname)
    {
        src = file_start;
        while (src < ext_start) *fname++ = *src++;
        *fname = '\0';
    }

    if (ext)
    {
        src = ext_start;
        while (*src != '\0') *ext++ = *src++;
        *ext = '\0';
    }
}

