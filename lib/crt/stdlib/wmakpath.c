/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   gdalsnes
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

/* $Id$
 */
#include <precomp.h>

/*
 * @implemented
 */
void _wmakepath(wchar_t* path, const wchar_t* drive, const wchar_t* dir, const wchar_t* fname, const wchar_t* ext)
{
    int dir_len;

    if ((drive != NULL) && (*drive)) {
        path[0] = *drive;
	path[1] = L':';
	path[2] = 0;
    } else {
        (*path) = 0;
    }

    if (dir != NULL) {
        wcscat(path, dir);
        dir_len = wcslen(dir);
        if (dir_len && *(dir + dir_len - 1) != L'\\')
            wcscat(path, L"\\");
    }

    if (fname != NULL) {
        wcscat(path, fname);
        if (ext != NULL && *ext != 0) {
            if (*ext != L'.')
                wcscat(path, L".");
            wcscat(path, ext);
        }
    }
}
