/* @(#)device.h	1.19 09/11/16 Copyright 1995-2007 J. Schilling */
/*
 *	Generic header for users of major(), minor() and makedev()
 *
 *	Copyright (c) 1995-2007 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_DEVICE_H
#define	_SCHILY_DEVICE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * On generic SVR4, major is a function (defined in sys/mkdev.h).
 * On Solaris it is defined ...
 * As we cannot just test if major is #define'd, we have to
 * define _FOUND_MAJOR_ instead.
 *
 * WARNING: Do never include <sys/sysmacros.h> in SVR4, it contains
 * an old definition for major()/minor() defining 8 minorbits.
 * Use <sys/mkdev.h> instead.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

/*
 * Some systems define major in <sys/types.h>.
 * We are ready...
 */
#ifdef major
#	define _FOUND_MAJOR_
#endif

#ifdef MAJOR_IN_MKDEV
#	ifndef	_INCL_SYS_MKDEV_H
#	include <sys/mkdev.h>
#	define	_INCL_SYS_MKDEV_H
#	endif
	/*
	 * Interix doesn't use makedev(); it uses mkdev()
	 */
#	if !defined(makedev) && defined(mkdev)
#		define makedev(a, b)	mkdev((a), (b))
#	endif
#	define _FOUND_MAJOR_
#endif

#ifndef _FOUND_MAJOR_
#	ifdef MAJOR_IN_SYSMACROS
#		ifndef	_INCL_SYS_SYSMACROS_H
#		include <sys/sysmacros.h>
#		define	_INCL_SYS_SYSMACROS_H
#		endif
#		define _FOUND_MAJOR_
#	endif
#endif

/*
 * If we are on HP/UX before HP/UX 8,
 * major/minor are not in <sys/sysmacros.h>.
 */
#ifndef _FOUND_MAJOR_
#	if defined(hpux) || defined(__hpux__) || defined(__hpux)
#		ifndef	_INCL_SYS_MKOD_H
#		include <sys/mknod.h>
#		define	_INCL_SYS_MKOD_H
#		endif
#		define _FOUND_MAJOR_
#	endif
#endif

#ifndef	_FOUND_MAJOR_
#	ifdef VMS
#		define major(dev)		(((((long)dev) >> 8) & 0xFF))
#		define minor(dev)		((((long)dev) & 0xFF))
#		define makedev(majo, mino)	(((majo) << 8) | (mino))
#		define _FOUND_MAJOR_
#	endif /* VMS */
#endif /* _FOUND_MAJOR_ */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * For all other systems define major()/minor() here.
 * XXX Check if this definition will be usefull for ms dos too.
 */
#ifndef _FOUND_MAJOR_
#	define major(dev)		(((dev) >> 8) & 0xFF)
#	define minor(dev)		((dev) & 0xFF)
#	define makedev(majo, mino)	(((majo) << 8) | (mino))
#endif

/*
 * Don't pollute namespace...
 */
#undef _FOUND_MAJOR_

#ifdef	__XDEV__
/*
 * The following defines are currently only needed for 'star'.
 * We make it conditional code to avoid to pollute the namespace.
 */
#define	XDEV_T	unsigned long

extern	int	minorbits;
extern	XDEV_T	minormask;
extern	XDEV_T	_dev_mask[];

#define	dev_major(dev)			(((XDEV_T)(dev)) >> minorbits)
#define	_dev_major(mbits, dev)		(((XDEV_T)(dev)) >> (mbits))

#define	dev_minor(dev)			(((XDEV_T)(dev)) & minormask)
#define	_dev_minor(mbits, dev)		(((XDEV_T)(dev)) & _dev_mask[(mbits)])


#define	dev_make(majo, mino)		((((XDEV_T)(majo)) << minorbits) | \
							((XDEV_T)(mino)))
#define	_dev_make(mbits, majo, mino)	((((XDEV_T)(majo)) << (mbits) | \
							((XDEV_T)(mino)))

extern	void	dev_init	__PR((BOOL debug));
#ifndef	dev_major
extern	XDEV_T	dev_major	__PR((XDEV_T dev));
extern	XDEV_T	_dev_major	__PR((int mbits, XDEV_T dev));
extern	XDEV_T	dev_minor	__PR((XDEV_T dev));
extern	XDEV_T	_dev_minor	__PR((int mbits, XDEV_T dev));
extern	XDEV_T	dev_make	__PR((XDEV_T majo, XDEV_T mino));
extern	XDEV_T	_dev_make	__PR((int mbits, XDEV_T majo, XDEV_T mino));
#endif

#endif	/* __XDEV__ */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_DEVICE_H */
