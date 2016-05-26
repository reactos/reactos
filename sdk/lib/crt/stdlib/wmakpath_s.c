/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     See COPYING in the top level directory
 * FILE:        lib/sdk/crt/stdlib/wmakpath_s.c
 * PURPOSE:     Creates a path
 * PROGRAMMERS: Wine team
 *              Copyright 1996,1998 Marcus Meissner
 *              Copyright 1996 Jukka Iivonen
 *              Copyright 1997,2000 Uwe Bonnes
 *              Copyright 2000 Jon Griffiths
 *
 */

#include <precomp.h>
#include <stdlib.h>
#include <string.h>

/*********************************************************************
 *		_wmakepath_s (MSVCRT.@)
 *
 * Safe version of _wmakepath.
 */
int CDECL _wmakepath_s(wchar_t *path, size_t size, const wchar_t *drive,
                       const wchar_t *directory, const wchar_t *filename,
                       const wchar_t *extension)
{
    wchar_t *p = path;

    if (!path || !size)
    {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if (drive && drive[0])
    {
        if (size <= 2)
            goto range;

        *p++ = drive[0];
        *p++ = ':';
        size -= 2;
    }

    if (directory && directory[0])
    {
        size_t len = strlenW(directory);
        unsigned int needs_separator = directory[len - 1] != '/' && directory[len - 1] != '\\';
        size_t copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, directory, copylen * sizeof(wchar_t));

        if (size <= len)
            goto range;

        p += copylen;
        size -= copylen;

        if (needs_separator)
        {
            if (size < 2)
                goto range;

            *p++ = '\\';
            size -= 1;
        }
    }

    if (filename && filename[0])
    {
        size_t len = strlenW(filename);
        size_t copylen = min(size - 1, len);

        if (size < 2)
            goto range;

        memmove(p, filename, copylen * sizeof(wchar_t));

        if (size <= len)
            goto range;

        p += len;
        size -= len;
    }

    if (extension && extension[0])
    {
        size_t len = strlenW(extension);
        unsigned int needs_period = extension[0] != '.';
        size_t copylen;

        if (size < 2)
            goto range;

        if (needs_period)
        {
            *p++ = '.';
            size -= 1;
        }

        copylen = min(size - 1, len);
        memcpy(p, extension, copylen * sizeof(wchar_t));

        if (size <= len)
            goto range;

        p += copylen;
    }

    *p = '\0';
    return 0;

range:
    path[0] = '\0';
    *_errno() = ERANGE;
    return ERANGE;
}

