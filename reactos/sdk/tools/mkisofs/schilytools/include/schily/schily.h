/* @(#)schily.h	1.122 16/12/18 Copyright 1985-2016 J. Schilling */
/*
 *	Definitions for libschily
 *
 *	This file should be included past:
 *
 *	schily/mconfig.h / config.h
 *	schily/standard.h
 *	stdio.h
 *	stdlib.h	(better use schily/stdlib.h)
 *	unistd.h	(better use schily/unistd.h) needed f. LARGEFILE support
 *	schily/string.h
 *	sys/types.h
 *
 *	If you need stdio.h, you must include it before schily/schily.h
 *
 *	NOTE: If you need ctype.h and did not include stdio.h you need to
 *	include ctype.h past schily/schily.h as OpenBSD does not follow POSIX
 *	and defines EOF in ctype.h
 *
 *	Copyright (c) 1985-2016 J. Schilling
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

#ifndef _SCHILY_SCHILY_H
#define	_SCHILY_SCHILY_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif
#ifndef _SCHILY_CCOMDEFS_H
#include <schily/ccomdefs.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_H) || defined(off_t)
#	ifndef	FOUND_OFF_T
#	define	FOUND_OFF_T
#	endif
#endif
#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_H) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif
#if	defined(_MSC_VER) && !defined(_SIZE_T_DEFINED)
#	undef	FOUND_SIZE_T
#endif

#ifdef	__never_def__
/*
 * It turns out that we cannot use the folloginw definition because there are
 * some platforms that do not behave application friendly. These are mainly
 * BSD-4.4 based systems (which #undef a definition when size_t is available.
 * We actually removed this code because of a problem with QNX Neutrino.
 * For this reason, it is important not to include <sys/types.h> directly but
 * via the Schily SING include files so we know whether it has been included
 * before we come here.
 */
#if	defined(_SIZE_T)	|| defined(_T_SIZE_)	|| defined(_T_SIZE) || \
	defined(__SIZE_T)	|| defined(_SIZE_T_)	|| \
	defined(_GCC_SIZE_T)	|| defined(_SIZET_)	|| \
	defined(__sys_stdtypes_h) || defined(___int_size_t_h) || defined(size_t)

#ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T	/* We already included a size_t definition */
#endif
#endif
#endif	/* __never_def__ */

#if	defined(HAVE_LARGEFILES)
#	define	_fcons		_fcons64
#	define	fdup		fdup64
#	define	fileluopen	fileluopen64
#	define	fileopen	fileopen64
#	define	filemopen	filemopen64
#	define	filepos		filepos64
#	define	filereopen	filereopen64
#	define	fileseek	fileseek64
#	define	filesize	filesize64
#	define	filestat	filestat64
#	define	_openfd		_openfd64
#endif

/*
 * The official POSIX rule is not to define "new" interfaces that
 * are in conflict with older interfaces of the same name.
 * Our interfaces fexec*() have been defined and published in 1982.
 * The new POSIX interfaces define a different interface and the
 * new POSIX interfaces even use names that are not compatible with
 * POSIX rules. The new POSIX interfaces in question should be called
 * fdexec*() to follow the rules of other similar POSIX functions.
 * Simiar problems exist with getline()/fgetline().
 */
#if	defined(HAVE_RAW_FEXECL) || defined(HAVE_RAW_FEXECLE) || \
	defined(HAVE_RAW_FEXECV) || defined(HAVE_RAW_FEXECVE)
#define	RENAME_FEXEC
#endif
#if	defined(HAVE_RAW_FSPAWNV) || defined(HAVE_RAW_FSPAWNL) || \
	defined(HAVE_RAW_FSPAWNV_NOWAIT)
#define	RENAME_FSPAWN
#endif
#if	defined(HAVE_RAW_GETLINE) || defined(HAVE_RAW_FGETLINE)
#define	RENAME_GETLINE
#endif

#ifdef	__needed__
#define	RENAME_FEXEC
#define	RENAME_FSPAWN
#define	RENAME_GETLINE
#endif

#if	defined(RENAME_FEXEC) || defined(RENAME_FSPAWN)
#ifndef	_SCHILY_UNISTD_H
#include <schily/unistd.h>	/* Need to incl. before fexec*() protoypes */
#endif
#endif

#if	defined(RENAME_GETLINE)
#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>	/* Need to incl. before *getline() protoypes */
#endif

