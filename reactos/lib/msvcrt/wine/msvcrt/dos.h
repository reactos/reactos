/*
 * DOS definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DOS_H
#define __WINE_DOS_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

/* The following are also defined in io.h */
#define _A_NORMAL 0x00000000
#define _A_RDONLY 0x00000001
#define _A_HIDDEN 0x00000002
#define _A_SYSTEM 0x00000004
#define _A_VOLID  0x00000008
#define _A_SUBDIR 0x00000010
#define _A_ARCH   0x00000020

#ifndef MSVCRT_DISKFREE_T_DEFINED
#define MSVCRT_DISKFREE_T_DEFINED
struct _diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};
#endif /* MSVCRT_DISKFREE_T_DEFINED */


#ifdef __cplusplus
extern "C" {
#endif

unsigned int _getdiskfree(unsigned int, struct _diskfree_t *);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define diskfree_t _diskfree_t
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_DOS_H */
