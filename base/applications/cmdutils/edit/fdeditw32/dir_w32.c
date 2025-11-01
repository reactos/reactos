#include <windows.h>
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include <ctype.h>
#include <string.h>

#include "dir_w32.h"

int findfirst(const char *_pathname, struct ffblk *_ffblk, int _attrib)
{
    struct _finddata_t c_file;
    int hFile;
    unsigned int wmask = 0;

    if (_attrib & FA_RDONLY) wmask |= _A_RDONLY;
    if (_attrib & FA_HIDDEN) wmask |= _A_HIDDEN;
    if (_attrib & FA_SYSTEM) wmask |= _A_SYSTEM;
    if (_attrib & FA_LABEL)  wmask |= 0; /* not available */
    if (_attrib & FA_DIREC)  wmask |= _A_SUBDIR;
    if (_attrib & FA_ARCH)   wmask |= _A_ARCH;

    if( (hFile = _findfirst(_pathname, &c_file )) == -1L )
        return 1;
    while (((c_file.attrib & wmask) != wmask && wmask != 0) ||
           ((c_file.attrib & (FA_RDONLY|FA_HIDDEN|FA_ARCH)) == 0 && wmask == 0)) {
        if (_findnext( hFile, &c_file ) != 0) {
            _findclose(hFile);
            return 1;
        }
    }

    _ffblk->__wmask = wmask;
    _ffblk->__hFile = hFile;
    strcpy(_ffblk->ff_name, c_file.name);
    _ffblk->ff_fsize = c_file.size;
    wmask = 0;
    if (c_file.attrib & _A_RDONLY) wmask |= FA_RDONLY;
    if (c_file.attrib & _A_HIDDEN) wmask |= FA_HIDDEN;
    if (c_file.attrib & _A_SYSTEM) wmask |= FA_SYSTEM;
    if (c_file.attrib & _A_SUBDIR) wmask |= FA_DIREC;
    if (c_file.attrib & _A_ARCH)   wmask |= FA_ARCH;
    _ffblk->ff_attrib = wmask;

    return 0;
}

int findnext(struct ffblk *_ffblk)
{
    struct _finddata_t c_file;
    int hFile = _ffblk->__hFile;
    unsigned int wmask = _ffblk->__wmask;

    do {
        if (_findnext( hFile, &c_file ) != 0) {
            _findclose(hFile);
            return 1;
        }
    } while (((c_file.attrib & wmask) != wmask && wmask != 0) ||
             ((c_file.attrib & (FA_RDONLY|FA_HIDDEN|FA_ARCH)) == 0 && wmask == 0));

    strcpy(_ffblk->ff_name, c_file.name);
    _ffblk->ff_fsize = c_file.size;
    wmask = 0;
    if (c_file.attrib & _A_RDONLY) wmask |= FA_RDONLY;
    if (c_file.attrib & _A_HIDDEN) wmask |= FA_HIDDEN;
    if (c_file.attrib & _A_SYSTEM) wmask |= FA_SYSTEM;
    if (c_file.attrib & _A_SUBDIR) wmask |= FA_DIREC;
    if (c_file.attrib & _A_ARCH)   wmask |= FA_ARCH;
    _ffblk->ff_attrib = wmask;

    return 0;
}

int getdisk(void)
{
    return _getdrive()-1;
}

int setdisk(int curdrive)
{
    DWORD drives;
    int maxdrive;

    /* If we can switch to the drive, it exists. */
    drives = GetLogicalDrives();
    for (maxdrive=31; maxdrive>=0; maxdrive--) {
        if (drives & (1<<maxdrive))
            break;
    }

    if (drives & (1<<curdrive))
        _chdrive(curdrive+1);

    return maxdrive;
}

int w32_getdisks(void)
{
    return GetLogicalDrives();
}

/* taken from DJGPP library source code */

static char *
max_ptr(char *p1, char *p2)
{
  if (p1 > p2)
    return p1;
  else
    return p2;
}

int
fnsplit (const char *path, char *drive, char *dir, 
	 char *name, char *ext)
{
  int flags = 0, len;
  const char *pp, *pe;

  if (drive)
    *drive = '\0';
  if (dir)
    *dir = '\0';
  if (name)
    *name = '\0';
  if (ext)
    *ext = '\0';

  pp = path;

  if ((isalpha((unsigned char )*pp)
       || strchr("[\\]^_`", *pp)) && (pp[1] == ':'))
  {
    flags |= DRIVE;
    if (drive)
    {
      strncpy(drive, pp, 2);
      drive[2] = '\0';
    }
    pp += 2;
  }

  pe = max_ptr(strrchr(pp, '\\'), strrchr(pp, '/'));
  if (pe) 
  { 
    flags |= DIRECTORY;
    pe++;
    len = pe - pp;
    if (dir)
    {
      strncpy(dir, pp, len);
      dir[len] = '\0';
    }
    pp = pe;
  }
  else
    pe = pp;

  /* Special case: "c:/path/." or "c:/path/.."
     These mean FILENAME, not EXTENSION.  */
  while (*pp == '.')
    ++pp;
  if (pp > pe)
  {
    flags |= FILENAME;
    if (name)
    {
      len = pp - pe;
      strncpy(name, pe, len);
      name[len] = '\0';
      /* advance name over '.'s so they don't get scragged later on when the
       * rest of the name (if any) is copied (for files like .emacs). - WJC
       */
      name+=len;
    }
  }

  pe = strrchr(pp, '.');
  if (pe)
  {
    flags |= EXTENSION;
    if (ext) 
      strcpy(ext, pe);
  }
  else 
    pe = strchr( pp, '\0');

  if (pp != pe)
  {
    flags |= FILENAME;
    len = pe - pp;
    if (name)
    {
      strncpy(name, pp, len);
      name[len] = '\0';
    }
  }

  if (strcspn(path, "*?[") < strlen(path))
    flags |= WILDCARDS;

  return flags;
}

void
fnmerge (char *path, const char *drive, const char *dir,
	 const char *name, const char *ext)
{
  *path = '\0';
  if (drive && *drive)
  {
    path[0] = drive[0];
    path[1] = ':';
    path[2] = 0;
  }
  if (dir && *dir)
  {
    char last_dir_char = dir[strlen(dir) - 1];

    strcat(path, dir);
    if (last_dir_char != '/' && last_dir_char != '\\')
      strcat(path, strchr(dir, '\\') ? "\\" : "/");
  }
  if (name)
    strcat(path, name);
  if (ext && *ext)
  {
    if (*ext != '.')
      strcat(path, ".");
    strcat(path, ext);
  }
}