#endif

#ifdef	EOF	/* stdio.h has been included */

extern	int	_cvmod __PR((const char *, int *, int *));
extern	FILE	*_fcons __PR((FILE *, int, int));
extern	FILE	*fdup __PR((FILE *));
#if	!defined(fdown) || defined(PROTOTYPES)
/*
 * We cannot declare fdown() with K&R in case that fdown() has been #define'd
 */
extern	int	fdown __PR((FILE *));
#endif
extern	int	js_fexecl __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
extern	int	js_fexecle __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
		/* 6th arg not const, fexecv forces av[ac] = NULL */
extern	int	js_fexecv __PR((const char *, FILE *, FILE *, FILE *, int,
							char **));
extern	int	js_fexecve __PR((const char *, FILE *, FILE *, FILE *,
					char * const *, char * const *));
extern	int	js_fspawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
extern	int	js_fspawnl __PR((FILE *, FILE *, FILE *, const char *, ...));
extern	int	js_fspawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
extern	int	js_fgetline __PR((FILE *, char *, int));
#ifdef	FOUND_SIZE_T
extern	ssize_t	fgetaline __PR((FILE *, char **, size_t *));
extern	ssize_t	getaline __PR((char **, size_t *));
#endif
extern	int	fgetstr __PR((FILE *, char *, int));
extern	int	file_getraise __PR((FILE *));
extern	void	file_raise __PR((FILE *, int));
extern	int	fileclose __PR((FILE *));
extern	FILE	*fileluopen __PR((int, const char *));
extern	FILE	*fileopen __PR((const char *, const char *));
#ifdef	_SCHILY_TYPES_H
extern	FILE	*filemopen __PR((const char *, const char *, mode_t));
#endif
#ifdef	FOUND_OFF_T
extern	off_t	filepos __PR((FILE *));
#endif
#ifdef	FOUND_SIZE_T
extern	ssize_t	fileread __PR((FILE *, void *, size_t));
extern	ssize_t	ffileread __PR((FILE *, void *, size_t));
#endif
extern	FILE	*filereopen __PR((const char *, const char *, FILE *));
#ifdef	FOUND_OFF_T
extern	int	fileseek __PR((FILE *, off_t));
extern	off_t	filesize __PR((FILE *));
#endif
#ifdef	S_IFMT
extern	int	filestat __PR((FILE *, struct stat *));
#endif
#ifdef	FOUND_SIZE_T
extern	ssize_t	filewrite __PR((FILE *, void *, size_t));
extern	ssize_t	ffilewrite __PR((FILE *, void *, size_t));
#endif
extern	int	flush __PR((void));
extern	int	fpipe __PR((FILE **));
#ifdef	__never__
extern	int	fprintf __PR((FILE *, const char *, ...)) __printflike__(2, 3);
#endif
extern	int	getbroken __PR((FILE *, char *, char, char **, int));
extern	int	ofindline __PR((FILE *, char, const char *, int,
							char **, int));
extern	int	peekc __PR((FILE *));

#ifdef	__never_def__
/*
 * We cannot define this or we may get into problems with DOS based systems.
 */
extern	int	spawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
extern	int	spawnl __PR((FILE *, FILE *, FILE *, const char *, ...));
extern	int	spawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
#endif	/* __never_def__ */
#endif	/* EOF */

/*
 * Flags for absfpath() and resolvefpath():
 */
#define	RSPF_EXIST		0x01	/* All path components must exist    */
#define	RSPF_NOFOLLOW_LAST	0x02	/* Don't follow link in last pathcomp */

#ifdef	FOUND_SIZE_T
extern	char	*abspath __PR((const char *relp, char *absp, size_t asize));
extern	char	*absnpath __PR((const char *relp, char *absp, size_t asize));
extern	char	*absfpath __PR((const char *relp, char *absp, size_t asize,
				int __flags));
#ifndef	HAVE_RESOLVEPATH
extern	int	resolvepath __PR((const char *__path,
				char *__buf, size_t __bufsiz));
#endif
extern	int	resolvenpath __PR((const char *__path,
				char *__buf, size_t __bufsiz));
extern	int	resolvefpath __PR((const char *__path,
				char *__buf, size_t __bufsiz, int __flags));
#endif

