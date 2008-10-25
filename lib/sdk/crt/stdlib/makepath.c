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
    int dir_len;

    if ((drive != NULL) && (*drive)) {
        path[0] = *drive;
	path[1] = ':';
	path[2] = 0;
    } else {
        (*path)=0;
    }

    if (dir != NULL) {
        strcat(path, dir);
        dir_len = strlen(dir);
        if (dir_len && *(dir + dir_len - 1) != '\\')
            strcat(path, "\\");
    }

    if (fname != NULL) {
        strcat(path, fname);
        if (ext != NULL && *ext != 0) {
            if (*ext != '.')
                strcat(path, ".");
            strcat(path, ext);
        }
    }
}
