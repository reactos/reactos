/*
 * DOS definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_DOS_H
#define __WINE_DOS_H

#include <corecrt.h>

#include <pshpack8.h>

/* The following are also defined in io.h */
#define _A_NORMAL 0x00000000
#define _A_RDONLY 0x00000001
#define _A_HIDDEN 0x00000002
#define _A_SYSTEM 0x00000004
#define _A_VOLID  0x00000008
#define _A_SUBDIR 0x00000010
#define _A_ARCH   0x00000020

#ifndef _DISKFREE_T_DEFINED
#define _DISKFREE_T_DEFINED
struct _diskfree_t {
  unsigned int total_clusters;
  unsigned int avail_clusters;
  unsigned int sectors_per_cluster;
  unsigned int bytes_per_sector;
};
#endif /* _DISKFREE_T_DEFINED */


#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP unsigned int __cdecl _getdiskfree(unsigned int, struct _diskfree_t *);

#ifdef __cplusplus
}
#endif


#define diskfree_t _diskfree_t

#include <poppack.h>

#endif /* __WINE_DOS_H */