#ifdef	_SCHILY_TYPES_H
extern	int	mkdirs __PR((char *, mode_t));
extern	int	makedirs __PR((char *, mode_t, int __striplast));
#endif

extern	int	lxchdir	__PR((char *));
#ifdef	HAVE_FCHDIR
#define	fdsetname(fd, name)	(0)
#define	fdclosename(fd)		(0)
#else
extern	int	fdsetname __PR((int fd, const char *name));
extern	int	fdclosename __PR((int fd));
#endif
extern	int	diropen __PR((const char *));
extern	int	dirrdopen __PR((const char *));
extern	int	dirclose __PR((int));

struct save_wd {
	int	fd;
	char	*name;
};

extern int	savewd	__PR((struct save_wd *sp));
extern void	closewd	__PR((struct save_wd *sp));
extern int	restorewd __PR((struct save_wd *sp));


#ifdef	_SCHILY_UTYPES_H
typedef struct gnmult {
	char	key;
	Llong	mult;
} gnmult_t;

extern	int	getllnum __PR((char *arg, Llong *lvalp));
extern	int	getxnum	 __PR((char *arg, long *valp, gnmult_t *mult));
extern	int	getllxnum __PR((char *arg, Llong *lvalp, gnmult_t *mult));

extern	int	getlltnum  __PR((char *arg, Llong *lvalp));
extern	int	getxtnum  __PR((char *arg, time_t *valp, gnmult_t *mult));
extern	int	getllxtnum __PR((char *arg, Llong *lvalp, gnmult_t *mult));
#endif
extern	int	getnum	__PR((char *arg, long *valp));
#ifdef	_SCHILY_TIME_H
extern	int	gettnum	__PR((char *arg, time_t *valp));
#endif

#ifdef	_SCHILY_TIME_H

extern	int		getnstimeofday	__PR((struct timespec *__tp));
extern	int		setnstimeofday	__PR((struct timespec *__tp));

#ifdef	_SCHILY_UTYPES_H
extern	Llong		mklgmtime	__PR((struct tm *));
#endif
extern	time_t		mkgmtime	__PR((struct tm *));
#endif


#ifdef	EOF			/* stdio.h has been included */
#ifdef	_SCHILY_TYPES_H
/*
 * getperm() flags:
 */
#define	GP_NOX		0	/* This is not a dir and 'X' is not valid */
#define	GP_DOX		1	/* 'X' perm character is valid		  */
#define	GP_XERR		2	/* 'X' perm characters are invalid	  */
#define	GP_FPERM	4	/* TRUE if we implement find -perm	  */
#define	GP_UMASK	8	/* TRUE if we implement umask		  */

extern	int	getperm	__PR((FILE *f, char *perm, char *opname, \
				mode_t *modep, int smode, int flag));
extern	void	permtostr	__PR((mode_t mode, char *));
#endif
#endif

