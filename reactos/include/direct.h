#ifndef __include_direct_h_
#define __include_direct_h_

struct _diskfree_t {
  unsigned short total_clusters;
  unsigned short avail_clusters;
  unsigned short sectors_per_cluster;
  unsigned short bytes_per_sector;
};


int _chdrive( int drive );
int _getdrive( void );
char *_getcwd( char *buffer, int maxlen );

int _chdir(const char *_path);
char *_getcwd(char *, int);
int  _mkdir(const char *_path);
int  _rmdir(const char *_path);
unsigned int   _getdiskfree(unsigned int _drive, struct _diskfree_t *_diskspace);
#define chdir _chdir
#define getcwd _getcwd
#define mkdir _mkdir
#define rmdir _rmdir


#endif
