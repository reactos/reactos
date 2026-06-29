/*
 * Path and directory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DIRECT_H
#define __WINE_DIRECT_H

#include <corecrt_wdirect.h>

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DISKFREE_T_DEFINED
#define _DISKFREE_T_DEFINED
struct _diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};
#endif /* _DISKFREE_T_DEFINED */

_ACRTIMP int           __cdecl _chdir(const char*);
_ACRTIMP int           __cdecl _chdrive(int);
_ACRTIMP char*         __cdecl _getcwd(char*,int);
_ACRTIMP char*         __cdecl _getdcwd(int,char*,int);
_ACRTIMP int           __cdecl _getdrive(void);
_ACRTIMP __msvcrt_ulong __cdecl _getdrives(void);
_ACRTIMP int           __cdecl _mkdir(const char*);
_ACRTIMP int           __cdecl _rmdir(const char*);

#ifdef __cplusplus
}
#endif


static inline int chdir(const char* newdir) { return _chdir(newdir); }
static inline char* getcwd(char * buf, int size) { return _getcwd(buf, size); }
static inline int mkdir(const char* newdir) { return _mkdir(newdir); }
static inline int rmdir(const char* dir) { return _rmdir(dir); }

#include <poppack.h>

#endif /* __WINE_DIRECT_H */