#ifdef	FOUND_SIZE_T
extern	ssize_t	_niread __PR((int, void *, size_t));
extern	ssize_t	_niwrite __PR((int, void *, size_t));
extern	ssize_t	_nixread __PR((int, void *, size_t));
extern	ssize_t	_nixwrite __PR((int, void *, size_t));
#endif
extern	int	_openfd __PR((const char *, int));
extern	int	on_comerr __PR((void (*fun)(int, void *), void *arg));
/*PRINTFLIKE1*/
extern	void	comerr __PR((const char *, ...)) __printflike__(1, 2);
/*PRINTFLIKE2*/
extern	void	xcomerr	 __PR((int, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE2*/
extern	void	comerrno __PR((int, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE3*/
extern	void	xcomerrno __PR((int, int, const char *, ...)) __printflike__(3, 4);
/*PRINTFLIKE1*/
extern	int	errmsg __PR((const char *, ...)) __printflike__(1, 2);
/*PRINTFLIKE2*/
extern	int	errmsgno __PR((int, const char *, ...)) __printflike__(2, 3);
#ifdef	FOUND_SIZE_T
/*PRINTFLIKE3*/
extern	int	serrmsg __PR((char *, size_t, const char *, ...))
					__printflike__(3, 4);
/*PRINTFLIKE4*/
extern	int	serrmsgno __PR((int, char *, size_t, const char *, ...))
					__printflike__(4, 5);
#endif
extern	void	comexit	__PR((int));
extern	char	*errmsgstr __PR((int));

#ifdef	EOF	/* stdio.h has been included */
/*PRINTFLIKE2*/
extern	void	fcomerr		__PR((FILE *, const char *, ...))
					__printflike__(2, 3);
/*PRINTFLIKE3*/
extern	void	fxcomerr	__PR((FILE *, int, const char *, ...))
					__printflike__(3, 4);
/*PRINTFLIKE3*/
extern	void	fcomerrno	__PR((FILE *, int, const char *, ...))
					__printflike__(3, 4);
/*PRINTFLIKE4*/
extern	void	fxcomerrno	__PR((FILE *, int, int, const char *, ...))
					__printflike__(4, 5);
/*PRINTFLIKE2*/
extern	int	ferrmsg		__PR((FILE *, const char *, ...))
					__printflike__(2, 3);
/*PRINTFLIKE3*/
extern	int	ferrmsgno	__PR((FILE *, int, const char *, ...))
					__printflike__(3, 4);
#ifdef	_SCHILY_VARARGS_H
#define	COMERR_RETURN	0
#define	COMERR_EXIT	1
#define	COMERR_EXCODE	2
/*PRINTFLIKE5*/
extern	int	_comerr		__PR((FILE *, int, int, int,
						const char *, va_list));
#endif
#endif

/*PRINTFLIKE1*/
extern	int	error __PR((const char *, ...)) __printflike__(1, 2);
#ifdef	FOUND_SIZE_T
extern	char	*fillbytes __PR((void *, ssize_t, char));
extern	char	*zerobytes __PR((void *, ssize_t));
extern	char	*findbytes __PR((const void *, ssize_t, char));
#endif
extern	char	*findinpath __PR((char *__name, int __mode,
					BOOL __plain_file, char *__path));
extern	int	findline __PR((const char *, char, const char *,
							int, char **, int));
extern	int	js_getline __PR((char *, int));
extern	int	getstr __PR((char *, int));
extern	int	breakline __PR((char *, char, char **, int));
extern	int	getallargs __PR((int *, char * const**, const char *, ...));
extern	int	getargs __PR((int *, char * const**, const char *, ...));
extern	int	getfiles __PR((int *, char * const**, const char *));
extern	char	*astoi __PR((const char *, int *));
extern	char	*astol __PR((const char *, long *));
extern	char	*astolb __PR((const char *, long *, int base));
#ifdef	_SCHILY_UTYPES_H
extern	char	*astoll __PR((const char *, Llong *));
extern	char	*astollb __PR((const char *, Llong *, int));
extern	char	*astoull __PR((const char *, Ullong *));
extern	char	*astoullb __PR((const char *, Ullong *, int));
#endif

extern	int		patcompile __PR((const unsigned char *, int, int *));
extern	unsigned char	*patmatch __PR((const unsigned char *, const int *,
					const unsigned char *,
					int, int, int, int[]));
extern	unsigned char	*patlmatch __PR((const unsigned char *, const int *,
					const unsigned char *,
					int, int, int, int[]));

#ifdef	__never__
extern	int	printf __PR((const char *, ...)) __printflike__(1, 2);
#endif
#ifdef	FOUND_SIZE_T
extern	char	*movebytes __PR((const void *, void *, ssize_t));
extern	char	*movecbytes __PR((const void *, void *, int, size_t));
#endif

extern	void	save_args __PR((int, char **));
extern	int	saved_ac __PR((void));
extern	char	**saved_av __PR((void));
extern	char	*saved_av0 __PR((void));
extern	char	*searchfileinpath __PR((char *__name, int __mode,
					int __file_mode, char *__path));
#define	SIP_ANY_FILE	0x00	/* Search for any file type		*/
#define	SIP_PLAIN_FILE	0x01	/* Search for plain files - not dirs	*/
#define	SIP_NO_PATH	0x10	/* Do not do PATH search		*/
#define	SIP_ONLY_PATH	0x20	/* Do only PATH search			*/
#define	SIP_NO_STRIPBIN	0x40	/* Do not strip "/bin" from PATH elem.	*/
#define	SIP_TYPE_MASK	0x0F	/* Mask file type related bits		*/

#ifndef	seterrno
extern	int	seterrno __PR((int));
#endif
extern	void	set_progname __PR((const char *));
extern	char	*get_progname __PR((void));
extern	char	*get_progpath __PR((void));
extern	char	*getexecpath __PR((void));

extern	void	setfp __PR((void * const *));
extern	int	wait_chld __PR((int));		/* for fspawnv_nowait() */
extern	int	geterrno __PR((void));
extern	void	raisecond __PR((const char *, long));
#ifdef	__never__
/*
 * sprintf() may be declared incorrectly somewhere else
 * e.g. in old BSD include files
 */
extern	int	sprintf __PR((char *, const char *, ...));
#endif
extern	char	*strcatl __PR((char *, ...));
#ifdef	FOUND_SIZE_T
extern	size_t	strlcatl __PR((char *, size_t, ...));
#endif
extern	int	streql __PR((const char *, const char *));
#ifdef	_SCHILY_WCHAR_H
extern	wchar_t	*wcscatl __PR((wchar_t *, ...));
#ifdef	FOUND_SIZE_T
extern	size_t	wcslcatl __PR((wchar_t *, size_t, ...));
#endif
extern	int	wcseql __PR((const wchar_t *, const wchar_t *));
#endif
#ifdef	va_arg
extern	int	format __PR((void (*)(char, long), long, const char *,
							va_list));
extern	int	fprformat __PR((long, const char *, va_list));
#else
extern	int	format __PR((void (*)(char, long), long, const char *, void *));
extern	int	fprformat __PR((long, const char *, void *));
#endif

extern	int	ftoes __PR((char *, double, int, int));
extern	int	ftofs __PR((char *, double, int, int));
#ifdef	HAVE_LONGDOUBLE
extern	int	qftoes __PR((char *, long double, int, int));
extern	int	qftofs __PR((char *, long double, int, int));
#endif

/*PRINTFLIKE1*/
extern	int	js_error __PR((const char *, ...)) __printflike__(1, 2);
/*PRINTFLIKE2*/
extern	int	js_dprintf	__PR((int, const char *, ...))
							__printflike__(2, 3);
#ifdef	EOF	/* stdio.h has been included */
/*PRINTFLIKE2*/
extern	int	js_fprintf	__PR((FILE *, const char *, ...))
							__printflike__(2, 3);
#endif	/* EOF */
/*PRINTFLIKE1*/
extern	int	js_printf	__PR((const char *, ...)) __printflike__(1, 2);
#ifdef	FOUND_SIZE_T
/*PRINTFLIKE3*/
extern	int	js_snprintf	__PR((char *, size_t, const char *, ...))
							__printflike__(3, 4);
#endif
/*PRINTFLIKE2*/
extern	int	js_sprintf	__PR((char *, const char *, ...))
							__printflike__(2, 3);

#ifdef	FOUND_SIZE_T
extern	void	swabbytes	__PR((void *, ssize_t));
#endif
extern	char	**getmainfp	__PR((void));
extern	char	**getavp	__PR((void));
extern	char	*getav0		__PR((void));
extern	void	**getfp		__PR((void));
extern	int	flush_reg_windows __PR((int));
#ifdef	FOUND_SIZE_T
extern	ssize_t	cmpbytes	__PR((const void *, const void *, ssize_t));
extern	int	cmpmbytes	__PR((const void *, const void *, ssize_t));
extern	ssize_t	cmpnullbytes	__PR((const void *, ssize_t));
#endif

#ifdef	nonono
#if	defined(HAVE_LARGEFILES)
/*
 * To allow this, we need to figure out how to do autoconfiguration for off64_t
 */
extern	FILE	*_fcons64	__PR((FILE *, int, int));
extern	FILE	*fdup64		__PR((FILE *));
extern	FILE	*fileluopen64	__PR((int, const char *));
extern	FILE	*fileopen64	__PR((const char *, const char *));
#ifdef	FOUND_OFF_T
extern	off64_t	filepos64	__PR((FILE *));
#endif
extern	FILE	*filereopen64	__PR((const char *, const char *, FILE *));
#ifdef	FOUND_OFF_T
extern	int	fileseek64	__PR((FILE *, off64_t));
extern	off64_t	filesize64	__PR((FILE *));
#endif
#ifdef	S_IFMT
extern	int	filestat64	__PR((FILE *, struct stat *));
#endif
extern	int	_openfd64	__PR((const char *, int));
#endif
#endif

#ifndef	NO_SCHILY_PRINT		/* Define to disable *printf() redirects */
#ifdef	SCHILY_PRINT
#ifdef	__never__
#undef	error
#define	error		js_error
#endif	/* __never__ */
#undef	dprintf
#define	dprintf		js_dprintf
#undef	fprintf
#define	fprintf		js_fprintf
#undef	printf
#define	printf		js_printf
#undef	snprintf
#define	snprintf	js_snprintf
#undef	sprintf
#define	sprintf		js_sprintf
#else
#ifndef	HAVE_SNPRINTF
#undef	snprintf
#define	snprintf	js_snprintf
#endif	/* HAVE_SNPRINTF */
#endif	/* SCHILY_PRINT */
#endif	/* NO_SCHILY_PRINT */

#ifndef	NO_SCHILY_GETLINE	/* Define to disable *getline() redirect */
#undef	getline
#define	getline		js_getline
#undef	fgetline
#define	fgetline	js_fgetline
#endif

#ifndef	NO_SCHILY_FEXEC		/* Define to disable fexec*() redirect */
#undef	fexecl
#define	fexecl		js_fexecl
#undef	fexecle
#define	fexecle		js_fexecle
#undef	fexecv
#define	fexecv		js_fexecv
#undef	fexecve
#define	fexecve		js_fexecve
#endif

#ifndef	NO_SCHILY_FSPAWN	/* Define to disable fspawn*() redirect */
#undef	fspawnv
#define	fspawnv		js_fspawnv
#undef	fspawnv_nowait
#define	fspawnv_nowait	js_fspawnv_nowait
#undef	fspawnl
#define	fspawnl		js_fspawnl
#endif

extern	int	js_mexval	__PR((int exval));
#ifdef	FOUND_SIZE_T
extern	void	*js_malloc	__PR((size_t size, char *msg));
extern	void	*js_realloc	__PR((void *ptr, size_t size, char *msg));
#endif
extern	char	*js_savestr	__PR((const char *s));

#ifdef	_SCHILY_JMPDEFS_H

/*
 * Special values for the "jmp" parameter.
 *
 * Control how the siglongjmp() should be handled:
 */
#define	JM_EXIT		((sigjmps_t *)-1) /* Call comexit(errno) instead */
#define	JM_RETURN	((sigjmps_t *)0)  /* Return instead		 */

extern	int	js_jmexval	__PR((int exval));
#ifdef	FOUND_SIZE_T
extern	void	*js_jmalloc	__PR((size_t size, char *msg, sigjmps_t *jmp));
extern	void	*js_jrealloc	__PR((void *ptr, size_t size, char *msg,
							sigjmps_t *jmp));
#endif
extern	char	*js_jsavestr	__PR((const char *s, sigjmps_t *jmp));

extern	int	js_fjmexval	__PR((int exval));
#ifdef	EOF	/* stdio.h has been included */
#ifdef	FOUND_SIZE_T
extern	void	*js_fjmalloc	__PR((FILE *f, size_t size, char *msg,
						sigjmps_t *jmp));
extern	void	*js_fjrealloc	__PR((FILE *f, void *ptr, size_t size,
						char *msg, sigjmps_t *jmp));
#endif
extern	char	*js_fjsavestr	__PR((FILE *f, const char *s, sigjmps_t *jmp));
#endif	/* EOF */
#endif	/* _SCHILY_JMPDEFS_H */

#define	___mexval	js_mexval
#define	___malloc	js_malloc
#define	___realloc	js_realloc
#define	___savestr	js_savestr
#define	__jmexval	js_jmexval
#define	__jmalloc	js_jmalloc
#define	__jrealloc	js_jrealloc
#define	__jsavestr	js_jsavestr
#define	__fjmalloc	js_fjmalloc
#define	__fjmexval	js_fjmexval
#define	__fjrealloc	js_fjrealloc
#define	__fjsavestr	js_fjsavestr

#ifdef	__cplusplus
}
#endif

#if defined(_JOS) || defined(JOS)
#	ifndef	_SCHILY_JOS_IO_H
#	include <schily/jos_io.h>
#	endif
#endif

#if !defined(_SCHILY_LIBPORT_H) && !defined(NO_LIBPORT_H)
#include <schily/libport.h>
#endif
#if !defined(_SCHILY_HOSTNAME_H) && defined(USE_HOSTNAME_H)
#include <schily/hostname.h>
#endif

#endif	/* _SCHILY_SCHILY_H */
