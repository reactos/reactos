/* $Id: makepath.c,v 1.8 2002/11/24 18:42:18 robd Exp $
 */
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

void _makepath(char *path, const char *drive, const char *dir, const char *fname, const char *ext)
{
  int dir_len;

    if ((drive != NULL) && (*drive)) {
      strcpy(path, drive);
      strcat(path, ":");
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
