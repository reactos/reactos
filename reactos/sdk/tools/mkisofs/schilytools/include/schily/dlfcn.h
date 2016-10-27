/* @(#)dlfcn.h	1.4 15/07/13 Copyright 2015 J. Schilling */
/*
 *	Abstraction from dlfcn.h
 *
 *	Copyright (c) 2015 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef _SCHILY_DLFCN_H
#define	_SCHILY_DLFCN_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_DLFCN_H
#ifndef _INCL_DLFCN_H
#include <dlfcn.h>			/* POSIX */
#define	_INCL_DLFCN_H
#define	FOUND_DLFCN_H
#endif
#endif

#ifdef	HAVE_DL_H
#ifndef	FOUND_DLFCN_H
#ifndef _INCL_DL_H
#include <dl.h>				/* HP-UX */
#define	_INCL_DL_H
#define	FOUND_DLFCN_H
#endif
#endif
#endif

#if defined(HAVE_DLOPEN) && defined(HAVE_DLCLOSE)		/* POSIX */
#define	HAVE_LOADABLE_LIBS
#endif

#if !defined(HAVE_LOADABLE_LIBS) && defined(HAVE_SHL_LOAD)	/* HP-UX */
#define	HAVE_LOADABLE_LIBS
#endif

#if !defined(HAVE_LOADABLE_LIBS) && defined(HAVE_LOADLIBRARY)	/* Win-DOS */
#define	HAVE_LOADABLE_LIBS
#endif

/*
 * dlopen() modes
 */
#ifndef	RTLD_LAZY			/* The only mode in SunOS-4.0 */
#define	RTLD_LAZY	0x00001
#define	RTLD_NOW	0x00002
#define	RTLD_GLOBAL	0x00100
#define	RTLD_LOCAL	0x00000
#endif

#ifdef	__never_
/*
 * dlsym() speudo handles
 * These handles are not valid on SunOS-4.0
 */
#ifndef	RTLD_NEXT
#define	RTLD_NEXT		(void *)-1
#define	RTLD_DEFAULT		(void *)-2
#define	RTLD_SELF		(void *)-3
#endif
#endif

/*
 * dlsym() speudo handle for SunOS-4.0
 */
#if	defined(HAVE_DLSYM) && !defined(RTLD_SELF)
#define	RTLD_SELF		(void *)0
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	HAVE_DLOPEN
extern	void	*dlopen	__PR((const char *__pathname, int mode));
#endif
#ifndef	HAVE_DLCLOSE
extern	int	dlclose	__PR((void *__handle));
#endif
#ifndef	HAVE_DLSYM
extern	void	*dlsym	__PR((void  *__handle, const char *name));
#endif
#ifndef	HAVE_DLERROR
extern	const char *dlerror __PR((void));
#endif

#ifdef	__cplusplus
}
#endif



#endif	/* _SCHILY_DLFCN_H */
