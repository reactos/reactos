#ifndef _DIRECT_H_
#define _DIRECT_H_

#ifndef _WCHAR_T_
typedef unsigned short wchar_t;
#define _WCHAR_T_
#endif

#ifndef _SIZE_T_
typedef unsigned int size_t;
#define _SIZE_T_
#endif

struct _diskfree_t {
  unsigned short total_clusters;
  unsigned short avail_clusters;
  unsigned short sectors_per_cluster;
  unsigned short bytes_per_sector;
};

unsigned int _getdiskfree(unsigned int _drive, struct _diskfree_t *_diskspace);

int _chdrive( int drive );
int _getdrive( void );

char *_getcwd( char *buffer, int maxlen );
char *_getdcwd (int nDrive, char* caBuffer, int nBufLen);

int _chdir(const char *_path);
int  _mkdir(const char *_path);
int  _rmdir(const char *_path);

#define chdir _chdir
#define getcwd _getcwd
#define mkdir _mkdir
#define rmdir _rmdir


wchar_t *_wgetcwd( wchar_t *buffer, int maxlen );
wchar_t *_wgetdcwd (int nDrive, wchar_t* caBuffer, int nBufLen);

int _wchdir(const wchar_t *_path);
int  _wmkdir(const wchar_t *_path);
int  _wrmdir(const wchar_t *_path);

#endif
