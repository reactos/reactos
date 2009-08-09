/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     See COPYING in the top level directory
 * FILE:        lib/sdk/crt/stdlib/makepath.c
 * PURPOSE:     Creates a path
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
#include <stdlib.h>
#include <string.h>

/*
 * @implemented
 */
void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext)
{
    char *p = path;

    if ( !path )
        return;

    if (drive && drive[0])
    {
        *p++ = drive[0];
        *p++ = ':';
    }
    if (dir && dir[0])
    {
        unsigned int len = strlen(dir);
        memmove(p, dir, len);
        p += len;
        if (p[-1] != '/' && p[-1] != '\\')
            *p++ = '\\';
    }
    if (fname && fname[0])
    {
        unsigned int len = strlen(fname);
        memmove(p, fname, len);
        p += len;
    }
    if (ext && ext[0])
    {
        if (ext[0] != '.')
            *p++ = '.';
        strcpy(p, ext);
    }
    else
        *p = '\0';
}
