/* fsusage.c -- return space usage of mounted filesystems
   Copyright (C) 1991, 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if !defined(NO_INFOMOUNT) || defined(__QNX__)

#if defined(__QNX__)
#define STAT_STATFS4
#endif

#include <sys/types.h>

#include "fsusage.h"

int statfs ();  /* We leave the type ambiguous intentionally here */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_FILSYS_H
#include <sys/filsys.h>		/* SVR2.  */
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#ifdef HAVE_DUSTAT_H		/* AIX PS/2.  */
#include <sys/dustat.h>
#endif

#ifdef HAVE_SYS_STATVFS_H	/* SVR4.  */
#include <sys/statvfs.h>
#endif

/* Return the number of TOSIZE-byte blocks used by
   BLOCKS FROMSIZE-byte blocks, rounding away from zero.
   TOSIZE must be positive.  Return -1 if FROMSIZE is not positive.  */

long fs_adjust_blocks (long blocks, int fromsize, int tosize)
{
    if (tosize <= 0)
	abort ();
    if (fromsize <= 0)
	return -1;

    if (fromsize == tosize)	/* E.g., from 512 to 512.  */
	return blocks;
    else if (fromsize > tosize)	/* E.g., from 2048 to 512.  */
	return blocks * (fromsize / tosize);
    else			/* E.g., from 256 to 512.  */
	return (blocks + (blocks < 0 ? -1 : 1)) / (tosize / fromsize);
}

/* Fill in the fields of FSP with information about space usage for
   the filesystem on which PATH resides.
   Return 0 if successful, -1 if not. */

int get_fs_usage (char *path, struct fs_usage *fsp)
{
#ifdef STAT_STATFS3_OSF1
    struct statfs fsd;

    if (statfs (path, &fsd, sizeof (struct statfs)) != 0)
	 return -1;
#define CONVERT_BLOCKS(b) fs_adjust_blocks ((b), fsd.f_fsize, 512)
#endif				/* STAT_STATFS3_OSF1 */

#ifdef STAT_STATFS2_FS_DATA	/* Ultrix.  */
    struct fs_data fsd;

    if (statfs (path, &fsd) != 1)
	return -1;
#define CONVERT_BLOCKS(b) fs_adjust_blocks ((b), 1024, 512)
    fsp->fsu_blocks = CONVERT_BLOCKS (fsd.fd_req.btot);
    fsp->fsu_bfree = CONVERT_BLOCKS (fsd.fd_req.bfree);
    fsp->fsu_bavail = CONVERT_BLOCKS (fsd.fd_req.bfreen);
    fsp->fsu_files = fsd.fd_req.gtot;
    fsp->fsu_ffree = fsd.fd_req.gfree;
#endif

#ifdef STAT_STATFS2_BSIZE	/* 4.3BSD, SunOS 4, HP-UX, AIX.  */
    struct statfs fsd;

    if (statfs (path, &fsd) < 0)
	return -1;
#define CONVERT_BLOCKS(b) fs_adjust_blocks ((b), fsd.f_bsize, 512)
#endif

#ifdef STAT_STATFS2_FSIZE	/* 4.4BSD.  */
    struct statfs fsd;

    if (statfs (path, &fsd) < 0)
	return -1;
#define CONVERT_BLOCKS(b) fs_adjust_blocks ((b), fsd.f_fsize, 512)
#endif

#ifdef STAT_STATFS4		/* SVR3, Dynix, Irix, AIX.  */
    struct statfs fsd;

    if (statfs (path, &fsd, sizeof fsd, 0) < 0)
	return -1;
    /* Empirically, the block counts on most SVR3 and SVR3-derived
       systems seem to always be in terms of 512-byte blocks,
       no matter what value f_bsize has.  */
#if _AIX
#define CONVERT_BLOCKS(b) fs_adjust_blocks ((b), fsd.f_bsize, 512)
#else
#define CONVERT_BLOCKS(b) (b)
#ifndef _SEQUENT_		/* _SEQUENT_ is DYNIX/ptx.  */
#ifndef DOLPHIN			/* DOLPHIN 3.8.alfa/7.18 has f_bavail */
#define f_bavail f_bfree
#endif
#endif
#endif
#endif

#ifdef STAT_STATVFS		/* SVR4.  */
    struct statvfs fsd;

    if (statvfs (path, &fsd) < 0)
	return -1;
    /* f_frsize isn't guaranteed to be supported.  */
#define CONVERT_BLOCKS(b) \
  fs_adjust_blocks ((b), fsd.f_frsize ? fsd.f_frsize : fsd.f_bsize, 512)
#endif

#if defined(CONVERT_BLOCKS) && !defined(STAT_STATFS2_FS_DATA) && !defined(STAT_READ_FILSYS)	/* !Ultrix && !SVR2.  */
    fsp->fsu_blocks = CONVERT_BLOCKS (fsd.f_blocks);
    fsp->fsu_bfree = CONVERT_BLOCKS (fsd.f_bfree);
    fsp->fsu_bavail = CONVERT_BLOCKS (fsd.f_bavail);
    fsp->fsu_files = fsd.f_files;
    fsp->fsu_ffree = fsd.f_ffree;
#endif

    return 0;
}

#if defined(_AIX) && defined(_I386)
/* AIX PS/2 does not supply statfs.  */

int statfs (char *path, struct statfs *fsb)
{
    struct stat stats;
    struct dustat fsd;

    if (stat (path, &stats))
	return -1;
    if (dustat (stats.st_dev, 0, &fsd, sizeof (fsd)))
	return -1;
    fsb->f_type = 0;
    fsb->f_bsize = fsd.du_bsize;
    fsb->f_blocks = fsd.du_fsize - fsd.du_isize;
    fsb->f_bfree = fsd.du_tfree;
    fsb->f_bavail = fsd.du_tfree;
    fsb->f_files = (fsd.du_isize - 2) * fsd.du_inopb;
    fsb->f_ffree = fsd.du_tinode;
    fsb->f_fsid.val[0] = fsd.du_site;
    fsb->f_fsid.val[1] = fsd.du_pckno;
    return 0;
}
#endif				/* _AIX && _I386 */
#endif /* NO_INFOMOUNT */
