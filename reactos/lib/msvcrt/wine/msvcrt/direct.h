/*
 * Path and directory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DIRECT_H
#define __WINE_DIRECT_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_DISKFREE_T_DEFINED
#define MSVCRT_DISKFREE_T_DEFINED
struct _diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};
#endif /* MSVCRT_DISKFREE_T_DEFINED */

int         _chdir(const char*);
int         _chdrive(int);
char*       _getcwd(char*,int);
char*       _getdcwd(int,char*,int);
int         _getdrive(void);
unsigned long _getdrives(void);
int         _mkdir(const char*);
int         _rmdir(const char*);

#ifndef MSVCRT_WDIRECT_DEFINED
#define MSVCRT_WDIRECT_DEFINED
int              _wchdir(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)* _wgetcwd(MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)* _wgetdcwd(int,MSVCRT(wchar_t)*,int);
int              _wmkdir(const MSVCRT(wchar_t)*);
int              _wrmdir(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WDIRECT_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
static inline int chdir(const char* newdir) { return _chdir(newdir); }
static inline char* getcwd(char * buf, int size) { return _getcwd(buf, size); }
static inline int mkdir(const char* newdir) { return _mkdir(newdir); }
static inline int rmdir(const char* dir) { return _rmdir(dir); }
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_DIRECT_H */
