/* $Id: wmakpath.c,v 1.2 2003/07/11 21:58:09 royce Exp $
 */
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


/*
 * @implemented
 */
void _wmakepath(wchar_t* path, const wchar_t* drive, const wchar_t* dir, const wchar_t* fname, const wchar_t* ext)
{
    int dir_len;

    if ((drive != NULL) && (*drive)) {
        wcscpy(path, drive);
        wcscat(path, L":");
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
