/* $Id: wmakpath.c,v 1.1 2002/11/24 18:42:25 robd Exp $
 */
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


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
