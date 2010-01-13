/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     See COPYING in the top level directory
 * FILE:        lib/sdk/crt/stdlib/wmakpath.c
 * PURPOSE:     Creates a unicode path
 * PROGRAMMERS: Wine team
 *              Copyright 1996,1998 Marcus Meissner
 *              Copyright 1996 Jukka Iivonen
 *              Copyright 1997,2000 Uwe Bonnes
 *              Copyright 2000 Jon Griffiths
 *
 */

/* $Id$
 */
#include <precomp.h>

/*
 * @implemented
 */
void _wmakepath(wchar_t* path, const wchar_t* drive, const wchar_t* dir, const wchar_t* fname, const wchar_t* ext)
{
    wchar_t *p = path;

    if ( !path )
        return;

    if (drive && drive[0])
    {
        *p++ = drive[0];
        *p++ = ':';
    }
    if (dir && dir[0])
    {
        unsigned int len = strlenW(dir);
        memmove(p, dir, len * sizeof(wchar_t));
        p += len;
        if (p[-1] != '/' && p[-1] != '\\')
            *p++ = '\\';
    }
    if (fname && fname[0])
    {
        unsigned int len = strlenW(fname);
        memmove(p, fname, len * sizeof(wchar_t));
        p += len;
    }
    if (ext && ext[0])
    {
        if (ext[0] != '.')
            *p++ = '.';
        strcpyW(p, ext);
    }
    else
        *p = '\0';
}
